// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy namhuyngn03@gmail.com
 */

#include <net.h>
#include <console.h>
#include <time.h>
#include <asm/io.h>
#include <asm/global_data.h>

#include "hcp_main.h"

DECLARE_GLOBAL_DATA_PTR;

/* Number of MACs (4 x 1G + 1 x 10G); not used in this file */
#define MAX_MAC_CNT 5
#define MAC_10G_APPID 5

/* Per-MAC offset from shim_phy_base_addr; not used in this file */
#define MAC_BASE_x_OFFSET		0x400
#define HCP_TIMEOUT_RETRY       1000
#define HCP_RX_TIMEOUT_MS       10
#define hcp_get_mac_cfg(hcp) \
	(((struct shim_mem_admin *)((hcp)->shim_priv))->mac_cfg)
static int hcp_mac_init(unsigned mac_idx, enum ax_eth_speed speed);
static int hcp_phy_init(unsigned mac_idx, int test_ops, struct ax_phy_info *info,
			enum ax_eth_speed speed);
static int hcp_start_link_up(unsigned mac_idx);
static int hcp_stop_link_up(unsigned mac_idx);
static int hcp_recv(unsigned mac_idx, unsigned char *packet, int length);
static int hcp_send(unsigned char *packet, int length);

struct ax_hcp_ops hcp = {
    .mac_init = hcp_mac_init,
    .phy_init = hcp_phy_init,
    .start_link_up = hcp_start_link_up,
    .stop_link_up = hcp_stop_link_up,
    .recv = hcp_recv,
    .send = hcp_send
};

/*
 * PHY config applied by the last hcp_phy_init() on each port, so that
 * hcp_stop_link_up() can undo exactly the loopback mode that was set up.
 * Indexed by mac_idx (0 = XGMII, 1..GMII_PORT_COUNT = SGMII/RMII).
 */
static struct ax_phy_info hcp_phy_state[GMII_PORT_COUNT + 1];
static bool hcp_phy_configured[GMII_PORT_COUNT + 1];
static enum ax_eth_speed hcp_eth_speed[GMII_PORT_COUNT + 1] = {
	[0 ... GMII_PORT_COUNT] = AX_ETH_1G,
};
static bool hcp_eip_enabled;
static bool hcp_eip_req[GMII_PORT_COUNT + 1];

void ax_hcp_set_eip(bool on)
{
	hcp_eip_enabled = on;
}

bool ax_hcp_get_eip(void)
{
	return hcp_eip_enabled;
}

void ax_hcp_eip_path(unsigned mac_idx, bool on)
{
	if (mac_idx <= GMII_PORT_COUNT)
		hcp_eip_req[mac_idx] = on;
}

static void hcp_set_data_path(unsigned mac_idx, bool eip)
{
	if (!eip) {
		eth_set_eipbypass(mac_idx);
		return;
	}
	mac_1g_cd_config(mac_idx);
	tdest_mask_config(mac_idx);
	eth_sgmii_reset(mac_idx);
}

static bool hcp_mac_idx_valid(unsigned mac_idx)
{
	return mac_idx >= 1 && mac_idx <= GMII_PORT_COUNT;
}

static int hcp_mac_init(unsigned mac_idx, enum ax_eth_speed speed)
{
	int ret = 0;
	bool eip;

	if (mac_idx > GMII_PORT_COUNT || !ax_speed_get(speed, mac_idx))
		return -EINVAL;
	hcp_eth_speed[mac_idx] = speed;
	eip = hcp_eip_req[mac_idx];
	hcp_eip_req[mac_idx] = false;

	switch (speed) {
	case AX_ETH_10G:
		mac_init_10g();
		xgmii_fast_sim();
		hcp_set_data_path(mac_idx, eip);
		return 0;
	case AX_ETH_100M:
		mac_init_rmii(mac_idx);
		hcp_set_data_path(mac_idx, eip);
		eth_enable_gmii_ncsi(mac_idx, 1);
		return 0;
	case AX_ETH_1G:
		break;
	default:
		return -EINVAL;
	}

	mac_reset_1g(mac_idx);
	mac_init_1g(mac_idx);
	mdelay(200);
	hcp_set_data_path(mac_idx, eip);
	/* HW specific config for MAC. */
	sgmii_innosilicon_pi_config(mac_idx);
	sgmii_fast_sim(mac_idx);
	sgmii_fast_sim_other(mac_idx);
	sgmii_innosilicon_adjust(mac_idx);
	/* Reset the internal PHY (SGMII) */
	mac_reset_sgmii(mac_idx);

	/* The mac_reset_1g() result is not checked: this always returns 0 */
	return ret;
}
static int hcp_phy_init(unsigned mac_idx, int test_ops, struct ax_phy_info *info,
			enum ax_eth_speed speed)
{
	const struct ax_phy_desc *desc = NULL;
	u8 phyaddr = INVALID_MAC_IDX, phy_type = UNKNOWNED_PHY;
	int test_result = 1;
	eth_test_options opt = { test_ops, DIS_EX_PHY_AN_BYBASS, ENA_EX_PHY_MODE };

