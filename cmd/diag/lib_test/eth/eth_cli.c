// SPDX-License-Identifier: GPL-2.0+
#include <command.h>
#include <dm.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <console.h>
#include <time.h>
#include <net.h>
#include "hcp_main.h"
#include "eth_net.h"

#define ETH_CLI_SGMII_PORTS	4
#define ETH_CLI_RMII_PORTS	2
/* SGMII link retries for one dropped frame before it counts as dropped */
#define ETH_TEST_LINK_RETRY	5

/* ===================== Part 1: helper functions ===================== */

/*
 * Send one frame and read it back into rxbuff. With link_retry, a drop
 * (RX timeout or wrong length) runs sgmii_link_try() and sends the same
 * frame again, up to ETH_TEST_LINK_RETRY times. Returns the RX length of
 * the last try, or < 0 on TX failure / RX timeout.
 */
static int eth_test_xfer(u8 mac_idx, u16 pkt, unsigned char *txbuff,
			 int frame_len, unsigned char *rxbuff, bool link_retry)
{
	int rx_len, retry;

	for (retry = 0; ; retry++) {
		/* debug: uncomment to dump the SGMII state per frame */
		// printf("Auto-Neg status: 0x%x\n", readl(SGMII0_BASE + ((mac_idx - 1) * SGMII_CONFIG_RANGE) + 0x1bc4));
		// printf("SGMII_CSR = 0x%x\n", readl(SGMII_CSR_BASE + (mac_idx - 1) * SGMII_CSR_OFFSET));
		// printf("SGMII_MON = 0x%x\n", readl(SGMII_MON_BASE + (mac_idx - 1) * SGMII_MON_OFFSET) >> 16);
		if (ax_hcp_send(txbuff, frame_len) != frame_len) {
			printf("frame length mismatch\n");
			return -EIO;
		}
		rx_len = ax_hcp_recv(mac_idx, rxbuff,
				     ETH_TEST_TRAFFIC_MAX_BUFFER);
		if (rx_len == frame_len || !link_retry ||
		    retry == ETH_TEST_LINK_RETRY)
			return rx_len;

		printf("Packet %d dropped, link retry %d/%d\n", pkt, retry + 1,
		       ETH_TEST_LINK_RETRY);
		sgmii_link_try(mac_idx);
		if (ax_hcp_start_link_up(mac_idx))
			printf("Internal PHY - Link Down after retry %d\n",
			       retry + 1);
	}
}

static int eth_test_traffic_frame(u8 mac_idx, u16 pkt_cnt,
				  unsigned char *txbuff, int frame_len,
				  bool link_retry)
{
	uint16_t i = 0;
	int dropped = 0, mismatched = 0;
	int rx_len;
	unsigned char rxbuff[ETH_TEST_TRAFFIC_MAX_BUFFER] = { 0 };

	for (i = 0; i < pkt_cnt; i++) {
		rx_len = eth_test_xfer(mac_idx, i, txbuff, frame_len, rxbuff,
				       link_retry);
		if (rx_len < 0)
			break;
		if (compare_packets(mac_idx, rxbuff, rx_len, txbuff,
				    frame_len) != 1) {
			if (rx_len != frame_len)
				dropped++;
			else
				mismatched++;
		}
	}
	if (i != pkt_cnt) {
		printf("Traffic test failed. Tx/Rx Stats: Actual/Expected %d\n",
		       pkt_cnt);
		return 0;
	}
	if (dropped || mismatched) {
		printf("Traffic test failed. %d dropped, %d mismatched out of %d\n",
		       dropped, mismatched, pkt_cnt);
		return 0;
	}
	printf("Traffic test passed. Tx/Rx Stats: Actual/Expected %d\n",
	       pkt_cnt);
	return 1;
}

static int eth_test_traffic(u8 mac_idx, u16 pkt_cnt, unsigned char *dest_mac,
			    bool link_retry)
{
	int frame_len;
	unsigned char txbuff[ETH_TEST_TRAFFIC_MAX_BUFFER] = { 0 };
	unsigned char src_mac[ETH_TEST_MAC_SIZE] = { 0x02, 0xbb, 0xbb,
						     0xbb, 0xbb, 0xbb };

	frame_len = generate_ethernet_packet(txbuff, dest_mac, src_mac,
					     ETH_TEST_TRAFFIC_SIZE);
	return eth_test_traffic_frame(mac_idx, pkt_cnt, txbuff, frame_len,
				      link_retry);
}

static int generate_ethernet_ctrlframe_packet(unsigned char *tx_buffer,
					      unsigned char *dest_mac,
					      unsigned char *src_mac,
					      int frame_size)
{
	int payload_len;

	memcpy(tx_buffer, dest_mac, ETH_TEST_MAC_SIZE);
	memcpy(tx_buffer + ETH_TEST_MAC_SIZE, src_mac, ETH_TEST_MAC_SIZE);
	tx_buffer[12] = (ETHERTYPE_FLOW_CONTROL >> 8) & 0xFF;
	tx_buffer[13] = ETHERTYPE_FLOW_CONTROL & 0xFF;
	tx_buffer[14] = (CTRL_OPCODE >> 8) & 0xFF;
	tx_buffer[15] = CTRL_OPCODE & 0xFF;
	tx_buffer[16] = (CTRL_FRM_PARAMETER >> 8) & 0xFF;
	tx_buffer[17] = CTRL_FRM_PARAMETER & 0xFF;

	payload_len = frame_size - (ETH_PF_HEADER_SIZE + 4);
	memset(tx_buffer + ETH_PF_HEADER_SIZE, 0, payload_len);

	printf("Ethernet control frame generated successfully.\n");
	return payload_len + ETH_PF_HEADER_SIZE;
}

