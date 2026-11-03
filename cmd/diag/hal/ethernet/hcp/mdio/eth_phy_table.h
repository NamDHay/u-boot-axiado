/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef __ETH_PHY_TABLE_H__
#define __ETH_PHY_TABLE_H__
#include <linux/types.h>

enum ax_eth_speed {
	AX_ETH_100M,
	AX_ETH_1G,
	AX_ETH_10G,
};

enum ax_mdio_clause {
	AX_MDIO_C22,
	AX_MDIO_C45,
};

struct ax_phy_desc {
	const char *name;
	u16 id1;
	u16 id2;
	u16 id2_mask;
	u8 clause;
	u8 phy_type;
};

struct ax_speed_desc {
	const char *name;
	u8 mac_idx_min;
	u8 mac_idx_max;
	const struct ax_phy_desc *phys;
	int phy_cnt;
};

const struct ax_speed_desc *ax_speed_get(u8 speed, u8 mac_idx);
const struct ax_phy_desc *ax_phy_probe(u8 mac_idx, u8 speed, u8 *addr);
u16 ax_mdio_read(u8 mac_idx, const struct ax_phy_desc *phy, u8 addr,
		 u8 devad, u16 reg);
void ax_mdio_write(u8 mac_idx, const struct ax_phy_desc *phy, u8 addr,
		   u8 devad, u16 reg, u16 val);

#endif
