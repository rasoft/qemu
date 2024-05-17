#ifndef _DWC_GMAC_H_
#define _DWC_GMAC_H_

#include "hw/sysbus.h"
#include "hw/irq.h"
#include "hw/ptimer.h"
#include "net/net.h"

typedef struct RTL8201CPState {
    uint16_t bmcr;
    uint16_t bmsr;
    uint16_t anar;
    uint16_t anlpar;
} RTL8201CPState;

typedef struct RxTxStats {
    uint64_t rx_bytes;
    uint64_t tx_bytes;

    uint64_t rx;
    uint64_t rx_bcast;
    uint64_t rx_mcast;
} RxTxStats;

#define R_MAX 0x500

struct DwcEMACState {
    SysBusDevice parent_obj;

    MemoryRegion iomem;
    qemu_irq sbd_irq;
    qemu_irq pmt_irq;
    qemu_irq mci_irq;
    NICState *nic;
    NICConf conf;

    uint8_t        phy_addr;
    unsigned int   desc_skip;
    RTL8201CPState mii;
    struct RxTxStats stats;
    uint32_t regs[R_MAX];
};

#define TYPE_DWC_EMAC "dwc_emac"

OBJECT_DECLARE_SIMPLE_TYPE(DwcEMACState, DWC_EMAC)

#endif
