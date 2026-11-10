// SPDX-License-Identifier: GPL-2.0+
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <asm/io.h>
#include <asm/cache.h>
#include <cpu_func.h>
#include <time.h>
#include <vsprintf.h>
#include "eip197.h"

void gen_transform_record_egress(uint32_t transf_rec_ocm_addr, CntxCtrl_t cntx_ctrl) {
    uint32_t transf_record[80];
    uint32_t packetType = ETH_EIP_PACKET_TYPE;

    transf_record[0] =
        ((cntx_ctrl.ToP & 0xf) | ((cntx_ctrl.pkt_options & 0xf) << 4) | ((cntx_ctrl.cntx_size & 0x3f) << 8) |
         ((cntx_ctrl.seq_append & 0x1) << 14) | ((cntx_ctrl.ext_ctx_format & 0x1) << 15) |
         ((cntx_ctrl.key & 0x1) << 16) | ((cntx_ctrl.crypto_algo & 0xf) << 17) |
         ((cntx_ctrl.digest_type & 0x3) << 21) | ((cntx_ctrl.hash_algo & 0xf) << 23) |
         ((cntx_ctrl.spi & 0x1) << 27) | ((cntx_ctrl.seq & 0x3) << 28) | ((cntx_ctrl.mask & 0x3) << 30));

    transf_record[1] =
        ((cntx_ctrl.crypto_mode & 0x7) | ((cntx_ctrl.feedback_mode & 0x3) << 3) |
         ((cntx_ctrl.iv & 0xf) << 5) | ((cntx_ctrl.digest_cnt & 0x1) << 9) |
         ((cntx_ctrl.iv_format & 0x3) << 10) | ((cntx_ctrl.crypto_store & 0x1) << 12) |
         ((cntx_ctrl.en_pre_pkt_op & 0x1) << 13) | ((cntx_ctrl.pad_type & 0x7) << 14) |
         ((cntx_ctrl.encrypt_hash_res & 0x1) << 17) | ((cntx_ctrl.MACsec_seq_num_chk & 0x1) << 18) |
         ((cntx_ctrl.hash_store & 0x1) << 19) | ((cntx_ctrl.crypto_algo_ext & 0x1) << 20) |
         ((cntx_ctrl.arc4_xts_state & 0x1) << 21) | ((cntx_ctrl.seq_num_option & 0x1) << 22) |
         ((cntx_ctrl.dis_mask_upd & 0x1) << 23) | ((cntx_ctrl.seq_num_ptr & 0x3f) << 24) |
         ((cntx_ctrl.seq_num_ptr_av & 0x1) << 30) | ((cntx_ctrl.cntx_fetch_mode & 0x1) << 31));

    transf_record[2] = cntx_ctrl.key_value[0];
    transf_record[3] = cntx_ctrl.key_value[1];
    transf_record[4] = cntx_ctrl.key_value[2];
    transf_record[5] = cntx_ctrl.key_value[3];
    transf_record[6] = cntx_ctrl.key_value[4];
    transf_record[7] = cntx_ctrl.key_value[5];
    transf_record[8] = cntx_ctrl.spi_value;
    transf_record[9] = cntx_ctrl.seq_num[0];
    transf_record[10] = 0x0;
    transf_record[11] = 0x0;
    transf_record[12] = 0x0;
    transf_record[13] = 0x0;
    transf_record[14] = 0x0;
    transf_record[15] = 0x0;
    transf_record[16] = 0x0;
    transf_record[17] = 0x0;
    transf_record[18] = 0x0;
    transf_record[19] = 0x0;
    transf_record[20] = 0x0;
    transf_record[21] = 0x0;
    transf_record[22] = 0x0;
    transf_record[23] = 0x0;
    transf_record[24] = 0x0;
    transf_record[25] = 0x0;
    transf_record[26] = 0x0;
    transf_record[27] = 0x0;
    transf_record[28] = 0x0;
    transf_record[29] = 0x0;
    transf_record[30] = 0x0;
    transf_record[31] = 0x0;
    transf_record[32] = 0x0;
    transf_record[33] = 0x0;
    transf_record[34] = 0x0;
    transf_record[35] = 0x0;
    transf_record[36] = 0x0;
    transf_record[37] = 0x0;
    transf_record[38] = 0x0;
    transf_record[39] = 0x0;
    transf_record[40] = 0x0;
    transf_record[41] = 0x0;
    transf_record[42] = 0x0;
    transf_record[43] = 0x0;
    transf_record[44] = 0x0;
    transf_record[45] = 0x0;
    transf_record[46] = 0x0;
    transf_record[47] = 0x0;
    transf_record[48] = 0x04020000; // Token header

    if(packetType == IPV6_PACKET) {
        transf_record[49] = 0x0008f900; // IPHdr Proc, Destination Interface, Redirect Enable, IPv6
        transf_record[52] = 0x020b0008; //IPV6 Out transport
    }else if(packetType == IPV4_PACKET) {
        transf_record[49] = 0x0008f800; // IPHdr Proc, Destination Interface, Redirect Enable, IPv4
        transf_record[52] = 0x02050008; //IPV4 Out transport
    }
    
    transf_record[50] = 0x00000004;
    transf_record[51] = 0xd8cbb848;
   
    transf_record[53] = 0x94119411;
    transf_record[54] = 0xd0060000;
    transf_record[55] = 0xe1560809;
    transf_record[56] = 0x0;
    transf_record[57] = 0x00000019;
    transf_record[58] = 0x0;
    transf_record[59] = 0x0;
    transf_record[60] = 0x0;
    transf_record[61] = 0x0;
    transf_record[62] = 0x0;
    transf_record[63] = 0x0;
    transf_record[64] = 0x0;
    transf_record[65] = 0x0;
    transf_record[66] = 0x0;
    transf_record[67] = 0x0;
    transf_record[68] = 0x00000002;
    transf_record[69] = 0x802CD002;
    transf_record[70] = 0x0;
    transf_record[71] = 0x0;
    transf_record[72] = 0x0;
    transf_record[73] = 0x0;
    transf_record[74] = 0x0;
    transf_record[75] = 0x0;
    transf_record[76] = 0x0;
    transf_record[77] = 0x0;
    transf_record[78] = 0x0;
    transf_record[79] = 0x0;

    for (uint32_t i = 0; i < 80; i = i + 1) {
        writel(transf_record[i], transf_rec_ocm_addr + (i * 4));
    }
}