/* Same traffic loop as eth_test_traffic(), but with a MAC control frame */
static int eth_test_traffic_ctrl_frame(u8 mac_idx, u16 pkt_cnt)
{
	int frame_len;
	unsigned char txbuff[ETH_TEST_TRAFFIC_MAX_BUFFER] = { 0 };
	unsigned char src_mac[ETH_TEST_MAC_SIZE] = { 0x02, 0xbb, 0xbb,
						     0xbb, 0xbb, 0xbb };
	unsigned char dest_mac[ETH_TEST_MAC_SIZE] = { 0x01, 0x80, 0xC2,
						      0x00, 0x00, 0x01 };

	frame_len = generate_ethernet_ctrlframe_packet(txbuff, dest_mac,
						       src_mac,
						       ETH_TEST_TRAFFIC_SIZE);
	return eth_test_traffic_frame(mac_idx, pkt_cnt, txbuff, frame_len,
				      false);
}

/* Send only, nothing is read back: the MAC drop counters are checked after */
static int eth_test_traffic_drop_events(u8 mac_idx, u16 pkt_cnt,
					unsigned char *dest_mac)
{
	int status = 1;
	uint16_t i = 0;
	int frame_len;
	unsigned char txbuff[ETH_TEST_TRAFFIC_MAX_BUFFER] = { 0 };
	unsigned char src_mac[ETH_TEST_MAC_SIZE] = { 0x02, 0xbb, 0xbb,
						     0xbb, 0xbb, 0xbb };

	frame_len = generate_ethernet_packet(txbuff, dest_mac, src_mac,
					     ETH_TEST_TRAFFIC_SIZE);

	for (i = 0; i < pkt_cnt; i++) {
		status = 0;
		/* debug: uncomment to dump the SGMII state per frame */
		// printf("Auto-Neg status: 0x%x\n", readl(SGMII0_BASE + ((mac_idx - 1) * SGMII_CONFIG_RANGE) + 0x1bc4));
		// printf("SGMII_CSR = 0x%x\n", readl(SGMII_CSR_BASE + (mac_idx - 1) * SGMII_CSR_OFFSET));
		// printf("SGMII_MON = 0x%x\n", readl(SGMII_MON_BASE + (mac_idx - 1) * SGMII_MON_OFFSET) >> 16);
		ax_hcp_send(txbuff, frame_len);
	}
	return status;
}

static int eth_test_traffic_rmii(u8 mac_idx, u16 pkt_cnt,
				 unsigned char *dest_mac)
{
	int frame_len;
	unsigned char txbuff[ETH_TEST_TRAFFIC_MAX_BUFFER] = { 0 };
	unsigned char src_mac[ETH_TEST_MAC_SIZE] = { 0x00, 0x00, 0x00,
						     0x00, 0x00, 0x00 };

	frame_len = generate_ethernet_packet(txbuff, dest_mac, src_mac,
					     ETH_TEST_TRAFFIC_SIZE);
	return eth_test_traffic_frame(mac_idx, pkt_cnt, txbuff, frame_len,
				      false);
}

/* Common failure exit of every test: tear the link down, report failure */
static int eth_test_fail(u8 mac_idx)
{
	ax_hcp_stop_link_up(mac_idx);
	return CMD_RET_FAILURE;
}

/* ======================== Part 2: test cases ======================== */

static int eth_mac_internal_lb(u8 mac_idx)
{
	int ret;
	unsigned char dest_mac[ETH_TEST_MAC_SIZE] = { 0x01, 0xaa, 0xaa,
						      0xaa, 0xaa, 0xaa };

	printf("Start Init: Port: %d\n", mac_idx - 1);

	if (ax_hcp_mac_init(mac_idx) < 0) {
		printf("MAC init failed\n");
		return eth_test_fail(mac_idx);
	}
	enable_internal_near_end_lb(mac_idx);
	ret = eth_test_traffic(mac_idx, ETH_TEST_DFLT_FRAMES, dest_mac, true);
	ax_hcp_stop_link_up(mac_idx);

	if (ret) {
		printf("MAC Internal Loopback init successed: %d\n", ret);
		return CMD_RET_SUCCESS;
	}
	printf("MAC Internal Loopback init failed: %d\n", ret);
	return CMD_RET_FAILURE;
}

static int eth_mac_external_lb(u8 mac_idx, u8 phyaddr,
			       eth_test_options opt, u8 reg)
{
	int ret = 1;
	u8 retry = 3;
	unsigned char dest_mac[ETH_TEST_MAC_SIZE] = { 0x01, 0xaa, 0xaa,
						      0xaa, 0xaa, 0xaa };

	mdio_setup_realtek_phy_autoneg(mac_idx);
	if (reg == REALTEK_PHY ||
	    opt.eth_run_test_ops == EXTERNAL_PHY_RJ45_LOOPBACK) {
		ret = wait_auto_neg_comple(mac_idx, phyaddr, reg);
		if (ret != 1)
			return ret;
	}
	printf("Start Init: Port: %d\n", mac_idx - 1);

	ax_hcp_mac_init(mac_idx);
try_again:
	ret = ax_hcp_start_link_up(mac_idx);
	if (ret) {
		printf("Internal PHY - Link Down! (%d)\n", ret);
		retry--;
		if (retry != 0) {
			printf("Internal PHY - Link Retry! %d\n", retry);
			sgmii_link_try(mac_idx);
			goto try_again;
		}
		if (reg == REALTEK_PHY)
			mdio_disable_realtek_phy_autoneg(mac_idx, phyaddr, reg);
		return 0;
	}
	printf("Internal PHY - Link Up!\n");

	ret = eth_test_traffic(mac_idx, ETH_TEST_DFLT_FRAMES, dest_mac, true);
	eth_print_mac_counters(mac_idx);
	return ret;
}

