/*
 * NationalChip TREC Project
 * Copyright (C) 2001-2020 NationalChip Co., Ltd
 * ALL RIGHTS RESERVED!
 *
 * dev_dw_ahb_dmac.h: DMA Controller
 *                 based on ahb_dmac_databook.pdf 2.22a
 *
 */

#ifndef __DEV_DW_AHB_DMAC_H__
#define __DEV_DW_AHB_DMAC_H__

typedef union {
    unsigned int value;
    struct {
        unsigned sar:32;    // current source address of DMA transfer
    };
} DW_AHB_DMAC_CHN_SAR;

typedef union {
    unsigned int value;
    struct {
        unsigned dar:32;    // current destination address of DMA transfer
    };
} DW_AHB_DMAC_CHN_DAR;

typedef union {
    unsigned int value;
    struct {
        unsigned lms:2;     // List Master Select
        unsigned loc:30;    // Starting Address in Memory
    };
} DW_AHB_DMAC_CHN_LLP;

#define DW_AHB_DMAC_CHN_CTL_TT_FC_M2M_DMAC  0
#define DW_AHB_DMAC_CHN_CTL_TT_FC_M2P_DMAC  1
#define DW_AHB_DMAC_CHN_CTL_TT_FC_P2M_DMAC  2
#define DW_AHB_DMAC_CHN_CTL_TT_FC_P2P_DMAC  3
#define DW_AHB_DMAC_CHN_CTL_TT_FC_P2M_PERP  4
#define DW_AHB_DMAC_CHN_CTL_TT_FC_P2P_SRCP  5
#define DW_AHB_DMAC_CHN_CTL_TT_FC_M2P_PERP  6
#define DW_AHB_DMAC_CHN_CTL_TT_FC_P2P_DSTP  7

typedef union {
    unsigned int value;
    struct {
        unsigned int_en:1;
        unsigned dst_tr_width:3;    // Destination Transfer Width
        unsigned src_tr_width:3;    // Source Transfer Width
        unsigned dinc:2;            // Destination Address Increment
        unsigned sinc:2;            // Source Address Increment
        unsigned dest_msize:3;      // Destination Burst Transaction Length
        unsigned src_msize:3;       // Source Burst Transaction Length
        unsigned src_gather_en:1;   // Source Gather Enable
        unsigned dst_scatter_en:1;  // Destination Scatter Enable
        unsigned :1;
        unsigned tt_fc:3;           // Transfer Type and Flow Control
        unsigned dms:2;             // Destination Master Select
        unsigned sms:2;             // Source Master Select
        unsigned llp_dst_en:1;      // Block chaining is enabled on the destination side
        unsigned llp_src_en:1;      // Block chaining is enabled on the source side
        unsigned :3;
    };
} DW_AHB_DMAC_CHN_CTL_L;

typedef union {
    unsigned int value;
    struct {
        unsigned block_ts:12;       // Block Transfer Size
        unsigned done:1;            // Done bit
        unsigned :19;
    };
} DW_AHB_DMAC_CHN_CTL_H;

typedef union {
    unsigned int value;
    struct {
        unsigned sstat:32;          // Source status information retrieved by hardware
    };
} DW_AHB_DMAC_CHN_SSTAT;

typedef union {
    unsigned int value;
    struct {
        unsigned dstat:32;
    };
} DW_AHB_DMAC_CHN_DSTAT;

typedef union {
    unsigned int value;
    struct {
        unsigned sstatar:32;
    };
} DW_AHB_DMAC_CHN_SSTATAR;

typedef union {
    unsigned int value;
    struct {
        unsigned dstatar:32;
    };
} DW_AHB_DMAC_CHN_DSTATAR;

