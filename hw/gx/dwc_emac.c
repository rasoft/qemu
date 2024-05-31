/*
 * QEMU model of DWC_EMAC Ethernet.
 * QEMU model for DesignWare
 *
 * derived from the Xilinx AXI-Ethernet by Edgar E. Iglesias.
 *
 * Copyright (c) 2011 Calxeda, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "qom/object.h"

#include "migration/vmstate.h"
#include "net/net.h"

#include "hw/irq.h"
#include "hw/qdev-properties.h"
#include "hw/sysbus.h"
#include "hw/net/mii.h"

#include "dwc_emac.h"

static void mii_set_link(RTL8201CPState *mii, bool link_ok)
{
    if (link_ok) {
        mii->bmsr |= MII_BMSR_LINK_ST | MII_BMSR_AN_COMP;
        mii->anlpar |= MII_ANAR_TXFD | MII_ANAR_10FD | MII_ANAR_10 |
                       MII_ANAR_CSMACD;
    } else {
        mii->bmsr &= ~(MII_BMSR_LINK_ST | MII_BMSR_AN_COMP);
        mii->anlpar = MII_ANAR_TX;
    }
}

static void mii_reset(RTL8201CPState *mii, bool link_ok)
{
    mii->bmcr = MII_BMCR_FD | MII_BMCR_AUTOEN | MII_BMCR_SPEED;
    mii->bmsr = MII_BMSR_100TX_FD | MII_BMSR_100TX_HD | MII_BMSR_10T_FD |
                MII_BMSR_10T_HD | MII_BMSR_MFPS | MII_BMSR_AUTONEG;
    mii->anar = MII_ANAR_TXFD | MII_ANAR_TX | MII_ANAR_10FD | MII_ANAR_10 |
                MII_ANAR_CSMACD;
    mii->anlpar = MII_ANAR_TX;

    mii_set_link(mii, link_ok);
}

static uint16_t rtl8201cp_mdio_read(DwcEMACState *s, uint8_t addr, uint8_t reg)
{
    RTL8201CPState *mii = &s->mii;
    uint16_t ret = 0xffff;

    // printf("dwc_emac: read mii addr=0x%x, reg=0x%x\n", addr, reg);
    if (addr == s->phy_addr) {
        switch (reg) {
        case MII_BMCR:
            return mii->bmcr;
        case MII_BMSR:
            return mii->bmsr;
        case MII_PHYID1:
            return RTL8201CP_PHYID1;
        case MII_PHYID2:
            return RTL8201CP_PHYID2;
        case MII_ANAR:
            return mii->anar;
        case MII_ANLPAR:
            return mii->anlpar;
        case MII_SNRDR:
        case MII_ANER:
        case MII_NSR:
        case MII_LBREMR:
        case MII_REC:
        case MII_TEST:
            // qemu_log_mask(LOG_UNIMP,
            //               "dwc_emac: read from unimpl. mii reg 0x%x\n",
            //               reg);
            return 0;
        default:
            qemu_log_mask(LOG_GUEST_ERROR,
                          "dwc_emac: read from invalid mii reg 0x%x\n",
                          reg);
            return 0;
        }
    }
    return ret;
}


static void rtl8201cp_mdio_write(DwcEMACState *s, uint8_t addr, uint8_t reg,
                                 uint16_t value)
{
    RTL8201CPState *mii = &s->mii;
    NetClientState *nc;

    // printf("dwc_emac: write mii addr=0x%x, reg=0x%x, value=0x%x\n", addr, reg, value);
    if (addr == s->phy_addr) {
        switch (reg) {
        case MII_BMCR:
            if (value & MII_BMCR_RESET) {
                nc = qemu_get_queue(s->nic);
                mii_reset(mii, !nc->link_down);
            } else {
                mii->bmcr = value;
            }
            break;
        case MII_ANAR:
            mii->anar = value;
            break;
        case MII_BMSR:
        case MII_PHYID1:
        case MII_PHYID2:
        case MII_ANLPAR:
        case MII_ANER:
            qemu_log_mask(LOG_GUEST_ERROR,
                          "dwc_emac: write to read-only mii reg 0x%x\n",
                          reg);
            break;
        case MII_NSR:
        case MII_LBREMR:
        case MII_REC:
        case MII_SNRDR:
        case MII_TEST:
            qemu_log_mask(LOG_UNIMP,
                          "dwc_emac: write to unimpl. mii reg 0x%x\n",
                          reg);
            break;
        default:
            qemu_log_mask(LOG_GUEST_ERROR,
                          "dwc_emac: write to invalid mii reg 0x%x\n",
                          reg);
        }
    }
}


//=================================================================================================

#define DEBUG_DWC_EMAC
#ifdef DEBUG_DWC_EMAC
#define DEBUGF_BRK(message, args...) do { \
                                         fprintf(stderr, (message), ## args); \
                                     } while (0)
#else
#define DEBUGF_BRK(message, args...) do { } while (0)
#endif

#define DWC_EMAC_CONTROL            (0x00000000 >> 2)   /* MAC Configuration */
#define DWC_EMAC_FRAME_FILTER       (0x00000004 >> 2)   /* MAC Frame Filter */

