/*
 * C-SKY UART emulation.
 *
 * Copyright (c) 2011-2019 C-SKY Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "qemu/osdep.h"
#include "qemu/main-loop.h"
#include "qemu/log.h"
#include "hw/sysbus.h"
#include "hw/irq.h"
#include "hw/qdev-properties.h"
#include "hw/qdev-properties-system.h"
#include "chardev/char-fe.h"
#include "sysemu/sysemu.h"
#include "migration/vmstate.h"

#include "dw_apb_uart.h"

#define FIFO_MODE   128

/* lsr:line status register */
#define lsr_TEMT    0x40
#define lsr_THRE    0x20    /* no new data has been written to the THR or TX FIFO */
#define lsr_OE      0x2     /* overruun error */
#define lsr_DR      0x1     /*
                             * at least one character in the RBR or
                             * the receiver FIFO
                             */


/* flags: USR user status register */
#define usr_RFF     0x10    /* Receive FIFO Full */
#define usr_RFNE    0x8     /* Receive FIFO not empty */
#define usr_TFE     0x4     /* transmit FIFO empty */
#define usr_TFNF    0x2     /* transmit FIFO not full */

/* interrupt type */
#define INT_NONE    0x1     /* no interrupt */
#define INT_TX      0x2     /* Transmitter holding register empty */
#define INT_RX      0x4     /* Receiver data available */

static void _dw_apb_uart_update(DwApbUartState *s)
{
    uint32_t flags = 0;

    flags = (s->iir & 0xf) == INT_TX && (s->ier & 0x2) != 0;
    flags |= (s->iir & 0xf) == INT_RX && (s->ier & 0x1) != 0;
    qemu_set_irq(s->irq, flags != 0);

    qemu_set_irq(s->tx_dma, 1);         // No FIFO, always can send data

    flags = s->lsr & lsr_DR;
    qemu_set_irq(s->rx_dma, !!flags);    // 0: has Data, 1: no Data
}

static uint64_t _dw_apb_uart_read(void *opaque, hwaddr offset, unsigned size)
{
    DwApbUartState *s = (DwApbUartState *)opaque;
    uint64_t ret = 0;

    if (size != 4) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "_dw_apb_uart_read: 0x%x must word align read\n",
                      (int)offset);
        // printf("_dw_apb_uart_read: 0x%x must word align read\n", (int)offset);
    }

    switch ((offset & 0xfff) >> 2) {
    case 0x0: /* 0x00 RBR,DLL */
        if (s->lcr & 0x80) {        // DLL
            ret = s->dll;
        } else if (s->fcr & 0x1) {  // RBR with FIFO
            s->usr &= ~usr_RFF;             // Receive FIFO is not Full
            uint32_t c = s->rx_fifo[s->rx_pos];
            if (s->rx_count > 0) {
                s->rx_count--;
                if (++s->rx_pos == 16) {
                    s->rx_pos = 0;
                }
            }
            if (s->rx_count == 0) {
                s->lsr &= ~lsr_DR;          // Data Readby Bit
                s->usr &= ~usr_RFNE;        // Receive FIFO is not Not Empty
            }
            s->iir = (s->iir & ~0xf) | INT_NONE;
            _dw_apb_uart_update(s);
            qemu_chr_fe_accept_input(&s->chr);
            ret =  c;
        } else {                    // RBR without FIFO
            s->usr &= ~usr_RFF;             // Receive FIFO is not Full
            s->usr &= ~usr_RFNE;            // Receive FIFO is not Not Empty
            s->lsr &= ~lsr_DR;              // Data Readby Bit
            s->iir = (s->iir & ~0xf) | INT_NONE;
            _dw_apb_uart_update(s);
            qemu_chr_fe_accept_input(&s->chr);
            ret =  s->rx_fifo[0];
        }
        break;
    case 0x1: /* 0x04 DLH, IER */
        if (s->lcr & 0x80) {
            ret = s->dlh;
        } else {
            ret = s->ier;
        }
        break;
    case 0x2: /* 0x08 IIR */
        ret = s->iir;
        if ((s->iir & 0xf) == INT_TX) {     // Clear THR empty interrupt
            s->iir = (s->iir & ~0xf) | INT_NONE;
            _dw_apb_uart_update(s);
        }
        break;
    case 0x3: /* 0x0C LCR */
        ret = s->lcr;
        break;
    case 0x4: /* 0x10 MCR */
        ret = s->mcr;
        break;
    case 0x5: /* 0x14 LSR */
        ret = s->lsr;
        break;
    case 0x6: /* 0x18 MSR */
        ret = s->msr;
        break;
    case 0x1f: /* 0x7C USR */
        ret = s->usr;
        break;
    case 0x3d: /* 0xF4 CPR */
        ret = 0x020002;
        break;
    case 0x3e: /* 0xF8 UCV */
        ret = 0x3230312A;
        break;
    case 0x3f: /* 0xFC CTR */
        ret = 0x44570110;
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "_dw_apb_uart_read: Bad offset %x\n", (int)offset);
        // printf("_dw_apb_uart_read: Bad offset %x\n", (int)offset);
    }

    return ret;
}

