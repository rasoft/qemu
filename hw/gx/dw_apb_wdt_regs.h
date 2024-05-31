/*
 * NationalChip TREC Project
 * Copyright (C) 2001-2021 NationalChip Co., Ltd
 * ALL RIGHTS RESERVED!
 *
 * dev_dw_apb_wdt.h: Designware DW_apb_wdt
 *                 based on dw_apb_wdt.pdf 1.11a
 *
 */

#ifndef __DEV_DW_APB_WDT_H__
#define __DEV_DW_APB_WDT_H__

typedef union {
    unsigned int value;
    struct {
        unsigned wdt_en:1;
        unsigned rmod:1;        // Response Mode
        unsigned rpl:3;         // Reset Pulse Length
        unsigned no_name:1;
        unsigned :26;
    };
} WDT_CR;

typedef union {
    unsigned int value;
    struct {
        unsigned top:4;
        unsigned top_init:4;
        unsigned :24;
    };
} WDT_TORR;

typedef union {
    unsigned int value;
    struct {
        unsigned ccvr:32;
    };
} WDT_CCVR;

#define WDT_CRR_CMD_RESTART 0x76
typedef union {
    unsigned int value;
    struct {
        unsigned crr:8;
        unsigned :24;
    };
} WDT_CRR;

typedef union {
    unsigned int value;
    struct {
        unsigned stat:1;
        unsigned :31;
    };
} WDT_STAT;

typedef union {
    unsigned int value;
    struct {
        unsigned eoi:1;
        unsigned :31;
    };
} WDT_EOI;

typedef union {
    unsigned int value;
    struct {
        unsigned prot_level:3;  // Protection Level
        unsigned :29;
    };
} WDT_PROT_LEVEL;

typedef union {
    unsigned int value;
    struct {
        unsigned user_top_max:32;
    };
} WDT_COMP_PARAM_5;

typedef union {
    unsigned int value;
    struct {
        unsigned user_top_init_max:32;
    };
} WDT_COMP_PARAM_4;

typedef union {
    unsigned int value;
    struct {
        unsigned top_rst:32;
    };
} WDT_COMP_PARAM_3;

typedef union {
    unsigned int value;
    struct {
        unsigned cnt_rst:32;
    };
} WDT_COMP_PARAM_2;

typedef union {
    unsigned int value;
    struct {
        unsigned wdt_always_en:1;
        unsigned wdt_dflt_rmod:1;
        unsigned wdt_dual_top:1;
        unsigned wdt_hc_rmod:1;
        unsigned wdt_hc_rpl:1;
        unsigned wdt_hc_top:1;
        unsigned wdt_use_fix_top:1;
        unsigned wdt_pause:1;
        unsigned apb_data_width:2;
        unsigned wdt_dflt_rpl:3;
        unsigned :3;
        unsigned wdt_dflt_top:4;
        unsigned wdt_dflt_top_init:4;
        unsigned wdt_cnt_width:5;
        unsigned :3;
    };
} WDT_COMP_PARAM_1;


#define WDT_VERSION_ID  0x3230312A
typedef union {
    unsigned int value;
    struct {
        unsigned version:32;
    };
} WDT_COMP_VERSION;

#define WDT_TYPE_ID     0x44570120
typedef union {
    unsigned int value;
    struct {
        unsigned comp_type:32;
    };
} WDT_COMP_TYPE;

typedef struct {
    WDT_CR          cr;
    WDT_TORR        torr;
    WDT_CCVR        ccvr;
    WDT_CRR         crr;
    WDT_STAT        stat;
    WDT_EOI         eoi;
    unsigned       :32;
    WDT_PROT_LEVEL  prot_level;
} WDT_REGS;

#define WDT_REG_CR              0x00
#define WDT_REG_TORR            0x04
#define WDT_REG_CCVR            0x08
#define WDT_REG_CRR             0x0C
#define WDT_REG_STAT            0x10
#define WDT_REG_EOI             0x14

#define WDT_REG_PROT_LEVEL      0x1C

#define WDT_REG_COMP_PARAM_5    0xE4
#define WDT_REG_COMP_PARAM_4    0xE8
#define WDT_REG_COMP_PARAM_3    0xEC
#define WDT_REG_COMP_PARAM_2    0xF0
#define WDT_REG_COMP_PARAM_1    0xF4
#define WDT_REG_COMP_VERSION    0xF8
#define WDT_REG_COMP_TYPE       0xFC

#endif /* __DEV_DW_APB_WDT_H__ */
