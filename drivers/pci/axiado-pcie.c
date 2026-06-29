// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2021-26 Axiado Corporation (or its affiliates).
 */

#include <clk.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <log.h>
#include <config.h>
#include <malloc.h>
#include <asm/io.h>
#include <asm/global_data.h>
#include <linux/list.h>
#include <pci.h>
#include <linux/bitops.h>
#include <hexdump.h>
#include <linux/printk.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/log2.h>
#include <linux/iopoll.h>

#include "axiado-pcie.h"

DECLARE_GLOBAL_DATA_PTR;

/* PCIE_TYPE1_TIMER_SUB_SEC_PRI_OFF */
#define PCIE_TYPE1_TIMER_SUB_SEC_PRI_OFF 0x18
#define PCIE_TYPE1_PRI_BUS_SET(dst, val) (((uint32_t)dst & ~0xFF) | ((uint32_t)val & 0xFF))
#define PCIE_TYPE1_SEC_BUS_SET(dst, val) (((uint32_t)dst & ~0xFF00) | (((uint32_t)val << 8) & 0xFF00))
#define PCIE_TYPE1_SUB_BUS_SET(dst, val) (((uint32_t)dst & ~0xFF0000) | (((uint32_t)val << 16) & 0xFF0000))

#define IS_COMMON_SLOT_CLOCK_SET 0
#define IS_LANE_REVERSAL_EN 1
#define EP_LINK_POLL_MS 1000

enum ax_dev_type {
    DEV_TYPE_SCM3000,
    DEV_TYPE_SCM3005,
};

inline u32 axiado_pcie_ioread(void __iomem *base, u32 offset)
{
	void __iomem *mmio_addr = NULL;

	mmio_addr = ((char *)(base)) + offset;
	return readl(mmio_addr);
}

inline void axiado_pcie_iowrite(void __iomem *base, u32 offset, u32 val)
{
	void __iomem *mmio_addr = NULL;

	mmio_addr = ((char *)(base)) + offset;
	writel(val, mmio_addr);
}

static unsigned int axiado_pcie_conf_offset(pci_dev_t bdf,
        unsigned int where)
{
    return (PCI_BUS(bdf)  << 20) |
           (PCI_DEV(bdf)  << 15) |
           (PCI_FUNC(bdf) << 12) |
           (where & (SZ_4K - 1) & ~3);
           /* (where & ~3); */
}

static int axiado_pcie_map_bus(struct axiado_pcie *pcie, pci_dev_t bdf,
				   int where, unsigned long long *address)
{
    unsigned int bus = PCI_BUS(bdf);
    /* printf("BDF=%d:%d.%d where=%x\n", */
    /*         PCI_BUS(bdf), */
    /*         PCI_DEV(bdf), */
    /*         PCI_FUNC(bdf), */
    /*         where); */

    if (bus == 0) {
        unsigned int slot = PCI_DEV(bdf);
        struct axiado_pcie_port *port;

        if (PCI_FUNC(bdf))
            return -ENODEV;

        list_for_each_entry(port, &pcie->ports, list) {
            if (!port->base)
                continue;

            if (port->index == slot) {
                *address = (unsigned long)port->base + (where & ~3);
                return 0;
            }
        }
        return -ENODEV;
    } else {
        unsigned int offset;

        /* move to correct offset within the 4 KiB page */
        /* *address = (unsigned long)pcie->base + (PCIE_WITH_BUS_OFFSET * bus) + */
        /*     (offset & (SZ_4K - 1)); */
        offset = axiado_pcie_conf_offset(bdf, where);
        *address = (unsigned long long)pcie->base + offset;
        return 0;
    }
}

static int axiado_pcie_config_read(const struct udevice *bus, pci_dev_t bdf,
        uint offset, ulong *valuep,
        enum pci_size_t size)
{
    struct axiado_pcie *pcie = dev_get_priv(bus);
    unsigned long long address;
    ulong value;
    int err;

    err = axiado_pcie_map_bus(pcie, bdf, offset, &address);
    printf("Config read at 0x%llx", address);
    if (err < 0) {
        value = 0xffffffff;
        goto done;
    }
    value = readl(address);

done:
    *valuep = pci_conv_32_to_size(value, offset, size);
    printf(" - 0x%lx\n", *valuep);

    return 0;
}

static int axiado_pcie_config_write(struct udevice *bus, pci_dev_t bdf,
        uint offset, ulong value,
        enum pci_size_t size)
{
    struct axiado_pcie *pcie = dev_get_priv(bus);
    unsigned long long address;
    ulong old;
    int err;

    err = axiado_pcie_map_bus(pcie, bdf, offset, &address);
    /* printf("Config write at 0x%llx - 0x%lx\n", address, value); */
    if (err < 0)
        return 0;

    old = readl(address);
    value = pci_conv_size_to_32(old, value, offset, size);

    writel(value, address);

    return 0;
}

int axiado_pcie_pll_wait(struct axiado_pcie *pcie, unsigned long timeout_ms)
{
    u32 val;
    u32 offset;
    ulong start;

    if (!pcie)
        return -EINVAL;

    offset = pcie->pcie_x1 ?
        PCIE_X1_PHY_PLL_LOCK_REG :
        PCIE_X2_PHY_PLL_LOCK_REG;

    start = get_timer(0);
    while (get_timer(start) < timeout_ms) {
        val = axiado_pcie_ioread(pcie->phy, offset);
        if (val) {
            printf("PLL Locked for port x%d\n",
                    pcie->lanes);
            return 0;
        }
    }

    pr_err("PLL is not locked for port x%d\n",
            pcie->lanes);

    return -ETIMEDOUT;
}

/*
 * If there are no PCIe cards attached, then calling this function
 * can result in the increase of the bootup time as there are big timeout
 * loops.
 */
