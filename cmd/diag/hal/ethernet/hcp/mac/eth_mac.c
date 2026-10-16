// SPDX-License-Identifier: GPL-2.0+
#include "eth_mac.h"
#include "../eth_common_api/eth_common_api.h"
#include <dm.h>
#include <asm/io.h>
#include <stdio.h>
#include <linux/types.h>
#include <linux/delay.h>
static inline void REG_WRITE32_DUMP_x3(u32 base_addr, u32 reg_offset,
			       u32 value)
{
	// REG_WRITE32(value, base_addr + reg_offset);
	// REG_WRITE32(value, base_addr + reg_offset);
	writel(value, base_addr + reg_offset);
}

void sgmii_phy_dump(u8 mac_idx) {
	printf("\n\n\n");
	printf("SGMII PHY DUMP\n");
    u32 SGMII_BASE = sgmii_phy_base(mac_idx);

    REG_READ32_DUMP(SGMII_BASE, 0x2c);  
    REG_READ32_DUMP(SGMII_BASE, 0x42c);
    // program_phy0_no_bandgap_rcomp_calib_sim
    REG_READ32_DUMP(SGMII_BASE, ((0x05 << 10) + (0x22 << 2))); //offset:1488
    REG_READ32_DUMP(SGMII_BASE, ((0x05 << 10) + (0x0a << 2))); //offset:1428
    REG_READ32_DUMP(SGMII_BASE, ((0x05 << 10) + (0x0b << 2))); //offset:142c
                                                                   
    // program_rc_phy_fast_detect
    REG_READ32_DUMP(SGMII_BASE, ((0x01 << 10) + (0xc3 << 2))); //offset:070c
    REG_READ32_DUMP(SGMII_BASE, ((0x01 << 10) + (0xc4 << 2))); //offset:0710
                                                                   
    // program_phy0_pll_fast_lock
    REG_READ32_DUMP(SGMII_BASE, ((0x05 << 10) + (0x6f << 2))); //offset:15bc
                                                                   
    // program_phy_0_no_offset_calib
    REG_READ32_DUMP(SGMII_BASE, ((0x02 << 10) + (0xc9 << 2))); //offset:0b24
    REG_READ32_DUMP(SGMII_BASE, ((0x02 << 10) + (0xca << 2))); //offset:0b28
    REG_READ32_DUMP(SGMII_BASE, ((0x02 << 10) + (0xa2 << 2))); //offset:0a88
    REG_READ32_DUMP(SGMII_BASE, ((0x02 << 10) + (0xa4 << 2))); //offset:0a90
    REG_READ32_DUMP(SGMII_BASE, ((0x01 << 10) + (0x0a << 2))); //offset:0428
                                                               
    // program_phy_0_fast_RxEq
    REG_READ32_DUMP(SGMII_BASE, ((0x05 << 10) + (0x6f << 2))); //offset:15bc
   
    REG_READ32_DUMP(SGMII_BASE, ((0x02 << 10) + (0xe3 << 2))); //offset:0b8c
    REG_READ32_DUMP(SGMII_BASE, ((0x02 << 10) + (0xdb << 2))); //offset:0b6c
    REG_READ32_DUMP(SGMII_BASE, ((0x02 << 10) + (0xdc << 2))); //offset:0b70
    REG_READ32_DUMP(SGMII_BASE, ((0x02 << 10) + (0xdd << 2))); //offset:0b74
    REG_READ32_DUMP(SGMII_BASE, ((0x01 << 10) + (0x99 << 2))); //offset:0664
    REG_READ32_DUMP(SGMII_BASE, ((0x05 << 10) + (0x50 << 2))); //offset:1540
    REG_READ32_DUMP(SGMII_BASE, ((0x05 << 10) + (0x51 << 2))); //offset:1544
    REG_READ32_DUMP(SGMII_BASE, ((0x05 << 10) + (0x52 << 2))); //offset:1548
    REG_READ32_DUMP(SGMII_BASE, ((0x05 << 10) + (0x53 << 2))); //offset:154c
    REG_READ32_DUMP(SGMII_BASE, ((0x05 << 10) + (0x54 << 2))); //offset:1550
    REG_READ32_DUMP(SGMII_BASE, ((0x05 << 10) + (0x55 << 2))); //offset:1554
    REG_READ32_DUMP(SGMII_BASE, ((0x05 << 10) + (0x56 << 2))); //offset:1558
    REG_READ32_DUMP(SGMII_BASE, ((0x05 << 10) + (0x57 << 2))); //offset:155c
    REG_READ32_DUMP(SGMII_BASE, ((0x05 << 10) + (0x58 << 2))); //offset:1560
    REG_READ32_DUMP(SGMII_BASE, ((0x05 << 10) + (0x59 << 2))); //offset:1564
    REG_READ32_DUMP(SGMII_BASE, ((0x05 << 10) + (0x5a << 2))); //offset:1568
    REG_READ32_DUMP(SGMII_BASE, ((0x05 << 10) + (0x5b << 2))); //offset:156c
    REG_READ32_DUMP(SGMII_BASE, ((0x05 << 10) + (0x5c << 2))); //offset:1570
    REG_READ32_DUMP(SGMII_BASE, ((0x05 << 10) + (0x5d << 2))); //offset:1574
    REG_READ32_DUMP(SGMII_BASE, ((0x02 << 10) + (0xcb << 2))); //offset:0b2c
    REG_READ32_DUMP(SGMII_BASE, ((0x02 << 10) + (0xce << 2))); //offset:0b38
    REG_READ32_DUMP(SGMII_BASE, ((0x02 << 10) + (0xcf << 2))); //offset:0b3c
 
    REG_READ32_DUMP(SGMII_BASE,((0x04 << 10) + (0x24 << 2))); //configure pll0 postdiv offset:1090
    REG_READ32_DUMP(SGMII_BASE,((0x03 << 10) + (0x75 << 2)));          //offset:0dd4
    REG_READ32_DUMP(SGMII_BASE,((0x05 << 10) + (0x24 << 2)));          //offset:1490
                                                                                       
    REG_READ32_DUMP(SGMII_BASE,((0x6 << 10) + (0x55 << 2)));           //offset:1954
    REG_READ32_DUMP(SGMII_BASE,((0x6 << 10) + (0x56 << 2)));           //offset:1958
    REG_READ32_DUMP(SGMII_BASE,((0x6 << 10) + (0x57 << 2)));           //offset:195c
                                                                                       
    // enable GTX_CLK and RX_CLK clock stop capability, on GMII interface
    REG_READ32_DUMP(SGMII_BASE,((0x6 << 10) + (0x00 << 2)));           //offset:1800
 
    REG_READ32_DUMP(SGMII_BASE, 0x0ddc);
    REG_READ32_DUMP(SGMII_BASE, 0x0800);
    REG_READ32_DUMP(SGMII_BASE, 0x0400);
    REG_READ32_DUMP(SGMII_BASE, 0x0404);
    REG_READ32_DUMP(SGMII_BASE, 0x0408);
    REG_READ32_DUMP(SGMII_BASE, 0x0bd0);
    REG_READ32_DUMP(SGMII_BASE, 0x0de0);
    REG_READ32_DUMP(SGMII_BASE, 0x0de4);
    REG_READ32_DUMP(SGMII_BASE, 0x0de8);
}

