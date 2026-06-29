// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2021-2026 Axiado Corporation (or its affiliates). All rights reserved.
 *
 * Axiado HCP (Header and Crypto Processing Engine) Platform Driver
 *
 * This is the unified platform driver for the HCP subsystem which includes:
 * - SHIM (packet FIFO interface)
 * - 5 Ethernet MACs (4x1G + 1x10G)
 * - PHY/SerDes (SGMII and RXAUI)
 * - Host FIFO network device
 */

#include <clk.h>
#include <dm.h>
#include <log.h>
#include <config.h>
#include <console.h>
#include <malloc.h>
#include <asm/global_data.h>
#include <malloc.h>
#include <net.h>
#include <phy.h>
#include <miiphy.h>
#include <eth_phy.h>
#include <wait_bit.h>
#include <asm/io.h>
#include <linux/printk.h>
#include <linux/err.h>
#include <hexdump.h>

#include "hcp.h"

DECLARE_GLOBAL_DATA_PTR;

static void __maybe_unused enable_internal_near_end_lb(uint8_t mac_idx)
{
	uint64_t sgmii_base = SGMII_BASE +
			      (mac_idx - 1) * SGMII_CONFIG_RANGE;
	uint32_t reg_val;

	reg_val = shim_read_phy_word(sgmii_base + 0x001c);
	reg_val |= (1 << 7); //TX_NEAR_LPBK_EN
	shim_write_phy_word(sgmii_base + 0x001c, reg_val);

	reg_val = shim_read_phy_word(sgmii_base + 0x0010);
	reg_val |= (1 << 1); //RX_NEAR_LPBK_EN
	shim_write_phy_word(sgmii_base + 0x0010, reg_val);
}

static int hcp_wait_internal_phy_linkup(struct hcp_eth_priv *hcp,
        struct shim_mem_admin *shim,
        struct hfifo_priv *hpriv,
        struct phy_device *phydev)
{
    u32 regval;
    u32 count = CONFIG_PHY_ANEG_TIMEOUT;
	u32 sgmii_base = SGMII_BASE + 0x120 + ((hpriv->mac_idx - 1) * SGMII_CSR_OFFSET);

    /* Wait internal PHY linkup */
    printf("Waiting internal PHY linkup ");
    do {
        regval = shim_read_word(sgmii_base);
        printf(".");
        mdelay(1);
	} while ((!(regval & BIT(SYNC_STATUS)) ||
		  !(regval & BIT(AUTO_NEG_COMPLETE))) &&
		 --count);
    printf("\n");

	if (count <= 0) {
		pr_err("Internal PHY - Link Down!\n");
		pr_err("SGMII_CSR = 0x%x\n", regval);
        return -ENETDOWN;
	} else {
		printf("Internal PHY - Link Up!\n");
		printf("SGMII_CSR = 0x%x\n", regval);
	}

	return 0;
}

static int hcp_start(struct udevice *dev)
{
    struct hcp_eth_priv *hcp = dev_get_priv(dev);
    struct hfifo_priv *hpriv = (struct hfifo_priv *)hcp->hfifo_priv;
    struct shim_mem_admin *shim = (struct shim_mem_admin *)hcp->shim_priv;
    struct phy_device *phydev = shim->mac_cfg[hpriv->mac_idx].phydev;
    int ret = 0;

    hfifo_reset_rx(hpriv->mac_idx);
    if (hpriv->mac_idx)
        ret = hcp_wait_internal_phy_linkup(hcp, shim, hpriv, phydev);

    return ret;
}

static void hcp_stop(struct udevice *dev)
{
    struct hcp_eth_priv *hcp = dev_get_priv(dev);
    struct hfifo_priv *hpriv = (struct hfifo_priv *)hcp->hfifo_priv;

    hfifo_reset_rx(hpriv->mac_idx);
}

static uchar etherrxbuff[PKTSIZE_ALIGN]; /* Receive buffer */

