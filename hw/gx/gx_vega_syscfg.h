#ifndef _GX_VEGA_SYSCFG_H_
#define _GX_VEGA_SYSCFG_H_

#include "hw/sysbus.h"

typedef struct VegaSysCfgState {
    SysBusDevice parent_obj;

    MemoryRegion    iomem;
} VegaSysCfgState;

#define TYPE_GX_VEGA_SYSCFG  "gx_vega_syscfg"
#define GX_VEGA_SYSCFG(obj)  OBJECT_CHECK(VegaSysCfgState, (obj), TYPE_GX_VEGA_SYSCFG)

static inline DeviceState *gx_vega_syscfg_create(hwaddr addr)
{
    DeviceState *dev = qdev_new(TYPE_GX_VEGA_SYSCFG);
    SysBusDevice *s = SYS_BUS_DEVICE(dev);
    sysbus_realize(s, NULL);
    sysbus_mmio_map(s, 0, addr);
    return dev;
}

#endif
