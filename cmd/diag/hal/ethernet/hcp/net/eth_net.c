// SPDX-License-Identifier: GPL-2.0+
/*
 * Minimal ARP / IPv4 / ICMP echo on top of the diag HCP HAL
 * (ax_hcp_send / ax_hcp_recv). It does not use U-Boot's net/ stack or the
 * DM ethernet driver: frames are built and parsed here and pushed through
 * the host packet FIFO directly.
 *
 * Supported: ARP request/reply, IPv4 without options on receive, no
 * fragments, ICMP echo request/reply. A peer outside the subnet is reached
 * through the gateway of the port (see eth_net_portN_cfg in eth_net.h).
 */
#include <command.h>
#include <console.h>
#include <net.h>
#include <time.h>
#include <linux/delay.h>
#include <linux/math64.h>
#include <linux/string.h>
#include "hcp_main.h"
#include "eth_net.h"

#define ETH_NET_ARP_TIMEOUT_MS	1000
#define ETH_NET_ARP_RETRIES	5
#define ETH_NET_PING_TIMEOUT_MS	1000
#define ETH_NET_PING_PERIOD_MS	1000
#define ETH_NET_PING_DATA_LEN	56
#define ETH_NET_ICMP_ID		0xa3c5
#define ETH_NET_MIN_FRAME	60	/* without FCS */
#define ETH_NET_BUF_SIZE	2048
#define ETH_NET_SPEED_PAYLOAD	1500
#define ETH_NET_SPEED_CTRLC_PERIOD	256
#define ETH_NET_SPEED_REPORT_MS	1000

static u8 eth_net_txbuf[ETH_NET_BUF_SIZE] __aligned(4);
static u8 eth_net_rxbuf[ETH_NET_BUF_SIZE] __aligned(4);
/* speed test frame, built once and resent */
static u8 eth_net_speedbuf[ETH_NET_BUF_SIZE] __aligned(4);

static const u8 eth_net_bcast[ARP_HLEN] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

/* indexed by diag port (mac_idx - 1); eth_cli.c only passes ports 0..3 */
static const struct eth_net_port_cfg *const eth_net_port_cfgs[] = {
	&eth_net_port0_cfg,
	&eth_net_port1_cfg,
	&eth_net_port2_cfg,
	&eth_net_port3_cfg,
};

void eth_net_init(struct eth_net *n, u8 mac_idx)
{
	const struct eth_net_port_cfg *cfg = eth_net_port_cfgs[mac_idx - 1];

	memset(n, 0, sizeof(*n));
	n->mac_idx = mac_idx;

	/* per-port identity from eth_net.h; the env is not used */
	memcpy(n->mac, cfg->mac, ARP_HLEN);
	n->ip = string_to_ip(cfg->ip);
	n->netmask = string_to_ip(cfg->netmask);
	n->gw = string_to_ip(cfg->gw);
}

