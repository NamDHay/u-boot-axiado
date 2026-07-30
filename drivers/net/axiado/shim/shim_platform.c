// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * shim_platform.c - SHIM subsystem initialization for HCP
 * (Header and Crypto Processing Engine).
 *
 * Copyright (c) 2022-2026 Axiado Corporation
 */

#include <linux/types.h>
#include <linux/compat.h>
#include <linux/bug.h>
#include <linux/iopoll.h>
#include <asm/unaligned.h>
/* #include <miiphy.h> */
#include <phy.h>
#include <net.h>
#include <asm/io.h>

#include "mac_config.h"
#include "phy_interrupt.h"
#include "shim_common.h"
#include "shim_mac.h"
#include "shim_platform.h"
#include "hcp.h"

/* Definitions and macros */

/* rxaui init for 10g uses this offset (in PHY_CSR region) */
#define RXAUI_BASE 0x40000

/* Global struct-variable for shim - must be non-static for built-in visibility */
struct shim_mem_admin shim_admin;

/**
 * mdio_reg_read - callback passed to mii_dev for mdio read (Clause 22)
 * @bus: bus that was registered
 * @phy_addr: phy-addr on the bus (0-4)
 * @regnum: phy-register where value to be read from
 *
 * Return: data read from mdio register, or negative errno
 */
static int mdio_reg_read(struct mii_dev *bus, int phy_addr, int devad, int regnum)
{
	u32 val = 0;
	int ret;

	/* Use phy_mask to check if PHY is present/enabled on the bus */
	if (phy_addr < 0 || phy_addr >= MAX_MAC_CNT ||
	    !(bus->phy_mask & (0x1 << phy_addr)))
		return -ENODEV;

    if (devad == MDIO_DEVAD_NONE) { 
        ret = mdiobus_reg_read(phy_addr, regnum, &val);
    } else {
        ret = mdiobus_reg_read_c45(phy_addr, devad, regnum, &val);
    }
    /* printf("%s: phy_addr=%d regnum=0x%x value=0x%x\n", __func__, phy_addr, */ 
    /*     regnum, val); */
	return ret ? ret : (int)val;
}

/**
 * mdio_reg_write - callback passed to mii_dev for mdio write (Clause 22)
 * @bus: bus that was registered
 * @phy_addr: phy-addr on the bus (0-4)
 * @regnum: phy-register where value to be written
 * @value: value to be written on the above register
 *
 * Return: 0 on success, or -ENODEV
 */
static int mdio_reg_write(struct mii_dev *bus, int phy_addr, int devad, int regnum,
        u16 value)
{
    if (phy_addr < 0 || phy_addr >= MAX_MAC_CNT ||
            !(bus->phy_mask & BIT(phy_addr)))
        return -ENODEV;

    if (devad == MDIO_DEVAD_NONE) {
        mdiobus_reg_write(phy_addr, regnum, value);
    } else {
        mdiobus_reg_write_c45(phy_addr, devad, regnum, value);
    }
    /* printf("%s: phy_addr=%d regnum=0x%x value=0x%x\n", __func__, phy_addr, */ 
    /*     regnum, value); */
    return 0;
}

/**
 * shim_read_shim_stats - function to read the shim stats
 * @shim_stat: (out) array to store the stat data
 * @mac_idx:  mac index
 */
void shim_read_shim_stats(u32 shim_stat[], int mac_idx)
{
    shim_stat[0] = shim_read_word(SHIM_RX_PKTS + (mac_idx * 4));
    shim_stat[1] = shim_read_word(SHIM_RX_GOOD_PKTS + (mac_idx * 4));
    shim_stat[2] = shim_read_word(SHIM_RX_BAD_PKTS + (mac_idx * 4));
}
EXPORT_SYMBOL_GPL(shim_read_shim_stats);

/**
 * shim_read_mac_rx_stats - to read the rx mac stats
 * @shim_mac_rx_stat: (out) array to store the rx mac stats data
 * @mac_idx: mac index
 */