#define DWC_EMAC_GMII_ADDR          (0x00000010 >> 2)   /* GMII Address */
#define DWC_EMAC_GMII_DATA          (0x00000014 >> 2)   /* GMII Data*/

#define DWC_EMAC_FLOW_CTRL          (0x00000018 >> 2)   /* MAC Flow Control */
#define DWC_EMAC_VLAN_TAG           (0x0000001C >> 2)   /* VLAN Tags */
#define DWC_EMAC_VERSION            (0x00000020 >> 2)   /* Version */
/* VLAN tag for insertion or replacement into tx frames */
#define DWC_EMAC_VLAN_INCL          (0x00000024 >> 2)   // ?
#define DWC_EMAC_LPI_CTRL           (0x00000030 >> 2)   /* LPI Control and Status */
#define DWC_EMAC_LPI_TIMER          (0x00000034 >> 2)  /* LPI Timers Control */
#define DWC_EMAC_TX_PACE            (0x0000000c >> 2)  /* Transmit Pace and Stretch */
#define DWC_EMAC_VLAN_HASH          (0x0000000d >> 2)  /* VLAN Hash Table */
#define DWC_EMAC_DEBUG              (0x0000000e >> 2)  /* Debug */
#define DWC_EMAC_INT_STATUS         (0x00000038 >> 2)  /* Interrupt and Control */
/* HASH table registers */
#define DWC_EMAC_HASH(n)            ((0x00000300/4) + (n))
#define DWC_EMAC_NUM_HASH           16
/* Operation Mode */
#define DWC_EMAC_OPMODE             (0x00000400/4)
/* Remote Wake-Up Frame Filter */
#define DWC_EMAC_REMOTE_WAKE        (0x00000700/4)
/* PMT Control and Status */
#define DWC_EMAC_PMT                (0x00000704/4)

#define DWC_EMAC_ADDR_HIGH(reg)     (0x00000010 + ((reg) * 2))
#define DWC_EMAC_ADDR_LOW(reg)      (0x00000011 + ((reg) * 2))