static int eth_sgmii_external_phy_lb(u8 mac_idx)
{
	int ret;
	struct ax_phy_info info;

	ret = ax_hcp_phy_init(mac_idx, EXTERNAL_PHY_LOOPBACK, &info);
	if (ret < 0 || info.phy_type == UNKNOWNED_PHY) {
		printf("External PHY init failed: %d\n", ret);
		return eth_test_fail(mac_idx);
	}
	ret = eth_mac_external_lb(mac_idx, info.phyaddr, info.opt,
				  info.phy_type);
	ax_hcp_stop_link_up(mac_idx);

	if (ret) {
		printf("External Loopback init successed: %d\n", ret);
		return CMD_RET_SUCCESS;
	}
	printf("External Loopback init failed: %d\n", ret);
	return CMD_RET_FAILURE;
}

static int eth_sgmii_rj45_lb(u8 mac_idx)
{
	int ret;
	struct ax_phy_info info;

	ret = ax_hcp_phy_init(mac_idx, EXTERNAL_PHY_RJ45_LOOPBACK, &info);
	if (ret < 0 || info.phy_type == UNKNOWNED_PHY) {
		printf("External PHY init failed: %d\n", ret);
		return eth_test_fail(mac_idx);
	}
	if (ret != 1) {
		printf("RJ45 copper link down: %d\n", ret);
		return eth_test_fail(mac_idx);
	}

	ret = eth_mac_external_lb(mac_idx, info.phyaddr, info.opt,
				  info.phy_type);
	ax_hcp_stop_link_up(mac_idx);

	if (ret) {
		printf("RJ45 Loopback init successed: %d\n", ret);
		return CMD_RET_SUCCESS;
	}
	printf("RJ45 Loopback init failed: %d\n", ret);
	return CMD_RET_FAILURE;
}

static int eth_rmii_internal_lb(u8 mac_idx)
{
	int ret;
	u8 rmii_mode = 1;
	u32 mac_base = XGMII_MAC_BASE + (mac_idx * MAC_CONFIG_BYTE_CNT);
	unsigned char dest_mac[ETH_TEST_MAC_SIZE] = { 0x00, 0x00, 0x00,
						      0x00, 0x00, 0x00 };

	printf("Start Init: RMII Port: %d\n", mac_idx - 1);
	mdelay(1000);
	eth_setup_gpio_rmii();
	mdelay(50);
	ax_hcp_mac_init(mac_idx);
	writel(MAC_MII_IF_MODE_VAL, mac_base + R_IF_MODE);	/* GMII -> MII */
	eth_enable_gmii_ncsi(mac_idx, rmii_mode);
	ret = eth_test_traffic_rmii(mac_idx, ETH_TEST_DFLT_FRAMES, dest_mac);
	eth_print_mac_counters(mac_idx);
	ax_hcp_stop_link_up(mac_idx);

	if (ret) {
		printf("RMII Loopback init successed: %d\n", ret);
		return CMD_RET_SUCCESS;
	}
	printf("RMII Loopback init failed: %d\n", ret);
	return CMD_RET_FAILURE;
}

/* Resolve ip_str with ARP, then send count ICMP echo requests to it. */
static int eth_sgmii_ping(u8 mac_idx, const char *ip_str, int count)
{
	struct eth_net n;
	int ret;

	eth_net_init(&n, mac_idx);
	n.peer_ip = string_to_ip(ip_str);
	if (!n.peer_ip.s_addr)
		return CMD_RET_USAGE;

	if (!((n.peer_ip.s_addr ^ n.ip.s_addr) & n.netmask.s_addr)) {
		n.nexthop_ip = n.peer_ip;
	} else if (n.gw.s_addr) {
		n.nexthop_ip = n.gw;
	} else {
		printf("%pI4 is outside the subnet and no gateway is set\n",
		       &n.peer_ip);
		return CMD_RET_FAILURE;
	}

	printf("PING %pI4 from %pI4 (%pM), port %d\n", &n.peer_ip, &n.ip,
	       n.mac, mac_idx - 1);
	if (n.nexthop_ip.s_addr != n.peer_ip.s_addr)
		printf("via gateway %pI4\n", &n.gw);
	if (eth_net_link_up(mac_idx)) {
		ax_hcp_stop_link_up(mac_idx);
		return CMD_RET_FAILURE;
	}

	ret = eth_net_run_ping(&n, count);
	eth_print_mac_counters(mac_idx);
	ax_hcp_stop_link_up(mac_idx);

	return ret;
}