typedef union {
    unsigned int value;
    struct {
        unsigned :5;
        unsigned ch_prior:3;        // Channel Priority
        unsigned ch_susp:1;         // Channel Suspend
        unsigned fifo_empty:1;      // Change FIFO status (RO)
        unsigned hs_sel_dst:1;      // Destination Software or Hardware Handshaking Select
        unsigned hs_sel_src:1;      // Source Software or Hardware Handshaking Select
        unsigned lock_ch_l:2;       // Channel Lock Level
        unsigned lock_b_l:2;        // Bus Lock Level
        unsigned lock_ch:1;         // Channel Lock Bit
        unsigned lock_b:1;          // Bus Lock Bit
        unsigned dst_hs_pol:1;      // Destination Handshaking Interface Polarity
        unsigned src_hs_pol:1;      // Source Handshaking Interface Polarity
        unsigned max_abrst:10;      // Maximum AMBA Burst Length
        unsigned reload_src:1;      // Automative Source Reload
        unsigned reload_dst:1;      // Automative Destination Reload
    };
} DW_AHB_DMAC_CHN_CFG_L;

typedef union {
    unsigned int value;
    struct {
        unsigned fcmode:1;
        unsigned fifo_mode:1;
        unsigned protctl:3;
        unsigned ds_upd_en:1;
        unsigned ss_upd_en:1;
        unsigned src_per:4;
        unsigned dst_per:4;
        unsigned :17;
    };
} DW_AHB_DMAC_CHN_CFG_H;

typedef union {
    unsigned int value;
    struct {
        unsigned sgi:20;
        unsigned sgc:12;
    };
} DW_AHB_DMAC_CHN_SGR;

typedef union {
    unsigned int value;
    struct {
        unsigned dsr:20;
        unsigned dsc:12;
    };
} DW_AHB_DMAC_CHN_DSR;

typedef struct {
    DW_AHB_DMAC_CHN_SAR         sar;
    unsigned                    :32;
    DW_AHB_DMAC_CHN_DAR         dar;
    unsigned                    :32;
    DW_AHB_DMAC_CHN_LLP         llp;
    unsigned                    :32;
    DW_AHB_DMAC_CHN_CTL_L       ctl_l;
    DW_AHB_DMAC_CHN_CTL_H       ctl_h;
    DW_AHB_DMAC_CHN_SSTAT       sstat;
    unsigned                    :32;
    DW_AHB_DMAC_CHN_DSTAT       dstat;
    unsigned                    :32;
    DW_AHB_DMAC_CHN_SSTATAR     sstatar;
    unsigned                    :32;
    DW_AHB_DMAC_CHN_DSTATAR     dstatar;
    unsigned                    :32;
    DW_AHB_DMAC_CHN_CFG_L       cfg_l;
    DW_AHB_DMAC_CHN_CFG_H       cfg_h;
    DW_AHB_DMAC_CHN_SGR         sgr;
    unsigned                    :32;
    DW_AHB_DMAC_CHN_DSR         dsr;
    unsigned                    :32;
} DW_AHB_DMAC_CHN;

#define DW_AHB_DMAC_REG_CHN_STRIDE          0x58

#define DW_AHB_DMAC_REG_CHN_SAR(c)          ((c) * DW_AHB_DMAC_REG_CHN_STRIDE + 0x00)
#define DW_AHB_DMAC_REG_CHN_DAR(c)          ((c) * DW_AHB_DMAC_REG_CHN_STRIDE + 0x08)
#define DW_AHB_DMAC_REG_CHN_LLP(c)          ((c) * DW_AHB_DMAC_REG_CHN_STRIDE + 0x10)
#define DW_AHB_DMAC_REG_CHN_CTL_L(c)        ((c) * DW_AHB_DMAC_REG_CHN_STRIDE + 0x18)
#define DW_AHB_DMAC_REG_CHN_CTL_H(c)        ((c) * DW_AHB_DMAC_REG_CHN_STRIDE + 0x1C)
#define DW_AHB_DMAC_REG_CHN_SSTAT(c)        ((c) * DW_AHB_DMAC_REG_CHN_STRIDE + 0x20)
#define DW_AHB_DMAC_REG_CHN_DSTAT(c)        ((c) * DW_AHB_DMAC_REG_CHN_STRIDE + 0x28)
#define DW_AHB_DMAC_REG_CHN_SSTATAR(c)      ((c) * DW_AHB_DMAC_REG_CHN_STRIDE + 0x30)
#define DW_AHB_DMAC_REG_CHN_DSTATAR(c)      ((c) * DW_AHB_DMAC_REG_CHN_STRIDE + 0x38)
#define DW_AHB_DMAC_REG_CHN_CFG_L(c)        ((c) * DW_AHB_DMAC_REG_CHN_STRIDE + 0x40)
#define DW_AHB_DMAC_REG_CHN_CFG_H(c)        ((c) * DW_AHB_DMAC_REG_CHN_STRIDE + 0x44)
#define DW_AHB_DMAC_REG_CHN_SGR(c)          ((c) * DW_AHB_DMAC_REG_CHN_STRIDE + 0x48)
#define DW_AHB_DMAC_REG_CHN_DSR(c)          ((c) * DW_AHB_DMAC_REG_CHN_STRIDE + 0x50)

