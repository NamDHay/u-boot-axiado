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
#include "eip197_fw.h"

/** 
 *EIP202: EIP-202 Host Interface Adapter
 */
 
/*
DFE Engine Configuration Register (HIA_DFE_0_CFG)
Value: 0xA7050905
-----------------------------------------------
    Bit [31] AGGRESSIVE = 1
		→ Enables “aggressive DMA mode.”
		This allows the DMA engine to prefetch data and start the next burst earlier,
		improving overall throughput.

	Bit [30] SKIP_ON_DMA_ERR = 0
		→ Default mode: When a DMA error occurs, the DFE halts and waits for
		host software to handle the fault.
		(If this were 1, it would skip the faulty packet automatically.)

	Bit [29] ADV_THRESHOLD_MODE = 1
		→ Enables latency compensation mechanism.
		Recommended for HCP HW v2.x or v3.x to reduce DMA wait cycles.

	Bit [28] Reserved = 0
		→ Always write 0.

	Bits [27:24] MAX_CTRL_SIZE = 0x7
		→ Sets the maximum “control data” DMA burst to 2⁷ = 128 bytes.
		Larger control sections are automatically divided into multiple DMA transfers.

	Bit [23] CTRL_SRC_LOCK = 0
		→ Control source address is *not locked*, meaning the DMA may fetch
		the next command descriptor without waiting for the current one to finish.

	Bits [22:20] CTRL_CACHE_CTRL = 0
		→ Default cache policy for control data — no cache optimization.

	Bits [19:16] MIN_CTRL_SIZE = 0x5
		→ Minimum “control data” DMA burst = 2⁵ = 32 bytes.
		Ensures efficiency by preventing very small DMA bursts.

	Bits [15:12] Reserved = 0

	Bits [11:8] MAX_DATA_SIZE = 0x9
		→ Maximum payload data DMA burst = 2⁹ = 512 bytes.
		If the packet is larger than this, DFE will split the transfer into multiple bursts.

	Bit [7] DATA_SRC_LOCK = 0
		→ Data source not locked. The DMA can continue fetching new data buffers
		without reloading the same source.

	Bits [6:4] DATA_CACHE_CTRL = 0
		→ Default data caching policy (no extra prefetch or cache).

	Bits [3:0] MIN_DATA_SIZE = 0x5
		→ Minimum payload DMA burst = 2⁵ = 32 bytes.
		Helps maintain consistent DMA transfer size and bus efficiency.
*/
/*
HIA_DFE_n_CFG = 0xe0000000 |
(itbufhi<<24) | (ctrl_rcache[3:1]<<20) | (itbuflo<<16) |
(ipbufhi<<8) | (data_rcache[3:1]<<4) | ipbuflo;
HIA_DSE_n_CFG = 0xc0000000 | (enable_single_wr<<29) | (buffer_ctrl<<14) |
(opbufhi<<8) | (data_wcache[3:1]<<4) | opbuflo;
*/
void eth_eip202_config_data_fetch_engine(void){
    uint32_t reg_offset, reg_addr;
    int reg_cnt;

    //#     Configure DFE
    //[27:24] MAX_CTRL_SIZE = 0x7 | [19:16] MIN_CTRL_SIZE = 0x5 | [11:8] MAX_DATA_SIZE = 0x9 | [3:0] MIN_DATA_SIZE = 0x5

    reg_offset = 0x80;
    for(reg_cnt = 0; reg_cnt < EIP197_NOF_PE; reg_cnt ++) {
        reg_addr = HIA_DFE_0_CFG + reg_cnt*reg_offset;
        writel(0xA7050905, AXI4S_EIP_BASE + reg_addr);
    }
}

//==============================
// Configure DSE (Data Store Engine)
//==============================
// Write configuration value 0xA002CA09 to HIA_DSE_0_CFG register.
// This register controls how DSE performs DMA transfers to store results
// from EIP-197 hardware back into system memory.
/*
Bit [31]  AGGRESSIVE = 1
    → Enable aggressive DMA mode.
      Allows DSE to issue continuous DMA bursts without waiting for host responses.
      Used mainly for debugging or high-performance testing.

 Bit [30]  SKIP_ON_DMA_ERR = 0
    → Stop DMA transfer immediately if an error occurs.
      Host must intervene to recover. Ensures reliable operation.

 Bit [29]  ENABLE_SINGLE_WR = 1
    → Use single-write DMA bursts.
      Ensures proper memory alignment for small or unaligned data blocks.
      Useful when the DMA bus cannot handle long bursts.

 Bit [28]  PAD_TO_TRF_BND = 0
    → Do not pad data up to the nearest transfer boundary.
      DMA size equals actual packet size.

 Bit [27]  STORE_ENGINE_ID = 1
    → Do not store Engine ID into the descriptor.
      Normally used only for debug or trace modes.

 Bit [17:16] Reserved / LAFFIO_WAIT_CTRL = 10b
    → wait only for the last transfer of each full packet (default);

 Bit [15:14]  WAIT_CTRL = 11b
    → never wait (default)

 Bit [13]  LAFIFO_ALLOW_NONBUF = 0
    → Disable non-buffered writes for LA-FIFO.
      All LA-FIFO writes are buffered.

 Bit [12]  ALLOW_NONBUF = 0
    → Disable non-buffered writes for main data path.
      all transfers will be performed bufferable.

 Bits [11:8]  MAX_DATA_SIZE = 0xa (binary 1010)
    → Maximum DMA burst size = 2^a = 1 Kb per burst.
      Small bursts are safer for systems with narrow AXI data width.

 Bit [7]  DATA_DEST_LOCK = 0
    → Destination address is not locked.
      DSE may update the address for next DMA block.

 Bits [6:4]  DATA_CACHE_CTRL = 0
    → Cache control setting for DMA:
      Indicates write-back caching or host-dependent cache policy.

 Bits [3:0]  MIN_DATA_SIZE = 0xa (binary 1001)
    → Minimum transfer size = 2^9 bytes = 512
      Defines the smallest DMA block that DSE can handle.*/
void eth_eip202_config_data_store_engine(void){
    uint32_t reg_offset, reg_addr;
    int reg_cnt;

    reg_offset = 0x80;
    //#     Configure DSE
    //STORE_ENGINE_ID = 1 | [11:8]  MAX_DATA_SIZE = 0xa | [3:0]  MIN_DATA_SIZE = 0xa (binary 1001)
    for(reg_cnt = 0; reg_cnt < EIP197_NOF_PE; reg_cnt ++) {
        reg_addr = HIA_DSE_0_CFG + reg_cnt*reg_offset;
        writel(0xa002ca09, AXI4S_EIP_BASE + reg_addr);
    }
}

/**
 * EIP202 Main Configuration
 */
void eth_eip202_config(){
    uint32_t read_data;
    //********************Configure Data Feth Engine********************************//
    /*
    DFE Engine Configuration Register (HIA_DFE_0_CFG)
    Value: 0xA7050905
    -----------------------------------------------
    */
    eth_eip202_config_data_fetch_engine();	


    //********************Configure Data Store Engine********************************//
    /*
    DSE Engine Configuration Register (HIA_DSE_0_CFG)
    Value: 0xa002ca09
    -----------------------------------------------
    */
    eth_eip202_config_data_store_engine();

    	// Configure HIA Inline Interface 0 - Inline Streaming Output(s)
	// FORCE_INORDER [31] = 0 -> do not force output packet data in order
	// Reserved [30:24] = 0 -> must write zeros
	// ODATA_BURST [23:20] = 5 -> 2^N beats per output burst = 2^5 beats
	// ODATA_THRESH_HIGH [19:16] = 10 -> max threshold for output data buffer read | must match the value in the max_thresh field of the PE_n_OUT_DBUF_THRESH
	// ODATA_THRESH_LOW [15:12] = 9 -> min threshold for output data buffer read | must match the value in the min_thresh field of the PE_n_OUT_DBUF_THRESH
	// ODATA_SWAP [11:8] = 0 -> no byte/word/chunk swapping for output
	// IPROT_ERR [7] = 0 -> clear input protocol error flag
	// Reserved [6:4] = 0 -> must write zeros
	// IDATA_SWAP [3:0] = 0 -> no byte/word/chunk swapping for input
	writel(0x805a9000, AXI4S_EIP_BASE + HIA_INLINE_CTRL_0);

    //#     InlineMasterIfc0: swap_mode=0, burst_size=32
	//PyExec self.m_ConnectObject.InlineMstIfc[0].Configure({'swap_mode': 1, 'burst_size': 32})

	//Document HW3.3 Program Manual 3.6.1
	read_data = readl(AXI4S_EIP_BASE + HIA_VERSION);
	read_data = read_data & 0xffff;
	writel(0xFE000044, AXI4S_EIP_BASE + HIA_MST_CTRL);
}

