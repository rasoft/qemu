/*
 *
 * Copyright (c) 2015 Linaro Limited
 * Copyright (c) 2024 NationalChip
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2 or later, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Emulate a virtual board which works by passing Linux all the information
 * it needs about what devices are present via the device tree.
 * There are some restrictions about what we can do here:
 *  + we can only present devices whose Linux drivers will work based
 *    purely on the device tree with no platform data at all
 *  + we want to present a very stripped-down minimalist platform,
 *    both because this reduces the security attack surface from the guest
 *    and also because it reduces our exposure to being broken when
 *    the kernel updates its device tree bindings and requires further
 *    information in a device binding that we aren't providing.
 * This is essentially the same approach kvmtool uses.
 */

#ifndef GX_VIRGO_H
#define GX_VIRGO_H

#include "exec/hwaddr.h"
#include "qemu/notify.h"
#include "hw/boards.h"
#include "hw/arm/boot.h"
#include "hw/block/flash.h"
#include "sysemu/kvm.h"
#include "hw/intc/arm_gicv3_common.h"
#include "qom/object.h"

#define NUM_GICV2M_SPIS       64
#define NUM_VIRTIO_TRANSPORTS  0 // 32
#define NUM_SMMU_IRQS          4

#define ARCH_GIC_MAINT_IRQ  9

#define ARCH_TIMER_VIRGO_IRQ   11
#define ARCH_TIMER_S_EL1_IRQ  13
#define ARCH_TIMER_NS_EL1_IRQ 14
#define ARCH_TIMER_NS_EL2_IRQ 10
#define ARCH_TIMER_NS_EL2_VIRGO_IRQ 12

#define VIRTUAL_PMU_IRQ 7

#define PPI(irq) ((irq) + 16)

/* See Linux kernel arch/arm64/include/asm/pvclock-abi.h */
#define PVTIME_SIZE_PER_CPU 64

enum {
    VIRGO_FLASH,
    VIRGO_MEM,
    VIRGO_CPUPERIPHS,
    VIRGO_GIC_DIST,
    VIRGO_GIC_CPU,
    VIRGO_GIC_V2M,
    VIRGO_GIC_HYP,
    VIRGO_GIC_VCPU,
    VIRGO_GIC_ITS,
    VIRGO_GIC_REDIST,
    VIRGO_SMMU,
    VIRGO_UART,
    VIRGO_SSI,
    VIRGO_MMIO,
    VIRGO_RTC,
    VIRGO_FW_CFG,
    VIRGO_PCIE,
    VIRGO_PCIE_MMIO,
    VIRGO_PCIE_PIO,
    VIRGO_PCIE_ECAM,
    VIRGO_PLATFORM_BUS,
    VIRGO_GPIO,
    VIRGO_SECURE_UART,
    VIRGO_SECURE_MEM,
    VIRGO_SECURE_GPIO,
    VIRGO_PCDIMM_ACPI,
    VIRGO_ACPI_GED,
    VIRGO_NVDIMM_ACPI,
    VIRGO_PVTIME,
    VIRGO_LOWMEMMAP_LAST,
};

/* indices of IO regions located after the RAM */
enum {
    VIRGO_HIGH_GIC_REDIST2 =  VIRGO_LOWMEMMAP_LAST,
    VIRGO_HIGH_PCIE_ECAM,
    VIRGO_HIGH_PCIE_MMIO,
};

typedef enum VirtIOMMUType {
    VIRGO_IOMMU_NONE,
    VIRGO_IOMMU_SMMUV3,
    VIRGO_IOMMU_VIRTIO,
} VirtIOMMUType;

typedef enum VirtMSIControllerType {
    VIRGO_MSI_CTRL_NONE,
    VIRGO_MSI_CTRL_GICV2M,
    VIRGO_MSI_CTRL_ITS,
} VirtMSIControllerType;

