/*
 * DesignWare Core Mobile Storage Host
 *
 * Copyright (c) 2011-2021 NationalChip. All rights reserved.
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
#include "qemu/log.h"
#include "sysemu/block-backend.h"
#include "sysemu/blockdev.h"
#include "hw/sysbus.h"
//#include "hw/sd/sd.h"
#include "hw/sd/sdcard_legacy.h"
#include "hw/irq.h"
#include "hw/qdev-properties.h"
#include "migration/vmstate.h"
#include "qapi/error.h"

#include "dwc_sdhc.h"

#define LOG_TAG "[TREC.SDHC] "

// update interrupt status and raise interrupt
static void _dwc_sdhc_interrupts_update(DwcSdhcState *s)
{
    qemu_set_irq(s->irq, (s->regs.rintsts.value & s->regs.intmask.value) && s->regs.ctrl.int_enable);
}

// update certain bits related to fifo status
static void _dwc_sdhc_fifolevel_update(DwcSdhcState *s)
{
    int fifocnt = s->regs.status.fifo_count;
    int fiforxwm = s->regs.fifoth.rx_wmark;
    int fifotxwm = s->regs.fifoth.tx_wmark;
    int direct = s->regs.cmd.read_or_write;

    if (s->regs.ctrl.fifo_reset) {          /* reset fifo */
        s->Fifo_start = 0;
        s->regs.status.fifo_count = 0;
        s->regs.status.fifo_tx_watermark = 1;
        s->regs.status.fifo_empty = 1;
        s->regs.status.fifo_rx_watermark = 0;
        s->regs.status.fifo_full = 0;

        s->regs.rintsts.transmit_fifo_data_request = 1;
        _dwc_sdhc_interrupts_update(s);
        return;
    }

    s->regs.status.fifo_empty = fifocnt ? 0 : 1;
    s->regs.status.fifo_full = fifocnt == DWC_SDHC_FIFO_DEPTH ? 1 : 0;
    if (fifocnt > fiforxwm && !direct ) {
        s->regs.rintsts.receive_fifo_data_request = 1;
    }
    if (fifocnt <= fifotxwm && direct ) {
        s->regs.rintsts.transmit_fifo_data_request = 1;
    }
    _dwc_sdhc_interrupts_update(s);

    s->regs.status.fifo_rx_watermark = fifocnt > fiforxwm ? 1 : 0;
    s->regs.status.fifo_tx_watermark = fifocnt <= fifotxwm ? 1 : 0;
}