typedef struct {
    DW_AHB_DMAC_CHN_SAR         sar;
    DW_AHB_DMAC_CHN_DAR         dar;
    DW_AHB_DMAC_CHN_LLP         llp;
    DW_AHB_DMAC_CHN_CTL_L       ctl_l;
    DW_AHB_DMAC_CHN_CTL_H       ctl_h;
    unsigned int                state[2];
} DW_AHB_DMAC_LLI;

//=================================================================================================

typedef union {
    unsigned int value;
    struct {
        unsigned raw:8;
        unsigned :24;
    };
} DW_AHB_DMAC_INT_RAW;

typedef union {
    unsigned int value;
    struct {
        unsigned status:8;
        unsigned :24;
    };
} DW_AHB_DMAC_INT_STATUS;

typedef union {
    unsigned int value;
    struct {
        unsigned int_mask:8;
        unsigned int_mask_we:8;
        unsigned :16;
    };
} DW_AHB_DMAC_INT_MASK;

typedef union {
    unsigned int value[2];
    struct {
        unsigned clear:8;
        unsigned :24;
    };
} DW_AHB_DMAC_INT_CLEAR;

typedef union {
    unsigned int value;
    struct {
        unsigned tfr:1;
        unsigned block:1;
        unsigned srct:1;
        unsigned dstt:1;
        unsigned err:1;
        unsigned :27;
    };
} DW_AHB_DMAC_INT_STATE;

#define DW_AHB_DMAC_REG_INT_RAW_TFR             0x2C0
#define DW_AHB_DMAC_REG_INT_RAW_BLOCK           0x2C8
#define DW_AHB_DMAC_REG_INT_RAW_SRC_TRAN        0x2D0
#define DW_AHB_DMAC_REG_INT_RAW_DST_TRAN        0x2D8
#define DW_AHB_DMAC_REG_INT_RAW_ERR             0x2E0

#define DW_AHB_DMAC_REG_INT_STATUS_TFR          0x2E8
#define DW_AHB_DMAC_REG_INT_STATUS_BLOCK        0x2F0
#define DW_AHB_DMAC_REG_INT_STATUS_SRC_TRAN     0x2F8
#define DW_AHB_DMAC_REG_INT_STATUS_DST_TRAN     0x300
#define DW_AHB_DMAC_REG_INT_STATUS_ERR          0x308

#define DW_AHB_DMAC_REG_INT_MASK_TFR            0x310
#define DW_AHB_DMAC_REG_INT_MASK_BLOCK          0x318
#define DW_AHB_DMAC_REG_INT_MASK_SRC_TRAN       0x320
#define DW_AHB_DMAC_REG_INT_MASK_DST_TRAN       0x328
#define DW_AHB_DMAC_REG_INT_MASK_ERR            0x330

#define DW_AHB_DMAC_REG_INT_CLEAR_TFR           0x338
#define DW_AHB_DMAC_REG_INT_CLEAR_BLOCK         0x340
#define DW_AHB_DMAC_REG_INT_CLEAR_SRC_TRAN      0x348
#define DW_AHB_DMAC_REG_INT_CLEAR_DST_TRAN      0x350
#define DW_AHB_DMAC_REG_INT_CLEAR_ERR           0x358

#define DW_AHB_DMAC_REG_INT_STATUS_INT          0x360

//=================================================================================================

typedef union {
    unsigned int value;
    struct {
        unsigned req:8;
        unsigned req_we:8;
        unsigned :16;
    };
} DW_AHB_DMAC_REQ;