	if (mac_idx > GMII_PORT_COUNT || !ax_speed_get(speed, mac_idx))
		return -EINVAL;
	hcp_eth_speed[mac_idx] = speed;

	if (info) {
		info->phyaddr = INVALID_MAC_IDX;
		info->phy_type = UNKNOWNED_PHY;
		info->opt = opt;
	}

	switch (speed) {
	case AX_ETH_10G:
		desc = ax_phy_probe(mac_idx, speed, &phyaddr);
		if (!desc) {
			printf("XGMII PHY not found\n");
			return -ENODEV;
		}
		if (eth_xgmii_TUNIT_sw_reset(phyaddr) ||
		    eth_xgmii_copper_sw_reset(phyaddr)) {
			printf("XGMII PHY reset failed\n");
			return -ETIMEDOUT;
		}
		xgmii_enable_phy_loopback(mac_idx, phyaddr);
		hcp_phy_state[mac_idx].phyaddr = phyaddr;
		hcp_phy_configured[mac_idx] = true;
		break;
	case AX_ETH_100M:
		eth_rmii_pinctrl_config();
		mdelay(1000);
		eth_setup_gpio_rmii();
		mdelay(50);
		desc = ax_phy_probe(mac_idx, speed, &phyaddr);
		if (!desc) {
			printf("RMII PHY not found\n");
			return -ENODEV;
		}
		mdio_rmii_enable_mii_loopback(mac_idx, phyaddr);
		mdelay(1000);
		hcp_phy_state[mac_idx].phyaddr = phyaddr;
		hcp_phy_configured[mac_idx] = true;
		break;
	case AX_ETH_1G:
		printf("Start Init: Port[%d]\n", mac_idx - 1);
		desc = ax_phy_probe(mac_idx, speed, &phyaddr);
		if (!desc) {
			printf("SGMII PHY not found\n");
			return -ENODEV;
		}
		phy_type = desc->phy_type;

		test_result = eth_init_exPhy(mac_idx, phyaddr, opt, phy_type);

		if (hcp_mac_idx_valid(mac_idx)) {
			hcp_phy_state[mac_idx].phyaddr = phyaddr;
			hcp_phy_state[mac_idx].phy_type = phy_type;
			hcp_phy_state[mac_idx].opt = opt;
			hcp_phy_configured[mac_idx] = true;
		}
		break;
	default:
		return -EINVAL;
	}

	if (info) {
		info->phyaddr = phyaddr;
		info->phy_type = phy_type;
		info->opt = opt;
	}
	return test_result;
}
/*
 * Wait for the SGMII link (sync + auto-neg complete) of an already
 * initialised port. Returns 0 on link up, -ETIMEDOUT otherwise.
 */
static int hcp_start_link_up(unsigned mac_idx)
{
	if (mac_idx > GMII_PORT_COUNT)
		return -EINVAL;

	if (hcp_eth_speed[mac_idx] != AX_ETH_1G)
		return 0;

	if (!hcp_mac_idx_valid(mac_idx))
		return -EINVAL;

	return eth_wait_internal_phy_linkup(mac_idx) ? 0 : -ETIMEDOUT;
}

/*
 * Tear down everything a test may have enabled on the port, so the next
 * test starts from a clean state: external PHY loopback (only what
 * hcp_phy_init() set up), SerDes near-end loopback, MAC TX/RX and the
 * host TX FIFO lock.
 */