/* Link bring-up: copper AN on the external PHY, then SGMII */
int eth_net_link_up(u8 mac_idx)
{
	struct ax_phy_info info;
	u8 retry = 3;
	int ret;

	ret = ax_hcp_phy_init(mac_idx, EXTERNAL_PHY_COPPER, &info);
	if (ret != 1 || info.phy_type == UNKNOWNED_PHY) {
		printf("External PHY copper link down: %d\n", ret);
		return -ENOLINK;
	}

	mdio_setup_realtek_phy_autoneg(mac_idx);
	ax_hcp_mac_init(mac_idx);
	while ((ret = ax_hcp_start_link_up(mac_idx)) && --retry) {
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

/* two-port mode (p2p, disabled): mac_idx the host RX FIFO last pointed at */
static u8 eth_net_rx_mac;

/*
 * Two-port mode (p2p, disabled): lock the host TX FIFO on the port of n
 * and point the host RX FIFO at the port of n->other, where the frame n
 * sends will arrive.
 */
static void eth_net_route(struct eth_net *n)
{
	u8 rx_mac = n->other->mac_idx;
	u32 regval;

	regval = readl(AXILS_HCP_BASE + gmii_port_ctrl_offset(rx_mac));
	regval &= ~BIT(HOST_TX_FIFO_EN);
	writel(regval, AXILS_HCP_BASE + gmii_port_ctrl_offset(rx_mac));

	regval = readl(AXILS_HCP_BASE + gmii_port_ctrl_offset(n->mac_idx));
	regval |= BIT(HOST_TX_FIFO_EN);
	writel(regval, AXILS_HCP_BASE + gmii_port_ctrl_offset(n->mac_idx));

	/* same reset + select sequence as hcp_recv() */
	writel(BIT(RX_FIFO_RST), AXILS_HCP_BASE + RX_PACKET_FIFO_0);
	writel((rx_mac - 1) << RX_FIFO_SEL, AXILS_HCP_BASE + RX_PACKET_FIFO_0);
	eth_net_rx_mac = rx_mac;
}

/* Frame payload is already in eth_net_txbuf after the Ethernet header */
static int eth_net_xmit(struct eth_net *n, const u8 *dst, u16 proto,
			int payload_len)
{
	struct ethernet_hdr *et = (struct ethernet_hdr *)eth_net_txbuf;
	int len = ETHER_HDR_SIZE + payload_len;

	memcpy(et->et_dest, dst, ARP_HLEN);
	memcpy(et->et_src, n->mac, ARP_HLEN);
	et->et_protlen = htons(proto);

	if (len < ETH_NET_MIN_FRAME) {
		memset(eth_net_txbuf + len, 0, ETH_NET_MIN_FRAME - len);
		len = ETH_NET_MIN_FRAME;
	}

	if (n->other)
		eth_net_route(n);

	return ax_hcp_send(eth_net_txbuf, len) == len ? 0 : -EIO;
}

/* tha is ignored for a request (target MAC is unknown) */
static int eth_net_send_arp(struct eth_net *n, u16 op, const u8 *tha,
			    struct in_addr tpa)
{
	struct arp_hdr *arp =
		(struct arp_hdr *)(eth_net_txbuf + ETHER_HDR_SIZE);

	arp->ar_hrd = htons(ARP_ETHER);
	arp->ar_pro = htons(PROT_IP);
	arp->ar_hln = ARP_HLEN;
	arp->ar_pln = ARP_PLEN;
	arp->ar_op = htons(op);
	memcpy(&arp->ar_sha, n->mac, ARP_HLEN);
	memcpy(&arp->ar_spa, &n->ip, ARP_PLEN);
	if (op == ARPOP_REQUEST)
		memset(&arp->ar_tha, 0, ARP_HLEN);
	else
		memcpy(&arp->ar_tha, tha, ARP_HLEN);
	memcpy(&arp->ar_tpa, &tpa, ARP_PLEN);

	return eth_net_xmit(n, op == ARPOP_REQUEST ? eth_net_bcast : tha,
			    PROT_ARP, ARP_HDR_SIZE);
}

/* id and seq are in network byte order */
static int eth_net_send_icmp(struct eth_net *n, const u8 *dst_mac,
			     struct in_addr dst_ip, u8 type, u16 id, u16 seq,
			     const u8 *data, int data_len)
{
	struct ip_hdr *ip = (struct ip_hdr *)(eth_net_txbuf + ETHER_HDR_SIZE);
	struct icmp_hdr *icmp = (struct icmp_hdr *)((u8 *)ip + IP_HDR_SIZE);
	int icmp_len = ICMP_HDR_SIZE + data_len;

	if (ETHER_HDR_SIZE + IP_HDR_SIZE + icmp_len > PKTSIZE)
		return -EMSGSIZE;

	icmp->type = type;
	icmp->code = 0;
	icmp->checksum = 0;
	icmp->un.echo.id = id;
	icmp->un.echo.sequence = seq;
	memcpy((u8 *)icmp + ICMP_HDR_SIZE, data, data_len);
	icmp->checksum = compute_ip_checksum(icmp, icmp_len);

	ip->ip_hl_v = 0x45;
	ip->ip_tos = 0;
	ip->ip_len = htons(IP_HDR_SIZE + icmp_len);
	ip->ip_id = htons(n->ip_id++);
	ip->ip_off = htons(IP_FLAGS_DFRAG);
	ip->ip_ttl = 64;
	ip->ip_p = IPPROTO_ICMP;
	ip->ip_sum = 0;
	memcpy(&ip->ip_src, &n->ip, sizeof(n->ip));
	memcpy(&ip->ip_dst, &dst_ip, sizeof(dst_ip));
	ip->ip_sum = compute_ip_checksum(ip, IP_HDR_SIZE);

	return eth_net_xmit(n, dst_mac, PROT_IP, IP_HDR_SIZE + icmp_len);
}

static void eth_net_rx_arp(struct eth_net *n, struct arp_hdr *arp, int len)
{
	struct in_addr spa, tpa;

	if (len < ARP_HDR_SIZE || ntohs(arp->ar_hrd) != ARP_ETHER ||
	    ntohs(arp->ar_pro) != PROT_IP || arp->ar_hln != ARP_HLEN ||
	    arp->ar_pln != ARP_PLEN)
		return;

	memcpy(&spa, &arp->ar_spa, ARP_PLEN);
	memcpy(&tpa, &arp->ar_tpa, ARP_PLEN);
	if (tpa.s_addr != n->ip.s_addr)
		return;

	/* learn the next hop from both its requests and its replies */
	if (n->nexthop_ip.s_addr && spa.s_addr == n->nexthop_ip.s_addr) {
		memcpy(n->peer_mac, &arp->ar_sha, ARP_HLEN);
		n->peer_valid = true;
	}

	if (ntohs(arp->ar_op) == ARPOP_REQUEST) {
		eth_net_send_arp(n, ARPOP_REPLY, &arp->ar_sha, spa);
		n->arp_replied++;
		if (n->verbose)
			printf("ARP: who-has %pI4 tell %pI4 (%pM) -> replied\n",
			       &n->ip, &spa, &arp->ar_sha);
	}
}

static void eth_net_rx_ip(struct eth_net *n, struct ethernet_hdr *et,
			  struct ip_hdr *ip, int len)
{
	struct icmp_hdr *icmp;
	struct in_addr src, dst;
	int hlen, tot, icmp_len;

	if (len < IP_HDR_SIZE || (ip->ip_hl_v & 0xf0) != 0x40)
		return;
	hlen = (ip->ip_hl_v & 0x0f) * 4;
	tot = ntohs(ip->ip_len);
	if (hlen < IP_HDR_SIZE || tot < hlen || tot > len)
		return;
	if (!ip_checksum_ok(ip, hlen))
		return;

	memcpy(&dst, &ip->ip_dst, sizeof(dst));
	if (dst.s_addr != n->ip.s_addr)
		return;
	if (ntohs(ip->ip_off) & (IP_OFFS | IP_FLAGS_MFRAG))
		return;
	if (ip->ip_p != IPPROTO_ICMP)
		return;

	icmp = (struct icmp_hdr *)((u8 *)ip + hlen);
	icmp_len = tot - hlen;
	if (icmp_len < ICMP_HDR_SIZE || !ip_checksum_ok(icmp, icmp_len))
		return;
	memcpy(&src, &ip->ip_src, sizeof(src));

	switch (icmp->type) {
	case ICMP_ECHO_REQUEST:
		eth_net_send_icmp(n, et->et_src, src, ICMP_ECHO_REPLY,
				  icmp->un.echo.id, icmp->un.echo.sequence,
				  (u8 *)icmp + ICMP_HDR_SIZE,
				  icmp_len - ICMP_HDR_SIZE);
		n->icmp_replied++;
		if (n->verbose)
			printf("ICMP: echo request from %pI4 seq=%u len=%d -> replied\n",
			       &src, ntohs(icmp->un.echo.sequence), icmp_len);
		break;
	case ICMP_ECHO_REPLY:
		if (src.s_addr == n->peer_ip.s_addr &&
		    ntohs(icmp->un.echo.id) == ETH_NET_ICMP_ID) {
			n->reply_seq = ntohs(icmp->un.echo.sequence);
			n->reply_seen = true;
			n->reply_us = n->rx_us;
			n->reply_ttl = ip->ip_ttl;
		}
		break;
	}
}

/* Receive and handle at most one frame. */
void eth_net_service(struct eth_net *n)
{
	struct ethernet_hdr *et = (struct ethernet_hdr *)eth_net_rxbuf;
	u8 *payload = eth_net_rxbuf + ETHER_HDR_SIZE;
	int len;

	/* p2p: the frame belongs to the port the RX FIFO listens to */
	if (n->other && eth_net_rx_mac == n->other->mac_idx)
		n = n->other;

	/* poll DAV here so hcp_recv() does not log a timeout per idle poll */
	if (!(readl(AXILS_HCP_BASE + RX_PACKET_FIFO_0) & BIT(RX_FIFO_DAV)))
		return;
	/* timestamp the frame as soon as DAV is seen, before hcp_recv() */
	n->rx_us = timer_get_us();

	len = ax_hcp_recv(n->mac_idx, eth_net_rxbuf, sizeof(eth_net_rxbuf));
	if (len <= ETHER_HDR_SIZE)
		return;
	len -= ETHER_HDR_SIZE;

	switch (ntohs(et->et_protlen)) {
	case PROT_ARP:
		eth_net_rx_arp(n, (struct arp_hdr *)payload, len);
		break;
	case PROT_IP:
		eth_net_rx_ip(n, et, (struct ip_hdr *)payload, len);
		break;
	}
}

static int eth_net_arp_resolve(struct eth_net *n)
{
	ulong start;
	int try;

	for (try = 0; try < ETH_NET_ARP_RETRIES; try++) {
		eth_net_send_arp(n, ARPOP_REQUEST, NULL, n->nexthop_ip);
		start = get_timer(0);
		while (get_timer(start) < ETH_NET_ARP_TIMEOUT_MS) {
			if (ctrlc())
				return -EINTR;
			eth_net_service(n);
			if (n->peer_valid)
				return 0;
		}
		printf("ARP: no reply from %pI4, retry %d\n", &n->nexthop_ip,
		       try + 1);
	}

	return -ETIMEDOUT;
}

int eth_net_run_ping(struct eth_net *n, int count)
{
	u8 data[ETH_NET_PING_DATA_LEN];
	int seq, sent = 0, received = 0;
	ulong start, tx_us, rtt, rtt_min = 0, rtt_max = 0, rtt_sum = 0;
	int ret, i;

	for (i = 0; i < ETH_NET_PING_DATA_LEN; i++)
		data[i] = i;

	ret = eth_net_arp_resolve(n);
	if (ret) {
		printf("ARP: %pI4 did not answer\n", &n->nexthop_ip);
		return CMD_RET_FAILURE;
	}
	printf("ARP: %pI4 is at %pM\n", &n->nexthop_ip, n->peer_mac);

	for (seq = 1; seq <= count; seq++) {
		n->reply_seen = false;
		start = get_timer(0);
		ret = eth_net_send_icmp(n, n->peer_mac, n->peer_ip,
					ICMP_ECHO_REQUEST,
					htons(ETH_NET_ICMP_ID), htons(seq),
					data, sizeof(data));
		if (ret) {
			printf("icmp_seq=%d: send failed (%d)\n", seq, ret);
			break;
		}
		/* after hcp_send(): the TX FIFO fill time is not counted */
		tx_us = timer_get_us();
		sent++;

		while (get_timer(start) < ETH_NET_PING_TIMEOUT_MS) {
			if (ctrlc())
				goto out;
			eth_net_service(n);
			if (n->reply_seen && n->reply_seq == seq)
				break;
		}
		if (n->reply_seen && n->reply_seq == seq) {
			rtt = n->reply_us - tx_us;	/* microseconds */
			if (!received || rtt < rtt_min)
				rtt_min = rtt;
			if (rtt > rtt_max)
				rtt_max = rtt;
			rtt_sum += rtt;
			received++;
			printf("%d bytes from %pI4: icmp_seq=%d ttl=%u time=%lu.%02lu ms\n",
			       (int)(ICMP_HDR_SIZE + sizeof(data)), &n->peer_ip,
			       seq, n->reply_ttl, rtt / 1000, (rtt % 1000) / 10);
		} else {
			printf("Request timeout for icmp_seq=%d\n", seq);
		}

		/* keep answering ARP/ping from the peer between requests */
		while (seq < count && get_timer(start) < ETH_NET_PING_PERIOD_MS) {
			if (ctrlc())
				goto out;
			eth_net_service(n);
		}
	}

out:
	printf("\n--- %pI4 ping statistics ---\n", &n->peer_ip);
	printf("%d packets transmitted, %d received, %d%% packet loss\n",
	       sent, received, sent ? (sent - received) * 100 / sent : 0);
	if (received) {
		rtt = rtt_sum / received;
		printf("rtt min/avg/max = %lu.%03lu/%lu.%03lu/%lu.%03lu ms\n",
		       rtt_min / 1000, rtt_min % 1000, rtt / 1000, rtt % 1000,
		       rtt_max / 1000, rtt_max % 1000);
	}

	return (sent == count && received == sent) ?
		CMD_RET_SUCCESS : CMD_RET_FAILURE;
}

/* bytes sent in ms milliseconds -> "<Mbps>.<3 digits>" */
static void eth_net_print_rate(const char *name, u32 frames, int frame_len,
			       ulong ms)
{
	u64 bytes = (u64)frames * frame_len;
	u32 kbps = div_u64(bytes * 8, ms);	/* bits per ms */

	printf("%s: %u frames, %llu bytes, %u.%03u Mbps, %u frames/s\n", name,
	       frames, bytes, kbps / 1000, kbps % 1000,
	       (u32)div_u64((u64)frames * 1000, ms));
}

/* iperf-like line for the frames sent between from_ms and to_ms */
static void eth_net_print_interval(ulong from_ms, ulong to_ms, u32 frames,
				   int frame_len)
{
	u64 bytes = (u64)frames * frame_len;
	u32 kbps = div_u64(bytes * 8, to_ms - from_ms);	/* bits per ms */
	u32 mib100 = div_u64(bytes * 100, 1024 * 1024);

	printf("[%3lu.%02lu-%3lu.%02lu sec]  %3u.%02u MBytes  %3u.%03u Mbits/sec\n",
	       from_ms / 1000, from_ms % 1000 / 10, to_ms / 1000,
	       to_ms % 1000 / 10, mib100 / 100, mib100 % 100, kbps / 1000,
	       kbps % 1000);
}

/*
 * Send raw broadcast frames back to back for seconds and report the TX
 * rate (frame bytes * 8 / elapsed time) twice: from the frames handed to
 * the host TX FIFO, and from the frames the MAC counted as sent.
 */
int eth_net_run_speed(struct eth_net *n, int seconds)
{
	u8 dst[ARP_HLEN];
	u32 sent = 0, mark_sent = 0, good, total;
	ulong start, ms, mark_ms = 0, limit = (ulong)seconds * 1000;
	int frame_len;

	memcpy(dst, eth_net_bcast, ARP_HLEN);
	/* the generator trims the frame to a multiple of 4 bytes */
	frame_len = generate_ethernet_packet(eth_net_speedbuf, dst, n->mac,
					     ETH_NET_SPEED_PAYLOAD);

	printf("Sending %d-byte frames from %pM on port %d for %d s, Ctrl+C to stop\n",
	       frame_len, n->mac, n->mac_idx - 1, seconds);

	good = AX_READ_MAC_REG32(n->mac_idx, SHIM_STATS_TX_MAC_GOOD);
	total = AX_READ_MAC_REG32(n->mac_idx, SHIM_STATS_TX_MAC_TOTAL);

	start = get_timer(0);
	while ((ms = get_timer(start)) < limit) {
		/* the time printf() takes is counted in the next interval */
		if (ms - mark_ms >= ETH_NET_SPEED_REPORT_MS) {
			eth_net_print_interval(mark_ms, ms, sent - mark_sent,
					       frame_len);
			mark_ms = ms;
			mark_sent = sent;
		}
		/* ctrlc() polls the UART, too slow to call per frame */
		if (!(sent % ETH_NET_SPEED_CTRLC_PERIOD) && ctrlc())
			break;
		if (ax_hcp_send(eth_net_speedbuf, frame_len) != frame_len) {
			printf("Send failed at frame %u\n", sent);
			break;
		}
		sent++;
	}
	ms = get_timer(start);
	if (ms > mark_ms)
		eth_net_print_interval(mark_ms, ms, sent - mark_sent,
				       frame_len);

	/* let the last frame leave the FIFO before reading the counters */
	mdelay(10);
	good = AX_READ_MAC_REG32(n->mac_idx, SHIM_STATS_TX_MAC_GOOD) - good;
	total = AX_READ_MAC_REG32(n->mac_idx, SHIM_STATS_TX_MAC_TOTAL) - total;

	if (!sent || !ms) {
		printf("No frame sent\n");
		return CMD_RET_FAILURE;
	}

	printf("\n--- port %d TX speed, %lu.%03lu s ---\n", n->mac_idx - 1,
	       ms / 1000, ms % 1000);
	eth_net_print_rate("Host FIFO", sent, frame_len, ms);
	eth_net_print_rate("TX_MAC good", good, frame_len, ms);
	printf("TX_MAC total: %u frames\n", total);
	// if (good != sent) {
	// 	printf("Warning: %u frames sent but TX_MAC good counted %u\n",
	// 	       sent, good);
	// 	return CMD_RET_FAILURE;
	// }

	return CMD_RET_SUCCESS;
}
