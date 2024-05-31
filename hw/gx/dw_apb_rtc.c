/*
 * Designware Real Time Clock
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
#include "migration/vmstate.h"
#include "sysemu/rtc.h"

#include "dw_apb_rtc.h"

#define LOG_TAG "[TREC.RTC] "
//#define LOG_DEBUG

#define RTC_BASE_FREQ       (32768)

#define RTC_CCVR            (0x00 / 4)      // current counter value register
#define RTC_CMR             (0x04 / 4)      // counter match register
#define RTC_CLR             (0x08 / 4)      // counter load register
#define RTC_CCR             (0x0C / 4)      // counter control register
#define RTC_STAT            (0x10 / 4)      // interrupt status register
#define RTC_RSTAT           (0x14 / 4)      // interrupt raw status register
#define RTC_EOI             (0x18 / 4)      // end of interrupt register
#define RTC_COMP_VERSION    (0x1C / 4)      // component version register
#define RTC_CPSR            (0x20 / 4)      // counter prescaler register
#define RTC_CPCVR           (0x24 / 4)      // current prescaler counter value register

static uint32_t _dwc_rtc_get_ccvr(DwcRtcState *rtc, int64_t offset)
{
    struct tm now;
    qemu_get_timedate(&now, offset);
    time_t seconds = mktimegm(&now);
    return seconds * RTC_BASE_FREQ / (rtc->prescaler + 1);
}

static void _dwc_rtc_update_irq(DwcRtcState *rtc)
{
    qemu_set_irq(rtc->irq, rtc->raw_state & (!rtc->control.rtc_mask) & rtc->control.rtc_ien);
}

static void _dwc_rtc_update_timer(DwcRtcState *rtc)
{
    /* start or stop match timer */
    int64_t delta = (int64_t)rtc->match - _dwc_rtc_get_ccvr(rtc, rtc->offset);
    if (delta > 0) {
        //printf("delta = %ld\n", delta);
        timer_mod(rtc->timer, qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL) + delta * 1000 * (rtc->prescaler + 1) / RTC_BASE_FREQ);
    }
    else {
        timer_del(rtc->timer);
    }
}

/*
 * tick handler
 */
static void _dwc_rtc_timeout(void *opaque)
{
    DwcRtcState *rtc = (DwcRtcState *)opaque;

    rtc->raw_state = 1;
    _dwc_rtc_update_irq(rtc);

    //int64_t now = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
    //printf("Now is %ld\n", now);

#ifdef LOG_DEBUG
    struct tm now;
    qemu_get_timedate(&now, 0);
    time_t seconds = mktimegm(&now);
    printf("Now is %ld\n", seconds);
#endif
}

static uint64_t _dwc_rtc_read(void *opaque, hwaddr offset, unsigned size)
{
    DwcRtcState *rtc = opaque;
    uint32_t reg = offset >> 2;
#ifdef LOG_DEBUG
    printf("%s(offset=%08lX)\n", __FUNCTION__, offset);
#endif

    switch (reg) {
    case RTC_CCVR:
        return _dwc_rtc_get_ccvr(rtc, rtc->offset);
    case RTC_CMR:
        return rtc->match;
    case RTC_CLR:
        return rtc->load;
    case RTC_CCR:
        return rtc->control.value;
    case RTC_STAT:
        return rtc->raw_state & (!rtc->control.rtc_mask);
    case RTC_RSTAT:
        return rtc->raw_state;
    case RTC_EOI:
        rtc->raw_state = 0;
        return 0;
    case RTC_COMP_VERSION:
        return 0x3230312A;
    case RTC_CPSR:
        return rtc->prescaler;
    case RTC_CPCVR:
        // provide current value of the internal prescaler counter
        return 0;
    }
    return 0;
}

static void _dwc_rtc_write(void *opaque, hwaddr offset, uint64_t value, unsigned size)
{
    DwcRtcState *rtc = opaque;
    uint32_t reg = offset >> 2;
#ifdef LOG_DEBUG
    printf("%s(offset=%08lX, value=%08lX)\n", __FUNCTION__, offset, value);
#endif

    switch (reg) {
    case RTC_CMR:
        rtc->match = (uint32_t)value;
        break;
    case RTC_CLR:
        rtc->load = (uint32_t)value;
        rtc->offset = ((int64_t)rtc->load - _dwc_rtc_get_ccvr(rtc, 0))  * (rtc->prescaler + 1) / RTC_BASE_FREQ;
        break;
    case RTC_CCR:
        rtc->control.value = (uint32_t)value;
        break;
    case RTC_CPSR:
        if (value)
            rtc->prescaler = (uint32_t)value;
        else
            printf(LOG_TAG"illegal CPSR value\n");
        break;
    }
    _dwc_rtc_update_timer(rtc);
    _dwc_rtc_update_irq(rtc);
}

static void _dwc_rtc_reset(DeviceState *d)
{
    DwcRtcState *rtc = DWC_RTC(d);

    rtc->offset = 0;

    rtc->match = 0;
    rtc->control.value = 0;
    rtc->control.rtc_en = 1;
    rtc->raw_state = 0;
    rtc->prescaler = RTC_BASE_FREQ - 1;
    rtc->load = _dwc_rtc_get_ccvr(rtc, 0);
}

static const MemoryRegionOps _dwc_rtc_ops = {
    .read = _dwc_rtc_read,
    .write = _dwc_rtc_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static const VMStateDescription _vmstate_dwc_rtc = {
    .name = TYPE_DWC_RTC,
    .version_id = 1,
    .fields = (VMStateField[]) {
        //VMSTATE_UINT32_ARRAY(reg, DwcRtcState, 0x28),
        VMSTATE_INT64(offset, DwcRtcState),
        VMSTATE_UINT32(match, DwcRtcState),
        VMSTATE_UINT32(load, DwcRtcState),
        VMSTATE_UINT32(control.value, DwcRtcState),
        VMSTATE_UINT32(raw_state, DwcRtcState),
        VMSTATE_UINT32(prescaler, DwcRtcState),
        VMSTATE_END_OF_LIST()
    }
};

static void _dwc_rtc_realize(DeviceState *dev, Error **errp)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
    DwcRtcState *s = DWC_RTC(dev);

    s->timer = timer_new_ms(QEMU_CLOCK_VIRTUAL, _dwc_rtc_timeout, s);

    sysbus_init_irq(sbd, &s->irq);

    memory_region_init_io(&s->iomem, OBJECT(s), &_dwc_rtc_ops, s,
                          "dwc-rtc", 0x28ULL);
    sysbus_init_mmio(sbd, &s->iomem);
}

static void _dwc_rtc_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = _dwc_rtc_realize;
    dc->vmsd = &_vmstate_dwc_rtc;
    dc->reset = _dwc_rtc_reset;
}

static const TypeInfo _dwc_rtc_info = {
    .name          = TYPE_DWC_RTC,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(DwcRtcState),
    .class_init    = _dwc_rtc_class_init,
};

static void _dwc_rtc_register_types(void)
{
    type_register_static(&_dwc_rtc_info);
}

type_init(_dwc_rtc_register_types)