#define DMA_BUS_MODE                (0x00001000 >> 2)   /* Bus Mode */
#define DMA_XMT_POLL_DEMAND         (0x00001004 >> 2)  /* Transmit Poll Demand */
#define DMA_RCV_POLL_DEMAND         (0x00001008 >> 2)  /* Received Poll Demand */
#define DMA_RCV_BASE_ADDR           (0x0000100C >> 2)  /* Receive List Base */
#define DMA_TX_BASE_ADDR            (0x00001010 >> 2)  /* Transmit List Base */
#define DMA_STATUS                  (0x00001014 >> 2)  /* Interrupt Status Register */
#define DMA_CONTROL                 (0x00001018 >> 2)  /* Ctrl (Operational Mode) */
#define DMA_INTR_ENA                (0x0000101C >> 2)  /* Interrupt Enable */
#define DMA_MISSED_FRAME_CTR        (0x00001020 >> 2)  /* Missed Frame Counter */
/* Receive Interrupt Watchdog Timer */
#define DMA_RI_WATCHDOG_TIMER       (0x00001024 >> 2)
#define DMA_AXI_BUS                 (0x00001028 >> 2)   /* AXI Bus Mode */
#define DMA_AXI_STATUS              (0x0000102C >> 2)   /* AXI Status */
#define DMA_CUR_TX_DESC_ADDR        (0x00001048 >> 2)   /* Current Host Tx Descriptor */
#define DMA_CUR_RX_DESC_ADDR        (0x0000104C >> 2)   /* Current Host Rx Descriptor */
#define DMA_CUR_TX_BUF_ADDR         (0x00001050 >> 2)   /* Current Host Tx Buffer */
#define DMA_CUR_RX_BUF_ADDR         (0x00001054 >> 2)   /* Current Host Rx Buffer */
#define DMA_HW_FEATURE              (0x00001058 >> 2)   /* Enabled Hardware Features */

/* DMA Status register defines */
#define DMA_STATUS_GMI              0x08000000   /* MMC interrupt */
#define DMA_STATUS_GLI              0x04000000   /* GMAC Line interface int */
#define DMA_STATUS_EB_MASK          0x00380000   /* Error Bits Mask */
#define DMA_STATUS_EB_TX_ABORT      0x00080000   /* Error Bits - TX Abort */
#define DMA_STATUS_EB_RX_ABORT      0x00100000   /* Error Bits - RX Abort */
#define DMA_STATUS_TS_MASK          0x00700000   /* Transmit Process State */
#define DMA_STATUS_TS_SHIFT         20
#define DMA_STATUS_RS_MASK          0x000e0000   /* Receive Process State */
#define DMA_STATUS_RS_SHIFT         17
#define DMA_STATUS_NIS              0x00010000   /* Normal Interrupt Summary */
#define DMA_STATUS_AIS              0x00008000   /* Abnormal Interrupt Summary */
#define DMA_STATUS_ERI              0x00004000   /* Early Receive Interrupt */
#define DMA_STATUS_FBI              0x00002000   /* Fatal Bus Error Interrupt */
#define DMA_STATUS_ETI              0x00000400   /* Early Transmit Interrupt */
#define DMA_STATUS_RWT              0x00000200   /* Receive Watchdog Timeout */
#define DMA_STATUS_RPS              0x00000100   /* Receive Process Stopped */
#define DMA_STATUS_RU               0x00000080   /* Receive Buffer Unavailable */
#define DMA_STATUS_RI               0x00000040   /* Receive Interrupt */
#define DMA_STATUS_UNF              0x00000020   /* Transmit Underflow */
#define DMA_STATUS_OVF              0x00000010   /* Receive Overflow */
#define DMA_STATUS_TJT              0x00000008   /* Transmit Jabber Timeout */
#define DMA_STATUS_TU               0x00000004   /* Transmit Buffer Unavailable */
#define DMA_STATUS_TPS              0x00000002   /* Transmit Process Stopped */
#define DMA_STATUS_TI               0x00000001   /* Transmit Interrupt */

/* DMA Control register defines */
#define DMA_CONTROL_ST              0x00002000   /* Start/Stop Transmission */
#define DMA_CONTROL_SR              0x00000002   /* Start/Stop Receive */
#define DMA_CONTROL_DFF             0x01000000   /* Disable flush of rx frames */

/*
struct desc {
    uint32_t ctl_stat;
    uint16_t buffer1_size;
    uint16_t buffer2_size;
    uint32_t buffer1_addr;
    uint32_t buffer2_addr;
    uint32_t ext_stat;
    uint32_t res[3];
};
*/

