/*
 * NationalChip TREC Project
 * Copyright (C) 2001-2020 NationalChip Co., Ltd
 * ALL RIGHTS RESERVED!
 *
 * dev_dwc_ssic.h: Synchronous Serial Interface
 *
 */

#ifndef __DEV_DWC_SSIC_H__
#define __DEV_DWC_SSIC_H__

// Control Register 0
typedef union {
    unsigned int value;
    struct {
        unsigned dfs:5;             // R/W Data Frame Size
        unsigned :1;
        unsigned frf:2;             // Frame Format
        unsigned scph:1;            // Serial Clock Phase
        unsigned scpol:1;           // Serial Clock Polarity
        unsigned tmod:2;            // Transfer Mode
        unsigned slv_oe:1;          // Slave Output Enable
        unsigned srl:1;             // Shift Register Loop
        unsigned sste:1;            // Slave Select Toggle Enable
        unsigned :1;
        unsigned cfs:4;             // Control Frame Size
        unsigned :2;
        unsigned spi_frf:2;         // SPI Frame Format
        unsigned spi_hyperbus_en:1; // SPI Hyperbus Frame Format Enable
        unsigned :6;
        unsigned ssi_is_mst:1;      // Is Master
    };
} DWC_SSIC_CTRLR0;

// Control Register 1
typedef union {
    unsigned int value;
    struct {
        unsigned ndf:16;            // Number of Data Frames
        unsigned :16;
    };
} DWC_SSIC_CTRLR1;

// SSI Enable Register
typedef union {
    unsigned int value;
    struct {
        unsigned ssic_en:1;         // SSI Enable
        unsigned :31;
    };
} DWC_SSIC_SSIENR;

// Microwire Control Register
typedef union {
    unsigned int value;
    struct {
        unsigned mwmod:1;           // Microwire Transfer Mode
        unsigned mod:1;             // Microwire Control
        unsigned mhs:1;             // Microwire Handshaking
        unsigned :29;
    };
} DWC_SSIC_MWCR;

// Slave Enable Register
typedef union {
    unsigned int value;
    struct {
        unsigned ser:16;
        unsigned :16;
    };
} DWC_SSIC_SER;

// Baud Register
// valid only when DWC_SSI is configured as a master device
typedef union {
    unsigned int value;
    struct {
        unsigned :1;
        unsigned sckdv:15;          // SSI Clock Divider
        unsigned :16;
    };
} DWC_SSIC_BAUDR;

// Transmit FIFO Threshold Level
// controls the threshold value for the transmit FIFO memory
typedef union {
    unsigned int value;
    struct {
        unsigned tft:16;            // Transmit FIFO Threshold
        unsigned txfthr:16;         // Transfer start FIFO level
    };
} DWC_SSIC_TXFTLR;

// Receive FIFO Threshold Level
// controls the threshold value for the receive FIFO memory
typedef union {
    unsigned int value;
    struct {
        unsigned rft:16;            // Receive FIFO Threshold
        unsigned :16;
    };
} DWC_SSIC_RXFTLR;

// Transmit FIFO Level Register
// contains the number of valid data entries in the transmit FIFO memory
typedef union {
    unsigned int value;
    struct {
        unsigned txtfl:16;          // Transmit FIFO Level
        unsigned :16;
    };
} DWC_SSIC_TXFLR;

// Receive FIFO Level Register
typedef union {
    unsigned int value;
    struct {
        unsigned rxtfl:16;          // Receive FIFO Level
        unsigned :16;
    };
} DWC_SSIC_RXFLR;

// Status Register
typedef union {
    unsigned int value;
    struct {
        unsigned busy:1;            // SSI Busy Flag
        unsigned tfnf:1;            // Transmit FIFO Not Full
        unsigned tfe:1;             // Transmit FIFO Empty
        unsigned rfne:1;            // Receive FIFO Not Empty
        unsigned rff:1;             // Receive FIFO Full
        unsigned txe:1;             // Transmission Error
        unsigned dcol:1;            // Data Collision Error
        unsigned :25;
    };
} DWC_SSIC_SR;