int axiado_pcie_port_check_link(struct axiado_pcie_port *port, unsigned long timeout_ms)
{
    u32 temp, temp1;
    ulong start;

    if (!port) {
        pr_err("Invalid Arguments passed for Link status check\n");
        return -EINVAL;
    }

    /* wait for LinkUp(at Gen1) */
    start = get_timer(0);
    while (get_timer(start) < timeout_ms) {
        temp = axiado_pcie_ioread(port->pcie->csr,
                LINK_STATUS_LOW_POWER_ADDR);
        temp1 = (temp >> 8) & 0x1F;
        if (temp1 == 0x10) {
            printf("Link %u UP\n", port->index);
            return 0;
        }
    }

    pr_err("Link %u DOWN, ignoring\n", port->index);
    return -ENODEV;
}

static int axiado_pcie_get_resources(struct axiado_pcie *pcie)
{
    struct udevice *dev = pcie->dev;

    pcie->csr = (void __iomem *)dev_read_addr_name(dev, "csr");
    if (IS_ERR(pcie->csr)) {
        pr_err("Could not find csr base\n");
        return PTR_ERR(pcie->csr);
    }
    printf("CSR: base=0x%llx\n", (unsigned long long)pcie->csr);

    pcie->phy = (void __iomem *)dev_read_addr_name(dev, "phy");
    if (IS_ERR(pcie->phy)) {
        pr_err("Could not find phy base\n");
        return PTR_ERR(pcie->phy);
    }
    printf("PHY: base=0x%llx\n", (unsigned long long)pcie->phy);

    pcie->cfg = (void __iomem *)dev_read_addr_name(dev, "cfg");
    if (IS_ERR(pcie->cfg)) {
        pr_err("Could not find cfg base\n");
        return PTR_ERR(pcie->cfg);
    }
    printf("CFG: base=0x%llx\n", (unsigned long long)pcie->cfg);

    pcie->bridge = (void __iomem *)dev_read_addr_name(dev, "bridge");
    if (IS_ERR(pcie->bridge)) {
        pr_err("Could not find bridge base\n");
        return PTR_ERR(pcie->bridge);
    }
    printf("BRIDGE: base=0x%llx\n", (unsigned long long)pcie->bridge);

    pcie->mbx = (void __iomem *)dev_read_addr_name(dev, "mbx");
    if (IS_ERR(pcie->mbx)) {
        pr_err("Could not find mbx base\n");
        return PTR_ERR(pcie->mbx);
    }
    printf("MBX: base=0x%llx\n", (unsigned long long)pcie->mbx);

    if (pcie->pcie_x1) {
        pcie->ioctl = (void __iomem *)dev_read_addr_name(dev, "ioctl");
        if (IS_ERR(pcie->ioctl)) {
            pr_err("Could not find ioctl base\n");
            return PTR_ERR(pcie->ioctl);
        }
        printf("IOCTL: base=0x%llx\n", (unsigned long long)pcie->ioctl);
    }

    pcie->ext = (void __iomem *)dev_read_addr_name(dev, "ext");
    if (IS_ERR(pcie->ext)) {
        pr_err("Could not find ext base\n");
        return PTR_ERR(pcie->ext);
    }
    printf("EXT: base=0x%llx\n", (unsigned long long)pcie->ext);

    return 0;
}

static int axiado_pcie_parse_dt(struct axiado_pcie *pcie)
{
    struct udevice *dev = pcie->dev;
    ofnode np = dev_ofnode(dev), port;
    u32 lanes;
    const char *role;
    int err = -EINVAL;

    err = ofnode_read_u32(np, "num-lanes", &lanes);
    if (err < 0) {
        pr_err("failed to parse # of lanes: %d\n", err);
        goto err_node_put;
    }

    if (lanes > 2) {
        pr_err("invalid # of lanes: %d\n", lanes);
        err = -EINVAL;
        goto err_node_put;
    } else
        pcie->lanes = lanes;

    if (pcie->lanes == 1) {
        pcie->pcie_x1 = true;
        pcie->pcie_x2 = false;
    } else if (pcie->lanes == 2) {
        pcie->pcie_x1 = false;
        pcie->pcie_x2 = true;
    }

    /*
     * Endpoint mode: use pci_alloc_host_bridge() to skip
     * devm_of_pci_bridge_init() which registers ranges as
     * iomem resources — those collide with reserved-memory
     * backing the PCIe BARs.
     * Root port mode: use devm_pci_alloc_host_bridge() as normal.
     */
    role = ofnode_get_property(dev_ofnode(dev),
            "device_role", NULL);

    if (!role) {
        pr_err("missing device_role\n");
        err = -EINVAL;
        goto err_node_put;
    }

    if (!strncmp(role, "host", strlen("host"))) {
        printf("Initialising as a Host Bridge\n");
        pcie->is_root_port = true;

        ofnode_for_each_subnode(port, np) {
            struct axiado_pcie_port *rp;
            unsigned int index;
            struct fdt_pci_addr reg;
            const u32 *addr;
            int len;

            err = ofnode_read_pci_addr(port, 0, "reg", &reg, NULL);
            if (err < 0) {
                pr_err("failed to parse \"reg\" property\n");
                goto err_node_put;
            }

            index = PCI_DEV(reg.phys_hi);
            printf("RP index = %u phys_hi = 0x%x\n",
                    index, reg.phys_hi);

            if (!ofnode_is_enabled(port))
                continue;

            rp = malloc(sizeof(*rp));
            if (!rp) {
                err = -ENOMEM;
                goto err_node_put;
            }
            memset(rp, 0, sizeof(*rp));

            INIT_LIST_HEAD(&rp->list);
            rp->index = index;
            rp->pcie = pcie;
            rp->np = port;

            addr = ofnode_get_property(port, "assigned-addresses", &len);
            if (!addr) {
                pr_err("property \"assigned-addresses\" not found\n");
                err = -FDT_ERR_NOTFOUND;
                goto err_node_put;
            }

            rp->regs.start = ((u64)fdt32_to_cpu(addr[1]) << 32) | fdt32_to_cpu(addr[2]);
            rp->regs.end = rp->regs.start + 
                (((u64)fdt32_to_cpu(addr[3]) << 32) | fdt32_to_cpu(addr[4])) - 
                1;

            pcie->base = (void __iomem *)rp->regs.start;
            rp->base = (void __iomem *)rp->regs.start;
            printf("RP base = 0x%lx\n",
                    (unsigned long)pcie->base);


            list_add_tail(&rp->list, &pcie->ports);
        }
    } else {
        pr_err("Not support Endpoint mode in Device tree\n");
        goto err_node_put;
    }

    return 0;

err_node_put:
    return err;
}