void gen_transform_record_ingress(uint32_t transf_rec_ocm_addr, CntxCtrl_t cntx_ctrl, uint8_t eip_rdr) {
    uint32_t transf_record[80];
    uint32_t packetType = ETH_EIP_PACKET_TYPE;

    transf_record[0] =
        ((cntx_ctrl.ToP & 0xf) | ((cntx_ctrl.pkt_options & 0xf) << 4) | ((cntx_ctrl.cntx_size & 0x3f) << 8) |
         ((cntx_ctrl.seq_append & 0x1) << 14) | ((cntx_ctrl.ext_ctx_format & 0x1) << 15) |
         ((cntx_ctrl.key & 0x1) << 16) | ((cntx_ctrl.crypto_algo & 0xf) << 17) |
         ((cntx_ctrl.digest_type & 0x3) << 21) | ((cntx_ctrl.hash_algo & 0xf) << 23) |
         ((cntx_ctrl.spi & 0x1) << 27) | ((cntx_ctrl.seq & 0x3) << 28) | ((cntx_ctrl.mask & 0x3) << 30));

    transf_record[1] =
        ((cntx_ctrl.crypto_mode & 0x7) | ((cntx_ctrl.feedback_mode & 0x3) << 3) |
         ((cntx_ctrl.iv & 0xf) << 5) | ((cntx_ctrl.digest_cnt & 0x1) << 9) |
         ((cntx_ctrl.iv_format & 0x3) << 10) | ((cntx_ctrl.crypto_store & 0x1) << 12) |
         ((cntx_ctrl.en_pre_pkt_op & 0x1) << 13) | ((cntx_ctrl.pad_type & 0x7) << 14) |
         ((cntx_ctrl.encrypt_hash_res & 0x1) << 17) | ((cntx_ctrl.MACsec_seq_num_chk & 0x1) << 18) |
         ((cntx_ctrl.hash_store & 0x1) << 19) | ((cntx_ctrl.crypto_algo_ext & 0x1) << 20) |
         ((cntx_ctrl.arc4_xts_state & 0x1) << 21) | ((cntx_ctrl.seq_num_option & 0x1) << 22) |
         ((cntx_ctrl.dis_mask_upd & 0x1) << 23) | ((cntx_ctrl.seq_num_ptr & 0x3f) << 24) |
         ((cntx_ctrl.seq_num_ptr_av & 0x1) << 30) | ((cntx_ctrl.cntx_fetch_mode & 0x1) << 31));

    transf_record[2] = cntx_ctrl.key_value[0];
    transf_record[3] = cntx_ctrl.key_value[1];
    transf_record[4] = cntx_ctrl.key_value[2];
    transf_record[5] = cntx_ctrl.key_value[3];
    transf_record[6] = cntx_ctrl.key_value[4];
    transf_record[7] = cntx_ctrl.key_value[5];
    transf_record[8] = cntx_ctrl.spi_value;
    transf_record[9] = cntx_ctrl.seq_num[0];
    transf_record[10] = 0x0;
    transf_record[11] = 0x0;
    transf_record[12] = 0x0;
    transf_record[13] = 0x0;
    transf_record[14] = 0x0;
    transf_record[15] = 0x0;
    transf_record[16] = 0x0;
    transf_record[17] = 0x0;
    transf_record[18] = 0x0;
    transf_record[19] = 0x0;
    transf_record[20] = 0x0;
    transf_record[21] = 0x0;
    transf_record[22] = 0x0;
    transf_record[23] = 0x0;
    transf_record[24] = 0x0;
    transf_record[25] = 0x0;
    transf_record[26] = 0x0;
    transf_record[27] = 0x0;
    transf_record[28] = 0x0;
    transf_record[29] = 0x0;
    transf_record[30] = 0x0;
    transf_record[31] = 0x0;
    transf_record[32] = 0x0;
    transf_record[33] = 0x0;
    transf_record[34] = 0x00000001;
    transf_record[35] = 0x0;
    transf_record[36] = 0x0;
    transf_record[37] = 0x0;
    transf_record[38] = 0x0;
    transf_record[39] = 0x0;
    transf_record[40] = 0x0;
    transf_record[41] = 0x0;
    transf_record[42] = 0x0;
    transf_record[43] = 0x0;
    transf_record[44] = 0x0;
    transf_record[45] = 0x0;
    transf_record[46] = 0x0;
    transf_record[47] = 0x0;
    transf_record[48] = 0x01420000; // Token header

    if(packetType == IPV6_PACKET) {
        transf_record[49] = 0x00080900 | (eip_rdr << 12); // IPHdr Proc, Destination Interface, Redirect Enable, IPv6
        transf_record[52] = 0x080c0008; //IPV6 In transport
    }else if(packetType == IPV4_PACKET) {
        transf_record[49] = 0x00080800 | (eip_rdr << 12); // IPHdr Proc, Destination Interface, Redirect Enable, IPv4
        transf_record[52] = 0x08060008; //IPV4 In transport
    }
    
    transf_record[50] = 0x00000004;
    transf_record[51] = 0xf56cdc5d;
    
    transf_record[53] = 0x94119411;
    transf_record[54] = 0xdd060000;
    transf_record[55] = 0xe156003f;
    transf_record[56] = 0x0;
    transf_record[57] = 0x0;
    transf_record[58] = 0x0;
    transf_record[59] = 0x0;
    transf_record[60] = 0x0;
    transf_record[61] = 0x0;
    transf_record[62] = 0x0;
    transf_record[63] = 0x0;
    transf_record[64] = 0x0;
    transf_record[65] = 0x0;
    transf_record[66] = 0x0;
    transf_record[67] = 0x0;
    transf_record[68] = 0x00000002;
    transf_record[69] = 0x802ED002;
    transf_record[70] = 0x0;
    transf_record[71] = 0x0;
    transf_record[72] = 0x0;
    transf_record[73] = 0x00000002;
    transf_record[74] = 0x0;
    transf_record[75] = 0x0;
    transf_record[76] = 0x0;
    transf_record[77] = 0x0;
    transf_record[78] = 0x0;
    transf_record[79] = 0x0;

    for (uint32_t i = 0; i < 80; i = i + 1) {
        writel(transf_record[i], transf_rec_ocm_addr + (i * 4));
    }
}