void shim_read_mac_rx_stats(u32 shim_mac_rx_stat[], int mac_idx)
{
    u32 mac_base = MAC_BASE_OFFSET + (mac_idx * MAC_CONFIG_BYTE_CNT);

    shim_mac_rx_stat[0] = shim_read_word(mac_base + SHIM_MAC_RX_GOOD);
    shim_mac_rx_stat[1] = shim_read_word(mac_base + SHIM_MAC_RX_DROP);
    shim_mac_rx_stat[2] = shim_read_word(mac_base + SHIM_MAC_RX_UNDER_SZ_ERR);
    shim_mac_rx_stat[3] = shim_read_word(mac_base + SHIM_MAC_RX_TOTAL);
    shim_mac_rx_stat[4] = shim_read_word(mac_base + SHIM_MAC_RX_CRC_ERR);
    shim_mac_rx_stat[5] = shim_read_word(mac_base + SHIM_MAC_RX_IF_IN_ERR);
    shim_mac_rx_stat[6] = shim_read_word(mac_base + SHIM_MAC_RX_OVR_SZ_ERR);
    shim_mac_rx_stat[7] = shim_read_word(mac_base + SHIM_MAC_RX_JABBER_ERR);
    shim_mac_rx_stat[8] = shim_read_word(mac_base + SHIM_MAC_RX_FRAG_ERR);
}
EXPORT_SYMBOL_GPL(shim_read_mac_rx_stats);

/**
 * shim_read_mac_tx_stats - to read the tx mac stats
 * @shim_mac_tx_stat: (out) array to store the tx mac stats data
 * @mac_idx: mac index
 */
void shim_read_mac_tx_stats(u32 shim_mac_tx_stat[], int mac_idx)
{
    u32 mac_base = MAC_BASE_OFFSET + (mac_idx * MAC_CONFIG_BYTE_CNT);

    shim_mac_tx_stat[0] = shim_read_word(mac_base + SHIM_MAC_TX_TOTAL);
    shim_mac_tx_stat[1] = shim_read_word(mac_base + SHIM_MAC_TX_GOOD);
    shim_mac_tx_stat[2] = shim_read_word(mac_base + SHIM_MAC_TX_DROP);
    shim_mac_tx_stat[3] = shim_read_word(mac_base + SHIM_MAC_TX_CRC_ERR);
    shim_mac_tx_stat[4] = shim_read_word(mac_base + SHIM_MAC_TX_IF_OUT_ERR);
}
EXPORT_SYMBOL_GPL(shim_read_mac_tx_stats);

/**
 * shim_setup_resources - Setup memory resources from HCP device
 * @hcp: HCP device structure (already has mapped addresses)
 * @shim: Shim admin structure
 *
 * Uses memory regions already mapped by hcp_main.c probe function.
 * No longer does ioremap - just copies pointers from hcp structure.
 *
 * Return: 0 on success, negative error code on failure
 */
static int shim_setup_resources(struct hcp_eth_priv *hcp,
        struct shim_mem_admin *shim)
{
    printf("Setting up SHIM resources from HCP device\n");

    /* Use SHIM registers already mapped by HCP */
    if (!hcp->shim_base) {
        printf("SHIM base address not mapped by HCP\n");
        return -EINVAL;
    }
    shim->virt_base_addr = hcp->shim_base;
    shim->res_byte_cnt = hcp->shim_size;

    /* Use PHY CSR already mapped by HCP */
    if (!hcp->phy_csr_base) {
        printf("PHY CSR base address not mapped by HCP\n");
        return -EINVAL;
    }
    shim->phy_csr_base = hcp->phy_csr_base;

    /* Use IOCTL already mapped by HCP */
    if (!hcp->ioctl_base) {
        printf("IOCTL base address not mapped by HCP\n");
        return -EINVAL;
    }
    shim->ioctl_base = hcp->ioctl_base;

    return 0;
}

