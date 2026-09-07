// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#ifndef __ETH_H
#define __ETH_H

#define ARP_HLEN 6
struct net_dev {
	void __iomem *base;
	unsigned char enetaddr[ARP_HLEN];
	int phy_interface;
	int max_speed;
	void *priv_pdata;    
};

struct ax_eth_ops {
	int (*start)(struct net_dev *port);
	int (*send)(struct net_dev *port, void *packet, int length);
	int (*recv)(struct net_dev *port, int flags, uchar **packetp);
	int (*free_pkt)(struct net_dev *port, uchar *packet, int length);
	void (*stop)(struct net_dev *port);
	int (*mcast)(struct net_dev *port, const u8 *enetaddr, int join);
	int (*write_hwaddr)(struct net_dev *port);
	int (*read_rom_hwaddr)(struct net_dev *port);
	int (*set_promisc)(struct net_dev *port, bool enable);
	int (*get_sset_count)(struct net_dev *port);
	void (*get_strings)(struct net_dev *port, u8 *data);
	void (*get_stats)(struct net_dev *port, u64 *data);


    /* DIAG config */
};
extern struct ax_eth_ops eth;

struct ethernet_hdr {
	u8		et_dest[ARP_HLEN];	/* Destination node	*/
	u8		et_src[ARP_HLEN];	/* Source node		*/
	u16		et_protlen;		/* Protocol or length	*/
} __packed;

/* Ethernet header size */
#define ETHER_HDR_SIZE	(sizeof(struct ethernet_hdr))

/**
 * net_random_ethaddr - Generate software assigned random Ethernet address
 * @addr: Pointer to a six-byte array containing the Ethernet address
 *
 * Generate a random Ethernet address (MAC) that is not multicast
 * and has the local assigned bit set.
 */
static inline void net_random_ethaddr(uchar *addr)
{
	int i;
	unsigned int seed = get_ticks();

	for (i = 0; i < 6; i++)
		addr[i] = rand_r(&seed);

	addr[0] &= 0xfe;	/* clear multicast bit */
	addr[0] |= 0x02;	/* set local assignment bit (IEEE802) */
}

/**
 * is_zero_ethaddr - Determine if give Ethernet address is all zeros.
 * @addr: Pointer to a six-byte array containing the Ethernet address
 *
 * Return true if the address is all zeroes.
 */
static inline int is_zero_ethaddr(const u8 *addr)
{
	return !(addr[0] | addr[1] | addr[2] | addr[3] | addr[4] | addr[5]);
}

/**
 * is_multicast_ethaddr - Determine if the Ethernet address is a multicast.
 * @addr: Pointer to a six-byte array containing the Ethernet address
 *
 * Return true if the address is a multicast address.
 * By definition the broadcast address is also a multicast address.
 */
static inline int is_multicast_ethaddr(const u8 *addr)
{
	return 0x01 & addr[0];
}

/*
 * is_broadcast_ethaddr - Determine if the Ethernet address is broadcast
 * @addr: Pointer to a six-byte array containing the Ethernet address
 *
 * Return true if the address is the broadcast address.
 */
static inline int is_broadcast_ethaddr(const u8 *addr)
{
	return (addr[0] & addr[1] & addr[2] & addr[3] & addr[4] & addr[5]) ==
		0xff;
}

/*
 * is_valid_ethaddr - Determine if the given Ethernet address is valid
 * @addr: Pointer to a six-byte array containing the Ethernet address
 *
 * Check that the Ethernet address (MAC) is not 00:00:00:00:00:00, is not
 * a multicast address, and is not FF:FF:FF:FF:FF:FF.
 *
 * Return true if the address is valid.
 */
static inline int is_valid_ethaddr(const u8 *addr)
{
	/* FF:FF:FF:FF:FF:FF is a multicast address so we don't need to
	 * explicitly check for it here. */
	return !is_multicast_ethaddr(addr) && !is_zero_ethaddr(addr);
}

static inline int ax_eth_start(struct net_dev *port)
{
	if (eth.start == NULL) return -ENOSYS;

	return eth.start(port);
}

static inline int ax_eth_send(struct net_dev *port, void *packet, int length)
{
	if (eth.send == NULL) return -ENOSYS;

	return eth.send(port, packet, length);
}

static inline int ax_eth_recv(struct net_dev *port, int flags, uchar **packetp)
{
	if (eth.recv == NULL) return -ENOSYS;

	return eth.recv(port, flags, packetp);
}

static inline int ax_eth_free_pkt(struct net_dev *port, uchar *packet,
				  int length)
{
	if (eth.free_pkt == NULL) return -ENOSYS;

	return eth.free_pkt(port, packet, length);
}

static inline void ax_eth_stop(struct net_dev *port)
{
	if (eth.stop == NULL) return;

	eth.stop(port);
}

static inline int ax_eth_mcast(struct net_dev *port,
			       const u8 *enetaddr, int join)
{
	if (eth.mcast == NULL) return -ENOSYS;

	return eth.mcast(port, enetaddr, join);
}

static inline int ax_eth_write_hwaddr(struct net_dev *port)
{
	if (eth.write_hwaddr == NULL) return -ENOSYS;

	return eth.write_hwaddr(port);
}

static inline int ax_eth_read_rom_hwaddr(struct net_dev *port)
{
	if (eth.read_rom_hwaddr == NULL) return -ENOSYS;

	return eth.read_rom_hwaddr(port);
}

static inline int ax_eth_set_promisc(struct net_dev *port, bool enable)
{
	if (eth.set_promisc == NULL) return -ENOSYS;

	return eth.set_promisc(port, enable);
}

static inline int ax_eth_get_sset_count(struct net_dev *port)
{
	if (eth.get_sset_count == NULL) return -ENOSYS;

	return eth.get_sset_count(port);
}

static inline void ax_eth_get_strings(struct net_dev *port, u8 *data)
{
	if (eth.get_strings == NULL) return;

	eth.get_strings(port, data);
}

static inline void ax_eth_get_stats(struct net_dev *port, u64 *data)
{
	if (eth.get_stats == NULL) return;

	eth.get_stats(port, data);
}

#endif /* __ETH_H */