void sgmii_fast_sim(u8 mac_idx)
{
    u32 SGMII_BASE = sgmii_phy_base(mac_idx);
 
    // REG_WRITE32_DUMP_x3(SGMII_BASE,((0x04 << 10) + (0x24 << 2)), 0xc8810321); // fracpll fbdiv
    // REG_WRITE32_DUMP_x3(SGMII_BASE,((0x04 << 10) + (0x25 << 2)), 0x10322040); // fracpll fbdiv
    // REG_WRITE32_DUMP_x3(SGMII_BASE,((0x04 << 10) + (0x06 << 2)), 0x000001b0); // fracpll dacpd=dsmpd=1
    // REG_WRITE32_DUMP_x3(SGMII_BASE,((0x04 << 10) + (0x07 << 2)), 0x1c000000); // fracpll frac
   
    // program_phy0_no_bandgap_rcomp_calib_sim
    REG_WRITE32_DUMP_x3(SGMII_BASE, ((0x05 << 10) + (0x22 << 2)), 0x04083789); //offset:1488
    REG_WRITE32_DUMP_x3(SGMII_BASE, ((0x05 << 10) + (0x0a << 2)), 0x640f80a0); //offset:1428
    REG_WRITE32_DUMP_x3(SGMII_BASE, ((0x05 << 10) + (0x0b << 2)), 0x20001f01); //offset:142c
                                                                               
    // program_rc_phy_fast_detect
    REG_WRITE32_DUMP_x3(SGMII_BASE, ((0x01 << 10) + (0xc3 << 2)), 0x00800000); //offset:070c
    REG_WRITE32_DUMP_x3(SGMII_BASE, ((0x01 << 10) + (0xc4 << 2)), 0x00a00090); //offset:0710
                                                                               
    // program_phy0_pll_fast_lock
    REG_WRITE32_DUMP_x3(SGMII_BASE, ((0x05 << 10) + (0x6f << 2)), 0x00010001); //offset:15bc
                                                                               
    // program_phy_0_no_offset_calib
    REG_WRITE32_DUMP_x3(SGMII_BASE, ((0x02 << 10) + (0xc9 << 2)), 0x00000061); //offset:0b24
    REG_WRITE32_DUMP_x3(SGMII_BASE, ((0x02 << 10) + (0xca << 2)), 0xffffff00); //offset:0b28
    REG_WRITE32_DUMP_x3(SGMII_BASE, ((0x02 << 10) + (0xa2 << 2)), 0x00020020); //offset:0a88
    REG_WRITE32_DUMP_x3(SGMII_BASE, ((0x02 << 10) + (0xa4 << 2)), 0x43009000); //offset:0a90
    REG_WRITE32_DUMP_x3(SGMII_BASE, ((0x01 << 10) + (0x0a << 2)), 0x0a0083e8); //offset:0428
                                                                           
    // program_phy_0_fast_RxEq
    REG_WRITE32_DUMP_x3(SGMII_BASE, ((0x05 << 10) + (0x6f << 2)), 0x00010001); //offset:15bc
   
    REG_WRITE32_DUMP_x3(SGMII_BASE, ((0x02 << 10) + (0xe3 << 2)), 0x00000000); //offset:0b8c
    REG_WRITE32_DUMP_x3(SGMII_BASE, ((0x02 << 10) + (0xdb << 2)), 0x00000001); //offset:0b6c
    REG_WRITE32_DUMP_x3(SGMII_BASE, ((0x02 << 10) + (0xdc << 2)), 0x00010001); //offset:0b70
    REG_WRITE32_DUMP_x3(SGMII_BASE, ((0x02 << 10) + (0xdd << 2)), 0x40000102); //offset:0b74
    REG_WRITE32_DUMP_x3(SGMII_BASE, ((0x01 << 10) + (0x99 << 2)), 0xb8000380); //offset:0664
    REG_WRITE32_DUMP_x3(SGMII_BASE, ((0x05 << 10) + (0x50 << 2)), 0xfffefffe); //offset:1540
    REG_WRITE32_DUMP_x3(SGMII_BASE, ((0x05 << 10) + (0x51 << 2)), 0xfffefffe); //offset:1544
    REG_WRITE32_DUMP_x3(SGMII_BASE, ((0x05 << 10) + (0x52 << 2)), 0xfffefffe); //offset:1548
    REG_WRITE32_DUMP_x3(SGMII_BASE, ((0x05 << 10) + (0x53 << 2)), 0xfffefffe); //offset:154c
    REG_WRITE32_DUMP_x3(SGMII_BASE, ((0x05 << 10) + (0x54 << 2)), 0xfffefffe); //offset:1550
    REG_WRITE32_DUMP_x3(SGMII_BASE, ((0x05 << 10) + (0x55 << 2)), 0xfffefffe); //offset:1554
    REG_WRITE32_DUMP_x3(SGMII_BASE, ((0x05 << 10) + (0x56 << 2)), 0xfffefffe); //offset:1558
    REG_WRITE32_DUMP_x3(SGMII_BASE, ((0x05 << 10) + (0x57 << 2)), 0xfffefffe); //offset:155c
    REG_WRITE32_DUMP_x3(SGMII_BASE, ((0x05 << 10) + (0x58 << 2)), 0xfffefffe); //offset:1560
    REG_WRITE32_DUMP_x3(SGMII_BASE, ((0x05 << 10) + (0x59 << 2)), 0xfffefffe); //offset:1564
    REG_WRITE32_DUMP_x3(SGMII_BASE, ((0x05 << 10) + (0x5a << 2)), 0xfffefffe); //offset:1568
    REG_WRITE32_DUMP_x3(SGMII_BASE, ((0x05 << 10) + (0x5b << 2)), 0xfffefffe); //offset:156c
    REG_WRITE32_DUMP_x3(SGMII_BASE, ((0x05 << 10) + (0x5c << 2)), 0xfffefffe); //offset:1570
    REG_WRITE32_DUMP_x3(SGMII_BASE, ((0x05 << 10) + (0x5d << 2)), 0x0000fffe); //offset:1574
    REG_WRITE32_DUMP_x3(SGMII_BASE, ((0x02 << 10) + (0xcb << 2)), 0xffffffff); //offset:0b2c
    REG_WRITE32_DUMP_x3(SGMII_BASE, ((0x02 << 10) + (0xce << 2)), 0xffffffff); //offset:0b38
    REG_WRITE32_DUMP_x3(SGMII_BASE, ((0x02 << 10) + (0xcf << 2)), 0xffffffff); //offset:0b3c
    //REG_WRITE32_DUMP_x3(SGMII_BASE, ((0x02 << 10) + (0xd2 << 2)), 0xffffffff); //offset:0b48
    //REG_WRITE32_DUMP_x3(SGMII_BASE, ((0x02 << 10) + (0xda << 2)), 0x01ff0000); //offset:0b68
}
void sgmii_fast_sim_other(u8 mac_idx) {
    u32 SGMII_BASE = sgmii_phy_base(mac_idx);
 
    REG_WRITE32_DUMP_x3(SGMII_BASE,((0x04 << 10) + (0x24 << 2)), 0x23810c81); //configure pll0 postdiv offset:1090
    REG_WRITE32_DUMP_x3(SGMII_BASE,((0x03 << 10) + (0x75 << 2)), 0x00000000);          //offset:0dd4
    REG_WRITE32_DUMP_x3(SGMII_BASE,((0x05 << 10) + (0x24 << 2)), 0x00f08098);          //offset:1490
                                                                                       
    REG_WRITE32_DUMP_x3(SGMII_BASE,((0x6 << 10) + (0x55 << 2)), 0x00000000);           //offset:1954
    REG_WRITE32_DUMP_x3(SGMII_BASE,((0x6 << 10) + (0x56 << 2)), 0x00000000);           //offset:1958
    REG_WRITE32_DUMP_x3(SGMII_BASE,((0x6 << 10) + (0x57 << 2)), 0x0000007e);           //offset:195c
                                                                                       
    // enable GTX_CLK and RX_CLK clock stop capability, on GMII interface
    REG_WRITE32_DUMP_x3(SGMII_BASE,((0x6 << 10) + (0x00 << 2)), 0x0000000d);           //offset:1800
}