/* Answer ARP requests and ICMP echo requests until Ctrl+C. */
static int eth_sgmii_recv(u8 mac_idx)
{
	struct eth_net n;
	ulong start, ms;

	eth_net_init(&n, mac_idx);
	n.verbose = true;

	if (eth_net_link_up(mac_idx)) {
		ax_hcp_stop_link_up(mac_idx);
		return CMD_RET_FAILURE;
	}

	printf("Answering ARP/ping for %pI4 (%pM) on port %d, Ctrl+C to stop\n",
	       &n.ip, n.mac, mac_idx - 1);
	start = get_timer(0);
	while (!ctrlc())
		eth_net_service(&n);
	ms = get_timer(start);

	printf("Stopped after %lu.%03lu s: %u ARP reply(s), %u ICMP echo reply(s)\n",
	       ms / 1000, ms % 1000, n.arp_replied, n.icmp_replied);
	eth_print_mac_counters(mac_idx);
	ax_hcp_stop_link_up(mac_idx);
	return CMD_RET_SUCCESS;
}

/* Send frames back to back for seconds and print the TX speed. */
static int eth_sgmii_speed(u8 mac_idx, int seconds)
{
	struct eth_net n;
	int ret;

	eth_net_init(&n, mac_idx);

	if (eth_net_link_up(mac_idx)) {
		ax_hcp_stop_link_up(mac_idx);
		return CMD_RET_FAILURE;
	}

	ret = eth_net_run_speed(&n, seconds);
	ax_hcp_stop_link_up(mac_idx);

	return ret;
}

#if 0	/* p2p: currently broken, disabled until fixed */
/* Ping from src_mac to dst_mac of this board, needs a cable between them */
static int eth_sgmii_p2p(u8 src_mac, u8 dst_mac, int count)
{
	struct eth_net src, dst;
	int ret;

	eth_net_init(&src, src_mac);
	eth_net_init(&dst, dst_mac);
	src.other = &dst;
	dst.other = &src;
	src.peer_ip = dst.ip;
	src.nexthop_ip = dst.ip;

	printf("PING %pI4 (%pM), port %d from %pI4 (%pM), port %d\n",
	       &dst.ip, dst.mac, dst_mac - 1, &src.ip, src.mac, src_mac - 1);
	ret = eth_net_link_up(dst_mac);
	if (eth_net_link_up(src_mac) || (ret && eth_net_link_up(dst_mac))) {
		ax_hcp_stop_link_up(dst_mac);
		return eth_test_fail(src_mac);
	}

	ret = eth_net_run_ping(&src, count);
	printf("Port %d answered %u ARP request(s), %u ICMP echo request(s)\n",
	       dst_mac - 1, dst.arp_replied, dst.icmp_replied);
	eth_print_mac_counters(src_mac);
	eth_print_mac_counters(dst_mac);
	ax_hcp_stop_link_up(src_mac);
	ax_hcp_stop_link_up(dst_mac);

	return ret;
}
#endif

/* ========================= Part 3: monitors ========================= */

static int eth_mac_auto_neg_mon(u8 mac_idx)
{
	u32 test_result = 1;

	printf("======== SGMII %d - AUTO NEG MON TEST =========\n",
	       mac_idx - 1);
	auto_neg_mon_clear_bit(mac_idx);
	printf("Start Init: Port: %d\n", mac_idx - 1);
	if (ax_hcp_mac_init(mac_idx) < 0) {
		printf("MAC init failed\n");
		return eth_test_fail(mac_idx);
	}
	if (ax_hcp_start_link_up(mac_idx)) {
		printf("Internal PHY - Link Down!\n");
		return eth_test_fail(mac_idx);
	}
	test_result = eth_auto_neg_mon(mac_idx);
	ax_hcp_stop_link_up(mac_idx);
	return test_result ? CMD_RET_SUCCESS : CMD_RET_FAILURE;
}

static int eth_mac_ge_mon(u8 mac_idx)
{
	u32 test_result = 1;

	printf("======== SGMII %d - GE MON TEST =========\n", mac_idx - 1);
	ge_mon_clear_bit(mac_idx);
	printf("Start Init: Port: %d\n", mac_idx - 1);
	if (ax_hcp_mac_init(mac_idx) < 0) {
		printf("MAC init failed\n");
		return eth_test_fail(mac_idx);
	}
	if (ax_hcp_start_link_up(mac_idx)) {
		printf("Internal PHY - Link Down!\n");
		return eth_test_fail(mac_idx);
	}
	test_result = eth_ge_mon(mac_idx);
	ax_hcp_stop_link_up(mac_idx);
	return test_result ? CMD_RET_SUCCESS : CMD_RET_FAILURE;
}

static int eth_broadcast_frame(u8 mac_idx)
{
	u32 test_result = 1;
	unsigned char dest_mac[ETH_TEST_MAC_SIZE] = { 0xFF, 0xFF, 0xFF,
						      0xFF, 0xFF, 0xFF };

	printf("======== SGMII %d - BROADCAST FRAME TEST =========\n",
	       mac_idx - 1);
	printf("Start Init: Port: %d\n", mac_idx - 1);
	if (ax_hcp_mac_init(mac_idx) < 0) {
		printf("MAC init failed\n");
		return eth_test_fail(mac_idx);
	}
	enable_internal_near_end_lb(mac_idx);
	eth_test_traffic(mac_idx, ETH_TEST_DFLT_FRAMES, dest_mac, false);
	test_result = eth_print_broadcast_counters(mac_idx);
	ax_hcp_stop_link_up(mac_idx);
	return test_result ? CMD_RET_SUCCESS : CMD_RET_FAILURE;
}

