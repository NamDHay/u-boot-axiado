// SPDX-License-Identifier: GPL-2.0+
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <asm/io.h>
#include <asm/cache.h>
#include <cpu_func.h>
#include <time.h>
#include <vsprintf.h>
#include <console.h>
#include <linux/math64.h>
#include "eip197.h"

void eth_print_eip_options_info(void){
    uint32_t read_data;
/*
 * EIP197_OPTIONS = 0xB7CBB948
 * -----------------------------
 * Bit31 TRC              = 1 → Transform Record Cache present
 * Bit30 FRC              = 0 → Flow Record Cache absent
 * Bit29 LOOKUP           = 1 → Lookup engine enabled
 * Bit28 VIRTUALIZATION   = 1 → Virtualization supported
 * Bit27 HW_TBUILDER      = 0 → No hardware token builder
 * Bit26 CENTRAL_PRNG     = 1 → Central PRNG available
 * Bit25 INPUT_CLASSIFIER = 1 → ICE (Ingress Classifier) present
 * Bit24 OUTPUT_CLASSIFIER= 1 → OCE (Egress Classifier) present
 * Bit23–20 INT_DBUF_SIZE = 0xC → 16 KB Intermediate buffer per PE
 * Bit19–16 OUT_DBUF_SIZE = 0xB → 8 KB Output buffer per PE | opkt_size_log2
 * Bit15–13 OUT_TBUF_SIZE = 0x5 → 1 KB Output token buffer (2^(5+3))*4 otok_size_log2
 * Bit12–9  IN_DBUF_SIZE  = 0xC → 16 KB Input data buffer per PE | ipkt_size_log2
 * Bit8–6   IN_TBUF_SIZE  = 0x5 → 1 KB Input token buffer (2^(5+3))*4 | itok_size_log2
 * Bit4–0   NR_OF_PES     = 0x8 → 8 Processing Engines configured
 */
// itok_size_log2 = 1kb; ipkt_size_log2 = 16kb; otok_size_log2 = 1 KB; opkt_size_log2 = 8 KB
	read_data = readl(AXI4S_EIP_BASE + EIP197_OPTIONS);
	printf("EIP197_OPTIONS: %x\n", read_data);

	/*
	* HIA_OPTIONS = 0xA4F33884
	*
	* Host Interface Adapter (HIA) Options Register
	* ---------------------------------------------
	* [31]    ADDR_64    = 1  -> 64-bit addressing mode enabled
	* [30]    TGT_ALIGN  = 0  -> Normal 4KB target alignment
	* [29]    PE_ARBITER = 1  -> Central FIFO/Ring arbiter present (multi-PE coordination)
	* [27:25] HDW        = 0x2 -> Host interface data width = 128 bits
	* [24:20] DMA_LEN    = 0xF -> Max DMA transfer = 2^15 = 32Kb
	* [19:16] HOST_IFC   = 0x3 -> Host interface = AXI
	* [15]    EXP_PLW    = 0  ->  Disable partial writes
	* [14:12] RF_SIZE    = 0x3 -> Result FIFO = 2^(3+4) = 128 entries
	* [11:9]  CF_SIZE    = 0x4 -> Command FIFO = 2^(4+4) = 256 entries
	* [7:4]   N_PES      = 0x8 -> 8 Processing Engines active
	* [3:0]   N_RINGS    = 0x4 -> 4 Rings configured
	*/

	read_data = readl(AXI4S_EIP_BASE + HIA_OPTIONS);
	printf("HIA_OPTIONS: %x\n", read_data);

/*
 * HIA_OPTIONS2 = 0x30CF0011
 *
 * Host Interface Adapter (HIA) Options 2 Register
 * ------------------------------------------------
 * [31:28] AXI_CPC        = 0x3  -> 4 read channels per AXI read cluster (3 + 1)
 * [27:20] AXI_RD_CLUSTERS = 0x0C -> 13 read clusters (12 + 1)
 * [19:16] AXI_WR_CHAN    = 0xF  -> 16 AXI write channels (15 + 1)
 * [15:8]  Reserved       = 0
 * [7:4]   N_INLINE_IF    = 0x1  -> 1 inline streaming I/O interface
 * [3:0]   N_LA_IF        = 0x1  -> 1 look-aside I/O interface pair
 *
 * Summary:
 *  - 52 total AXI read channels (13 clusters × 4 each).
 *  - 16 AXI write channels for high DMA throughput.
 *  - 1 Inline and 1 Look-aside I/O path configured.
 *  - Optimized for parallel memory access and dual-path processing
 *    (inline data + look-aside acceleration).
 */

	read_data = readl(AXI4S_EIP_BASE + HIA_OPTIONS2);
	printf("HIA_OPTIONS2: %x\n", read_data);

/*
| **Component**   | **Configuration Field** | **Value (Hex)** | **Count** | **Notes**                                 |
| --------------- | ----------------------- | --------------- | --------- | ----------------------------------------- |
| **Rings (CDR)** | `N_RINGS [3:0]`         | `0x4`           | 4         | Number of Command Descriptor Rings        |
| **Look-aside**  | `N_LA_IF [3:0]`         | `0x1`           | 1         | Number of Look-aside I/O interface pairs  |
| **Inline**      | `N_INLINE_IF [7:4]`     | `0x1`           | 1         | Number of Inline streaming I/O interfaces |

*/
	/**
	 * HIA_OPTIONS: 0xA4F33884
	 * HIA_OPTIONS2: 0x30CF0011
	 */

	read_data = readl(AXI4S_EIP_BASE + HIA_DFE_0_OPTIONS);
	printf("HIA_DFE_0_OPTIONS: %x\n", read_data);

	read_data = readl(AXI4S_EIP_BASE + HIA_DFE_1_OPTIONS);
	printf("HIA_DFE_1_OPTIONS: %x\n", read_data);

	read_data = readl(AXI4S_EIP_BASE + HIA_DSE_0_OPTIONS);
	printf("HIA_DSE_0_OPTIONS: %x\n", read_data);

	read_data = readl(AXI4S_EIP_BASE + HIA_DSE_1_OPTIONS);
	printf("HIA_DSE_1_OPTIONS: %x\n", read_data);

	read_data = readl(AXI4S_EIP_BASE + HIA_AIC_R0_OPTIONS);
	printf("HIA_AIC_R0_OPTIONS: %x\n", read_data);

	read_data = readl(AXI4S_EIP_BASE + HIA_AIC_R1_OPTIONS);
	printf("HIA_AIC_R0_OPTIONS: %x\n", read_data);

	read_data = readl(AXI4S_EIP_BASE + HIA_AIC_G_OPTIONS);
	printf("HIA_AIC_G_OPTIONS: %x\n", read_data);

	read_data = readl(AXI4S_EIP_BASE + PE_0_ICE_OPTIONS);
	printf("PE_0_ICE_OPTIONS: %x\n", read_data);
	
	read_data = readl(AXI4S_EIP_BASE + PE_1_ICE_OPTIONS);
	printf("PE_1_ICE_OPTIONS: %x\n", read_data);

	read_data = readl(AXI4S_EIP_BASE + PE_0_OCE_OPTIONS);
	printf("PE_0_OCE_OPTIONS: %x\n", read_data);

	read_data = readl(AXI4S_EIP_BASE + PE_0_AIC_OPTIONS);
	printf("PE_0_AIC_OPTIONS: %x\n", read_data);

	read_data = readl(AXI4S_EIP_BASE + PE_0_PE_OPTIONS);
	printf("PE_0_PE_OPTIONS: %x\n", read_data);

	/**
        HIA_OPTIONS: 0xA4F3388C  //Old 0xA4F33884
        HIA_OPTIONS2: 0x30CF0011
        HIA_DFE_0_OPTIONS: 0xA4F3388C
        HIA_DFE_1_OPTIONS: 0xA4F3388C
        HIA_DSE_0_OPTIONS: 0xA4F3388C
        HIA_DSE_1_OPTIONS: 0xA4F3388C
        HIA_AIC_R0_OPTIONS: 0x00000018
        HIA_AIC_R0_OPTIONS: 0x00000018
        HIA_AIC_G_OPTIONS: 0x00000020
        PE_0_ICE_OPTIONS: 0x00000004
        PE_1_ICE_OPTIONS: 0x00000004
        PE_0_OCE_OPTIONS: 0x00000004
        PE_0_AIC_OPTIONS: 0x0000001E
        PE_0_PE_OPTIONS: 0xBB940560
		CS_OPTIONS: 0xB4001F03 //12 lookup tables

	 */


}

