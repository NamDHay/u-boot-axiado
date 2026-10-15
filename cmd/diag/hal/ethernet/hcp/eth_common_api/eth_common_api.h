/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef __ETH_COMMON_API_H__
#define __ETH_COMMON_API_H__
#include <dm.h>
#include <asm/io.h>
#include <stdio.h>
#include <stdint.h>
#include <linux/types.h>

#define INVALID_MAC_IDX 0xFF

/* Test mode constants (shared by mdio/'s eth_init_exPhy and its callers) */
#define DIS_EX_PHY_AN_BYBASS       	0
#define ENA_EX_PHY_AN_BYBASS       	1
#define DIS_EX_PHY_MODE             0
#define ENA_EX_PHY_MODE             1

#define INTERNAL_PHY_LOOPBACK       0
#define EXTERNAL_PHY_LOOPBACK       1
#define EXTERNAL_PHY_RJ45_LOOPBACK  2
#define EXTERNAL_PHY_COPPER         3	/* copper AN, no loopback */

typedef struct {
	int eth_run_test_ops;
	int exPhy_an_bypass;
	int exPhy_mode;
} eth_test_options;

#define ETH_TEST_MAC_SIZE			 6
#define ETH_TEST_HEADER_SIZE		 14
#define ETH_TEST_MIN_PAYLOAD		 46
#define ETH_TEST_MAX_PAYLOAD		 9600
#define ETH_TEST_TRAFFIC_SIZE		 248
#define ETH_TEST_TRAFFIC_MAX_BUFFER	  (10 * 1024)
#define ETH_TEST_ETHER_TYPE			 0x88B5
#define ETH_TEST_TEST_TRAFFIC_PATTERN 0xAA
#define ETH_TEST_DFLT_FRAMES		 0x40

/* MAC control frame (IEEE 802.3 Annex 31A) used by the ctrl-frame test */
#define ETHERTYPE_FLOW_CONTROL		 0x8808
#define ETH_PF_HEADER_SIZE			 18	/* DA + SA + type + opcode + param */
#define CTRL_OPCODE					 0x0002	/* any value other than PAUSE (0x0001) */
#define CTRL_FRM_PARAMETER			 0xFFFF

/* --- Packet API (test helpers) --- */
int generate_ethernet_packet(unsigned char *tx_buffer, unsigned char *dest_mac,
   unsigned char *src_mac, int payload_len);
int compare_packets(u8 mac_idx, unsigned char *received, int received_len,
unsigned char *sent, int sent_len);
#endif