void sgmii_innosilicon_adjust(u8 mac_idx) {
    u32 SGMII_BASE = sgmii_phy_base(mac_idx);
 
    REG_WRITE32_DUMP_x3(SGMII_BASE, 0x0ddc , 0x0000001b);
    REG_WRITE32_DUMP_x3(SGMII_BASE, 0x0800 , 0x85000002);
    REG_WRITE32_DUMP_x3(SGMII_BASE, 0x0400 , 0x00086018);
    REG_WRITE32_DUMP_x3(SGMII_BASE, 0x0404 , 0x0010a018);
    REG_WRITE32_DUMP_x3(SGMII_BASE, 0x0408 , 0x04000018);
    REG_WRITE32_DUMP_x3(SGMII_BASE, 0x0bd0 , 0x0ff03355);
    REG_WRITE32_DUMP_x3(SGMII_BASE, 0x0de0 , 0x00000098);
    REG_WRITE32_DUMP_x3(SGMII_BASE, 0x0de4 , 0x00000000);
    REG_WRITE32_DUMP_x3(SGMII_BASE, 0x0de8 , 0x00000000);
}

void sgmii_innosilicon_pi_config(u8 mac_idx) {
    u32 SGMII_BASE = sgmii_phy_base(mac_idx);
    REG_WRITE32_DUMP_x3(SGMII_BASE, 0x2c, 0x00C10C92);  
    REG_WRITE32_DUMP_x3(SGMII_BASE, 0x42c, 0x00000011);

    // REG_WRITE32_DUMP_x3(SGMII_BASE, 0x2c, 0x00810C92);  
    // REG_WRITE32_DUMP_x3(SGMII_BASE, 0x42c, 0x00000013);
}
 
int sgmii_link_try(u8 mac_idx) 
{
	int status = 1;
	u32 rval;
    u32 mac_base = XGMII_MAC_BASE + (mac_idx * MAC_CONFIG_BYTE_CNT);
    printf("Retry \n");
    printf("SGMII Reset \n");
    writel(0x10, AXILS_HCP_BASE + gmii_port_ctrl_offset(mac_idx));
    mdelay(10);
 
    sgmii_innosilicon_pi_config(mac_idx);
 
    printf("Reset TX FIFO\n");
    rval = readl(AXILS_HCP_BASE + TX_PACKET_FIFO_0);
    rval = (rval | 0x1);
    writel(rval, AXILS_HCP_BASE + TX_PACKET_FIFO_0);
    mdelay(10);
 
    printf("Reset RX FIFO\n");
    rval = readl(AXILS_HCP_BASE + RX_PACKET_FIFO_0);
    rval = (rval | 0x1);
    writel(rval, AXILS_HCP_BASE + RX_PACKET_FIFO_0);
    mdelay(10);
 
    printf("Reset MAC\n");
    rval = readl(mac_base + R_COMMAND_CONFIG);
    rval = (rval | (0x1 << 12));
    writel(rval, mac_base + R_COMMAND_CONFIG);
    mdelay(10);
 
    printf("Release Reset\n");
    writel(0x02, AXILS_HCP_BASE + gmii_port_ctrl_offset(mac_idx));
    mdelay(10);

	return status;
}

/*
 * Hardware specific config for the 10G MAC. Needed on the chip, not on
 * FPGA.
 */