static int hcp_stop_link_up(unsigned mac_idx)
{
	struct ax_phy_info *phy;
	u32 mac_base, regval;

	if (mac_idx > GMII_PORT_COUNT)
		return -EINVAL;

	phy = &hcp_phy_state[mac_idx];
	switch (hcp_eth_speed[mac_idx]) {
	case AX_ETH_10G:
		if (hcp_phy_configured[mac_idx])
			xgmii_disable_phy_loopback(mac_idx, phy->phyaddr);
		hcp_phy_configured[mac_idx] = false;
		xgmii_disable_near_loopback();
		mac_stop_10g();
		return 0;
	case AX_ETH_100M:
		if (hcp_phy_configured[mac_idx])
			mdio_rmii_disable_mii_loopback(mac_idx, phy->phyaddr);
		break;
	default:
		if (!hcp_mac_idx_valid(mac_idx))
			return -EINVAL;
		if (hcp_phy_configured[mac_idx] &&
		    phy->phyaddr != INVALID_MAC_IDX) {
			if (phy->opt.eth_run_test_ops == EXTERNAL_PHY_RJ45_LOOPBACK)
				mdio_disable_stub_lb(mac_idx, phy->phyaddr, phy->phy_type);
			else if (phy->opt.eth_run_test_ops == EXTERNAL_PHY_LOOPBACK) {
				mdio_disable_exphy_lb(mac_idx, phy->phyaddr, phy->phy_type);
				if (phy->phy_type == MARVELL_PHY)
					mdio_setup_copper_an(mac_idx, phy->phyaddr,
							     phy->phy_type);
			}
		}
		break;
	}
	hcp_phy_configured[mac_idx] = false;

	disable_internal_near_end_lb(mac_idx);

	mac_base = XGMII_MAC_BASE + (mac_idx * MAC_CONFIG_BYTE_CNT);
	regval = readl(mac_base + R_COMMAND_CONFIG);
	regval &= ~(MAC_CC_TX_ENA | MAC_CC_RX_ENA);
	writel(regval, mac_base + R_COMMAND_CONFIG);

	regval = readl(AXILS_HCP_BASE + gmii_port_ctrl_offset(mac_idx));
	regval &= ~BIT(HOST_TX_FIFO_EN);
	writel(regval, AXILS_HCP_BASE + gmii_port_ctrl_offset(mac_idx));

	if (hcp_eth_speed[mac_idx] == AX_ETH_1G)
		mac_reset_1g(mac_idx);

	return 0;
}
static int hcp_recv(unsigned mac_idx, unsigned char buf[], int blen)
{
  u8 mod = 0;
	int len = 0;
	int rlen = -1;
	volatile u32 read_data;
	int count;
	ulong start;
	u32 wb;
	u8 *bp;
	u8 port = mac_idx;
	if (mac_idx == 0) {
		port = 4;
	} else {
		port = mac_idx - 1;
	}

	/*
	 * Wait for data available in the host receive packet interface.
	 * The wait is bounded by time, not by a poll count: with the caches
	 * on, a fixed number of polls ends before the frame is back.
	 */
	start = get_timer(0);
	do {
		read_data = readl(AXILS_HCP_BASE + RX_PACKET_FIFO_0);
	} while (!(read_data & BIT(RX_FIFO_DAV)) &&
		 get_timer(start) < HCP_RX_TIMEOUT_MS);
	if (!(read_data & BIT(RX_FIFO_DAV))) {
		printf("Timeout on rx \n");
		// return sgmii_link_try(mac_idx, true);
		return rlen;
	}

	// frame length in 32-bit words and MOD (valid bytes in the last word)
	read_data = readl(AXILS_HCP_BASE+RX_PACKET_FIFO_1);

	len = read_data >> RX_FIFO_FRMLEN & 0x0000ffff;
	mod = (read_data >> RX_FIFO_MOD) & RX_FIFO_DAV;

	rlen = len * 4 - ((4 - mod) % 4);

	/* packet doesn't fit in the buffer: drop it */
	if (len * 4 > blen) {
		len = mod = 0;
		rlen = -2;
	}

	/* read packet from FIFO to buffer */
	bp = buf; /* index buffer */
	for (count = 0; count < len; count++) {
		wb = readl(AXILS_HCP_BASE+RX_PACKET_FIFO_RDATA);
		memcpy(bp, &wb, sizeof(wb));
		bp += 4;
	}

	// write 1 to clear EOP and advance to the next packet
	writel(BIT(RX_FIFO_EOP), AXILS_HCP_BASE + RX_PACKET_FIFO_1);

	/* EOP issue workaround: reset the RX FIFO, select the port again */
	writel(BIT(RX_FIFO_RST), AXILS_HCP_BASE + RX_PACKET_FIFO_0);
	writel(port << RX_FIFO_SEL, AXILS_HCP_BASE + RX_PACKET_FIFO_0);
	/* end workaround */

	/* return length in bytes */
	return (rlen); 
}
static int hcp_send(unsigned char buf[], int len)
{
  u32 mod;
	int pkt_int;
	u32 wb;
	u32 count = HCP_TIMEOUT_RETRY;

	/* reset the TX FIFO before writing a new frame */
	writel(BIT(TX_FIFO_RST), AXILS_HCP_BASE + TX_PACKET_FIFO_0);

	// wait for TX FIFO ready (bit 3), at most HCP_TIMEOUT_RETRY polls
	do {
		// mdelay(1);
	} while (!(readl(AXILS_HCP_BASE + TX_PACKET_FIFO_0) & (1 << 3)) &&
	       --count);
	if (count <= 0) {
		printf("Timeout waiting for TX FIFO ready\n");
		return -1;
	}

	/* shim-doc-page-25 */
	// set start of packet (SOP)
	writel(BIT(TX_FIFO_SOP), AXILS_HCP_BASE + TX_PACKET_FIFO_1);
	for (pkt_int = 0; pkt_int < len; pkt_int += 4) {
		memcpy(&wb, buf, sizeof(wb));
		buf += 4;
		writel(wb, AXILS_HCP_BASE + TX_PACKET_FIFO_WDATA);
	}

	// write MOD (valid bytes in the last word) and EOP
	mod = len % 4;
	writel(BIT(TX_FIFO_EOP) | mod << TX_FIFO_MOD,
	       AXILS_HCP_BASE + TX_PACKET_FIFO_1);
	return pkt_int - ((4 - mod) % 4);
}