/**
 * @brief Maps the Flow Lookup Engine (FLUE) to its memory base addresses.
 * * This function configures the hardware registers to point to the start 
 * of the Security Context buffer and the Hash Lookup table.
 */
void eip197_bind_flue_memory(void) {
    // Set the base address for the Context Record Cache (FLUE Cache)
    writel(CONTEXTBUF_COMMAND_BASE_ADDR, AXI4S_EIP_BASE + FLUE_CACHEBASE_0_LO);

    // Set the base address for the Flow ID Hash Table (FLUE Hash)
    writel(LOOKUP_HASH_BASE, AXI4S_EIP_BASE + FLUE_HASHBASE_0_LO);

    printf("EIP-197: FLUE memory binding complete.\n");
}

/**
 * @brief Initialization for the EIP-197 cryptographic engine.
 * * This function handles the complete hardware init sequence. It ensures 
 * the sub-modules are initialized in the correct order before any traffic 
 * is permitted.
 */
int eth_eip197_init(uint32_t b_len, uint8_t eip_cdr){
	int ret;

    eth_print_eip_options_info();

	// EIP 197 Sum of all packets dropped (GlobalStatistics)
	eth_eip197_clear_statistics_dropped_allPE();

	/**
	* EIP202: EIP-202 Host Interface Adapter Configuration
	*/
	eth_eip202_config();

	/**
	* EIP-206 Processing Engine Configuration
	*/
	ret = eth_eip206_config();
	if (ret)
		return ret;

	/**
	* EIP207 Classif.Support Configuration
	*/
	ret = eth_eip207_config();
	if (ret)
		return ret;


	/**
	 *  EIP-197 interface configuration
	 */
	eth_eip_197_interface_configuration(b_len, eip_cdr);

	//Advanced Interrupt Controller (AIC)
    //eth_eip197_config_AIC();
	//eth_eip197_setup_cd_irq();
	return 0;
}