void eth_eip197_clear_statistics_dropped_allPE() {
    uint32_t write_loc, da_index;
    uint32_t reg_offset = 0x2000;
    for (int eip_pe = 0; eip_pe < EIP197_NOF_PE; eip_pe ++) {
        //Allow to write to scratch RAM
        //28:25 – SCRATCH_ACCESS 31 block × 128 B ≈ 3968 Maximumn
        writel(0x1f000000, AXI4S_EIP_BASE + (PE_0_ICE_SCRATCH_CTRL + eip_pe*reg_offset)); 

        //PE_n_ICE_SCRATCH_RAM — Byte Address 0xA0800- 0xA0BFF + (n * 8K)
        //Statistics Dropped [31:0] & Statistics Dropped [63:32]; write 0 to these fields
        //Sum of all packets dropped
        //0xa0838: Scratchpad ICE - 0x38 = Word 14 (0x38/4)
        //PE_n_ICE_SCRATCH_RAM — Byte Address 0xA0800- 0xA0BFF + (n * 8K)
        write_loc = 0x000a0838 + eip_pe*8*1024;
        for (da_index = 0; da_index < 2; da_index++) {
            writel(0, AXI4S_EIP_BASE + write_loc);
            write_loc += 4;
        }
        //Disable Write to SCRATCH_RAM
        writel(0x01000000, AXI4S_EIP_BASE + (PE_0_ICE_SCRATCH_CTRL + eip_pe * reg_offset));
    }
}

void eth_eip197_clear_oce_scratchpad(void) {
    uint32_t read_data, reg_addr;
    uint32_t reg_offset;
    int reg_cnt;

    reg_offset = 0x2000;
    //Output Classification Engine Pull-up Micro-engine Control/Status Registers
	//SW Reset
    for(reg_cnt = 0; reg_cnt < 8; reg_cnt ++) {
        reg_addr = PE_0_OCE_SCRATCH_CTRL + reg_cnt*reg_offset;
        //SCRATCH_ACCESS: 100b (4x128 bytes); CHANGE_ACCESS: 1
	    writel(0x09000000, AXI4S_EIP_BASE + reg_addr);  

        //Clear all OCE scratchpad
        //Output Classification Engine (OCE) Scratchpad RAM Area
        //PE_n_OCE_SCRATCH_RAM — Byte Address 0xA1400-0xA17FF + (n * 8K)
        //# EIP197.PE_0.OCE: InitScratchPad(Size=0x200 bytes)
        
        reg_addr = PE_0_OCE_SCRATCH_RAM + reg_cnt*reg_offset;
        for(int i=0; i<128; i=i+1) {
            writel(0x00000000, AXI4S_EIP_BASE + ((reg_addr + (i*4))));
        }

    }
}

void eth_eip197_clear_ice_scratchpad(void) {
    uint32_t read_data, reg_addr;
    uint32_t reg_offset;
    int reg_cnt;

    reg_offset = 0x2000;

    for(reg_cnt = 0; reg_cnt < 8; reg_cnt ++) {
        reg_addr = PE_0_ICE_SCRATCH_CTRL + reg_cnt*reg_offset;

        //SCRATCH_ACCESS: 100b (4x128 bytes); CHANGE_ACCESS: 1
	    writel(0x09000000, AXI4S_EIP_BASE + reg_addr);  

        //Clear all ICE scratchpad
        //Input Classification Engine Scratchpad RAM area
        //PE_n_ICE_SCRATCH_RAM — Byte Address 0xA0800- 0xA0BFF + (n * 8K)
        //# EIP197.PE_0.ICE: InitScratchPad(Size=0x200 bytes)
        reg_addr = PE_0_ICE_SCRATCH_RAM + reg_cnt*reg_offset;
        for(int i=0; i<128; i=i+1) {
            writel(0x00000000, AXI4S_EIP_BASE + ((reg_addr+(i*4))));
        }
    }
}

void eth_eip96_config(void){
    uint32_t read_data, reg_addr;
    uint32_t pe_offset = 0x2000;
    int pe_cnt;

     // Configure EIP-96 Context/Token Control Register
    // [16] Optimal context updates = 1  → enable pipelined context update
    // [17] CT no token wait = 1         → skip waiting for previous token (safe if CT-bit set)
    // [18] Absolute ARC4 pointer = 0    → use relative pointer
    // [19:20] Context reuse = 0         → no cached reuse
    // [21] Zero-length result pkt = 0   → still output result token
    // [22] Time-out counter enable = 1  → enable watchdog timer for hang detection
    // [30] Detailed error = 0           → standard error codes
    // [31] Hold processing = 0          → continuous processing
    
	//Base 0xa1000 from EIP96_TOKEN_CTRL_STAT
    for(pe_cnt = 0; pe_cnt < EIP197_NOF_PE; pe_cnt ++) {
        reg_addr = PE_0_EIP96_TOKEN_CTRL_STAT + pe_cnt*pe_offset;
	    //AX_BASE_WRITE_REG32(AXI4S_EIP_BASE, reg_addr, 0x00430000);
        writel(0x00434004, AXI4S_EIP_BASE + reg_addr);
    }

    // Configure EIP-96 Token Control Register 2
    // [0] adv_fifo_mode = 1 → enable Advanced FIFO mode (direct packet streaming, no pointer in token)
    // [1] prep_ovs_chk_dis = 1 → disable preprocessor oversize check (flush excess bytes silently)
    // [2] fetch_ovs_chk_dis = 0 → keep fetch oversize check enabled (safe memory boundary)
    // => inline mode, high performance, fault-tolerant input stream

    for(pe_cnt = 0; pe_cnt < EIP197_NOF_PE; pe_cnt ++) {
        reg_addr = PE_0_EIP96_TOKEN_CTRL2 + pe_cnt*pe_offset;
	    writel(0x00000003, AXI4S_EIP_BASE + reg_addr);
    }

    // Configure EIP-96 Context Fetch behavior
    // Bit[7:0]  Context_Size = 0x3E (62 dwords) → full context size for EIP-96ies
    // Bit[8]    Address_Mode = 0 → fetch context from address provided in token
    // Bit[9]    Control_Mode = 1 → selective fetch mode (fetch only relevant fields)
    // Bit[31:10] Reserved = 0
    for(pe_cnt = 0; pe_cnt < EIP197_NOF_PE; pe_cnt ++) {
        reg_addr = PE_0_EIP96_CONTEXT_CTRL + pe_cnt*pe_offset;
	    writel(0x0000023e, AXI4S_EIP_BASE + reg_addr);
    }


	//# EIP197.PE_0.EIP96: Configure
	//#     'ip_len_delta_en': 1(0x1)
	//#     'detailed_err_codes_en': 0(0x0)
	//#     'block_update_append': 0(0x0)
	//# Set EIP96_OUT_BUF_CTRL.block_update=0

    for(pe_cnt = 0; pe_cnt < EIP197_NOF_PE; pe_cnt ++) {
        reg_addr = PE_0_EIP96_OUT_BUF_CTRL + pe_cnt*pe_offset;
	    writel(0x00000000, AXI4S_EIP_BASE + reg_addr);
    }

	//# Set EIP96_OUT_BUF_CTRL.ip_len_delta_en=1
	for(pe_cnt = 0; pe_cnt < EIP197_NOF_PE; pe_cnt ++) {
        reg_addr = PE_0_EIP96_OUT_BUF_CTRL + pe_cnt*pe_offset;
	    writel(0x40000000, AXI4S_EIP_BASE + reg_addr);
    }
	//# EIP197.PE_0.EIP96: EARLY DETECT ROLLOVER
	//# Configure detailed err codes: Enable = 0

	// Configure EIP-96 Token Control/Status
	// bit2 = 1 → A new token can be read by the EIP-96.
	// bit14 = 1 → No (part of) tokens are in the EIP-96 and no context update required
    for(pe_cnt = 0; pe_cnt < EIP197_NOF_PE; pe_cnt ++) {
        reg_addr = PE_0_EIP96_TOKEN_CTRL_STAT + pe_cnt*pe_offset;
	    writel(0x00434004, AXI4S_EIP_BASE + reg_addr);
    }

	// Reset the EIP96_ECN_ERR_CFG register to 0
	// This disables all IP and Next Header (NH) error checking in EIP-197
	// Explanation of fields after writing 0x00000000:
	// ProtoIPv4 [7:0]      = 0   -> Protocol value for IPv4, ignored
	// ProtoIPv6 [15:8]     = 0   -> Protocol value for IPv6, ignored
	// InvalidIPCLE [20:16] = 0   -> CLE code for invalid IP header version, ignored
	// InvIPErrEn [23]      = 0   -> Disable Invalid IP header version error generation
	// InvalidNHCLE [28:24] = 0   -> CLE code for invalid NH value, ignored
	// InvNHErrEn [31]      = 0   -> Disable Invalid NH value error generation

    for(pe_cnt = 0; pe_cnt < EIP197_NOF_PE; pe_cnt ++) {
        reg_addr = PE_0_EIP96_ECN_ERR_CFG + pe_cnt*pe_offset;
	    writel(0x00000000, AXI4S_EIP_BASE + reg_addr);
    }
}

