/*
 * Designware AHB DMA Controller
 * Alan.REN
 *
 * Copyright 2020 NationalChip
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
// #include "qemu-common.h"
#include "qemu/log.h"
#include "qemu/timer.h"
#include "qemu/cutils.h"
#include "qapi/error.h"
#include "hw/qdev-properties.h"
#include "migration/vmstate.h"

#include "dw_ahb_dmac.h"

#define LOG_TAG "[TREC.DMAC]"
//#define LOG_REG_DUMP
//#define LOG_DEBUG

#ifdef LOG_REG_DUMP

static void _dw_ahb_dump_channel(DwAhbDmacState *dmac, uint32_t channel_index)
{
    printf(LOG_TAG" Channel%d Information\n", channel_index);
    printf(LOG_TAG" SAR%d     = %08X\n", channel_index, dmac->channel[channel_index].sar.value);
    printf(LOG_TAG" DAR%d     = %08X\n", channel_index, dmac->channel[channel_index].dar.value);
    printf(LOG_TAG" LLP%d     = %08X\n", channel_index, dmac->channel[channel_index].llp.value);

    printf(LOG_TAG" CTL_L%d   = %08X\n", channel_index, dmac->channel[channel_index].ctl_l.value);
    printf(LOG_TAG" \tINT_EN=%d,\n", dmac->channel[channel_index].ctl_l.int_en);
    printf(LOG_TAG" \tDST_TR_WIDTH=%d, SRC_TR_WIDTH=%d,\n", dmac->channel[channel_index].ctl_l.dst_tr_width, dmac->channel[channel_index].ctl_l.src_tr_width);
    printf(LOG_TAG" \tDINC=%d, SINC=%d,\n", dmac->channel[channel_index].ctl_l.dinc, dmac->channel[channel_index].ctl_l.sinc);
    printf(LOG_TAG" \tDST_MSIZE=%d, SRC_MSIZE=%d,\n", dmac->channel[channel_index].ctl_l.dest_msize, dmac->channel[channel_index].ctl_l.src_msize);
    printf(LOG_TAG" \tSRC_GATHER_EN=%d, DST_SCATTER_EN=%d,\n", dmac->channel[channel_index].ctl_l.src_gather_en, dmac->channel[channel_index].ctl_l.dst_scatter_en);
    printf(LOG_TAG" \tTT_FC=%d, DMS=%d, SMS=%d\n", dmac->channel[channel_index].ctl_l.tt_fc, dmac->channel[channel_index].ctl_l.dms, dmac->channel[channel_index].ctl_l.sms);
    printf(LOG_TAG" \tLLP_DST_EN=%d, LLP_SRC_EN=%d,\n", dmac->channel[channel_index].ctl_l.llp_dst_en, dmac->channel[channel_index].ctl_l.llp_src_en);

    printf(LOG_TAG" CTL_H%d   = %08X\n", channel_index, dmac->channel[channel_index].ctl_h.value);
    printf(LOG_TAG" \tBLOCK_TS=%d, DONE=%d\n", dmac->channel[channel_index].ctl_h.block_ts, dmac->channel[channel_index].ctl_h.done);

    printf(LOG_TAG" CFG_L%d   = %08X\n", channel_index, dmac->channel[channel_index].cfg_l.value);
    printf(LOG_TAG" \tCH_PRIOR=%d, CH_SUSP=%d,\n", dmac->channel[channel_index].cfg_l.ch_prior, dmac->channel[channel_index].cfg_l.ch_susp);
    printf(LOG_TAG" \tHS_SEL_DST=%d, HS_SEL_SRC=%d,\n", dmac->channel[channel_index].cfg_l.hs_sel_dst, dmac->channel[channel_index].cfg_l.hs_sel_src);
    printf(LOG_TAG" \tLOCK_CH_L=%d, LOCK_B_L=%d, ", dmac->channel[channel_index].cfg_l.lock_ch_l, dmac->channel[channel_index].cfg_l.lock_b_l);
    printf(          "LOCK_CH=%d, LOCK_B=%d,\n", dmac->channel[channel_index].cfg_l.lock_ch, dmac->channel[channel_index].cfg_l.lock_b);
    printf(LOG_TAG" \tDST_HS_POL=%d, SRC_HS_POL=%d,\n", dmac->channel[channel_index].cfg_l.dst_hs_pol, dmac->channel[channel_index].cfg_l.src_hs_pol);
    printf(LOG_TAG" \tMAX_ABRST=%d,\n", dmac->channel[channel_index].cfg_l.max_abrst);
    printf(LOG_TAG" \tRELOAD_SRC=%d, RELOAD_DST=%d,\n", dmac->channel[channel_index].cfg_l.reload_src, dmac->channel[channel_index].cfg_l.reload_dst);

    printf(LOG_TAG" CFG_H%d   = %08X\n", channel_index, dmac->channel[channel_index].cfg_h.value);
    printf(LOG_TAG" \tFCMODE=%d, FIFO_MODE=%d,\n", dmac->channel[channel_index].cfg_h.fcmode, dmac->channel[channel_index].cfg_h.fifo_mode);
    printf(LOG_TAG" \tPROTCTL=%d, \n", dmac->channel[channel_index].cfg_h.protctl);
    printf(LOG_TAG" \tDS_UPD_EN=%d, SS_UPD_EN=%d\n", dmac->channel[channel_index].cfg_h.ds_upd_en, dmac->channel[channel_index].cfg_h.ss_upd_en);
    printf(LOG_TAG" \tSRC_PER=%d, DST_PER=%d\n", dmac->channel[channel_index].cfg_h.src_per, dmac->channel[channel_index].cfg_h.dst_per);

    printf(LOG_TAG" SGR%d     = %08X\n", channel_index, dmac->channel[channel_index].sgr.value);
    printf(LOG_TAG" DSR%d     = %08X\n", channel_index, dmac->channel[channel_index].dsr.value);
    printf("\n");
}

#else

#define _dw_ahb_dump_channel(x...) while(0)

#endif

//=================================================================================================

static inline uint32_t _dw_ahb_dmac_get_int_status(DwAhbDmacState *dmac)
{
    uint32_t status = 0;
    status |= ((dmac->raw_tfr      & dmac->mask_tfr     ) ? 1 : 0) << 0;
    status |= ((dmac->raw_block    & dmac->mask_block   ) ? 1 : 0) << 1;
    status |= ((dmac->raw_src_tran & dmac->mask_src_tran) ? 1 : 0) << 2;
    status |= ((dmac->raw_dst_tran & dmac->mask_dst_tran) ? 1 : 0) << 3;
    status |= ((dmac->raw_err      & dmac->mask_err     ) ? 1 : 0) << 4;
    return status;
}

static void _dw_ahb_dmac_update_irq(DwAhbDmacState *dmac)
{
    uint32_t status = _dw_ahb_dmac_get_int_status(dmac);
    qemu_set_irq(dmac->irq, status ? 1 : 0);
}

//=================================================================================================
// if need transfer, return 1, otherwise return 0
static int _dw_ahb_dmac_check_channel(DwAhbDmacState *dmac, uint32_t channel_index)
{
    DwAhbDmacChannelState *channel = &dmac->channel[channel_index];
    uint32_t channel_mask = 1 << channel_index;

    // Check suspend
    if (channel->cfg_l.ch_susp) {
        if (channel->ctl_l.int_en) {
            dmac->raw_err |= channel_mask;
            _dw_ahb_dmac_update_irq(dmac);
        }
        dmac->channel_enable &= ~channel_mask;
        return 0;
    }

    // flow control by peripheral
    if (channel->ctl_l.tt_fc & 0x4) {
        printf(LOG_TAG" Currently, NOT Suppprt Flow Control by Peripheral!\n");
        if (channel->ctl_l.int_en) {
            dmac->raw_err |= channel_mask;
            _dw_ahb_dmac_update_irq(dmac);
        }
        dmac->channel_enable &= ~channel_mask;
        return 0;
    }

    // flow control by dmac
    while (channel->transfer_num >= channel->ctl_h.block_ts) {
        channel->ctl_h.done = 1;
        // TODO: write back to memory

        // if no next llp, transfer is finished
        if (channel->llp.loc == 0) {
            if (channel->ctl_l.int_en) {
                dmac->raw_block |= channel_mask;
                dmac->raw_tfr |= channel_mask;
                _dw_ahb_dmac_update_irq(dmac);
            }
            channel->transfer_num = 0;
            dmac->channel_enable &= ~channel_mask;
            return 0;
        }

        // load next llp
        uint32_t lli_addr = channel->llp.value & 0xFFFFFFFC;
        DW_AHB_DMAC_LLI lli;
        if (address_space_read(&dmac->dma_as[channel->llp.lms], lli_addr, MEMTXATTRS_UNSPECIFIED, (uint8_t *)&lli, sizeof(lli)) != MEMTX_OK) {
            printf(LOG_TAG" Failed to load LLI from address %08X on master %d!\n", lli_addr, channel->llp.lms);
            if (channel->ctl_l.int_en) {
                dmac->raw_err |= channel_mask;
                _dw_ahb_dmac_update_irq(dmac);
            }
            dmac->channel_enable &= ~channel_mask;
            return 0;
        }

        // update channel registers
        channel->sar.value = lli.sar.value;
        channel->dar.value = lli.dar.value;
        channel->llp.value = lli.llp.value;
        channel->ctl_l.value = lli.ctl_l.value;
        channel->ctl_h.value = lli.ctl_h.value;
        channel->transfer_num = 0;
        if (channel->ctl_l.int_en) {
            dmac->raw_block |= channel_mask;
            _dw_ahb_dmac_update_irq(dmac);
        }
    }
    return 1;
}

// if src can transfer, return 1, otherwise return 0;
static int _dw_ahb_dmac_check_src(DwAhbDmacState *dmac, uint32_t channel_index)
{
    DwAhbDmacChannelState *channel = &dmac->channel[channel_index];
    uint32_t channel_mask = 1 << channel_index;

    if (channel->ctl_l.tt_fc == DW_AHB_DMAC_CHN_CTL_TT_FC_M2M_DMAC ||
        channel->ctl_l.tt_fc == DW_AHB_DMAC_CHN_CTL_TT_FC_M2P_DMAC ||
        channel->ctl_l.tt_fc == DW_AHB_DMAC_CHN_CTL_TT_FC_M2P_PERP)
        return 1;

    if (channel->cfg_l.hs_sel_src == 1) {   // Don't support Software
        printf(LOG_TAG" Currently, NOT Suppprt Source Software Handshake!\n");
        if (channel->ctl_l.int_en) {
            dmac->raw_err |= channel_mask;
            _dw_ahb_dmac_update_irq(dmac);
        }
        dmac->channel_enable &= ~channel_mask;
        return 0;
    }

    // hardware handshake
    if (dmac->handshake[channel->cfg_h.src_per])
        return 1;

    return 0;
}

// if dst can receive, return 1, otherwise return 0
static int _dw_ahb_dmac_check_dst(DwAhbDmacState *dmac, uint32_t channel_index)
{
    DwAhbDmacChannelState *channel = &dmac->channel[channel_index];
    uint32_t channel_mask = 1 << channel_index;

    if (channel->ctl_l.tt_fc == DW_AHB_DMAC_CHN_CTL_TT_FC_M2M_DMAC ||
        channel->ctl_l.tt_fc == DW_AHB_DMAC_CHN_CTL_TT_FC_P2M_DMAC ||
        channel->ctl_l.tt_fc == DW_AHB_DMAC_CHN_CTL_TT_FC_P2M_PERP)
        return 1;

    if (channel->cfg_l.hs_sel_dst == 1) {   // Don't support Software
        printf(LOG_TAG" Currently, NOT Suppprt Destination Software Handshake!\n");
        if (channel->ctl_l.int_en) {
            dmac->raw_err |= channel_mask;
            _dw_ahb_dmac_update_irq(dmac);
        }
        dmac->channel_enable &= ~channel_mask;
        return 0;
    }

    // hardware handshake
    if (dmac->handshake[channel->cfg_h.dst_per])
        return 1;

    return 0;
}

static const uint8_t _tr_width[] = {
    1, 2, 4, 8, 16, 32, 32, 32
};

/*
 * dma process
 */