typedef union {
	unsigned int words[4];
	struct {
		unsigned db:1;    // Deferred Bit
		unsigned uf:1;    // Underflow Error
		unsigned ed:1;    // Excessive Deferral
		unsigned cc:4;    // Collision Count
		unsigned vf:1;    // VLAN Frame
		unsigned ec:1;    // Excessive Collision
		unsigned lc:1;    // Late Collision
		unsigned nc:1;    // No Carrier
		unsigned loc:1;   // Loss of Carrier
		unsigned pce:1;   // Payload Checksum Error
		unsigned ff:1;    // Frame Flushed
		unsigned jt:1;    // Jabber Timeout
		unsigned es:1;    // Error Summary
		unsigned ihe:1;   // IP Header Error
		unsigned ttss:1;  // Tx Timestamp Status
		unsigned :13;
		unsigned own:1;   // Own Bit

		unsigned tbs1:11; // Transmit Buffer 1 Size
		unsigned tbs2:11; // Transmit Buffer 2 Size
		unsigned ttse:1;  // Transmit Timestamp Enable
		unsigned dp:1;    // Disable Padding
		unsigned tch:1;   // Second Address Control
		unsigned ter:1;   // Transmit End of Ring
		unsigned dc:1;    // Disable CRC
		unsigned cic:2;   // Checksum Insertion Control
		unsigned fs:1;    // First Segment
		unsigned ls:1;    // Last Segment
		unsigned ic:1;    // Interrupt on Completion

		unsigned b1ap;
		unsigned b2ap;
	} tx;
	struct {
		unsigned rma:1;   // Rx MAC Address or Payload Checksum Error
		unsigned ce:1;    // CRC Error
		unsigned dbe:1;   // Driblle Bit Error
		unsigned re:1;    // Receive Error
		unsigned rwt:1;   // Receive Watchdog Timeout
		unsigned ft:1;    // Frame Type
		unsigned lc:1;    // Late Collision
		unsigned ice:1;   // IPC Checksum Error or Giant Frame
		unsigned ls:1;    // Last Description
		unsigned fs:1;    // First Description
		unsigned vlan:1;  // VLAN Tag
		unsigned oe:1;    // Overflow Error
		unsigned le:1;    // Length Error
		unsigned saf:1;   // Source Address Filter Fail
		unsigned de:1;    // Descriptor Error
		unsigned es:1;    // Error Summary
		unsigned fl:14;   // Frame Length
		unsigned afm:1;   // Destination Address Filter Fail
		unsigned own:1;   // Own Bit

		unsigned rbs1:11; // Receive Buffer 1 Size
		unsigned rbs2:11; // Receive Buffer 2 Size
		unsigned :2;
		unsigned rch:1;   // Second Address Chained
		unsigned per:1;   // Receive End of Ring
		unsigned :5;
		unsigned dic:1;   // Disable Interrupt on Completion

		unsigned b1ap;
		unsigned b2ap;
	} rx;
	struct {
		unsigned status:16;
		unsigned ttss:1;
		unsigned :14;
		unsigned own:1;

		unsigned bbc1:11;
		unsigned bbc2:11;
		unsigned ttse:1;
		unsigned :9;

		unsigned ttsl:32;
		unsigned ttsh:32;
	} ts;
} desc;

static const VMStateDescription vmstate_rxtx_stats = {
    .name = "dwc_emac_stats",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT64(rx_bytes, RxTxStats),
        VMSTATE_UINT64(tx_bytes, RxTxStats),
        VMSTATE_UINT64(rx, RxTxStats),
        VMSTATE_UINT64(rx_bcast, RxTxStats),
        VMSTATE_UINT64(rx_mcast, RxTxStats),
        VMSTATE_END_OF_LIST()
    }
};

static const VMStateDescription vmstate_dwc_emac = {
    .name = TYPE_DWC_EMAC,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_STRUCT(stats, DwcEMACState, 0, vmstate_rxtx_stats, RxTxStats),
        VMSTATE_UINT32_ARRAY(regs, DwcEMACState, R_MAX),
        VMSTATE_END_OF_LIST()
    }
};

