// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#ifndef __CAN_H
#define __CAN_H

/*
 * CAN mode
 */
enum can_mode {
	CAN_MODE_STOP = 0,
	CAN_MODE_START,
	CAN_MODE_SLEEP
};

#define ID_LEN 29
struct can_dev {
	void __iomem *base;
	unsigned char mask[ARP_HLEN];
	int max_speed;
	void *priv_pdata;    
};

struct ax_can_ops {
	int (*start)(struct can_dev *port);
	int (*send)(struct can_dev *port, void *packet, int length);
	int (*recv)(struct can_dev *port, int flags, uchar **packetp);
	void (*stop)(struct can_dev *port);
	void (*get_stats)(struct can_dev *port, u64 *data);
};
extern struct ax_can_ops can;

#endif /*  __CAN_H */