static void _dw_ahb_dmac_process(void *opaque)
{
    DwAhbDmacState *dmac = (DwAhbDmacState *)opaque;

    for (int i = 0; i < 8; i++) {
        if ((dmac->channel_enable & (1 << i)) == 0)
            continue;

        _dw_ahb_dump_channel(dmac, i);

        DwAhbDmacChannelState *channel = &dmac->channel[i];
        uint32_t channel_mask = 1 << i;

        // Check whether current block is finished
        // if (_dw_ahb_dmac_check_channel(dmac, i) == 0)
        //     continue;

        // check handshake signal
        // if (_dw_ahb_dmac_check_src(dmac, i) == 0 || _dw_ahb_dmac_check_dst(dmac, i) == 0)
        //     continue;

        //printf(LOG_TAG" do DMA..., %d\n", channel->transfer_num);
        // read from source
        while (_dw_ahb_dmac_check_channel(dmac, i) && _dw_ahb_dmac_check_src(dmac, i) && _dw_ahb_dmac_check_dst(dmac, i)) {

            uint8_t data[32];
            uint8_t src_tr_width = _tr_width[channel->ctl_l.src_tr_width];
            if (address_space_read(&dmac->dma_as[channel->ctl_l.sms], channel->sar.value, MEMTXATTRS_UNSPECIFIED, data, src_tr_width) != MEMTX_OK) {
                printf(LOG_TAG" Failed to Read Data from Source Address %08X Size %d on Master %d!\n", channel->sar.value, src_tr_width, channel->ctl_l.sms);
                if (channel->ctl_l.int_en) {
                    dmac->raw_err |= channel_mask;
                    _dw_ahb_dmac_update_irq(dmac);
                }
                dmac->channel_enable &= ~channel_mask;
                continue;
            }
            switch(channel->ctl_l.sinc) {
            case 0:
                channel->sar.value += src_tr_width;
                break;
            case 1:
                channel->sar.value -= src_tr_width;
                break;
            }

            uint8_t dst_tr_width = _tr_width[channel->ctl_l.dst_tr_width];
            if (address_space_write(&dmac->dma_as[channel->ctl_l.dms], channel->dar.value, MEMTXATTRS_UNSPECIFIED, data, dst_tr_width) != MEMTX_OK) {
                printf(LOG_TAG" Failed to Write Data to Destination Address %08X Size %d on Master %d!\n", channel->dar.value, dst_tr_width, channel->ctl_l.dms);
                if (channel->ctl_l.int_en) {
                    dmac->raw_err |= channel_mask;
                    _dw_ahb_dmac_update_irq(dmac);
                }
                dmac->channel_enable &= ~channel_mask;
                continue;
            }
            switch(channel->ctl_l.dinc) {
            case 0:
                channel->dar.value += dst_tr_width;
                break;
            case 1:
                channel->dar.value -= dst_tr_width;
                break;
            }

            channel->transfer_num++;
        }
    }

    // if (dmac->channel_enable) {
    //     timer_mod(dmac->process_timer, qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) + 1000);
    // }
}