static void dwc_emac_read_desc(DwcEMACState *s, desc *d, int rx)
{
    uint32_t addr = rx ? s->regs[DMA_CUR_RX_DESC_ADDR] : s->regs[DMA_CUR_TX_DESC_ADDR];
    cpu_physical_memory_read(addr, d, sizeof(*d));
#if 0
    printf("  %s: desc %s @ %x\n", __FUNCTION__, rx ? "rx" : "tx", addr);
    for (int i = 0; i < 4; i++) {
        printf("    %x\n", d->words[i]);
    }
#endif
}

static void dwc_emac_write_desc(DwcEMACState *s, desc *d, int rx)
{
    int reg = rx ? DMA_CUR_RX_DESC_ADDR : DMA_CUR_TX_DESC_ADDR;
    uint32_t addr = s->regs[reg];

    if (!rx && d->tx.ter) {
        s->regs[reg] = s->regs[DMA_TX_BASE_ADDR];
    }
    else if (rx && d->rx.per) {
        s->regs[reg] = s->regs[DMA_RCV_BASE_ADDR];
    }
    else {
        s->regs[reg] += sizeof(*d) + s->desc_skip;
    }

#if 0
    printf("  %s: desc %s @ %x\n", __FUNCTION__, rx ? "rx" : "tx", addr);
    for (int i = 0; i < 4; i++) {
        printf("    %x\n", d->words[i]);
    }
#endif
    cpu_physical_memory_write(addr, d, sizeof(*d));
}

static void dwc_emac_send(DwcEMACState *s)
{
    desc bd;
    int frame_size;
    int len;
    uint8_t frame[8192];
    uint8_t *ptr;

    ptr = frame;
    frame_size = 0;
    while (1) {
        dwc_emac_read_desc(s, &bd, 0);
        // if ((bd.ctl_stat & 0x80000000) == 0) {
        if (bd.tx.own == 0) {
            /* Run out of descriptors to transmit.  */
            break;
        }
        // len = (bd.buffer1_size & 0xfff) + (bd.buffer2_size & 0xfff);
        len = bd.tx.tbs1 + bd.tx.tbs2;

        /*
         * FIXME: these cases of malformed tx descriptors (bad sizes)
         * should probably be reported back to the guest somehow
         * rather than simply silently stopping processing, but we
         * don't know what the hardware does in this situation.
         * This will only happen for buggy guests anyway.
         */
        // if ((bd.buffer1_size & 0xfff) > 2048) {
        //     DEBUGF_BRK("qemu:%s:ERROR...ERROR...ERROR... -- "
        //                 "dwc_emac buffer 1 len on send > 2048 (0x%x)\n",
        //                  __func__, bd.buffer1_size & 0xfff);
        //     break;
        // }
        // if ((bd.buffer2_size & 0xfff) != 0) {
        if (bd.tx.tbs2) {
            DEBUGF_BRK("qemu:%s:ERROR...ERROR...ERROR... -- "
                        "dwc_emac buffer 2 len on send != 0 (0x%x)\n",
                        __func__, bd.tx.tbs2);
            break;
        }
        if (frame_size + len >= sizeof(frame)) {
            DEBUGF_BRK("qemu:%s: buffer overflow %d read into %zu "
                        "buffer\n" , __func__, frame_size + len, sizeof(frame));
            DEBUGF_BRK("qemu:%s: buffer1.size=%d; buffer2.size=%d\n",
                        __func__, bd.tx.tbs1, bd.tx.tbs2);
            break;
        }

        cpu_physical_memory_read(bd.tx.b1ap, ptr, len);
        ptr += len;
        frame_size += len;
        if (bd.tx.ls) {
            /* Last buffer in frame.  */
            // printf("qemu_send_packet, len = %d\n", len);
            qemu_send_packet(qemu_get_queue(s->nic), frame, len);
            ptr = frame;
            frame_size = 0;
            s->regs[DMA_STATUS] |= DMA_STATUS_TI | DMA_STATUS_NIS;
        }
        bd.tx.own = 0;
        /* Write back the modified descriptor.  */
        dwc_emac_write_desc(s, &bd, 0);
    }
}