void xgmii_fast_sim(void)
{
	u32 reg_val;
	u32 xgmii_base = RXAUI_BASE;
	// Release SerDes APB reset
	/* Configure registers for fast simulation */
	// Program common pstate change.
	writel(0x3feffbfe, xgmii_base + 0x1540);
	writel(0x3feffbfe, xgmii_base + 0x1544);
	writel(0x3feffbfe, xgmii_base + 0x1548);
	writel(0x3feffbfe, xgmii_base + 0x154c);
	writel(0x3feffbfe, xgmii_base + 0x1550);
	writel(0x3feffbfe, xgmii_base + 0x1554);
	writel(0x3feffbfe, xgmii_base + 0x1558);
	writel(0x3feffbfe, xgmii_base + 0x155c);
	writel(0x3feffbfe, xgmii_base + 0x1560);

	// Program Tx detect Rx for Lane 0
	writel(0x00800000, xgmii_base + 0x070c);
	writel(0x00a00090, xgmii_base + 0x0710);

	// Program pstate change for Lane 0
	writel(0x3fffffff, xgmii_base + 0x0b28);
	writel(0x3fffffff, xgmii_base + 0x0b2c);
	writel(0x3fffffff, xgmii_base + 0x0b30);
	writel(0x3fffffff, xgmii_base + 0x0b34);
	writel(0x3fffffff, xgmii_base + 0x0b38);
	writel(0x3fffffff, xgmii_base + 0x0b3c);
	writel(0x3fffffff, xgmii_base + 0x0b40);
	writel(0x3fffffff, xgmii_base + 0x0b44);
	writel(0x3fffffff, xgmii_base + 0x0b48);

	// Program Rx EQ fast simulation for Lane 0.
	writel(0x90000000, xgmii_base + 0x0664);
	writel(0x00cc0063, xgmii_base + 0x08c0);
	writel(0x40008800, xgmii_base + 0x0a90);
	writel(0x00200001, xgmii_base + 0x08d4);
	writel(0x00200001, xgmii_base + 0x08e4);
	writel(0x00108310, xgmii_base + 0x2410);

	// Program Tx detect Rx for Lane 1.
	writel(0x00800000, xgmii_base + 0x470c);
	writel(0x00a00090, xgmii_base + 0x4710);

	// Program pstate change for Lane 1.
	writel(0x3fffffff, xgmii_base + 0x4b28);
	writel(0x3fffffff, xgmii_base + 0x4b2c);
	writel(0x3fffffff, xgmii_base + 0x4b30);
	writel(0x3fffffff, xgmii_base + 0x4b34);
	writel(0x3fffffff, xgmii_base + 0x4b38);
	writel(0x3fffffff, xgmii_base + 0x4b3c);
	writel(0x3fffffff, xgmii_base + 0x4b40);
	writel(0x3fffffff, xgmii_base + 0x4b44);
	writel(0x3fffffff, xgmii_base + 0x4b48);

	// Program Rx EQ fast simulation for Lane 1.
	writel(0x90000000, xgmii_base + 0x4664);
	writel(0x00cc0063, xgmii_base + 0x48c0);
	writel(0x40008800, xgmii_base + 0x4a90);
	writel(0x00200001, xgmii_base + 0x48d4);
	writel(0x00200001, xgmii_base + 0x48e4);
	writel(0x00108310, xgmii_base + 0x6410);

	// Program bypass rcomp.
	writel(0x3c083089, xgmii_base + 0x1488);
	writel(0x501f80c0, xgmii_base + 0x1428);
	writel(0x20000001, xgmii_base + 0x142c);

	// Program bypass offset.
	writel(0x00000000, xgmii_base + 0x0b24);
	writel(0x00100400, xgmii_base + 0x0a60);
	writel(0x0a1083e8, xgmii_base + 0x0428);
	writel(0x00000000, xgmii_base + 0x4b24);
	writel(0x00100400, xgmii_base + 0x4a60);
	writel(0x0a1083e8, xgmii_base + 0x4428);
	writel(0x120010fa, xgmii_base + 0x107c);
	writel(0x000d0800, xgmii_base + 0x1048);
	writel(0x0060c533, xgmii_base + 0x109c);

	// Release PMA POR reset
	// Wait for PMA stability
	reg_val = 0;
	uint16_t timeout = 100;
	while (!(reg_val & (1 << 1))) //wait bit 1 = 1
	{
		reg_val = readl(RXAUI_BASE + 0x2c77);
		mdelay(50);
		timeout--;
		if (timeout == 0) {
			printf(
				"Time out Wait for PMA stability - address 16'h2c77[1]\n");
			break;
		}
	}
	reg_val = 0;
	timeout = 100;
	while (!(reg_val & (1 << 1))) //wait bit 1 = 1
	{
		reg_val = readl(RXAUI_BASE + 0x6c77);
		mdelay(50);
		timeout--;
		if (timeout == 0) {
			printf(
				"Time out Wait for PMA stability - address 16'h6c77[1]\n");
			break;
		}
	}

	// Configure PCS registers
	reg_val = readl(RXAUI_BASE + 0x808c);
	reg_val |= (1 << 1);
	writel(reg_val, xgmii_base + 0x808c); //bit1=0 for Dune mode; bit1=1 for Marvell mode

	reg_val = readl(RXAUI_BASE + 0x8080);
	reg_val |= (1 << 15);
	writel(reg_val, xgmii_base + 0x8080); //bit15 to initiate vendor soft-reset and wait for the hardware toclear the value for the bit[15].

	readl(xgmii_base + 0x8084); //check bit2 = 1

	reg_val = 0;
	timeout = 100;
	while (!(reg_val & (1 << 2))) //wait bit 2 = 1
	{
		reg_val = readl(RXAUI_BASE + 0x8084);
		mdelay(50);
		timeout--;
		if (timeout == 0) {
			printf("Time out Wait for RLU (bit[2])\n");
			break;
		}
	}
}
/* Reset the SGMII block of the port (port control bit 4) */
void mac_reset_sgmii(u8 mac_idx)
{
	u32 gmii_port_ctrl;
	int reg_val, auto_neg_mon,sgmii_reset_val,sgmii_reset;
	volatile u32 read_data,read_data_3;
	gmii_port_ctrl =
		readl(AXILS_HCP_BASE + gmii_port_ctrl_offset(mac_idx));
	gmii_port_ctrl |= (1 << 4); //Set bit 4 (SGMII Reset)
	writel(gmii_port_ctrl, AXILS_HCP_BASE + gmii_port_ctrl_offset(mac_idx));
	mdelay(10);
	gmii_port_ctrl =
		readl(AXILS_HCP_BASE + gmii_port_ctrl_offset(mac_idx));
	gmii_port_ctrl &= ~(1 << 4); //Clear bit 4 (SGMII Reset)
	writel(gmii_port_ctrl, AXILS_HCP_BASE + gmii_port_ctrl_offset(mac_idx));
	mdelay(50);
}
/* MAC 1G operation */
int mac_reset_1g(u8 mac_idx)
{
	u32 mac_base = XGMII_MAC_BASE + (mac_idx * MAC_CONFIG_BYTE_CNT);
	u32 regval;

	writel(MAC_CC_NULL, mac_base + R_COMMAND_CONFIG);
	writel((1 << 12), mac_base + R_COMMAND_CONFIG); //Reset MAC

	int timeout = 100;
	do {
		regval = readl(mac_base + R_COMMAND_CONFIG);
		if ((regval & (1 << 12)) == 0) {
			break; // Bit 12 Clear <=> Reset Done
		}
		mdelay(100);
		timeout--;
	} while (timeout > 0);

	if (timeout == 0) {
		printf("MAC SW_RESET timeout!\n");
		return 0;
	}
	printf("MAC Reset done\n");
	return 1;
}

void mac_init_1g(u8 mac_idx)
{
	u32 mac_base = XGMII_MAC_BASE + (mac_idx * MAC_CONFIG_BYTE_CNT);

	/* MAC address 00:11:22:33:44:55 (XG_R_MAC_0/1_VAL) */
	writel(XG_R_MAC_0_VAL, mac_base + R_MAC_0);

	writel(XG_R_MAC_1_VAL, mac_base + R_MAC_1);

	/* 
	 * Adding multicast address. 
	 * This will accept any mac address whose 6-bit hash value equal to  111001
	 * Example: 01:01:01:00:00:01
	 */
	writel(0x000001e7, mac_base + R_HASHTABLE_LOAD);

	/* 
	 * This will accept any mac address whose 6-bit hash value equal to  110001
	 * Example: 01:01:00:00:00:01 
	 */
	writel(0x00000123, mac_base + R_HASHTABLE_LOAD);

	/* Configure RX / TX FIFO Sections*/
	writel(MAC_1G_R_RX_FIFO_VAL, mac_base + R_RX_FIFO_SECTIONS);
	writel(MAC_1G_R_TX_FIFO_VAL, mac_base + R_TX_FIFO_SECTIONS);

	/* Configure the MAC-COMMAND with specific bits */
	writel(MAC_CC_NULL |
			    MAC_CC_TX_ENA | /* Enable outbound flow of pkts. */
			    MAC_CC_RX_ENA | /* Enable inbound flow of pkts. */
			    MAC_CC_PROMIS_EN | /* Enable promiscuous mode */
				MAC_CC_PAUSE_FWD |
				MAC_CC_CNTL_FRM_ENA |
			    MAC_CC_TX_PAD_EN, mac_base + R_COMMAND_CONFIG); /* Enable padding to the frame */

	/* Set frame length value */
	writel(MAC_1G_R_FRM_LENGTH_VAL, mac_base + R_FRM_LENGTH);
	/*Set interface mode to GMII mode*/
	writel(MAC_1G_R_IF_MODE_VAL, mac_base + R_IF_MODE);

	/* HW specific config, now done in hcp_mac_init() */
	// sgmii_fast_sim(mac_idx);
	// sgmii_fast_sim_other(mac_idx);
	// sgmii_innosilicon_adjust(mac_idx);
		
	u32 cmd_cfg = readl(mac_base + R_COMMAND_CONFIG);
	printf("1G MAC-%d Configured (Cmd: 0x%08x)\n", mac_idx, cmd_cfg);
}

