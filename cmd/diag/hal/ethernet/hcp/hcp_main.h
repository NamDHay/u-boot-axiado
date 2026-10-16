/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy namhuyngn03@gmail.com
 */
#ifndef __HCP_H
#define __HCP_H
#include "mac/eth_mac.h"
#include "mdio/eth_mdio.h"
#include "eth_common_api/eth_common_api.h"

struct ax_phy_info {
    u8 phyaddr;
    u8 phy_type;    
    eth_test_options opt;
};

static int hcp_mac_init(unsigned mac_idx);
static int hcp_phy_init(unsigned mac_idx, int test_ops, struct ax_phy_info *info);
static int hcp_start_link_up(unsigned mac_idx);
static int hcp_stop_link_up(unsigned mac_idx);
static int hcp_recv(unsigned mac_idx, unsigned char *packet, int length);
static int hcp_send(unsigned char *packet, int length);
struct ax_hcp_ops {
    int (*mac_init)(unsigned mac_idx);
    int (*phy_init)(unsigned mac_idx, int test_ops, struct ax_phy_info *info);
    int (*start_link_up)(unsigned mac_idx);
    int (*stop_link_up)(unsigned mac_idx);    
    int (*recv)(unsigned mac_idx,unsigned char *packet,int length);
    int (*send)(unsigned char *packet,int length);
    
};
extern struct ax_hcp_ops hcp;
static inline int ax_hcp_mac_init(unsigned mac_idx) {  
    if (hcp.mac_init == NULL) return -ENOSYS;

    return hcp.mac_init(mac_idx); 
}
/*
 * test_ops: EXTERNAL_PHY_LOOPBACK, EXTERNAL_PHY_RJ45_LOOPBACK or
 * EXTERNAL_PHY_COPPER.
 * Returns eth_init_exPhy() verdict: for RJ45/COPPER, 1 = copper link up.
 */
static inline int ax_hcp_phy_init(unsigned mac_idx, int test_ops, struct ax_phy_info *info) {  
    if (hcp.phy_init == NULL) return -ENOSYS;

    return hcp.phy_init(mac_idx, test_ops, info); 
}
/* Wait for SGMII link up. Returns 0, -ETIMEDOUT or -EINVAL. */
static inline int ax_hcp_start_link_up(unsigned mac_idx) {
    if (hcp.start_link_up == NULL) return -ENOSYS;

    return hcp.start_link_up(mac_idx); 
}
/* Undo loopbacks, disable MAC TX/RX and release host TX FIFO. */
static inline int ax_hcp_stop_link_up(unsigned mac_idx) {
    if (hcp.stop_link_up == NULL) return -ENOSYS;

    return hcp.stop_link_up(mac_idx); 
}

static inline int ax_hcp_recv(unsigned mac_idx,unsigned char *packet,int length) {  
    if (hcp.recv == NULL) return -ENOSYS;

    return hcp.recv(mac_idx,packet,length); 
}
static inline int ax_hcp_send(unsigned char*packet,int length) {  
    if (hcp.send == NULL) return -ENOSYS;

    return hcp.send(packet,length); 
}

#endif