/*
Egress and Igress Condition
| Field                                         | Role                                            | Must Match?                                    | Notes                                                               |   |
| --------------------------------------------- | ----------------------------------------------- | ---------------------------------------------- | ------------------------------------------------------------------- | - |
| `key_value[]`                                 | **Encryption key**                              | ✅ REQUIRED                                     | Egress uses it to encrypt, ingress uses it to decrypt — must match. |   |
| `crypto_algo`, `crypto_mode`, `feedback_mode` | **Algorithm + mode (AES, GCM, CBC, CTR, etc.)** | ✅ REQUIRED                                     | If different → decrypted data will be garbage.                      |   |
| `digest_type`, `hash_algo`                    | **Hash algorithm (HMAC-SHA256, SHA512, etc.)**  | ✅ REQUIRED                                     | Used to verify the Integrity Tag.                                   |   |
| `spi_value`                                   | **Security Parameter Index (SPI)**              | ✅ REQUIRED                                     | Identifies the SA (Security Association).                           |   |
| `seq_num[]`                                   | **Sequence Number**                             | ⚠️ May differ (TX increases, RX checks replay) |                                                                     |   |
| `ToP`, `pkt_options`, `cntx_size`             | **Common control bits**                         | ✅ Should match                                 | Ensures consistent interpretation of the context.                   |   |
| `iv_format`, `iv`                             | **Initialization Vector format**                | ✅                                              | Ensures counter or nonce consistency between TX/RX.                 | _ |
*/
void eth_init_transform_record_egress(CntxCtrl_t cntx_ctrl_out){
    uint32_t read_data;
    // Transform Record Generation
	/*
	*   ToP: 
    *	0010 - Outbound hash operation (for Null-crypto mode)
    *	0011 - Inbound hash operation (for Null-crypto mode)
    *	0100 - Outbound encrypt operation
    *	0101 - Inbound decrypt operation
    *	0110 - Outbound encrypt-then-hash operation for AES-GCM
    *	0111 - Inbound decrypt-hash operation (for all other cipher algorithms)
    * 	1110 - Outbound hash-then-encrypt operation for AES-GMAC
    *	1111 - Inbound hash-then-decrypt operation for both GCM and GMAC
	*/
	cntx_ctrl_out.ToP = 4;			//Outbound encrypt operation
	cntx_ctrl_out.pkt_options = 0;	// Default
	cntx_ctrl_out.cntx_size = 8;	//Context Size ( - 2 first words)
	cntx_ctrl_out.seq_append = 0;	
	cntx_ctrl_out.ext_ctx_format = 0;	// =1 if use an external context format
	cntx_ctrl_out.key = 1;
	cntx_ctrl_out.crypto_algo = 2; // Crypto algorithm: 3-DES | Should try AES
	cntx_ctrl_out.digest_type = 0;	//Basic encrypt: use 0 //Basic hash (MD5, SHA-1, SHA-21, SHA-3, SM3)
	cntx_ctrl_out.hash_algo = 0;	//Basic encrypt: use 0 //MD5 or CRC-32
	cntx_ctrl_out.spi = 1;
	cntx_ctrl_out.seq = 1;
	cntx_ctrl_out.mask = 0;
	
	cntx_ctrl_out.crypto_mode = 1; // 3-DES (CBC) //CBC mode (in combination with (3)DES, SM4, BC0 and AES)
    cntx_ctrl_out.feedback_mode = 0;
    cntx_ctrl_out.iv = 0;
    cntx_ctrl_out.digest_cnt = 0; //digest counter is not used
    cntx_ctrl_out.iv_format = 0;
    cntx_ctrl_out.crypto_store = 0;
    cntx_ctrl_out.en_pre_pkt_op = 0;
    cntx_ctrl_out.pad_type = 0;
    cntx_ctrl_out.encrypt_hash_res = 0;
    cntx_ctrl_out.MACsec_seq_num_chk = 0;
    cntx_ctrl_out.hash_store = 0;
    cntx_ctrl_out.crypto_algo_ext = 0;
    cntx_ctrl_out.arc4_xts_state = 0;
    cntx_ctrl_out.seq_num_option = 1;
    cntx_ctrl_out.dis_mask_upd = 0;
    cntx_ctrl_out.seq_num_ptr = 9; //seq_num is at word 9
    cntx_ctrl_out.seq_num_ptr_av = 1;
    cntx_ctrl_out.cntx_fetch_mode = 0;
    cntx_ctrl_out.key_value[0] = 0x11223344;
    cntx_ctrl_out.key_value[1] = 0x55667788;
    cntx_ctrl_out.key_value[2] = 0x99AABBCC;
    cntx_ctrl_out.key_value[3] = 0xDDEEFF00;
    cntx_ctrl_out.key_value[4] = 0x11223344;
    cntx_ctrl_out.key_value[5] = 0x55667788;
    cntx_ctrl_out.key_value[6] = 0x99AABBCC;
    cntx_ctrl_out.key_value[7] = 0xDDEEFF00;
    cntx_ctrl_out.spi_value = 0xAABBCCDD;
    cntx_ctrl_out.seq_num[0] = 0;
    gen_transform_record_egress(CONTEXTBUF_COMMAND_BASE_ADDR, cntx_ctrl_out);
    printf("Egress Txfrm Record Gen Done \n");

    printf("Egress Txfrm Record Data \n");

    for(int txr_cnt0=0; txr_cnt0<80; txr_cnt0++) {
        read_data = readl(CONTEXTBUF_COMMAND_BASE_ADDR + (txr_cnt0*4));
        //printf("Egress Txfrm Record Data: %x\n", read_data);
    }
}