static void _dw_apb_uart_fcr_update(DwApbUartState *s)
{
    /* update rx_trigger */
    if (s->fcr & 0x1) {
        /* fifo enabled */
        switch ((s->fcr >> 6) & 0x3) {
        case 0:
            s->rx_trigger = 1;
            break;
        case 1:
            s->rx_trigger = 4;
            break;
        case 2:
            s->rx_trigger = 8;
            break;
        case 3:
            s->rx_trigger = 14;
            break;
        default:
            s->rx_trigger = 1;
            break;
        }
    } else {
        s->rx_trigger = 1;
    }

    /* reset rx_fifo */
    if (s->fcr & 0x2) {
        s->rx_pos = 0;
        s->rx_count = 0;
    }
}

static void _dw_apb_uart_write(void *opaque, hwaddr offset, uint64_t value,
                            unsigned size)
{
    DwApbUartState *s = (DwApbUartState *)opaque;

    if (size != 4) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "_dw_apb_uart_write: 0x%x must word align read\n",
                      (int)offset);
        // printf("_dw_apb_uart_write: 0x%x must word align read\n", (int)offset);
    }

    switch (offset >> 2) {
    case 0x0: /* 0x00 DLL, THR */
        if (s->lcr & 0x80) {    // DLL
            s->dll = value;
        } else {
            unsigned char ch = value;
            qemu_chr_fe_write_all(&s->chr, &ch, 1);

            s->lsr |= (lsr_THRE | lsr_TEMT);    // Transmit Holding Register is Empty,
                                                // Transmitter is EMPty
            if ((s->iir & 0xf) != INT_RX) {
                s->iir = (s->iir & ~0xf) | INT_TX;
            }
            _dw_apb_uart_update(s);
        }
        break;
    case 0x1: /* 0x04 DLH, IER */
        if (s->lcr & 0x80) {    // DLH
            s->dlh = value;
        } else {
            s->ier = value;
            s->iir = (s->iir & ~0xf) | INT_TX;
            _dw_apb_uart_update(s);
        }
        break;
    case 0x2: /* 0x08 FCR */
        if ((s->fcr & 0x1) ^ (value & 0x1)) {
            /* change fifo enable bit, reset rx_fifo */
            s->rx_pos = 0;
            s->rx_count = 0;
        }
        s->fcr = value;
        _dw_apb_uart_fcr_update(s);
        break;
    case 0x3: /* 0x0C LCR */
        s->lcr = value;
        break;
    case 0x4: /* MCR */
        s->mcr = value;
        break;
    case 0x5: /* LSR read only*/
        return;
    case 0x6: /* MSR read only*/
        return;
    case 0x1f: /* USR read only*/
        return;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "_dw_apb_uart_write: Bad offset %x\n", (int)offset);
        // printf("_dw_apb_uart_write: Bad offset %x\n", (int)offset);
    }
}

static int _dw_apb_uart_can_receive(void *opaque)
{
    /* always can receive data */
    DwApbUartState *s = (DwApbUartState *)opaque;

    if (s->fcr & 0x1) { /* fifo enabled */
        return s->rx_count < 16;
    } else {
        return s->rx_count < 1;
    }
}