#define AX3000_CSR_BASE_ADRS_GPIO_2  0x33100000
#define AX3000_CSR_BASE_ADRS_GPIO_3  0x33180000

static void shim_config_rmii_mode(struct udevice *dev,
        struct mac_phy *mac_cfg, u8 mac_idx)
{
    struct hcp_eth_priv *hcp = dev_get_priv(dev);
	u32 mac_base = MAC_BASE_OFFSET + (mac_idx * GMII_PORT_CONTROL_OFFSET);
    u32 gpio_val = 0x0;
    writel(0x10, hcp->ioctl_base + 0xcc);

    if (mac_idx == 1) {
        gpio_val = readl(AX3000_CSR_BASE_ADRS_GPIO_2);
        gpio_val |= 0x3f800000;
        writel(gpio_val, AX3000_CSR_BASE_ADRS_GPIO_2); // RMII0
    } else if (mac_idx == 2) {
        gpio_val = readl(AX3000_CSR_BASE_ADRS_GPIO_3);
        gpio_val |= 0x07f00000;
        writel(gpio_val, AX3000_CSR_BASE_ADRS_GPIO_3); // RMII1
    }

    if(mac_cfg->use_ncsi) {
        writel(0x64, hcp->shim_base + mac_base);
    }else {
        writel(0x24, hcp->shim_base + mac_base);
    }
}

/**
 * shim_parse_mac_node - Parse a single MAC child node from Device Tree
 * @dev: Device pointer for logging
 * @child: Device tree node for this MAC
 * @mac_cfg: MAC configuration structure to populate
 * @mac_idx: MAC index (0-4)
 * @phy_mask: Bitmask of MACs with external PHYs
 *
 * Parses MAC properties:
 * - use-ncsi: bool from "use-ncsi"
 * - mdi-swap: bool from "swap-abcd"
 * - phy-mode: string property "phy-mode", handles "rxaui" and "rmii"
 * - phy-handle: presence recorded to drive MDIO phy_mask
 *
 * Return: 0 on success, negative on error
 */
static int shim_parse_mac_node(struct udevice *dev, ofnode child,
        struct mac_phy *mac_cfg, u32 mac_idx,
        u32 *phy_mask)
{
    const char *property;
    bool has_phy = false;

    /* Check if MAC is enabled in DT */
    if (!ofnode_is_enabled(child)) {
        mac_cfg->enabled = false;
        printf("MAC-%d: disabled in DT\n", mac_idx);
        return 0;
    }

    mac_cfg->enabled = true;

    mac_cfg->app_id = mac_idx ? mac_idx : MAC_10G_APPID;

    /* Parse use-ncsi (optional) */
    mac_cfg->use_ncsi = ofnode_read_bool(child, "use-ncsi");

    /* Parse mdi-swap */
    mac_cfg->mdi_swap = ofnode_read_bool(child, "swap-abcd");

    /* Parse phy-mode */
    property = ofnode_get_property(child, "phy-mode", NULL);
    mac_cfg->phy_mode = PHY_INTERFACE_MODE_SGMII; /* Default */

    if (property) {
        if (!strncmp(property, "rxaui", strlen("rxaui")))
            mac_cfg->phy_mode = PHY_INTERFACE_MODE_RXAUI;
        else if (!strncmp(property, "rmii", strlen("rmii"))) {
            mac_cfg->phy_mode = PHY_INTERFACE_MODE_RMII;
            shim_config_rmii_mode(dev, mac_cfg, mac_idx);
        }
    }

    /* New code logic for phy-handle check (sets phy_mask later, but here logs) */
    property = ofnode_get_property(child, "phy-handle", NULL);
    has_phy = !!property;
    if (has_phy && phy_mask)
        *phy_mask |= (1 << mac_idx);

    printf("MAC-%u: use_ncsi: %u, swap-abcd: %u, app_id: %u, phy_mode: %u, phy: %s\n",
            mac_idx, mac_cfg->use_ncsi, mac_cfg->mdi_swap, mac_cfg->app_id,
            mac_cfg->phy_mode, has_phy ? "yes" : "no");

    return 0;
}

