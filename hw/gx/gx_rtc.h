/*
 * NationalChip Real Time Clock
 * Alan.REN
 *
 * Copyright 2024 NationalChip
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _GX_RTC_H_
#define _GX_RTC_H_

#include <stdint.h>

#include "hw/hw.h"
#include "hw/irq.h"
#include "hw/sysbus.h"
#include "hw/ptimer.h"

typedef struct GxRtcState {
    SysBusDevice parent_obj;

    MemoryRegion iomem;
    qemu_irq irq;

    QEMUTimer *timer;

    int64_t offset;

    uint32_t match;
    uint32_t load;
    uint32_t raw_state;
    uint32_t prescaler;
    uint32_t regs[36];

    uint64_t tick_offset[2];
    uint64_t alarm_offset[2];
} GxRtcState;

#define TYPE_GX_RTC "gx_rtc"
#define GX_RTC(obj) OBJECT_CHECK(GxRtcState, (obj), TYPE_GX_RTC)

#endif /* _DWC_RTC_H_ */