static void _dw_ahb_dmac_handshake(void *opaque, int hs, int level)
{
    DwAhbDmacState *dmac = (DwAhbDmacState *)opaque;
    dmac->handshake[hs] = level;
    if (level) {
        timer_mod(dmac->process_timer, qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL));
    }

#ifdef LOG_DEBUG
    if (hs == 8 || hs == 9) {
        printf(LOG_TAG" handshake (hs=%d, level=%d);\n", hs, level);
    }
#endif
}

//=================================================================================================

static const DW_AHB_DMAC_COMP_PARAM_CHANNEL _dw_ahb_dmac_param_channel = {
    .dtw = DW_AHB_DMAC_DMAH_CHX_DTW_NO_HARDCODE,
    .stw = DW_AHB_DMAC_DMAH_CHX_STW_NO_HARDCODE,
    .stat_dst = 0,
    .stat_src = 0,
    .dst_sca_en = 1,
    .src_gat_en = 1,
    .lock_en = 0,
    .multi_blk_en = 1,
    .ctl_wb_en = 1,
    .hc_llp = 0,
    .fc = DW_AHB_DMAC_DMAH_CHX_FC_ANY,
    .max_mult_size = DW_AHB_DMAC_DMAH_CHX_MAX_MULT_SIZE_16,
    .dms = DW_AHB_DMAC_DMAH_CHX_DMS_PROGRAMMABLE,
    .lms = DW_AHB_DMAC_DMAH_CHX_LMS_PROGRAMMABLE,
    .sms = DW_AHB_DMAC_DMAH_CHX_SMS_PROGRAMMABLE,
    .fifo_depth = DW_AHB_DMAC_DMAH_CHX_FIFO_DEPTH_128,
};