//Record Cache Coherence Control
/*
 * PIPE_n_ICE_COHERENCE_CTRL = 0x7D0A3820
 *
 * Coherence control configuration for ICE pipeline:
 * -------------------------------------------------
 * [30] IFPP_HOST_FR_COH  = 1 → Keep IFPP flow record coherent with host
 * [29] IFPP_IFPP_FR_COH  = 1 → Keep IFPP flow record coherent with other IFPPs
 * [28] IFPP_HOST_TR_COH  = 1 → Keep IFPP transform record coherent with host
 * [27] IFPP_IFPP_TR_COH  = 0 → Disable coherence between IFPP transform records
 * [26] HOST_TR_COH       = 1 → Keep core transform record coherent with host
 * [25] IFPP_TR_COH       = 1 → Keep core transform record coherent with IFPP
 * [24] TR_COH            = 1 → Keep core transform record coherent with other cores
 *
 * Coherence Boundaries:
 * [23:16] IFPP_FR_COH_BOUND = 0x0A (10 words → 40 bytes)
 *     - Flow record coherence only above word index 10
 * [15:8]  IFPP_TR_COH_BOUND = 0x38 (56 words → 224 bytes)
 *     - IFPP transform coherence only above word index 56
 * [7:0]   TR_COH_BOUND      = 0x20 (32 words → 128 bytes)
 *     - Core transform coherence only above word index 32
 *
 * Summary:
 *  - All Host/IFPP/Core coherence paths are enabled except IFPP↔IFPP transform (bit 27 = 0).
 *  - Coherence is skipped for lower regions of records (below 10/32/56 words)
 *    to improve performance and reduce unnecessary cache synchronization.
 */


/*
 * PIPE_n_OCE_COHERENCE_CTRL = 0x59000000
 *
 * Output Classification Engine (OCE) Coherence Control Register
 * --------------------------------------------------------------
 * [31] Reserved               = 0
 * [30] OFPP_HOST_FR_COH       = 1 → Keep OFPP flow record coherent with host
 * [29] OFPP_IFPP_FR_COH       = 0 → Disable flow record coherence with IFPP
 * [28] OFPP_OFPP_FR_COH       = 1 → Keep OFPP flow record coherent with other OFPPs
 * [27] OFPP_HOST_TR_COH       = 1 → Keep OFPP transform record coherent with host
 * [26] OFPP_IFPP_TR_COH       = 0 → Disable transform record coherence with IFPP
 * [25] OFPP_TR_COH            = 1 → Keep OFPP transform record coherent with cores
 * [24] OFPP_OFPP_TR_COH       = 0 → Disable transform record coherence with other OFPPs
 *
 * Coherence Boundaries:
 * [23:16] OFPP_FR_COH_BOUND   = 0x00 → Full flow record coherence (no boundary)
 * [15:8]  OFPP_TR_COH_BOUND   = 0x00 → Full transform record coherence (no boundary)
 * [7:0]   Reserved            = 0x00
 *
 * Summary:
 *  - Coherence with Host and cores is enabled.
 *  - Coherence with IFPP and between OFPP instances is disabled.
 *  - Flow and Transform records are fully coherent (boundaries = 0).
 *  - This configuration prioritizes Host ↔ Core consistency while reducing
 *    inter-block (IFPP/OFPP) coherence overhead to improve performance.
 */
void eth_eip197_config_record_cache_coherence(void){
    writel(0x7d0a3820, AXI4S_EIP_BASE + EIP197_PIPE0_ICE_COHERENCE_CTRL);
    writel(0x59000000, AXI4S_EIP_BASE + EIP197_PIPE0_OCE_COHERENCE_CTRL);
    writel(0xffffffff, AXI4S_EIP_BASE + EIP197_PIPE0_COHERENT_WITH);
}

//Configure the Processing Engine
void eth_eip197_config_pe(void){
    uint32_t read_data, reg_addr;
    uint32_t pe_offset = 0x2000;
    int pe_cnt;

    for(pe_cnt = 0; pe_cnt < EIP197_NOF_PE; pe_cnt ++) {
        reg_addr = PE_0_ICE_PUTF_CTRL + pe_cnt*pe_offset;
        // [7:0]   TOKEN_LIMIT = 0x03 → FIFO limited to 3 + 2 tokens (recommended by documentation)
	    writel(0x00000003, AXI4S_EIP_BASE + reg_addr); //Token limit = 3 in fifo
    }

    for(pe_cnt = 0; pe_cnt < EIP197_NOF_PE; pe_cnt ++) {
        reg_addr = PE_0_ICE_PPTF_CTRL + pe_cnt*pe_offset;
	    writel(0x00000000, AXI4S_EIP_BASE + reg_addr);
    }

    /*
    PE_0_IN_TBUF_THRESH:
    MAX_THRESH [15:12] = upper 4 bits = 0111 = 0x7
    MIN_THRESH [11:8] = next 4 bits = 0101 = 0x5
    FREE_PKT_THRESH [7:0] = low 8 bits = 0x0E = 14 (decimal)
    */
    for(pe_cnt = 0; pe_cnt < EIP197_NOF_PE; pe_cnt ++) {
        reg_addr = PE_0_IN_TBUF_THRESH + pe_cnt*pe_offset;
	    writel(0x0000750e, AXI4S_EIP_BASE + reg_addr);
    }
    
    /*
    MAX_THRESH [15:12]	1001	0x9	2⁹ = 512 bytes
    MIN_THRESH [11:8]	0101	0x5	2⁵ = 32 bytes
    FREE_PKT_THRESH [7:0]	00000000	0x00	0 — not use (reset default).
    */
    for(pe_cnt = 0; pe_cnt < EIP197_NOF_PE; pe_cnt ++) {
        reg_addr = PE_0_IN_DBUF_THRESH + pe_cnt*pe_offset;
	    writel(0x00009500, AXI4S_EIP_BASE + reg_addr);
    }
    
    /*
    MAX_THRESH [7:4]	1010	0xA = 10	2¹⁰ = 1024 bytes
    MIN_THRESH [3:0]	1001	0x9 = 9	2⁹ = 512 bytes
    */

    for(pe_cnt = 0; pe_cnt < EIP197_NOF_PE; pe_cnt ++) {
        reg_addr = PE_0_OUT_DBUF_THRESH + pe_cnt*pe_offset;
	    writel(0x000000a9, AXI4S_EIP_BASE + reg_addr);
    }

    eth_eip96_config();

	// Write special key and max_pkt_size to PE_0_ICE_ADAPT_CTRL
	// KEY [31:16] = 0xC0DE -> enables writing the write-only MAX_PKT_SIZE field
	// MAX_PKT_SIZE [15:2] = 0x380 -> sets the maximum depth (in DWORDs) 
	//   that the microengine can access when parsing headers or end of packet
	//   Note: This is not always equal to the maximum packet size.
	//   Be careful not to set too close to input packet buffer size to avoid dead-lock.
	// Reserved [1:0] = 0 -> must write zeros

    for(pe_cnt = 0; pe_cnt < EIP197_NOF_PE; pe_cnt ++) {
        reg_addr = PE_0_ICE_ADAPT_CTRL + pe_cnt*pe_offset;
	    writel(0xc0de3800, AXI4S_EIP_BASE + reg_addr);
    }
}

/**
 * Release reset thread on DFE and DSE
 * Enable PE
 * Enable Processing on DSE
 */
void eth_eip197_config_PE_thread(void) {
    uint32_t read_data, reg_addr;
    uint32_t hia_offset = 0x80;
    int hia_cnt;
    
    //(soft) Reset the Thread for Processing Engine
    //(soft) Reset the DFE/DSE Thread for Processing Engine
    for(hia_cnt = 0; hia_cnt < EIP197_NOF_PE; hia_cnt ++) {
        reg_addr = HIA_DFE_0_THR_CTRL + hia_cnt*hia_offset;
	    writel(0x80000000, AXI4S_EIP_BASE + reg_addr);
    }
    for(hia_cnt = 0; hia_cnt < EIP197_NOF_PE; hia_cnt ++) {
        reg_addr = HIA_DSE_0_THR_CTRL + hia_cnt*hia_offset;
	    writel(0x80000000, AXI4S_EIP_BASE + reg_addr);
    }
    
    //Enables the Processing Engine to process packets
    //HIA Input Interface Arbiter Processing Engine Control
    //0x90010 + (n*8)
    int ra_pe_offset = 8;
    for(hia_cnt = 0; hia_cnt < EIP197_NOF_PE; hia_cnt ++) {
        reg_addr = HIA_RA_PE_0_CTRL + hia_cnt*ra_pe_offset;
	    writel(0x40000000, AXI4S_EIP_BASE + reg_addr);
    }
    
    //Release Reset the DFE Thread for Processing Engine
    for(hia_cnt = 0; hia_cnt < EIP197_NOF_PE; hia_cnt ++) {
        reg_addr = HIA_DFE_0_THR_CTRL + hia_cnt*hia_offset;
	    writel(0x00000000, AXI4S_EIP_BASE + reg_addr);
    }

    mdelay(1000);

    //Release reset and Enable processing on DSE
    for(hia_cnt = 0; hia_cnt < EIP197_NOF_PE; hia_cnt ++) {
        reg_addr = HIA_DSE_0_THR_CTRL + hia_cnt*hia_offset;
	    writel(0x40000040, AXI4S_EIP_BASE + reg_addr);
    }
}

/**
 * EIP206 Configuration
 */
int eth_eip206_config(void){
	//Record Cache Coherence Control
	eth_eip197_config_record_cache_coherence();

	//Configure the Processing Engine [0-7]
	eth_eip197_config_pe();

	//Enable PE and release reset HIA DSE/ HIA DFE
	eth_eip197_config_PE_thread();

	//# EIP197_Driver_FW: Init
	//# EIP197_Driver_FW: Firmware load
	return eth_eip197_load_micro_engines_firmware();
}