void eth_init_transform_record_ingress(CntxCtrl_t cntx_ctrl_in, uint8_t eip_rdr) {
    // Transform Record Generation
    cntx_ctrl_in.ToP = 5;
    cntx_ctrl_in.pkt_options = 0;
    cntx_ctrl_in.cntx_size = 8;
    cntx_ctrl_in.seq_append = 0;
    cntx_ctrl_in.ext_ctx_format = 0;
    cntx_ctrl_in.key = 1;
    cntx_ctrl_in.crypto_algo = 2; // 3-DES
    cntx_ctrl_in.digest_type = 0;
    cntx_ctrl_in.hash_algo = 0;
    cntx_ctrl_in.spi = 1;
    cntx_ctrl_in.seq = 0;
    cntx_ctrl_in.mask = 1;
    cntx_ctrl_in.crypto_mode = 1; // 3-DES (CBC)
    cntx_ctrl_in.feedback_mode = 0;
    cntx_ctrl_in.iv = 0;
    cntx_ctrl_in.digest_cnt = 0;
    cntx_ctrl_in.iv_format = 0;
    cntx_ctrl_in.crypto_store = 0;
    cntx_ctrl_in.en_pre_pkt_op = 0;
    cntx_ctrl_in.pad_type = 4;
    cntx_ctrl_in.encrypt_hash_res = 0;
    cntx_ctrl_in.MACsec_seq_num_chk = 0;
    cntx_ctrl_in.hash_store = 0;
    cntx_ctrl_in.crypto_algo_ext = 0;
    cntx_ctrl_in.arc4_xts_state = 0;
    cntx_ctrl_in.seq_num_option = 0;
    cntx_ctrl_in.dis_mask_upd = 0;
    cntx_ctrl_in.seq_num_ptr = 0;
    cntx_ctrl_in.seq_num_ptr_av = 0;
    cntx_ctrl_in.cntx_fetch_mode = 0;
    cntx_ctrl_in.key_value[0] = 0x11223344;
    cntx_ctrl_in.key_value[1] = 0x55667788;
    cntx_ctrl_in.key_value[2] = 0x99AABBCC;
    cntx_ctrl_in.key_value[3] = 0xDDEEFF00;
    cntx_ctrl_in.key_value[4] = 0x11223344;
    cntx_ctrl_in.key_value[5] = 0x55667788;
    cntx_ctrl_in.key_value[6] = 0x99AABBCC;
    cntx_ctrl_in.key_value[7] = 0xDDEEFF00;
    cntx_ctrl_in.spi_value = 0xAABBCCDD;
    cntx_ctrl_in.seq_num[0] = 0;
    gen_transform_record_ingress((CONTEXTBUF_COMMAND_BASE_ADDR + CONTEXTBUF_IGRS_OFFSET), cntx_ctrl_in, eip_rdr);
    printf("Ingress Txfrm Record Desc Gen Done\n");
/* 
    printf("Ingress Txfrm Record Data \n");
    
    uint32_t read_data;
    for(int itxr_cnt0=0; itxr_cnt0<80; itxr_cnt0++) {
        read_data = readl(CONTEXTBUF_COMMAND_BASE_ADDR+CONTEXTBUF_IGRS_OFFSET + (itxr_cnt0*4));
        //printf("Ingress Txfrm Record Data: %x\n", read_data);
    }
     */
}