static const DW_AHB_DMAC_COMP_PARAM_2_H _dw_ahb_dmac_param_2_h = {
    .ch0_multi_blk_type = DW_AHB_DMAC_DMAH_CHX_MULTI_BLK_TYPE_PROGRAMMABLE,
    .ch1_multi_blk_type = DW_AHB_DMAC_DMAH_CHX_MULTI_BLK_TYPE_PROGRAMMABLE,
    .ch2_multi_blk_type = DW_AHB_DMAC_DMAH_CHX_MULTI_BLK_TYPE_PROGRAMMABLE,
    .ch3_multi_blk_type = DW_AHB_DMAC_DMAH_CHX_MULTI_BLK_TYPE_PROGRAMMABLE,
    .ch4_multi_blk_type = DW_AHB_DMAC_DMAH_CHX_MULTI_BLK_TYPE_PROGRAMMABLE,
    .ch5_multi_blk_type = DW_AHB_DMAC_DMAH_CHX_MULTI_BLK_TYPE_PROGRAMMABLE,
    .ch6_multi_blk_type = DW_AHB_DMAC_DMAH_CHX_MULTI_BLK_TYPE_PROGRAMMABLE,
    .ch7_multi_blk_type = DW_AHB_DMAC_DMAH_CHX_MULTI_BLK_TYPE_PROGRAMMABLE,
};

static const DW_AHB_DMAC_COMP_PARAM_1_L _dw_ahb_dmac_param_1_l = {
    .ch0_max_blk_size = DW_AHB_DMAC_DMAH_CHX_MAX_BLK_SIZE_4095,
    .ch1_max_blk_size = DW_AHB_DMAC_DMAH_CHX_MAX_BLK_SIZE_4095,
    .ch2_max_blk_size = DW_AHB_DMAC_DMAH_CHX_MAX_BLK_SIZE_4095,
    .ch3_max_blk_size = DW_AHB_DMAC_DMAH_CHX_MAX_BLK_SIZE_4095,
    .ch4_max_blk_size = DW_AHB_DMAC_DMAH_CHX_MAX_BLK_SIZE_4095,
    .ch5_max_blk_size = DW_AHB_DMAC_DMAH_CHX_MAX_BLK_SIZE_4095,
    .ch6_max_blk_size = DW_AHB_DMAC_DMAH_CHX_MAX_BLK_SIZE_4095,
    .ch7_max_blk_size = DW_AHB_DMAC_DMAH_CHX_MAX_BLK_SIZE_4095,
};

static const DW_AHB_DMAC_COMP_PARAM_1_H _dw_ahb_dmac_param_1_h = {
    .big_endian = 0,
    .intr_io = DW_AHB_DMAC_DMAH_INTR_IO_COMBINED_INT,
    .max_abrst = 1,
    .num_channels = 7,
    .num_master_int = 2,
    .s_hdata_width = DW_AHB_DMAC_DMAH_S_HDATA_WIDTH_32,
    .m4_hdata_width = DW_AHB_DMAC_DMAH_MX_HDATA_WIDTH_32,
    .m3_hdata_width = DW_AHB_DMAC_DMAH_MX_HDATA_WIDTH_32,
    .m2_hdata_width = DW_AHB_DMAC_DMAH_MX_HDATA_WIDTH_32,
    .m1_hdata_width = DW_AHB_DMAC_DMAH_MX_HDATA_WIDTH_64,
    .num_hs_int = 16,
    .add_encoded_params = 1,
    .static_endian_select = 1,
};

static uint32_t _dw_ahb_dmac_channel_read(DwAhbDmacState *dmac, hwaddr offset)
{
    uint32_t channel_index = offset / DW_AHB_DMAC_REG_CHN_STRIDE;
    uint32_t channel_offset = offset % DW_AHB_DMAC_REG_CHN_STRIDE;

    if (channel_index > _dw_ahb_dmac_param_1_h.num_channels) {
        printf(LOG_TAG" Warning: Channel Index %d > %d\n", channel_index, _dw_ahb_dmac_param_1_h.num_channels);
        return 0;
    }

    switch (channel_offset) {
    case DW_AHB_DMAC_REG_CHN_SAR(0):
        return dmac->channel[channel_index].sar.value;
    case DW_AHB_DMAC_REG_CHN_DAR(0):
        return dmac->channel[channel_index].dar.value;
    case DW_AHB_DMAC_REG_CHN_LLP(0):
        return dmac->channel[channel_index].llp.value;
    case DW_AHB_DMAC_REG_CHN_CTL_L(0):
        return dmac->channel[channel_index].ctl_l.value;
    case DW_AHB_DMAC_REG_CHN_CTL_H(0):
        if (dmac->channel_enable & (1 << channel_index)) {
            return dmac->channel[channel_index].transfer_num;
        }
        return dmac->channel[channel_index].ctl_h.value;
    case DW_AHB_DMAC_REG_CHN_SSTAT(0):
    case DW_AHB_DMAC_REG_CHN_DSTAT(0):
    case DW_AHB_DMAC_REG_CHN_SSTATAR(0):
    case DW_AHB_DMAC_REG_CHN_DSTATAR(0):
        //printf(LOG_TAG" Warning: Read Channel Registers 0x%08lX is not exist!\n", offset);
        return 0;
    case DW_AHB_DMAC_REG_CHN_CFG_L(0):
        return dmac->channel[channel_index].cfg_l.value;
    case DW_AHB_DMAC_REG_CHN_CFG_H(0):
        return dmac->channel[channel_index].cfg_h.value;
    case DW_AHB_DMAC_REG_CHN_SGR(0):
        return dmac->channel[channel_index].sgr.value;
    case DW_AHB_DMAC_REG_CHN_DSR(0):
        return dmac->channel[channel_index].dsr.value;
    default:
        //printf(LOG_TAG" Warning: Read Channel Reserved Region 0x%08lX!\n", offset);
        return 0;
    }
    return 0;
}