int hfifo_recv_frame(struct udevice *dev, int flags, uchar **packetp)
{
    struct hcp_eth_priv *hcp = dev_get_priv(dev);
    struct hfifo_priv *hfifo = hcp->hfifo_priv;
    int length;

    /* u32 rx_stat[9]; */
    /* shim_read_mac_rx_stats(rx_stat, hfifo->mac_idx); */
    /* printf("RX_STAT: good=%u drop=%u under=%u total=%u crc=%u " */
    /*         "if_in=%u over=%u jabber=%u frag=%u\n", */
    /*         rx_stat[0], rx_stat[1], rx_stat[2], rx_stat[3], */
    /*         rx_stat[4], rx_stat[5], rx_stat[6], rx_stat[7], */
    /*         rx_stat[8]); */

    length = hfifo_rx_pkt_len(hfifo->mac_idx);

    if (length <= 0) {
        if (length < 0) {
            hfifo_reset_rx(hfifo->mac_idx);
            return -EIO;
        }

        return 0;
    }

    if (length > PKTSIZE_ALIGN)
        length = PKTSIZE_ALIGN;

    hfifo_packet_rx(etherrxbuff, length, hfifo->mac_idx);


    /* print_hex_dump("RX: ", */
    /*         DUMP_PREFIX_OFFSET, */
    /*         16, 1, */
    /*         etherrxbuff, */
    /*         min(length, 64U), */
    /*         true); */

    *packetp = etherrxbuff;

    return length;
}

int hfifo_xmit_frame(struct udevice *dev, void *packet, int length)
{
    int ret = 0;

    if (length > PKTSIZE)
        length = PKTSIZE;

    /* struct ethernet_hdr *eth = packet; */
    /* printf("HFIFO TX: len=%d\n", length); */
    /* print_hex_dump("TX: ", */
    /*         DUMP_PREFIX_OFFSET, */
    /*         16, 1, */
    /*         packet, length, true); */

    /* printf("dst %pM\n", eth->et_dest); */
    /* printf("src %pM\n", eth->et_src); */
    /* printf("type 0x%04x\n", ntohs(eth->et_protlen)); */

    ret = hfifo_packet_tx((u8 *)packet, length);
    if (ret)
        pr_err("Error when sending frame\n");


    /* u32 stat[5]; */
    /* struct hcp_eth_priv *hcp = dev_get_priv(dev); */
    /* struct hfifo_priv *hfifo = (struct hfifo_priv *)hcp->hfifo_priv; */
    /* shim_read_mac_tx_stats(stat, hfifo->mac_idx); */
    /* printf("TX_STAT: total=%u good=%u drop=%u crc=%u out_err=%u\n", */
    /*         stat[0], stat[1], stat[2], stat[3], stat[4]); */

    return ret;
}

static int hfifo_free_pkt(struct udevice *dev, uchar *packet, int length)
{
    return 0;
}

static const struct eth_ops hcp_eth_ops = {
    .start = hcp_start,
    .stop = hcp_stop,
    .free_pkt = hfifo_free_pkt,
    .recv = hfifo_recv_frame,
    .send = hfifo_xmit_frame,
    /* .write_hwaddr = hcp_eth_write_hwaddr, */
};

/**
 * hcp_hfifo_init - Parse DT for Host Packet FIFO binding
 * @dev: udevice
 *
 * Parses the "hfifo" child of the HCP DT node to identify which MAC port
 * should be exposed as a Host FIFO netdev
 *
 * Return: 0 on success, negative error code on failure
 */
