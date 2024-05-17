/*
 * Designware APB Watchdog
 * Alan.REN
 *
 * Copyright 2021 NationalChip
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _DW_APB_WDT_H_
#define _DW_APB_WDT_H_

#include <stdint.h>

#include "hw/hw.h"
#include "hw/irq.h"
#include "hw/sysbus.h"
#include "hw/ptimer.h"

#include "dw_apb_wdt_regs.h"


typedef struct DwApbWdtState {
    SysBusDevice    parent_obj;

    MemoryRegion    iomem;
    qemu_irq        irq;

    QEMUTimer      *timer;

    int64_t         start;
    int64_t         timeout;

    WDT_CR          cr;         // control
    WDT_TORR        torr;       // timeout range

    uint32_t        stat;       // interrupt status
} DwApbWdtState;

#define TYPE_DW_APB_WDT "dw_apb_wdt"
#define DW_APB_WDT(obj) OBJECT_CHECK(DwApbWdtState, (obj), TYPE_DW_APB_WDT)

#endif /* _DW_APB_WDT_H_ */