static int eth_crc_fwd(u8 mac_idx)
{
	u32 test_result = 1;
	unsigned char dest_mac[ETH_TEST_MAC_SIZE] = { 0x01, 0xaa, 0xaa,
						      0xaa, 0xaa, 0xaa };

	printf("======== SGMII %d - CRC FORWARDING TEST =========\n",
	       mac_idx - 1);
	printf("Start Init: Port: %d\n", mac_idx - 1);
	if (ax_hcp_mac_init(mac_idx) < 0) {
		printf("MAC init failed\n");
		return eth_test_fail(mac_idx);
	}
	enable_internal_near_end_lb(mac_idx);
	test_result = eth_test_traffic(mac_idx, ETH_TEST_DFLT_FRAMES, dest_mac,
				       false);
	ax_hcp_stop_link_up(mac_idx);
	return test_result ? CMD_RET_SUCCESS : CMD_RET_FAILURE;
}

static int eth_ctrl_frame_enable(u8 mac_idx)
{
	int ret, status;
	u32 test_result;
	struct ax_phy_info info;
	u32 mac_base = XGMII_MAC_BASE + (mac_idx * MAC_CONFIG_BYTE_CNT);

	printf("======== SGMII %d - CONTROLLER FRAME TEST =========\n",
	       mac_idx - 1);
	printf("Start Init: Port: %d\n", mac_idx - 1);

	/* Traffic goes MAC -> SGMII -> external PHY (PHY-side loopback) -> back */
	ret = ax_hcp_phy_init(mac_idx, EXTERNAL_PHY_LOOPBACK, &info);
	if (ret < 0 || info.phy_type == UNKNOWNED_PHY) {
		printf("External PHY init failed: %d\n", ret);
		return eth_test_fail(mac_idx);
	}
	ax_hcp_mac_init(mac_idx);
	/*
	 * Forward control frames to the host instead of consuming them in MAC
	 * (mac_init_1g() already sets PAUSE_FWD, kept here to be explicit)
	 */
	writel(readl(mac_base + R_COMMAND_CONFIG) | MAC_CC_PAUSE_FWD,
	       mac_base + R_COMMAND_CONFIG);
	if (ax_hcp_start_link_up(mac_idx)) {
		printf("Internal PHY - Link Down!\n");
		if (info.phy_type == REALTEK_PHY)
			mdio_disable_realtek_phy_autoneg(mac_idx, info.phyaddr,
							 info.phy_type);
		return eth_test_fail(mac_idx);
	}
	wait_auto_neg_comple(mac_idx, info.phyaddr, info.phy_type);
	status = eth_test_traffic_ctrl_frame(mac_idx, ETH_TEST_DFLT_FRAMES);
	if (!status)
		printf("Warning: control frame traffic reported errors\n");
	test_result = eth_print_ctrl_frame_counters(mac_idx);
	ax_hcp_stop_link_up(mac_idx);
	return test_result ? CMD_RET_SUCCESS : CMD_RET_FAILURE;
}

static int eth_drop_events(u8 mac_idx)
{
	u32 test_result = 1;
	unsigned char dest_mac[ETH_TEST_MAC_SIZE] = { 0x01, 0x80, 0xC2,
						      0x00, 0x00, 0x01 };

	printf("======== SGMII %d - DROP EVENTS TEST =========\n", mac_idx - 1);
	printf("Start Init: Port: %d\n", mac_idx - 1);
	if (ax_hcp_mac_init(mac_idx) < 0) {
		printf("MAC init failed\n");
		return eth_test_fail(mac_idx);
	}
	enable_internal_near_end_lb(mac_idx);
	eth_test_traffic_drop_events(mac_idx, ETH_TEST_DFLT_FRAMES, dest_mac);
	test_result = eth_print_drop_event_counters(mac_idx);
	ax_hcp_stop_link_up(mac_idx);
	return test_result ? CMD_RET_SUCCESS : CMD_RET_FAILURE;
}

static int eth_mac_fragments_packet(u8 mac_idx)
{
	int ret, retry = 20;
	u32 test_result = 0;
	struct ax_phy_info info;
	struct ax_phy_info gen_info = { .phy_type = UNKNOWNED_PHY };
	static unsigned char rxbuff[ETH_TEST_TRAFFIC_MAX_BUFFER];

	printf("======== SGMII %d - Fragments Packets TEST =========\n",
	       mac_idx - 1);
	printf("Start Init: Port: %d\n", mac_idx - 1);

	ax_hcp_mac_init(mac_idx);
	ret = ax_hcp_phy_init(mac_idx, EXTERNAL_PHY_COPPER, &info);
	printf("DUT PHY: phyaddr %d, phy_type %d\n", info.phyaddr,
	       info.phy_type);
	if (ret < 0 || info.phy_type == UNKNOWNED_PHY) {
		printf("External PHY init failed: %d\n", ret);
		return eth_test_fail(mac_idx);
	}
	mac_cfg_jabber_packet(mac_idx);
	while (ax_hcp_start_link_up(mac_idx)) {
		printf("Internal PHY - Link Down!\n");
		if (!retry--) {
			printf("Internal PHY - Link retries exhausted\n");
			return eth_test_fail(mac_idx);
		}
		printf("Retry Init MAC - PHY: %d\n", retry);
		ax_hcp_phy_init(mac_idx, EXTERNAL_PHY_COPPER, &info);
		ax_hcp_mac_init(mac_idx);
		mac_cfg_jabber_packet(mac_idx);
	}
	printf("Internal PHY - Link Up!\n");
	mdelay(700);
	ret = ax_hcp_phy_init(4, EXTERNAL_PHY_COPPER, &gen_info);
	printf("Generator PHY: phyaddr %d, phy_type %d\n",
	       gen_info.phyaddr, gen_info.phy_type);
	if (gen_info.phy_type != MARVELL_PHY) {
		printf("Generator PHY is not 88E1512\n");
		return eth_test_fail(mac_idx);
	}
	if (ret != 1 ||
	    !wait_auto_neg_comple(mac_idx, info.phyaddr, info.phy_type)) {
		printf("Copper link not ready, packet generator not started\n");
		return eth_test_fail(mac_idx);
	}
	mdio_phy_88e1512_packet_64bytes_gen(4, gen_info.phyaddr,
					    gen_info.phy_type);
	ingress_packet_error_check(mac_idx, rxbuff, ETH_TEST_TRAFFIC_MAX_BUFFER);
	test_result = eth_print_fragments_counters(mac_idx);
	ax_hcp_stop_link_up(mac_idx);
	return test_result ? CMD_RET_SUCCESS : CMD_RET_FAILURE;
}