/**
 * @brief Configures the security context and lookup rules.
 * * This function prepares the transform records (keys/algorithms) and the 
 * hash lookup entries in memory. It also initializes the FLUE* registers.
 */
void eth_eip197_setup_session(CntxCtrl_t cntx_ctrl_out, CntxCtrl_t cntx_ctrl_in, HashRecd_t hash_rec, uint32_t eip_rdr, uint32_t b_len) {
    // Generate Security Contexts (Transform Records) for Egress and Ingress
    eth_init_transform_record_egress(cntx_ctrl_out);
    eth_init_transform_record_ingress(cntx_ctrl_in, eip_rdr);

    // Generate Flow Lookup entries (Hash Records)
    eth_init_lookup_hash_record_egress(&hash_rec);
    eth_init_lookup_hash_record_ingress(&hash_rec);

    // Map the hardware engines to the newly created memory records
    eip197_bind_flue_memory();
}

/**
 * @brief Prepares the packet payload and allocates result descriptors.
 * * This function writes the raw packet to the egress buffer and prepares 
 * the Result Descriptor Ring (RDR) to receive the processed data.
 * @param pkt_len Length of the packet payload.
 * @param b_len   The number of descriptors to prepare (Batch Length).
 */
void eth_eip197_prepare_data(pktConfig_t *pkt_cfg, uint32_t pkt_len, uint32_t b_len) {
    // Write the packet into the hardware-accessible egress memory
    eth_setup_egress_packet(pkt_cfg, pkt_len);

    // Setup the result descriptor to handle the engine's output
    eth_gen_result_descriptor(b_len);
}