static uint64_t _dw_ahb_dmac_read(void *opaque, hwaddr offset, unsigned size)
{
    DwAhbDmacState *dmac = (DwAhbDmacState *)opaque;

#ifdef LOG_DEBUG
    printf(LOG_TAG" %s(offset=0x%lx, size=%d)\n", __FUNCTION__, offset, size);
#endif

    switch (offset) {
    // Channel Registers
    case DW_AHB_DMAC_REG_CHN_SAR(0) ... DW_AHB_DMAC_REG_CHN_DSR(7):
        return _dw_ahb_dmac_channel_read(dmac, offset);
    // Interrupt Status
    case DW_AHB_DMAC_REG_INT_RAW_TFR:
        return dmac->raw_tfr;
    case DW_AHB_DMAC_REG_INT_RAW_BLOCK:
        return dmac->raw_block;
    case DW_AHB_DMAC_REG_INT_RAW_SRC_TRAN:
        return dmac->raw_src_tran;
    case DW_AHB_DMAC_REG_INT_RAW_DST_TRAN:
        return dmac->raw_dst_tran;
    case DW_AHB_DMAC_REG_INT_RAW_ERR:
        return dmac->raw_err;
    case DW_AHB_DMAC_REG_INT_STATUS_TFR:
        return dmac->raw_tfr & dmac->mask_tfr;
    case DW_AHB_DMAC_REG_INT_STATUS_BLOCK:
        return dmac->raw_block & dmac->mask_block;
    case DW_AHB_DMAC_REG_INT_STATUS_SRC_TRAN:
        return dmac->raw_src_tran & dmac->mask_src_tran;
    case DW_AHB_DMAC_REG_INT_STATUS_DST_TRAN:
        return dmac->raw_dst_tran & dmac->mask_dst_tran;
    case DW_AHB_DMAC_REG_INT_STATUS_ERR:
        return dmac->raw_err & dmac->mask_err;
    case DW_AHB_DMAC_REG_INT_MASK_TFR:
        return dmac->mask_tfr;
    case DW_AHB_DMAC_REG_INT_MASK_BLOCK:
        return dmac->mask_block;
    case DW_AHB_DMAC_REG_INT_MASK_SRC_TRAN:
        return dmac->mask_src_tran;
    case DW_AHB_DMAC_REG_INT_MASK_DST_TRAN:
        return dmac->mask_dst_tran;
    case DW_AHB_DMAC_REG_INT_MASK_ERR:
        return dmac->mask_err;
    case DW_AHB_DMAC_REG_INT_CLEAR_TFR:        // WO
    case DW_AHB_DMAC_REG_INT_CLEAR_BLOCK:      // WO
    case DW_AHB_DMAC_REG_INT_CLEAR_SRC_TRAN:   // WO
    case DW_AHB_DMAC_REG_INT_CLEAR_DST_TRAN:   // WO
    case DW_AHB_DMAC_REG_INT_CLEAR_ERR:        // WO
        printf(LOG_TAG" Warning: ClearX registers 0x%08lX is Write Only!\n", offset);
        return 0;
    case DW_AHB_DMAC_REG_INT_STATUS_INT:
        return _dw_ahb_dmac_get_int_status(dmac);
    // Software Handshake Registers
    case DW_AHB_DMAC_REG_REQ_SRC:
    case DW_AHB_DMAC_REG_REQ_DST:
    case DW_AHB_DMAC_REG_SGL_RQ_SRC:
    case DW_AHB_DMAC_REG_SGL_RQ_DST:
    case DW_AHB_DMAC_REG_LST_SRC:
    case DW_AHB_DMAC_REG_LST_DST:
        printf(LOG_TAG" Warning: Software Handshake is not supported!\n");
        return 0;
    // Miscellaneous Registers
    case DW_AHB_DMAC_REG_DMA_CFG:
        return dmac->dma_enable;
    case DW_AHB_DMAC_REG_CH_EN:
        return dmac->channel_enable;
    case DW_AHB_DMAC_REG_DMA_ID:
        return DW_AHB_DMAC_DMAH_ID_NUM;
    case DW_AHB_DMAC_REG_DMA_TEST:             // No Support
        return 0;
    case DW_AHB_DMAC_REG_DMA_LP_TIMEOUT:
        return DW_AHB_DMAC_DMAH_LP_TIMEOUT_VALUE;
    case DW_AHB_DMAC_REG_DMA_COMP_PARAM_6_L:
        return 0;
    case DW_AHB_DMAC_REG_DMA_COMP_PARAM_6_H:
    case DW_AHB_DMAC_REG_DMA_COMP_PARAM_5_L:
    case DW_AHB_DMAC_REG_DMA_COMP_PARAM_5_H:
    case DW_AHB_DMAC_REG_DMA_COMP_PARAM_4_L:
    case DW_AHB_DMAC_REG_DMA_COMP_PARAM_4_H:
    case DW_AHB_DMAC_REG_DMA_COMP_PARAM_3_L:
    case DW_AHB_DMAC_REG_DMA_COMP_PARAM_3_H:
    case DW_AHB_DMAC_REG_DMA_COMP_PARAM_2_L:
        return _dw_ahb_dmac_param_channel.value;
    case DW_AHB_DMAC_REG_DMA_COMP_PARAM_2_H:
        return _dw_ahb_dmac_param_2_h.value;
    case DW_AHB_DMAC_REG_DMA_COMP_PARAM_1_L:
        return _dw_ahb_dmac_param_1_l.value;
    case DW_AHB_DMAC_REG_DMA_COMP_PARAM_1_H:
        return _dw_ahb_dmac_param_1_h.value;
    case DW_AHB_DMAC_REG_DMA_COMP_ID:
        return 0x44571110;
    default:
        break;
    }

    return 0;
}

