/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Minimal ARP / IPv4 / ICMP echo on top of the diag HCP HAL.
 */
#ifndef __ETH_NET_H__
#define __ETH_NET_H__

#include <net.h>
#include <linux/types.h>

/* Network identity of one SGMII port (diag port N = mac_idx N + 1) */
struct eth_net_port_cfg {
	const char *ip;
	const char *netmask;
	const char *gw;		/* next hop for peers outside the subnet */
	u8 mac[6];
};

static const struct eth_net_port_cfg eth_net_port0_cfg = {
	"10.7.10.200", "255.255.255.0", "10.7.10.1",
	{ 0x00, 0x11, 0x22, 0x33, 0x44, 0x55 }
};

static const struct eth_net_port_cfg eth_net_port1_cfg = {
	"10.7.10.201", "255.255.255.0", "10.7.10.1",
	{ 0x00, 0x11, 0x22, 0x33, 0x44, 0x56 }
};

static const struct eth_net_port_cfg eth_net_port2_cfg = {
	"10.7.10.202", "255.255.255.0", "10.7.10.1",
	{ 0x00, 0x11, 0x22, 0x33, 0x44, 0x57 }
};

static const struct eth_net_port_cfg eth_net_port3_cfg = {
	"10.7.10.203", "255.255.255.0", "10.7.10.1",
	{ 0x00, 0x11, 0x22, 0x33, 0x44, 0x58 }
};

/* State of one ping / receive session on a port */
struct eth_net {
	u8 mac_idx;
	u8 mac[ARP_HLEN];
	struct in_addr ip;
	struct in_addr netmask;
	struct in_addr gw;
	u16 ip_id;
	bool verbose;
	/* host being pinged */
	struct in_addr peer_ip;
	/* single-entry ARP cache: peer_ip itself, or the gateway if off-subnet */
	struct in_addr nexthop_ip;
	u8 peer_mac[ARP_HLEN];
	bool peer_valid;
	/* timer_get_us() when the frame being handled was seen in the RX FIFO */
	ulong rx_us;
	/* last echo reply received for ETH_NET_ICMP_ID */
	u16 reply_seq;
	bool reply_seen;
	ulong reply_us;
	u8 reply_ttl;
	u32 arp_replied;
	u32 icmp_replied;
	/* two-port mode (p2p, disabled): session at the other end, else NULL */
	struct eth_net *other;
};

/* Load the identity of the port (eth_net_portN_cfg) into n. */
void eth_net_init(struct eth_net *n, u8 mac_idx);
/* Bring the copper + SGMII link up. Returns 0 on link up. */
int eth_net_link_up(u8 mac_idx);
/* Receive and handle at most one frame. */
void eth_net_service(struct eth_net *n);
/* Resolve n->nexthop_ip with ARP, then send count ICMP echo requests. */
int eth_net_run_ping(struct eth_net *n, int count);
/* Send raw broadcast frames for seconds and print the TX rate. */
int eth_net_run_speed(struct eth_net *n, int seconds);

#endif