#define DW_AHB_DMAC_REG_REQ_SRC                 0x368       // Source Software Transaction Request
#define DW_AHB_DMAC_REG_REQ_DST                 0x370       // Destination Software Transaction Request
#define DW_AHB_DMAC_REG_SGL_RQ_SRC              0x378       // Source Software Transaction Request
#define DW_AHB_DMAC_REG_SGL_RQ_DST              0x380       // Destination Software Transaction Request
#define DW_AHB_DMAC_REG_LST_SRC                 0x388       // Source Single Transaction Request
#define DW_AHB_DMAC_REG_LST_DST                 0x390       // Destination Single Transaction Request

//=================================================================================================

typedef union {
    unsigned int value;
    struct {
        unsigned dma_en:1;
        unsigned :31;
    };
} DW_AHB_DMAC_DMA_CFG;

typedef union {
    unsigned int value;
    struct {
        unsigned ch_en:8;
        unsigned ch_en_we:8;
        unsigned :16;
    };
} DW_AHB_DMAC_CH_EN;

// Const Parameters from DMAC configuration
#define DW_AHB_DMAC_DMAH_ID_NUM                 0x02080901
#define DW_AHB_DMAC_DMAH_LP_TIMEOUT_VALUE       0x08

// const for DMA_COMP_PARAMS_CHANNEL
#define DW_AHB_DMAC_DMAH_CHX_DTW_NO_HARDCODE    0x00
#define DW_AHB_DMAC_DMAH_CHX_DTW_8              0x01
#define DW_AHB_DMAC_DMAH_CHX_DTW_16             0x02
#define DW_AHB_DMAC_DMAH_CHX_DTW_32             0x03
#define DW_AHB_DMAC_DMAH_CHX_DTW_64             0x04
#define DW_AHB_DMAC_DMAH_CHX_DTW_128            0x05
#define DW_AHB_DMAC_DMAH_CHX_DTW_256            0x06

#define DW_AHB_DMAC_DMAH_CHX_STW_NO_HARDCODE    0x00
#define DW_AHB_DMAC_DMAH_CHX_STW_8              0x01
#define DW_AHB_DMAC_DMAH_CHX_STW_16             0x02
#define DW_AHB_DMAC_DMAH_CHX_STW_32             0x03
#define DW_AHB_DMAC_DMAH_CHX_STW_64             0x04
#define DW_AHB_DMAC_DMAH_CHX_STW_128            0x05
#define DW_AHB_DMAC_DMAH_CHX_STW_256            0x06

#define DW_AHB_DMAC_DMAH_CHX_HC_LLP_PROGRAMMABLE   0x00
#define DW_AHB_DMAC_DMAH_CHX_HC_LLP_HARDCODE    0x01

#define DW_AHB_DMAC_DMAH_CHX_FC_DMA             0x00
#define DW_AHB_DMAC_DMAH_CHX_FC_SRC             0x01
#define DW_AHB_DMAC_DMAH_CHX_FC_DST             0x02
#define DW_AHB_DMAC_DMAH_CHX_FC_ANY             0x03

#define DW_AHB_DMAC_DMAH_CHX_MAX_MULT_SIZE_4    0x00
#define DW_AHB_DMAC_DMAH_CHX_MAX_MULT_SIZE_8    0x01
#define DW_AHB_DMAC_DMAH_CHX_MAX_MULT_SIZE_16   0x02
#define DW_AHB_DMAC_DMAH_CHX_MAX_MULT_SIZE_32   0x03
#define DW_AHB_DMAC_DMAH_CHX_MAX_MULT_SIZE_64   0x04
#define DW_AHB_DMAC_DMAH_CHX_MAX_MULT_SIZE_128  0x05
#define DW_AHB_DMAC_DMAH_CHX_MAX_MULT_SIZE_256  0x06

#define DW_AHB_DMAC_DMAH_CHX_SMS_PROGRAMMABLE   0x04
#define DW_AHB_DMAC_DMAH_CHX_LMS_PROGRAMMABLE   0x04
#define DW_AHB_DMAC_DMAH_CHX_DMS_PROGRAMMABLE   0x04