static void eth_dump_registers(uint32_t base, uint32_t len)
{
	uint32_t off;

	for (off = 0; off < len; off += 4) {
		if (!(off % 16))
			printf("%08x:", base + off);
		printf(" %08x", readl(base + off));
		if ((off % 16) == 12)
			printf("\n");
	}
	if (off % 16)
		printf("\n");
}

static void eip197_dcache_flush(uint32_t start, uint32_t size)
{
	flush_dcache_range(start, start + ALIGN(size, ARCH_DMA_MINALIGN));
}

static void eip197_dcache_inval(uint32_t start, uint32_t size)
{
	invalidate_dcache_range(start, start + ALIGN(size, ARCH_DMA_MINALIGN));
}

void eth_eip197_print_dbg(uint8_t mac_idx, uint32_t cdr_y_prep_count, uint32_t rdr_y_prep_count){
    uint32_t reg_offset, reg_addr, read_data, reg_cnt;

    eip197_dcache_inval(EIP197_DDR_BASE, EIP197_DDR_SIZE);

    reg_offset = 0x1000;
    
    for(reg_cnt = 0; reg_cnt < 1; reg_cnt ++) {
        reg_addr = HIA_CDR_0_PROC_COUNT + reg_cnt*reg_offset;
        read_data = readl(AXI4S_EIP_BASE + reg_addr);
        printf("HIA_CDR_%d_PROC_COUNT - %x\n", reg_cnt, read_data);

        read_data = readl(AXI4S_EIP_BASE + cdr_y_prep_count);
        printf("HIA_CDR_%d_PREP_COUNT - %x\n", reg_cnt, read_data);
    }

    for(reg_cnt = 0; reg_cnt < 1; reg_cnt ++) {
        reg_addr = HIA_RDR_0_PROC_COUNT + reg_cnt*reg_offset;
        read_data = readl(AXI4S_EIP_BASE + reg_addr);
        printf("HIA_RDR_%d_PROC_COUNT - %x\n", reg_cnt, read_data);
        /* 
        read_data = readl(AXI4S_EIP_BASE + rdr_y_prep_count);
        printf("HIA_RDR_%d_PREP_COUNT - %x\n", reg_cnt, read_data);
            */
    }

    read_data = readl(AXI4S_EIP_BASE + EIP197_DBG_PIPE_COUNT_0);
    printf("EIP197_DBG_PIPE_COUNT_0 - %x\n", read_data);
    
    read_data = readl(AXI4S_EIP_BASE + EIP197_DBG_PIPE_STATE_0);
    printf("EIP197_DBG_PIPE_STATE_0 - %x\n", read_data);

    read_data = readl(AXI4S_EIP_BASE + EIP197_DBG_PIPE_DCOUNT_LO_0);
    printf("EIP197_DBG_PIPE_DCOUNT_LO_0 - %x\n", read_data);

    read_data = readl(AXI4S_EIP_BASE + EIP197_DBG_PIPE_DCOUNT_HI_0);
    printf("EIP197_DBG_PIPE_DCOUNT_HI_0 - %x\n", read_data);
    
    read_data = readl(AXI4S_EIP_BASE + EIP197_DBG_RING_IN_COUNT_LO_0);
    printf("EIP197_DBG_RING_IN_COUNT_LO_0 - %x\n", read_data);

    read_data = readl(AXI4S_EIP_BASE + 0xffbd0);
    printf("EIP197_DBG_RING_IN_COUNT_LO_13 - %x\n", read_data);

    read_data = readl(AXI4S_EIP_BASE + EIP197_DBG_RING_OUT_COUNT_LO_0);
    printf("EIP197_DBG_RING_OUT_COUNT_LO_0 - %x\n", read_data);
    read_data = readl(AXI4S_EIP_BASE + 0xffbd8);
    printf("EIP197_DBG_RING_OUT_COUNT_LO_13 - %x\n", read_data);

    read_data = readl(AXI4S_EIP_BASE + HIA_INLINE_CTRL_0);
    printf("HIA_INLINE_CTRL_0 - %x\n", read_data);

    read_data = 0;
    read_data = readl(CONTEXTBUF_COMMAND_BASE_ADDR + 0xF0);
    printf("CONTEXTBUF_COMMAND_BASE_ADDR + F0 %x\n", read_data);
    read_data = 0;
    read_data = readl((CONTEXTBUF_COMMAND_BASE_ADDR + CONTEXTBUF_IGRS_OFFSET) + 0xF0);
    printf("CONTEXTBUF_COMMAND_BASE_ADDR + CONTEXTBUF_IGRS_OFFSET + F0 %x\n", read_data);

    eth_print_mac_counters(mac_idx);

    read_data = readl(AXILS_HCP_BASE + (XGE_SHIM_RX_PKTS + mac_idx*4));
    printf("SHIM RX Packets - %x\n", read_data);

    read_data = readl(AXILS_HCP_BASE + (XGE_SHIM_RX_GOOD_PKTS + mac_idx*4));
    printf("SHIM RX Good Packets - %x\n", read_data);
    
    read_data = readl(AXILS_HCP_BASE + (XGE_SHIM_RX_BAD_PKTS + mac_idx*4));
    printf("SHIM RX Bad Packets - %x\n", read_data);

/* 
    eth_dump_registers(AXI4S_EIP_BASE + PE_0_ICE_SCRATCH_RAM + eip_pe*8*1024, 0xf0);
    eth_dump_registers(AXI4S_EIP_BASE + PE_0_OCE_SCRATCH_RAM + eip_pe*8*1024, 0xf0);
*/
    //eth_dump_registers(AXILS_HCP_BASE, 0xf0);
    printf("CDR_RING_BASE_ADDR Data\n");	
    eth_dump_registers(CDR_RING_BASE_ADDR, 0xf0);
    printf("RDR_RING_BASE_ADDR Data\n");	
    eth_dump_registers(RDR_RING_BASE_ADDR, 0xf0);

    printf("EGRS Data\n");	
    eth_dump_registers(EGRS_PACKET_BASE_ADDR, 0xf0);
    printf("IGRS Data\n");
    eth_dump_registers(IGRS_PACKET_BASE_ADDR, 0xf0);

    //eth_dump_registers(0x43100900, 0xff);
    //eth_dump_registers(0x43100a00, 0xff);

        
/* 	read_data = readl(AXI4S_EIP_BASE + HIA_RDR_0_STAT);
    printf("HIA_RDR_0_STAT %x\n", read_data);

    read_data = readl(AXI4S_EIP_BASE + HIA_CDR_3_STAT);
    printf("HIA_CDR_3_STAT %x\n", read_data);
*/
}