static int eth_mac_hpi_mon(u8 mac_idx)
{
	int phy, ret, frame_len, tx_len;
	u32 test_result = 0;
	static unsigned char txbuff[ETH_TEST_TRAFFIC_MAX_BUFFER];
	unsigned char dest_mac[ETH_TEST_MAC_SIZE] = { 0xaa, 0xaa, 0xaa,
						      0xaa, 0xaa, 0xaa };
	unsigned char src_mac[ETH_TEST_MAC_SIZE] = { 0xbb, 0xbb, 0xbb,
						     0xbb, 0xbb, 0xbb };
	struct ax_phy_info info = { 0 };

	printf("======== SGMII %d - HPI MON TEST =========\n", mac_idx - 1);
	printf("Start Init: Port: %d\n", mac_idx - 1);
	phy = ax_hcp_phy_init(mac_idx, EXTERNAL_PHY_LOOPBACK, &info);
	printf("DUT PHY: phyaddr %d, phy_type %d\n", info.phyaddr,
	       info.phy_type);
	if (phy < 0 || info.phy_type == UNKNOWNED_PHY) {
		printf("External PHY init failed: %d\n", phy);
		return eth_test_fail(mac_idx);
	}
	ret = ax_hcp_mac_init(mac_idx);
	if (ret < 0) {
		printf("MAC init failed: %d\n", ret);
		return eth_test_fail(mac_idx);
	}
	ret = ax_hcp_start_link_up(mac_idx);
	if (ret) {
		printf("SGMII link down: %d\n", ret);
		return eth_test_fail(mac_idx);
	}
	if (phy < 0 || info.phy_type == REALTEK_PHY) {
		if (!wait_auto_neg_comple(mac_idx, info.phyaddr, info.phy_type))
			return eth_test_fail(mac_idx);
	}
	if (!irq_hpi_clear_bit(mac_idx)) {
		printf("HPI MON bit clear failed, cannot proceed\n");
		return eth_test_fail(mac_idx);
	}
	memset(txbuff, 0, sizeof(txbuff));
	frame_len = generate_ethernet_packet(txbuff, dest_mac, src_mac,
					     ETH_TEST_TRAFFIC_SIZE);
	tx_len = ax_hcp_send(txbuff, frame_len);
	if (tx_len != frame_len) {
		printf("Packet send failed: %d/%d\n", tx_len, frame_len);
		return eth_test_fail(mac_idx);
	}
	test_result = eth_print_irq_hpi(mac_idx);
	ax_hcp_stop_link_up(mac_idx);
	return test_result ? CMD_RET_SUCCESS : CMD_RET_FAILURE;
}

/* ================= Part 4: commands, log and results ================= */

struct udiag_log {
	int result;
	char test_name[64];
};

/* One "ax_eth <rmii|sgmii> <cmd> <port|all>" entry; run() takes port + 1 */
struct eth_cli_test {
	const char *cmd;
	const char *name;
	int (*run)(u8 mac_idx);
};

static const struct eth_cli_test eth_rmii_tests[] = {
	{ "internal",	"Internal",		eth_rmii_internal_lb },
};

static const struct eth_cli_test eth_sgmii_tests[] = {
	{ "internal",	"Internal",		eth_mac_internal_lb },
	{ "external",	"External",		eth_sgmii_external_phy_lb },
	{ "rj45",	"RJ45",			eth_sgmii_rj45_lb },
};

static const struct eth_cli_test eth_sgmii_monitors[] = {
	{ "autoneg",	"Auto-Neg",		eth_mac_auto_neg_mon },
	{ "ge",		"GE",			eth_mac_ge_mon },
	{ "broadcast",	"BROADCAST",		eth_broadcast_frame },
	{ "crcfwd",	"CRC FORWARDING",	eth_crc_fwd },
	{ "ctrlframe",	"CONTROLLER FRAME",	eth_ctrl_frame_enable },
	{ "dropevents",	"DROP EVENTS",		eth_drop_events },
	{ "fragments",	"FRAGMENTS",		eth_mac_fragments_packet },
	{ "hpi",	"HPI MON",		eth_mac_hpi_mon },
};

static const struct eth_cli_test *eth_find_test(const struct eth_cli_test *tbl,
						int n, const char *cmd)
{
	int i;

	for (i = 0; i < n; i++)
		if (!strcmp(cmd, tbl[i].cmd))
			return &tbl[i];
	return NULL;
}

