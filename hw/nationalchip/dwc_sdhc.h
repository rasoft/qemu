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

#ifndef _DWC_SDHC_H_
#define _DWC_SDHC_H_

#include "qemu/osdep.h"
#include "sysemu/block-backend.h"
#include "sysemu/blockdev.h"
#include "hw/sysbus.h"
// #include "hw/sd/sd.h"
#include "hw/sd/sdcard_legacy.h"

#include "dwc_sdhc_regs.h"

#define DWC_SDHC_FIFO_DEPTH    0x80         /* FIFO depth */

typedef struct {
    SysBusDevice parent_obj;

    MemoryRegion iomem;
    qemu_irq irq;

    // Module Properities
    MemoryRegion *dma_mr;

    // Internal State
#ifdef HW_SDCARD_LEGACY_H
    BlockDriverState *bdrv;
    SDState *card;
#else
    SDBus sdbus;
#endif

    // for IMDA
    AddressSpace dma_as;
    QEMUTimer *dma_timer;

    // for Registers
    DWC_SDHC_REGS       regs;
    DWC_SDHC_EXT_REGS   ext_regs;

    uint32_t Fifo[DWC_SDHC_FIFO_DEPTH];
    uint8_t  Fifo_start;

} DwcSdhcState;

#define TYPE_DWC_SDHC "dwc_sdhc"
#define DWC_SDHC(obj) OBJECT_CHECK(DwcSdhcState, (obj), TYPE_DWC_SDHC)

#endif /* _DWC_SDHC_H_ */