uint16_t eth_eip197_run_traffic_test(uint8_t mac_idx, uint8_t tdest, pktConfig_t egress_pkt_cfg, uint32_t b_len, uint8_t eip_cdr, uint8_t eip_rdr ){
    uint32_t iter_cnt = 0;
    uint32_t egress_data;
    uint32_t ingress_data;
	
	uint32_t cdr_y_prep_count, rdr_y_prep_count, rdr_y_ring_base_addr_lo, cdr_y_ring_base_addr_lo;

    uint8_t packetType = ETH_EIP_PACKET_TYPE;

	//0x8002C + (y*4K)
	cdr_y_prep_count = HIA_CDR_0_COUNT + eip_cdr*4*1024;
	//0x8082C + (y*4K)
	rdr_y_prep_count = HIA_RDR_0_PREP_COUNT + eip_rdr*4*1024;
	//0x80000 + (y*4K)
	cdr_y_ring_base_addr_lo = HIA_CDR_0_RING_BASE_ADDR_LO + eip_cdr*4*1024;
	//0x80800 + (y*4K)
	rdr_y_ring_base_addr_lo = HIA_RDR_0_RING_BASE_ADDR_LO + eip_rdr*4*1024;

    while (iter_cnt < 10) {
		printf("=============================================\n");
		printf("iter_cnt = %d\n", iter_cnt);
        printf("pkt_length = %d\n", egress_pkt_cfg.pkt_length);
	
		for(int b_pkt_cnt=0; b_pkt_cnt<b_len; b_pkt_cnt++) {
			gen_command_desc((CDR_RING_BASE_ADDR+(b_pkt_cnt*0x40)), (EGRS_PACKET_BASE_ADDR+(b_pkt_cnt*0x800)), egress_pkt_cfg.pkt_length, 1, 1, tdest);
            if(packetType == IPV6_PACKET) {
                gen_egress_pkt((EGRS_PACKET_BASE_ADDR+(b_pkt_cnt*0x800)), egress_pkt_cfg, (iter_cnt * 3));
            }else if(packetType == IPV4_PACKET) {
                gen_egress_pkt_ipv4((EGRS_PACKET_BASE_ADDR+(b_pkt_cnt*0x800)), egress_pkt_cfg, (iter_cnt * 3));
            }else {
                gen_egress_pkt((EGRS_PACKET_BASE_ADDR+(b_pkt_cnt*0x800)), egress_pkt_cfg, (iter_cnt * 3));
            }
		}
		eip197_dcache_flush(CDR_RING_BASE_ADDR, b_len * 0x40);
		eip197_dcache_flush(EGRS_PACKET_BASE_ADDR, b_len * 0x800);

		// Reset head/tail
		writel(0x80000000, AXI4S_EIP_BASE + cdr_y_prep_count);
		writel(0x80000000, AXI4S_EIP_BASE + rdr_y_prep_count);

		mdelay(200);
		writel(RDR_RING_BASE_ADDR, AXI4S_EIP_BASE + rdr_y_ring_base_addr_lo);
        writel(b_len * 0x0000040, AXI4S_EIP_BASE + rdr_y_prep_count);

        writel(CDR_RING_BASE_ADDR, AXI4S_EIP_BASE + cdr_y_ring_base_addr_lo);
        writel(b_len * 0x0000040, AXI4S_EIP_BASE + cdr_y_prep_count); //HIA_CDR_y_PREP_COUNT
		mdelay(200);

		uint32_t tx_good = 0, rx_good = 0;
		tx_good = AX_READ_MAC_REG32(mac_idx, SHIM_STATS_TX_MAC_GOOD);
		rx_good = AX_READ_MAC_REG32(mac_idx, SHIM_STATS_RX_MAC_GOOD);

		uint32_t tx_good_count = 0, rx_good_count = 0;
		eip197_dcache_inval(CONTEXTBUF_COMMAND_BASE_ADDR, CONTEXTBUF_IGRS_OFFSET + 0x100);
		tx_good_count = readl(CONTEXTBUF_COMMAND_BASE_ADDR + 0xF0);
		rx_good_count = readl((CONTEXTBUF_COMMAND_BASE_ADDR + CONTEXTBUF_IGRS_OFFSET) + 0xF0);

		printf("MAC tx_good %x rx_good %x | ctx egress %x ingress %x\n",
		       tx_good, rx_good, tx_good_count, rx_good_count);

		if (tx_good == 0 || tx_good != rx_good ||
		    tx_good != tx_good_count || tx_good != rx_good_count) {
			if (tx_good == 0)
				printf("FAIL: MAC tx_good = 0\n");
			else if (tx_good != rx_good)
				printf("FAIL: MAC tx_good != rx_good\n");
			else if (tx_good != tx_good_count)
				printf("FAIL: MAC tx_good != egress context count\n");
			else
				printf("FAIL: MAC tx_good != ingress context count\n");
			eth_eip197_print_dbg(mac_idx, cdr_y_prep_count, rdr_y_prep_count);
			return 0;
		}

		iter_cnt++;
    }
    return 1;
}