// Interrupt Mask Register
typedef union {
    unsigned int value;
    struct {
        unsigned txeim:1;           // Transmit FIFO Empty Interrupt Mask
        unsigned txoim:1;           // Transmit FIFO Overflow Interrupt Mask
        unsigned rxuim:1;           // Receive FIFO Underflow Interrupt Mask
        unsigned rxoim:1;           // Receive FIFO Overflow Interrupt Mask
        unsigned rxfim:1;           // Receive FIFO Full Interrupt Mask
        unsigned mstim:1;           // Multi-Master Contention Interrupt Mask
        unsigned xrxoim:1;          // XIP Receive FIFO Overflow Interrupt Mask
        unsigned :25;
    };
} DWC_SSIC_IMR;

// Interrupt Status Register
typedef union {
    unsigned int value;
    struct {
        unsigned txeis:1;
        unsigned txois:1;
        unsigned rxuis:1;
        unsigned rxois:1;
        unsigned rxfis:1;
        unsigned mstis:1;
        unsigned xrxois:1;
        unsigned :25;
    };
} DWC_SSIC_ISR;

// Raw Interrupt Status Register
typedef union {
    unsigned int value;
    struct {
        unsigned txeir:1;
        unsigned txoir:1;
        unsigned rxuir:1;
        unsigned rxoir:1;
        unsigned rxfir:1;
        unsigned mstir:1;
        unsigned xrxoir:1;
        unsigned :25;
    };
} DWC_SSIC_RISR;

// Transmit FIFO Overflow Interrupt Clear Register
typedef union {
    unsigned int value;
    struct {
        unsigned txoicr:1;          // RO
        unsigned :31;
    };
} DWC_SSIC_TXOICR;

// Receive FIFO Overflow Interrupt Clear Register
typedef union {
    unsigned int value;
    struct {
        unsigned rxoicr:1;          // RO
        unsigned :31;
    };
} DWC_SSIC_RXOICR;

// Receive FIFO Underflow Interrupt Clear Register
typedef union {
    unsigned int value;
    struct {
        unsigned rxuicr:1;          // RO
        unsigned :31;
    };
} DWC_SSIC_RXUICR;

// Multi-Master Interrupt Clear Register
typedef union {
    unsigned int value;
    struct {
        unsigned msticr:1;          // RO
        unsigned :31;
    };
} DWC_SSIC_MSTICR;

// Interrupt Clear Register
typedef union {
    unsigned int value;
    struct {
        unsigned icr:1;             // RO
        unsigned :31;
    };
} DWC_SSIC_ICR;

// DMA Control Register
typedef union {
    unsigned int value;
    struct {
        unsigned rdmae:1;           // Receive DMA Enable
        unsigned tdmae:1;           // Transmit DMA Enable
        unsigned :30;
    };
} DWC_SSIC_DMACR;

// DMA Transmit Data Level
typedef union {
    unsigned int value;
    struct {
        unsigned dmatdl:16;         // Transmit Data Level
        unsigned :16;
    };
} DWC_SSIC_DMATDLR;

// DMA Transmit Receive Data Level
typedef union {
    unsigned int value;
    struct {
        unsigned dmardl:16;         // Receive Data Level
        unsigned :16;
    };
} DWC_SSIC_DMARDLR;

// Identification Register
typedef union {
    unsigned int value;
    struct {
        unsigned idcode:32;         // Identification Code
    };
} DWC_SSIC_IDR;

typedef union {
    unsigned int value;
    struct {
        unsigned ssic_comp_version:32;  // "2.01"
    };
} DWC_SSIC_VERSION_ID;

// DRx
typedef unsigned int DWC_SSIC_DRx;

// RX Sample Delay Register
typedef union {
    unsigned int value;
    struct {
        unsigned rsd:8;             // Receive Data (rxd) Sample Delay
        unsigned :8;
        unsigned se:1;              // Receive Data (rxd) Sampling Edge
        unsigned :15;
    };
} DWC_SSIC_RX_SAMPLE_DELAY;