typedef enum VirtGICType {
    VIRGO_GIC_VERSION_MAX = 0,
    VIRGO_GIC_VERSION_HOST = 1,
    /* The concrete GIC values have to match the GIC version number */
    VIRGO_GIC_VERSION_2 = 2,
    VIRGO_GIC_VERSION_3 = 3,
    VIRGO_GIC_VERSION_4 = 4,
    VIRGO_GIC_VERSION_NOSEL,
} VirtGICType;

#define VIRGO_GIC_VERSION_2_MASK BIT(VIRGO_GIC_VERSION_2)
#define VIRGO_GIC_VERSION_3_MASK BIT(VIRGO_GIC_VERSION_3)
#define VIRGO_GIC_VERSION_4_MASK BIT(VIRGO_GIC_VERSION_4)

struct VirgoMachineClass {
    MachineClass parent;
    bool disallow_affinity_adjustment;
    bool no_its;
    bool no_tcg_its;
    bool no_pmu;
    bool claim_edge_triggered_timers;
    bool smbios_old_sys_ver;
    bool no_highmem_compact;
    bool no_highmem_ecam;
    bool no_ged;   /* Machines < 4.2 have no support for ACPI GED device */
    bool kvm_no_adjvtime;
    bool no_kvm_steal_time;
    bool acpi_expose_flash;
    bool no_secure_gpio;
    /* Machines < 6.2 have no support for describing cpu topology to guest */
    bool no_cpu_topology;
    bool no_tcg_lpa2;
};

struct VirgoMachineState {
    MachineState parent;
    Notifier machine_done;
    DeviceState *platform_bus_dev;
    FWCfgState *fw_cfg;
    PFlashCFI01 *flash[2];
    bool secure;
    bool highmem;
    bool highmem_compact;
    bool highmem_ecam;
    bool highmem_mmio;
    bool highmem_redists;
    bool its;
    bool tcg_its;
    bool virt;
    bool ras;
    bool mte;
    bool dtb_randomness;
    OnOffAuto acpi;
    VirtGICType gic_version;
    VirtIOMMUType iommu;
    bool default_bus_bypass_iommu;
    VirtMSIControllerType msi_controller;
    uint16_t virtio_iommu_bdf;
    struct arm_boot_info bootinfo;
    MemMapEntry *memmap;
    char *pciehb_nodename;
    const int *irqmap;
    int fdt_size;
    uint32_t clock_phandle;
    uint32_t gic_phandle;
    uint32_t msi_phandle;
    uint32_t iommu_phandle;
    int psci_conduit;
    hwaddr highest_gpa;
    DeviceState *gic;
    DeviceState *acpi_dev;
    Notifier powerdown_notifier;
    PCIBus *bus;
    char *oem_id;
    char *oem_table_id;
};

#define VIRGO_ECAM_ID(high) (high ? VIRGO_HIGH_PCIE_ECAM : VIRGO_PCIE_ECAM)

#define TYPE_VIRGO_MACHINE   MACHINE_TYPE_NAME("virgo")
OBJECT_DECLARE_TYPE(VirgoMachineState, VirgoMachineClass, VIRGO_MACHINE)

void virgo_acpi_setup(VirgoMachineState *vms);
bool virgo_is_acpi_enabled(VirgoMachineState *vms);

/* Return number of redistributors that fit in the specified region */
static uint32_t virgo_redist_capacity(VirgoMachineState *vms, int region)
{
    uint32_t redist_size;

    if (vms->gic_version == VIRGO_GIC_VERSION_3) {
        redist_size = GICV3_REDIST_SIZE;
    } else {
        redist_size = GICV4_REDIST_SIZE;
    }
    return vms->memmap[region].size / redist_size;
}

/* Return the number of used redistributor regions  */
static inline int virgo_gicv3_redist_region_count(VirgoMachineState *vms)
{
    uint32_t redist0_capacity = virgo_redist_capacity(vms, VIRGO_GIC_REDIST);

    assert(vms->gic_version != VIRGO_GIC_VERSION_2);

    return (MACHINE(vms)->smp.cpus > redist0_capacity &&
            vms->highmem_redists) ? 2 : 1;
}

#endif /* GX_VIRGO_H */