uint16_t eth_eip_197_test(uint8_t mac_idx, uint8_t tdest){
	uint8_t eip_rdr = 0, eip_cdr = 0;

	uint32_t b_len = 1;
	uint32_t pkt_len = 80;

	pktConfig_t egress_pkt_cfg;
	CntxCtrl_t cntx_ctrl_out;
	CntxCtrl_t cntx_ctrl_in;
	HashRecd_t hash_rec;

	memset(&egress_pkt_cfg, 0, sizeof(egress_pkt_cfg));
	memset(&cntx_ctrl_out, 0, sizeof(cntx_ctrl_out));
	memset(&cntx_ctrl_in, 0, sizeof(cntx_ctrl_in));
	memset(&hash_rec, 0, sizeof(hash_rec));

	memset((void *)(uintptr_t)EIP197_DDR_BASE, 0, EIP197_DDR_SIZE);
	eip197_dcache_flush(EIP197_DDR_BASE, EIP197_DDR_SIZE);

//************************************** EIP-197 Configuration *************************************//

	if (eth_eip197_init(b_len, eip_cdr)) {
		printf("EIP-197 init failed\n");
		return 0;
	}
	mdelay(200);
	//eth_eip197_setup_cd_irq();

	eth_eip197_setup_session(cntx_ctrl_out, cntx_ctrl_in, hash_rec, eip_rdr, b_len);
	mdelay(200);

	eth_eip197_prepare_data(&egress_pkt_cfg, pkt_len, b_len);
	mdelay(100);
	eip197_dcache_flush(EIP197_DDR_BASE, EIP197_DDR_SIZE);
	//eth_gen_command_desc();
	//gen_command_desc(CDR_RING_BASE_ADDR, EGRS_PACKET_BASE_ADDR, pkt_len, 1, 1, tdest);

	return eth_eip197_run_traffic_test(mac_idx, tdest, egress_pkt_cfg, b_len, eip_cdr, eip_rdr);

}