/*
 * Jabber-packet MAC config (baremetal mac_init_1g_jabber_packet): minimal
 * COMMAND_CONFIG and FRM_LENGTH 500. Apply after mac_init_1g().
 */
void mac_cfg_jabber_packet(u8 mac_idx)
{
	u32 mac_base = XGMII_MAC_BASE + (mac_idx * MAC_CONFIG_BYTE_CNT);

	writel(MAC_CC_NULL | MAC_CC_TX_ENA | MAC_CC_RX_ENA |
		    MAC_CC_PROMIS_EN | MAC_CC_TX_PAD_EN, mac_base + R_COMMAND_CONFIG);
	writel(500, mac_base + R_FRM_LENGTH);
	printf("1G MAC-%d Jabber config (Cmd: 0x%08x, FrmLen: %d)\n", mac_idx,
	       readl(mac_base + R_COMMAND_CONFIG),
	       readl(mac_base + R_FRM_LENGTH));
}

static void eth_set_host_tx_fifo_en(u8 mac_idx)
{
	u32 reg_val;
	for (int i = 0; i < MAX_MAC_CNT; i++) {
		reg_val =
			readl(AXILS_HCP_BASE + XGMII_PORT_CONTROL + (i * sizeof(u32)));
		reg_val &= ~(1 << 1);
		/* Disable HOST_TX_FIFO_EN */
		writel(reg_val, AXILS_HCP_BASE + XGMII_PORT_CONTROL + (i * sizeof(u32)));
	}
	/* TX: Lock on to mac where traffic will egress from host-tx-buffer */
	writel(BIT(HOST_TX_FIFO_EN), AXILS_HCP_BASE + XGMII_PORT_CONTROL + (mac_idx * sizeof(u32)));
}
void eth_set_eipbypass(u8 mac_idx)
{
	u8 port = mac_idx;
	if (mac_idx == 0) {
		port = 4;
	} else if (mac_idx < MAX_MAC_CNT) {
		port = mac_idx - 1;
	}
	/*
	 * Port selection and reset. RX_PACKET_FIFO_0 bits [6:4] (RX_FIFO_SEL)
	 * select the port, bit 0 resets the FIFO:
	 *   port 0..3 = mac_idx 1..4 (1G ports)
	 *   port 4    = mac_idx 0 (XGMII, 10G)
	 * Capture on the given port, so frames go to the host packet buffer
	 * instead of EIP inline (applies to both TX and RX).
	 * NOTE: the sequence below must be kept to lock on host processing.
	 */
	/* Reset host rx-fifo */
	writel(BIT(RX_FIFO_RST) | 
								BIT(RX_FIFO_DAV_MON) | 
								BIT(RX_FIFO_DAV_MON) | 
								(port << RX_FIFO_SEL), AXILS_HCP_BASE + RX_PACKET_FIFO_0);
	mdelay(100);

	writel(port << RX_FIFO_SEL, AXILS_HCP_BASE + RX_PACKET_FIFO_0);

	eth_set_host_tx_fifo_en(mac_idx);

	/* Reset tx fifos */
	writel(BIT(TX_FIFO_RST), AXILS_HCP_BASE + TX_PACKET_FIFO_0);
}

void enable_internal_near_end_lb(u8 mac_idx)
{
	u64 sgmii_base = sgmii_phy_base(mac_idx);
	u32 reg_val;

	reg_val = readl(sgmii_base + 0x001c);
	reg_val |= (1 << 7); //TX_NEAR_LPBK_EN
	writel(reg_val, sgmii_base + 0x001c);

	reg_val = readl(sgmii_base + 0x0010);
	reg_val |= (1 << 1); //RX_NEAR_LPBK_EN
	writel(reg_val, sgmii_base + 0x0010);
}
void disable_internal_near_end_lb(u8 mac_idx)
{
	u64 sgmii_base = sgmii_phy_base(mac_idx);
	u32 reg_val;

	reg_val = readl(sgmii_base + 0x001c);
	reg_val &= ~(1 << 7); //TX_NEAR_LPBK_EN
	writel(reg_val, sgmii_base + 0x001c);

	reg_val = readl(sgmii_base + 0x0010);
	reg_val &= ~(1 << 1); //RX_NEAR_LPBK_EN
	writel(reg_val, sgmii_base + 0x0010);

	reg_val = readl(sgmii_base + 0x0020);
	reg_val &= ~(1 << 17); //NEAR_LPBK_EN
	writel(reg_val, sgmii_base + 0x0020);
}

/* MAC 10G operation */
static inline int mac_reset_10g(void)
{
	u32 mac_base = XGMII_MAC_BASE;
	u32 regval;

	writel(MAC_CC_NULL, mac_base + R_COMMAND_CONFIG);
	writel((1 << 12), mac_base + R_COMMAND_CONFIG); //Reset MAC

	int timeout = 20;
	do {
		regval = readl(mac_base + R_COMMAND_CONFIG);
		if ((regval & (1 << 12)) == 0) {
			break; // Bit 12 Clear <=> Reset Done
		}
		mdelay(50);
		timeout--;
	} while (timeout > 0);

	if (timeout == 0) {
		printf("MAC SW_RESET timeout!");
		return 0;
	}
	printf("MAC Reset done.");
	return 1;
}
void mac_init_10g(void)
{
	u32 mac_base = XGMII_MAC_BASE;
	/* set R_MAC_0/1 value */
	/* MAC address 00:11:22:33:44:55 (XG_R_MAC_0/1_VAL) */
	writel(XG_R_MAC_0_VAL, mac_base + R_MAC_0);
	writel(XG_R_MAC_1_VAL, mac_base + R_MAC_1);

	/* 
	 * Adding multicast address. 
	 * This will accept any mac address whose 6-bit hash value equal to  111001
	 * Example: 01:01:01:00:00:01
	 */
	writel(0x000001e7, mac_base + R_HASHTABLE_LOAD);

	/* 
	 * This will accept any mac address whose 6-bit hash value equal to  110001
	 * Example: 01:01:00:00:00:01 
	 */
	writel(0x00000123, mac_base + R_HASHTABLE_LOAD);

	/* Configure RX / TX FIFO Sections*/
	writel(XG_MAC_R_RX_FIFO_VAL, mac_base + R_RX_FIFO_SECTIONS);
	writel(XG_MAC_R_TX_FIFO_VAL, mac_base + R_TX_FIFO_SECTIONS);
	//writel(MAC_1G_R_RX_FIFO_VAL, mac_base + (R_RX_FIFO_SECTIONS));
	//writel(MAC_1G_R_TX_FIFO_VAL, mac_base + (R_TX_FIFO_SECTIONS));

	/* Configure the MAC-COMMAND with specific bits */
	writel((MAC_CC_NULL |
		     MAC_CC_TX_ENA | /* Enable outbound flow of pkts. */
		     MAC_CC_RX_ENA | /* Enable inbound flow of pkts. */
		     MAC_CC_PROMIS_EN | /* Enable promiscuous mode */
		     MAC_CC_TX_PAD_EN), mac_base + R_COMMAND_CONFIG); /* Enable padding to the frame */

	/* Set frame length value */
	writel(XG_MAC_R_FRM_LENGTH_VAL, mac_base + R_FRM_LENGTH);

	printf("MAC is configured with command bits: 0x%x",
		 readl(mac_base + R_COMMAND_CONFIG));

	/*Set interface mode to GMII mode*/
	writel(XG_MAC_R_IF_MODE_VAL, mac_base + R_IF_MODE);

	/* let xgmii settings be effective */
	mdelay(200);

	printf("Successfully configured MAC 10G.\n");
}
/*
 * Enable the near-end loopback (internal loop) on XGMII SerDes lanes 0 and 1
 * (offsets 0x0xxx and 0x4xxx):
 *   0x0010: write 0x020000a2
 *   0x001c: write 0x020000c4
 *   0x0044: write 0x00026490
 * This step can be skipped by connecting TXP/N to RXP/N on the pads
 * (external loop).
 */