static void _dw_apb_uart_receive(void *opaque, const uint8_t *buf, int size)
{
    DwApbUartState *s = (DwApbUartState *)opaque;

    if (size < 1) {
        return;
    }

    if (s->usr & usr_RFF) {
        s->lsr |= lsr_OE;  /* overrun error */
    }

    if (!(s->fcr & 0x1)) { /* none fifo mode */
        s->rx_fifo[0] = *buf;
        s->usr |= usr_RFF;
        s->usr |= usr_RFNE;
        s->iir = (s->iir & ~0xf) | INT_RX;
        s->lsr |= lsr_DR;
        _dw_apb_uart_update(s);
        return;
    }

    /* fifo mode */
    int slot = s->rx_pos + s->rx_count;
    if (slot >= 16) {
        slot -= 16;
    }
    s->rx_fifo[slot] = *buf;
    s->rx_count++;
    s->lsr |= lsr_DR;
    s->usr |= usr_RFNE;     /* receive fifo not empty */
    if (s->rx_count == 16) {
        s->usr |= usr_RFF;    /* receive fifo full */
    }
    s->iir = (s->iir & ~0xf) | INT_RX;
    _dw_apb_uart_update(s);
    return;
}

static void _dw_apb_uart_event(void *opaque, QEMUChrEvent event)
{
    //printf("%s(event=%d)\n", __FUNCTION__, event);
}

static const MemoryRegionOps _dw_apb_uart_ops = {
    .read = _dw_apb_uart_read,
    .write = _dw_apb_uart_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .impl = {
        .min_access_size = 4,
        .max_access_size = 4,
    }
};

static const VMStateDescription _vmstate_dw_apb_uart = {
    .name = TYPE_DW_APB_UART,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(dll, DwApbUartState),
        VMSTATE_UINT32(dlh, DwApbUartState),
        VMSTATE_UINT32(ier, DwApbUartState),
        VMSTATE_UINT32(iir, DwApbUartState),
        VMSTATE_UINT32(fcr, DwApbUartState),
        VMSTATE_UINT32(lcr, DwApbUartState),
        VMSTATE_UINT32(mcr, DwApbUartState),
        VMSTATE_UINT32(lsr, DwApbUartState),
        VMSTATE_UINT32(msr, DwApbUartState),
        VMSTATE_UINT32(usr, DwApbUartState),
        VMSTATE_UINT32_ARRAY(rx_fifo, DwApbUartState, 16),
        VMSTATE_INT32(rx_pos, DwApbUartState),
        VMSTATE_INT32(rx_count, DwApbUartState),
        VMSTATE_INT32(rx_trigger, DwApbUartState),
        VMSTATE_END_OF_LIST()
    }
};

static Property _dw_apb_uart_properties[] = {
    DEFINE_PROP_CHR("chardev", DwApbUartState, chr),
    DEFINE_PROP_END_OF_LIST(),
};

static void _dw_apb_uart_init(Object *obj)
{
    DwApbUartState *s = DW_APB_UART(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    memory_region_init_io(&s->iomem, OBJECT(s), &_dw_apb_uart_ops, s,
                          TYPE_DW_APB_UART, 0x1000);
    sysbus_init_mmio(sbd, &s->iomem);
    sysbus_init_irq(sbd, &s->irq);
    qdev_init_gpio_out_named(DEVICE(obj), &s->rx_dma, "rx-dma", 1);
    qdev_init_gpio_out_named(DEVICE(obj), &s->tx_dma, "tx-dma", 1);

    s->rx_trigger = 1;
    s->dlh = 0x4;
    s->iir = 0x1;
    s->lsr = 0x60;
    s->usr = 0x6;
}

static void _dw_apb_uart_realize(DeviceState *dev, Error **errp)
{
    DwApbUartState *s = DW_APB_UART(dev);

    qemu_chr_fe_set_handlers(&s->chr, _dw_apb_uart_can_receive, _dw_apb_uart_receive,
                             _dw_apb_uart_event, NULL, s, NULL, true);
}

static void _dw_apb_uart_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
    set_bit(DEVICE_CATEGORY_MISC, dc->categories);

    dc->realize = _dw_apb_uart_realize;
    dc->vmsd = &_vmstate_dw_apb_uart;
    device_class_set_props(dc, _dw_apb_uart_properties);
    dc->desc = "DesignWare APB UART";
    dc->user_creatable = true;
}

static const TypeInfo _dw_apb_uart_info = {
    .name          = TYPE_DW_APB_UART,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(DwApbUartState),
    .instance_init = _dw_apb_uart_init,
    .class_init    = _dw_apb_uart_class_init,
};


static void _dw_apb_uart_register_types(void)
{
    type_register_static(&_dw_apb_uart_info);
}

type_init(_dw_apb_uart_register_types)
