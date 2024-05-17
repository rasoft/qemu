/*
 * NationalChip Vega emulation
 *
 * Copyright (c) 2024 NationalChip
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "qemu/osdep.h"
#include "qemu/units.h"
#include "exec/address-spaces.h"
#include "qapi/error.h"
#include "qemu/error-report.h"

#include "hw/boards.h"
#include "hw/qdev-properties.h"
#include "hw/misc/unimp.h"
#include "hw/loader.h"
#include "hw/intc/arm_gic.h"
#include "hw/arm/boot.h"
#include "hw/usb/hcd-ehci.h"

#include "target/arm/cpu.h"

#include "sysemu/block-backend.h"
#include "sysemu/sysemu.h"

#include "dw_apb_uart.h"
#include "dwc_emac.h"
#include "gx_vega_syscfg.h"

#define VEGA_NUM_CPUS       2
#define VEGA_NUM_GIC_SPI    128

enum {
    VEGA_DEV_DDR,
    VEGA_DEV_ROM,
    VEGA_DEV_SRAM,
    VEGA_DEV_PATE_TABLE,
    VEGA_DEV_BOOT_MONITOR,
    VEGA_DEV_DW_SPI,
    VEGA_DEV_GX_I2C_0,
    VEGA_DEV_GX_I2C_1,
    VEGA_DEV_GX_I2C_2,
    VEGA_DEV_GX_I2C_3,
    VEGA_DEV_DW_I2C_0,
    VEGA_DEV_DW_I2C_1,
    VEGA_DEV_DW_I2C_2,
    VEGA_DEV_DW_I2C_3,
    VEGA_DEV_GX_IRR,
    VEGA_DEV_GX_RTC,
    VEGA_DEV_GX_CNT,
    VEGA_DEV_GX_WDT,
    VEGA_DEV_SEC_APB0,
    VEGA_DEV_GPIO_0,
    VEGA_DEV_GPIO_1,
    VEGA_DEV_GPIO_2,
    VEGA_DEV_GPIO_3,
    VEGA_DEV_SEC_APB1,
    VEGA_DEV_DW_UART0,
    VEGA_DEV_DW_UART1,
    VEGA_DEV_DW_UART2,
    VEGA_DEV_GX_OSC,
    VEGA_DEV_SEC_APB2,
    VEGA_DEV_GX_SMARTCARD,
    VEGA_DEV_SEC_APB3,
    VEGA_DEV_SEC_AHB0,
    VEGA_DEV_NAND_CTL,
    VEGA_DEV_GMAC,
    VEGA_DEV_USB,
    VEGA_DEV_USB_PHY,
    VEGA_DEV_MB_TEE,
    VEGA_DEV_MB_REE,
    VEGA_DEV_SDC,
    VEGA_DEV_GC300,
    VEGA_DEV_SDIO,
    VEGA_DEV_SENSOR_27M_0,
    VEGA_DEV_SENSOR_27M_1,
    VEGA_DEV_SENSOR_170M_0,
    VEGA_DEV_SENSOR_170M_1,
    VEGA_DEV_SEC_ABN_PROC,
    VEGA_DEV_SEC_SENSOR,
    VEGA_DEV_FIREWALL,
    VEGA_DEV_SEC_AHB1,
    VEGA_DEV_ACPU_CRYPTO,
    VEGA_DEV_ACPU_KLM,
    VEGA_DEV_SECURE_M2M,
    VEGA_DEV_DVB,
    VEGA_DEV_SYS_CONFIG,
    VEGA_DEV_RNG_EIP76,
    VEGA_DEV_ACPU_HASH,
    VEGA_DEV_PMU,
    VEGA_DEV_RCC,
    VEGA_DEV_OTPC,
    VEGA_DEV_A7_DAPLITE,
    VEGA_DEV_AKL,
    VEGA_DEV_CLOCK_CONFIG,
    VEGA_DEV_PIN_CONFIG,
    VEGA_DEV_RNG,
    VEGA_DEV_SEC_AHB2,
    VEGA_DEV_AHB_FIREWALL,
    VEGA_DEV_GSE,
    VEGA_DEV_DEMUX,
    VEGA_DEV_VIDEO_DECODER,
    VEGA_DEV_PP,
    VEGA_DEV_AUDIO_DECODER,
    VEGA_DEV_JPEG_DECODER,
    VEGA_DEV_GA,
    VEGA_DEV_VPU,
    VEGA_DEV_VDAC,
    VEGA_DEV_RSA,
    VEGA_DEV_PID_FILTER,
    VEGA_DEV_TSIO,
    VEGA_DEV_AUDIO_PLAY,
    VEGA_DEV_HDMI,
    VEGA_DEV_HDMI_PHY,
    VEGA_DEV_SEC_AHB3,
    VEGA_DEV_GIC_DIST,
    VEGA_DEV_GIC_CPU,
};

/* Memory map */
static const hwaddr _vega_memmap[] = {
    [VEGA_DEV_DDR]          = 0x00000000,
    [VEGA_DEV_ROM]          = 0x80000000,
    [VEGA_DEV_SRAM]         = 0x80400000,
    [VEGA_DEV_PATE_TABLE]   = 0x80800000,
    [VEGA_DEV_BOOT_MONITOR] = 0x80C00000,

    [VEGA_DEV_DW_SPI]       = 0x81000000,

    [VEGA_DEV_GX_I2C_0]     = 0x82000000,
    [VEGA_DEV_GX_I2C_1]     = 0x82001000,
    [VEGA_DEV_GX_I2C_2]     = 0x82002000,
    [VEGA_DEV_GX_I2C_3]     = 0x82003000,
    [VEGA_DEV_DW_I2C_0]     = 0x82004000,
    [VEGA_DEV_DW_I2C_1]     = 0x82005000,
    [VEGA_DEV_DW_I2C_2]     = 0x82006000,
    [VEGA_DEV_DW_I2C_3]     = 0x82007000,
    [VEGA_DEV_GX_IRR]       = 0x82008000,
    [VEGA_DEV_GX_RTC]       = 0x8200A000,
    [VEGA_DEV_GX_CNT]       = 0x8200C000,
    [VEGA_DEV_GX_WDT]       = 0x8200E000,
    [VEGA_DEV_SEC_APB0]     = 0x8200F000,
    [VEGA_DEV_GPIO_0]       = 0x82404000,
    [VEGA_DEV_GPIO_1]       = 0x82405000,
    [VEGA_DEV_GPIO_2]       = 0x82406000,
    [VEGA_DEV_GPIO_3]       = 0x82407000,
    [VEGA_DEV_SEC_APB1]     = 0x8240F000,
    [VEGA_DEV_DW_UART0]     = 0x82800000,
    [VEGA_DEV_DW_UART1]     = 0x82801000,
    [VEGA_DEV_DW_UART2]     = 0x82802000,
    [VEGA_DEV_GX_OSC]       = 0x82808000,
    [VEGA_DEV_SEC_APB2]     = 0x8280F000,
    [VEGA_DEV_GX_SMARTCARD] = 0x82C00000,
    [VEGA_DEV_SEC_APB3]     = 0x82C0F000,

    [VEGA_DEV_SEC_AHB0]     = 0x83C00000,

    [VEGA_DEV_NAND_CTL]     = 0x88000000,
    [VEGA_DEV_GMAC]         = 0x88100000,
    [VEGA_DEV_USB]          = 0x88200000,
    [VEGA_DEV_USB_PHY]      = 0x88209000,
    [VEGA_DEV_MB_TEE]       = 0x88300000,
    [VEGA_DEV_MB_REE]       = 0x88400000,
    [VEGA_DEV_SDC]          = 0x88500000,
    [VEGA_DEV_GC300]        = 0x88700000,
    [VEGA_DEV_SDIO]         = 0x88800000,
    [VEGA_DEV_SENSOR_27M_0] = 0x88C00000,
    [VEGA_DEV_SENSOR_27M_1] = 0x88C10000,
    [VEGA_DEV_SENSOR_170M_0]= 0x88C20000,
    [VEGA_DEV_SENSOR_170M_1]= 0x88C30000,
    [VEGA_DEV_SEC_ABN_PROC] = 0x88C60000,
    [VEGA_DEV_SEC_SENSOR]   = 0x88CF0000,
    [VEGA_DEV_FIREWALL]     = 0x88E00000,
    [VEGA_DEV_SEC_AHB1]     = 0x88F00000,

    [VEGA_DEV_ACPU_CRYPTO]  = 0x89000000,
    [VEGA_DEV_ACPU_KLM]     = 0x89100000,
    [VEGA_DEV_SECURE_M2M]   = 0x89200000,
    [VEGA_DEV_DVB]          = 0x89300000,
    [VEGA_DEV_SYS_CONFIG]   = 0x89400000,
    [VEGA_DEV_RNG_EIP76]    = 0x89500000,
    [VEGA_DEV_ACPU_HASH]    = 0x89600000,
    [VEGA_DEV_PMU]          = 0x89700000,
    [VEGA_DEV_RCC]          = 0x89800000,
    [VEGA_DEV_OTPC]         = 0x89900000,
    [VEGA_DEV_A7_DAPLITE]   = 0x89A00000,
    [VEGA_DEV_AKL]          = 0x89B00000,
    [VEGA_DEV_CLOCK_CONFIG] = 0x89C00000,
    [VEGA_DEV_PIN_CONFIG]   = 0x89D00000,
    [VEGA_DEV_RNG]          = 0x89E00000,
    [VEGA_DEV_SEC_AHB2]     = 0x89F00000,

    [VEGA_DEV_AHB_FIREWALL] = 0x8A000000,
    [VEGA_DEV_GSE]          = 0x8A100000,
    [VEGA_DEV_DEMUX]        = 0x8A200000,
    [VEGA_DEV_VIDEO_DECODER]= 0x8A300000,
    [VEGA_DEV_PP]           = 0x8A400000,
    [VEGA_DEV_AUDIO_DECODER]= 0x8A500000,
    [VEGA_DEV_JPEG_DECODER] = 0x8A600000,
    [VEGA_DEV_GA]           = 0x8A700000,
    [VEGA_DEV_VPU]          = 0x8A800000,
    [VEGA_DEV_VDAC]         = 0x8A80F000,
    [VEGA_DEV_RSA]          = 0x8A900000,
    [VEGA_DEV_PID_FILTER]   = 0x8AA00000,
    [VEGA_DEV_TSIO]         = 0x8AC00000,
    [VEGA_DEV_AUDIO_PLAY]   = 0x8AD00000,
    [VEGA_DEV_HDMI]         = 0x8AE00000,
    [VEGA_DEV_HDMI_PHY]     = 0x8AE08000,
    [VEGA_DEV_SEC_AHB3]     = 0x8AF00000,

    [VEGA_DEV_GIC_DIST]     = 0xA2001000,
    [VEGA_DEV_GIC_CPU]      = 0xA2002000,
};