static uint32_t hash[4];
void hash_compute(uint32_t iv[4], uint32_t hash_rec[13]) {
    hash[0] = iv[0];
    hash[1] = iv[1];
    hash[2] = iv[2];
    hash[3] = iv[3];

    for (int i = 0; i < 13; i = i + 1) {
        hash[0] = hash[0] + hash_rec[i];
        hash[0] = (hash[0] + (hash[0] << 10));
        hash[0] = (hash[0] ^ (hash[0] >> 6));

        hash[1 + (i % 3)] = hash[1 + (i % 3)] ^ hash_rec[i];

        if (((i % 3) == 2) && i < 12) {
            hash[1] = hash[1] - hash[2];
            hash[1] = hash[1] - hash[3];
            hash[1] = (hash[1] ^ (hash[3] >> 13));

            hash[2] = hash[2] - hash[3];
            hash[2] = hash[2] - hash[1];
            hash[2] = (hash[2] ^ (hash[1] << 8));

            hash[3] = hash[3] - hash[1];
            hash[3] = hash[3] - hash[2];
            hash[3] = (hash[3] ^ (hash[2] >> 13));

            hash[1] = hash[1] - hash[2];
            hash[1] = hash[1] - hash[3];
            hash[1] = (hash[1] ^ (hash[3] >> 12));

            hash[2] = hash[2] - hash[3];
            hash[2] = hash[2] - hash[1];
            hash[2] = (hash[2] ^ (hash[1] << 16));

            hash[3] = hash[3] - hash[1];
            hash[3] = hash[3] - hash[2];
            hash[3] = (hash[3] ^ (hash[2] >> 5));

            hash[1] = hash[1] - hash[2];
            hash[1] = hash[1] - hash[3];
            hash[1] = (hash[1] ^ (hash[3] >> 3));

            hash[2] = hash[2] - hash[3];
            hash[2] = hash[2] - hash[1];
            hash[2] = (hash[2] ^ (hash[1] << 10));

            hash[3] = hash[3] - hash[1];
            hash[3] = hash[3] - hash[2];
            hash[3] = (hash[3] ^ (hash[2] >> 15));
        }
    }
}

void gen_lookup_hash_record(uint32_t hash_rec_ocm_addr, HashRecd_t hash_recd, uint32_t egress,
                            uint32_t iv_value[4]) {
    uint32_t lookup_hash_table[13];
    uint32_t hash_bucket[16];

    lookup_hash_table[0] = 0x0; // Reserved
    lookup_hash_table[1] = ((hash_recd.is_ipv6 & 1) << 25) | ((hash_recd.protocol & 0xff) << 8); // IPv6, Next Protocl field
    lookup_hash_table[2] = hash_recd.spi_esp_cpi_ipcomp; // SPI(ESP)/CPI(IPcomp)/0 (DTLS)
    lookup_hash_table[3] = 0x0;
    lookup_hash_table[4] = ((hash_recd.dest_port & 0xffff) << 16) | (hash_recd.sa_port & 0xffff);
    lookup_hash_table[5] = hash_recd.dest_addr[0];
    lookup_hash_table[6] = hash_recd.dest_addr[1];
    lookup_hash_table[7] = hash_recd.dest_addr[2];
    lookup_hash_table[8] = hash_recd.dest_addr[3];
    lookup_hash_table[9] = hash_recd.sa_addr[0];
    lookup_hash_table[10] = hash_recd.sa_addr[1];
    lookup_hash_table[11] = hash_recd.sa_addr[2];
    lookup_hash_table[12] = hash_recd.sa_addr[3];
    
    hash_compute(iv_value, lookup_hash_table);

    //The 64-Byte ‘hash bucket’ as read from the hash table contains the following information:
    hash_bucket[0] = 0x0;
    hash_bucket[1] = 0x0;
    hash_bucket[2] = 0x0;
    hash_bucket[3] = 0x0;
    hash_bucket[4] = hash[0];
    hash_bucket[5] = hash[1];
    hash_bucket[6] = hash[2];
    hash_bucket[7] = hash[3];
    hash_bucket[8] = 0x0;
    hash_bucket[9] = 0x0;
    hash_bucket[10] = 0x0;
    hash_bucket[11] = 0x0;
    hash_bucket[12] = 0x0;
    if (egress)
        hash_bucket[13] = 0x2; //Use small transform record
    else
        hash_bucket[13] = 0x2 | CONTEXTBUF_IGRS_OFFSET; //offset to second flow/transform record
    hash_bucket[14] = 0x0;
    hash_bucket[15] = (hash[0] & 0x3fc0); //offset to an ‘overflow hash bucket’. // max 1 bucket = 4kb ??
/* 
    for (uint32_t i = 0; i < 16; i = i + 1) {
        printf("hash_bucket[%d]= %x\n", i, hash_bucket[i]);
    }
     */
    for (uint32_t i = 0; i < 16; i = i + 1) {
        writel(hash_bucket[i], ((hash_rec_ocm_addr + (hash[0] & 0xffc0))) + (i * 4));
    }
}