//-------------------------------------------------------------------------------------------------

static inline uint32_t _dw_ahb_dmac_masked_value(uint32_t orignal, uint32_t mask, uint32_t value)
{
    return (~mask & orignal) | (mask & value);
}

static void _dw_ahb_dmac_channel_write(DwAhbDmacState *dmac, hwaddr offset, uint32_t value)
{
    uint32_t channel_index = offset / DW_AHB_DMAC_REG_CHN_STRIDE;
    uint32_t channel_offset = offset % DW_AHB_DMAC_REG_CHN_STRIDE;

    if (channel_index > _dw_ahb_dmac_param_1_h.num_channels) {
        printf(LOG_TAG" Warning: Channel Index %d > %d\n", channel_index, _dw_ahb_dmac_param_1_h.num_channels);
        return;
    }

    if (dmac->channel_enable & (1 << channel_index)) {
        printf(LOG_TAG" Warning: Channel %d is enabled yet, and could not write %x with value 0x%08X\n", channel_index, channel_offset, value);
        // return;
    }

    switch (channel_offset) {
    case DW_AHB_DMAC_REG_CHN_SAR(0):
        dmac->channel[channel_index].sar.value = value;
        return;
    case DW_AHB_DMAC_REG_CHN_DAR(0):
        dmac->channel[channel_index].dar.value = value;
        return;
    case DW_AHB_DMAC_REG_CHN_LLP(0):
        dmac->channel[channel_index].llp.value = value;
        return;
    case DW_AHB_DMAC_REG_CHN_CTL_L(0):
        dmac->channel[channel_index].ctl_l.value = value;
        return;
    case DW_AHB_DMAC_REG_CHN_CTL_H(0):
        dmac->channel[channel_index].ctl_h.value = value;
        return;
    case DW_AHB_DMAC_REG_CHN_SSTAT(0):
    case DW_AHB_DMAC_REG_CHN_DSTAT(0):
    case DW_AHB_DMAC_REG_CHN_SSTATAR(0):
    case DW_AHB_DMAC_REG_CHN_DSTATAR(0):
        printf(LOG_TAG" Warning: Write Channel Registers 0x%08lX is not exist!\n", offset);
        return;
    case DW_AHB_DMAC_REG_CHN_CFG_L(0):
        dmac->channel[channel_index].cfg_l.value = value;
        dmac->channel[channel_index].cfg_l.fifo_empty = 1;  // fifo always empty for us
        return;
    case DW_AHB_DMAC_REG_CHN_CFG_H(0):
        dmac->channel[channel_index].cfg_h.value = value;
        return;
    case DW_AHB_DMAC_REG_CHN_SGR(0):
        dmac->channel[channel_index].sgr.value = value;
        return;
    case DW_AHB_DMAC_REG_CHN_DSR(0):
        dmac->channel[channel_index].dsr.value = value;
        return;
    default:
        printf(LOG_TAG" Warning: Write Channel Reserved Region 0x%08lX!\n", offset);
        break;
    }
}