void ax_set_atr_entry(struct axiado_pcie *pcie, phys_addr_t src_addr,
        phys_addr_t trsl_addr, size_t window_size, int trsl_param)
{
    u32 offset = PCIE_ATR_AXI4_SLV0;

    if (pcie->atr_table_num >= PCIE_ATR_MAX_TABLE_NUM)
        pcie->atr_table_num = PCIE_ATR_MAX_TABLE_NUM - 1;
    offset +=  PCIE_ATR_TABLE_OFFSET * pcie->atr_table_num;
    pcie->atr_table_num++;

    /* PCIE_ATR_SRC_ADDR_LOW:
     *   - bit 0: enable entry,
     *   - bits 1-6: ATR window size: total size in bytes: 2^(ATR_WSIZE + 1)
     *   - bits 7-11: reserved
     *   - bits 12-31: start of source address
     */
    axiado_pcie_iowrite(pcie->bridge, (offset+PCIE_ATR_SRC_ADDR_LOW),
            (lower_32_bits(src_addr) & PCIE_ATR_SRC_ADDR_MASK) |
            (ilog2(window_size) - 1) << PCIE_ATR_SRC_WIN_SIZE_SHIFT |
            1);
    axiado_pcie_iowrite(pcie->bridge, (offset+PCIE_ATR_SRC_ADDR_HIGH),
            (upper_32_bits(src_addr)));

    axiado_pcie_iowrite(pcie->bridge, (offset+PCIE_ATR_TRSL_ADDR_LOW),
            (lower_32_bits(trsl_addr) & PCIE_ATR_TRSL_ADDR_MASK));

    axiado_pcie_iowrite(pcie->bridge, (offset+PCIE_ATR_TRSL_ADDR_HIGH),
            (upper_32_bits(trsl_addr)));

    axiado_pcie_iowrite(pcie->bridge, (offset+PCIE_ATR_TRSL_PARAM), trsl_param);

    printf("ATR entry [table: %d]: 0x%010llx %s 0x%010llx [0x%010llx] (param: 0x%06x)\n",
            pcie->atr_table_num, src_addr, (trsl_param & PCIE_ATR_TRSL_DIR) ? "<-" : "->",
            trsl_addr, (u64)window_size, trsl_param);
}

static int axiado_pcie_setup_windows(struct axiado_pcie *pcie)
{
    struct pci_region *io, *mem, *pref;

    pci_get_regions(pcie->dev, &io, &mem, &pref);

    /* ax_set_atr_entry(pcie, */
    /*                  0x500000000, */
    /*                  0x500000000, */
    /*                  SZ_256M, */
    /*                  PCIE_ATR_TRSLID_PCIE_MEMORY); */

    /* ax_set_atr_entry(pcie, */
    /*                  0x550000000, */
    /*                  0x550000000, */
    /*                  SZ_256M, */
    /*                  PCIE_ATR_TRSLID_PCIE_MEMORY); */

    /* pcie->atr_table_num = 4; */
    /* ax_set_atr_entry(pcie, */
    /*                  0x5E0000000, */
    /*                  0x5E0000000, */
    /*                  SZ_256M, */
    /*                  0x1); */

    /* if (io) */
    /*     ax_set_atr_entry(pcie, */
    /*                      io->phys_start, */
    /*                      io->bus_start, */
    /*                      io->size, */
    /*                      PCIE_ATR_TRSLID_PCIE_MEMORY); */
    /* if (mem) */
    /*     ax_set_atr_entry(pcie, */
    /*                      mem->phys_start, */
    /*                      mem->bus_start, */
    /*                      mem->size, */
    /*                      PCIE_ATR_TRSLID_PCIE_MEMORY); */

    if (pref)
        ax_set_atr_entry(pcie,
                         pref->phys_start,
                         pref->bus_start,
                         pref->size,
                         PCIE_ATR_TRSLID_PCIE_MEMORY);

    return 0;
}