void eth_init_lookup_hash_record_egress(HashRecd_t *hash_rec){
    uint32_t iv[4];
    uint32_t read_data;
    uint32_t packetType = ETH_EIP_PACKET_TYPE;

    if(packetType == IPV6_PACKET) {
        hash_rec->is_ipv6 = 1;
        hash_rec->dest_addr[3] = 0x05555555;
        hash_rec->dest_addr[2] = 0x66666666;
        hash_rec->dest_addr[1] = 0x77777777;
        hash_rec->dest_addr[0] = 0x88888880;

        hash_rec->sa_addr[3] = 0x01111111;
        hash_rec->sa_addr[2] = 0x22222222;
        hash_rec->sa_addr[1] = 0x33333333;
        hash_rec->sa_addr[0] = 0x44444440;
    }else if(packetType == IPV4_PACKET) {
        hash_rec->is_ipv6 = 0;
        hash_rec->dest_addr[3] = 0;
        hash_rec->dest_addr[2] = 0;
        hash_rec->dest_addr[1] = 0;
        hash_rec->dest_addr[0] = 0x88888880;
        
        hash_rec->sa_addr[3] = 0;
        hash_rec->sa_addr[2] = 0;
        hash_rec->sa_addr[1] = 0;
        hash_rec->sa_addr[0] = 0x44444440;
    }
    
    hash_rec->protocol = PROTO_UDP;
    hash_rec->spi_esp_cpi_ipcomp = 0;
    hash_rec->sa_port = UDP_SRC_PORT;
    hash_rec->dest_port = UDP_DST_PORT;

    iv[0] = IV_0;
    iv[1] = IV_1;
    iv[2] = IV_2;
    iv[3] = IV_3;

    gen_lookup_hash_record(LOOKUP_HASH_BASE, *hash_rec, 1, iv);
}

void eth_init_lookup_hash_record_ingress(HashRecd_t *hash_rec){
    uint32_t iv[4];
    uint32_t read_data;
    uint32_t packetType = ETH_EIP_PACKET_TYPE;
    
    iv[0] = IV_0;
    iv[1] = IV_1;
    iv[2] = IV_2;
    iv[3] = IV_3;

    if(packetType == IPV6_PACKET) {
        hash_rec->is_ipv6 = 1;
        hash_rec->dest_addr[3] = 0x05555555;
        hash_rec->dest_addr[2] = 0x66666666;
        hash_rec->dest_addr[1] = 0x77777777;
        hash_rec->dest_addr[0] = 0x88888880;
        
    }else if(packetType == IPV4_PACKET) {
        hash_rec->is_ipv6 = 0;
        hash_rec->dest_addr[3] = 0;
        hash_rec->dest_addr[2] = 0;
        hash_rec->dest_addr[1] = 0;
        hash_rec->dest_addr[0] = 0x88888880;
    }
    // Lookup Hash Record Generation
    hash_rec->protocol = PROTO_ESP;
    hash_rec->spi_esp_cpi_ipcomp = 0xAABBCCDD;
    hash_rec->sa_port = 0;
    hash_rec->dest_port = 0;
    hash_rec->sa_addr[3] = 0;
    hash_rec->sa_addr[2] = 0;
    hash_rec->sa_addr[1] = 0;
    hash_rec->sa_addr[0] = 0;
    gen_lookup_hash_record(LOOKUP_HASH_BASE, *hash_rec, 0, iv);
}

void gen_command_desc(uint32_t cmd_desc_ocm_addr, uint32_t packet_addr, uint32_t particle_size,
                      uint32_t first_seg, uint32_t last_seg, uint32_t tdest) {
    uint32_t cmd_desc[16];

    cmd_desc[0] = ((first_seg & 1) << 23) | ((last_seg & 1) << 22) | (particle_size & 0xffff);
    cmd_desc[1] = 0;
    cmd_desc[2] = packet_addr; //egress address
    cmd_desc[3] = 0;
    cmd_desc[4] = 0;
    cmd_desc[5] = 0;
    cmd_desc[6] = ((0xf000) << 16) | (particle_size & 0xffff);
    cmd_desc[7] = ((tdest & 0x3f) << 9);

    cmd_desc[8] = 0xfffffffc;
    cmd_desc[9] = 0xffffffff;

    cmd_desc[10] = 0x02000000; //HW Service: LIP, IIP
    
    cmd_desc[11] = 0x00000e00;
    cmd_desc[12] = 0;
    cmd_desc[13] = 0;
    cmd_desc[14] = 0;
    cmd_desc[15] = 0;

    for (uint32_t i = 0; i < 16; i = i + 1) {
        writel(cmd_desc[i], cmd_desc_ocm_addr + (i * 4));
        mdelay(10);
    }
}

