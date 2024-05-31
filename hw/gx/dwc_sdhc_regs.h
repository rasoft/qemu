/*
 * NationalChip TREC Project
 * Copyright (C) 2001-2020 NationalChip Co., Ltd
 * ALL RIGHTS RESERVED!
 *
 * dev_dwc_sdhc.h: SD Host Controller
 *
 */

#ifndef __DEV_DWC_SDHC_H__
#define __DEV_DWC_SDHC_H__

typedef union {
    unsigned int value;
    struct {
        unsigned controller_reset:1;
        unsigned fifo_reset:1;
        unsigned dma_reset:1;
        unsigned :1;
        unsigned int_enable:1;
        unsigned dma_enable:1;
        unsigned read_wait:1;
        unsigned send_irq_response:1;
        unsigned abort_read_data:1;
        unsigned send_ccsd:1;
        unsigned send_auto_stop_ccsd:1;
        unsigned ceata_device_interrupt_status:1;
        unsigned :4;
        unsigned card_voltage_a:4;
        unsigned card_voltage_b:4;
        unsigned enable_od_pullup:1;
        unsigned use_internal_dmac:1;
        unsigned :6;
    };
} DWC_SDHC_CTRL;

typedef union {
    unsigned int value;
    struct {
        unsigned power_enable:30;
        unsigned :2;
    };
} DWC_SDHC_PWREN;

typedef union {
    unsigned int value;
    struct {
        unsigned clk_divider0:8;
        unsigned clk_divider1:8;
        unsigned clk_divider2:8;
        unsigned clk_divider3:8;
    };
} DWC_SDHC_CLKDIV;

typedef union {
    unsigned int value;
    struct {
        unsigned clk_source:32;
    };
} DWC_SDHC_CLKSRC;

typedef union {
    unsigned int value;
    struct {
        unsigned cclk_enable:16;
        unsigned cclk_low_power:16;
    };
} DWC_SDHC_CLKENA;

typedef union {
    unsigned int value;
    struct {
        unsigned response_timeout:8;
        unsigned data_timeout:24;
    };
} DWC_SDHC_TMOUT;

typedef union {
    unsigned int value;
    struct {
        unsigned card_width_a:16;   // 1bit or 4bit
        unsigned card_width_b:16;   // non 8bit or 8bit
    };
} DWC_SDHC_CTYPE;

typedef union {
    unsigned int value;
    struct {
        unsigned block_size:16;
        unsigned :16;
    };
} DWC_SDHC_BLKSIZ;

typedef unsigned int DWC_SDHC_BYTCNT;

typedef union {
    unsigned int value;
    struct {
        unsigned card_detect:1;                         // CD
        unsigned response_error:1;                      // RE
        unsigned command_done:1;                        // CE
        unsigned data_transfer_over:1;                  // DTO
        unsigned transmit_fifo_data_request:1;          // TXDR
        unsigned receive_fifo_data_request:1;           // RXDR
        unsigned response_crc_error:1;                  // RCRC
        unsigned data_crc_error:1;                      // DCRC
        unsigned response_timeout:1;                    // RTO
        unsigned data_read_timeout:1;                   // DRTO
        unsigned data_starvation_by_host_timeout:1;     // HTO
        unsigned fifo_underrun_error:1;                 // FRUN
        unsigned hardware_lock_write_error:1;           // HLE
        unsigned start_bit_error:1;                     // SBE
        unsigned auto_command_done:1;                   // ACD
        unsigned end_bit_error:1;                       // EBE
        unsigned sdio_int_mask:16;
    };
} DWC_SDHC_INT;

typedef unsigned int DWC_SDHC_CMDARG;

typedef union {
    unsigned int value;
    struct {
        unsigned cmd_index:6;
        unsigned response_expect:1;
        unsigned response_length:1;
        unsigned check_response_crc:1;
        unsigned data_expected:1;
        unsigned read_or_write:1;
        unsigned transfer_mode:1;
        unsigned send_auto_stop:1;
        unsigned wait_prvdata_complete:1;
        unsigned stop_abort_cmd:1;
        unsigned send_initialization:1;
        unsigned card_number:5;
        unsigned update_clock_registers_only:1;
        unsigned read_ceata_device:1;
        unsigned ccs_expected:1;
        unsigned enable_boot:1;
        unsigned expect_boot_ack:1;
        unsigned disable_boot:1;
        unsigned boot_mode:1;
        unsigned vol_switch:1;
        unsigned use_hold_reg:1;
        unsigned :1;
        unsigned start_cmd:1;
    };
} DWC_SDHC_CMD;