int config_eq_gen3_4(struct axiado_pcie *pcie) {
    u8 lane;
    u32 val, val1, val2;
    u32 gen3_rx_tx_preset;
    u32 gen4_rx_tx_preset;

    //  set preset value and max fine tuning attempts
    val1 = axiado_pcie_ioread(pcie->bridge, REG_PCIE_EQ_TUNING_31_0_ADRS_OFFSET);
    val2 = axiado_pcie_ioread(pcie->bridge, REG_PCIE_EQ_TUNING_63_32_ADRS_OFFSET);
    val = ((u64)val2 << 32) | ((u64)val1);

    val = PCIE_EQ_TUNING_63_0_PRESET_TUNING_SETTINGS_SET(val, BEST_PRESET_REAPPLIED_CONT);
    val = PCIE_EQ_TUNING_63_0_CONT_FINE_TUNE_EVEN_IF_NO_COEFF_SET(val, 0x1);
    val = PCIE_EQ_TUNING_63_0_GEN3_PRESET_SET(val, PCIE_GEN3_PRESET_VECTOR);
    val = PCIE_EQ_TUNING_63_0_GEN3_MAX_TUNING_ITER_SET(val, PCIE_GEN3_MAX_TUNING_ITER);

    val1 = ((u64)val) & 0xFFFFFFFF;
    val2 = ((u64)val >> 32) & 0xFFFFFFFF;
    axiado_pcie_iowrite(pcie->bridge, REG_PCIE_EQ_TUNING_31_0_ADRS_OFFSET, val1);
    axiado_pcie_iowrite(pcie->bridge, REG_PCIE_EQ_TUNING_63_32_ADRS_OFFSET, val2);

    gen3_rx_tx_preset =
        PCIE_GEN3_RX_TX_PRESET_GET(PCIE_GEN3_TX_PRESET, PCIE_GEN3_RX_PRESET);
    // 1 lane need 16-bit config
    // 2 lane need 2*8 = 32-bit config ==> 32-bit is the size of a singe register
    // access so 2 lane is set at a time
    for (lane = 0; lane < 16; lane += 2) {
        axiado_pcie_iowrite(pcie->bridge, REG_PCIE_EQ_PRESET_8G_31_0_ADRS_OFFSET + sizeof(u32) * (lane / 2),
                gen3_rx_tx_preset);
    }

    gen4_rx_tx_preset = PCIE_GEN4_RX_TX_PRESET_GET(PCIE_GEN4_TX_PRESET, PCIE_GEN4_RX_PRESET);
    // 1 lane need 8-bit config
    // 4 lane need 4*8 = 32-bit config ==> 32-bit is the size of a singe register
    // access so 4 lane is set at a time
    for (lane = 0; lane < 16; lane += 4) {
        axiado_pcie_iowrite(pcie->bridge, REG_PCIE_EQ_PRESET_16G_31_0_ADRS_OFFSET + sizeof(u32) * (lane / 4),
                gen4_rx_tx_preset);
    }

    return 0;
}

static void axiado_pcie_ep_setup_p2a_atr(struct axiado_pcie *pcie,
        u64 bar0_pci, u64 bar2_pci)
{
    u32 atr_base;

    /*
     * SBL style: SRC addr=0, just enable + window size.
     * In EP mode the PLDA IP routes BAR hits to TAB entries
     * based on BAR number, not SRC address matching.
     * We only need to set TRSL_ADDR (AXI target) and TRSL_PARAM.
     */

    /* TAB0: BAR0 1MB → 0x89300000 */
    atr_base = PCIE_ATR_PCIE_WIN0;
    axiado_pcie_iowrite(pcie->bridge, atr_base + PCIE_ATR_SRC_ADDR_LOW,
            ((ilog2(pcie->ep_bar0_size) - 1) << PCIE_ATR_SRC_WIN_SIZE_SHIFT) | 1);
    axiado_pcie_iowrite(pcie->bridge, atr_base + PCIE_ATR_SRC_ADDR_HIGH, 0);
    axiado_pcie_iowrite(pcie->bridge, atr_base + PCIE_ATR_TRSL_ADDR_LOW,
            (u32)pcie->ep_bar0_addr & PCIE_ATR_TRSL_ADDR_MASK);
    axiado_pcie_iowrite(pcie->bridge, atr_base + PCIE_ATR_TRSL_ADDR_HIGH, 0);
    axiado_pcie_iowrite(pcie->bridge, atr_base + PCIE_ATR_TRSL_PARAM,
            PCIE_ATR_TRSLID_AXI4_MASTER_0);

    /* TAB2: BAR2 64MB → 0x8C000000 */
    atr_base = PCIE_ATR_PCIE_WIN0 + 2 * PCIE_ATR_TABLE_OFFSET;
    axiado_pcie_iowrite(pcie->bridge, atr_base + PCIE_ATR_SRC_ADDR_LOW,
            ((ilog2(pcie->ep_bar1_size) - 1) << PCIE_ATR_SRC_WIN_SIZE_SHIFT) | 1);
    axiado_pcie_iowrite(pcie->bridge, atr_base + PCIE_ATR_SRC_ADDR_HIGH, 0);
    axiado_pcie_iowrite(pcie->bridge, atr_base + PCIE_ATR_TRSL_ADDR_LOW,
            (u32)pcie->ep_bar1_addr & PCIE_ATR_TRSL_ADDR_MASK);
    axiado_pcie_iowrite(pcie->bridge, atr_base + PCIE_ATR_TRSL_ADDR_HIGH, 0);
    axiado_pcie_iowrite(pcie->bridge, atr_base + PCIE_ATR_TRSL_PARAM,
            PCIE_ATR_TRSLID_AXI4_MASTER_0);

    printf("P2A ATR: TAB0 src=0 trsl=0x%llx (%dMB) TAB2 src=0 trsl=0x%llx (%dMB)\n",
            pcie->ep_bar0_addr, (int)(pcie->ep_bar0_size >> 20),
            pcie->ep_bar1_addr, (int)(pcie->ep_bar1_size >> 20));
}