/******************************************************************************
 * EIP197 Micro Engines Firmware Control
 *
 * This module handles firmware loading, scratchpad initialization,
 * engine configuration, and readiness polling for all micro-engines.
 ******************************************************************************/
//# Configure engine sync mask for interface to ffffffff
void eth_eip197_config_engine_sync_mask(void) {
    //# Configure engine 0 sync mask for interface 0 to ffffffff
	writel(0xffffffff, AXI4S_EIP_BASE + 0x000a0868);

	//# Configure engine 0 sync mask for interface 1 to ffffffff
	writel(0xffffffff, AXI4S_EIP_BASE + 0x000a086c);

	//# Configure engine 0 sync mask for interface 2 to ffffffff
	writel(0xffffffff, AXI4S_EIP_BASE + 0x000a0870);

	//# Configure engine 0 sync mask for interface 3 to ffffffff
	writel(0xffffffff, AXI4S_EIP_BASE + 0x000a0874);

	//# Configure engine 0 sync mask for interface 4 to ffffffff
	writel(0xffffffff, AXI4S_EIP_BASE + 0x000a0878);

	//# Configure engine 0 sync mask for interface 5 to ffffffff
	writel(0xffffffff, AXI4S_EIP_BASE + 0x000a087c);

	//# Configure engine 0 sync mask for interface 6 to ffffffff
	writel(0xffffffff, AXI4S_EIP_BASE + 0x000a0880);

	//# Configure engine 0 sync mask for interface 7 to ffffffff
	writel(0xffffffff, AXI4S_EIP_BASE + 0x000a0884);

	//# Configure engine 0 sync mask for interface 8 to ffffffff
	writel(0xffffffff, AXI4S_EIP_BASE + 0x000a0888);

	//# Configure engine 0 sync mask for interface 9 to ffffffff
	writel(0xffffffff, AXI4S_EIP_BASE + 0x000a088c);

	//# Configure engine 0 sync mask for interface 10 to ffffffff
	writel(0xffffffff, AXI4S_EIP_BASE + 0x000a0890);

	//# Configure engine 0 sync mask for interface 11 to ffffffff
	writel(0xffffffff, AXI4S_EIP_BASE + 0x000a0894);

	//# Configure engine 0 sync mask for interface 12 to ffffffff
	writel(0xffffffff, AXI4S_EIP_BASE + 0x000a0898);

	//# Configure engine 0 sync mask for interface 13 to ffffffff
	writel(0xffffffff, AXI4S_EIP_BASE + 0x000a089c);

	//# Configure engine 0 sync mask for interface 14 to ffffffff
	writel(0xffffffff, AXI4S_EIP_BASE + 0x000a08a0);

	//# Configure engine 0 sync mask for interface 15 to 0
	writel(0x00000000, AXI4S_EIP_BASE + 0x000a08a4);
}

void eth_eip197_config_reload_oce(void) {
    uint32_t read_data, reg_addr;
    uint32_t reg_offset;
    int reg_cnt;

    reg_offset = 0x2000;

    //# Set time-out value to: 15
    for(reg_cnt = 0; reg_cnt < EIP197_NOF_PE; reg_cnt ++) {
        reg_addr = PE_0_OCE_FPP_TIMEOUT_CTRL + reg_cnt*reg_offset;
	    writel(0x0000000f, AXI4S_EIP_BASE + reg_addr);  
    }
	//# Set time-out value to: 15
    for(reg_cnt = 0; reg_cnt < EIP197_NOF_PE; reg_cnt ++) {
        reg_addr = PE_0_OCE_PUE_TIMEOUT_CTRL + reg_cnt*reg_offset;
	    writel(0x0000000f, AXI4S_EIP_BASE + reg_addr);  
    }
	//# Start(DebugReset=1, ReloadConfig=1)
    for(reg_cnt = 0; reg_cnt < EIP197_NOF_PE; reg_cnt ++) {
        reg_addr = PE_0_OCE_FPP_CTRL + reg_cnt*reg_offset;
	    writel(0x84e00008, AXI4S_EIP_BASE + reg_addr);  
    }
	//# Start(DebugReset=1, ReloadConfig=1)
    for(reg_cnt = 0; reg_cnt < EIP197_NOF_PE; reg_cnt ++) {
        reg_addr = PE_0_OCE_PUE_CTRL + reg_cnt*reg_offset;
	    writel(0x84e00008, AXI4S_EIP_BASE + reg_addr);  
    }
}

void eth_eip197_config_reload_ice(void) {
    uint32_t read_data, reg_addr;
    uint32_t reg_offset;
    int reg_cnt;

    reg_offset = 0x2000;

    //# Set time-out value to: 15
    for(reg_cnt = 0; reg_cnt < EIP197_NOF_PE; reg_cnt ++) {
        reg_addr = PE_0_ICE_FPP_TIMEOUT_CTRL + reg_cnt*reg_offset;
	    writel(0x0000000f, AXI4S_EIP_BASE + reg_addr);  
    }
	//# Set time-out value to: 15
    for(reg_cnt = 0; reg_cnt < EIP197_NOF_PE; reg_cnt ++) {
        reg_addr = PE_0_ICE_PUE_TIMEOUT_CTRL + reg_cnt*reg_offset;
	    writel(0x0000000f, AXI4S_EIP_BASE + reg_addr);  
    }
	//# Start(DebugReset=1, ReloadConfig=1)
    for(reg_cnt = 0; reg_cnt < EIP197_NOF_PE; reg_cnt ++) {
        reg_addr = PE_0_ICE_FPP_CTRL + reg_cnt*reg_offset;
	    writel(0x8fe00008, AXI4S_EIP_BASE + reg_addr);  
    }
	//# Start(DebugReset=1, ReloadConfig=1)
    for(reg_cnt = 0; reg_cnt < EIP197_NOF_PE; reg_cnt ++) {
        reg_addr = PE_0_ICE_PUE_CTRL + reg_cnt*reg_offset;
	    writel(0x87e00008, AXI4S_EIP_BASE + reg_addr);  
    }
}

//*******************************Polling IFPPn*****************************************************//
static int eth_eip_polling_firmware(const char *name, uint32_t scratch_ram,
				    uint32_t status_off)
{
    uint32_t status, ver, timeout;
    uint32_t scratchpad_base;
    uint32_t scratchpad_offset = 0x2000;
    uint16_t Ver_Major, Ver_Minor, Ver_Patch;
    int ret = 0;

    for(int i=0; i < EIP197_NOF_PE; i++) {
        scratchpad_base = scratch_ram + (i*scratchpad_offset);
        timeout = 10;
        while (1) {
            status = readl(AXI4S_EIP_BASE + (scratchpad_base + status_off));
            if (status == 0x00000001 || !timeout--)
                break;
            mdelay(100);
        }
        if (status == 0x00000001) {
            ver = readl(AXI4S_EIP_BASE + scratchpad_base);
            Ver_Major = (ver & 0xf00) >> 8;
            Ver_Minor = (ver & 0xf0) >> 4;
            Ver_Patch = ver & 0xf;
            printf("%s%d Firmware Found - Version %d.%d.%d\n", name, i, Ver_Major, Ver_Minor, Ver_Patch);
        } else {
            printf("%s%d Firmware not ready (status %x)\n", name, i, status);
            ret = -ETIMEDOUT;
        }
    }
    return ret;
}

int eth_eip_polling_ifpp_firmware(void) {
    return eth_eip_polling_firmware("IFPP", PE_0_ICE_SCRATCH_RAM, IFPP_FW_STATUS);
}

//*******************************Polling IPUEn*****************************************************//
int eth_eip_polling_ipue_firmware(void) {
    return eth_eip_polling_firmware("IPUE", PE_0_ICE_SCRATCH_RAM, IPUE_FW_STATUS);
}

void eth_eip197_ifpp_firmware_load(void){
    uint32_t read_data, reg_addr;
    uint32_t reg_offset;
    int reg_cnt;

    reg_offset = 0x2000;
    //Input Classification Engine Flow Post-Processor Micro-Engine Control/Status Registers
	//# SW Reset
    for(reg_cnt = 0; reg_cnt < 8; reg_cnt ++) {
        reg_addr = PE_0_ICE_FPP_CTRL + reg_cnt*reg_offset;
	    writel(0x00000001, AXI4S_EIP_BASE + reg_addr);  
    }

	//Input Classification Engine RAM Access Control Register
    for(reg_cnt = 0; reg_cnt < 8; reg_cnt ++) {
        reg_addr = PE_0_ICE_RAM_CTRL + reg_cnt*reg_offset;
        //FPP_PROG_EN: enable access to Flow Post-processor Micro-engine program RAM
	    writel(0x00000002, AXI4S_EIP_BASE + reg_addr);  
    }

	uint32_t ifpp_address = 0xe0000;
    uint32_t ifpp_i = 0;
    for (ifpp_i=0; ifpp_i<4077; ifpp_i++)
    {
        writel(ifpp_data_arr[ifpp_i], AXI4S_EIP_BASE + ifpp_address);
        ifpp_address = ifpp_address + 4;
    }
    for(reg_cnt = 0; reg_cnt < 8; reg_cnt ++) {
        reg_addr = PE_0_ICE_RAM_CTRL + reg_cnt*reg_offset;
        //FPP_PROG_EN: Disable access to Flow Post-processor Micro-engine program RAM
	    writel(0x00000000, AXI4S_EIP_BASE + reg_addr);  
    }
}