typedef unsigned int DWC_SDHC_RESP;

typedef union {
    unsigned int value;
    struct {
        unsigned fifo_rx_watermark:1;
        unsigned fifo_tx_watermark:1;
        unsigned fifo_empty:1;
        unsigned fifo_full:1;
        unsigned command_fsm_states:4;
        unsigned data_3_status:1;
        unsigned data_busy:1;
        unsigned data_state_mc_busy:1;
        unsigned response_index:6;
        unsigned fifo_count:13;
        unsigned dma_ack:1;
        unsigned dma_req:1;
    };
} DWC_SDHC_STATUS;

typedef union {
    unsigned int value;
    struct {
        unsigned tx_wmark:12;
        unsigned :4;
        unsigned rx_wmark:12;
        unsigned dw_dma_multiple_transaction_size:3;
        unsigned :1;
    };
} DWC_SDHC_FIFOTH;

typedef union {
    unsigned int value;
    struct {
        unsigned card_detect_n:30;
        unsigned :2;
    };
} DWC_SDHC_CDETECT;

typedef union {
    unsigned int value;
    struct {
        unsigned write_protect:30;
        unsigned :2;
    };
} DWC_SDHC_WRTPRT;

typedef union {
    unsigned int value;
    struct {
        unsigned gpi:8;
        unsigned gpo:16;
        unsigned :8;
    };
} DWC_SDHC_GPIO;

typedef unsigned int DWC_SDHC_TCBCNT;

typedef unsigned int DWC_SDHC_TBBCNT;

typedef union {
    unsigned int value;
    struct {
        unsigned debounce_count:24;
        unsigned :8;
    };
} DWC_SDHC_DEBNCE;

typedef unsigned int DWC_SDHC_USRID;

typedef unsigned int DWC_SDHC_VERID;

typedef union {
    unsigned int value;
    struct {
        unsigned card_type:1;
        unsigned num_cards:5;
        unsigned h_bus_type:1;
        unsigned h_data_width:3;
        unsigned h_addr_width:6;
        unsigned dma_interface:2;
        unsigned ge_dma_data_width:3;
        unsigned fifo_ram_inside:1;
        unsigned implement_hold_reg:1;
        unsigned set_clk_false_path:1;
        unsigned num_clk_divider:2;
        unsigned area_optimized:1;
        unsigned :5;
    };
} DWC_SDHC_HCON;

typedef union {
    unsigned int value;
    struct {
        unsigned vol_reg:16;
        unsigned ddr_reg:16;
    };
} DWC_SDHC_UHS_REG;                 // UHS-1 Register

typedef union {
    unsigned int value;
    struct {
        unsigned card_reset:16;
        unsigned :16;
    };
} DWC_SDHC_RST_N;                   // H/W Reset

//-------------------------------------------------------------------------------------------------

typedef union {
    unsigned int value;
    struct {
        unsigned swr:1;
        unsigned fb:1;
        unsigned dsl:5;
        unsigned de:1;
        unsigned pbl:3;
        unsigned :21;
    };
} DWC_SDHC_BMOD;                    // Bus Mode Register

typedef unsigned int DWC_SDHC_PLDMND;        // Poll Demand Register

typedef unsigned int DWC_SDHC_DBADDR;        // Descriptor List Base Address Register


#define DWC_SDHC_IDSTS_FSM_DMA_IDLE         0
#define DWC_SDHC_IDSTS_FSM_DMA_SUSPEND      1
#define DWC_SDHC_IDSTS_FSM_DESC_RD          2
#define DWC_SDHC_IDSTS_FSM_DESC_CHK         3
#define DWC_SDHC_IDSTS_FSM_DMA_RD_REQ_WAIT  4
#define DWC_SDHC_IDSTS_FSM_DMA_WR_REQ_WAIT  5
#define DWC_SDHC_IDSTS_FSM_DMA_RD           6
#define DWC_SDHC_IDSTS_FSM_DMA_WR           7
#define DWC_SDHC_IDSTS_FSM_DESC_CLOSE       8
typedef union {
    unsigned int value;
    struct {
        unsigned ti:1;              // Transmit Interrupt
        unsigned ri:1;              // Receive Interrupt
        unsigned fbe:1;             // Fatal Bus Error Interrupt
        unsigned :1;
        unsigned du:1;              // Descriptor Unavailable Interrupt
        unsigned ces:1;             // Card Error Summary
        unsigned :2;
        unsigned nis:1;             // Normal Interrupt Summary
        unsigned ais:1;             // Abnormal Interrupt Summary
        unsigned fbe_code:3;        // Fatal Bus Error Code
        unsigned fsm:4;             // DMAC FSM Present State
        unsigned :15;
    };
} DWC_SDHC_IDSTS;                   // Internal DMAC Status Register