static void _dw_ahb_dmac_write(void *opaque, hwaddr offset, uint64_t value, unsigned size)
{
    DwAhbDmacState *dmac = (DwAhbDmacState *)opaque;

#ifdef LOG_DEBUG
    printf(LOG_TAG" %s(offset=0x%lx, value=0x%lx, size=%d)\n", __FUNCTION__, offset, value, size);
#endif
    switch (offset) {
    // Channel Registers
    case 0x000 ... DW_AHB_DMAC_REG_CHN_DSR(7):
        _dw_ahb_dmac_channel_write(dmac, offset, value);
        break;
    // Interrupt Status
    case DW_AHB_DMAC_REG_INT_RAW_TFR:
        printf(LOG_TAG" Warning: Write 0x%08lX to RAW_TFR\n", value);
        dmac->raw_tfr = value & 0xFF;
        _dw_ahb_dmac_update_irq(dmac);
        break;
    case DW_AHB_DMAC_REG_INT_RAW_BLOCK:
        printf(LOG_TAG" Warning: Write 0x%08lX to RAW_BLOCK\n", value);
        dmac->raw_block = value & 0xFF;
        _dw_ahb_dmac_update_irq(dmac);
        break;
    case DW_AHB_DMAC_REG_INT_RAW_SRC_TRAN:
        printf(LOG_TAG" Warning: Write 0x%08lX to RAW_SRC_TRAN\n", value);
        dmac->raw_src_tran = value & 0xFF;
        _dw_ahb_dmac_update_irq(dmac);
        break;
    case DW_AHB_DMAC_REG_INT_RAW_DST_TRAN:
        printf(LOG_TAG" Warning: Write 0x%08lX to RAW_DST_TRAN\n", value);
        dmac->raw_dst_tran = value & 0xFF;
        _dw_ahb_dmac_update_irq(dmac);
        break;
    case DW_AHB_DMAC_REG_INT_RAW_ERR:
        printf(LOG_TAG" Warning: Write 0x%08lX to RAW_ERR\n", value);
        dmac->raw_err = value & 0xFF;
        _dw_ahb_dmac_update_irq(dmac);
        break;
    case DW_AHB_DMAC_REG_INT_STATUS_TFR:       // RO
    case DW_AHB_DMAC_REG_INT_STATUS_BLOCK:     // RO
    case DW_AHB_DMAC_REG_INT_STATUS_SRC_TRAN:  // RO
    case DW_AHB_DMAC_REG_INT_STATUS_DST_TRAN:  // RO
    case DW_AHB_DMAC_REG_INT_STATUS_ERR:       // RO
        printf(LOG_TAG" Warning: Write 0x%08lX to ReadOnly Register 0x%08lX\n", value, offset);
        break;
    case DW_AHB_DMAC_REG_INT_MASK_TFR:
        dmac->mask_tfr = _dw_ahb_dmac_masked_value(dmac->mask_tfr, (value >> 8) & 0xFF, value);
        _dw_ahb_dmac_update_irq(dmac);
        break;
    case DW_AHB_DMAC_REG_INT_MASK_BLOCK:
        dmac->mask_block = _dw_ahb_dmac_masked_value(dmac->mask_block, (value >> 8) & 0xFF, value);
        _dw_ahb_dmac_update_irq(dmac);
        break;
    case DW_AHB_DMAC_REG_INT_MASK_SRC_TRAN:
        dmac->mask_src_tran = _dw_ahb_dmac_masked_value(dmac->mask_src_tran, (value >> 8) & 0xFF, value);
        _dw_ahb_dmac_update_irq(dmac);
        break;
    case DW_AHB_DMAC_REG_INT_MASK_DST_TRAN:
        dmac->mask_dst_tran = _dw_ahb_dmac_masked_value(dmac->mask_dst_tran, (value >> 8) & 0xFF, value);
        _dw_ahb_dmac_update_irq(dmac);
        break;
    case DW_AHB_DMAC_REG_INT_MASK_ERR:
        dmac->mask_err = _dw_ahb_dmac_masked_value(dmac->mask_err, (value >> 8) & 0xFF, value);
        _dw_ahb_dmac_update_irq(dmac);
        break;
    case DW_AHB_DMAC_REG_INT_CLEAR_TFR:
        dmac->raw_tfr &= ~(value & 0xFF);
        _dw_ahb_dmac_update_irq(dmac);
        break;
    case DW_AHB_DMAC_REG_INT_CLEAR_BLOCK:
        dmac->raw_block &= ~(value & 0xFF);
        _dw_ahb_dmac_update_irq(dmac);
        break;
    case DW_AHB_DMAC_REG_INT_CLEAR_SRC_TRAN:
        dmac->raw_src_tran &= ~(value & 0xFF);
        _dw_ahb_dmac_update_irq(dmac);
        break;
    case DW_AHB_DMAC_REG_INT_CLEAR_DST_TRAN:
        dmac->raw_dst_tran &= ~(value & 0xFF);
        _dw_ahb_dmac_update_irq(dmac);
        break;
    case DW_AHB_DMAC_REG_INT_CLEAR_ERR:
        dmac->raw_err &= ~(value & 0XFF);
        _dw_ahb_dmac_update_irq(dmac);
        break;
    case DW_AHB_DMAC_REG_INT_STATUS_INT:   // RO
        printf(LOG_TAG" Warning: Write 0x%08lX to ReadOnly Register 0x%08lX\n", value, offset);
        break;
    // Software Handshake Registers
    case DW_AHB_DMAC_REG_REQ_SRC:
    case DW_AHB_DMAC_REG_REQ_DST:
    case DW_AHB_DMAC_REG_SGL_RQ_SRC:
    case DW_AHB_DMAC_REG_SGL_RQ_DST:
    case DW_AHB_DMAC_REG_LST_SRC:
    case DW_AHB_DMAC_REG_LST_DST:
        printf(LOG_TAG" Warning: Software Handshake is not supported!\n");
        break;
    // Miscellaneous Registers
    case DW_AHB_DMAC_REG_DMA_CFG:
        dmac->dma_enable = value & 0x1;
        if (dmac->dma_enable == 0) {
            dmac->channel_enable = 0;
        }
        break;
    case DW_AHB_DMAC_REG_CH_EN:
        if (dmac->dma_enable) {
            dmac->channel_enable = _dw_ahb_dmac_masked_value(dmac->channel_enable, (value >> 8) & 0xFF, value);
            if (dmac->channel_enable) {
                timer_mod(dmac->process_timer, qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL));
            }
        }
        else {
            printf(LOG_TAG" Warning: DmaCfgReg[0] is 0!\n");
        }
        break;
    case DW_AHB_DMAC_REG_DMA_ID:                   // RO
    case DW_AHB_DMAC_REG_DMA_TEST:
    case DW_AHB_DMAC_REG_DMA_LP_TIMEOUT:
    case DW_AHB_DMAC_REG_DMA_COMP_PARAM_6_L:
    case DW_AHB_DMAC_REG_DMA_COMP_PARAM_6_H:
    case DW_AHB_DMAC_REG_DMA_COMP_PARAM_5_L:
    case DW_AHB_DMAC_REG_DMA_COMP_PARAM_5_H:
    case DW_AHB_DMAC_REG_DMA_COMP_PARAM_4_L:
    case DW_AHB_DMAC_REG_DMA_COMP_PARAM_4_H:
    case DW_AHB_DMAC_REG_DMA_COMP_PARAM_3_L:
    case DW_AHB_DMAC_REG_DMA_COMP_PARAM_3_H:
    case DW_AHB_DMAC_REG_DMA_COMP_PARAM_2_L:
    case DW_AHB_DMAC_REG_DMA_COMP_PARAM_2_H:
    case DW_AHB_DMAC_REG_DMA_COMP_PARAM_1_L:
    case DW_AHB_DMAC_REG_DMA_COMP_PARAM_1_H:
    case DW_AHB_DMAC_REG_DMA_COMP_ID:
        printf(LOG_TAG" Warning: Write 0x%08lX to ReadOnly Register 0x%08lX\n", value, offset);
        break;
    default:
        printf(LOG_TAG" Warning: Illeage Write address 0x%08lX\n", offset);
        break;
    }
}