void eth_eip197_ipue_firmware_load(void){
    uint32_t read_data, reg_addr;
    uint32_t reg_offset;
    int reg_cnt;

    reg_offset = 0x2000;
    //Input Classification Engine Pull-up Micro-Engine Control/Status Registers
	//SW Reset
    for(reg_cnt = 0; reg_cnt < 8; reg_cnt ++) {
        reg_addr = PE_0_ICE_PUE_CTRL + reg_cnt*reg_offset;
	    writel(0x00000001, AXI4S_EIP_BASE + reg_addr);  
    }

	//Input Classification Engine RAM Access Control Register
    for(reg_cnt = 0; reg_cnt < 8; reg_cnt ++) {
        reg_addr = PE_0_ICE_RAM_CTRL + reg_cnt*reg_offset;
        //PUE_PROG_EN: enable access to Flow Post-processor Micro-engine program RAM
	    writel(0x00000001, AXI4S_EIP_BASE + reg_addr);  
    }
	uint32_t ipue_address = 0xe0000;
    uint32_t ipue_i = 0;
    for (ipue_i=0; ipue_i<2034; ipue_i++)
    {
        writel(ipue_data_arr[ipue_i], AXI4S_EIP_BASE + ipue_address);
        ipue_address = ipue_address + 4;
    }

    //Input Classification Engine RAM Access Control Register
    for(reg_cnt = 0; reg_cnt < 8; reg_cnt ++) {
        reg_addr = PE_0_ICE_RAM_CTRL + reg_cnt*reg_offset;
        //PUE_PROG_EN: Disable access to Flow Post-processor Micro-engine program RAM
	    writel(0x00000000, AXI4S_EIP_BASE + reg_addr);  
    }
}

//*******************************Polling OFPPn*****************************************************//
int eth_eip_polling_ofpp_firmware(void) {
    return eth_eip_polling_firmware("OFPP", PE_0_OCE_SCRATCH_RAM, OFPP_FW_STATUS);
}

//*******************************Polling OPUEn*****************************************************//
int eth_eip_polling_opue_firmware(void) {
    return eth_eip_polling_firmware("OPUE", PE_0_OCE_SCRATCH_RAM, OPUE_FW_STATUS);
}

void eth_eip197_ofpp_firmware_load(void){
    uint32_t read_data, reg_addr;
    uint32_t reg_offset;
    int reg_cnt;

    reg_offset = 0x2000;
    //Output Classification Engine Flow Post-processor Micro-engine Control/Status Registers
	// SW Reset
    
    for(reg_cnt = 0; reg_cnt < 8; reg_cnt ++) {
        reg_addr = PE_0_OCE_FPP_CTRL + reg_cnt*reg_offset;
	    writel(0x00000001, AXI4S_EIP_BASE + reg_addr);  
    }
	//# EIP197.PE_0.OCE: ClassifRAMEnable(ObjectName=EIP197.PE_0.OCE.FPP, Enable=1)

	//Output Classification Engine RAM Access Control Register
    for(reg_cnt = 0; reg_cnt < 8; reg_cnt ++) {
        reg_addr = PE_0_OCE_RAM_CTRL + reg_cnt*reg_offset;
        //FPP_PROG_EN: enable access to Flow Post-processor Micro-engine program RAM
	    writel(0x00000002, AXI4S_EIP_BASE + reg_addr);  
    }
	
	uint32_t ofpp_address = 0xe0000;
    uint32_t ofpp_i = 0;

    for (ofpp_i=0; ofpp_i<1261; ofpp_i++){
        writel(ofpp_data_arr[ofpp_i], AXI4S_EIP_BASE + ofpp_address);
        ofpp_address = ofpp_address + 4;
    }
    
	//# EIP197.PE_0.OCE: ClassifRAMEnable(ObjectName=EIP197.PE_0.OCE.FPP, Enable=0)

	//Output Classification Engine RAM Access Control Register
    for(reg_cnt = 0; reg_cnt < 8; reg_cnt ++) {
        reg_addr = PE_0_OCE_RAM_CTRL + reg_cnt*reg_offset;
	    writel(0x00000000, AXI4S_EIP_BASE + reg_addr);  // Disable program RAM access
    }
}

void eth_eip197_opue_firmware_load(void){
    uint32_t read_data, reg_addr;
    uint32_t reg_offset;
    int reg_cnt;

    reg_offset = 0x2000;
    //Output Classification Engine Pull-up Micro-engine Control/Status Registers
	//SW Reset
    for(reg_cnt = 0; reg_cnt < 8; reg_cnt ++) {
        reg_addr = PE_0_OCE_PUE_CTRL + reg_cnt*reg_offset;
	    writel(0x00000001, AXI4S_EIP_BASE + reg_addr);  
    }
	
	//Output Classification Engine RAM Access Control Register
    for(reg_cnt = 0; reg_cnt < 8; reg_cnt ++) {
        reg_addr = PE_0_OCE_RAM_CTRL + reg_cnt*reg_offset;
        //PUE_PROG_EN: enable access to Pull-up Micro-engine program RAM.
	    writel(0x00000001, AXI4S_EIP_BASE + reg_addr);  
    }
	
	uint32_t opue_address = 0xe0000;
    uint32_t opue_i = 0;
    for (opue_i=0; opue_i<1259; opue_i++)
    {
        writel(opue_data_arr[opue_i], AXI4S_EIP_BASE + opue_address);
        opue_address = opue_address + 4;
    }
	
	//# EIP197.PE_0.OCE: ClassifRAMEnable(ObjectName=EIP197.PE_0.OCE.PUE, Enable=0)

	//Output Classification Engine RAM Access Control Register
    for(reg_cnt = 0; reg_cnt < 8; reg_cnt ++) {
        reg_addr = PE_0_OCE_RAM_CTRL + reg_cnt*reg_offset;
       //PUE_PROG_EN: Disable program RAM access
	    writel(0x00000000, AXI4S_EIP_BASE + reg_addr);  
    }
}

/*========================= OCE =========================*/

// Load micro-engines firmware (OFPP, OPUE)
static void eth_eip197_load_outbound_firmware(void)
{
    eth_eip197_ofpp_firmware_load();
    eth_eip197_opue_firmware_load();
}

/*========================= ICE =========================*/

// Load micro-engines firmware (IFPP, IPUE)
static void eth_eip197_load_inbound_firmware(void)
{
    eth_eip197_ifpp_firmware_load();
    eth_eip197_ipue_firmware_load();
}

/*========================= CONFIG =========================*/

// Configure engine behavior and synchronization
static void eth_eip197_config_engines(void)
{
    // NOTE:
    // This may not be required depending on HW configuration.
    // Keep for now and verify during integration.
    eth_eip197_config_engine_sync_mask();

    // Reload configuration
    eth_eip197_config_reload_oce();
    eth_eip197_config_reload_ice();
}

/*========================= STATUS =========================*/

// Wait until all firmware is loaded and ready
int eth_eip197_wait_for_firmware_ready(void)
{
    int ret = 0;

    // Poll ICE Firmware status
    ret |= eth_eip_polling_ifpp_firmware();
    ret |= eth_eip_polling_ipue_firmware();

    // Poll OCE Firmware status
    ret |= eth_eip_polling_ofpp_firmware();
    ret |= eth_eip_polling_opue_firmware();

    return ret ? -ETIMEDOUT : 0;
}

/*========================= MAIN =========================*/

// Load and initialize all micro-engines firmware
int eth_eip197_load_micro_engines_firmware(void)
{
    /* Step 1: Load outbound firmware */
    eth_eip197_load_outbound_firmware();

    /* Step 2: Clear oce scratchpad */
    eth_eip197_clear_oce_scratchpad();

    /* Step 3: Load inbound firmware */
    eth_eip197_load_inbound_firmware();

    /* Step 4: Clear inbound scratchpad */
    eth_eip197_clear_ice_scratchpad();

    /* Step 5: Configure engines */
    eth_eip197_config_engines();

    /* Step 6: Wait until all firmware is ready */
    return eth_eip197_wait_for_firmware_ready();
}

/**
 *EIP-207s Classif.Support
 */
void eth_eip197_csram_trc_config(void) {
    //TRC_EN_0 = 1, FRC_EN_0 = 0, BANK_SEL = 0
    writel(0x00000010, AXI4S_EIP_BASE + CS_RAM_CTRL);
	//#         Record size=80 (32-bit) words, requested records=0, requested hash table size=0
	//#         Data RAM size=11520 (32-bit) words, admin RAM size=320 (114-bit) words
	//#         Nr of records=144, nr of hash table entries=1024, entries per record=7.111
	//#     Push cache in SW reset mode
    //SW_RESET
	writel(0x00000001, AXI4S_EIP_BASE + TRC_0_PARAMS);
	//#     Start building admin buffer RAM image
	//#     Build a doubly linked chain of free records:

	uint32_t ram_address = 0xe0000;
	uint32_t ram_i = 0;
	for (ram_i = 0 ; ram_i < 1088; ram_i ++ )
	{
		writel(ram_data_arr[ram_i], AXI4S_EIP_BASE + ram_address);
		ram_address = ram_address + 4;
	}

	//# EIP197.Support: ClassifRAMEnable(ObjectName=EIP197.Support.TRC0, Enable=0, Banksel=0)

	writel(0x00000000, AXI4S_EIP_BASE + CS_RAM_CTRL);
}