/**
 * shim_parse_mac_config - Parse all MAC child nodes from Device Tree
 * @hcp: HCP device structure
 * @shim: Shim admin structure
 * @phy_mask: Bitmask of MACs with external PHYs
 *
 * Iterates through MAC child nodes and populates shim->mac_cfg array
 *
 * Return: 0 on success, negative on error
 */
static int shim_parse_mac_config(struct udevice *dev, struct hcp_eth_priv *hcp,
        struct shim_mem_admin *shim, u32 *phy_mask)
{
    ofnode child;
    u32 mac_idx;
    int ret;

    printf("Parsing MAC configuration from Device Tree\n");

    ofnode_for_each_subnode(child, dev_ofnode(dev)) {
        if (ofnode_name_eq(child, "mdio") ||
                ofnode_name_eq(child, "hfifo"))
            continue;

        /* Get MAC index from 'reg' property */
        ret = ofnode_read_u32(child, "reg", &mac_idx);
        if (ret) {
            printf(
                    "mac node missing 'reg' property, skipping\n");
            continue;
        }

        if (mac_idx >= MAX_MAC_CNT) {
            printf(
                    "Invalid MAC index %u (max %d), skipping\n",
                    mac_idx, MAX_MAC_CNT - 1);
            continue;
        }

        /* Parse this MAC's configuration */
        ret = shim_parse_mac_node(dev, child, &shim->mac_cfg[mac_idx],
                mac_idx, phy_mask);
        if (ret) {
            printf("Failed to parse MAC-%d config: %d\n",
                    mac_idx, ret);
            /* Continue parsing other MACs */
        }
    }

    return 0;
}

static enum AX_SHIM_STATUS shim_parse_phy_nodes(struct hcp_eth_priv *hcp,
        struct shim_mem_admin *shim, u32 phy_mask)
{
    struct udevice *dev = hcp->dev;
    struct hfifo_priv *hpriv;
    int i;

    hpriv = hcp->hfifo_priv;

    for (i = 0; i < MAX_MAC_CNT; i++) {
        struct phy_device *phydev;

        if (!shim->mac_cfg[i].enabled || !(phy_mask & (1 << i)))
			continue;


        phydev = phy_connect(shim->mii, i,
                dev, shim->mac_cfg[i].phy_mode);
        if (!phydev) {
            printf("MAC-%d: no PHY found on MDIO\n", i);
            return -ENOMEM;
        }

        printf("Phy-%d name: %s\n", phydev->phy_id, phydev->drv->name);

        shim->mac_cfg[i].phydev = phydev;

        if (i != hpriv->mac_idx)
            continue;

        phy_config(shim->mac_cfg[i].phydev);
        phy_startup(shim->mac_cfg[i].phydev);
        printf("Link %s %dMbps %s duplex\n",
                shim->mac_cfg[i].phydev->link ? "up" : "down",
                shim->mac_cfg[i].phydev->speed,
                shim->mac_cfg[i].phydev->duplex ? "full" : "half");
    }

    return SHIM_STATUS_SUCCESS;
}

/**
 * shim_register_mdio_bus - Allocate and register MDIO bus
 * @hcp: HCP device structure
 * @shim: Shim admin structure
 * @phy_mask: Bitmask of MACs with external PHYs
 *
 * Return: 0 on success, negative on error
 */
static int shim_register_mdio_bus(struct hcp_eth_priv *hcp,
        struct shim_mem_admin *shim, u32 phy_mask)
{
    int ret;

    /* Allocate MDIO bus */
    shim->mii = mdio_alloc();
    if (!shim->mii) {
        pr_err("Failed to allocate MDIO bus\n");
        return SHIM_STATUS_NULL_POINTER;
    }

    /* Configure MDIO bus */
    snprintf(shim->mii->name, MDIO_NAME_LEN, "ax-mii");
    shim->mii->read = mdio_reg_read;
    shim->mii->write = mdio_reg_write;
    shim->mii->phy_mask = phy_mask;