static int axiado_pcie_init(struct axiado_pcie *pcie)
{
    u32 temp = 0;
    u32 speed = 0x1;
    struct axiado_pcie_port *port;
    u32 offset = 0x0;

    /*
     * TBD: Reset the PCIe controller, only in x1 case. Both x1 and x2
     * are having the same region for IOCTL. The same address cannot
     * be accessed by both x1 and x2 at the same time.
     **/
    if (pcie->pcie_x1) {
        offset = REG_PAD_CFG_REG_25_ADRS_OFFSET;
        temp = axiado_pcie_ioread(pcie->ioctl, offset);
        temp = temp | PCIE_PERST_PULLUP_SET;
        axiado_pcie_iowrite(pcie->ioctl, offset, temp);
    }
    /* 1)
     * For x1:
     *	14'h10b8 address, bit[11:0] write 12'h032
     * For x2:
     *	14'h20b8 address, bit[11:0] write 12'h032
     **/

    if (pcie->scm_version == DEV_TYPE_SCM3000) {
        if (pcie->pcie_x1)
            offset = PCIE_X1_PHY_PLL_CTRL1_REG;
        else if (pcie->pcie_x2)
            offset = PCIE_X2_PHY_PLL_CTRL1_REG;

        temp = axiado_pcie_ioread(pcie->phy, offset);
        temp = (temp & PCIE_PHY_PLL_CTRL1_MASK) | 0x32;
        axiado_pcie_iowrite(pcie->phy, offset, temp);
        temp = axiado_pcie_ioread(pcie->phy, offset);

        /* 2)
         * For x1:
         *	14'h10c4 address, bit[12:8] write 5'h03
         * For x2:
         *	14'h20c4 address, bit[12:8] write 5'h03
         **/
        if (pcie->pcie_x1)
            offset = PCIE_X1_PHY_PLL_CTRL2_REG;
        else if (pcie->pcie_x2)
            offset = PCIE_X2_PHY_PLL_CTRL2_REG;

        temp = axiado_pcie_ioread(pcie->phy, offset);
        temp = (temp & PCIE_PHY_CTRL_CONF1_MASK) | PCIE_PHY_PLL_CTRL1_SET;
        axiado_pcie_iowrite(pcie->phy, offset, temp);
        temp = axiado_pcie_ioread(pcie->phy, offset);

        /* 3) 14'h0024/14'h1024 address, bit[18:16] write 3'b100 for
         * lane0/lane1 respectively
         **/
        temp = axiado_pcie_ioread(pcie->phy, PCIE_PHY_LANE0_CTRL_REG);
        temp = (temp & PCIE_PHY_LANE_CTRL_MASK) | PCIE_PHY_PLL_CTRL2_SET;
        axiado_pcie_iowrite(pcie->phy, PCIE_PHY_LANE0_CTRL_REG, temp);
        temp = axiado_pcie_ioread(pcie->phy, PCIE_PHY_LANE0_CTRL_REG);

        if (pcie->pcie_x2) { /* For x2 only */
            temp = axiado_pcie_ioread(pcie->phy, PCIE_PHY_LANE1_CTRL_REG);
            temp = (temp & PCIE_PHY_LANE_CTRL_MASK) |
                PCIE_PHY_PLL_CTRL2_SET;
            axiado_pcie_iowrite(pcie->phy, PCIE_PHY_LANE1_CTRL_REG, temp);
            temp = axiado_pcie_ioread(pcie->phy, PCIE_PHY_LANE1_CTRL_REG);
        }

        /* 4) To adjust vga gain of PCIe x2 PHY, write 5'b11111 to bit[12:8]
         * of address 14'h0804/14'h1804 for lane0/1 respectively
         */
        temp = axiado_pcie_ioread(pcie->phy, PCIE_PHY_VGA0_GAIN_REG);
        temp = (temp & PCIE_PHY_CTRL_CONF1_MASK) | PCIE_PHY_PLL_VGA_SET;
        axiado_pcie_iowrite(pcie->phy, PCIE_PHY_VGA0_GAIN_REG, temp);
        temp = axiado_pcie_ioread(pcie->phy, PCIE_PHY_VGA0_GAIN_REG);

        if (pcie->pcie_x2) { /* For x2 only */
            temp = axiado_pcie_ioread(pcie->phy, PCIE_PHY_VGA1_GAIN_REG);
            temp = (temp & PCIE_PHY_CTRL_CONF1_MASK) | PCIE_PHY_PLL_VGA_SET;
            axiado_pcie_iowrite(pcie->phy, PCIE_PHY_VGA1_GAIN_REG, temp);
            temp = axiado_pcie_ioread(pcie->phy, PCIE_PHY_VGA1_GAIN_REG);
        }

        /*
         * For x1:
         *	13'h0804 address bit[7:4], value = 0x4 -- CTLE setting
         * For x2:
         *	13'h1804 address bit[7:4], value = 0x4 -- CTLE setting
         **/
        temp = axiado_pcie_ioread(pcie->phy, PCIE_PHY_VGA0_GAIN_REG);
        temp = (temp & PCIE_PHY_CTRL_CONF2_MASK) | PCIE_PHY_PLL_CTLE_SET;
        axiado_pcie_iowrite(pcie->phy, PCIE_PHY_VGA0_GAIN_REG, temp);
        temp = axiado_pcie_ioread(pcie->phy, PCIE_PHY_VGA0_GAIN_REG);

        if (pcie->pcie_x2) { /* For x2 only */
            temp = axiado_pcie_ioread(pcie->phy, PCIE_PHY_VGA1_GAIN_REG);
            temp = (temp & PCIE_PHY_CTRL_CONF2_MASK) |
                PCIE_PHY_PLL_CTLE_SET;
            axiado_pcie_iowrite(pcie->phy, PCIE_PHY_VGA1_GAIN_REG, temp);
            temp = axiado_pcie_ioread(pcie->phy, PCIE_PHY_VGA1_GAIN_REG);
        }
        /* POR reset */
        if (pcie->pcie_x1)
            axiado_pcie_iowrite(pcie->ext, REG_PCIE_X1_RESET_CTRL_ADRS_OFFSET,
                    PCIE_PHY_POR_RESET);
        else if (pcie->pcie_x2)
            axiado_pcie_iowrite(pcie->ext, REG_PCIE_X2_RESET_CTRL_ADRS_OFFSET,
                    PCIE_PHY_POR_RESET);
    }

    /* if (pcie->scm_version == DEV_TYPE_SCM3005) { */
    /* axiado_pcie_iowrite(pcie->ext, REG_PCIE_X1_GEN_OFF, REG_PCIE_X1_GEN_VAL); */
    /* } */

    /* Set pcie_rstn */
    if (pcie->pcie_x1)
        temp = axiado_pcie_ioread(pcie->ext, REG_PCIE_X1_RESET_CTRL_ADRS_OFFSET);
    else if (pcie->pcie_x2)
        temp = axiado_pcie_ioread(pcie->ext, REG_PCIE_X2_RESET_CTRL_ADRS_OFFSET);
    temp = REG_PCIE_RESET_CTRL_PCIE_RSTN_SET(temp, 0x1);
    temp = REG_PCIE_RESET_CTRL_HOLD_PIPE_RST_SET(temp, 0x1);
    if (pcie->pcie_x1)
        axiado_pcie_iowrite(pcie->ext, REG_PCIE_X1_RESET_CTRL_ADRS_OFFSET,
                temp);
    else if (pcie->pcie_x2)
        axiado_pcie_iowrite(pcie->ext, REG_PCIE_X2_RESET_CTRL_ADRS_OFFSET,
                temp);
    mdelay(100);

    /* Core configuration should only be modified in reset mode */
    /* Set supported speeds and RP/EP in k_gen settings */
    if (pcie->pcie_x1)
        temp = axiado_pcie_ioread(pcie->ext, REG_PCIE_X1_IP_CTRL_ADRS_OFFSET);
    else if (pcie->pcie_x2)
        temp = axiado_pcie_ioread(pcie->ext, REG_PCIE_X2_IP_CTRL_ADRS_OFFSET);

    temp = REG_PCIE_IP_CTRL_FREQ_SET(temp, AX_PCIE_TL_CLOCK_FREQ_MHZ);
    if (pcie->is_root_port) {
        /* set GEN_SETTINGS BIT0 to set RP mode */
        temp = REG_PCIE_IP_CTRL_RP_NEP_SET(temp, 1);  // 1--RP
    } else {
        temp = REG_PCIE_IP_CTRL_RP_NEP_SET(temp, 0);  // 0--EP
    }
    temp = REG_PCIE_IP_CTRL_TL_CLK_GATE_EN_SET(temp, 0);

    if (pcie->pcie_x1)
        axiado_pcie_iowrite(pcie->ext, REG_PCIE_X1_IP_CTRL_ADRS_OFFSET,
                temp);
    else if (pcie->pcie_x2)
        axiado_pcie_iowrite(pcie->ext, REG_PCIE_X2_IP_CTRL_ADRS_OFFSET,
                temp);

    if (pcie->pcie_x1)
        temp = axiado_pcie_ioread(pcie->bridge, REG_PCIE_X1_GEN_SETTINGS_ADRS_OFFSET);
    else if (pcie->pcie_x2)
        temp = axiado_pcie_ioread(pcie->bridge, REG_PCIE_X2_GEN_SETTINGS_ADRS_OFFSET);
    /* 0--EP 1--RP */
    if (pcie->is_root_port) {
        temp = PCIE_K_SET_PCIE_PORT_TYPE_SET(temp, 1);
        /* Gen supported from Gen2 -> speed */
        temp = PCIE_K_SET_LINK_SPEED_SET(temp, ((0x1 << (speed - 1)) - 1));
    } else {
        /* Don't set target gen for PCIE_EP, keep EP configured at default gen */
        temp = PCIE_K_SET_PCIE_PORT_TYPE_SET(temp, 0);
    }
    if (pcie->pcie_x2) {
        temp = PCIE_K_SET_LINK_WIDTH_X2_SET(temp, 0x1);
    }
    if (IS_LANE_REVERSAL_EN) {
        temp = PCIE_K_SET_LANE_REVERSAL_EN_SET(temp, 0x1);
    }

    if (pcie->pcie_x1)
        axiado_pcie_iowrite(pcie->bridge, REG_PCIE_X1_GEN_SETTINGS_ADRS_OFFSET,
                temp);
    else if (pcie->pcie_x2)
        axiado_pcie_iowrite(pcie->bridge, REG_PCIE_X2_GEN_SETTINGS_ADRS_OFFSET,
                temp);

    if (pcie->is_root_port) {
        /* Change class to PCIe Bridge */
        axiado_pcie_iowrite(pcie->bridge,
                REG_PCIE_PCIE_PCI_IDS_63_32_ADRS_OFFSET,
                PCIE_BRIDGE_CLASS_CODE);
    } else if (pcie->is_vga) {
        /* Change class to VGA (PCIe End-Point) */
        axiado_pcie_iowrite(pcie->bridge,
                REG_PCIE_PCIE_PCI_IDS_63_32_ADRS_OFFSET,
                PCIE_VGA_CLASS_CODE);
    } else if (pcie->is_eth) {
        /* Change class to network (PCIe End-Point) */
        axiado_pcie_iowrite(pcie->bridge,
                REG_PCIE_PCIE_PCI_IDS_63_32_ADRS_OFFSET,
                PCIE_ETHERNET_CLASS_CODE);
    }

    /* enable EQ PH2,3 */
    temp = axiado_pcie_ioread(pcie->bridge, REG_PCIE_PHYMAC_CFG_ADRS_OFFSET);

    temp = PCIE_PHYMAC_CFG_PER_EQ_PHASE_2_3_SET(temp, 0x1);
    temp = PCIE_PHYMAC_CFG_RXELECIDLE_SET(temp, 0x1);
    temp = PCIE_PHYMAC_CFG_RX_VALID_FILTER_EN_SET(temp, 0x1);
    temp = PCIE_PHYMAC_CFG_ENHANCE_EQ_SET(temp, 1);
    if (IS_COMMON_SLOT_CLOCK_SET) {
        temp = PCIE_PHYMAC_CFG_CLOCK_IS_COMMON_SET(temp, 0x1);
    }
    axiado_pcie_iowrite(pcie->bridge, REG_PCIE_PHYMAC_CFG_ADRS_OFFSET,
            temp);

    config_eq_gen3_4(pcie);

    if (pcie->scm_version == DEV_TYPE_SCM3005) {
        temp = axiado_pcie_ioread(pcie->bridge, REG_PCIE_X1_PCIE_PEX_SPC_ADRS_OFFSET);
        temp = temp | 0x3000; 
        axiado_pcie_iowrite(pcie->bridge, REG_PCIE_X1_PCIE_PEX_SPC_ADRS_OFFSET, temp);
    } else {
        temp = axiado_pcie_ioread(pcie->bridge,
                REG_PCIE_EQ_TUNING_63_32_ADRS_OFFSET);
        temp = temp &
            PCIE_PHY_EQ_TUNING_MASK; /* 0 out bits 53:48(fine tuning) */
        temp = temp & PCIE_PHY_PRESET_MASK; /* presets to test 46:36 */
        axiado_pcie_iowrite(pcie->bridge, REG_PCIE_EQ_TUNING_63_32_ADRS_OFFSET,
                temp | PCIE_PHY_EQ_TUNING_SET);
    }

    if (pcie->is_root_port) {
        /* wait for PHY PLL lock -- Inno PCIe PHY doc says
         * PLL lock status is at offset 14'h23C0? but paddr
         * for PHY model is 12:0
         */
        temp = axiado_pcie_pll_wait(pcie, PCIE_LINKUP_TIMEOUT);
        if (temp == -EAGAIN) {
            pr_err("PLL is not locked for port x%d, ignoring\n",
                    pcie->lanes);
            /*
             * If PLL is not locked yet, it is not to be considered
             * as a fatal error Please check AXBUGS-1250
             */
        } else if (temp == -EINVAL)
            return temp;
        /* The axiado_pcie structure passed for PLL
         * check is invalid.
         */
    }

    /* release PIPE_RST_N for PCIe */
    if (pcie->scm_version == DEV_TYPE_SCM3000) {
        if (pcie->pcie_x1)
            axiado_pcie_iowrite(pcie->ext, REG_PCIE_X1_RESET_CTRL_ADRS_OFFSET,
                    0x5);
        else if (pcie->pcie_x2)
            axiado_pcie_iowrite(pcie->ext, REG_PCIE_X2_RESET_CTRL_ADRS_OFFSET,
                    0x1);
    } else {
        if (pcie->pcie_x1)
            axiado_pcie_iowrite(pcie->ext, REG_PCIE_X1_RESET_CTRL_ADRS_OFFSET,
                    0x5);
        else if (pcie->pcie_x2)
            axiado_pcie_iowrite(pcie->ext, REG_PCIE_X2_RESET_CTRL_ADRS_OFFSET,
                    0x5);
    }
    usleep_range(1000, 2000);

    if (pcie->scm_version == DEV_TYPE_SCM3000) {
        /* set RP mode(in ext regs, a soft strap to the IP) */
        if (pcie->pcie_x1)
            offset = REG_PCIE_X1_IP_CTRL_ADRS_OFFSET;
        else if (pcie->pcie_x2)
            offset = REG_PCIE_X2_IP_CTRL_ADRS_OFFSET;

        temp = axiado_pcie_ioread(pcie->ext, offset);
        temp = (temp & PCIE_PHY_FREQ_MASK) |
            PCIE_PHY_FREQ_SET; /* set tl_clk freq to 250MHz from default */
        if (pcie->is_root_port)
            axiado_pcie_iowrite(pcie->ext, offset, temp | 0x1);
        else
            axiado_pcie_iowrite(pcie->ext, offset, temp);
    }

    if (pcie->is_root_port) {
        /* Bus master enable */
        usleep_range(10, 20); /* 10uS sleep for the BME. */
        temp = axiado_pcie_ioread(pcie->base,
                REG_PCIE_X1_COMMAND_STATUS_ADRS_OFFSET);
        axiado_pcie_iowrite(pcie->base,
                REG_PCIE_X1_COMMAND_STATUS_ADRS_OFFSET,
                temp | PCIE_CFG_BUS_MASTER_EN | PCIE_CFG_MEM_SPACE_EN);
        /* Setup A2P address translation table */
        axiado_pcie_setup_windows(pcie);

        /* Enumeration bus number to access Endpoint on the other side of the link */
        temp = axiado_pcie_ioread(pcie->base, axiado_pcie_conf_offset(0, PCIE_TYPE1_TIMER_SUB_SEC_PRI_OFF));
        temp = PCIE_TYPE1_PRI_BUS_SET(temp, 0x0);   // Primary bus num
        temp = PCIE_TYPE1_SEC_BUS_SET(temp, 0x1);   // Secondary bus num
        temp = PCIE_TYPE1_SUB_BUS_SET(temp, 0xFF);  // Subordinary bus num
        axiado_pcie_iowrite(pcie->base, PCIE_TYPE1_TIMER_SUB_SEC_PRI_OFF, temp);

    } else {
        /* Bus master + memory space enable in EP config space */
        usleep_range(10, 20);
        temp = axiado_pcie_ioread(pcie->cfg,
                REG_PCIE_X1_COMMAND_STATUS_ADRS_OFFSET);
        axiado_pcie_iowrite(pcie->cfg,
                REG_PCIE_X1_COMMAND_STATUS_ADRS_OFFSET,
                temp | PCIE_CFG_BUS_MASTER_EN);
        printf("EP CMD [0x%llx]: 0x%x -> 0x%x\n",
                (u64)pcie->cfg + REG_PCIE_X1_COMMAND_STATUS_ADRS_OFFSET,
                temp, temp | PCIE_CFG_BUS_MASTER_EN);

        /* P2A ATR will be configured by EP link monitor after
         * host enumerates and assigns BAR addresses.
         * Pre-program translation targets now, source addresses
         * will be set once we read host-assigned BARs.
         */
        axiado_pcie_ep_setup_p2a_atr(pcie, 0, 0);
        pcie->ep_bar0_pci = 0;
        pcie->ep_bar2_pci = 0;
        printf("P2A ATR pre-configured (src=0, will update after host enum)\n");
    }

    /* enable LTSSM — clear DISABLE_LTSSM (bit 2) in CFGCTRL */
    temp = axiado_pcie_ioread(pcie->bridge, REG_PCIE_PCIE_CFGCTRL_ADRS_OFFSET);
    printf("LTSSM enable [0x%llx]: 0x%x -> 0x%x\n",
            (u64)pcie->bridge + REG_PCIE_PCIE_CFGCTRL_ADRS_OFFSET,
            temp, temp & PCIE_PHY_CONF_CTRL_LTSSM);
    axiado_pcie_iowrite(pcie->bridge, REG_PCIE_PCIE_CFGCTRL_ADRS_OFFSET,
            temp & PCIE_PHY_CONF_CTRL_LTSSM);

    if (pcie->is_root_port) {
        if (pcie->pcie_x1)
            temp = axiado_pcie_ioread(pcie->ext, REG_PCIE_X1_RESET_CTRL_ADRS_OFFSET);
        else if (pcie->pcie_x2)
            temp = axiado_pcie_ioread(pcie->ext, REG_PCIE_X2_RESET_CTRL_ADRS_OFFSET);
        temp = REG_PCIE_RESET_CTRL_PCIE_RP_PERST_SET(temp, 0);
        if (pcie->pcie_x1)
            axiado_pcie_iowrite(pcie->ext, REG_PCIE_X1_RESET_CTRL_ADRS_OFFSET,
                    temp);
        else if (pcie->pcie_x2)
            axiado_pcie_iowrite(pcie->ext, REG_PCIE_X2_RESET_CTRL_ADRS_OFFSET,
                    temp);

        if (pcie->pcie_x1)
            temp = axiado_pcie_ioread(pcie->ext, REG_PCIE_X1_RESET_CTRL_ADRS_OFFSET);
        else if (pcie->pcie_x2)
            temp = axiado_pcie_ioread(pcie->ext, REG_PCIE_X2_RESET_CTRL_ADRS_OFFSET);
        temp = REG_PCIE_RESET_CTRL_PCIE_RP_PERST_SET(temp, 1);
        if (pcie->pcie_x1)
            axiado_pcie_iowrite(pcie->ext, REG_PCIE_X1_RESET_CTRL_ADRS_OFFSET,
                    temp);
        else if (pcie->pcie_x2)
            axiado_pcie_iowrite(pcie->ext, REG_PCIE_X2_RESET_CTRL_ADRS_OFFSET,
                    temp);

        /* Check the link status for Ports in RP mode */
        list_for_each_entry(port, &pcie->ports, list) {
            temp = axiado_pcie_port_check_link(port,
                    PCIE_LINKUP_TIMEOUT);
            if (temp == -EINVAL)
                return temp;
        }
    } else {
        printf("Not support EP mode\n");
        return -EINVAL;
    }

    return 0;
}