/* Decimal number in [min, max]; returns 0 or -EINVAL */
static int eth_parse_ulong(const char *str, ulong min, ulong max, ulong *val)
{
	char *endp;

	*val = simple_strtoul(str, &endp, 10);
	if (*endp != '\0' || *val < min || *val > max)
		return -EINVAL;
	return 0;
}

static int eth_print_summary(const char *title, struct udiag_log *log,
			     int total)
{
	int overall = CMD_RET_SUCCESS;
	int passed = 0;
	int i;

	printf("\n=== %s Summary ===\n", title);
	for (i = 0; i < total; i++) {
		printf("[%s] %s\n",
		       (log[i].result == CMD_RET_SUCCESS) ? "PASS" : "FAIL",
		       log[i].test_name);
		if (log[i].result == CMD_RET_SUCCESS)
			passed++;
		else
			overall = CMD_RET_FAILURE;
	}
	printf("=== Overall: %s (%d/%d passed) ===\n",
	       (overall == CMD_RET_SUCCESS) ? "PASS" : "FAIL", passed, total);

	return overall;
}

/*
 * Run t on port_str, or on ports 0..nports-1 in turn for "all", then print
 * the summary. log_fmt takes (port, t->name), title_fmt takes t->name.
 */
static int eth_run_ports(const struct eth_cli_test *t, const char *port_str,
			 int nports, const char *log_fmt,
			 const char *title_fmt)
{
	struct udiag_log log[ETH_CLI_SGMII_PORTS];
	char title[64];
	ulong port = 0;
	int total = 1;
	int i;

	if (!strcmp(port_str, "all"))
		total = nports;
	else if (eth_parse_ulong(port_str, 0, nports - 1, &port))
		return CMD_RET_USAGE;

	for (i = 0; i < total; i++, port++) {
		log[i].result = t->run(port + 1);
		snprintf(log[i].test_name, sizeof(log[i].test_name), log_fmt,
			 (int)port, t->name);
		mdelay(1000);
	}

	snprintf(title, sizeof(title), title_fmt, t->name);
	return eth_print_summary(title, log, total);
}

/*
 * ax_eth rmii internal <0|1|all>: RMII0 is MAC-1, RMII1 is MAC-2
 */
static int eth_cmd_rmii(struct cmd_tbl *cmdtp, int flag,
			int argc, char *const argv[])
{
	const struct eth_cli_test *testcase;

	testcase = eth_find_test(eth_rmii_tests, ARRAY_SIZE(eth_rmii_tests), argv[1]);
	if (!testcase)
		return CMD_RET_USAGE;

	return eth_run_ports(testcase, argv[2], ETH_CLI_RMII_PORTS,
			     "RMII_%d %s Loopback test", "ETH RMII %s Loopback");
}

/*
 * ax_eth sgmii ping <port> <ip> [count]
 */
static int eth_cmd_ping(struct cmd_tbl *cmdtp, int flag,
			int argc, char *const argv[])
{
	ulong port, count = 4;

	if (argc != 3 && argc != 4)
		return CMD_RET_USAGE;
	if (eth_parse_ulong(argv[1], 0, ETH_CLI_SGMII_PORTS - 1, &port))
		return CMD_RET_USAGE;
	if (argc == 4 && eth_parse_ulong(argv[3], 1, ULONG_MAX, &count))
		return CMD_RET_USAGE;

	return eth_sgmii_ping(port + 1, argv[2], count);
}

/*
 * ax_eth sgmii recv <port>
 */
static int eth_cmd_recv(struct cmd_tbl *cmdtp, int flag,
			int argc, char *const argv[])
{
	ulong port;

	if (argc != 2)
		return CMD_RET_USAGE;
	if (eth_parse_ulong(argv[1], 0, ETH_CLI_SGMII_PORTS - 1, &port))
		return CMD_RET_USAGE;

	return eth_sgmii_recv(port + 1);
}

/*
 * ax_eth sgmii speed <port> <seconds>: TX only
 */
static int eth_cmd_speed(struct cmd_tbl *cmdtp, int flag,
			 int argc, char *const argv[])
{
	ulong port, seconds;

	if (argc != 3)
		return CMD_RET_USAGE;
	if (eth_parse_ulong(argv[1], 0, ETH_CLI_SGMII_PORTS - 1, &port))
		return CMD_RET_USAGE;
	if (eth_parse_ulong(argv[2], 1, 3600, &seconds))
		return CMD_RET_USAGE;

	return eth_sgmii_speed(port + 1, seconds);
}

#if 0	/* p2p: currently broken, disabled until fixed */
/*
 * ax_eth sgmii p2p <src port> <dst port|all> [count]
 */
static int eth_cmd_p2p(struct cmd_tbl *cmdtp, int flag,
		       int argc, char *const argv[])
{
	struct udiag_log log[ETH_CLI_SGMII_PORTS];
	ulong src, dst, count = 4;
	int total = 0;

	if (argc != 3 && argc != 4)
		return CMD_RET_USAGE;
	if (eth_parse_ulong(argv[1], 0, ETH_CLI_SGMII_PORTS - 1, &src))
		return CMD_RET_USAGE;
	if (argc == 4 && eth_parse_ulong(argv[3], 1, ULONG_MAX, &count))
		return CMD_RET_USAGE;

	if (strcmp(argv[2], "all")) {
		if (eth_parse_ulong(argv[2], 0, ETH_CLI_SGMII_PORTS - 1, &dst) ||
		    dst == src)
			return CMD_RET_USAGE;
		return eth_sgmii_p2p(src + 1, dst + 1, count);
	}

	/* src port to every other port, one pair after the other */
	for (dst = 0; dst < ETH_CLI_SGMII_PORTS; dst++) {
		if (dst == src)
			continue;
		log[total].result = eth_sgmii_p2p(src + 1, dst + 1, count);
		snprintf(log[total].test_name, sizeof(log[total].test_name),
			 "SGMII_%lu -> SGMII_%lu ping test", src, dst);
		total++;
		mdelay(1000);
	}

	return eth_print_summary("ETH SGMII P2P", log, total);
}
#endif