/* List of unimplemented devices */
static struct {
    const char *name;
    hwaddr base;
    hwaddr size;
} _vega_unimplemented[] = {
    { "fallback",       0x00000000,     4 * GiB - 4 * KiB},
};

/* Per Processor Interrupts */
enum {
    VEGA_GIC_PPI_MAINT          =  9,
    VEGA_GIC_PPI_HYPTIMER       = 10,
    VEGA_GIC_PPI_VIRTTIMER      = 11,
    VEGA_GIC_PPI_SECTIMER       = 13,
    VEGA_GIC_PPI_PHYSTIMER      = 14
};

/* Shared Processor Interrupts */
enum {
    VEGA_GIC_SPI_SDC            =  0,
    VEGA_GIC_SPI_DEMUX_0        =  1,
    VEGA_GIC_SPI_DEMUX_1        =  2,
    VEGA_GIC_SPI_PIDFILTER      =  3,
    VEGA_GIC_SPI_GSE            =  4,
    VEGA_GIC_SPI_GP             =  5,
    VEGA_GIC_SPI_AUDIO_DSP      =  6,
    VEGA_GIC_SPI_AUDIO_PLAY_IIS =  7,
    VEGA_GIC_SPI_AUDIO_PLAY_SPD =  8,
    VEGA_GIC_SPI_AUDIO_PLAY_AC3 =  9,
    VEGA_GIC_SPI_JPEG           = 10,
    VEGA_GIC_SPI_PP             = 11,
    VEGA_GIC_SPI_GA             = 12,
    VEGA_GIC_SPI_GA_V2          = 13,
    VEGA_GIC_SPI_VPU            = 14,
    VEGA_GIC_SPI_VIDEO_DECODER  = 15,
    VEGA_GIC_SPI_NPU            = 16,