void gen_result_desc(uint32_t num_desc, uint32_t result_desc_ocm_addr, uint32_t first_seg, uint32_t last_seg,
                     uint32_t payload_addr, uint32_t payload_length) {
    uint32_t result_desc[(num_desc * 16)];
    for (uint32_t desc_no = 0; desc_no < num_desc; desc_no = desc_no + 1) {
        result_desc[((desc_no * 16) + 0)] =
            ((first_seg & 1) << 23) | ((last_seg & 1) << 22) | (payload_length & 0xffff);
        result_desc[((desc_no * 16) + 1)] = (1 << 24);
        result_desc[((desc_no * 16) + 2)] = (payload_addr + (desc_no * payload_length));
        result_desc[((desc_no * 16) + 3)] = 0;
        result_desc[((desc_no * 16) + 4)] = 0;
        result_desc[((desc_no * 16) + 5)] = 0;
        result_desc[((desc_no * 16) + 6)] = 0;
        result_desc[((desc_no * 16) + 7)] = 0;
        result_desc[((desc_no * 16) + 8)] = 0;
        result_desc[((desc_no * 16) + 9)] = 0;
        result_desc[((desc_no * 16) + 10)] = 0;
        result_desc[((desc_no * 16) + 11)] = 0;
        result_desc[((desc_no * 16) + 12)] = 0;
        result_desc[((desc_no * 16) + 13)] = 0;
        result_desc[((desc_no * 16) + 14)] = 0;
        result_desc[((desc_no * 16) + 15)] = 0;
    }

    for (uint32_t i = 0; i < (num_desc * 16); i = i + 1) {
        writel(result_desc[i], result_desc_ocm_addr + (i * 4));
    }
}

void eth_gen_result_descriptor(uint32_t b_len){
    //gen_result_desc(10, RDR_RING_BASE_ADDR, 1, 1, IGRS_PACKET_BASE_ADDR, 0x1000);
    gen_result_desc(b_len, RDR_RING_BASE_ADDR, 1, 1, IGRS_PACKET_BASE_ADDR, 0x1000);
	printf("Result Gen Done \n");
    uint32_t read_data;
	//printf("Result Desc Data \n");
	for(int rdr_cnt=0; rdr_cnt<32; rdr_cnt++) {
		read_data = readl(RDR_RING_BASE_ADDR + (rdr_cnt*4));
		//printf("Result Desc Data: %x\n", read_data);
	}
}

void gen_egress_pkt(uint32_t pkt_ocm_addr, pktConfig_t pktcfg, uint32_t ind) {
    static uint8_t eip_data[EIP197_PKT_BUF_SIZE];
    uint32_t ip_pkt_length;
    uint32_t udp_pkt_length;
    uint32_t eip_data_word;

    ip_pkt_length = pktcfg.pkt_length - 14; // Subtract Ethernet header size
    udp_pkt_length = ip_pkt_length - 40; // Subtract IP header size

    // Ethernet Header Generation
    for (int i = 0; i < 14; i = i + 1) {
        eip_data[i] = i;
    }

    // IPv6 Header Generation
    eip_data[14] = 0x60; // IP packet version is IPv6, Traffic Class[3:0]
    eip_data[15] = 0x00; // Flow Table[3:0], Traffic Class[7:4]
    eip_data[16] = 0x00; // Flow Table [11:4]
    eip_data[17] = 0x00; // Flow Table [19:12]
    eip_data[18] = ((udp_pkt_length & 0xff00) >> 8); // Payload Length [15:8]
    eip_data[19] = (udp_pkt_length & 0xff); // Payload Length [7:0]
    eip_data[20] = pktcfg.ip_protocol; // Next Header 6-TCP, 17-UDP,
    // 41-Encapsulated IPv6 Header, 43-Routing
    // Header, 44-Fragment Header,
    // 51-Authentication Header, 58-ICMPv6,
    // 59-No next Header
    eip_data[21] = 0x80; // Hop Limit

    for (int i = 0; i < 16; i = i + 1) {
        eip_data[22 + i] = pktcfg.src_addr[i];
        eip_data[38 + i] = pktcfg.dest_addr[i];
    }

    eip_data[54] = ((pktcfg.src_port & 0xff00) >> 8);
    eip_data[55] = pktcfg.src_port & 0xff;
    eip_data[56] = ((pktcfg.dest_port & 0xff00) >> 8);
    eip_data[57] = pktcfg.dest_port & 0xff;
    eip_data[58] = ((udp_pkt_length & 0xff00) >> 8);
    eip_data[59] = udp_pkt_length & 0xff;
    eip_data[60] = 0x0; // udp_checksum
    eip_data[61] = 0x0; // udp_checksum

    for (uint32_t i = 0; i < (udp_pkt_length - 8); i = i + 1) {
        eip_data[62 + i] = i + ind;
    }

    for (uint32_t i = 0; i < pktcfg.pkt_length; i = i + 4) {
        eip_data_word =
            (eip_data[i] | (eip_data[i + 1] << 8) | (eip_data[i + 2] << 16) | (eip_data[i + 3] << 24));
        writel(eip_data_word, pkt_ocm_addr + i);
    }
}