    ret = mdio_register(shim->mii);
    if (ret) {
        pr_err("Failed to register MDIO bus: %d\n", ret);
        mdio_free(shim->mii);
        shim->mii = NULL;
        ret = SHIM_STATUS_INTERNAL_ERROR;
        return ret;
    }

    printf("Registered MDIO bus: %s mask=0x%x\n",
            shim->mii->name, phy_mask);

    ret = shim_parse_phy_nodes(hcp, shim, phy_mask);
    if (ret != SHIM_STATUS_SUCCESS) {
        printf("Failed to parse PHY node: %d\n", ret);
        return ret;
    }

    return SHIM_STATUS_SUCCESS;
}

/**
 * shim_subsystem_init - Initialize SHIM subsystem
 * @hcp: HCP device structure
 *
 * Initializes the SHIM subsystem including:
 * 1. Setup memory regions (already mapped by HCP)
 * 2. Parse MAC configuration from DT
 * 3. Initialize hardware
 * 4. Register MDIO bus
 * 5. Setup External phy 
 *
 * Return: 0 on success, negative error code on failure
 */
int shim_subsystem_init(struct udevice *dev)
{
    struct hcp_eth_priv *hcp = dev_get_priv(dev);
    struct shim_mem_admin *shim = &shim_admin;
    struct hfifo_priv *hpriv;
    u32 phy_mask = 0;
    int ret;

    /* Setup memory resources from HCP (already mapped) */
    ret = shim_setup_resources(hcp, shim);
    if (ret)
        return ret;

    /* Parse MAC configuration from Device Tree */
    ret = shim_parse_mac_config(dev, hcp, shim, &phy_mask);
    if (ret) {
        printf("Failed to parse MAC configuration: %d\n", ret);
        return ret;
    }

    /* Store platform device reference */
    shim->dev = hcp->dev;
    hpriv = hcp->hfifo_priv;

    /* Mark initialization as done before hardware access.
     * Required by shim_mac_init() and other functions.
     */
    shim->init_done = true;

    /* Initialize MDIO bus and External PHY */
    printf("Initialize MDIO bus and External PHY\n");
    ret = shim_register_mdio_bus(hcp, shim, phy_mask);
    if (ret != SHIM_STATUS_SUCCESS) {
        printf("Hardware initialization failed\n");
        ret = -EIO;
        goto init_failed;
    }

    /* Initialize SHIM subsystem and MAC hardware */
    printf("Initializing SHIM subsystem\n");
    ret = shim_mac_init();
    if (ret != SHIM_STATUS_SUCCESS) {
        printf("Hardware initialization failed\n");
        ret = -EIO;
        goto init_failed;
    }

    if (!shim->mac_cfg[hpriv->mac_idx].enabled) {
        printf("HOST FIFO MAC-%d not enabled\n", hpriv->mac_idx);
        ret = -EINVAL;
        goto init_failed;
    }
    port_set_hfifo_mode(hpriv->mac_idx);

    /* Save references in HCP device */
    hcp->shim_priv = shim;

    printf("SHIM subsystem initialized successfully\n");
    return 0;

init_failed:
    /* Just reset our initialization flags.
    */
    shim->init_done = false;
    return ret;
}
EXPORT_SYMBOL_GPL(shim_subsystem_init);

/* Global Accessors */

/**
 * shim_is_init_done - get shim-platform driver initialisation status.
 *
 * Return: true if shim-driver is successfully initialised, else false.
 */
bool shim_is_init_done(void)
{
    return shim_admin.init_done;
}

/**
 * shim_get_virt_base_addr - Get virtual base address for shim memory
 *
 * Return: void __iomem * Virtual address(Base) of shim memory space.
 */
void __iomem *shim_get_virt_base_addr(void)
{
    return shim_admin.virt_base_addr;
}

/**
 * shim_get_mem_byte_cnt - get byte count of shim memory.
 *
 * Return: u32 size of shim memory in bytes
 */