static void dwc_emac_update_irq(DwcEMACState *s)
{
    int stat = s->regs[DMA_STATUS] & s->regs[DMA_INTR_ENA];
    qemu_set_irq(s->sbd_irq, !!stat);
}

static uint64_t dwc_emac_read(void *opaque, hwaddr addr, unsigned size)
{
    DwcEMACState *s = opaque;
    uint64_t r = 0;
    addr >>= 2;

    switch (addr) {
    case DWC_EMAC_VERSION:
        r = 0x0037;
        break;
    default:
        if (addr < ARRAY_SIZE(s->regs)) {
            r = s->regs[addr];
        }
        break;
    }

    // printf("dwc_emac r 0x%lX => 0x%lx\n", addr << 2, r);
    return r;
}

static void dwc_emac_write(void *opaque, hwaddr addr,
                       uint64_t value, unsigned size)
{
    DwcEMACState *s = opaque;
    // printf("dwc_emac w 0x%lX <= 0x%lx\n", addr, value);
    addr >>= 2;
    switch (addr) {
    case DWC_EMAC_GMII_ADDR:
        if (value & 0x2) {
            rtl8201cp_mdio_write(s, (value >> 11) & 0x1F, (value >> 6) & 0x1F, s->regs[DWC_EMAC_GMII_DATA] & 0xFFFF);
        }
        else {
            s->regs[DWC_EMAC_GMII_DATA] = rtl8201cp_mdio_read(s, (value >> 11) & 0x1F, (value >> 6) & 0x1F);
        }
        s->regs[DWC_EMAC_GMII_ADDR] = value & ~0x1;
        break;
    case DMA_BUS_MODE:
        s->desc_skip = ((value >> 2) & 0x1F) * ((value >> 8) & 0x3F) / 8;
        // printf("~~~~ s->desc_skip = %d\n", s->desc_skip);
        s->regs[DMA_BUS_MODE] = value & ~0x1;
        break;
    case DMA_XMT_POLL_DEMAND:
        dwc_emac_send(s);
        break;
    case DMA_STATUS:
        s->regs[DMA_STATUS] = s->regs[DMA_STATUS] & ~value;
        break;
    case DMA_RCV_BASE_ADDR:
        s->regs[DMA_RCV_BASE_ADDR] = s->regs[DMA_CUR_RX_DESC_ADDR] = value;
        break;
    case DMA_TX_BASE_ADDR:
        s->regs[DMA_TX_BASE_ADDR] = s->regs[DMA_CUR_TX_DESC_ADDR] = value;
        break;
    default:
        if (addr < ARRAY_SIZE(s->regs)) {
            s->regs[addr] = value;
        }
        break;
    }
    dwc_emac_update_irq(s);
}