static int axiado_pcie_probe(struct udevice *dev)
{
    struct axiado_pcie *pcie = dev_get_priv(dev);
    int err;

    pcie->dev = dev;
    pcie->scm_version = (int)dev_get_driver_data(dev);
    INIT_LIST_HEAD(&pcie->ports);

    err = axiado_pcie_parse_dt(pcie);
    if (err < 0) {
        pr_err("failed to request resources: %d\n", err);
        goto put_resources;
    }

    err = axiado_pcie_get_resources(pcie);
    if (err < 0) {
        pr_err("failed to request resources: %d\n", err);
        goto put_resources;
    }

    /* Fix for segregating the PCIe PLDA register configuration needed
     * for PCIe EP mode as VGA and PCIe x2 Root Port Mode so that
     * reconfiguration of PCIe EP for x2 should not happen as X2 for NIC
     * is getting initialized in SBL
     */
    if ((pcie->pcie_x1) ||
            (pcie->pcie_x2 && pcie->is_root_port)) {
        err = axiado_pcie_init(pcie);
        if (err < 0) {
            pr_err("failed to initialize PCIe device: %d\n",
                    err);
            goto put_resources;
        }
    }

    return 0;

put_resources:
    return err;
}

static const struct dm_pci_ops axiado_pcie_ops = {
    .read_config = axiado_pcie_config_read,
    .write_config = axiado_pcie_config_write,
};

static const struct udevice_id axiado_pcie_of_match[] = {
    {
        .compatible = "axiado,ax3000-pcie",.data = (ulong)DEV_TYPE_SCM3000
    },
    {
        .compatible = "axiado,ax3005-pcie",.data = (ulong)DEV_TYPE_SCM3005
    },
};

U_BOOT_DRIVER(axiado_pcie) = {
    .name			= "axiado_pcie",
    .id			    = UCLASS_PCI,
    .of_match		= axiado_pcie_of_match,
    .ops			= &axiado_pcie_ops,
    .probe			= axiado_pcie_probe,
    .priv_auto	    = sizeof(struct axiado_pcie),
    .plat_auto	    = sizeof(struct pci_controller),
};