// SPI Control Register
typedef union {
    unsigned int value;
    struct {
        unsigned trans_type:2;      // Address and instruction transfer format
        unsigned addr_l:4;          // Length of Address to be transmitted
        unsigned :1;
        unsigned xip_md_bit_en:1;   // Mode bits enable in XIP mode
        unsigned inst_l:2;          // Dual/Quad/Octal mode instruction length in bits
        unsigned :1;
        unsigned wait_cycles:5;     // Wait cycles in Dual/Qual/Octal mode between control frames transmit and data reception
        unsigned spi_ddr_en:1;      // SPI DDR Enable bit
        unsigned inst_ddr_en:1;     // Instruction DDR Enable bit
        unsigned spi_rxds_en:1;     // Read data strobe enable bit
        unsigned xip_dfs_hc:1;      // Fix DFS for XIP transfer
        unsigned xip_inst_en:1;     // XIP instruction enable bit
        unsigned ssic_xip_cont_xfer_en:1;   // Enable continuous transfer in XIP mode
        unsigned :2;
        unsigned spi_dm_en:1;       // SPI data mask enable bit
        unsigned spi_rxds_sig_en:1; // Enable rxds signaling during address and command phase of Hyperbus transfer
        unsigned xip_mbl:2;         // XIP mode bits length
        unsigned :1;
        unsigned xip_prefetch_en:1; // Enable XIP pre-fetch functionality
        unsigned clk_stretch_en:1;  // Enable clock stretching capability in SPI transfer
        unsigned :1;
    };
} DWC_SSIC_SPI_CTRLR0;

// Transmit Drive Edge Register
typedef union {
    unsigned int value;
    struct {
        unsigned tde:8;             // TXD Drive edge
        unsigned :24;
    };
} DWC_SSIC_DDR_DRIVER_EDGE;

typedef union {
    unsigned int value;
    struct {
        unsigned xip_md_bits:16;    // XIP mode bits
        unsigned :16;
    };
} DWC_SSIC_XIP_MODE_BITS;

typedef struct {
    DWC_SSIC_CTRLR0         ctrlr0;
    DWC_SSIC_CTRLR1         ctrlr1;
    DWC_SSIC_SSIENR         ssienr;
    DWC_SSIC_MWCR           mwcr;
    DWC_SSIC_SER            ser;
    DWC_SSIC_BAUDR          baudr;
    DWC_SSIC_TXFTLR         txftlr;
    DWC_SSIC_RXFTLR         rxftlr;
    DWC_SSIC_TXFLR          txflr;
    DWC_SSIC_RXFLR          rxflr;
    DWC_SSIC_SR             sr;
    DWC_SSIC_IMR            imr;
    DWC_SSIC_ISR            isr;
    DWC_SSIC_RISR           risr;
    DWC_SSIC_TXOICR         txoicr;
    DWC_SSIC_RXOICR         rxoicr;
    DWC_SSIC_RXUICR         rxuicr;
    DWC_SSIC_MSTICR         msticr;
    DWC_SSIC_ICR            icr;
    DWC_SSIC_DMACR          dmacr;
    DWC_SSIC_DMATDLR        dmatdlr;
    DWC_SSIC_DMARDLR        dmardlr;
    DWC_SSIC_IDR            idr;
    DWC_SSIC_VERSION_ID     version_id;
} DWC_SSIC_REGS;

#define DWC_SSIC_REG_CTRLR0             0x000
#define DWC_SSIC_REG_CTRLR1             0x004
#define DWC_SSIC_REG_SSIENR             0x008
#define DWC_SSIC_REG_MWCR               0x00C
#define DWC_SSIC_REG_SER                0x010
#define DWC_SSIC_REG_BAUDR              0x014
#define DWC_SSIC_REG_TXFTLR             0x018
#define DWC_SSIC_REG_RXFTLR             0x01C
#define DWC_SSIC_REG_TXFLR              0x020
#define DWC_SSIC_REG_RXFLR              0x024
#define DWC_SSIC_REG_SR                 0x028
#define DWC_SSIC_REG_IMR                0x02C
#define DWC_SSIC_REG_ISR                0x030
#define DWC_SSIC_REG_RISR               0x034
#define DWC_SSIC_REG_TXOICR             0x038
#define DWC_SSIC_REG_RXOICR             0x03C
#define DWC_SSIC_REG_RXUICR             0x040
#define DWC_SSIC_REG_MSTICR             0x044
#define DWC_SSIC_REG_ICR                0x048
#define DWC_SSIC_REG_DMACR              0x04C
#define DWC_SSIC_REG_DMATDLR            0x050
#define DWC_SSIC_REG_DMARDLR            0x054
#define DWC_SSIC_REG_IDR                0x058
#define DWC_SSIC_REG_VERSION_ID         0x05C