// transfer data between sdhc and SD card
static void _dwc_sdhc_transfer(DwcSdhcState *s)
{
    uint8_t value[4];
    uint32_t value1;
    uint32_t fifocnt = s->regs.status.fifo_count;
    uint32_t write = s->regs.cmd.read_or_write;
    if (!s->regs.cmd.data_expected) {           /* return if no data expected */
        return;
    }
    if (!write) {         /* read a date from sd card */
#ifdef HW_SDCARD_LEGACY_H
        while ((fifocnt < DWC_SDHC_FIFO_DEPTH) && (s->regs.bytcnt > 0) && sd_data_ready(s->card)) {
            value[0] = sd_read_byte(s->card);
            value[1] = sd_read_byte(s->card);
            value[2] = sd_read_byte(s->card);
            value[3] = sd_read_byte(s->card);
#else
        while ((fifocnt < DWC_SDHC_FIFO_DEPTH) && (s->regs.bytcnt > 0) && sdbus_data_ready(&s->sdbus)) {
            sdbus_read_data(&s->sdbus, value, 4);
#endif
            s->Fifo[(s->Fifo_start + fifocnt) % DWC_SDHC_FIFO_DEPTH] =
                (value[3] << 24) | (value[2] << 16) | (value[1] << 8) |
                value[0];
            fifocnt++;
            s->regs.bytcnt -= 4;
        }
    } else {                            /* write date to SD Card */
        while ((fifocnt > 0) && (s->regs.bytcnt > 0)) {
            value1 = s->Fifo[s->Fifo_start];
            s->Fifo_start = (s->Fifo_start + 1) % DWC_SDHC_FIFO_DEPTH;
#ifdef HW_SDCARD_LEGACY_H
            sd_write_byte(s->card, value1);
            sd_write_byte(s->card, value1 >> 8);
            sd_write_byte(s->card, value1 >> 16);
            sd_write_byte(s->card, value1 >> 24);
#else
            sdbus_write_data(&s->sdbus, &value1, sizeof(value1));
#endif
            fifocnt--;
            s->regs.bytcnt -= 4;
        }
    }

    if (s->regs.bytcnt == 0) {                  /* data transfer over */
        s->regs.rintsts.data_transfer_over = 1;
        _dwc_sdhc_interrupts_update(s);
    }

    s->regs.status.fifo_count = fifocnt;
}

#define DWC_SDHC_IDMA_BURST_SIZE 16
static int _dwc_sdhc_idma_transfer(DwcSdhcState *s, uint32_t dma_addr, uint32_t dma_size)
{
    uint32_t count = 0;
    uint8_t buffer[DWC_SDHC_IDMA_BURST_SIZE];
    if (s->regs.cmd.read_or_write) {
        // memory => SD card
        s->regs.idsts.fsm = DWC_SDHC_IDSTS_FSM_DMA_WR;
        while ((count < dma_size) && (s->regs.bytcnt > 0)) {
            uint32_t addr = dma_addr + count;
            uint32_t size = dma_size - count;
            if (size > s->regs.bytcnt) size = s->regs.bytcnt;
            if (size > DWC_SDHC_IDMA_BURST_SIZE) size = DWC_SDHC_IDMA_BURST_SIZE;

            // Read Data from Memory
            if (address_space_read(&s->dma_as, addr, MEMTXATTRS_UNSPECIFIED, buffer, size) != MEMTX_OK) {
                printf(LOG_TAG" IDMA failed to read data from memory address %08X size %d!\n", addr, size);
                return -2;
            }

            // Write Data to SD Card
#ifdef HW_SDCARD_LEGACY_H
            for (int i = 0; i < size; i++) {
                sd_write_byte(s->card, buffer[i]);
            }
#else
            sdbus_write_data(&s->sdbus, buffer, size);
#endif

            count += size;
            s->regs.bytcnt -= size;
        }

    }
    else {
        // SD card => memory
        s->regs.idsts.fsm = DWC_SDHC_IDSTS_FSM_DMA_RD;
#ifdef HW_SDCARD_LEGACY_H
        while ((count < dma_size) && (s->regs.bytcnt > 0) && sd_data_ready(s->card)) {
#else
        while ((count < dma_size) && (s->regs.bytcnt > 0) && sdbus_data_ready(&s->sdbus)) {
#endif
            uint32_t addr = dma_addr + count;
            uint32_t size = dma_size - count;
            if (size > s->regs.bytcnt) size = s->regs.bytcnt;
            if (size > DWC_SDHC_IDMA_BURST_SIZE) size = DWC_SDHC_IDMA_BURST_SIZE;

            // Read Data from SD Card
#ifdef HW_SDCARD_LEGACY_H
            for (int i = 0; i < size; i++) {
                buffer[i] = sd_read_byte(s->card);
            }
#else
            sdbus_read_data(&s->sdbus, buffer, size);
#endif

            // Write Data to Memory
            if (address_space_write(&s->dma_as, addr, MEMTXATTRS_UNSPECIFIED, buffer, size) != MEMTX_OK) {
                printf(LOG_TAG" IDMA failed to write data to memory address %08X size %d!\n", addr, size);
                return -1;
            }
            count += size;
            s->regs.bytcnt -= size;
        }
    }
    return count;
}

static void _dwc_sdhc_idma_process(void *opaque)
{
    DwcSdhcState *s = (DwcSdhcState *)opaque;
    if (!s->regs.cmd.data_expected) {           /* return if no data expected */
        return;
    }

    DWC_SDHC_IDMA_DESCRIPTOR desc;
    s->regs.dscaddr = s->regs.dbaddr;
    while (1) {
        // First Read a Description
        s->regs.idsts.fsm = DWC_SDHC_IDSTS_FSM_DESC_RD;
        if (address_space_read(&s->dma_as, s->regs.dscaddr, MEMTXATTRS_UNSPECIFIED, (uint8_t *)&desc, sizeof(desc)) != MEMTX_OK) {
            printf(LOG_TAG" IDMA failed to read a descriptor from address %08X!\n", s->regs.dscaddr);
            // TODO: Setup IDSTS
            return;
        }

        // Check the Descriptor
        s->regs.idsts.fsm = DWC_SDHC_IDSTS_FSM_DESC_CHK;
        if (desc.des0.own == 0) {
            printf(LOG_TAG" IDMA desc[%08X].OWN bit is 0\n", s->regs.dscaddr);
            s->regs.idsts.fsm = DWC_SDHC_IDSTS_FSM_DMA_SUSPEND;
            return;
        }

        // printf("desc.own = %d\n", desc.des0.own);
        // printf("desc.ces = %d\n", desc.des0.ces);
        // printf("desc.er  = %d\n", desc.des0.er);
        // printf("desc.ch  = %d\n", desc.des0.ch);
        // printf("desc.fs  = %d\n", desc.des0.fs);
        // printf("desc.ld  = %d\n", desc.des0.ld);
        // printf("desc.dic = %d\n", desc.des0.dic);

        // printf("desc.bs1 = %d\n", desc.des1.bs1);
        // printf("desc.bs2 = %d\n", desc.des1.bs2);

        // printf("desc.bap1= %08x\n", desc.des2);
        // printf("desc.next= %08x\n", desc.des3);

        if (desc.des1.bs1 == 0) {
            printf(LOG_TAG" IDMA desc[%08X].BS1 shouldn't be zero!\n", s->regs.dscaddr);
            return;
        }
        // Do the Transfer Buffer 1
        int32_t result = _dwc_sdhc_idma_transfer(s, desc.des2, desc.des1.bs1);
        if (result < 0) {
            return;
        }

        if (desc.des0.ch) {
            // Chained
            if (desc.des1.bs2) {
                printf(LOG_TAG" IDMA desc[%08X].BS2 should be zero if it is chaned mode!\n", s->regs.dscaddr);
            }
            if (desc.des0.ld) {
                break;
            }
            s->regs.dscaddr = desc.des3;
        }
        else {
            // Dual Buffer
            if (desc.des1.bs2) {
                int32_t result = _dwc_sdhc_idma_transfer(s, desc.des3, desc.des1.bs2);
                if (result < 0) {
                    return;
                }
            }
            if (desc.des0.ld) {
                break;
            }
            if (desc.des0.er) {
                s->regs.dscaddr = s->regs.dbaddr;
                continue;
            }
            s->regs.dscaddr += s->regs.bmod.dsl;
        }
    }

    if (s->regs.cmd.send_auto_stop) {       // sending CMD12
        SDRequest request;
        uint8_t response[16];
        request.cmd = 12;                   // fetch command index
        sd_do_command(s->card, &request, response);
    }

    if (!desc.des0.dic) {
        if (s->regs.cmd.read_or_write) {
            s->regs.idsts.ti = 1;
        }
        else {
            s->regs.idsts.ri = 1;
        }
    }
}

// sending commmanfs to sd card
static void _dwc_sdhc_command(DwcSdhcState *s)
{
    SDRequest request;
    request.cmd = s->regs.cmd.cmd_index;                    // fetch command index
    request.arg = s->regs.cmdarg;
    if (s->regs.cmd.update_clock_registers_only) {          // do not send command
        s->regs.cmd.start_cmd = 0;                          // clear start_cmd bit
        return;
    }

    uint8_t response[16];
    int rlen = sd_do_command(s->card, &request, response);
    s->regs.cmd.start_cmd = 0;

    if (s->regs.cmd.response_expect) {                      // response expected
        if (rlen == 4 && !(s->regs.cmd.response_length)) {  // SHORT RESPONSE
            // bit 0 in Resp correspond to LSB
            s->regs.resp[0] = (response[ 0] << 24) | (response[ 1] << 16) | (response[ 2] << 8) | response[ 3];
            s->regs.resp[1] = s->regs.resp[2] = s->regs.resp[3] = 0;
        }
        else if (rlen == 16 && s->regs.cmd.response_length) {   // LONG RESPONSE
            s->regs.resp[3] = (response[ 0] << 24) | (response[ 1] << 16) | (response[ 2] << 8) | response[ 3];
            s->regs.resp[2] = (response[ 4] << 24) | (response[ 5] << 16) | (response[ 6] << 8) | response[ 7];
            s->regs.resp[1] = (response[ 8] << 24) | (response[ 9] << 16) | (response[10] << 8) | response[11];
            s->regs.resp[0] = (response[12] << 24) | (response[13] << 16) | (response[14] << 8) | response[15];
        }
        else {
            s->regs.rintsts.response_timeout = 1;   // RESPONSE ERROR
        }
    }

    if (s->regs.cmd.send_auto_stop && !s->regs.ctrl.use_internal_dmac) {       // sending CMD12 if no using dmac
        request.cmd = 12;                   // fetch command index
        sd_do_command(s->card, &request, response);
    }
    s->regs.rintsts.command_done = 1;       // set command done bit
    _dwc_sdhc_interrupts_update(s);
}

static void _dwc_sdhc_update(DwcSdhcState *s)
{
    _dwc_sdhc_transfer(s);
    _dwc_sdhc_fifolevel_update(s);
}

static uint64_t _dwc_sdhc_read(void *opaque, hwaddr offset, unsigned size)
{
    DwcSdhcState *s = (DwcSdhcState *) opaque;
    // printf(LOG_TAG"%s(offset=0x%08lX, size=%d);\n", __FUNCTION__, offset, size);
    if (size != 4) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "_dwc_sdhc_read: 0x%x must word align read\n",
                      (int)offset);
    }

    if (offset >= DWC_SDHC_REG_FIFO_BASE) {
        int fifocnt = s->regs.status.fifo_count;
        if (s->regs.status.fifo_empty) {
            s->regs.rintsts.fifo_underrun_error = 1;
            qemu_log_mask(LOG_GUEST_ERROR,
                            "MMC: FIFO underrun\n");
            return 0;
        }
        uint32_t value = s->Fifo[s->Fifo_start];
        s->Fifo_start = (s->Fifo_start + 1) % DWC_SDHC_FIFO_DEPTH;
        fifocnt--;
        s->regs.status.fifo_count = fifocnt;
        _dwc_sdhc_update(s);
        return value;
    }

    if (offset == DWC_SDHC_REG_MINTSTS) {
        return s->regs.rintsts.value & s->regs.intmask.value;
    }

    if (offset < sizeof(s->regs)) {
        uint32_t *regs = (uint32_t *)&s->regs;
        return regs[offset / 4];
    }

    qemu_log_mask(LOG_GUEST_ERROR, "%s: Bad register %x\n", __func__, (int)offset);
    return 0;
}