/*
 * ax_eth sgmii <test|monitor> <port|all>
 * ax_eth sgmii ping|recv|speed ...
 */
static int eth_cmd_sgmii(struct cmd_tbl *cmdtp, int flag,
			 int argc, char *const argv[])
{
	const struct eth_cli_test *testcase;

	if (!strcmp(argv[1], "ping"))
		return eth_cmd_ping(cmdtp, flag, argc - 1, &argv[1]);

	if (!strcmp(argv[1], "recv"))
		return eth_cmd_recv(cmdtp, flag, argc - 1, &argv[1]);

	if (!strcmp(argv[1], "speed"))
		return eth_cmd_speed(cmdtp, flag, argc - 1, &argv[1]);

#if 0	/* p2p: currently broken, disabled until fixed */
	if (!strcmp(argv[1], "p2p"))
		return eth_cmd_p2p(cmdtp, flag, argc - 1, &argv[1]);
#endif

	testcase = eth_find_test(eth_sgmii_tests, ARRAY_SIZE(eth_sgmii_tests),
			  argv[1]);
	if (testcase)
		return eth_run_ports(testcase, argv[2], ETH_CLI_SGMII_PORTS,
				     "SGMII_%d Mac %s Loopback test",
				     "ETH SGMII %s Loopback");

	testcase = eth_find_test(eth_sgmii_monitors, ARRAY_SIZE(eth_sgmii_monitors),
			  argv[1]);
	if (testcase)
		return eth_run_ports(testcase, argv[2], ETH_CLI_SGMII_PORTS,
				     "SGMII_%d %s monitor",
				     "ETH SGMII %s Monitor");

	return CMD_RET_USAGE;
}

static int do_ax_eth(struct cmd_tbl *cmdtp, int flag,
		     int argc, char *const argv[])
{
	if (argc < 4)
		return CMD_RET_USAGE;

	if (!strcmp(argv[1], "rmii"))
		return eth_cmd_rmii(cmdtp, flag, argc - 1, &argv[1]);

	if (!strcmp(argv[1], "sgmii"))
		return eth_cmd_sgmii(cmdtp, flag, argc - 1, &argv[1]);

	return CMD_RET_USAGE;
}

U_BOOT_CMD(
	ax_eth,
	6,
	1,
	do_ax_eth,
	"Axiado ETH diagnostic command",
	"sgmii internal <port|all>\n"
	"    - MAC internal (near-end) loopback test\n"
	"Test cases:\n"
	"ax_eth sgmii external <port|all>\n"
	"    - External PHY loopback test (auto-detect Realtek/Marvell PHY)\n"
	"ax_eth sgmii rj45 <port|all>\n"
	"    - RJ45 loopback test (requires RJ45 loopback plug)\n"
	"ax_eth rmii internal <0|1|all>\n"
	"    - RMII loopback test (RMII0 = MAC-1, RMII1 = MAC-2)\n"
	"ax_eth sgmii ping <port> <ip> [count]\n"
	"    - ARP + ICMP echo to <ip> (default 4 times), no U-Boot net stack;\n"
	"      <ip> outside the subnet goes through the gateway\n"
	"ax_eth sgmii recv <port>\n"
	"    - Answer ARP/ping from the network until Ctrl+C\n"
	"ax_eth sgmii speed <port> <seconds>\n"
	"    - Send raw broadcast frames for <seconds> and print the TX\n"
	"      speed (bytes * 8 / time), needs a link partner on the port\n"
	/* p2p: currently broken, disabled until fixed
	"ax_eth sgmii p2p <src port> <dst port|all> [count]\n"
	"    - Ping from one port to another port of this board (default 4\n"
	"      times); needs a cable between the two ports\n"
	"      all: to every other port in turn, all 4 ports on one switch\n"
	*/
	"    (ping/recv: MAC/IP/gateway are per port, set in eth_net.h\n"
	"     as eth_net_port0_cfg..eth_net_port3_cfg)\n"
	"Monitors:\n"
	"ax_eth sgmii autoneg <port|all>\n"
	"    - Auto-Negotiation monitor\n"
	"ax_eth sgmii ge <port|all>\n"
	"    - GE monitor\n"
	"ax_eth sgmii broadcast <port|all>\n"
	"    - Broadcast frame monitor\n"
	"ax_eth sgmii crcfwd <port|all>\n"
	"    - CRC forwarding monitor\n"
	"ax_eth sgmii ctrlframe <port|all>\n"
	"    - Controller frame monitor\n"
	"ax_eth sgmii dropevents <port|all>\n"
	"    - Drop events monitor\n"
	"ax_eth sgmii fragments <port|all>\n"
	"    - Fragments packets monitor\n"
	"ax_eth sgmii hpi <port|all>\n"
	"    - HPI monitor\n"
);