static void _dw_ahb_dmac_reset(DeviceState *d)
{
    DwAhbDmacState *dmac = DW_AHB_DMAC(d);
    dmac->raw_tfr = 0;
    dmac->raw_block = 0;
    dmac->raw_src_tran = 0;
    dmac->raw_dst_tran = 0;
    dmac->raw_err = 0;

    dmac->mask_tfr = 0;
    dmac->mask_block = 0;
    dmac->mask_src_tran = 0;
    dmac->mask_dst_tran = 0;
    dmac->mask_err = 0;

    dmac->dma_enable = 0;
    dmac->channel_enable = 0;

    memset(dmac->handshake, 0, sizeof(dmac->handshake));
    memset(dmac->channel, 0, sizeof(dmac->channel));
}

static const MemoryRegionOps _dw_ahb_dmac_ops = {
    .read = _dw_ahb_dmac_read,
    .write = _dw_ahb_dmac_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static const VMStateDescription _vmstate_dw_ahb_dmac = {
    .name = TYPE_DW_AHB_DMAC,
    .version_id = 1,
    .fields = (VMStateField[]) {
        // VMSTATE_UINT32_ARRAY(reg, DwcRtcState, 0x28),
        // VMSTATE_INT64(offset, DwcRtcState),
        // VMSTATE_UINT32(match, DwcRtcState),
        // VMSTATE_UINT32(load, DwcRtcState),
        // VMSTATE_UINT32(control.value, DwcRtcState),
        // VMSTATE_UINT32(raw_state, DwcRtcState),
        // VMSTATE_UINT32(prescaler, DwcRtcState),
        VMSTATE_END_OF_LIST()
    }
};

static void _dw_ahb_dmac_realize(DeviceState *dev, Error **errp)
{
    //SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
    DwAhbDmacState *dmac = DW_AHB_DMAC(dev);

    dmac->process_timer = timer_new_ns(QEMU_CLOCK_VIRTUAL, _dw_ahb_dmac_process, dmac);

    // Check DMA Memory Region Properity
    if (!dmac->ahb_master[0]) {
        error_setg(errp, LOG_TAG" 'ahb_master[0]' link not set!");
        return;
    }
    for (int i = 0; i < 4; i++) {
        if (dmac->ahb_master[i])
            address_space_init(&dmac->dma_as[i], dmac->ahb_master[i], "DW_AHB_DMAC[*]");
    }
}

static void _dw_ahb_dmac_init(Object *obj)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);
    DwAhbDmacState *s = DW_AHB_DMAC(obj);

    /* DMAC initialization */
    sysbus_init_irq(sbd, &s->irq);
    qdev_init_gpio_in(DEVICE(obj), _dw_ahb_dmac_handshake, 16);

    memory_region_init_io(&s->iomem, OBJECT(s), &_dw_ahb_dmac_ops, s,
                          "dwc-dmac", 0x400ULL);
    sysbus_init_mmio(sbd, &s->iomem);
}

static Property _dw_ahb_dmac_properties[] = {
    DEFINE_PROP_LINK("ahb-master-0", DwAhbDmacState, ahb_master[0], TYPE_MEMORY_REGION, MemoryRegion *),
    DEFINE_PROP_LINK("ahb-master-1", DwAhbDmacState, ahb_master[1], TYPE_MEMORY_REGION, MemoryRegion *),
    DEFINE_PROP_LINK("ahb-master-2", DwAhbDmacState, ahb_master[2], TYPE_MEMORY_REGION, MemoryRegion *),
    DEFINE_PROP_LINK("ahb-master-3", DwAhbDmacState, ahb_master[3], TYPE_MEMORY_REGION, MemoryRegion *),
    DEFINE_PROP_END_OF_LIST(),
};

static void _dw_ahb_dmac_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = _dw_ahb_dmac_realize;
    dc->vmsd = &_vmstate_dw_ahb_dmac;
    dc->reset = _dw_ahb_dmac_reset;
    device_class_set_props(dc, _dw_ahb_dmac_properties);
}

static const TypeInfo _dw_ahb_dmac_info = {
    .name          = TYPE_DW_AHB_DMAC,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(DwAhbDmacState),
    .instance_init = _dw_ahb_dmac_init,
    .class_init    = _dw_ahb_dmac_class_init,
};

static void _dw_ahb_dmac_register_types(void)
{
    type_register_static(&_dw_ahb_dmac_info);
}

type_init(_dw_ahb_dmac_register_types)
