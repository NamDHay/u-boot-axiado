/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef __ETH_MAC_H__
#define __ETH_MAC_H__
#include <dm.h>
#include <asm/io.h>
#include <stdio.h>
#include <stdint.h>
#include <linux/types.h>
#include "../shim/eth_reg.h"
#include "../shim/ax_reg.h"
#include "../shim/mac_define.h"

#define AX_READ_MAC_REG32(mac, x) \
	(readl((AXILS_SHIM_MAC_BASE + (mac * MAC_BASE_OFFSET)) + x))

/* --- SGMII PHY / SerDes dump + fast-sim / tuning --- */
void sgmii_phy_dump(u8 mac_idx);
void sgmii_fast_sim(u8 mac_idx);
void sgmii_fast_sim_other(u8 mac_idx);
void sgmii_innosilicon_adjust(u8 mac_idx);
void sgmii_innosilicon_pi_config(u8 mac_idx);
int sgmii_link_try(u8 mac_idx);

/* --- XGMII (10G) SerDes / PCS / PMA fast-sim --- */
void xgmii_fast_sim(void);
void mac_reset_sgmii(u8 mac_idx);
void mac_init_10g(void);
void xgmii_enable_near_loopback(void);

/* --- MAC 1G reset / init / EIP bypass / loopback --- */
int mac_reset_1g(u8 mac_idx);
void mac_init_1g(u8 mac_idx);
void mac_cfg_jabber_packet(u8 mac_idx);
void eth_set_eipbypass(u8 mac_idx);
void enable_internal_near_end_lb(u8 mac_idx);
void disable_internal_near_end_lb(u8 mac_idx);

/* --- RMII: pin setup + GMII/NCSI port control --- */
void eth_setup_gpio_rmii(void);
void eth_enable_gmii_ncsi(u8 mac_idx, u8 rmii_mode);

/* --- MAC stats counter print (test helper, uses AX_READ_MAC_REG32) --- */
void eth_print_mac_counters(uint32_t mac_idx);

u8 eth_wait_internal_phy_linkup(uint32_t mac_idx);

/* --- Monitor helpers: auto-neg, GE, HPI and MAC counters --- */
int auto_neg_mon_clear_bit(int mac_idx);
int eth_auto_neg_mon(int mac_idx);
int ge_mon_clear_bit(int mac_idx);
int eth_ge_mon(int mac_idx);
int eth_print_broadcast_counters(int mac_idx);
int eth_print_ctrl_frame_counters(int mac_idx);
int eth_print_drop_event_counters(int mac_idx);
u32 eth_read_fragments_counter(int mac_idx);
u32 eth_print_fragments_counters(int mac_idx);
u32 eth_print_crc_err_counters(int mac_idx);
u32 eth_print_lengtherrors_counters(int mac_idx);
int eth_print_irq_hpi(int mac_idx);
int irq_hpi_clear_bit(int mac_idx);

/* --- Ingress RX FIFO drain (long timeout, port of baremetal ingress_packet_error_check) --- */
int ingress_packet_error_check(int mac_idx, unsigned char buf[], int blen);
#endif
