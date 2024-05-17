/*
 * NationalChip TREC Project
 * Copyright (C) 2001-2021 NationalChip Co., Ltd
 * ALL RIGHTS RESERVED!
 *
 * dev_dw_apb_uart.h: UART
 *                 based on dw_apb_uart_db.pdf 3.08a
 *
 */

#ifndef __DEV_DW_APB_UART_H__
#define __DEV_DW_APB_UART_H__

// 0x00
typedef union {
    unsigned int value;
    struct {
        unsigned rbr:8;     // Receive Buffer Register
        unsigned :24;
    };
} DW_APB_UART_RBR;

typedef union {
    unsigned int value;
    struct {
        unsigned thr:8;     // Transmit Holding Register
        unsigned :24;
    };
} DW_APB_UART_THR;

typedef union {
    unsigned int value;
    struct {
        unsigned dll:8;     // Divisor Latch (Low)
        unsigned :24;
    };
} DW_APB_UART_DLL;

// 0x04
typedef union {
    unsigned int value;
    struct {
        unsigned dlh:8;     // Divisor Latch (High)
        unsigned :24;
    };
} DW_APB_UART_DLH;

typedef union {
    unsigned int value;
    struct {
        unsigned erbfi:1;   // Enable Received Data Available Interrupt             2nd
        unsigned etbei:1;   // Enable Transmit Holding Register Empty Interrupt     3rd
        unsigned elsi:1;    // Enable Receiver Line Status Interrupt                1st
        unsigned edssi:1;   // Enable Modem Status Interrupt                        4th
        unsigned :3;
        unsigned ptime:1;   // Program THRE Interrupt Mode Enable
        unsigned :24;
    };
} DW_APB_UART_IER;

// 0x08

#define DW_APB_UART_IIR_IID_MS              0x00    // Modem Status                 4th
#define DW_APB_UART_IIR_IID_NIP             0x01    // No Interrupt Pending
#define DW_APB_UART_IIR_IID_TE              0x02    // THR Empty                    3rd
#define DW_APB_UART_IIR_IID_RDA             0x04    // Received Data Available      2nd
#define DW_APB_UART_IIR_IID_RLS             0x06    // Receiver Line Status         1st
#define DW_APB_UART_IIR_IID_BD              0x07    // Busy Detect                  5th
#define DW_APB_UART_IIR_IID_CT              0x0C    // Character Timeout            2nd

#define DW_APB_UART_IIR_FIFOSE_ENABLED      0x11
#define DW_APB_UART_IIR_FIFOSE_DISABLED     0x00

typedef union {
    unsigned int value;
    struct {
        unsigned iid:4;     // Interrupt ID
        unsigned :2;
        unsigned fifose:2;  // FIFOs Enabled
        unsigned :24;
    };
} DW_APB_UART_IIR;

#define DW_APB_UART_FCR_TET_FE              0x00    // FIFO Empty
#define DW_APB_UART_FCR_TET_2C              0x01    // 2 Characters in the FIFO
#define DW_APB_UART_FCR_TET_QUARTER         0x02    // FIFO 1/4 Full
#define DW_APB_UART_FCR_TET_HALF            0x03    // FIFO 1/2 Full

#define DW_APB_UART_FCR_RT_1C               0x00    // 1 Characters in the FIFO
#define DW_APB_UART_FCR_RT_QUARTER          0x01    // FIFO 1/4 Full
#define DW_APB_UART_FCR_RT_HALF             0x02    // FIFO 1/2 Full
#define DW_APB_UART_FCR_RT_2L               0x03    // FIFO 2 Less than Full

typedef union {
    unsigned int value;
    struct {
        unsigned fifoe:1;   // FIFO Enable
        unsigned rfifor:1;  // RCVR FIFO Reset
        unsigned xfifor:1;  // XMIT FIFO Reset
        unsigned dmam:1;    // DMA Mode
        unsigned tet:2;     // TX Empty Trigger
        unsigned rt:2;      // RCVR Trigger
        unsigned :24;
    };
} DW_APB_UART_FCR;

// 0x0C

#define DW_APB_UART_LCR_DLS_5BITS           0x00
#define DW_APB_UART_LCR_DLS_6BITS           0x01
#define DW_APB_UART_LCR_DLS_7BITS           0x02
#define DW_APB_UART_LCR_DLS_8BITS           0x03

typedef union {
    unsigned int value;
    struct {
        unsigned dls:2;     // Data Length Select
        unsigned stop:1;    // Number of Stop Bits
        unsigned pen:1;     // Parity Enable
        unsigned eps:1;     // Even Parity Select
        unsigned :1;
        unsigned bc:1;      // Break Control Bit
        unsigned dlab:1;    // Divisor Latch Access Bit
        unsigned :24;
    };
} DW_APB_UART_LCR;

// 0x10
typedef union {
    unsigned int value;
    struct {
        unsigned dtr:1;     // Data Terminal Ready
        unsigned rts:1;     // Request to Send
        unsigned out1:1;    // OUT1
        unsigned out2:1;    // OUT2
        unsigned lb:1;      // Loop Back
        unsigned afce:1;    // Auto Flow Control Enable
        unsigned sire:1;    // SIR Mode Enable
        unsigned :25;
    };
} DW_APB_UART_MCR;