void gen_egress_pkt_ipv4(uint32_t pkt_ocm_addr, pktConfig_t pktcfg, uint32_t ind) {
    static uint8_t eip_data[EIP197_PKT_BUF_SIZE];
    uint32_t ip_pkt_length;
    uint32_t udp_pkt_length;
    uint32_t eip_data_word;

    ip_pkt_length = pktcfg.pkt_length - 14; // Subtract Ethernet header size
    udp_pkt_length = ip_pkt_length - 20;    // Subtract IPv4 header size (20 bytes)

    // 1. Ethernet Header Generation (14 bytes)
    for (int i = 0; i < 12; i = i + 1) {
        eip_data[i] = i; // Dummy MAC addresses
    }
    eip_data[12] = 0x08; // EtherType: IPv4 (0x0800)
    eip_data[13] = 0x00;

    // 2. IPv4 Header Generation (20 bytes)
    eip_data[14] = 0x45; // Version (4) and IHL (5 words = 20 bytes)
    eip_data[15] = 0x00; // Differentiated Services / TOS
    eip_data[16] = ((ip_pkt_length & 0xff00) >> 8); // Total Length
    eip_data[17] = (ip_pkt_length & 0xff);
    eip_data[18] = 0x00; // Identification
    eip_data[19] = 0x00;
    eip_data[20] = 0x40; // Flags (Don't Fragment) & Fragment Offset
    eip_data[21] = 0x00;
    eip_data[22] = 0x40; // Time to Live (TTL = 64)
    eip_data[23] = pktcfg.ip_protocol; // Protocol (e.g., 17 for UDP)
    eip_data[24] = 0x00; // Header Checksum (initialized to 0, calculated later)
    eip_data[25] = 0x00;

    // Source IP Address (4 bytes)
    for (int i = 0; i < 4; i = i + 1) {
        eip_data[26 + i] = pktcfg.src_addr[i];
    }

    // Destination IP Address (4 bytes)
    for (int i = 0; i < 4; i = i + 1) {
        eip_data[30 + i] = pktcfg.dest_addr[i];
    }

    // 3. IPv4 Header Checksum Calculation
    uint32_t checksum = 0;
    for (int i = 14; i < 34; i += 2) {
        checksum += (eip_data[i] << 8) | eip_data[i + 1];
    }
    while (checksum >> 16) {
        checksum = (checksum & 0xFFFF) + (checksum >> 16);
    }
    checksum = ~checksum;
    eip_data[24] = (checksum >> 8) & 0xFF;
    eip_data[25] = checksum & 0xFF;

    // 4. UDP Header Generation (8 bytes)
    eip_data[34] = ((pktcfg.src_port & 0xff00) >> 8);
    eip_data[35] = pktcfg.src_port & 0xff;
    eip_data[36] = ((pktcfg.dest_port & 0xff00) >> 8);
    eip_data[37] = pktcfg.dest_port & 0xff;
    eip_data[38] = ((udp_pkt_length & 0xff00) >> 8);
    eip_data[39] = udp_pkt_length & 0xff;
    eip_data[40] = 0x00; // UDP Checksum (Optional in IPv4, 0x0000 means unused)
    eip_data[41] = 0x00;

    // 5. Payload Generation
    for (uint32_t i = 0; i < (udp_pkt_length - 8); i = i + 1) {
        eip_data[42 + i] = i + ind;
    }

    // 6. Write Data to OCM memory
    for (uint32_t i = 0; i < pktcfg.pkt_length; i = i + 4) {
        eip_data_word =
            (eip_data[i] | (eip_data[i + 1] << 8) | (eip_data[i + 2] << 16) | (eip_data[i + 3] << 24));
        writel(eip_data_word, pkt_ocm_addr + i);
    }
}

void eth_setup_egress_packet(pktConfig_t *pkt_cfg, uint32_t pkt_len){
    uint32_t read_data;
    uint32_t packetType = ETH_EIP_PACKET_TYPE;
    // Egress Packet Generation
    if(packetType == IPV6_PACKET) {
        pkt_cfg->is_ipv6 = 1;
    }else if(packetType == IPV4_PACKET) {
        pkt_cfg->is_ipv6 = 0;
    }
    pkt_cfg->ip_protocol = PROTO_UDP;
    pkt_cfg->pkt_length = pkt_len;
    pkt_cfg->src_port = UDP_SRC_PORT;
    pkt_cfg->dest_port = UDP_DST_PORT;

    pkt_cfg->src_addr[15] = 0x01;
    pkt_cfg->src_addr[14] = 0x11;
    pkt_cfg->src_addr[13] = 0x11;
    pkt_cfg->src_addr[12] = 0x11;
    pkt_cfg->src_addr[11] = 0x22;
    pkt_cfg->src_addr[10] = 0x22;
    pkt_cfg->src_addr[9]  = 0x22;
    pkt_cfg->src_addr[8]  = 0x22;
    pkt_cfg->src_addr[7]  = 0x33;
    pkt_cfg->src_addr[6]  = 0x33;
    pkt_cfg->src_addr[5]  = 0x33;
    pkt_cfg->src_addr[4]  = 0x33;
    pkt_cfg->src_addr[3]  = 0x44;
    pkt_cfg->src_addr[2]  = 0x44;
    pkt_cfg->src_addr[1]  = 0x44;
    pkt_cfg->src_addr[0]  = 0x40;

    pkt_cfg->dest_addr[15] = 0x05;
    pkt_cfg->dest_addr[14] = 0x55;
    pkt_cfg->dest_addr[13] = 0x55;
    pkt_cfg->dest_addr[12] = 0x55;
    pkt_cfg->dest_addr[11] = 0x66;
    pkt_cfg->dest_addr[10] = 0x66;
    pkt_cfg->dest_addr[9]  = 0x66;
    pkt_cfg->dest_addr[8]  = 0x66;
    pkt_cfg->dest_addr[7]  = 0x77;
    pkt_cfg->dest_addr[6]  = 0x77;
    pkt_cfg->dest_addr[5]  = 0x77;
    pkt_cfg->dest_addr[4]  = 0x77;
    pkt_cfg->dest_addr[3]  = 0x88;
    pkt_cfg->dest_addr[2]  = 0x88;
    pkt_cfg->dest_addr[1]  = 0x88;
    pkt_cfg->dest_addr[0]  = 0x80;
}

