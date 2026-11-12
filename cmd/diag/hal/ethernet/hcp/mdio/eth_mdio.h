/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef __ETH_MDIO_H__
#define __ETH_MDIO_H__
#include <dm.h>
#include <asm/io.h>
#include <stdio.h>
#include <stdint.h>
#include <linux/types.h>
#include <linux/delay.h>
#include "../shim/eth_reg.h"
#include "../shim/ax_reg.h"
#include "../shim/mac_define.h"
#include "../eth_common_api/eth_common_api.h"

#define REALTEK_PHY 	31
#define MARVELL_PHY 	22
#define UNKNOWNED_PHY	0xFF
#define INVALID_MAC_IDX 0xFF
#define XGMII_PHY_RESET_TIMEOUT_MS	2000

#define DP83822_PHY_ID1		0x2000
#define DP83822_PHY_ID2		0xa240
#define DP83822_PHY_ID2_MASK	0xfff0

enum ax_mdio_clause {
	AX_MDIO_C22,
	AX_MDIO_C45,
};

void ax_mdio_init(u8 mac_idx, enum ax_mdio_clause clause);
u32 ax_mdio_read(u8 mac_idx, enum ax_mdio_clause clause, u8 phy_addr,
		 u8 devad, u16 reg);
void ax_mdio_write(u8 mac_idx, enum ax_mdio_clause clause, u8 phy_addr,
		   u8 devad, u16 reg, u16 val);

/* --- MDIO Phy Power Cycle --- */
void mdio_phy_power_cycle(u8 mac_idx, u8 phy_addr, u8 reg);
void mdio_set_phy_powerup(u8 mac_idx, u8 phy_addr, u8 reg);
void mdio_set_phy_powerdown(u8 mac_idx, u8 phy_addr, u8 reg);

/* --- Common PHY discovery and config --- */
u8 mdio_find_phy_id(u8 mac_idx, u16 *id1, u16 *id2);
u8 mdio_find_phy_by_id(u8 mac_idx, u16 match_id1, u16 match_id2,
		       u16 id2_mask, u16 *id1, u16 *id2);
u8 xgmii_mdio_scan_phy_address(u8 mac_idx);
void mdio_setup_phy_mode(u8 mac_idx, u8 phy_addr, u8 reg);
void mdio_select_page(u8 mac_idx, u8 phy_addr, u8 reg,
		      u8 page);
void mdio_phy_config(u8 mac_idx, u8 phy_addr, u8 reg);
void mdio_reset_phy(u8 mac_idx, u8 phy_addr, u8 reg);
void mdio_set_phy_an(u8 mac_idx, u8 phy_addr, u8 reg);
void mdio_setup_copper_an(u8 mac_idx, u8 phy_addr, u8 reg);
void mdio_enable_exphy_lb(u8 mac_idx, u8 phy_addr, u8 reg);
void mdio_disable_exphy_lb(u8 mac_idx, u8 phy_addr, u8 reg);
int mdio_wait_copper_linkup(u8 mac_idx, u8 phy_addr, u8 reg);

/* --- Marvell PHY configuration --- */
void mdio_set_advertise_1000base_t_full_duplex(u8 mac_idx,
					       u8 phy_addr);
void mdio_set_sgmii_an_bypass_mode(u8 mac_idx, u8 phy_addr);

/* --- Stub loopback + PG --- */
void mdio_disable_stub_lb(u8 mac_idx, u8 phy_addr, u8 reg);
void mdio_set_stub_lb(u8 mac_idx, u8 phy_addr, u8 reg);
void mdio_setup_start_pg(u8 mac_idx, u8 phy_addr, u8 reg);
void mdio_phy_88e1512_packet_64bytes_gen(u8 mac_idx, u8 phy_addr, u8 reg);
void mdio_phy_88e1512_packet_gen_stop(u8 mac_idx, u8 phy_addr, u8 reg);
void mdio_check_packet_gen_result(u8 mac_idx, u8 phy_addr, u8 reg);

void realtek_conf(u8 mac_idx, int phy_addr);

/* --- Realtek PHY auto-neg enable/disable (SGMII bit-12) + AN wait --- */
void mdio_setup_realtek_phy_autoneg(u8 mac_idx);
void mdio_disable_sgmii_autoneg(u8 mac_idx);
void mdio_rmii_enable_mii_loopback(u8 mac_idx, u8 phy_addr);
void mdio_rmii_disable_mii_loopback(u8 mac_idx, u8 phy_addr);
void mdio_disable_realtek_phy_autoneg(u8 mac_idx, u8 phy_addr,
				     u8 reg);
uint32_t wait_auto_neg_comple(u8 mac_idx, u8 phyaddr,
				     u8 reg);

/* --- Eth re-init the exPHY (top-level policy driver) --- */
int eth_init_exPhy(u8 mac_idx, u8 phyaddr, eth_test_options opt,
		    u8 reg);

/* --- XGMII (10G) Clause-45 PHY bring-up (SW reset / FW revision / loopback) --- */
int eth_xgmii_TUNIT_sw_reset(u32 phyaddr);
int eth_xgmii_copper_sw_reset(u32 phyaddr);
void xgmii_enable_phy_loopback(u8 mac_idx, u32 phyaddr);
void xgmii_disable_phy_loopback(u8 mac_idx, u32 phyaddr);
#endif
