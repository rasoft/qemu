/* "Unimplemented" device
 *
 * This is a dummy device which accepts and logs all accesses.
 * It's useful for stubbing out regions of an SoC or board
 * map which correspond to devices that have not yet been
 * implemented. This is often sufficient to placate initial
 * guest device driver probing such that the system will
 * come up.
 *
 * Copyright Linaro Limited, 2017
 * Written by Peter Maydell
 */

#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include "gx_unimp.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "qapi/error.h"

static uint64_t gx_unimp_read(void *opaque, hwaddr offset, unsigned size)
{
    GxUnimplementedDeviceState *s = GX_UNIMPLEMENTED_DEVICE(opaque);

    printf("%s: gx_unimplemented device read  "
                  "(size %d, offset 0x%0*" HWADDR_PRIx ")\n",
                  s->name, size, s->offset_fmt_width, offset);
    return 0;
}

static void gx_unimp_write(void *opaque, hwaddr offset,
                        uint64_t value, unsigned size)
{
    GxUnimplementedDeviceState *s = GX_UNIMPLEMENTED_DEVICE(opaque);

    printf("%s: gx_unimplemented device write "
                  "(size %d, offset 0x%0*" HWADDR_PRIx
                  ", value 0x%0*" PRIx64 ")\n",
                  s->name, size, s->offset_fmt_width, offset, size << 1, value);
}

static const MemoryRegionOps gx_unimp_ops = {
    .read = gx_unimp_read,
    .write = gx_unimp_write,
    .impl.min_access_size = 1,
    .impl.max_access_size = 8,
    .valid.min_access_size = 1,
    .valid.max_access_size = 8,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void gx_unimp_realize(DeviceState *dev, Error **errp)
{
    GxUnimplementedDeviceState *s = GX_UNIMPLEMENTED_DEVICE(dev);

    if (s->size == 0) {
        error_setg(errp, "property 'size' not specified or zero");
        return;
    }

    if (s->name == NULL) {
        error_setg(errp, "property 'name' not specified");
        return;
    }

    s->offset_fmt_width = DIV_ROUND_UP(64 - clz64(s->size - 1), 4);

    memory_region_init_io(&s->iomem, OBJECT(s), &gx_unimp_ops, s,
                          s->name, s->size);
    sysbus_init_mmio(SYS_BUS_DEVICE(s), &s->iomem);
}

static Property gx_unimp_properties[] = {
    DEFINE_PROP_UINT64("size", GxUnimplementedDeviceState, size, 0),
    DEFINE_PROP_STRING("name", GxUnimplementedDeviceState, name),
    DEFINE_PROP_END_OF_LIST(),
};

static void gx_unimp_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = gx_unimp_realize;
    device_class_set_props(dc, gx_unimp_properties);
}

static const TypeInfo gx_unimp_info = {
    .name = TYPE_GX_UNIMPLEMENTED_DEVICE,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(GxUnimplementedDeviceState),
    .class_init = gx_unimp_class_init,
};

static void gx_unimp_register_types(void)
{
    type_register_static(&gx_unimp_info);
}

type_init(gx_unimp_register_types)