typedef union {
    unsigned int value;
    struct {
        unsigned ti:1;              // Transmit Interrupt Enable
        unsigned ri:1;              // Receive Interrupt Enable
        unsigned fbe:1;             // Fatal Bus Error Enable
        unsigned :1;
        unsigned du:1;              // Descriptor Unavailable Interrupt Enable
        unsigned ces:1;             // Card Error Summary
        unsigned :2;
        unsigned nis:1;             // Normal Interrupt Summary Enable
        unsigned ais:1;             // Abnormal Interrupt Summary Enable
        unsigned :22;
    };
} DWC_SDHC_IDINTEN;                 // Internal DMAC Interrtupt Enable Register

typedef unsigned int DWC_SDHC_DSCADDR;       // Current Host Descriptor Address Register

typedef unsigned int DWC_SDHC_BUFADDR;       // Current Buffer Descriptor Address Register

//-------------------------------------------------------------------------------------------------

typedef union {
    unsigned int value;
    struct {
        unsigned card_rd_thr_en:1;  // Card Read Threshold Enable
        unsigned bsy_clr_int_en:1;  // Busy Clear Interrupt Generation
        unsigned card_wr_thr_en:1;  // Card Write Threshold Enable
        unsigned :13;
        unsigned card_threshold:12; // Card Threshold Size
        unsigned :4;
    };
} DWC_SDHC_CARD_THR_CTL;            // Card Threshold Control Register

typedef union {
    unsigned int value;
    struct {
        unsigned back_end_power:16;
        unsigned :16;
    };
} DWC_SDHC_BACK_END_POWER;          // Back-end Power Register

typedef union {
    unsigned int value;
    struct {
        unsigned mmc_volt_reg:16;
        unsigned clk_smpl_phase_ctrl:7;
        unsigned clk_drv_phase_ctrl:7;
        unsigned :2;
    };
} DWC_SDHC_UHS_REG_EXT;             // UHS Register

typedef union {
    unsigned int value;
    struct {
        unsigned half_start_bit:16;
        unsigned :15;
        unsigned hs400_mode:1;
    };
} DWC_SDHC_EMMC_DDR_REG;            // eMMC DDR Register

typedef unsigned int DWC_SDHC_ENABLE_SHIFT;

//-------------------------------------------------------------------------------------------------

typedef struct {
    DWC_SDHC_CTRL           ctrl;               // 0x000
    DWC_SDHC_PWREN          pwren;
    DWC_SDHC_CLKDIV         clkdiv;
    DWC_SDHC_CLKSRC         clksrc;
    DWC_SDHC_CLKENA         clkena;
    DWC_SDHC_TMOUT          tmout;
    DWC_SDHC_CTYPE          ctype;
    DWC_SDHC_BLKSIZ         blksiz;
    DWC_SDHC_BYTCNT         bytcnt;
    DWC_SDHC_INT            intmask;
    DWC_SDHC_CMDARG         cmdarg;
    DWC_SDHC_CMD            cmd;
    DWC_SDHC_RESP           resp[4];
    DWC_SDHC_INT            mintsts;
    DWC_SDHC_INT            rintsts;
    DWC_SDHC_STATUS         status;
    DWC_SDHC_FIFOTH         fifoth;
    DWC_SDHC_CDETECT        cdetect;
    DWC_SDHC_WRTPRT         wrtprt;
    DWC_SDHC_GPIO           gpio;
    DWC_SDHC_TCBCNT         tcbcnt;
    DWC_SDHC_TBBCNT         tbbcnt;
    DWC_SDHC_DEBNCE         debnce;
    DWC_SDHC_USRID          usrid;
    DWC_SDHC_VERID          verid;
    DWC_SDHC_HCON           hcon;
    DWC_SDHC_UHS_REG        uhs_reg;
    DWC_SDHC_RST_N          rst_n;              // 0x078
    unsigned :32;
    DWC_SDHC_BMOD           bmod;               // 0x080
    DWC_SDHC_PLDMND         pldmnd;
    DWC_SDHC_DBADDR         dbaddr;
    DWC_SDHC_IDSTS          idsts;
    DWC_SDHC_IDINTEN        idinten;
    DWC_SDHC_DSCADDR        dscaddr;
    DWC_SDHC_BUFADDR        bufaddr;            // 0x098
} DWC_SDHC_REGS;