void xgmii_enable_near_loopback(void)
{
	writel(0x020000a2, RXAUI_BASE + 0x0010);
	writel(0x020000a2, RXAUI_BASE + 0x4010);

	mdelay(50);

	writel(0x020000c4, RXAUI_BASE + 0x001c);
	writel(0x020000c4, RXAUI_BASE + 0x401c);

	mdelay(50);

	writel(0x00026490, RXAUI_BASE + 0x0044);
	writel(0x00026490, RXAUI_BASE + 0x4044);
}

u8 eth_wait_internal_phy_linkup(u32 mac_idx)
{
	u32 regval;
	u32 count = 10000;

	do {
		regval = readl(SGMII_CSR_BASE + sgmii_stride(mac_idx));

		mdelay(1);
	} while ((!(regval & BIT(SYNC_STATUS)) ||
		  !(regval & BIT(AUTO_NEG_COMPLETE))) &&
		 --count);


	if (count <= 0) {
		printf("SGMII link timeout!\n");
	} else {
		printf("SGMII link up!\n");
	}

	u32 SGMII_BASE = sgmii_phy_base(mac_idx);
	printf("Auto-Neg status: 0x%x\n", readl(SGMII_BASE + 0x1bc4));
	printf("SGMII_CSR = 0x%x\n", readl(SGMII_CSR_BASE + sgmii_stride(mac_idx)));
	printf("SGMII_MON = 0x%x\n", readl(SGMII_MON_BASE + sgmii_stride(mac_idx)) >> 16);

	return (count > 0) ? 1 : 0;
}

void eth_print_mac_counters(uint32_t mac_idx)
{
	uint32_t tx_total, tx_good, rx_total, rx_good, crc_error, crc_error_tx;
	tx_total = AX_READ_MAC_REG32(mac_idx, SHIM_STATS_TX_MAC_TOTAL);
	tx_good = AX_READ_MAC_REG32(mac_idx, SHIM_STATS_TX_MAC_GOOD);
	rx_total = AX_READ_MAC_REG32(mac_idx, SHIM_STATS_RX_MAC_TOTAL);
	rx_good = AX_READ_MAC_REG32(mac_idx, SHIM_STATS_RX_MAC_GOOD);
	crc_error = AX_READ_MAC_REG32(mac_idx, 0x128);
	crc_error_tx = AX_READ_MAC_REG32(mac_idx, 0x228);

	printf("Stats for MAC-idx: %d\n", mac_idx);
	printf("TX_MAC(total/good): 0x%x 0x%x\n", tx_total, tx_good);
	printf("RX_MAC(total/good):0x%x 0x%x\n", rx_total, rx_good);
	printf("crc_error: 0x%x\n", crc_error);
	printf("crc_error_tx: 0x%x\n", crc_error_tx);
}
/* Hand the RMII0 / RMII1 pins to the RMII function, other pins untouched */
void eth_setup_gpio_rmii(void){
	uint32_t gpio_val = 0;

	gpio_val = readl(AX3000_CSR_BASE_ADRS_GPIO_2);
	gpio_val |= 0x3f800000;
	writel(gpio_val, AX3000_CSR_BASE_ADRS_GPIO_2); // RMII0

	gpio_val = readl(AX3000_CSR_BASE_ADRS_GPIO_3);
	gpio_val |= 0x07f00000;
	writel(gpio_val, AX3000_CSR_BASE_ADRS_GPIO_3); // RMII1

	mdelay(1000);
}
/* port control bits written by baremetal: 0x64 (output) / 0x24 (input) */
#define GMII_PORT_NCSI_MASK		0x64
#define GMII_PORT_NCSI_RMII_OUT	0x64
#define GMII_PORT_NCSI_RMII_IN	0x24
/*
 * Enable GMII/NCSI on the port. rmii_mode 0: output; 1: input.
 * Only the NCSI bits change, so HOST_TX_FIFO_EN set by eth_set_eipbypass()
 * survives: call this after eth_set_eipbypass().
 */
void eth_enable_gmii_ncsi(u8 mac_idx, u8 rmii_mode){
	uint32_t reg_val;

	reg_val = readl(AXILS_HCP_BASE + gmii_port_ctrl_offset(mac_idx));
	reg_val &= ~GMII_PORT_NCSI_MASK;
	reg_val |= rmii_mode ? GMII_PORT_NCSI_RMII_IN : GMII_PORT_NCSI_RMII_OUT;
	writel(reg_val, AXILS_HCP_BASE + gmii_port_ctrl_offset(mac_idx));
	mdelay(1000);
}
/* Clear AUTO_NEG_MON before the test; returns 1 if it is cleared */
int auto_neg_mon_clear_bit(int mac_idx)
{
	int  reg_val, auto_neg_mon,sgmii_reset_val,sgmii_reset;
    volatile u32 read_data,read_data_3;
	reg_val = readl(GE_MON_BASE + sgmii_stride(mac_idx));
	reg_val |= (1 << AUTO_NEG_MON);
	writel(reg_val, GE_MON_BASE + sgmii_stride(mac_idx));
	//check bit
	read_data = readl(GE_MON_BASE + sgmii_stride(mac_idx));
	auto_neg_mon = (read_data >> AUTO_NEG_MON ) & 0x1;
	printf("Bit Auto Neg Mon:%d\n",auto_neg_mon);
	if (auto_neg_mon == 1 )
    {
        printf("AUTO_NEG_MON = %d not clear before test\n", auto_neg_mon);
        return 0;
    }
    return 1;
}
/* Check AUTO_NEG (auto-neg complete); returns 1 if it is set */
int eth_auto_neg_mon(int mac_idx)
{
	u32 read_data;
    u32 auto_neg_mon;
    read_data = readl(GE_MON_BASE + sgmii_stride(mac_idx));
	auto_neg_mon = (read_data >> AUTO_NEG) & 0x1;
	printf("read_data = 0x%08X\n", read_data);
    printf("ACTUAL: AUTO_NEG_MON:%d\n",auto_neg_mon);
    printf("EXPECT AUTO_NEG_MON == 1\n");
    if (auto_neg_mon == 1)
    {
        return 1;
    }else{
		return 0;
	}
}
/*
 * Clear SGMII_MON and GE_MON and enable the SGMII IRQ before the test;
 * returns 1 if both are cleared
 */