// Initialize the Deterministic Random Bit Generator
int eth_eip197_config_drbg(void){
    uint32_t read_data;
    ulong start;
    //Reset / disable DRBG
	writel(0x00000000, AXI4S_EIP_BASE + DRBG_CONTROL);

	read_data = 0;
    //check ready
	start = get_timer(0);
	while (1) {
		read_data = readl(AXI4S_EIP_BASE + DRBG_STATUS);
		if (read_data == 0x00000202)
		break;
		if (get_timer(start) > EIP197_DRBG_TIMEOUT_MS) {
			printf("DRBG not ready (status %x)\n", read_data);
			return -ETIMEDOUT;
		}
	}

	//# ...poll succeeded in 9 clocks (90 ns)

    writel(0x00000040, AXI4S_EIP_BASE + DRBG_GEN_BLK_SIZE);
    writel(0xb3065c36, AXI4S_EIP_BASE + DRBG_PS_AI_0);
    writel(0x159446a7, AXI4S_EIP_BASE + DRBG_PS_AI_1);
    writel(0x62034199, AXI4S_EIP_BASE + DRBG_PS_AI_2);
    writel(0xde478e79, AXI4S_EIP_BASE + DRBG_PS_AI_3);
    writel(0xbabf774d, AXI4S_EIP_BASE + DRBG_PS_AI_4);
    writel(0x6c4e4021, AXI4S_EIP_BASE + DRBG_PS_AI_5);
    writel(0x63e38237, AXI4S_EIP_BASE + DRBG_PS_AI_6);
    writel(0xab791030, AXI4S_EIP_BASE + DRBG_PS_AI_7);
    writel(0x85567cb4, AXI4S_EIP_BASE + DRBG_PS_AI_8);
    writel(0xe370eed3, AXI4S_EIP_BASE + DRBG_PS_AI_9);
    writel(0x6fdc5015, AXI4S_EIP_BASE + DRBG_PS_AI_10);
    writel(0x9c53ba67, AXI4S_EIP_BASE + DRBG_PS_AI_11);
	//Enable DRBG
	writel(0x00000404, AXI4S_EIP_BASE + DRBG_CONTROL);

	read_data = readl(AXI4S_EIP_BASE + DRBG_OPTIONS);
	return 0;
}

//# EIP197: Configure Support
	//#     'trc_init': 1(0x1)
	//#     'flue_table_size': 5(0x5)
	//#     'flue_init': 1(0x1)
	//#     'frc_init': 1(0x1)
	//#     'trc_params': {'ram_size_words': 11520, 'admin_ram_size_words': 320}
	//#     'frc_params': {'ram_size_words': 3072, 'admin_ram_size_words': 272}
	//# EIP197.Support.TRC0: Init(RAM=11520 words, record=80 (act_record=80) words, small record=64 words, combined=0)
	//# EIP197.Support.TRC0: InitCache: Full initialization
	//# EIP197.Support: ClassifRAMEnable(ObjectName=EIP197.Support.TRC0, Enable=1, Banksel=0)
void eth_eip197_config_classification_support(void) {
    uint32_t write_loc;
	eth_eip197_csram_trc_config();
    //#     Write head and tail pointers
    //HEAD_POINTER= 0 | TAIL_POINTER = 0x8F
	writel(0x008f0000, AXI4S_EIP_BASE + TRC_0_FREECHAIN);
	//#     Take cache out of reset with correct parameters
    /*
    31     27 26           18 17      10 9                0
    +---------+---------------+----------+----------------+
    |Reserved | RECORD_SIZE2  |DMA_COMBD | HASH_TABLE_START|
    +---------+---------------+----------+----------------+
    */

   /*
    ┌───────────────────────────────┐
    │ Transform Record Admin RAM    │
    │───────────────────────────────│
    │ Record[0]  ─┐                 │
    │ Record[1]   │                 │
    │ Record[2]   ├─→  < HASH_TABLE_START = 64 >
    │ ...         │                 │
    │ Record[63] ─┘                 │
    │───────────────────────────────│
    │ Hash Table Entries start here │
    │───────────────────────────────│
    │ Hash[0]                      │
    │ Hash[1]                      │
    │ ...                           │
    └───────────────────────────────┘
*/
// TRC_0_PARAMS2 = 0x01001C90
// [26:18] RECORD_SIZE2 = 0x40 (64 words = 256 bytes) → size of the small transform record
// [17:10] DMA_WR_COMB_DLY = 0x07 (7 cycles) → debug feature for DMA write combining (should normally be 0)
// [9:0]   HASH_TABLE_START = 0x90 (index 144) → hash table starts at record index 144 in admin RAM
	writel(0x01001c90, AXI4S_EIP_BASE + TRC_0_PARAMS2);

// Configure Transform Record Cache 0 TRC_0_PARAMS
// -----------------------------------------------------------
// 0x01400450 = 0000 0001 0100 0000 0000 0100 0101 0000
// Bit 31   : DMA_WR_ERR     = 0   → No DMA write error
// Bit 30   : DMA_RD_ERR     = 0   → No DMA read error
// Bit 29   : DMA_ERR_ACC    = 0   → Error accumulation disabled
// Bit 28   : CTRL_ERROR     = 0   → No control logic error
// Bit 27   : Reserved       = 0
// Bits26:18: RECORD_SIZE    = 0x50 (80 words = 320 bytes per record)
// Bits17:16: Reserved       = 0
// Bit 15   : CACHE_LOCKED   = 0   → Cache not locked
// Bit 14   : WIPING_DATA    = 0   → Not wiping cache
// Bit 13   : Reserved       = 0
// Bits12:10: BLOCK_TIMEBASE = 0x1 → Debug tick every 1 cycles
// Bits9:8  : Reserved       = 0
// Bits7:4  : HASH_TABLE_SIZE= 0x5 → 2^(5+5)  = 1024 entries
// Bit 3    : DATA_ACCESS    = 0   → Data RAM access enabled
// Bit 2    : NO_BLOCK       = 0   → Host can be blocked if cache busy
// Bit 1    : Reserved       = 0
// Bit 0    : SW_RESET       = 0   → No software reset
// -----------------------------------------------------------
	writel(0x01400450, AXI4S_EIP_BASE + TRC_0_PARAMS);
	//# EIP197.Support.FLUE: Init(Tables=12, CacheEnabled=0)
	//# EIP197.Support.FLUE: HashInit(9afa758f27e9e0de9347ab9ef4d801ba)

    /*
    FHASH_IV_0 (Read/Write), Byte Address 0xF68C0 
    FHASH_IV_1 (Read/Write), Byte Address 0xF68C4 
    FHASH_IV_2 (Read/Write), Byte Address 0xF68C8 
    FHASH_IV_3 (Read/Write), Byte Address 0xF68CC
     */
	uint32_t data_arr_22[] = {
	//0x8f75fa9a, 0xdee0e927, 0x9eab4793, 0xba01d8f4
	IV_0, IV_1, IV_2, IV_3 
	};
	write_loc = 0x000f68c0;
	for (int da_index = 0; da_index < 4; da_index++) {
		writel(data_arr_22[da_index], AXI4S_EIP_BASE + write_loc);
		write_loc += 4;
	}

    // [31:24] XFORM_REC_OFFSET = 0x04
    //     → Word offset (4 bytes per word) within the Flow Record where
    //       the Transform Record pointer is located.
    //     → 0x04 * 4 bytes = 16 bytes offset from the start of Flow Record.
    //       This means the Transform Record pointer field begins at offset +16B
    //       in each Flow Record stored in host memory.
    //       It is used by the Flow Record Cache to prefetch the Transform Record.
	writel(0x04000000, AXI4S_EIP_BASE + FLUE_OFFSETS); 

    // ----------------------------------------------------
    // Bit [31] = 1 → ACCESS_ENABLE: Allows host access to the lookup table
    // Bit [30] = 1 → TABLE_ENABLE: Enables the lookup table (flow/transform record lookup active)
    // Bit [29:27] = 000 → PROTMASK: No AXI access restrictions (full access allowed)
    // Bit [26:24] = 111b (7) → GENERATION: Current record generation = 7
    // Bit [23:16] = 00000000 → VMID: Virtual Machine ID = 0 (table assigned to VM0)
    // Bit [15:8]  = 00000000 → Reserved
    // Bit [7:4]   = 0101b (5) → TABLE_SIZE: 2^(N+11) = 2^(5+11) = 128 KB hash table, 2^(5+5) = 1024 entries
    // Bit [3]     = 0 → Reserved
    // Bit [2]     = 1 → PREFETCH_TRC: Enable transform record prefetch
    // Bit [1]     = 0 → PREFETCH_DIS: Prefetch is not disabled
    // Bit [0]     = 0 → Reserved
    // ----------------------------------------------------
    //AX_BASE_WRITE_REG32(AXI4S_EIP_BASE, FLUE_CONFIG_0, 0xC700005C);
}