#define DWC_SSIC_REG_DR00               0x060
#define DWC_SSIC_REG_DR35               0x0EC

#define DWC_SSIC_REG_RX_SAMPLE_DELAY    0x0F0
#define DWC_SSIC_REG_SPI_CTRLR0         0x0F4
#define DWC_SSIC_REG_DDR_DRIVE_EDGE     0x0F8
#define DWC_SSIC_REG_XIP_MODE_BITS      0x0FC

//=================================================================================================

// XIP INCR trasnfer opcode
typedef union {
    unsigned int value;
    struct {
        unsigned incr_inst:16;
        unsigned :16;
    };
} DWC_SSIC_XIP_INCR_INST;

// XIP WRAP transfer opcode
typedef union {
    unsigned int value;
    struct {
        unsigned wrap_inst:16;
        unsigned :16;
    };
} DWC_SSIC_XIP_WRAP_INST;

// XIP Control Register
typedef union {
    unsigned int value;
    struct {
        unsigned frf:2;             // SPI Frame Format
        unsigned trans_type:2;      // Address and instruction transfer format
        unsigned addr_l:4;          // Length of Address to be transmitted
        unsigned :1;
        unsigned inst_l:2;          // Dual/Quad/Octal mode instruction length in bits
        unsigned :1;
        unsigned md_bits_en:1;      // Mode bits enable in XIP mode
        unsigned wait_cycles:5;     // Wait cycle in Dual/Quad/Octal mode between control frames transmit and data reception
        unsigned dfs_hc:1;          // Fix DFS for XIP transfers
        unsigned ddr_en:1;          // SPI DDR Enable bit
        unsigned inst_ddr_en:1;     // Instruction DDR Enable
        unsigned rxds_en:1;         // Read data strobe enable bit
        unsigned inst_en:1;         // XIP instruction enable bit
        unsigned cont_xfer_en:1;    // Enable continuous transfer in XIP mode
        unsigned xip_hyperbus_en:1; // SPI Hyperbus Frame format enable for XIP transfers
        unsigned rxds_sig_en:1;     // Enabel rxds signaling
        unsigned xip_mbl:2;         // XIP mode bits length
        unsigned :1;
        unsigned xip_prefetch_en:1; // Enable XIP pre-fetch functionality
        unsigned :2;
    };
} DWC_SSIC_XIP_CTRL;

// Slave Enable Register
typedef union {
    unsigned int value;
    struct {
        unsigned ser:16;
        unsigned :16;
    };
} DWC_SSIC_XIP_SER;

// XIP Receive FIFO Overflow Interrupt Clear Register
typedef union {
    unsigned int value;
    struct {
        unsigned xrxoicr:1;
        unsigned :31;
    };
} DWC_SSIC_XIP_XRXOICR;

// XIP timeout register for continuous transfer
typedef union {
    unsigned int value;
    struct {
        unsigned xtoc:8;
        unsigned :24;
    };
} DWC_SSIC_XIP_CNT_TIME_OUT;

#define DWC_SSIC_REG_XIP_INCR_INST      0x100
#define DWC_SSIC_REG_XIP_WRAP_INST      0x104
#define DWC_SSIC_REG_XIP_CTRL           0x108
#define DWC_SSIC_REG_XIP_SER            0x10C
#define DWC_SSIC_REG_XIP_XRXOICR        0x110
#define DWC_SSIC_REG_XIP_CNT_TIME_OUT   0x114

#endif /* __DEV_DWC_SSIC_H__ */
