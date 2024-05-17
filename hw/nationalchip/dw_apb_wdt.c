/*
 * Designware APB Watchdog
 * Alan.REN
 *
 * Copyright 2021 NationalChip
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
// #include "qemu-common.h"
#include "qemu/log.h"
#include "qemu/timer.h"
#include "qemu/cutils.h"
#include "migration/vmstate.h"

#include "dw_apb_wdt.h"

#define LOG_TAG "[TREC.WDT] "
//#define LOG_DEBUG

#define WDT_BASE_FREQ   (1000000)

static const WDT_COMP_PARAM_1 _wdt_comp_param_1 = {
    .wdt_always_en      = 0,
    .wdt_dflt_rmod      = 1,
    .wdt_dual_top       = 0,
    .wdt_hc_rmod        = 0,
    .wdt_hc_rpl         = 0,
    .wdt_hc_top         = 0,
    .wdt_use_fix_top    = 1,
    .wdt_pause          = 0,
    .apb_data_width     = 2,    // APB Data Width is 32bit
    .wdt_dflt_rpl       = 0,    // 2 pclk cycles
    .wdt_dflt_top       = 0,
    .wdt_dflt_top_init  = 0,
    .wdt_cnt_width      = 16,   // Counter width is 32
};

static void _dw_apb_wdt_update_irq(DwApbWdtState *wdt)
{
    qemu_set_irq(wdt->irq, wdt->stat);
}

static void _dw_apb_wdt_reset_timer(DwApbWdtState *wdt)
{
    if (wdt->cr.wdt_en) {
        wdt->start = qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL);
        timer_mod(wdt->timer, wdt->start + wdt->timeout);
    }
}

static uint32_t _dw_apb_wdt_get_ccvr(DwApbWdtState *wdt)
{
    return (wdt->start + wdt->timeout - qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL)) * WDT_BASE_FREQ / 1000;
}

/*
 * timeout handler
 */
static void _dw_apb_wdt_timeout(void *opaque)
{
    DwApbWdtState *wdt = (DwApbWdtState *)opaque;

    if (wdt->cr.rmod) { // Interrupt Mode
        if (wdt->stat) {
            printf(LOG_TAG"Mode 1: System Reset!\n");
            // TODO
        }
        else {
            printf(LOG_TAG"Mode 1: Invoke Interrupt!\n");
            wdt->stat = 1;
            _dw_apb_wdt_update_irq(wdt);
            _dw_apb_wdt_reset_timer(wdt);
        }
    }
    else {  // Reset Mode
        printf(LOG_TAG"Mode 0: System Reset!\n");
        // TODO
    }
}

static uint64_t _dw_apb_wdt_read(void *opaque, hwaddr offset, unsigned size)
{
    DwApbWdtState *wdt = (DwApbWdtState *)opaque;

#ifdef LOG_DEBUG
    printf("%s(offset=%08lX)\n", __FUNCTION__, offset);
#endif
    if (offset % 4 != 0)
        return 0;

    switch (offset) {
    case WDT_REG_CR:
        return wdt->cr.value;
    case WDT_REG_TORR:
        return wdt->torr.value;
    case WDT_REG_CCVR:
        return _dw_apb_wdt_get_ccvr(wdt);
    case WDT_REG_STAT:
        return wdt->stat;
    case WDT_REG_EOI:
        wdt->stat = 0;
        _dw_apb_wdt_update_irq(wdt);
        _dw_apb_wdt_reset_timer(wdt);
        return 0;
    case WDT_REG_COMP_PARAM_5:
        return 0;
    case WDT_REG_COMP_PARAM_4:
        return 0;
    case WDT_REG_COMP_PARAM_3:
        return 0;
    case WDT_REG_COMP_PARAM_2:
        return 0;
    case WDT_REG_COMP_PARAM_1:
        return _wdt_comp_param_1.value;
    case WDT_REG_COMP_VERSION:
        return WDT_VERSION_ID;
    case WDT_REG_COMP_TYPE:
        return WDT_TYPE_ID;
    default:
        printf(LOG_TAG"Warning: Read No-Exit Address %08lx\n", offset);
        break;
    }
    return 0;
}

