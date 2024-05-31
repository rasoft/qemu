/*
 * Designware AHB DMA Controller
 * Alan.REN
 *
 * Copyright 2021 NationalChip
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _DW_AHB_DMAC_H_
#define _DW_AHB_DMAC_H_

#include <stdint.h>

#include "hw/hw.h"
#include "hw/irq.h"
#include "hw/sysbus.h"
#include "hw/ptimer.h"

#include "dw_ahb_dmac_regs.h"

typedef struct DwAhbDmacChannelState {
    DW_AHB_DMAC_CHN_SAR     sar;           // Source Address
    DW_AHB_DMAC_CHN_DAR     dar;           // Destination Address
    DW_AHB_DMAC_CHN_LLP     llp;
    DW_AHB_DMAC_CHN_CTL_L   ctl_l;
    DW_AHB_DMAC_CHN_CTL_H   ctl_h;
    DW_AHB_DMAC_CHN_CFG_L   cfg_l;
    DW_AHB_DMAC_CHN_CFG_H   cfg_h;
    DW_AHB_DMAC_CHN_SGR     sgr;
    DW_AHB_DMAC_CHN_DSR     dsr;

    uint32_t                transfer_num;   // transfer num in current block

    uint8_t                 fifo[128];
    uint32_t                fifo_wr_pos;
    uint32_t                fifo_rd_pos;
} DwAhbDmacChannelState;

typedef struct DwAhbDmacState {
    SysBusDevice parent_obj;

    // Module I/F
    MemoryRegion iomem;
    qemu_irq irq;

    // Module Properities
    MemoryRegion *ahb_master[4];

    // Internal Status
    QEMUTimer *process_timer;
    AddressSpace dma_as[4];

    // Channel Status
    DwAhbDmacChannelState channel[8];
    int32_t handshake[16];

    // Interrupt Status
    uint32_t raw_tfr;
    uint32_t raw_block;
    uint32_t raw_src_tran;
    uint32_t raw_dst_tran;
    uint32_t raw_err;

    uint32_t mask_tfr;
    uint32_t mask_block;
    uint32_t mask_src_tran;
    uint32_t mask_dst_tran;
    uint32_t mask_err;

    // Miscellaneous Status
    uint32_t dma_enable;
    uint32_t channel_enable;

} DwAhbDmacState;

#define TYPE_DW_AHB_DMAC "dw.ahb.dmac"
#define DW_AHB_DMAC(obj) OBJECT_CHECK(DwAhbDmacState, (obj), TYPE_DW_AHB_DMAC)

#endif /* _DW_AHB_DMAC_H_ */
