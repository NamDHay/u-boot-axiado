/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef __EIP197_REG_H
#define __EIP197_REG_H

#include <linux/types.h>
#include "eth_reg.h"

#define IPV6_PACKET 1
#define IPV4_PACKET 2
#define ETH_EIP_PACKET_TYPE		IPV6_PACKET
#define CONTEXTBUF_COMMAND_BASE_ADDR		0x850B0000
#define LOOKUP_HASH_BASE 			0x851D0000
#define CONTEXTBUF_IGRS_OFFSET			0x00008000
#define EGRS_PACKET_BASE_ADDR			0x852E0000
#define IGRS_PACKET_BASE_ADDR			0x85310000
#define CDR_RING_BASE_ADDR 			0x852C0000
#define RDR_RING_BASE_ADDR 			0x853C8000
#define EIP197_DDR_BASE				0x85000000
#define EIP197_DDR_SIZE				0x00600000
#define EIP197_NOF_PE	8
#define IV_0   0x11111111
#define IV_1   0x22222222
#define IV_2   0x33333333
#define IV_3   0x44444444
#define PROTO_UDP  	0x11
#define PROTO_ESP  	0x32
#define UDP_SRC_PORT	0x5995
#define UDP_DST_PORT	0x6886
#define HIA_CDR_0_RING_BASE_ADDR_LO     0x80000
#define HIA_CDR_0_DATA_BASE_ADDR_LO     0x80008
#define HIA_CDR_0_ATOK_BASE_ADDR_LO     0x80010
#define HIA_CDR_0_RING_SIZE             0x80018
#define HIA_CDR_0_DESC_SIZE             0x8001c
#define HIA_CDR_0_CFG                   0x80020
#define HIA_CDR_0_DMA_CFG               0x80024
#define HIA_CDR_0_THRESH                0x80028
#define HIA_CDR_0_COUNT                 0x8002c
#define HIA_CDR_0_PROC_COUNT            0x80030
#define HIA_CDR_0_PREP_PNTR             0x80034
#define HIA_CDR_0_PROC_PNTR             0x80038
#define HIA_CDR_0_STAT                  0x8003c
#define HIA_RDR_0_RING_BASE_ADDR_LO     0x80800
#define HIA_RDR_0_DATA_BASE_ADDR_LO     0x80808
#define HIA_RDR_0_RING_SIZE             0x80818
#define HIA_RDR_0_DESC_SIZE             0x8081c
#define HIA_RDR_0_CFG                   0x80820
#define HIA_RDR_0_DMA_CFG               0x80824
#define HIA_RDR_0_THRESH                0x80828
#define HIA_RDR_0_PREP_COUNT            0x8082c
#define HIA_RDR_0_PROC_COUNT            0x80830
#define HIA_RDR_0_PREP_PNTR             0x80834
#define HIA_RDR_0_PROC_PNTR             0x80838
#define HIA_RDR_0_STAT                  0x8083c
#define HIA_DFE_0_CFG                   0x8c000
#define HIA_DFE_0_THR_CTRL              0x8c040
#define HIA_DFE_0_OPTIONS               0x8c078
#define HIA_DFE_1_OPTIONS               0x8c0f8
#define HIA_DSE_0_CFG                   0x8d000
#define HIA_DSE_0_THR_CTRL              0x8d040
#define HIA_DSE_0_OPTIONS               0x8d078
#define HIA_DSE_1_OPTIONS               0x8d0f8
#define HIA_RA_PE_0_CTRL                0x90010
#define HIA_AIC_R1_OPTIONS              0x9d018
#define HIA_AIC_R0_ENABLE_CTRL          0x9e008
#define HIA_AIC_R0_ENABLED_STAT         0x9e010
#define HIA_AIC_R0_OPTIONS              0x9e018
#define HIA_AIC_G_OPTIONS               0x9f818
#define HIA_INLINE_CTRL_0               0x9ff80
#define HIA_OPTIONS2                    0x9fff0
#define HIA_MST_CTRL                    0x9fff4
#define HIA_OPTIONS                     0x9fff8
#define HIA_VERSION                     0x9fffc
#define PE_0_IN_DBUF_THRESH             0xa0000
#define PE_0_IN_TBUF_THRESH             0xa0100
#define PE_0_ICE_SCRATCH_RAM			0xa0800
#define PE_0_ICE_ADAPT_CTRL             0xa0c00
#define PE_0_ICE_PUE_CTRL               0xa0c80
#define PE_0_ICE_PUE_TIMEOUT_CTRL       0xa0c8c
#define PE_0_ICE_PUTF_CTRL              0xa0d00
#define PE_0_ICE_SCRATCH_CTRL           0xa0d04
#define PE_0_ICE_FPP_CTRL               0xa0d80
#define PE_0_ICE_FPP_TIMEOUT_CTRL       0xa0d8c
#define PE_0_ICE_PPTF_CTRL              0xa0e00
#define PE_0_ICE_RAM_CTRL               0xa0ff0
#define PE_0_ICE_OPTIONS                0xa0ff8
#define PE_0_EIP96_TOKEN_CTRL_STAT      0xa1000
#define PE_0_EIP96_CONTEXT_CTRL         0xa1008
#define PE_0_EIP96_OUT_BUF_CTRL         0xa101c
#define PE_0_EIP96_TOKEN_CTRL2          0xa102c
#define PE_0_EIP96_ECN_ERR_CFG          0xa13f0
#define PE_0_OCE_SCRATCH_RAM			0xa1400
#define PE_0_OCE_PUE_CTRL               0xa1880
#define PE_0_OCE_PUE_TIMEOUT_CTRL       0xa188c
#define PE_0_OCE_SCRATCH_CTRL           0xa1904
#define PE_0_OCE_FPP_CTRL               0xa1980
#define PE_0_OCE_FPP_TIMEOUT_CTRL       0xa198c
#define PE_0_OCE_RAM_CTRL               0xa1bf0
#define PE_0_OCE_OPTIONS                0xa1bf8
#define PE_0_OUT_DBUF_THRESH            0xa1c00
#define PE_0_AIC_ENABLE_CTRL            0xa1f08
#define PE_0_AIC_RAW_STAT               0xa1f0c
#define PE_0_AIC_OPTIONS                0xa1f18
#define PE_0_PE_OPTIONS                 0xa1ff8
#define PE_1_ICE_OPTIONS                0xa2ff8
#define TRC_0_PARAMS                    0xf0820
#define TRC_0_FREECHAIN                 0xf0824
#define TRC_0_PARAMS2                   0xf0828
#define FLUE_CACHEBASE_0_LO             0xf6000
#define FLUE_HASHBASE_0_LO              0xf6008
#define FLUE_CONFIG_0                   0xf6010
#define FLUE_OFFSETS                    0xf6808
#define FLUE_IFC_LUT_0                  0xf6820
#define FLUE_IFC_LUT_1                  0xf6824
#define FLUE_IFC_LUT_2                  0xf6828
#define FLUE_IFC_LUT_3                  0xf682c
#define DRBG_STATUS                     0xf7010
#define DRBG_CONTROL                    0xf7014
#define DRBG_GEN_BLK_SIZE               0xf702c
#define DRBG_PS_AI_0                    0xf7040
#define DRBG_PS_AI_1                    0xf7044
#define DRBG_PS_AI_2                    0xf7048
#define DRBG_PS_AI_3                    0xf704c
#define DRBG_PS_AI_4                    0xf7050
#define DRBG_PS_AI_5                    0xf7054
#define DRBG_PS_AI_6                    0xf7058
#define DRBG_PS_AI_7                    0xf705c
#define DRBG_PS_AI_8                    0xf7060
#define DRBG_PS_AI_9                    0xf7064
#define DRBG_PS_AI_10                   0xf7068
#define DRBG_PS_AI_11                   0xf706c
#define DRBG_OPTIONS                    0xf7078
#define EIP197_PIPE0_ICE_COHERENCE_CTRL 0xf7400
#define EIP197_PIPE0_OCE_COHERENCE_CTRL 0xf7404
#define EIP197_PIPE0_COHERENT_WITH      0xf7408
#define CS_RAM_CTRL                     0xf7ff0
#define EIP197_DBG_RING_IN_COUNT_LO_0   0xffb00
#define EIP197_DBG_RING_OUT_COUNT_LO_0  0xffb08
#define EIP197_DBG_PIPE_COUNT_0         0xffc00
#define EIP197_DBG_PIPE_STATE_0         0xffc04
#define EIP197_DBG_PIPE_DCOUNT_LO_0     0xffc08
#define EIP197_DBG_PIPE_DCOUNT_HI_0     0xffc0c
#define EIP197_OPTIONS                  0xffff8
#define IPUE_FW_STATUS					0x14
#define IFPP_FW_STATUS					0x18
#define OPUE_FW_STATUS					0x14
#define OFPP_FW_STATUS					0x18
#define ICE_SCRATCHPAD_RAM_WORD24		0x60
#define EIP_INLINE_IFC					(1 << 13)
#define EIP_LAFIFO_IFC					(1 << 12)

