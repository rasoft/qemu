/*
 * Designware Synchronous Serial Interface
 * Alan.REN
 *
 * Copyright 2020 NationalChip
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
// #include "qemu-common.h"
#include "qemu/log.h"
#include "hw/qdev-properties.h"
#include "hw/block/flash.h"
#include "sysemu/blockdev.h"
#include "migration/vmstate.h"

#include "dwc_ssi.h"

// #define DEBUG
#ifdef DEBUG
#define LOG(x...) printf("[SSI] "x)
#else
#define LOG(x...) while(0)
#endif
#define NLOG(x...) while(0)

#define FIFO_CAPACITY   32

#define CTRLR0_TMOD_TX_AND_RX   0
#define CTRLR0_TMOD_TX_ONLY     1
#define CTRLR0_TMOD_RX_ONLY     2
#define CTRLR0_TMOD_EEPROM_READ 3

#define DEFAULT_FLASH_TYPE  "w25q64"

static void _dwc_ssi_update_irq(DwcSsiState *ssi)
{
    qemu_set_irq(ssi->irq, !!(ssi->risr.value & ssi->imr.value));
}

static void _dwc_ssi_update_dma(DwcSsiState *ssi)
{
    // TX
    int tx_flag = fifo32_num_used(&ssi->tx_fifo) <= ssi->dmatdlr.dmatdl;
    qemu_set_irq(ssi->tx_dma, tx_flag && ssi->dmacr.tdmae);
    LOG("tx_flag = %d\n", tx_flag && ssi->dmacr.tdmae);

    // RX
    int rx_flag = fifo32_num_used(&ssi->rx_fifo) > 0;
    qemu_set_irq(ssi->rx_dma, rx_flag && ssi->dmacr.rdmae);
    LOG("rx_flag = %d\n", rx_flag && ssi->dmacr.rdmae);
}

// Transmit
static void _dwc_ssi_tx(DwcSsiState *ssi)
{
    // Flush all data outside
    while (!fifo32_is_empty(&ssi->tx_fifo)) {
        uint32_t word = fifo32_pop(&ssi->tx_fifo);
        LOG("Send 0x%08X\n", word);

        word = ssi_transfer(ssi->ssi, word);
        ssi->tx_count++;

        if (ssi->ctrlr0.tmod == CTRLR0_TMOD_TX_AND_RX) {
            fifo32_push(&ssi->rx_fifo, word);
            ssi->rx_count++;
        }
    }

    // Update TXEIR
    ssi->risr.txeir = 1;
    _dwc_ssi_update_irq(ssi);

    // Update DMA
    _dwc_ssi_update_dma(ssi);
}

// Receive
static void _dwc_ssi_rx(DwcSsiState *ssi)
{
    // Do Transfer
    while ((!fifo32_is_full(&ssi->rx_fifo)) && ssi->rx_count < (ssi->ctrlr1.ndf + 1)) {
        uint32_t word = ssi_transfer(ssi->ssi, 0);
        LOG("Recv 0x%08X\n", word);
        ssi->rx_count++;
        fifo32_push(&ssi->rx_fifo, word);
    }

    // Update RXFIR
    if (fifo32_num_used(&ssi->rx_fifo) > ssi->rxftlr.value) {
        ssi->risr.rxfir = 1;
        _dwc_ssi_update_irq(ssi);
    }

    // Update DMA
    _dwc_ssi_update_dma(ssi);
}

// TX and RX
static void _dwc_ssi_enhance_xfer(DwcSsiState *ssi)
{
    if (!ssi->inst_addr_sent) { // Send Instr and Addr first
        uint32_t addr_word_num = (ssi->spi_ctrlr0.addr_l * 4 + 31) / 32;
        uint32_t inst_word_num = (ssi->spi_ctrlr0.inst_l ? 1 : 0);
        if (fifo32_num_used(&ssi->tx_fifo) < addr_word_num + inst_word_num)
            return;

        LOG("enhance_xfer: addr_word_num = %d, inst_word_num = %d\n", addr_word_num, inst_word_num);

        if (ssi->spi_ctrlr0.trans_type == 0) {   // Instruction and Address will be sent in Standard SPI Mode
            // Send Instruction in Standard SPI Mode
            if (inst_word_num) {
                uint32_t inst_word = fifo32_pop(&ssi->tx_fifo);
                uint32_t inst_byte_num = ssi->spi_ctrlr0.inst_l != 3 ? 1 : 2;
                for (int i = 0; i < inst_byte_num; i++) {
                    uint32_t word = (inst_word >> (i * 8)) & 0xFF;
                    ssi_transfer(ssi->ssi, word);
                    LOG("SSI Send 0x%08X\n", word);
                }
            }
            // Send Address in Standard SPI Mode
            if (addr_word_num > 1) {
                for (int i = 0; i < addr_word_num; i++) {
                    uint32_t addr_word = fifo32_pop(&ssi->tx_fifo);
                    ssi_transfer(ssi->ssi, addr_word);
                    LOG("SSI Send 0x%08X\n", addr_word);
                }
            }
            else {
                uint32_t addr_word = fifo32_pop(&ssi->tx_fifo);
                uint32_t addr_byte_num = (ssi->spi_ctrlr0.addr_l * 4 + 7) / 8;
                // LOG("addr_byte_num = %d\n", addr_byte_num);
                for (int i = addr_byte_num - 1; i >= 0; i --) {
                    uint32_t word = (addr_word >> (i * 8)) & 0xFF;
                    ssi_transfer(ssi->ssi, word);
                    LOG("SSI Send 0x%08X\n", word);
                }
            }
        }
        else if (ssi->spi_ctrlr0.trans_type == 1) {  // Instruction in Standard SPI Mode and Address in SPI_FRF
            // Send Instruction in Standard SPI Mode
            if (inst_word_num) {
                uint32_t inst_word = fifo32_pop(&ssi->tx_fifo);
                uint32_t inst_byte_num = ssi->spi_ctrlr0.inst_l != 3 ? 1 : 2;
                for (int i = 0; i < inst_byte_num; i++) {
                    uint32_t word = (inst_word >> (i * 8)) & 0xFF;
                    ssi_transfer(ssi->ssi, word);
                    LOG("SSI Send 0x%08X\n", word);
                }
            }
            // Send Address in SPI_FRF (TODO)
            for (int i = 0; i < addr_word_num; i++) {
                uint32_t addr_word = fifo32_pop(&ssi->tx_fifo);
                ssi_transfer(ssi->ssi, addr_word);
                LOG("SSI Send 0x%08X\n", addr_word);
            }
        }
        else if (ssi->spi_ctrlr0.trans_type == 2) {  // Instruction and Address in SPI_FRF
            // TODO:
            uint32_t inst_word = fifo32_pop(&ssi->tx_fifo);
            uint32_t addr_word = fifo32_pop(&ssi->tx_fifo);
            ssi_transfer(ssi->ssi,  (inst_word & 0xFF) << 24 | (addr_word & 0xFFFFFF));
            LOG("SSI Send 0x%08X\n", addr_word);
        }

        // Mark State
        ssi->inst_addr_sent = 1;

        // if RX_ONLY mode, recv dummy first
        if (ssi->ctrlr0.tmod == CTRLR0_TMOD_RX_ONLY) {
            // Wait for some cycles
            for (int i = 0; i < ssi->spi_ctrlr0.wait_cycles; i++)
                ssi_transfer(ssi->ssi, 0);

            _dwc_ssi_rx(ssi);
        }
    }

    if (ssi->ctrlr0.tmod == CTRLR0_TMOD_EEPROM_READ) {
        LOG("Error mode: CTRLR0_TMOD_EEPROM_READ in Dual/Quad/Octrl mode\n");
        return;
    }

    if (ssi->ctrlr0.tmod == CTRLR0_TMOD_RX_ONLY) {
        LOG("Error mode: CTRLR0_TMOD_RX_ONLY in Dual/Quad/Octrl mode after instr/addr is sent!\n");
        return;
    }

    // Normal Sent
    _dwc_ssi_tx(ssi);
}


static void _dwc_ssi_normal_xfer(DwcSsiState *ssi)
{
    // Flush data to bus
    _dwc_ssi_tx(ssi);

    // if EEPROM_READ mode, recv something first
    if (ssi->ctrlr0.tmod == CTRLR0_TMOD_EEPROM_READ) {
        _dwc_ssi_rx(ssi);
    }
}

// Register Read
static uint64_t _dwc_ssi_read(void *opaque, hwaddr offset, unsigned size)
{
    DwcSsiState *ssi = opaque;
    LOG("Read 0x%08X\n", (unsigned int)offset);

    // 0x000
    if (offset == DWC_SSIC_REG_CTRLR0) {
        return ssi->ctrlr0.value;
    }

    // 0x004
    if (offset == DWC_SSIC_REG_CTRLR1) {
        return ssi->ctrlr1.value;
    }

    // 0x008
    if (offset == DWC_SSIC_REG_SSIENR) {
        return ssi->enable;
    }

    // 0x010
    if (offset == DWC_SSIC_REG_SER) {
        return ssi->ser.value;
    }

    // 0x014
    if (offset == DWC_SSIC_REG_BAUDR) {
        return ssi->baudr.value;
    }

    // 0x018
    if (offset == DWC_SSIC_REG_TXFTLR) {
        return ssi->txftlr.value;
    }

    // 0x01C
    if (offset == DWC_SSIC_REG_RXFTLR) {
        return ssi->rxftlr.value;
    }

    // 0x020
    if (offset == DWC_SSIC_REG_TXFLR) {
        return fifo32_num_used(&ssi->tx_fifo);
    }

    // 0x024
    if (offset == DWC_SSIC_REG_RXFLR) {
        return fifo32_num_used(&ssi->rx_fifo);
    }

    // 0x028
    if (offset == DWC_SSIC_REG_SR) {
        DWC_SSIC_SR sr;
        sr.value = 0;
        sr.tfnf = !fifo32_is_full(&ssi->tx_fifo);
        sr.tfe  =  fifo32_is_empty(&ssi->tx_fifo);
        sr.rfne = !fifo32_is_empty(&ssi->rx_fifo);
        sr.rff  =  fifo32_is_full(&ssi->rx_fifo);

        return sr.value;
    }

    // 0x02C
    if (offset == DWC_SSIC_REG_IMR) {
        return ssi->imr.value;
    }

    // 0x030
    if (offset == DWC_SSIC_REG_ISR) {
        return ssi->imr.value & ssi->risr.value;
    }

    // 0x034
    if (offset == DWC_SSIC_REG_RISR) {
        return ssi->risr.value;
    }

    // 0x038
    if (offset == DWC_SSIC_REG_TXOICR) {
        unsigned int txoicr = ssi->risr.txoir;
        ssi->risr.txoir = 0;
        _dwc_ssi_update_irq(ssi);
        return txoicr;
    }

    // 0x03C
    if (offset == DWC_SSIC_REG_RXOICR) {
        unsigned int rxoicr = ssi->risr.rxoir;
        ssi->risr.rxoir = 0;
        _dwc_ssi_update_irq(ssi);
        return rxoicr;
    }

    // 0x040
    if (offset == DWC_SSIC_REG_RXUICR) {
        unsigned int rxuicr = ssi->risr.rxuir;
        ssi->risr.rxuir = 0;
        _dwc_ssi_update_irq(ssi);
        return rxuicr;
    }

    // 0x044
    if (offset == DWC_SSIC_REG_MSTICR) {
        unsigned int msticr = ssi->risr.mstir;
        ssi->risr.mstir = 0;
        _dwc_ssi_update_irq(ssi);
        return msticr;
    }

    // 0x04C
    if (offset == DWC_SSIC_REG_DMACR) {
        return ssi->dmacr.value;
    }

    // 0x050
    if (offset == DWC_SSIC_REG_DMATDLR) {
        return ssi->dmatdlr.value;
    }

    // 0x054
    if (offset == DWC_SSIC_REG_DMARDLR) {
        return ssi->dmardlr.value;
    }

    // 0x058
    if (offset == DWC_SSIC_REG_IDR) {
        return 0x20210205;
    }

    // 0x05C
    if (offset == DWC_SSIC_REG_VERSION_ID) {
        return 0x3230312A;
    }

    // 0x060 - 0x0EF
    if (offset >= DWC_SSIC_REG_DR00 && offset <= DWC_SSIC_REG_DR35) {
        // if in TX only mode, do nothing.
        if (ssi->ctrlr0.tmod == CTRLR0_TMOD_TX_ONLY)
            return 0;

        // read data from bus
        _dwc_ssi_rx(ssi);

        if (fifo32_is_empty(&ssi->rx_fifo)) {
            ssi->risr.rxuir = 1;
            _dwc_ssi_update_irq(ssi);
        }
        return fifo32_pop(&ssi->rx_fifo);
    }

    // 0x0F0
    if (offset == DWC_SSIC_REG_RX_SAMPLE_DELAY) {
        return ssi->rx_sample_delay.value;
    }

    // 0x0F4
    if (offset == DWC_SSIC_REG_SPI_CTRLR0) {
        return ssi->spi_ctrlr0.value;
    }

    return 0;
}

// Register Write
static void _dwc_ssi_write(void *opaque, hwaddr offset, uint64_t value, unsigned size)
{
    DwcSsiState *ssi = opaque;

    LOG("Write 0x%08X => 0x%08X\n", (unsigned int)value, (unsigned int)offset);

    if (offset == DWC_SSIC_REG_CTRLR0) {
        if (ssi->enable) {
            printf("[TREC.SSI] Error: Write 0x%08X => CTRLR0 while enable!\n", (uint32_t)value);
            return;
        }

        ssi->ctrlr0.value = value;
        LOG("ctrlr0 = {dfs: %d, frf: %d, scph: %d, scpol: %d, tmod: %d,\n"
            "          slv_oe: %d, srl: %d, sste: %d, cfs: %d, spi_frf: %d,\n"
            "          spi_hyperbus_en: %d, ssi_is_mst: %d}\n",
            ssi->ctrlr0.dfs, ssi->ctrlr0.frf, ssi->ctrlr0.scph, ssi->ctrlr0.scpol, ssi->ctrlr0.tmod,
            ssi->ctrlr0.slv_oe, ssi->ctrlr0.srl, ssi->ctrlr0.sste, ssi->ctrlr0.cfs, ssi->ctrlr0.spi_frf,
            ssi->ctrlr0.spi_hyperbus_en, ssi->ctrlr0.ssi_is_mst);
        return;
    }

    if (offset == DWC_SSIC_REG_CTRLR1) {
        if (ssi->enable) {
            printf("[TREC.SSI] Error: Write 0x%08X => CTRLR1 while enable!\n", (uint32_t)value);
            return;
        }
        ssi->ctrlr1.value = value;
        LOG("ctrlr1 = {ndf: %d}\n", ssi->ctrlr1.ndf);
        return;
    }

    if (offset == DWC_SSIC_REG_SSIENR) {
        // Enable and disable all DWC_ssi operations.
        DWC_SSIC_SSIENR req;
        req.value = value;
        ssi->enable = req.ssic_en;
        LOG("ssienr = {ssic_en: %d}\n", req.ssic_en);
        if (req.ssic_en) {
            // when enable,
            // * it is impossible to program some of the DWC_ssi control registers
            // * ssi sleep output is set to inform the system that it is safe to remove the ssi_clk
            // if (ssi->ctrlr0.tmod == CTRLR0_TMOD_RX_ONLY) {
            //     _dwc_ssi_rx(ssi);
            // }
            qemu_set_irq(ssi->dev_cs, !(ssi->ser.value & 1));
        }
        else {
            // when disabled,
            // * all serial transfers are halted immediately.
            // * transmit and receive FIFO buffers are cleared.
            fifo32_reset(&ssi->tx_fifo);
            fifo32_reset(&ssi->rx_fifo);
            ssi->rx_count = 0;
            ssi->tx_count = 0;
            ssi->inst_addr_sent = 0;
            qemu_set_irq(ssi->dev_cs, 1);
        }
        return;
    }

    // 0x010
    if (offset == DWC_SSIC_REG_SER) {
        ssi->ser.value = value;
        LOG("ser = {ser: 0x%x}\n", ssi->ser.ser);
        qemu_set_irq(ssi->dev_cs, !(ssi->ser.value & 1));
        return;
    }

    // 0x014
    if (offset == DWC_SSIC_REG_BAUDR) {
        if (ssi->enable) {
            printf("[TREC.SSI] Error: Write 0x%08X => BAUDR while enable!\n", (uint32_t)value);
            return;
        }

        ssi->baudr.value = value;
        LOG("baudr = {sckdv: %d}\n", ssi->baudr.sckdv);
        return;
    }

    // 0x018
    if (offset == DWC_SSIC_REG_TXFTLR) {
        DWC_SSIC_TXFTLR req;
        req.value = value;
        if (req.tft < FIFO_CAPACITY)
            ssi->txftlr.tft = req.tft;
        ssi->txftlr.txfthr = req.txfthr;
        LOG("txftlr = {tft: %d, txfthr: %d}\n", ssi->txftlr.tft, ssi->txftlr.txfthr);
    }

    // 0x1C
    if (offset == DWC_SSIC_REG_RXFTLR) {
        if (value <= FIFO_CAPACITY) {
            ssi->rxftlr.value = value;
            LOG("rxftlr = {rft: %d}\n", ssi->rxftlr.rft);
        }
        return;
    }

    // 0x02C
    if (offset == DWC_SSIC_REG_IMR) {
        ssi->imr.value = value & 0x7F;
        LOG("imr = {txeim: %d, txoim: %d, rxuim: %d, rxoim: %d, rxfim: %d, mstim: %d, xrxoim: %d}\n",
            ssi->imr.txeim, ssi->imr.txoim, ssi->imr.rxuim, ssi->imr.rxoim,
            ssi->imr.rxfim, ssi->imr.mstim, ssi->imr.xrxoim);
        return;
    }

    // 0x04C
    if (offset == DWC_SSIC_REG_DMACR) {
        ssi->dmacr.value = value;
        LOG("dmacr = {rdmae: %d, tdmae: %d}\n", ssi->dmacr.rdmae, ssi->dmacr.tdmae);
        _dwc_ssi_update_dma(ssi);
        return;
    }

    // 0x050
    if (offset == DWC_SSIC_REG_DMATDLR) {
        ssi->dmatdlr.value = value;
        LOG("dmatdlr = {dmatdl: %d}\n", ssi->dmatdlr.dmatdl);
        _dwc_ssi_update_dma(ssi);
        return;
    }

    // 0x054
    if (offset == DWC_SSIC_REG_DMARDLR) {
        ssi->dmardlr.value = value;
        LOG("dmardlr = {dmardl: %d}\n", ssi->dmardlr.dmardl);
        _dwc_ssi_update_dma(ssi);
        return;
    }

    // DRx 0x060 - 0x0EF
    if (offset >= DWC_SSIC_REG_DR00 && offset <= DWC_SSIC_REG_DR35) {
        fifo32_push(&ssi->tx_fifo, value);

        if (ssi->tx_count || fifo32_num_used(&ssi->tx_fifo) > ssi->txftlr.txfthr) {
            if (ssi->ctrlr0.spi_frf == 0) {
                _dwc_ssi_normal_xfer(ssi);
            } else {
                _dwc_ssi_enhance_xfer(ssi);
            }
        }
        return;
    }

    // 0x0F0
    if (offset == DWC_SSIC_REG_RX_SAMPLE_DELAY) {
        if (ssi->enable) {
            printf("[TREC.SSI] Error: Write 0x%08X => RX_SAMPLE_DELAY while enable!\n", (uint32_t)value);
            return;
        }
        ssi->rx_sample_delay.value = value;
        LOG("rx_sample_rate = {rsd: %d, se: %d}\n", ssi->rx_sample_delay.rsd, ssi->rx_sample_delay.se);
        return;
    }

    // 0x0F4
    if (offset == DWC_SSIC_REG_SPI_CTRLR0) {
        if (ssi->enable) {
            printf("[TREC.SSI] Error: Write 0x%08X => SPI_CTRLR0 while enable!\n", (uint32_t)value);
            return;
        }

        ssi->spi_ctrlr0.value = value;
        LOG("spi_ctrlr0 = {trans_type: %d, addr_l: %d, xip_md_bit_en: %d, inst_l: %d, wait_cycles: %d\n"
            "              spi_ddr_en: %d, inst_ddr_en: %d, spi_rxds_en: %d, xip_dfs_hc: %d, xip_inst_en: %d\n"
            "              ssic_xip_cont_xfer_en: %d, spi_dm_en: %d, spi_rxds_sig_en: %d, xip_mbl: %d, xip_prefetch_en: %d\n"
            "              clk_stretch_en: %d}\n",
            ssi->spi_ctrlr0.trans_type, ssi->spi_ctrlr0.addr_l, ssi->spi_ctrlr0.xip_md_bit_en, ssi->spi_ctrlr0.inst_l, ssi->spi_ctrlr0.wait_cycles,
            ssi->spi_ctrlr0.spi_ddr_en, ssi->spi_ctrlr0.inst_ddr_en, ssi->spi_ctrlr0.spi_rxds_en, ssi->spi_ctrlr0.xip_dfs_hc, ssi->spi_ctrlr0.xip_inst_en,
            ssi->spi_ctrlr0.ssic_xip_cont_xfer_en, ssi->spi_ctrlr0.spi_dm_en, ssi->spi_ctrlr0.spi_rxds_sig_en, ssi->spi_ctrlr0.xip_mbl, ssi->spi_ctrlr0.xip_prefetch_en,
            ssi->spi_ctrlr0.clk_stretch_en);

        return;
    }

    // 0x0F8
    if (offset == DWC_SSIC_REG_DDR_DRIVE_EDGE) {
        if (ssi->enable) {
            printf("[TREC.SSI] Error: Write 0x%08X => DDR_DRIVE_EDGE while enable!\n", (uint32_t)value);
            return;
        }

        return;
    }

    _dwc_ssi_update_irq(ssi);
}

static void _dwc_ssi_reset(DeviceState *d)
{
    DwcSsiState *ssi = DWC_SSI(d);

    ssi->enable = 0;
    fifo32_reset(&ssi->tx_fifo);
    fifo32_reset(&ssi->rx_fifo);
    ssi->rx_count = 0;
    ssi->tx_count = 0;
    ssi->inst_addr_sent = 0;

    ssi->dmacr.value = 0;
    ssi->dmatdlr.value = 0;
    ssi->dmardlr.value = 0;
}

static const MemoryRegionOps _dwc_ssi_ops = {
    .read = _dwc_ssi_read,
    .write = _dwc_ssi_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static const VMStateDescription _vmstate_dwc_ssi = {
    .name = TYPE_DWC_SSI,
    .version_id = 1,
    .fields = (VMStateField[]) {
        //VMSTATE_UINT32_ARRAY(reg, DwcRtcState, 0x28),
        VMSTATE_END_OF_LIST()
    }
};

static void _dwc_ssi_realize(DeviceState *dev, Error **errp)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
    DwcSsiState *s = DWC_SSI(dev);

    memory_region_init_io(&s->iomem, OBJECT(s), &_dwc_ssi_ops, s, "dwc-ssi", 0x1000);
    sysbus_init_mmio(sbd, &s->iomem);

    sysbus_init_irq(sbd, &s->irq);
    qdev_init_gpio_out_named(dev, &s->rx_dma, "rx-dma", 1);
    qdev_init_gpio_out_named(dev, &s->tx_dma, "tx-dma", 1);
    qdev_init_gpio_out_named(dev, &s->dev_cs, "dev-cs", 1);

    s->ssi = ssi_create_bus(dev, "ssi");

    fifo32_create(&s->tx_fifo, FIFO_CAPACITY);
    fifo32_create(&s->rx_fifo, FIFO_CAPACITY);

#if 0
    //  create a SPI flash
    DriveInfo *dinfo = drive_get_next(IF_MTD);
    if (dinfo) {
        // get flash type
        const char *flash_type = getenv("AQUILA_FLASH_TYPE");
        if (flash_type == NULL) {
            flash_type = DEFAULT_FLASH_TYPE;
        }

        s->ssi_dev = qdev_new(flash_type);

        //s->ssi_dev = ssi_create_peripheral(s->ssi, flash_type);
        qdev_prop_set_drive_err(s->ssi_dev, "drive", blk_by_legacy_dinfo(dinfo), errp);
        qdev_realize_and_unref(s->ssi_dev, BUS(s->ssi), errp);
        // qdev_init_nofail(s->ssi_dev);

        s->dev_cs = qdev_get_gpio_in_named(s->ssi_dev, SSI_GPIO_CS, 0);

        printf("[TREC.SSI] Attach a %s SPI Flash\n", flash_type);
    }
#endif

}

static void _dwc_ssi_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = _dwc_ssi_realize;
    dc->vmsd = &_vmstate_dwc_ssi;
    dc->reset = _dwc_ssi_reset;
}

static const TypeInfo _dwc_ssi_info = {
    .name          = TYPE_DWC_SSI,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(DwcSsiState),
    .class_init    = _dwc_ssi_class_init,
};

static void _dwc_ssi_register_types(void)
{
    type_register_static(&_dwc_ssi_info);
}

type_init(_dwc_ssi_register_types)
