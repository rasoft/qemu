/*
 * Designware Synchronous Serial Interface
 * Alan.REN
 *
 * Copyright 2020 NationalChip
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _DWC_SSI_H_
#define _DWC_SSI_H_

#include <stdint.h>

#include "hw/hw.h"
#include "hw/irq.h"
#include "hw/sysbus.h"
#include "hw/ssi/ssi.h"
#include "qemu/fifo32.h"

#include "dwc_ssi_regs.h"

typedef struct DwcSsiState {
    SysBusDevice parent_obj;

    MemoryRegion iomem;
    qemu_irq irq;
    qemu_irq rx_dma;
    qemu_irq tx_dma;
    qemu_irq dev_cs;

    SSIBus *ssi;

    Fifo32 rx_fifo;
    Fifo32 tx_fifo;

    // DeviceState *ssi_dev;

    unsigned int enable;
    unsigned int inst_addr_sent;
    unsigned int rx_count;
    unsigned int tx_count;

    // RW Registers
    DWC_SSIC_CTRLR0             ctrlr0;             // 0x00
    DWC_SSIC_CTRLR1             ctrlr1;             // 0x04
    DWC_SSIC_SER                ser;                // 0x10
    DWC_SSIC_BAUDR              baudr;              // 0x14
    DWC_SSIC_TXFTLR             txftlr;             // 0x18
    DWC_SSIC_RXFTLR             rxftlr;             // 0x1C
    DWC_SSIC_IMR                imr;                // 0x2C
    DWC_SSIC_DMACR              dmacr;              // 0x4C
    DWC_SSIC_DMATDLR            dmatdlr;            // 0x50
    DWC_SSIC_DMARDLR            dmardlr;            // 0x54
    DWC_SSIC_RX_SAMPLE_DELAY    rx_sample_delay;    // 0xF0
    DWC_SSIC_SPI_CTRLR0         spi_ctrlr0;         // 0xF4

    // RO Registers
    // DWC_SSIC_SR                 sr;
    DWC_SSIC_RISR               risr;

} DwcSsiState;

#define TYPE_DWC_SSI "dwc.ssi"
#define DWC_SSI(obj) OBJECT_CHECK(DwcSsiState, (obj), TYPE_DWC_SSI)

#endif /* _DWC_SSI_H_ */