u32 shim_get_mem_byte_cnt(void)
{
    return shim_admin.res_byte_cnt;
}

/**
 * is_shim_offset_valid - checks that the parameters are valid to make the access
 * of shim regs.
 * @byte_offset: shim register offset to be checked.
 *
 * Return: true if offset is valid, otherwise false.
 */
bool is_shim_offset_valid(const u32 byte_offset)
{
    if (!shim_is_init_done()) {
        pr_err("SHIM not initialized\n");
        return false;
    }
    if (byte_offset > shim_get_mem_byte_cnt()) {
        pr_err("Invalid SHIM offset: 0x%08x\n",
                byte_offset);
        return false;
    }
    return true;
}

/**
 * shim_read_word() - Read from main SHIM register space
 * @offset: Byte offset from virt_base_addr
 * Return: 32-bit value read
 */
u32 shim_read_word(const u32 offset)
{
    if (WARN_ON_ONCE(!is_shim_offset_valid(offset)))
        return 0;
    return readl(shim_admin.virt_base_addr + offset);
}

/**
 * shim_write_word() - Write to main SHIM register space
 * @offset: Byte offset from virt_base_addr
 * @value_in: 32-bit value to write
 */
void shim_write_word(const u32 offset, const u32 value_in)
{
    if (WARN_ON_ONCE(!is_shim_offset_valid(offset)))
        return;
    writel(value_in, shim_admin.virt_base_addr + offset);
}

/**
 * shim_read_phy_word() - Read from ETH_PHY CSR register space
 * @offset: Byte offset from phy_csr_base
 * Return: 32-bit value read
 */
u32 shim_read_phy_word(u32 offset)
{
    if (WARN_ON_ONCE(!shim_admin.phy_csr_base))
        return 0;
    return readl(shim_admin.phy_csr_base + offset);
}

/**
 * shim_write_phy_word() - Write to ETH_PHY CSR register space
 * @offset: Byte offset from phy_csr_base
 * @value_in: 32-bit value to write
 */
void shim_write_phy_word(u32 offset, u32 value_in)
{
    if (WARN_ON_ONCE(!shim_admin.phy_csr_base))
        return;
    writel(value_in, shim_admin.phy_csr_base + offset);
}

/**
 * rxaui_write_phy_word - Write 32 bits at the offset in RXAUI configuration.
 * @offset: 32 bit byte offset in RXAUI config space
 * @value_in: value to be written at the offset.
 */
void rxaui_write_phy_word(u32 offset, u32 value_in)
{
    shim_write_phy_word(RXAUI_BASE + offset, value_in);
}

/**
 * rxaui_read_phy_word - Read 32 bits(4 Bytes) of RXAUI memory(regs).
 * @offset: Phy register offset to be read.
 *
 * Return: u32 value at the RXAUI config reg(offset).
 */
u32 rxaui_read_phy_word(u32 offset)
{
    return shim_read_phy_word(RXAUI_BASE + offset);
}

/**
 * hfifo_packet_tx - Xmit the buffer using Host FIFO
 * @buff: data buffer to transmit
 * @len: length of buffer
 * Return: 0 on success and < 0 on failure
 */