static void _dwc_sdhc_write(void *opaque, hwaddr offset, uint64_t value, unsigned size)
{
    DwcSdhcState *s = (DwcSdhcState *) opaque;
    // printf(LOG_TAG"%s(offset=0x%08lX, value=0x%08lx, size=%d);\n", __FUNCTION__, offset, value, size);
    if (size != 4) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "_dwc_sdhc_write: 0x%x must word align read\n",
                      (int)offset);
    }

    if (offset >= DWC_SDHC_REG_FIFO_BASE) {
        int fifocnt = s->regs.status.fifo_count;
        if (s->regs.status.fifo_full) {                 /* FIFO is full */
            s->regs.rintsts.fifo_underrun_error = 1;
            _dwc_sdhc_interrupts_update(s);
            qemu_log_mask(LOG_GUEST_ERROR, "MMC: FIFO overrun\n");
            return;
        }
        s->Fifo[(fifocnt + s->Fifo_start) % DWC_SDHC_FIFO_DEPTH] = value;
        fifocnt++;
        /* new filled location in FIFO */
        s->regs.status.fifo_count = fifocnt;
        _dwc_sdhc_update(s);
        return;
    }

    if (offset >= sizeof(s->regs)) {
        qemu_log_mask(LOG_GUEST_ERROR, "%s: Bad register %x\n", __func__, (int)offset);
        return;
    }

    // offset < 0x9c
    switch(offset) {
    case DWC_SDHC_REG_CTRL:
        s->regs.ctrl.value = value;
        _dwc_sdhc_update(s);
        s->regs.ctrl.controller_reset = 0;
        s->regs.ctrl.fifo_reset = 0;
        break;
    case DWC_SDHC_REG_PWREN:
        s->regs.pwren.value = value;
        sd_enable(s->card, (s->regs.pwren.power_enable & 1));      /* enable sd card */
        break;
    case DWC_SDHC_REG_INTMASK:
        s->regs.intmask.value = value;
        _dwc_sdhc_interrupts_update(s);
        break;
    case DWC_SDHC_REG_CMD:                                        /* processing command */
        s->regs.cmd.value = value;
        memset(&s->regs.resp, 0, sizeof(s->regs.resp));
        _dwc_sdhc_command(s);
        if (s->regs.ctrl.use_internal_dmac) {
            timer_mod(s->dma_timer, qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL));
        }
        else {
            _dwc_sdhc_update(s);
        }
        break;
    case DWC_SDHC_REG_RINTSTS:
        s->regs.rintsts.value &= ~value;
        // Update FIFO flags
        _dwc_sdhc_fifolevel_update(s);
        //_dwc_sdhc_interrupts_update(s);
        break;
    // IDMA
    case DWC_SDHC_REG_BMOD:
        s->regs.bmod.value = value;
        // TODO:
        break;
    case DWC_SDHC_REG_PLDMND:
        timer_mod(s->dma_timer, qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL));
        break;
    case DWC_SDHC_REG_IDSTS: {
        DWC_SDHC_IDSTS idsts;
        idsts.value = value;
        if (idsts.ti)   s->regs.idsts.ti  = 0;
        if (idsts.ri)   s->regs.idsts.ri  = 0;
        if (idsts.fbe)  s->regs.idsts.fbe = 0;
        if (idsts.du)   s->regs.idsts.du  = 0;
        if (idsts.ces)  s->regs.idsts.ces = 0;
        if (idsts.nis)  s->regs.idsts.nis = 0;
        if (idsts.ais)  s->regs.idsts.ais = 0;
        _dwc_sdhc_interrupts_update(s);
        break;
    }
    case DWC_SDHC_REG_IDINTEN:
        s->regs.idinten.value = value;
        _dwc_sdhc_interrupts_update(s);
        break;
    // Readonly Registers
    case DWC_SDHC_REG_RESP0:
    case DWC_SDHC_REG_RESP1:
    case DWC_SDHC_REG_RESP2:
    case DWC_SDHC_REG_RESP3:
    case DWC_SDHC_REG_MINTSTS:
    case DWC_SDHC_REG_STATUS:
    case DWC_SDHC_REG_CDETECT:
    case DWC_SDHC_REG_WRTPRT:
    case DWC_SDHC_REG_TCBCNT:
    case DWC_SDHC_REG_TBBCNT:
    case DWC_SDHC_REG_VERID:
    case DWC_SDHC_REG_HCON:
    case DWC_SDHC_REG_DSCADDR:          // DSCADDR(R)
    case DWC_SDHC_REG_BUFADDR:          // BUFADDR(R)
        printf(LOG_TAG" Write to readonly offset 0x%08lX\n", offset);
        break;
    // Default Behavior
    default: {
        uint32_t *regs = (uint32_t *)&s->regs;
        regs[offset / 4] = value;
        }
    }
}