static int hcp_hfifo_init(struct udevice *dev)
{
    struct hcp_eth_priv *hcp = dev_get_priv(dev);
    struct hfifo_priv *hpriv = NULL;
    ofnode child;
    u32 mac_idx = 0;
    int ret = 0;

    child = ofnode_find_subnode(dev_ofnode(dev), "hfifo");
    if (!ofnode_valid(child)) {
        ret = -EINVAL;
        goto end;
    }

    if (!ofnode_is_enabled(child)) {
        ret = -EINVAL;
        goto end;
    }

    ret = ofnode_read_u32(child, "mac", &mac_idx);
    if (ret) {
        pr_err("Port 'mac' to bind is missing for Host Packet FIFO\n");
        ret = -EINVAL;
        goto end;
    }

    if (mac_idx >= MAX_MAC_CNT) {
        pr_err("Invalid mac index %u, max %d\n",
                mac_idx, MAX_MAC_CNT - 1);
        ret = -EINVAL;
        goto end;
    }

    hpriv = calloc(1, sizeof(struct hfifo_priv));
    if (!hpriv) {
        ret = -ENOMEM;
        goto end;
    }

    hpriv->mac_idx = mac_idx;
    pr_err("Host Packet FIFO Port is set to MAC-%u\n",
            mac_idx);


end:
    hcp->hfifo_priv = hpriv;
    return ret;
}

/**
 * hcp_map_resources - Map all memory regions from Device Tree
 * @dev: udevice
 *
 * Maps SHIM and PHY CSR regions from the consolidated HCP DT node.
 *
 * Return: 0 on success, negative error code on failure
 */
static int hcp_map_resources(struct udevice *dev)
{
    struct hcp_eth_priv *hcp = dev_get_priv(dev);

    printf("Mapping HCP memory resources\n");

    /* Map SHIM registers - REQUIRED */
    hcp->shim_base = (void __iomem *)dev_read_addr_size_name(dev, "shim", &hcp->shim_size);
    if (IS_ERR(hcp->shim_base)) {
        pr_err("Missing shim base in Device Tree\n");
        return PTR_ERR(hcp->shim_base);
    }
    printf("SHIM: phys=0x%llx size=0x%llx\n",
            (unsigned long long)hcp->shim_base,
            (unsigned long long)hcp->shim_size);

    /* Map PHY CSR - REQUIRED */
    hcp->phy_csr_base = (void __iomem *)dev_read_addr_name(dev, "phy-csr");
    if (IS_ERR(hcp->phy_csr_base)) {
        pr_err("Missing phy-csr resource in Device Tree\n");
        return PTR_ERR(hcp->phy_csr_base);
    }

    printf("PHY CSR: phys=0x%llx\n", (unsigned long long)hcp->phy_csr_base);

    return 0;
}

static int hcp_eth_probe(struct udevice *dev)
{
    struct hcp_eth_priv *hcp = dev_get_priv(dev);
    int ret = 0;

    printf("Axiado HCP probing\n");

    hcp->dev = dev;

    ret = hcp_hfifo_init(dev);
    if (ret) {
        pr_err("Failed to get Packet Path: %d\n", ret);
        return ret;
    }

    ret = hcp_map_resources(dev);
    if (ret) {
        pr_err("Failed to map resources: %d\n", ret);
        return ret;
    }

    ret = shim_subsystem_init(dev);
    if (ret) {
        pr_err("Shim subsystem init failed: %d\n", ret);
        return ret;
    }

    printf("Axiado HCP initialized successfully\n");
    return ret;
}

static const struct udevice_id hcp_eth_ids[] = {
    { .compatible = "axiado,ax3000-hcp", },
    { /* sentinel */ }
};

static int hcp_eth_of_to_plat(struct udevice *dev)
{
    struct eth_pdata *pdata = dev_get_plat(dev);
    const u8 mac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    fdt_addr_t addr;

    /* Register base */
    addr = dev_read_addr(dev);
    if (addr == FDT_ADDR_T_NONE) {
        pr_err("Failed to get register base\n");
        return -EINVAL;
    }

    pdata->iobase = addr;

    if (is_valid_ethaddr(mac)) {
        memcpy(pdata->enetaddr, mac, ETH_ALEN);
    }

    return 0;
}

U_BOOT_DRIVER(hcp_eth) = {
    .name = "hcp_eth",
    .id = UCLASS_ETH,
    .of_match = hcp_eth_ids,
    .ops = &hcp_eth_ops,
    .priv_auto	= sizeof(struct hcp_eth_priv),
    .plat_auto	= sizeof(struct eth_pdata),
    .probe = hcp_eth_probe,
    .of_to_plat = hcp_eth_of_to_plat,
};
