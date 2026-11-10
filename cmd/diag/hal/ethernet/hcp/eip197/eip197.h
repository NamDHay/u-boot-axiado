/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef __EIP197_H
#define __EIP197_H

#include <linux/types.h>
#include <linux/string.h>
#include "eip197_reg.h"
#include "eth_mac.h"

#define EIP197_DRBG_TIMEOUT_MS	1000
#define EIP197_SPEED_BATCH	16
#define EIP197_SPEED_PKT_LEN	1200
#define EIP197_SPEED_PKT_MIN	62
#define EIP197_SPEED_PKT_MAX	9572
#define EIP197_SPEED_SLOT	0x3000
#define EIP197_PKT_BUF_SIZE	9600
#define EIP197_SPEED_TIMEOUT_MS	100
#define EIP197_SPEED_REPORT_MS	1000

void eth_eip202_config_data_fetch_engine(void);
void eth_eip202_config_data_store_engine(void);
void eth_eip202_config(void);
void eth_eip197_clear_statistics_dropped_allPE(void);
void eth_eip197_clear_oce_scratchpad(void);
void eth_eip197_clear_ice_scratchpad(void);
void eth_eip96_config(void);
void eth_eip197_config_record_cache_coherence(void);
void eth_eip197_config_pe(void);
void eth_eip197_config_PE_thread(void);
int eth_eip206_config(void);
void eth_eip197_config_engine_sync_mask(void);
void eth_eip197_config_reload_oce(void);
void eth_eip197_config_reload_ice(void);
int eth_eip_polling_ifpp_firmware(void);
int eth_eip_polling_ipue_firmware(void);
void eth_eip197_ifpp_firmware_load(void);
void eth_eip197_ipue_firmware_load(void);
int eth_eip_polling_ofpp_firmware(void);
int eth_eip_polling_opue_firmware(void);
void eth_eip197_ofpp_firmware_load(void);
void eth_eip197_opue_firmware_load(void);
int eth_eip197_wait_for_firmware_ready(void);
int eth_eip197_load_micro_engines_firmware(void);
void eth_eip197_csram_trc_config(void);
int eth_eip197_config_drbg(void);
void eth_eip197_config_classification_support(void);
void eth_eip197_config_ifc_lut(void);
void eth_eip197_config_flue_lut_enable(void);
int eth_eip207_config(void);
void eth_eip197_config_RDR_ring(uint32_t b_len);
void eth_eip197_assign_IFC_to_allPE(uint8_t eip_cdr);
void eth_eip197_redirect_src_ifc_to_inline_interface_allPE(uint8_t eip_cdr);
void eth_eip197_config_CDR_ring(uint32_t b_len);
void eth_eip_197_interface_configuration(uint32_t b_len, uint8_t eip_cdr);

void gen_transform_record_egress(uint32_t transf_rec_ocm_addr, CntxCtrl_t cntx_ctrl);
void gen_transform_record_ingress(uint32_t transf_rec_ocm_addr, CntxCtrl_t cntx_ctrl, uint8_t eip_rdr);
void eth_init_transform_record_egress(CntxCtrl_t cntx_ctrl_out);
void eth_init_transform_record_ingress(CntxCtrl_t cntx_ctrl_in, uint8_t eip_rdr);
void hash_compute(uint32_t iv[4], uint32_t hash_rec[13]);
void gen_lookup_hash_record(uint32_t hash_rec_ocm_addr, HashRecd_t hash_recd, uint32_t egress, uint32_t iv_value[4]);
void eth_init_lookup_hash_record_egress(HashRecd_t *hash_rec);
void eth_init_lookup_hash_record_ingress(HashRecd_t *hash_rec);
void gen_command_desc(uint32_t cmd_desc_ocm_addr, uint32_t packet_addr, uint32_t particle_size, uint32_t first_seg, uint32_t last_seg, uint32_t tdest);
void gen_result_desc(uint32_t num_desc, uint32_t result_desc_ocm_addr, uint32_t first_seg, uint32_t last_seg, uint32_t payload_addr, uint32_t payload_length);
void eth_gen_result_descriptor(uint32_t b_len);
void gen_egress_pkt(uint32_t pkt_ocm_addr, pktConfig_t pktcfg, uint32_t ind);
void gen_egress_pkt_ipv4(uint32_t pkt_ocm_addr, pktConfig_t pktcfg, uint32_t ind);
void eth_setup_egress_packet(pktConfig_t *pkt_cfg, uint32_t pkt_len);

void eth_print_eip_options_info(void);
void eip197_bind_flue_memory(void);
int eth_eip197_init(uint32_t b_len, uint8_t eip_cdr);
void eth_eip197_setup_session(CntxCtrl_t cntx_ctrl_out, CntxCtrl_t cntx_ctrl_in, HashRecd_t hash_rec, uint32_t eip_rdr, uint32_t b_len);
void eth_eip197_prepare_data(pktConfig_t *pkt_cfg, uint32_t pkt_len, uint32_t b_len);
void eth_eip197_print_dbg(uint8_t mac_idx, uint32_t cdr_y_prep_count, uint32_t rdr_y_prep_count);
uint16_t eth_eip197_run_traffic_test(uint8_t mac_idx, uint8_t tdest, pktConfig_t egress_pkt_cfg, uint32_t b_len, uint8_t eip_cdr, uint8_t eip_rdr);
uint16_t eth_eip_197_test(uint8_t mac_idx, uint8_t tdest);
int eth_eip197_speed(uint8_t mac_idx, uint8_t tdest, int seconds,
		     uint32_t pkt_len);

#endif
