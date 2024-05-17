/*
 * NationalChip Real Time Clock
 * Alan.REN
 *
 * Copyright 2024 NationalChip
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/timer.h"
#include "qemu/cutils.h"
#include "sysemu/rtc.h"
#include "migration/vmstate.h"

#include "gx_rtc.h"

#define LOG_TAG "[RTC] "
// #define LOG_DEBUG

#define RTC_BASE_FREQ       (32768)

#define RTC_CON             (0x00 / 4)
#define RTC_IER             (0x04 / 4)
#define RTC_ISR             (0x08 / 4)
#define RTC_DIV             (0x0C / 4)
#define RTC_TICK0           (0x10 / 4)
#define RTC_TICK1           (0x88 / 4)

#define RTC_ALARM0          (0x14 / 4)
#define RTC_ALARM1          (0x3C / 4)

#define RTC_ALARM_CON       (0x00 / 4)
#define RTC_ALARM_US        (0x04 / 4)
#define RTC_ALARM_MS        (0x08 / 4)
#define RTC_ALARM_SEC       (0x0C / 4)
#define RTC_ALARM_MIN       (0x10 / 4)
#define RTC_ALARM_HOUR      (0x14 / 4)
#define RTC_ALARM_WEEK      (0x18 / 4)
#define RTC_ALARM_DATE      (0x1C / 4)
#define RTC_ALARM_MON       (0x20 / 4)
#define RTC_ALARM_YEAR      (0x24 / 4)


#define RTC_SNAP            (0x8C / 4)

//=================================================================================================

static void _gx_rtc_update_irq(GxRtcState *rtc)
{
    // qemu_set_irq(rtc->irq, rtc->raw_state & (!rtc->control.rtc_mask) & rtc->control.rtc_ien);
}

static void _gx_rtc_update_timer(GxRtcState *rtc)
{
    /* start or stop match timer */
    // int64_t delta = (int64_t)rtc->match - _dwc_rtc_get_ccvr(rtc, rtc->offset);
    // if (delta > 0) {
    //     //printf("delta = %ld\n", delta);
    //     timer_mod(rtc->timer, qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL) + delta * 1000 * (rtc->prescaler + 1) / RTC_BASE_FREQ);
    // }
    // else {
    //     timer_del(rtc->timer);
    // }
}

/*
 * tick handler
 */
static void _gx_rtc_timeout(void *opaque)
{
    GxRtcState *rtc = (GxRtcState *)opaque;

    rtc->raw_state = 1;
    _gx_rtc_update_irq(rtc);

    //int64_t now = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
    //printf("Now is %ld\n", now);

#ifdef LOG_DEBUG
    struct tm now;
    qemu_get_timedate(&now, 0);
    time_t seconds = mktimegm(&now);
    printf("Now is %ld\n", seconds);
#endif
}

static uint64_t _gx_rtc_read(void *opaque, hwaddr offset, unsigned size)
{
    GxRtcState *rtc = (GxRtcState *)opaque;
    uint32_t n = offset >> 2;
    uint32_t ret = rtc->regs[n];
#ifdef LOG_DEBUG
    printf("%s(offset=%08lX)\n", __FUNCTION__, offset);
#endif

    switch (n) {
    case RTC_TICK0:
        ret = (qemu_clock_get_us(QEMU_CLOCK_VIRTUAL) - rtc->tick_offset[0]) / 10;
        break;
    case RTC_TICK1:
        ret = (qemu_clock_get_us(QEMU_CLOCK_VIRTUAL) - rtc->tick_offset[1]) / 10;
        break;
    default:
        break;
    }
    return ret;
}

static void _gx_rtc_write(void *opaque, hwaddr offset, uint64_t value, unsigned size)
{
    GxRtcState *rtc = (GxRtcState *)opaque;
    uint32_t n = offset >> 2;
#ifdef LOG_DEBUG
    printf("%s(offset=%08lX, value=%08lX)\n", __FUNCTION__, offset, value);
#endif

    switch(n) {
    case RTC_TICK0:
        rtc->tick_offset[0] = qemu_clock_get_us(QEMU_CLOCK_VIRTUAL) - value * 10;
        break;
    case RTC_TICK1:
        rtc->tick_offset[1] = qemu_clock_get_us(QEMU_CLOCK_VIRTUAL) - value * 10;
        break;
    default:
        rtc->regs[n] = value;
        break;
    }

    _gx_rtc_update_timer(rtc);
    _gx_rtc_update_irq(rtc);
}

static void _gx_rtc_reset(DeviceState *d)
{
    GxRtcState *rtc = GX_RTC(d);

    rtc->offset = 0;

    rtc->match = 0;
    // rtc->control.value = 0;
    // rtc->control.rtc_en = 1;
    rtc->raw_state = 0;
    rtc->prescaler = RTC_BASE_FREQ - 1;
}

static const MemoryRegionOps _gx_rtc_ops = {
    .read = _gx_rtc_read,
    .write = _gx_rtc_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static const VMStateDescription _vmstate_gx_rtc = {
    .name = TYPE_GX_RTC,
    .version_id = 1,
    .fields = (VMStateField[]) {
        //VMSTATE_UINT32_ARRAY(reg, GxRtcState, 0x28),
        VMSTATE_INT64(offset, GxRtcState),
        VMSTATE_UINT32(match, GxRtcState),
        VMSTATE_UINT32(load, GxRtcState),
        // VMSTATE_UINT32(control.value, GxRtcState),
        VMSTATE_UINT32(raw_state, GxRtcState),
        VMSTATE_UINT32(prescaler, GxRtcState),
        VMSTATE_END_OF_LIST()
    }
};

static void _gx_rtc_realize(DeviceState *dev, Error **errp)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
    GxRtcState *s = GX_RTC(dev);

    s->timer = timer_new_ms(QEMU_CLOCK_VIRTUAL, _gx_rtc_timeout, s);

    sysbus_init_irq(sbd, &s->irq);

    memory_region_init_io(&s->iomem, OBJECT(s), &_gx_rtc_ops, s,
                          "gx_rtc", sizeof(s->regs));
    sysbus_init_mmio(sbd, &s->iomem);
}

static void _gx_rtc_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = _gx_rtc_realize;
    dc->vmsd = &_vmstate_gx_rtc;
    dc->reset = _gx_rtc_reset;
}

static const TypeInfo _dwc_rtc_info = {
    .name          = TYPE_GX_RTC,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(GxRtcState),
    .class_init    = _gx_rtc_class_init,
};

static void _dwc_rtc_register_types(void)
{
    type_register_static(&_dwc_rtc_info);
}

type_init(_dwc_rtc_register_types)