    VEGA_GIC_SPI_MAC_WORK       = 18,
    VEGA_GIC_SPI_MAC_PWR        = 19,
    VEGA_GIC_SPI_DVB            = 20,
    VEGA_GIC_SPI_NFC            = 21,
    VEGA_GIC_SPI_SDIO           = 22,
    VEGA_GIC_SPI_USB_OHCI1      = 23,
    VEGA_GIC_SPI_USB_OHCI0      = 24,
    VEGA_GIC_SPI_USB_EHCI       = 25,
    VEGA_GIC_SPI_ACPU_CRYPTO    = 26,
    VEGA_GIC_SPI_SEC_M2M        = 27,
    VEGA_GIC_SPI_ACPU_KLM       = 28,
    VEGA_GIC_SPI_ACPU_HASH      = 29,
    VEGA_GIC_SPI_ACPU_SOFT      = 30,
    VEGA_GIC_SPI_IFCP_REE       = 31,
    VEGA_GIC_SPI_IFCP_TEE       = 32,

    VEGA_GIC_SPI_SMART_CARD     = 34,

    VEGA_GIC_SPI_DW_UART2       = 36,
    VEGA_GIC_SPI_DW_UART1       = 37,
    VEGA_GIC_SPI_DW_UART0       = 38,


    VEGA_GIC_SPI_GPIO3          = 41,
    VEGA_GIC_SPI_GPIO2          = 42,
    VEGA_GIC_SPI_GPIO1          = 43,
    VEGA_GIC_SPI_GPIO0          = 44,
    VEGA_GIC_SPI_DW_SPI         = 45,
    VEGA_GIC_SPI_DW_I2C4        = 46,
    VEGA_GIC_SPI_DW_I2C3        = 47,
    VEGA_GIC_SPI_DW_I2C2        = 48,
    VEGA_GIC_SPI_DW_I2C1        = 49,
    VEGA_GIC_SPI_GX_I2C4        = 50,
    VEGA_GIC_SPI_GX_I2C3        = 51,
    VEGA_GIC_SPI_GX_I2C2        = 52,
    VEGA_GIC_SPI_GX_I2C1        = 53,
    VEGA_GIC_SPI_RTC            = 54,
    VEGA_GIC_SPI_WDT            = 55,
    VEGA_GIC_SPI_IR             = 56,
    VEGA_GIC_SPI_COUNT4         = 57,
    VEGA_GIC_SPI_COUNT3         = 58,
    VEGA_GIC_SPI_COUNT2         = 59,
    VEGA_GIC_SPI_COUNT1         = 60,