#define DW_AHB_DMAC_DMAH_CHX_FIFO_DEPTH_8       0x00
#define DW_AHB_DMAC_DMAH_CHX_FIFO_DEPTH_16      0x01
#define DW_AHB_DMAC_DMAH_CHX_FIFO_DEPTH_32      0x02
#define DW_AHB_DMAC_DMAH_CHX_FIFO_DEPTH_64      0x03
#define DW_AHB_DMAC_DMAH_CHX_FIFO_DEPTH_128     0x04
#define DW_AHB_DMAC_DMAH_CHX_FIFO_DEPTH_256     0x05

typedef union {
    unsigned int value;
    struct {
        unsigned dtw:3;
        unsigned stw:3;
        unsigned stat_dst:1;
        unsigned stat_src:1;
        unsigned dst_sca_en:1;
        unsigned src_gat_en:1;
        unsigned lock_en:1;
        unsigned multi_blk_en:1;
        unsigned ctl_wb_en:1;
        unsigned hc_llp:1;
        unsigned fc:2;
        unsigned max_mult_size:3;
        unsigned dms:3;
        unsigned lms:3;
        unsigned sms:3;
        unsigned fifo_depth:3;
        unsigned :1;
    };
} DW_AHB_DMAC_COMP_PARAM_CHANNEL;

#define DW_AHB_DMAC_DMAH_CHX_MULTI_BLK_TYPE_PROGRAMMABLE   0x00
#define DW_AHB_DMAC_DMAH_CHX_MULTI_BLK_TYPE_CONT_RELOAD    0x01
#define DW_AHB_DMAC_DMAH_CHX_MULTI_BLK_TYPE_RELOAD_CONT    0x02
#define DW_AHB_DMAC_DMAH_CHX_MULTI_BLK_TYPE_RELOAD_RELOAD  0x03
#define DW_AHB_DMAC_DMAH_CHX_MULTI_BLK_TYPE_CONT_LLP       0x04
#define DW_AHB_DMAC_DMAH_CHX_MULTI_BLK_TYPE_RELOAD_LLP     0x05
#define DW_AHB_DMAC_DMAH_CHX_MULTI_BLK_TYPE_CNT_LLP        0x06
#define DW_AHB_DMAC_DMAH_CHX_MULTI_BLK_TYPE_LLP_RELOAD     0x07
#define DW_AHB_DMAC_DMAH_CHX_MULTI_BLK_TYPE_LLP_LLP        0x08

typedef union {
    unsigned int value;
    struct {
        unsigned ch0_multi_blk_type:4;
        unsigned ch1_multi_blk_type:4;
        unsigned ch2_multi_blk_type:4;
        unsigned ch3_multi_blk_type:4;
        unsigned ch4_multi_blk_type:4;
        unsigned ch5_multi_blk_type:4;
        unsigned ch6_multi_blk_type:4;
        unsigned ch7_multi_blk_type:4;
    };
} DW_AHB_DMAC_COMP_PARAM_2_H;

#define DW_AHB_DMAC_DMAH_CHX_MAX_BLK_SIZE_3     0x00
#define DW_AHB_DMAC_DMAH_CHX_MAX_BLK_SIZE_7     0x01
#define DW_AHB_DMAC_DMAH_CHX_MAX_BLK_SIZE_15    0x02
#define DW_AHB_DMAC_DMAH_CHX_MAX_BLK_SIZE_31    0x03
#define DW_AHB_DMAC_DMAH_CHX_MAX_BLK_SIZE_63    0x04
#define DW_AHB_DMAC_DMAH_CHX_MAX_BLK_SIZE_127   0x05
#define DW_AHB_DMAC_DMAH_CHX_MAX_BLK_SIZE_255   0x06
#define DW_AHB_DMAC_DMAH_CHX_MAX_BLK_SIZE_511   0x07
#define DW_AHB_DMAC_DMAH_CHX_MAX_BLK_SIZE_1023  0x08
#define DW_AHB_DMAC_DMAH_CHX_MAX_BLK_SIZE_2047  0x09
#define DW_AHB_DMAC_DMAH_CHX_MAX_BLK_SIZE_4095  0x0A