// 0x14
typedef union {
    unsigend int value;
    struct {
        unsigned dr:1;      // Data Ready Bit
        unsigned oe:1;      // Overrun Error Bit
        unsigned pe:1;      // Parity Error Bit
        unsigned fe:1;      // Frame Error Bit
        unsigned bi:1;      // Break Interrupt Bit
        unsigned thre:1;    // Transmit Holding Register Empty Bit
        unsigned temt:1;    // Transmitter Empty Bit
        unsigned rfe:1;     // Receiver FIFO Error Bit
        unsigned :24;
    };
} DW_APB_UART_LSR;

// 0x18
typedef union {
    unsigned int value;
    struct {
        unsigned dcts:1;    // Delta Clear to Send
        unsigned ddsr:1;    // Delta Data Set Ready
        unsigned teri:1;    // Trailing Edge of Ring Indicator
        unsigned ddcd:1;    // Delta Data Carrier Detect
        unsigned cts:1;     // Clear to Send
        unsigned dsr:1;     // Data Set Ready
        unsigned ri:1;      // Ring Indicator
        unsigned dcd:1;     // Data Carrier Detect
        unsigned :24; 
    };
} DW_APB_UART_MSR;

// 0x1C
typedef union {
    unsigned int value;
    struct {
        unsigned scr:8;
        unsigned :24;
    };
} DW_APB_UART_SCR;

// 0x20
typedef union {
    unsigned int value;
    struct {
        unsigned lpdll:8;
        unsigned :24; 
    };
} DW_APB_UART_LPDLL;

// 0x24
typedef union {
    unsigned int value;
    struct {
        unsigend lpdlh:8;
        unsigned :24;
    };
} DW_APB_UART_LPDLH;

// 0x70
typedef union {
    unsigned int value;
    struct {
        unsigned far:1;
        unsigned :31;
    };
} DW_APB_UART_FAT;

// 0x74
typedef union {
    unsigned int value;
    struct {
        unsigned tfr:8;     // Transmit FIFO Read
        unsigned :24;
    };
} DW_APB_UART_FTR;

// 0x78
typedef union {
    unsigned int value;
    struct {
        unsigned rfwd:8;    // Receive FIFO Write Data
        unsigned rfpe:1;    // Receive FIFO Parity Error
        unsigned rffe:1;    // Receive FIFO Framing Error
        unsigned :22;
    };
} DW_APB_UART_RFW;

// 0x7C
typedef union {
    unsigned int value;
    struct {
        unsigned busy:1;    // UART Busy
        unsigned tfnf:1;    // Transmit FIFO Not Full
        unsigned tfe:1;     // Transmit FIFO Empty
        unsigned rfne:1;    // Receive FIFO Not Empty
        unsigned rff:1;     // Receive FIFO Full
        unsigned :27;
    };
} DW_APB_UART_USR;

#define DW_APB_UART_REG_RBR         0x00        // R  LCR[7] == 0
#define DW_APB_UART_REG_THR         0x00        // W  LCR[7] == 0
#define DW_APB_UART_REG_DLL         0x00        // RW LCR[7] == 1
#define DW_APB_UART_REG_DLH         0x04        // RW LCR[7] == 1
#define DW_APB_UART_REG_IER         0x04        // RW LCR[7] == 0
#define DW_APB_UART_REG_IIR         0x08        // R
#define DW_APB_UART_REG_FCR         0x08        // W
#define DW_APB_UART_REG_LCR         0x0C        // RW
#define DW_APB_UART_REG_MCR         0x10        // RW
#define DW_APB_UART_REG_LSR         0x14        // R
#define DW_APB_UART_REG_MSR         0x18        // R
#define DW_APB_UART_REG_SCR         0x1C        // RW
#define DW_APB_UART_REG_LPDLL       0x20
#define DW_APB_UART_REG_LPDLH       0x24

#define DW_APB_UART_REG_SRBR0       0x30
#define DW_APB_UART_REG_STHR0       0x30

#define DW_APB_UART_REG_FAR         0x70
#define DW_APB_UART_REG_TFR         0x74
#define DW_APB_UART_REG_RFW         0x78
#define DW_APB_UART_REG_USR         0x7C
#define DW_APB_UART_REG_TFL         0x80
#define DW_APB_UART_REG_RFL         0x84
#define DW_APB_UART_REG_SRR         0x88
#define DW_APB_UART_REG_SRTS        0x8C
#define DW_APB_UART_REG_SBCR        0x90
#define DW_APB_UART_REG_SDMAM       0x94
#define DW_APB_UART_REG_SFE         0x98
#define DW_APB_UART_REG_SRT         0x9C
#define DW_APB_UART_REG_STET        0xA0
#define DW_APB_UART_REG_HTX         0xA4
#define DW_APB_UART_REG_DMASA       0xA8

#define DW_APB_UART_REG_CPR         0xF4
#define DW_APB_UART_REG_UCV         0xF8
#define DW_APB_UART_REG_CTR         0xFC

#endif /* __DEV_DW_APB_UART_H__ */