    VEGA_GIC_SPI_PMU            = 62,
    VEGA_GIC_SPI_HDMI_CTRL      = 63,
    VEGA_GIC_SPI_HDMI_CEC       = 64,
    VEGA_GIC_SPI_HDMI_PHY       = 65,
    VEGA_GIC_SPI_RSA            = 66,
    VEGA_GIC_SPI_HDMI_HDCP      = 67,
    VEGA_GIC_SPI_AKL            = 68,
    VEGA_GIC_SPI_DDR_FIREWALL   = 69,
    VEGA_GIC_SPI_AHB_FIREWALL   = 70,
    VEGA_GIC_SPI_TSIO_REE       = 71,
    VEGA_GIC_SPI_TSIO_TEE       = 72,
};

static void vega_init(MachineState *machine)
{
    // MachineClass *mc = MACHINE_GET_CLASS(machine);
    MemoryRegion *sysmem = get_system_memory();
    unsigned int smp_cpus = machine->smp.cpus;

    if (machine->firmware) {
        error_report("BIOS not supported for this machine");
        exit(1);
    }

    if (strcmp(machine->cpu_type, ARM_CPU_TYPE_NAME("cortex-a7")) != 0) {
        error_report("mach-vega: CPU type %s not supported", machine->cpu_type);
        exit(1);
    }

    if (machine->smp.cpus > 2) {
        error_report("mach-vego: CPU num %d > 2", machine->smp.cpus);
        exit(1);
    }

    if (machine->ram_size > 2 * GiB) {
        error_report("mach-vega: Memory size (%lu) > 2GB", machine->ram_size);
        exit(1);
    }

    /* DDR Memory */
    memory_region_add_subregion(sysmem, _vega_memmap[VEGA_DEV_DDR], machine->ram);

    /* Generic Interrupt Controller */
    Object *gicobj = object_new(TYPE_ARM_GIC);
    GICState *gic = ARM_GIC(gicobj);
    qdev_prop_set_uint32(DEVICE(gic), "num-irq", VEGA_NUM_GIC_SPI + GIC_INTERNAL);
    qdev_prop_set_uint32(DEVICE(gic), "revision", 2);
    qdev_prop_set_uint32(DEVICE(gic), "num-cpu", VEGA_NUM_CPUS);
    qdev_prop_set_bit(DEVICE(gic), "has-security-extensions", true);
    qdev_prop_set_bit(DEVICE(gic), "has-virtualization-extensions", false);
    sysbus_realize(SYS_BUS_DEVICE(gic), &error_fatal);

    sysbus_mmio_map(SYS_BUS_DEVICE(gic), 0, _vega_memmap[VEGA_DEV_GIC_DIST]);
    sysbus_mmio_map(SYS_BUS_DEVICE(gic), 1, _vega_memmap[VEGA_DEV_GIC_CPU]);
    // sysbus_mmio_map(SYS_BUS_DEVICE(gic), 2, _vega_memmap[AW_H3_DEV_GIC_HYP]);
    // sysbus_mmio_map(SYS_BUS_DEVICE(gic), 3, _vega_memmap[AW_H3_DEV_GIC_VCPU]);

    /* ARM CPUs */
    for (int n = 0; n < smp_cpus; n++) {
        Object *cpuobj = object_new(machine->cpu_type);
        CPUState *cs = CPU(cpuobj);
        cs->cpu_index = n;
        object_property_set_bool(cpuobj, "has_el3", true, NULL);
        object_property_set_bool(cpuobj, "has_el2", false, NULL);

        qdev_realize(DEVICE(cpuobj), NULL, &error_fatal);

        /* Connect CPU timer outputs to GIC PPI inputs */
        const int timer_irq[] = {
            [GTIMER_PHYS] = VEGA_GIC_PPI_PHYSTIMER,
            [GTIMER_VIRT] = VEGA_GIC_PPI_VIRTTIMER,
            [GTIMER_HYP]  = VEGA_GIC_PPI_HYPTIMER,
            [GTIMER_SEC]  = VEGA_GIC_PPI_SECTIMER,
        };
        int ppibase = VEGA_NUM_GIC_SPI + n * GIC_INTERNAL + GIC_NR_SGIS;
        for (int irq = 0; irq < ARRAY_SIZE(timer_irq); irq++) {
            qdev_connect_gpio_out(DEVICE(cpuobj), irq, qdev_get_gpio_in(DEVICE(gic), ppibase + timer_irq[irq]));
        }

        /* Connect GIC outputs to CPU interrupt inputs */
        sysbus_connect_irq(SYS_BUS_DEVICE(gic), n + (0 * VEGA_NUM_CPUS), qdev_get_gpio_in(DEVICE(cpuobj), ARM_CPU_IRQ));
        sysbus_connect_irq(SYS_BUS_DEVICE(gic), n + (1 * VEGA_NUM_CPUS), qdev_get_gpio_in(DEVICE(cpuobj), ARM_CPU_FIQ));
        // sysbus_connect_irq(SYS_BUS_DEVICE(gic), n + (2 * VEGA_NUM_CPUS), qdev_get_gpio_in(DEVICE(cpuobj), ARM_CPU_VIRQ));
        // sysbus_connect_irq(SYS_BUS_DEVICE(gic), n + (3 * VEGA_NUM_CPUS), qdev_get_gpio_in(DEVICE(cpuobj), ARM_CPU_VFIQ))

        /* GIC maintenance signal */
        // sysbus_connect_irq(SYS_BUS_DEVICE(gic), n + (4 * VEGA_NUM_CPUS), qdev_get_gpio_in(DEVICE(gic), ppibase + VEGA_GIC_PPI_MAINT));
    }

    /* UART */
    {
        DeviceState *dev = qdev_new(TYPE_DW_APB_UART);
        SysBusDevice *s = SYS_BUS_DEVICE(dev);
        qdev_prop_set_chr(dev, "chardev", serial_hd(0));
        sysbus_realize_and_unref(s, &error_fatal);
        memory_region_add_subregion(sysmem, _vega_memmap[VEGA_DEV_DW_UART0], sysbus_mmio_get_region(s, 0));
        sysbus_connect_irq(s, 0, qdev_get_gpio_in(DEVICE(gic), VEGA_GIC_SPI_DW_UART0));
    }

    /* Ethernet */
    {
#if 1
        DeviceState *dev = qemu_create_nic_device(TYPE_DWC_EMAC, true, NULL);
        if (dev) {
            SysBusDevice *s = SYS_BUS_DEVICE(dev);
            sysbus_realize_and_unref(s, &error_fatal);
            sysbus_mmio_map(s, 0, _vega_memmap[VEGA_DEV_GMAC]);
            sysbus_connect_irq(s, 0, qdev_get_gpio_in(DEVICE(gic), VEGA_GIC_SPI_MAC_WORK));
        }
#else
        DeviceState *dev = qdev_new(TYPE_DWC_EMAC);
        SysBusDevice *s = SYS_BUS_DEVICE(dev);
        if (nd_table[0].used) {
            qemu_check_nic_model(&nd_table[0], TYPE_DWC_EMAC);
            qdev_set_nic_properties(dev, &nd_table[0]);
        }
        // object_property_set_link(OBJECT(dev), "dma-memory", OBJECT(get_system_memory()), &error_fatal);
        sysbus_realize_and_unref(s, &error_fatal);
        sysbus_mmio_map(s, 0, _vega_memmap[VEGA_DEV_GMAC]);
        sysbus_connect_irq(s, 0, qdev_get_gpio_in(DEVICE(gic), VEGA_GIC_SPI_MAC_WORK));
#endif
    }

    /* USB */
    {
        DeviceState *dev = qdev_new(TYPE_AW_H3_EHCI);
        SysBusDevice *s = SYS_BUS_DEVICE(dev);
        object_property_set_bool(OBJECT(dev), "companion-enable", true, &error_abort);
        SYS_BUS_EHCI(s)->ehci.portnr = 2; SYS_BUS_EHCI(s)->ehci.caps[4] = 2;
        sysbus_realize_and_unref(s, &error_abort);
        sysbus_mmio_map(s, 0, _vega_memmap[VEGA_DEV_USB] + 0x4000);
        sysbus_connect_irq(s, 0, qdev_get_gpio_in(DEVICE(gic), VEGA_GIC_SPI_USB_EHCI));
    }
    {
        DeviceState *dev = qdev_new("sysbus-ohci");
        SysBusDevice *s = SYS_BUS_DEVICE(dev);
        object_property_set_str(OBJECT(dev), "masterbus", "usb-bus.0", &error_abort);
        object_property_set_uint(OBJECT(dev), "num-ports", 1, &error_abort);
        sysbus_realize_and_unref(s, &error_abort);
        sysbus_mmio_map(s, 0, _vega_memmap[VEGA_DEV_USB] + 0x0000);
        sysbus_connect_irq(s, 0, qdev_get_gpio_in(DEVICE(gic), VEGA_GIC_SPI_USB_OHCI0));
    }
    {
        DeviceState *dev = qdev_new("sysbus-ohci");
        SysBusDevice *s = SYS_BUS_DEVICE(dev);
        object_property_set_str(OBJECT(dev), "masterbus", "usb-bus.0", &error_abort);
        object_property_set_uint(OBJECT(dev), "firstport", 1, &error_abort);
        object_property_set_uint(OBJECT(dev), "num-ports", 1, &error_abort);
        sysbus_realize_and_unref(s, &error_abort);
        sysbus_mmio_map(s, 0, _vega_memmap[VEGA_DEV_USB] + 0x1000);
        sysbus_connect_irq(s, 0, qdev_get_gpio_in(DEVICE(gic), VEGA_GIC_SPI_USB_OHCI1));
    }

    /* SysCfg */
    gx_vega_syscfg_create(_vega_memmap[VEGA_DEV_SYS_CONFIG]);


    /* Unimplemented devices, use -d unimp to see it */
    for (int i = 0; i < ARRAY_SIZE(_vega_unimplemented); i++) {
        create_unimplemented_device(_vega_unimplemented[i].name,
                                    _vega_unimplemented[i].base,
                                    _vega_unimplemented[i].size);
    }

    /* Boot */
    struct arm_boot_info vega_binfo;
    vega_binfo.endianness = ARM_ENDIANNESS_LE;
    vega_binfo.ram_size = machine->ram_size;
    vega_binfo.board_id = 0x123;
    vega_binfo.loader_start = _vega_memmap[VEGA_DEV_DDR];
    // vega_binfo.skip_dtb_autoload = true;
    vega_binfo.psci_conduit = QEMU_PSCI_CONDUIT_DISABLED;

    arm_load_kernel(ARM_CPU(first_cpu), machine, &vega_binfo);
}

static void vega_machine_init(MachineClass *mc)
{
    mc->desc = "NationalChip Vega";
    mc->init = vega_init;
    mc->block_default_type = IF_SD;
    mc->units_per_default_bus = 1;
    mc->min_cpus = 1;
    mc->max_cpus = VEGA_NUM_CPUS;
    mc->default_cpus = 1;
    mc->default_cpu_type = ARM_CPU_TYPE_NAME("cortex-a7");
    mc->default_ram_size = 1 * GiB;
    mc->default_ram_id = "vega.ram";
}

DEFINE_MACHINE("vega", vega_machine_init)