#define HCP_ETH_LINK_RETRY	3

static u8 hcp_eth_rxbuf[2048] __aligned(4);

static int hcp_eth_mac_idx(struct net_dev *port)
{
	if (port->port >= GMII_PORT_COUNT)
		return -EINVAL;

	return port->port + 1;
}

static int hcp_eth_start(struct net_dev *port)
{
	struct ax_phy_info *cfg = port->priv_pdata;
	struct ax_phy_info info;
	int test_ops = cfg ? cfg->opt.eth_run_test_ops : EXTERNAL_PHY_COPPER;
	int mac_idx = hcp_eth_mac_idx(port);
	u8 retry = HCP_ETH_LINK_RETRY;
	int ret;

	if (mac_idx < 0)
		return mac_idx;

	ret = hcp_phy_init(mac_idx, test_ops, &info, AX_ETH_1G);
	if (test_ops == EXTERNAL_PHY_COPPER) {
		if (ret != 1 || info.phy_type == UNKNOWNED_PHY) {
			printf("External PHY copper link down: %d\n", ret);
			return -ENOLINK;
		}
		mdio_setup_realtek_phy_autoneg(mac_idx);
	}

	hcp_mac_init(mac_idx, AX_ETH_1G);
	while ((ret = hcp_start_link_up(mac_idx)) && --retry) {
		printf("Internal PHY - Link Retry! %d\n", retry);
		sgmii_link_try(mac_idx);
	}
	if (ret) {
		printf("Internal PHY - Link Down! (%d)\n", ret);
		return ret;
	}
	printf("Internal PHY - Link Up!\n");

	return 0;
}

static int hcp_eth_send(struct net_dev *port, void *packet, int length)
{
	if (hcp_eth_mac_idx(port) < 0)
		return -EINVAL;

	return hcp_send(packet, length) == length ? 0 : -EIO;
}

static int hcp_eth_recv(struct net_dev *port, int flags, uchar **packetp)
{
	int mac_idx = hcp_eth_mac_idx(port);
	int len;

	if (mac_idx < 0)
		return mac_idx;

	len = hcp_recv(mac_idx, hcp_eth_rxbuf, sizeof(hcp_eth_rxbuf));
	if (len == -2)
		return -EMSGSIZE;
	if (len < 0)
		return -EAGAIN;

	*packetp = hcp_eth_rxbuf;
	return len;
}

static int hcp_eth_free_pkt(struct net_dev *port, uchar *packet, int length)
{
	return 0;
}

static void hcp_eth_stop(struct net_dev *port)
{
	int mac_idx = hcp_eth_mac_idx(port);

	if (mac_idx > 0)
		hcp_stop_link_up(mac_idx);
}

struct ax_eth_ops hcp_eth_ops = {
	.start = hcp_eth_start,
	.send = hcp_eth_send,
	.recv = hcp_eth_recv,
	.free_pkt = hcp_eth_free_pkt,
	.stop = hcp_eth_stop,
};