typedef struct {
	uint32_t protocol;
	uint32_t is_ipv6;
	uint32_t spi_esp_cpi_ipcomp;
	uint32_t sa_port;
	uint32_t dest_port;
	uint32_t sa_addr[4];
	uint32_t dest_addr[4];
} HashRecd_t;

typedef struct {
	// WORD0
	uint32_t ToP;
	uint32_t pkt_options;
	uint32_t cntx_size;
	uint32_t seq_append;
	uint32_t ext_ctx_format;
	uint32_t key;
	uint32_t crypto_algo;
	uint32_t digest_type;
	uint32_t hash_algo;
	uint32_t spi;
	uint32_t seq;
	uint32_t mask;

	// WORD1
	uint32_t crypto_mode;
	uint32_t feedback_mode;
	uint32_t iv;
	uint32_t digest_cnt;
	uint32_t iv_format;
	uint32_t crypto_store;
	uint32_t en_pre_pkt_op;
	uint32_t pad_type;
	uint32_t encrypt_hash_res;
	uint32_t MACsec_seq_num_chk;
	uint32_t hash_store;
	uint32_t crypto_algo_ext;
	uint32_t arc4_xts_state;
	uint32_t seq_num_option;
	uint32_t dis_mask_upd;
	uint32_t seq_num_ptr;
	uint32_t seq_num_ptr_av;
	uint32_t cntx_fetch_mode;

	uint32_t key_value[8];
	uint32_t spi_value;
	uint32_t seq_num[2];
} CntxCtrl_t;

typedef struct {
	uint32_t is_ipv6;
	uint32_t ip_protocol;
	uint32_t pkt_length;
	uint32_t src_port;
	uint32_t dest_port;
	uint8_t src_addr[16];
	uint8_t dest_addr[16];
} pktConfig_t;

#endif