int hfifo_packet_tx(u8 *buf, u32 len)
{
    void __iomem *base = shim_get_virt_base_addr();
    void __iomem *fifo0 = base + TX_PACKET_FIFO_0;
    void __iomem *fifo1 = base + TX_PACKET_FIFO_1;
    void __iomem *wdata = base + TX_PACKET_FIFO_WDATA;
    u32 nwords = DIV_ROUND_UP(len, 4);
    u32 val, i;
    int ret;

    /* Reset TX FIFO before each packet (HW workaround for stale EOP) */
    writel(readl(fifo0) | BIT(TX_FIFO_RST), fifo0);

    /* Wait for FIFO ready after reset */
    ret = readl_poll_timeout(fifo0, val, val & BIT(TX_FIFO_RDY), 100);
    if (ret) {
        pr_err("Timedout for TX_FIFO_RDY\n");
        return ret;
    }

    /* Mark start of packet (SOP/EOP/MOD are the only fields in FIFO_1) */
    writel(BIT(TX_FIFO_SOP), fifo1);

    /* Burst-write packet data to FIFO */
    for (i = 0; i < nwords; i++, buf += 4)
        writel(get_unaligned((u32 *)buf), wdata);

    /* Mark end of packet with mod field (valid bytes in last word) */
    writel(BIT(TX_FIFO_EOP) | ((len & 0x3) << TX_FIFO_MOD), fifo1);

    return 0;
}
EXPORT_SYMBOL_GPL(hfifo_packet_tx);

/**
 * hfifo_reset_rx - Reset the Rx FIFO Regs and MAC-port
 * @mac_idx:  mac index
 */
void hfifo_reset_rx(u8 mac_idx)
{
    void __iomem *fifo0 = shim_get_virt_base_addr() + RX_PACKET_FIFO_0;
    u32 val;
    u8 port;

    if (!mac_idx)
        port = 4;
    else
        port = mac_idx - 1;

    val = readl(fifo0);
    /* Clear stale port-select bits (6:4) before programming the new port */
    val &= ~GENMASK(6, 4);
    /* W1C: clear OVF/DAV monitor; W1TRG: trigger FIFO reset */
    val |= BIT(RX_FIFO_OVF_MON) | BIT(RX_FIFO_DAV_MON) | BIT(RX_FIFO_RST);
    val |= port << RX_FIFO_SEL;
    writel(val, fifo0);
}
EXPORT_SYMBOL_GPL(hfifo_reset_rx);

/**
 * hfifo_rx_pkt_len - Check and get the available packet length
 * @mac_idx:  mac index
 * @Return: 0 on no packet, < 0 on buff overflow, > 0 - curr packet len
 */
int hfifo_rx_pkt_len(u8 mac_idx)
{
    void __iomem *base = shim_get_virt_base_addr();
    u32 val, len, mod;
    int ret = -1;

    val = readl(base + RX_PACKET_FIFO_0);
    if (likely(val & BIT(RX_FIFO_DAV))) {
        val = readl(base + RX_PACKET_FIFO_1);
        if (val) {
            len = (val >> RX_FIFO_FRMLEN) & GENMASK(15, 0);
            mod = (val >> RX_FIFO_MOD) & GENMASK(1, 0);
            ret = len * 4 - ((4 - mod) % 4);
        }
    } else {
        if (val & (BIT(RX_FIFO_OVF) | BIT(RX_FIFO_OVF_MON) |
                    BIT(RX_FIFO_FULL)))
            ret = -1;
        else
            ret = 0;
    }

    return ret;
}
EXPORT_SYMBOL_GPL(hfifo_rx_pkt_len);

/**
 * hfifo_packet_rx - Dequeue a single packet from FIFO
 * @buf: buffer to store the dequeued data
 * @buf_len:  lenght of data buffer
 * @mac_idx:  mac index
 * @Return: buffere length dequeued
 */
int hfifo_packet_rx(u8 *buf, u32 buf_len, u8 mac_idx)
{
    void __iomem *base = shim_get_virt_base_addr();
    void __iomem *rdata = base + RX_PACKET_FIFO_RDATA;
    u32 nwords = DIV_ROUND_UP(buf_len, 4);
    u32 i;

    for (i = 0; i < nwords; i++, buf += 4)
        put_unaligned(readl(rdata), (u32 *)buf);

    /* dummy read RX_FIFO_FRMSTAT for MAC RX Frame Status */
    readl(base + RX_PACKET_FIFO_2);
    /* advance FIFO to next start-of-packet */
    writel(BIT(RX_FIFO_EOP), base + RX_PACKET_FIFO_1);

    return buf_len;
}
EXPORT_SYMBOL_GPL(hfifo_packet_rx);