static const MemoryRegionOps _dwc_sdhc_ops = {
    .read = _dwc_sdhc_read,
    .write = _dwc_sdhc_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void _dwc_sdhc_init(Object *obj)
{
    DwcSdhcState *s = DWC_SDHC(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    memory_region_init_io(&s->iomem, obj, &_dwc_sdhc_ops, s,
                          TYPE_DWC_SDHC, 0x1000);
    sysbus_init_mmio(sbd, &s->iomem);
    sysbus_init_irq(sbd, &s->irq);

#ifndef HW_SDCARD_LEGACY_H
    qbus_create_inplace(&s->sdbus, sizeof(s->sdbus),
                        TYPE_SD_BUS, dev, "sd-bus");
#endif
}

static void _dwc_sdhc_realize(DeviceState *dev, Error **errp)
{
    DwcSdhcState *s = DWC_SDHC(dev);

    // Check DMA Memory Region Properity
    if (!s->dma_mr) {
        error_setg(errp, LOG_TAG"'dma-mr' link not set");
        return;
    }
    address_space_init(&s->dma_as, s->dma_mr, "sdhc idma");
    s->dma_timer = timer_new_ns(QEMU_CLOCK_VIRTUAL, _dwc_sdhc_idma_process, s);

#ifdef HW_SDCARD_LEGACY_H
    /* FIXME use a qdev drive property instead of drive_get_next() */
    DriveInfo *dinfo = drive_get_next(IF_SD);
    s->card = sd_init(dinfo ? blk_by_legacy_dinfo(dinfo) : NULL, false);
    if (s->card == NULL) {
        error_setg(errp, "sd_init failed");
    }
#endif
}

static void _dwc_sdhc_reset(DeviceState *dev)
{
    DwcSdhcState *s = DWC_SDHC(dev);
    memset(&s->regs, 0, sizeof(s->regs));
    s->regs.tmout.value     = 0xffffff40;
    s->regs.blksiz.value    = 0x200;
    s->regs.bytcnt          = 0x200;
    s->regs.status.value    = 0x106;        /* need to be consider */
    //s->regs.fifoth.tx_wmark = 0x000f0000;
    s->regs.fifoth.rx_wmark = DWC_SDHC_FIFO_DEPTH - 1;
    s->regs.cdetect.value   = 0xfffffffe;
    s->regs.debnce.value    = 0x00ffffff;
    s->regs.usrid           = 0x0;
    s->regs.verid           = 0x5432270a;
    s->regs.hcon.value      = 0x792cc3;
    s->regs.rst_n.value     = 0xFFFF;

    memset(&s->ext_regs, 0, sizeof(s->ext_regs));
}

static const VMStateDescription _vmstate_dwc_sdhc = {
    .name = TYPE_DWC_SDHC,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_END_OF_LIST()
    }
};

static Property _dwc_sdhc_properties[] = {
    DEFINE_PROP_LINK("dma-mr", DwcSdhcState, dma_mr, TYPE_MEMORY_REGION, MemoryRegion *),
    DEFINE_PROP_END_OF_LIST(),
};

static void _dwc_sdhc_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->vmsd = &_vmstate_dwc_sdhc;
    dc->reset = _dwc_sdhc_reset;
    dc->realize = _dwc_sdhc_realize;
    device_class_set_props(dc, _dwc_sdhc_properties);
    dc->desc = "Designware Core SDHC";
}

static const TypeInfo _dwc_sdhc_info = {
    .name          = TYPE_DWC_SDHC,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(DwcSdhcState),
    .instance_init = _dwc_sdhc_init,
    .class_init    = _dwc_sdhc_class_init,
};

static void _dwc_sdhc_register_types(void)
{
    type_register_static(&_dwc_sdhc_info);
}

type_init(_dwc_sdhc_register_types)