/*
| **Component**   | **Configuration Field** | **Value (Hex)** | **Count** | **Notes**                                 |
| --------------- | ----------------------- | --------------- | --------- | ----------------------------------------- |
| **Rings (CDR)** | `N_RINGS [3:0]`         | `0x4`           | 4         | Number of Command Descriptor Rings        |
| **Look-aside**  | `N_LA_IF [3:0]`         | `0x1`           | 1         | Number of Look-aside I/O interface pairs  |
| **Inline**      | `N_INLINE_IF [7:4]`     | `0x1`           | 1         | Number of Inline streaming I/O interfaces |
*/
void eth_eip197_config_ifc_lut(void){
    uint32_t read_data;

    //IFC[3:0]: select table 0
    for(int count=0; count < 4; count ++) {
        read_data = readl(AXI4S_EIP_BASE + FLUE_IFC_LUT_0); //Default val is 0x03020100
        read_data &= ~(0xff << (count * 8));
        //read_data |= (0x1 << (count * 8));
        //printf("FLUE_IFC_LUT_0 :%x\n", read_data);
        writel(read_data, AXI4S_EIP_BASE + FLUE_IFC_LUT_0);
    }
    //IFC[7:4]: select table 0
    for(int count=0; count < 4; count ++) {
        read_data = readl(AXI4S_EIP_BASE + FLUE_IFC_LUT_1);
        read_data &= ~(0xff << (count * 8));
        //read_data |= (0x1 << (count * 8));
        //printf("FLUE_IFC_LUT_1 :%x\n", read_data);
        writel(read_data, AXI4S_EIP_BASE + FLUE_IFC_LUT_1);
    }
        //IFC[11:8]: select table 0
    for(int count=0; count < 4; count ++) {
        read_data = readl(AXI4S_EIP_BASE + FLUE_IFC_LUT_2); 
        read_data &= ~(0xff << (count * 8));
        //read_data |= (0x1 << (count * 8));
        writel(read_data, AXI4S_EIP_BASE + FLUE_IFC_LUT_2);
    }
        //IFC[15:12]: select table 0
    for(int count=0; count < 4; count ++) {
        read_data = readl(AXI4S_EIP_BASE + FLUE_IFC_LUT_3); 
        read_data &= ~(0xff << (count * 8));
        //read_data |= (0x1 << (count * 8));
        writel(read_data, AXI4S_EIP_BASE + FLUE_IFC_LUT_3);
    }
}

void eth_eip197_config_flue_lut_enable(void){
    //CS_OPTIONS: 0xB4001F03 //12 lookup tables
    //Enable 12 LUT Tables.
    //access_enable = 1 | table_enable = 1
    uint32_t flue_config_base = FLUE_CONFIG_0, read_data;

    for(int i=0; i<12; i++) {
        writel(0xC701005C, AXI4S_EIP_BASE + (flue_config_base + (i*0x20)));
    }

}

/**
 * EIP207 Configuration
 */
int eth_eip207_config(void){
	int ret;

    // Initialize the Deterministic Random Bit Generator
	ret = eth_eip197_config_drbg();
	if (ret)
		return ret;

	//Config input interface use table 0
	eth_eip197_config_ifc_lut();

	//Enable 12 LUT Tables
	eth_eip197_config_flue_lut_enable();

	eth_eip197_config_classification_support();
	return 0;
}

/*
RDR0 base address → [ Entry 0 ] (48 bytes)
                     [ Padding ] (16 bytes)
                     [ Entry 1 ] (48 bytes)
                     [ Padding ] (16 bytes)
                     ...
                     [ Entry 255 ]

*/
void eth_eip197_config_RDR_ring(uint32_t b_len) {
    uint32_t read_data, reg_addr;
    uint32_t reg_offset;
    int reg_cnt;

    reg_offset = 0x1000;
    
    //PAD_TO_OFFSET |WR_OWN_NOWAIT|WR_CTRL_NOWAIT|WR_RES_NOWAIT|Noncacheable and nonbufferable
    for(reg_cnt = 0; reg_cnt < 12; reg_cnt ++) {
        reg_addr = HIA_RDR_0_DMA_CFG + reg_cnt*reg_offset;
	    writel(0x11c00000, AXI4S_EIP_BASE + reg_addr);
    }

    //# EIP197.EIP202.RDR0: Init(Entries=256, EntrySize=48, EntryOffset=64, TCM=0, 64BitMode=1, OwnershipWrite=1, SingleWr=2)
    //Mode 64 bit -> HIA_RDR_0_DATA_BASE_ADDR_LO = 0x00000000
    for(reg_cnt = 0; reg_cnt < 12; reg_cnt ++) {
        reg_addr = HIA_RDR_0_DATA_BASE_ADDR_LO + reg_cnt*reg_offset;
	    writel(0x00000000, AXI4S_EIP_BASE + reg_addr);  
    }
    //#     Ring base address
    for(reg_cnt = 0; reg_cnt < 12; reg_cnt ++) {
        reg_addr = HIA_RDR_0_RING_BASE_ADDR_LO + reg_cnt*reg_offset;
	    writel(RDR_RING_BASE_ADDR, AXI4S_EIP_BASE + reg_addr); 
    }
    //AX_BASE_WRITE_REG32(AXI4S_EIP_BASE, HIA_RDR_0_RING_BASE_ADDR_LO, RDR_RING_BASE_ADDR);
    //#     Ring size

    //Write (num_entries*rd_offset*4) to HIA_RDR_y_RING_SIZE: 256x16x4 = 0x4000
    //AX_BASE_WRITE_REG32(AXI4S_EIP_BASE, HIA_RDR_0_RING_SIZE, 0x00004000); // KM
    //This test write: 0x40 (64 decimal) = num_entries × rd_offset × 4 → num_entries × rd_offset = 16 => num_entries = 1

    for(reg_cnt = 0; reg_cnt < 12; reg_cnt ++) {
        reg_addr = HIA_RDR_0_RING_SIZE + reg_cnt*reg_offset;
        //0x40 (64 decimal) = num_entries × rd_offset × 4 → num_entries × rd_offset = 16 => num_entries = 1
	    writel(b_len * 0x00000040, AXI4S_EIP_BASE + reg_addr);  //remove last 2 bit: 0x10 = 16 word = 64 bytes = rd_offset 
    }
    //#     Set sizes (in 32-bit words)
	// rd_offset= 0x10 = 64 words || rd_size = 0xc = 48 words
    for(reg_cnt = 0; reg_cnt < 12; reg_cnt ++) {
        reg_addr = HIA_RDR_0_DESC_SIZE + reg_cnt*reg_offset;
	    writel(0x8010000c, AXI4S_EIP_BASE + reg_addr);  
    }

    //#     FIFO usage control
    //# EIP197.EIP202.RDR0:     Requested fetch count: 19
	//write (owm_enable « 31) || (fetch_count*(64bit*2+2)) « 16) || (fetch_count*rd_offset)
	//fetch_count*rd_offset = 19x16 = 0x130
	//fetch_count*(64bit*2+2) = 19*(1*2+2) = 0x4c
	// RD_FETCH_THRESH = 0x4c | RD_FETCH_SIZE = 0x130
    for(reg_cnt = 0; reg_cnt < 12; reg_cnt ++) {
        reg_addr = HIA_RDR_0_CFG + reg_cnt*reg_offset;
	    writel(0x804c0130, AXI4S_EIP_BASE + reg_addr);  
    }

	//HIA Prepared Result Descriptors Count Register
    //#     Clear descriptors counters
    for(reg_cnt = 0; reg_cnt < 12; reg_cnt ++) {
        reg_addr = HIA_RDR_0_PREP_COUNT + reg_cnt*reg_offset;
	    writel(0x80000000, AXI4S_EIP_BASE + reg_addr);  
    }

	//HIA Processed Result Descriptors Count Register
	//#     Clear descriptors counters
    for(reg_cnt = 0; reg_cnt < 12; reg_cnt ++) {
        reg_addr = HIA_RDR_0_PROC_COUNT + reg_cnt*reg_offset;
	    writel(0x80000000, AXI4S_EIP_BASE + reg_addr);  
    }

    //# EIP197.EIP202.RDR0:     Decrement batch size: 8192
    //(re-)initialize the RDR; this resets the prep_rdr_pntr pointer to all zeroes.
    for(reg_cnt = 0; reg_cnt < 12; reg_cnt ++) {
        reg_addr = HIA_RDR_0_PREP_PNTR + reg_cnt*reg_offset;
	    writel(0x00000000, AXI4S_EIP_BASE + reg_addr);  
    }

    //re-)initialize the RDR; this resets the proc_rdr_pntr pointer to all zeroes.
    for(reg_cnt = 0; reg_cnt < 12; reg_cnt ++) {
        reg_addr = HIA_RDR_0_PROC_PNTR + reg_cnt*reg_offset;
	    writel(0x00000000, AXI4S_EIP_BASE + reg_addr);  
    }

	//Write (proc_timeout « 24) || (proc_threshold*rd_offset) to HIA_RDR_y_THRESH.
    //#     Threshold (Before clearing any interrupts)
    for(reg_cnt = 0; reg_cnt < 12; reg_cnt ++) {
        reg_addr = HIA_RDR_0_THRESH + reg_cnt*reg_offset;
	    writel(0x00000000, AXI4S_EIP_BASE + reg_addr);  
    }

    //#     Clear any pending interrupts
    for(reg_cnt = 0; reg_cnt < 12; reg_cnt ++) {
        reg_addr = HIA_RDR_0_STAT + reg_cnt*reg_offset;
	    writel(0x000000ff, AXI4S_EIP_BASE + reg_addr);  
    }

    //# EIP197.EIP202.RDR0: Init_final(EntrySize=48, EntryOffset=64, FetchCount=19, ThreshCount=19, MaxFetchCount=511, FetchThresh=76, FetchSize=304)
    // FetchSize=304)
    //#     Ring number: 0
    //#     Ring entries: 256
    //#     Ring entry size: 48
    //#     Ring entry offset: 64
    //#     Ring base address: 0x100400000
    //#     Assign RDR0 to engine: {'slot': 15, 'eng_no': 0, 'prio': 0}
    
    //Will try do delete later
	//Write 0x40000000 to all HIA_DSE_n_THR_CTRL registers to enables the thread for Processing Engine
    //AX_BASE_WRITE_REG32(AXI4S_EIP_BASE, HIA_DSE_0_THR_CTRL, 0x40000001);
}

