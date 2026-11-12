// SPDX-License-Identifier: GPL-2.0+
#include "eth_mdio.h"
#include "eth_phy_table.h"

static const struct ax_phy_desc ax_phy_100m[] = {
	{ "DP83822", 0x2000, 0xa240, 0xfff0, AX_MDIO_C22, UNKNOWNED_PHY },
};

static const struct ax_phy_desc ax_phy_1g[] = {
	{ "Realtek", 0x001c, 0xc916, 0xffff, AX_MDIO_C22, REALTEK_PHY },
	{ "Marvell", 0x0141, 0x0dd0, 0xfff0, AX_MDIO_C22, MARVELL_PHY },
};

static const struct ax_phy_desc ax_phy_10g[] = {
	{ "88X3310", 0x002b, 0x09a0, 0xfff0, AX_MDIO_C45, UNKNOWNED_PHY },
};

static const struct ax_speed_desc ax_speeds[] = {
	[AX_ETH_100M] = { "100M RMII", 1, 2, ax_phy_100m,
			  ARRAY_SIZE(ax_phy_100m) },
	[AX_ETH_1G]   = { "1G SGMII", 1, 4, ax_phy_1g,
			  ARRAY_SIZE(ax_phy_1g) },
	[AX_ETH_10G]  = { "10G XGMII", 0, 0, ax_phy_10g,
			  ARRAY_SIZE(ax_phy_10g) },
};

const struct ax_speed_desc *ax_speed_get(u8 speed, u8 mac_idx)
{
	const struct ax_speed_desc *sp;

	if (speed >= ARRAY_SIZE(ax_speeds))
		return NULL;
	sp = &ax_speeds[speed];
	if (mac_idx < sp->mac_idx_min || mac_idx > sp->mac_idx_max)
		return NULL;
	return sp;
}

static const struct ax_phy_desc *ax_phy_match(const struct ax_speed_desc *sp,
					      u8 clause, u16 id1, u16 id2)
{
	const struct ax_phy_desc *phy;
	int i;

	for (i = 0; i < sp->phy_cnt; i++) {
		phy = &sp->phys[i];
		if (phy->clause == clause && phy->id1 == id1 &&
		    (id2 & phy->id2_mask) == (phy->id2 & phy->id2_mask))
			return phy;
	}
	return NULL;
}

static bool ax_phy_list_has(const struct ax_speed_desc *sp, u8 clause)
{
	int i;

	for (i = 0; i < sp->phy_cnt; i++)
		if (sp->phys[i].clause == clause)
			return true;
	return false;
}

const struct ax_phy_desc *ax_phy_probe(u8 mac_idx, u8 speed, u8 *addr)
{
	const struct ax_speed_desc *sp = ax_speed_get(speed, mac_idx);
	const struct ax_phy_desc *phy;
	u16 id1, id2;
	u8 clause, phy_addr;

	*addr = INVALID_MAC_IDX;
	if (!sp)
		return NULL;

	for (clause = AX_MDIO_C22; clause <= AX_MDIO_C45; clause++) {
		if (!ax_phy_list_has(sp, clause))
			continue;
		ax_mdio_init(mac_idx, clause);
		for (phy_addr = 0; phy_addr < 32; phy_addr++) {
			id1 = ax_mdio_read(mac_idx, clause, phy_addr, 1, 2);
			id2 = ax_mdio_read(mac_idx, clause, phy_addr, 1, 3);
			phy = ax_phy_match(sp, clause, id1, id2);
			if (phy) {
				printf("MDIO Found PHY at addr: 0x%x\n",
				       phy_addr);
				printf("ID1: 0x%x\n", id1);
				printf("ID2: 0x%x\n", id2);
				printf("%s phy (%s, Clause %d)\n", phy->name,
				       sp->name,
				       clause == AX_MDIO_C45 ? 45 : 22);
				*addr = phy_addr;
				return phy;
			}
		}
	}
	return NULL;
}