int ge_mon_clear_bit(int mac_idx)
{
    u32  reg_val, ge_mon, sgmii_mon, sync_status_mon,auto_neg,sgmii_reset;
	volatile u32 read_data,read_data_2,read_data_3 ;
	reg_val = readl(AXILS_HCP_BASE + IRQ_GE_CSR_BASE + sgmii_stride(mac_idx));
	reg_val |= (1 << SGMII_MON) ; 
    reg_val |= (1 << GE_MON) ; 
    reg_val |= (1 << SGMII_IRQ_EN) ;
	writel(reg_val, AXILS_HCP_BASE + IRQ_GE_CSR_BASE + sgmii_stride(mac_idx));
	read_data = readl(AXILS_HCP_BASE + IRQ_GE_CSR_BASE + sgmii_stride(mac_idx));
	sgmii_mon = (read_data >> SGMII_MON) & 0x1;
	ge_mon = (read_data >> GE_MON) & 0x1;
	read_data_2 = readl(GE_MON_BASE + sgmii_stride(mac_idx));
	auto_neg = (read_data_2 >> AUTO_NEG ) & 0x1;
	printf("bit Auto Neg Complete:%d\n",auto_neg);
	read_data_3 = readl(AXILS_HCP_BASE + 0x0004 + sgmii_stride(mac_idx));
	sgmii_reset = (read_data_3 >> 4 ) & 0x1;
	printf("bit sgmii reset:%d\n",sgmii_reset);
	printf("Register = 0x%08X\n", read_data_3);
	if (sgmii_mon == 1 ||  ge_mon ==1)
    {
        printf("SGMII_MON =%d, GE_MON = %d not clear before test\n", sgmii_mon, ge_mon);
        return 0;
    }
    return 1;
}
/*
 * Clear HPI_MON, GE_MON and RX_FIFO_DAV_MON and enable the HPI and DAV
 * IRQs before the test; returns 1 if HPI_MON and GE_MON are cleared
 */
int irq_hpi_clear_bit(int mac_idx)
{
    int  reg_val, hpi_mon, ge_mon;
    volatile int read_data;
    // clear HPI_MON

    writel(HPI_MON, AXILS_HCP_BASE + HPI_CSR);
    hpi_mon = readl(AXILS_HCP_BASE + HPI_CSR);

    reg_val = readl(AXILS_HCP_BASE + IRQ_GE_CSR_BASE + (mac_idx - 1) * SGMII_CSR_OFFSET);
    reg_val |= BIT(GE_HPI_IRQ_EN) ; 
    reg_val |= BIT(GE_MON) ; // clear GE_MON
    writel(reg_val, AXILS_HCP_BASE + IRQ_GE_CSR_BASE + (mac_idx - 1) * SGMII_CSR_OFFSET);


    reg_val = readl(AXILS_HCP_BASE + RX_PACKET_FIFO_0);
    reg_val |= BIT(RX_FIFO_DAV_IRQ_EN) ; // enable dav_irq
    reg_val |=  BIT(RX_FIFO_DAV_MON) ; //clear dav_mon
    writel(reg_val, AXILS_HCP_BASE + RX_PACKET_FIFO_0);


    read_data = readl(AXILS_HCP_BASE + IRQ_GE_CSR_BASE + (mac_idx - 1) * SGMII_CSR_OFFSET);
    ge_mon = (read_data >> GE_MON) & 0x1;
    if (hpi_mon == 1 || ge_mon == 1)
    {
        printf("HPI_MON =%d ,GE_MON = %d not clear before test\n", hpi_mon, ge_mon);
        return 0;
    }
    return 1;
}
/* Wait for RX data, then check GE_MON and HPI_MON; returns 1 if both set */
int eth_print_irq_hpi(int mac_idx)
{
	volatile u32 read_data;
    int ge_mon,hpi_mon;
    int count = 40;
    // wait data available
    do {
        read_data = readl(AXILS_HCP_BASE + RX_PACKET_FIFO_0);
        if (read_data & BIT(RX_FIFO_DAV))
            break;
        mdelay(150);
    } while (--count);
    if (!(read_data & BIT(RX_FIFO_DAV))) {
        printf("RX data not available (timeout)\n");
        return 0;
    }
    read_data = readl(AXILS_HCP_BASE + IRQ_GE_CSR_BASE + (mac_idx - 1) * SGMII_CSR_OFFSET );
    ge_mon = (read_data >> GE_MON) & 0x1;
    printf("EXPECT: GE_MON == 1 && HPI MON == 1\n");
    printf("ACTUAL: GE_MON:%d\n",ge_mon);

    hpi_mon = readl(AXILS_HCP_BASE + HPI_CSR);
    printf("ACTUAL: HPI_MON %d\n",hpi_mon);
    
    if (hpi_mon == 1 && ge_mon == 1)
    {
        return 1;
    }
    return 0;
}
/* Check SGMII_MON and GE_MON; returns 1 if both are set */
int eth_ge_mon(int mac_idx)
{
	u32 read_data,read_data_2;
    u32 ge_mon,sgmii_mon,auto_neg;
    read_data = readl(AXILS_HCP_BASE + IRQ_GE_CSR_BASE+ sgmii_stride(mac_idx));
    sgmii_mon = (read_data >> SGMII_MON) & 0x1;
    ge_mon = (read_data >> GE_MON) & 0x1;
	read_data_2 = readl(GE_MON_BASE + sgmii_stride(mac_idx));
	auto_neg = (read_data_2 >> AUTO_NEG ) & 0x1;
	printf("bit Auto Neg Complete:%d\n",auto_neg);
    printf("SGMII_MON:%d, GE_MON: %d\n",sgmii_mon,ge_mon);
    printf("EXPECT SGMII_MON == 1 , GE_MON == 1\n");
    if (ge_mon == 1 && sgmii_mon==1)
    {
        return 1;
    }else{
		return 0;
	}
}
/* Print TX/RX broadcast counters; 1 if both equal ETH_TEST_DFLT_FRAMES */
int eth_print_broadcast_counters(int mac_idx)
{
    u32 tx_broadcast, rx_broadcast;
    tx_broadcast = readl((AXILS_SHIM_MAC_BASE + (mac_idx * MAC_BASE_OFFSET)) + SHIM_STATS_TX_BROADCAST);
	rx_broadcast = readl((AXILS_SHIM_MAC_BASE + (mac_idx * MAC_BASE_OFFSET)) + SHIM_STATS_RX_BROADCAST);
	printf("Stats for MAC-idx: %d\n", mac_idx);
    printf("EXPECT TX-BROADCAST GOOD FRAME == RX-BROADCAST GOOD FRAME == %x\n", ETH_TEST_DFLT_FRAMES);
    printf("ACTUAL: TX-BROADCAST GOOD FRAME:%x\n", tx_broadcast);
	printf("ACTUAL: RX-BROADCAST GOOD FRAME: %x\n", rx_broadcast);
    if((tx_broadcast > 0) && (rx_broadcast > 0) && (tx_broadcast == rx_broadcast) && (ETH_TEST_DFLT_FRAMES == rx_broadcast)) {
        return 1;
    }else {
        return 0;
    }
}
/* TX/RX MAC control frame counters; 1 if both == ETH_TEST_DFLT_FRAMES */
int eth_print_ctrl_frame_counters(int mac_idx)
{
    u32 tx_mac_ctrl_frame, rx_mac_ctrl_frame;
    tx_mac_ctrl_frame = readl((AXILS_SHIM_MAC_BASE + (mac_idx * MAC_BASE_OFFSET)) + SHIM_STATS_MAC_CTRL_FRAME_TX);
	rx_mac_ctrl_frame = readl((AXILS_SHIM_MAC_BASE + (mac_idx * MAC_BASE_OFFSET)) + SHIM_STATS_MAC_CTRL_FRAME_RX);
	printf("Stats for MAC-idx: %d\n", mac_idx);
    printf("EXPECT TX MAC CTRL FRAME == RX MAC CTRL FRAME == %x\n", ETH_TEST_DFLT_FRAMES);
    printf("ACTUAL: TX MAC CTRL FRAME:%x\n", tx_mac_ctrl_frame);
	printf("ACTUAL: RX MAC CTRL FRAME: %x\n", rx_mac_ctrl_frame);
    if((tx_mac_ctrl_frame > 0) && (rx_mac_ctrl_frame > 0) && (tx_mac_ctrl_frame == rx_mac_ctrl_frame) && (ETH_TEST_DFLT_FRAMES == rx_mac_ctrl_frame)) {
        return 1;
    }else {
        return 0;
    }
}
/* Print the RX drop events counter; returns 1 if it is > 0 */
int eth_print_drop_event_counters(int mac_idx)
{
    u32 rx_mac_drop_event;
	rx_mac_drop_event = readl((AXILS_SHIM_MAC_BASE + (mac_idx * MAC_BASE_OFFSET)) + SHIM_STATS_RX_DROP_EVENTS);
	printf("Stats for MAC-idx: %d\n", mac_idx);
    printf("EXPECT: RX-DROP EVENTS > 0\n");
	printf("ACTUAL: RX-DROP EVENTS: %x\n",rx_mac_drop_event);
    if(rx_mac_drop_event > 0) {
        return 1;
    }else {
        return 0;
    }
}
u32 eth_read_fragments_counter(int mac_idx)
{
	return readl((AXILS_SHIM_MAC_BASE + (mac_idx * MAC_BASE_OFFSET)) + SHIM_STATS_RX_FRAGMENTS_PKTS);
}

