// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#ifndef __AX_NET_H
#define __AX_NET_H

#include <net.h>
#include <linux/errno.h>
#include <linux/types.h>

struct ax_eth_ops;

struct net_dev {
	const struct ax_eth_ops *ops;
	uint32_t port;
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

static inline int ax_eth_start(struct net_dev *port)
{
	if (!port || !port->ops || port->ops->start == NULL) return -ENOSYS;

	return port->ops->start(port);
}

static inline int ax_eth_send(struct net_dev *port, void *packet, int length)
{
	if (!port || !port->ops || port->ops->send == NULL) return -ENOSYS;

	return port->ops->send(port, packet, length);
}

static inline int ax_eth_recv(struct net_dev *port, int flags, uchar **packetp)
{
	if (!port || !port->ops || port->ops->recv == NULL) return -ENOSYS;

	return port->ops->recv(port, flags, packetp);
}

static inline int ax_eth_free_pkt(struct net_dev *port, uchar *packet,
				  int length)
{
	if (!port || !port->ops || port->ops->free_pkt == NULL) return -ENOSYS;

	return port->ops->free_pkt(port, packet, length);
}

static inline void ax_eth_stop(struct net_dev *port)
{
	if (!port || !port->ops || port->ops->stop == NULL) return;

	port->ops->stop(port);
}

static inline int ax_eth_mcast(struct net_dev *port,
			       const u8 *enetaddr, int join)
{
	if (!port || !port->ops || port->ops->mcast == NULL) return -ENOSYS;

	return port->ops->mcast(port, enetaddr, join);
}

static inline int ax_eth_write_hwaddr(struct net_dev *port)
{
	if (!port || !port->ops || port->ops->write_hwaddr == NULL) return -ENOSYS;

	return port->ops->write_hwaddr(port);
}

static inline int ax_eth_read_rom_hwaddr(struct net_dev *port)
{
	if (!port || !port->ops || port->ops->read_rom_hwaddr == NULL) return -ENOSYS;

	return port->ops->read_rom_hwaddr(port);
}

static inline int ax_eth_set_promisc(struct net_dev *port, bool enable)
{
	if (!port || !port->ops || port->ops->set_promisc == NULL) return -ENOSYS;

	return port->ops->set_promisc(port, enable);
}

static inline int ax_eth_get_sset_count(struct net_dev *port)
{
	if (!port || !port->ops || port->ops->get_sset_count == NULL) return -ENOSYS;

	return port->ops->get_sset_count(port);
}

static inline void ax_eth_get_strings(struct net_dev *port, u8 *data)
{
	if (!port || !port->ops || port->ops->get_strings == NULL) return;

	port->ops->get_strings(port, data);
}

static inline void ax_eth_get_stats(struct net_dev *port, u64 *data)
{
	if (!port || !port->ops || port->ops->get_stats == NULL) return;

	port->ops->get_stats(port, data);
}

#endif /* __AX_NET_H */