void eth_eip197_assign_IFC_to_allPE(uint8_t eip_cdr) {
    uint32_t ra_pe_n_ctrl_val;
    //0x90010 + (n*8)
    for (int eip_pe = 0; eip_pe < 8; eip_pe ++) {
        uint32_t hia_ra_pe_n_ctl = HIA_RA_PE_0_CTRL + eip_pe*8;
        ra_pe_n_ctrl_val = 0x40000000 | EIP_INLINE_IFC | EIP_LAFIFO_IFC | (1 << eip_cdr);
        //printf("ra_pe_n_ctrl_val: %x\n", ra_pe_n_ctrl_val);
        //#     Assign Inline Ifc 0 to engine: {'slot': 15, 'eng_no': 0, 'prio': 0}
        writel(ra_pe_n_ctrl_val, AXI4S_EIP_BASE + hia_ra_pe_n_ctl);  //0011 1000: IFC3, LAFIFO and Inline Interface
    }
}

void eth_eip197_redirect_src_ifc_to_inline_interface_allPE(uint8_t eip_cdr) {
    //redirect ifcy (CDRy) to inline interface (always 15 in egress transform record)
    for (int eip_pe = 0; eip_pe < 8; eip_pe ++) {
        uint32_t PE_n_ICE_SCRATCH_RAM = PE_0_ICE_SCRATCH_RAM + eip_pe*8*1024;
        uint32_t transform_redirect_en_offset = 8*4; //word 8th
        uint32_t PE_n_ICE_SCRATCH_RAM_val = (1 << 15) | (1 << eip_cdr); //Bit 15: Inline Interface
    /* 
        printf("==========================================================\n");
        printf("PE_n_ICE_SCRATCH_RAM + transform_redirect_en_offset: %x\n", PE_n_ICE_SCRATCH_RAM + transform_redirect_en_offset);
        printf("PE_n_ICE_SCRATCH_RAM_val: %x\n", PE_n_ICE_SCRATCH_RAM_val);
    */
        writel(PE_n_ICE_SCRATCH_RAM_val, AXI4S_EIP_BASE + (PE_n_ICE_SCRATCH_RAM + transform_redirect_en_offset)); //Transform Redirect Enable[15:0]
    }
}

void eth_eip197_config_CDR_ring(uint32_t b_len) {
    uint32_t read_data, reg_addr;
    uint32_t reg_offset;
    int reg_cnt;

    reg_offset = 0x1000;

    //# EIP197.EIP202.CDR0: Init(Entries=256, EntrySize=48, EntryOffset=64, AddToken=1, TCM=0, 64BitMode=1)
    //#     Program data base address. Due to mode 64 bit, then ADDR_LO = 0
    for(reg_cnt = 0; reg_cnt < 12; reg_cnt ++) {
        reg_addr = HIA_CDR_0_DATA_BASE_ADDR_LO + reg_cnt*reg_offset;
	    writel(0x00000000, AXI4S_EIP_BASE + reg_addr);  
    }
    //#     Program additional token base address. Due to mode 64 bit, then ADDR_LO = 0
    for(reg_cnt = 0; reg_cnt < 12; reg_cnt ++) {
        reg_addr = HIA_CDR_0_ATOK_BASE_ADDR_LO + reg_cnt*reg_offset;
	    writel(0x00000000, AXI4S_EIP_BASE + reg_addr);  
    }
    //#     Temporarily disable ext triggerring
    for(reg_cnt = 0; reg_cnt < 12; reg_cnt ++) {
        reg_addr = HIA_CDR_0_CFG + reg_cnt*reg_offset;
	    writel(0x00000000, AXI4S_EIP_BASE + reg_addr);  
    }
    //#     Clear pending prepared descriptors counter
    for(reg_cnt = 0; reg_cnt < 12; reg_cnt ++) {
        reg_addr = HIA_CDR_0_COUNT + reg_cnt*reg_offset;
	    writel(0x80000000, AXI4S_EIP_BASE + reg_addr);  
    }
    //#     Clear pending prepared descriptors pointer
    for(reg_cnt = 0; reg_cnt < 12; reg_cnt ++) {
        reg_addr = HIA_CDR_0_PREP_PNTR + reg_cnt*reg_offset;
	    writel(0x00000000, AXI4S_EIP_BASE + reg_addr);  
    }
    //#     Clear pending ownership update pointer
    for(reg_cnt = 0; reg_cnt < 12; reg_cnt ++) {
        reg_addr = HIA_CDR_0_PROC_PNTR + reg_cnt*reg_offset;
	    writel(0x00000000, AXI4S_EIP_BASE + reg_addr);  
    }
    //#     Ring size
    for(reg_cnt = 0; reg_cnt < 12; reg_cnt ++) {
        reg_addr = HIA_CDR_0_RING_SIZE + reg_cnt*reg_offset;
	    writel(b_len * 0x00000040, AXI4S_EIP_BASE + reg_addr);  //1 entry
    }
    //#     Ring base address
    for(reg_cnt = 0; reg_cnt < 12; reg_cnt ++) {
        reg_addr = HIA_CDR_0_RING_BASE_ADDR_LO + reg_cnt*reg_offset;
	    writel(CDR_RING_BASE_ADDR, AXI4S_EIP_BASE + reg_addr);
    }
    //AX_BASE_WRITE_REG32(AXI4S_EIP_BASE, HIA_CDR_0_RING_BASE_ADDR_LO, CDR_RING_BASE_ADDR);
    //#     Clear pending processed descriptors counter
    for(reg_cnt = 0; reg_cnt < 12; reg_cnt ++) {
        reg_addr = HIA_CDR_0_PROC_COUNT + reg_cnt*reg_offset;
	    writel(0x80000000, AXI4S_EIP_BASE + reg_addr);
    }
    //#     Configure data swapping: no swap, nowait AXI response
    for(reg_cnt = 0; reg_cnt < 12; reg_cnt ++) {
        reg_addr = HIA_CDR_0_DMA_CFG + reg_cnt*reg_offset;
	    writel(0x01000000, AXI4S_EIP_BASE + reg_addr);
    }
    //#     Set sizes (in 32-bit words)
	//CD_SIZE = 0xc | CD_OFFSET = 0x10 | MODE_64_BIT | ACDP_PRESENT
    for(reg_cnt = 0; reg_cnt < 12; reg_cnt ++) {
        reg_addr = HIA_CDR_0_DESC_SIZE + reg_cnt*reg_offset;
	    writel(0xc010000c, AXI4S_EIP_BASE + reg_addr);
    }
    //#     Clear any pending interrupts
    for(reg_cnt = 0; reg_cnt < 12; reg_cnt ++) {
        reg_addr = HIA_CDR_0_STAT + reg_cnt*reg_offset;
	    writel(0x0000003f, AXI4S_EIP_BASE + reg_addr);
    }
    //#     FIFO usage control
    //# EIP197.EIP202.CDR0:     Requested fetch count: 20
	//CD_FETCH_THRESH:0xf0 | CD_FETCH_SIZE: 0x140
    for(reg_cnt = 0; reg_cnt < 12; reg_cnt ++) {
        reg_addr = HIA_CDR_0_CFG + reg_cnt*reg_offset;
	    writel(0x00f00140, AXI4S_EIP_BASE + reg_addr);
    }
}

void eth_eip_197_interface_configuration(uint32_t b_len, uint8_t eip_cdr){
    
	eth_eip197_config_RDR_ring(b_len);
	
	eth_eip197_config_CDR_ring(b_len);

    	//Advanced Interrupt Controller (AIC)
    //eth_eip197_config_AIC();

	//eth_eip197_assign_IFC_to_PE(eip_pe, eip_cdr);
	eth_eip197_assign_IFC_to_allPE(eip_cdr);

    // ******* Redirecting data onto inline interface ******** //
	//eth_eip197_redirect_src_ifc_to_inline_interface(eip_pe, eip_cdr);
	eth_eip197_redirect_src_ifc_to_inline_interface_allPE(eip_cdr);
}