typedef union {
    unsigned int value;
    struct {
        unsigned ch0_max_blk_size:4;
        unsigned ch1_max_blk_size:4;
        unsigned ch2_max_blk_size:4;
        unsigned ch3_max_blk_size:4;
        unsigned ch4_max_blk_size:4;
        unsigned ch5_max_blk_size:4;
        unsigned ch6_max_blk_size:4;
        unsigned ch7_max_blk_size:4;
    };
} DW_AHB_DMAC_COMP_PARAM_1_L;

#define DW_AHB_DMAC_DMAH_INTR_IO_ALL_INT        0x00
#define DW_AHB_DMAC_DMAH_INTR_IO_TYPE_INT       0x01
#define DW_AHB_DMAC_DMAH_INTR_IO_COMBINED_INT   0x02

#define DW_AHB_DMAC_DMAH_S_HDATA_WIDTH_32       0x00
#define DW_AHB_DMAC_DMAH_S_HDATA_WIDTH_64       0x01
#define DW_AHB_DMAC_DMAH_S_HDATA_WIDTH_128      0x02
#define DW_AHB_DMAC_DMAH_S_HDATA_WIDTH_256      0x03

#define DW_AHB_DMAC_DMAH_MX_HDATA_WIDTH_32      0x00
#define DW_AHB_DMAC_DMAH_MX_HDATA_WIDTH_64      0x01
#define DW_AHB_DMAC_DMAH_MX_HDATA_WIDTH_128     0x02
#define DW_AHB_DMAC_DMAH_MX_HDATA_WIDTH_256     0x03

typedef union {
    unsigned int value;
    struct {
        unsigned big_endian:1;
        unsigned intr_io:2;
        unsigned max_abrst:1;
        unsigned :4;
        unsigned num_channels:3;
        unsigned num_master_int:2;
        unsigned s_hdata_width:2;
        unsigned m4_hdata_width:2;
        unsigned m3_hdata_width:2;
        unsigned m2_hdata_width:2;
        unsigned m1_hdata_width:2;
        unsigned num_hs_int:5;
        unsigned add_encoded_params:1;
        unsigned static_endian_select:1;
        unsigned :2;
    };
} DW_AHB_DMAC_COMP_PARAM_1_H;

// Offsets
#define DW_AHB_DMAC_REG_DMA_CFG                 0x398       // Configuration Register
#define DW_AHB_DMAC_REG_CH_EN                   0x3A0       // Channel Enable Register
#define DW_AHB_DMAC_REG_DMA_ID                  0x3A8       // ID Register
#define DW_AHB_DMAC_REG_DMA_TEST                0x3B0       // Test Register
#define DW_AHB_DMAC_REG_DMA_LP_TIMEOUT          0x3B8

#define DW_AHB_DMAC_REG_DMA_COMP_PARAM_6_L      0x3C8
#define DW_AHB_DMAC_REG_DMA_COMP_PARAM_6_H      0x3CC
#define DW_AHB_DMAC_REG_DMA_COMP_PARAM_5_L      0x3D0
#define DW_AHB_DMAC_REG_DMA_COMP_PARAM_5_H      0x3D4
#define DW_AHB_DMAC_REG_DMA_COMP_PARAM_4_L      0x3D8
#define DW_AHB_DMAC_REG_DMA_COMP_PARAM_4_H      0x3DC
#define DW_AHB_DMAC_REG_DMA_COMP_PARAM_3_L      0x3E0
#define DW_AHB_DMAC_REG_DMA_COMP_PARAM_3_H      0x3E4
#define DW_AHB_DMAC_REG_DMA_COMP_PARAM_2_L      0x3E8
#define DW_AHB_DMAC_REG_DMA_COMP_PARAM_2_H      0x3EC
#define DW_AHB_DMAC_REG_DMA_COMP_PARAM_1_L      0x3F0
#define DW_AHB_DMAC_REG_DMA_COMP_PARAM_1_H      0x3F4

#define DW_AHB_DMAC_REG_DMA_COMP_ID             0x3F8

#endif /* __DEV_DW_AHB_DMAC_H__ */