static const MemoryRegionOps dwc_emac_mem_ops = {
    .read = dwc_emac_read,
    .write = dwc_emac_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static int eth_can_rx(DwcEMACState *s)
{
    /* RX enabled?  */
    return s->regs[DMA_CONTROL] & DMA_CONTROL_SR;
}

static ssize_t eth_rx(NetClientState *nc, const uint8_t *buf, size_t size)
{
    DwcEMACState *s = qemu_get_nic_opaque(nc);
    static const unsigned char sa_bcast[6] = {0xff, 0xff, 0xff,
                                              0xff, 0xff, 0xff};
    int unicast, broadcast, multicast;
    desc bd;
    ssize_t ret;

    // printf("eth_rx, len = %ld\n", size);

    if (!eth_can_rx(s)) {
        return -1;
    }
    unicast = ~buf[0] & 0x1;
    broadcast = memcmp(buf, sa_bcast, 6) == 0;
    multicast = !unicast && !broadcast;
    if (size < 12) {
        s->regs[DMA_STATUS] |= DMA_STATUS_RI | DMA_STATUS_NIS;
        ret = -1;
        goto out;
    }

    dwc_emac_read_desc(s, &bd, 1);
    if ((bd.rx.own) == 0) {
        s->regs[DMA_STATUS] |= DMA_STATUS_RU | DMA_STATUS_AIS;
        ret = size;
        goto out;
    }

    cpu_physical_memory_write(bd.rx.b1ap, buf, size);

    /* Add in the 4 bytes for crc (the real hw returns length incl crc) */
    size += 4;
    bd.rx.fl = size;
    bd.rx.own = 0;
    bd.rx.ls = 1;
    bd.rx.fs = 1;
    dwc_emac_write_desc(s, &bd, 1);

    s->stats.rx_bytes += size;
    s->stats.rx++;
    if (multicast) {
        s->stats.rx_mcast++;
    } else if (broadcast) {
        s->stats.rx_bcast++;
    }

    s->regs[DMA_STATUS] |= DMA_STATUS_RI | DMA_STATUS_NIS;
    ret = size;

out:
    dwc_emac_update_irq(s);
    return ret;
}

static void dwc_emac_set_link(NetClientState *nc)
{
    DwcEMACState *s = qemu_get_nic_opaque(nc);
    mii_set_link(&s->mii, !nc->link_down);
}

static NetClientInfo net_dwc_emac_info = {
    .type = NET_CLIENT_DRIVER_NIC,
    .size = sizeof(NICState),
    .receive = eth_rx,
    .link_status_changed = dwc_emac_set_link,
};

static void dwc_emac_realize(DeviceState *dev, Error **errp)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
    DwcEMACState *s = DWC_EMAC(dev);

    memory_region_init_io(&s->iomem, OBJECT(s), &dwc_emac_mem_ops, s, "dwc_emac", sizeof(s->regs));
    sysbus_init_mmio(sbd, &s->iomem);
    sysbus_init_irq(sbd, &s->sbd_irq);
    sysbus_init_irq(sbd, &s->pmt_irq);
    sysbus_init_irq(sbd, &s->mci_irq);

    qemu_macaddr_default_if_unset(&s->conf.macaddr);
    s->nic = qemu_new_nic(&net_dwc_emac_info, &s->conf,
                          object_get_typename(OBJECT(dev)), dev->id, s);
    qemu_format_nic_info_str(qemu_get_queue(s->nic), s->conf.macaddr.a);

    s->regs[DWC_EMAC_GMII_ADDR] = 0;
    s->regs[DWC_EMAC_ADDR_HIGH(0)] = (s->conf.macaddr.a[5] << 8)  | s->conf.macaddr.a[4];
    s->regs[DWC_EMAC_ADDR_LOW(0)]  = (s->conf.macaddr.a[3] << 24) |
                                     (s->conf.macaddr.a[2] << 16) |
                                     (s->conf.macaddr.a[1] << 8)  | s->conf.macaddr.a[0];

    NetClientState *nc = qemu_get_queue(s->nic);
    mii_reset(&s->mii, !nc->link_down);
}

static Property dwc_emac_properties[] = {
    DEFINE_NIC_PROPERTIES(DwcEMACState, conf),
    DEFINE_PROP_UINT8("phy-addr", DwcEMACState, phy_addr, 1),
    DEFINE_PROP_END_OF_LIST(),
};

static void dwc_emac_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = dwc_emac_realize;
    dc->vmsd = &vmstate_dwc_emac;
    device_class_set_props(dc, dwc_emac_properties);
}

static const TypeInfo dwc_emac_info = {
    .name          = TYPE_DWC_EMAC,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(DwcEMACState),
    .class_init    = dwc_emac_class_init,
};

static void dwc_emac_register_types(void)
{
    type_register_static(&dwc_emac_info);
}

type_init(dwc_emac_register_types)