static void _dw_apb_wdt_write(void *opaque, hwaddr offset, uint64_t value, unsigned size)
{
    DwApbWdtState *wdt = (DwApbWdtState *)opaque;

#ifdef LOG_DEBUG
    printf(LOG_TAG"%s(offset=%08lX, value=%08lX)\n", __FUNCTION__, offset, value);
#endif
    if (offset % 4 != 0)
        return;

    if (offset == WDT_REG_CR) {
        WDT_CR cr;
        cr.value = value;
        // Enable
        if (cr.wdt_en && wdt->cr.wdt_en == 0) {
            wdt->cr.wdt_en = 1;
            _dw_apb_wdt_reset_timer(wdt);
        }
        // Response Mode
        if (_wdt_comp_param_1.wdt_hc_rmod == 0) {
            wdt->cr.rmod = cr.rmod;
        }
        // Reset Pulse Length
        if (_wdt_comp_param_1.wdt_hc_rpl == 0) {
            wdt->cr.rpl = cr.rpl;
        }
        wdt->cr.no_name = cr.no_name;
        return;
    }

    if (offset == WDT_REG_TORR) {
        WDT_TORR torr;
        torr.value = value;
        if (_wdt_comp_param_1.wdt_hc_top == 0) {
            wdt->torr.top = torr.top;
            if (_wdt_comp_param_1.wdt_always_en == 0 && _wdt_comp_param_1.wdt_dual_top == 1) {
                wdt->torr.top_init = torr.top_init;
            }
        }
        wdt->timeout = (1ULL << (wdt->torr.top + 16)) * 1000 / WDT_BASE_FREQ;
        return;
    }

    if (offset == WDT_REG_CRR) {
        WDT_CRR crr;
        crr.value = value;
        if (crr.crr == WDT_CRR_CMD_RESTART) {
            wdt->stat = 0;
            _dw_apb_wdt_update_irq(wdt);
            _dw_apb_wdt_reset_timer(wdt);
        }

        return;
    }

    printf(LOG_TAG"Warning: Write 0x%08lX to ReadOnly Register 0x%08lX\n", value, offset);
}

static const MemoryRegionOps _dw_apb_wdt_ops = {
    .read = _dw_apb_wdt_read,
    .write = _dw_apb_wdt_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static const VMStateDescription _vmstate_dw_apb_wdt = {
    .name = TYPE_DW_APB_WDT,
    .version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_TIMER_PTR(timer, DwApbWdtState),
        VMSTATE_INT64(start, DwApbWdtState),
        VMSTATE_INT64(timeout, DwApbWdtState),
        VMSTATE_UINT32(cr.value, DwApbWdtState),
        VMSTATE_UINT32(torr.value, DwApbWdtState),
        VMSTATE_UINT32(stat, DwApbWdtState),
        VMSTATE_END_OF_LIST()
    }
};

static void _dw_apb_wdt_realize(DeviceState *dev, Error **errp)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
    DwApbWdtState *s = DW_APB_WDT(dev);

    s->timer = timer_new_ms(QEMU_CLOCK_VIRTUAL, _dw_apb_wdt_timeout, s);

    sysbus_init_irq(sbd, &s->irq);

    memory_region_init_io(&s->iomem, OBJECT(s), &_dw_apb_wdt_ops, s,
                          "dw-apb-wdt", 0x100);
    sysbus_init_mmio(sbd, &s->iomem);
}

static void _dw_apb_wdt_reset(DeviceState *d)
{
    DwApbWdtState *wdt  = DW_APB_WDT(d);
    wdt->cr.wdt_en      = _wdt_comp_param_1.wdt_always_en;
    wdt->cr.rmod        = _wdt_comp_param_1.wdt_dflt_rmod;
    wdt->cr.rpl         = _wdt_comp_param_1.wdt_dflt_rpl;
    wdt->cr.no_name     = 0;
    wdt->torr.top       = _wdt_comp_param_1.wdt_dflt_top;
    wdt->torr.top_init  = _wdt_comp_param_1.wdt_dflt_top_init;
    wdt->timeout        = (1 << (_wdt_comp_param_1.wdt_dflt_top + 16)) * 1000 / WDT_BASE_FREQ;
    wdt->stat           = 0;
}

static void _dw_apb_wdt_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = _dw_apb_wdt_realize;
    dc->vmsd    = &_vmstate_dw_apb_wdt;
    dc->reset   = _dw_apb_wdt_reset;
}

static const TypeInfo _dw_apb_wdt_info = {
    .name          = TYPE_DW_APB_WDT,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(DwApbWdtState),
    .class_init    = _dw_apb_wdt_class_init,
};

static void _dw_apb_wdt_register_types(void)
{
    type_register_static(&_dw_apb_wdt_info);
}

type_init(_dw_apb_wdt_register_types)
