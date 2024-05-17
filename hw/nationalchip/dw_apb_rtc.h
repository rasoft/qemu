/*
 * Designware Real Time Clock
 * Alan.REN
 *
 * Copyright 2020 NationalChip
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _DWC_RTC_H_
#define _DWC_RTC_H_

#include <stdint.h>

#include "hw/hw.h"
#include "hw/irq.h"
#include "hw/sysbus.h"
#include "hw/ptimer.h"

typedef union {
    unsigned int value;
    struct {
        unsigned rtc_ien:1;
        unsigned rtc_mask:1;
        unsigned rtc_en:1;
        unsigned rtc_wen:1;
        unsigned rtc_psclr_en:1;
        unsigned rtc_prot_level:3;
        unsigned :24;
    };
} DWC_RTC_CCR;

typedef struct DwcRtcState {
    SysBusDevice parent_obj;

    MemoryRegion iomem;
    qemu_irq irq;

    QEMUTimer *timer;

    int64_t offset;

    uint32_t match;
    uint32_t load;
    DWC_RTC_CCR control;
    uint32_t raw_state;
    uint32_t prescaler;

} DwcRtcState;

#define TYPE_DWC_RTC "dwc.rtc"
#define DWC_RTC(obj) OBJECT_CHECK(DwcRtcState, (obj), TYPE_DWC_RTC)

#endif /* _DWC_RTC_H_ */
