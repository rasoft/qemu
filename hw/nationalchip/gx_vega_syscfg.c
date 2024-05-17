#include "qemu/osdep.h"
#include "qemu/main-loop.h"
#include "qemu/log.h"
#include "hw/sysbus.h"
#include "hw/irq.h"
#include "hw/qdev-properties.h"
#include "hw/qdev-properties-system.h"
#include "chardev/char-fe.h"
#include "sysemu/sysemu.h"
#include "migration/vmstate.h"

#include "gx_vega_syscfg.h"

static uint64_t _gx_vega_syscfg_read(void *opaque, hwaddr offset, unsigned size)
{
    // VegaSysCfgState *s = (VegaSysCfgState *)opaque;
    uint64_t ret = 0;

    if (size != 4) {
        qemu_log_mask(LOG_GUEST_ERROR, "%s: 0x%x must word align read\n", __FUNCTION__, (int)offset);
        // printf("%s: 0x%x must word align read\n", __FUNCTION__, (int)offset);
    }

    switch (offset) {
    case 0x184: // Chip ID
        return (0x1 << 30) | (0x6633 << 14) | (0x0);

    default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: Bad offset %x\n", __FUNCTION__, (int)offset);
        printf("%s: Bad offset %x\n", __FUNCTION__, (int)offset);
    }

    return ret;
}

static void _gx_vega_syscfg_write(void *opaque, hwaddr offset, uint64_t value,
                                  unsigned size)
{
    // VegaSysCfgState *s = (VegaSysCfgState *)opaque;

    if (size != 4) {
        qemu_log_mask(LOG_GUEST_ERROR, "%s: 0x%x must word align read\n", __FUNCTION__, (int)offset);
        // printf("%s: 0x%x must word align read\n", __FUNCTION__, (int)offset);
    }

    switch (offset) {
    default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: Bad offset %x\n", __FUNCTION__, (int)offset);
        printf("%s: Bad offset %x\n", __FUNCTION__, (int)offset);
    }
}

static const MemoryRegionOps _gx_vega_syscfg_ops = {
    .read = _gx_vega_syscfg_read,
    .write = _gx_vega_syscfg_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .impl = {
        .min_access_size = 4,
        .max_access_size = 4,
    }
};

static void _gx_vega_syscfg_init(Object *obj)
{
    VegaSysCfgState *s = GX_VEGA_SYSCFG(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    memory_region_init_io(&s->iomem, OBJECT(s), &_gx_vega_syscfg_ops, s,
                          TYPE_GX_VEGA_SYSCFG, 0x1000);
    sysbus_init_mmio(sbd, &s->iomem);
}

static void _gx_vega_syscfg_realize(DeviceState *dev, Error **errp)
{
    // VegaSysCfgState *s = GX_VEGA_SYSCFG(dev);
}

static void _gx_vega_syscfg_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
    set_bit(DEVICE_CATEGORY_MISC, dc->categories);
    dc->realize = _gx_vega_syscfg_realize;
    dc->desc = "NationalChip Vega's System Configuration";
    dc->user_creatable = true;
}

static const TypeInfo _gx_vega_syscfg_info = {
    .name          = TYPE_GX_VEGA_SYSCFG,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(VegaSysCfgState),
    .instance_init = _gx_vega_syscfg_init,
    .class_init    = _gx_vega_syscfg_class_init,
};


static void _gx_vega_syscfg_register_types(void)
{
    type_register_static(&_gx_vega_syscfg_info);
}

type_init(_gx_vega_syscfg_register_types)