/* Print and return the raw RX fragments counter (non-zero = pass) */
u32 eth_print_fragments_counters(int mac_idx)
{
	u32 rx_fragments_pkts = eth_read_fragments_counter(mac_idx);

	printf("Stats for MAC-idx: %d\n", mac_idx);
	printf("ACTUAL: Fragments Packets: %x\n", rx_fragments_pkts);

	return rx_fragments_pkts;
}

u32 eth_print_crc_err_counters(int mac_idx)
{
	u32 shim_base = AXILS_SHIM_MAC_BASE + (mac_idx * MAC_BASE_OFFSET);
	u32 rx_total = readl(shim_base + SHIM_STATS_RX_MAC_TOTAL);
	u32 rx_good = readl(shim_base + SHIM_STATS_RX_MAC_GOOD);
	u32 crc_error = readl(shim_base + SHIM_STATS_CRCERRORS);

	printf("Stats for MAC-idx: %d\n", mac_idx);
	printf("RX_MAC(total): %d\n", rx_total);
	printf("RX_MAC(good): %d\n", rx_good);
	printf("CRC Error: %d\n", crc_error);

	return crc_error;
}
u32 eth_print_lengtherrors_counters(int mac_idx)
{
	u32 lgth_error;
	lgth_error = readl(mac_idx + SHIM_STATS_RX_LNTHERROR);

    printf("Stats for MAC-idx: %d\n", mac_idx);
    printf("Expect: RX-LNTHERROR >0\n");
	printf("Actual: RX-LNTHERROR (total):%d\n", lgth_error);

    return lgth_error;
}
/*
 * Drain one frame from the host RX FIFO with a long timeout (port of the
 * baremetal ingress_packet_error_check)
 */
int ingress_packet_error_check(int mac_idx, unsigned char buf[], int blen)
{
	uint8_t mod = 0;
	int len = 0;
	int rlen = -1;
	volatile uint32_t read_data;
	int count = 40;
	uint32_t wb;
	uint8_t *bp;
	uint8_t port = mac_idx;
	if (mac_idx == 0) {
		port = 4;
	} else {
		port = mac_idx - 1;
	}
	//eth_printf("Ingress...\n");
	/* wait for data available in the host receive packet interface */
	do {
		read_data = readl(AXILS_HCP_BASE + RX_PACKET_FIFO_0);
        mdelay(150);
	} while (!(read_data & BIT(RX_FIFO_DAV)) && count--);
	if (count <= 0) {
		printf("Timeout on rx \n");
		printf("Timeout on rx \n");
		return rlen;
	}

	// frame length in 32-bit words and MOD (valid bytes in the last word)
	read_data = readl(AXILS_HCP_BASE + RX_PACKET_FIFO_1);

	len = read_data >> RX_FIFO_FRMLEN & 0x0000ffff;
	mod = (read_data >> RX_FIFO_MOD) & RX_FIFO_DAV;
    
	//eth_print_info("len: ", len);
	//eth_print_info("mod: ", mod);

	rlen = len * 4 - ((4 - mod) % 4);

	//eth_print_info("rlen: ", rlen);

	/* packet doesn't fit in the buffer: drop it */
	if (len * 4 > blen) {
		len = mod = 0;
		rlen = -2;
	}

	/* read packet from FIFO to buffer */
	printf("Ingress packet\n");
	bp = buf; /* index buffer */
	for (count = 0; count < len; count++) {
		wb = readl(AXILS_HCP_BASE + RX_PACKET_FIFO_RDATA);
		memcpy(bp, &wb, sizeof(wb));
		bp += 4;
	}

	// write 1 to clear EOP and advance to the next packet
	writel(BIT(RX_FIFO_EOP), AXILS_HCP_BASE + RX_PACKET_FIFO_1);

	/* EOP issue workaround: reset the RX FIFO, select the port again */
	writel(BIT(RX_FIFO_RST), AXILS_HCP_BASE + RX_PACKET_FIFO_0);
	writel(port << RX_FIFO_SEL, AXILS_HCP_BASE + RX_PACKET_FIFO_0);
	/* end workaround */

	printf("End of ingress\n");
	/* return length in bytes */
	return (rlen);
}