typedef struct {
    DWC_SDHC_CARD_THR_CTL   card_thr_ctrl;      // 0x100
    DWC_SDHC_BACK_END_POWER back_end_power;
    DWC_SDHC_UHS_REG_EXT    uhs_reg_ext;
    DWC_SDHC_EMMC_DDR_REG   emmc_ddr_reg;
    DWC_SDHC_ENABLE_SHIFT   enable_shift;
} DWC_SDHC_EXT_REGS;

//-------------------------------------------------------------------------------------------------

#define DWC_SDHC_REG_CTRL               0x000
#define DWC_SDHC_REG_PWREN              0x004
#define DWC_SDHC_REG_CLKDIV             0x008
#define DWC_SDHC_REG_CLKSRC             0x00C
#define DWC_SDHC_REG_CLKENA             0x010
#define DWC_SDHC_REG_TMOUT              0x014
#define DWC_SDHC_REG_CTYPE              0x018
#define DWC_SDHC_REG_BLKSIZ             0x01C
#define DWC_SDHC_REG_BYTCNT             0x020
#define DWC_SDHC_REG_INTMASK            0x024
#define DWC_SDHC_REG_CMDARG             0x028
#define DWC_SDHC_REG_CMD                0x02C
#define DWC_SDHC_REG_RESP0              0x030
#define DWC_SDHC_REG_RESP1              0x034
#define DWC_SDHC_REG_RESP2              0x038
#define DWC_SDHC_REG_RESP3              0x03C
#define DWC_SDHC_REG_MINTSTS            0x040
#define DWC_SDHC_REG_RINTSTS            0x044
#define DWC_SDHC_REG_STATUS             0x048
#define DWC_SDHC_REG_FIFOTH             0x04C
#define DWC_SDHC_REG_CDETECT            0x050
#define DWC_SDHC_REG_WRTPRT             0x054
#define DWC_SDHC_REG_GPIO               0x058
#define DWC_SDHC_REG_TCBCNT             0x05C
#define DWC_SDHC_REG_TBBCNT             0x060
#define DWC_SDHC_REG_DEBNCE             0x064
#define DWC_SDHC_REG_USRID              0x068
#define DWC_SDHC_REG_VERID              0x06C
#define DWC_SDHC_REG_HCON               0x070
#define DWC_SDHC_REG_UHS_REG            0x074
#define DWC_SDHC_REG_RST_N              0x078

#define DWC_SDHC_REG_BMOD               0x080
#define DWC_SDHC_REG_PLDMND             0x084
#define DWC_SDHC_REG_DBADDR             0x088
#define DWC_SDHC_REG_IDSTS              0x08C
#define DWC_SDHC_REG_IDINTEN            0x090
#define DWC_SDHC_REG_DSCADDR            0x094
#define DWC_SDHC_REG_BUFADDR            0x098

#define DWC_SDHC_REG_CARD_THR_CTL       0x100
#define DWC_SDHC_REG_BACK_END_POWER     0x104
#define DWC_SDHC_REG_UHS_REG_EXT        0x108
#define DWC_SDHC_REG_EMMC_DDR_REG       0x10C
#define DWC_SDHC_REG_ENABLE_SHIFT       0x110

#define DWC_SDHC_REG_FIFO_BASE          0x200      /* FIFO ADDRESS */

//=================================================================================================

typedef union {
    unsigned int value;
    struct {
        unsigned :1;
        unsigned dic:1;         // Disable Interrupt on Completion (DIC)
        unsigned ld:1;          // Last Descriptor (LD)
        unsigned fs:1;          // First Descriptor (FS)
        unsigned ch:1;          // Second Address Chained (CH)
        unsigned er:1;          // End of Ring (ER)
        unsigned :24;
        unsigned ces:1;         // Card Error Summary (CES)
        unsigned own:1;         // Indicate the descriptor is owned by IDMAC
    };
} DWC_SDHC_IDMA_DES0;

typedef union {
    unsigned int value;
    struct {
        unsigned bs1:13;
        unsigned bs2:13;
        unsigned :6;
    };
} DWC_SDHC_IDMA_DES1;

typedef unsigned int DWC_SDHC_IDMA_DES2;
typedef unsigned int DWC_SDHC_IDMA_DES3;

typedef struct {
    DWC_SDHC_IDMA_DES0  des0;
    DWC_SDHC_IDMA_DES1  des1;
    DWC_SDHC_IDMA_DES2  des2;
    DWC_SDHC_IDMA_DES3  des3;
} DWC_SDHC_IDMA_DESCRIPTOR;

#endif /* __DEV_DWC_SDHC_H__ */