static void eth_eip197_print_interval(ulong from_ms, ulong to_ms, u32 pkts,
				      u32 pkt_len)
{
	ulong span = to_ms - from_ms;
	u32 pps = span ? div_u64((u64)pkts * 1000, span) : 0;
	u32 kbps = span ? div_u64((u64)pkts * pkt_len * 8, span) : 0;

	printf("[%3lu.%02lu-%3lu.%02lu sec]  %8u pkts  %7u pkts/s  %3u.%03u Mbits/sec\n",
	       from_ms / 1000, from_ms % 1000 / 10, to_ms / 1000,
	       to_ms % 1000 / 10, pkts, pps, kbps / 1000, kbps % 1000);
}

int eth_eip197_speed(uint8_t mac_idx, uint8_t tdest, int seconds,
		     uint32_t pkt_len)
{
	uint8_t eip_rdr = 0, eip_cdr = 0;
	uint32_t b_len = EIP197_SPEED_BATCH;
	uint32_t cdr_y_prep_count = HIA_CDR_0_COUNT + eip_cdr*4*1024;
	uint32_t rdr_y_prep_count = HIA_RDR_0_PREP_COUNT + eip_rdr*4*1024;
	uint32_t cdr_y_ring_base_addr_lo = HIA_CDR_0_RING_BASE_ADDR_LO + eip_cdr*4*1024;
	uint32_t rdr_y_ring_base_addr_lo = HIA_RDR_0_RING_BASE_ADDR_LO + eip_rdr*4*1024;
	uint32_t good, good_start, target, sent = 0, mark_sent = 0, ctx_egress;
	ulong start, ms, mark_ms = 0, wait, limit = (ulong)seconds * 1000;
	pktConfig_t egress_pkt_cfg;
	CntxCtrl_t cntx_ctrl_out;
	CntxCtrl_t cntx_ctrl_in;
	HashRecd_t hash_rec;
	int ret = 0;
	uint32_t i;

	memset(&egress_pkt_cfg, 0, sizeof(egress_pkt_cfg));
	memset(&cntx_ctrl_out, 0, sizeof(cntx_ctrl_out));
	memset(&cntx_ctrl_in, 0, sizeof(cntx_ctrl_in));
	memset(&hash_rec, 0, sizeof(hash_rec));

	memset((void *)(uintptr_t)EIP197_DDR_BASE, 0, EIP197_DDR_SIZE);
	eip197_dcache_flush(EIP197_DDR_BASE, EIP197_DDR_SIZE);

	if (eth_eip197_init(b_len, eip_cdr)) {
		printf("EIP-197 init failed\n");
		return -EIO;
	}
	eth_eip197_setup_session(cntx_ctrl_out, cntx_ctrl_in, hash_rec, eip_rdr, b_len);
	eth_eip197_prepare_data(&egress_pkt_cfg, pkt_len, b_len);

	for (i = 0; i < b_len; i++) {
		gen_command_desc((CDR_RING_BASE_ADDR+(i*0x40)), (EGRS_PACKET_BASE_ADDR+(i*EIP197_SPEED_SLOT)), egress_pkt_cfg.pkt_length, 1, 1, tdest);
		gen_egress_pkt((EGRS_PACKET_BASE_ADDR+(i*EIP197_SPEED_SLOT)), egress_pkt_cfg, 0);
	}
	eip197_dcache_flush(EIP197_DDR_BASE, EIP197_DDR_SIZE);

	printf("Sending %u-byte IPv6/UDP packets through EIP-197 on port %d for %d s, batch %u, Ctrl+C to stop\n",
	       pkt_len, mac_idx - 1, seconds, b_len);

	good = AX_READ_MAC_REG32(mac_idx, SHIM_STATS_TX_MAC_GOOD);
	good_start = good;
	start = get_timer(0);
	while ((ms = get_timer(start)) < limit) {
		if (ms - mark_ms >= EIP197_SPEED_REPORT_MS) {
			eth_eip197_print_interval(mark_ms, ms, sent - mark_sent, pkt_len);
			mark_ms = ms;
			mark_sent = sent;
		}
		if (ctrlc())
			break;

		writel(0x80000000, AXI4S_EIP_BASE + cdr_y_prep_count);
		writel(0x80000000, AXI4S_EIP_BASE + rdr_y_prep_count);
		writel(RDR_RING_BASE_ADDR, AXI4S_EIP_BASE + rdr_y_ring_base_addr_lo);
		writel(b_len * 0x0000040, AXI4S_EIP_BASE + rdr_y_prep_count);
		writel(CDR_RING_BASE_ADDR, AXI4S_EIP_BASE + cdr_y_ring_base_addr_lo);
		writel(b_len * 0x0000040, AXI4S_EIP_BASE + cdr_y_prep_count);

		target = good + b_len;
		wait = get_timer(0);
		while ((int32_t)((good = AX_READ_MAC_REG32(mac_idx, SHIM_STATS_TX_MAC_GOOD)) - target) < 0) {
			if (get_timer(wait) > EIP197_SPEED_TIMEOUT_MS)
				break;
		}
		if ((int32_t)(good - target) < 0) {
			sent += b_len - (target - good);
			printf("Timeout: %u/%u packets of the batch left the MAC\n",
			       b_len - (target - good), b_len);
			ret = -ETIMEDOUT;
			break;
		}
		sent += b_len;
	}
	ms = get_timer(start);
	if (ms > mark_ms)
		eth_eip197_print_interval(mark_ms, ms, sent - mark_sent, pkt_len);

	eip197_dcache_inval(CONTEXTBUF_COMMAND_BASE_ADDR, 0x100);
	ctx_egress = readl(CONTEXTBUF_COMMAND_BASE_ADDR + 0xF0);
	good = AX_READ_MAC_REG32(mac_idx, SHIM_STATS_TX_MAC_GOOD) - good_start;

	printf("\n--- port %d EIP-197 TX speed, %lu.%03lu s ---\n", mac_idx - 1,
	       ms / 1000, ms % 1000);
	printf("Packets sent (TX_MAC good): %u, EIP egress context: %u\n",
	       good, ctx_egress);
	if (ms) {
		printf("Rate: %u pkts/s\n", (u32)div_u64((u64)good * 1000, ms));
		eth_eip197_print_interval(0, ms, good, pkt_len);
	}
	printf("(Mbits/sec counted on the %u-byte plaintext packet, the ESP frame on the wire is longer)\n",
	       pkt_len);

	return ret;
}
