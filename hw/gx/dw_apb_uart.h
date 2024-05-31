/*
 * DW APB UART
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
#ifndef _DW_APB_UART_H_
#define _DW_APB_UART_H_

#include "chardev/char-fe.h"

typedef struct DwApbUartState {
    SysBusDevice parent_obj;

    MemoryRegion    iomem;
    CharBackend     chr;

    qemu_irq irq;
    qemu_irq rx_dma;
    qemu_irq tx_dma;

    uint32_t dll;   /* 0x00 Divisor Latch Low */
    uint32_t dlh;   /* 0x04 LCR[7] = 1 Divisor Latch High */
    uint32_t ier;   /* 0x04 LCR[7] = 0 Interrupt Enable Register */
    uint32_t iir;   /* 0x08 R Interrupt Identity Register */
    uint32_t fcr;   /* 0x08 W FIFO control register */
    uint32_t lcr;   /* 0x0C line control register */
    uint32_t mcr;   /* 0x10 modem control register */
    uint32_t lsr;   /* 0x14 line status register */
    uint32_t msr;   /* 0x18 modem status register */
    uint32_t usr;   /* 0x7C uart status register */

    uint32_t rx_fifo[16];
    int rx_pos;
    int rx_count;
    int rx_trigger;

} DwApbUartState;

#define TYPE_DW_APB_UART  "dw_apb_uart"
#define DW_APB_UART(obj)  OBJECT_CHECK(DwApbUartState, (obj), TYPE_DW_APB_UART)

static inline DeviceState *dw_apb_uart_create(hwaddr addr,
                                              qemu_irq irq,
                                              Chardev *chr)
{
    DeviceState *dev = qdev_new(TYPE_DW_APB_UART);
    SysBusDevice *s = SYS_BUS_DEVICE(dev);

    qdev_prop_set_chr(dev, "chardev", chr);
    sysbus_realize(s, NULL);
    sysbus_mmio_map(s, 0, addr);
    sysbus_connect_irq(s, 0, irq);

    return dev;
}

#endif /* _DW_APB_UART_H_ */
