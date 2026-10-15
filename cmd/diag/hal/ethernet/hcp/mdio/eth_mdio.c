// SPDX-License-Identifier: GPL-2.0+
#include "eth_mdio.h"
/* packet generator burst size used by the CRC error test: 1 packet */
#define ETH_PACKET_CRC_ERROR	0x1

/* Clause 22 API */
void mdio_wait(u8 mac_idx)
{
	u32 mac_base = XGMII_MAC_BASE + (mac_idx * MAC_CONFIG_BYTE_CNT);
	u32 temp = 0x0001; //MDIO Busy bit 0

	while ((temp & 0x0001) == 0x0001) {
		temp = readl(mac_base+R_MDIO_CFG_STATUS);
	}
}
void mdio_clause22_write(u8 mac_idx, u8 phy_addr, u8 reg_addr,
			 u16 data)
{
	// Wait for MDIO controller to be ready
	mdio_wait(mac_idx);
	u32 mac_base = XGMII_MAC_BASE+(mac_idx * MAC_CONFIG_BYTE_CNT);

	// Prepare the command (Clause 22 format):
	// bits [9:5] = PHY address, bits [4:0] = register address
	// Bit 15 (READ) = 0 for write operation
	u16 command = ((phy_addr & 0x1F) << 5) | (reg_addr & 0x1F);
	// Write the command to the COMMAND REG
	writel(command, mac_base + R_MDIO_COMMAND);

	// The data to be written to the DATA REG and start
	writel(data, mac_base + R_MDIO_DATA);

	// Wait for the controller to complete the transaction
	mdio_wait(mac_idx);
}
u16 mdio_clause22_read(u8 mac_idx, u8 phy_addr, u8 reg_addr)
{
	// Wait for MDIO controller to be ready
	mdio_wait(mac_idx);
	u32 mac_base = XGMII_MAC_BASE+(mac_idx * MAC_CONFIG_BYTE_CNT);
	// Prepare the read command:
	// Bit 15 = 1 (start read)
	// bits [9:5] = PHY address, bits [4:0] = register address
	u16 command = (1 << 15) | // Set read flag (bit 15)
			   ((phy_addr & 0x1F) << 5) | // PHY address (bits 9:5)
			   (reg_addr & 0x1F); // Register address (bits 4:0)

	// Issue the read command
	writel(command, mac_base + R_MDIO_COMMAND);

	// Wait for the read transaction to complete
	mdio_wait(mac_idx);

	// Read the result
	return (u16)(readl(mac_base + R_MDIO_DATA) & 0xFFFF);
}

/* Clause 45 API */
static void mdio_clause45_wait(u32 mac_base)
{
	u32 temp = 0x0001;
	while ((temp & 0x0001) == 0x0001) {
		temp = readl(mac_base + R_MDIO_CFG_STATUS);
	}
}
void mdio_clause45_init(u32 mac_base)
{
	u32 mdio_cfg = readl(mac_base + R_MDIO_CFG_STATUS);
	mdio_cfg |= (1 << 6); // select clause 45
	printf("mdio config :%d\n", mdio_cfg);

	writel(mdio_cfg, mac_base + R_MDIO_CFG_STATUS);
	mdio_clause45_wait(mac_base);
}
static void clause45_addr_phase(u32 mac_base, u32 phyaddr,
				u32 dev_addr, u32 reg_addr)
{
	writel(
			    (0x0 | (phyaddr << 5) | dev_addr),
	       mac_base + R_MDIO_COMMAND); // address phase
	writel(
			    reg_addr, mac_base + R_MDIO_REGADDR); // mdio register
	mdio_clause45_wait(mac_base);
}
void mdio_clause45_write(u32 mac_base, u32 phyaddr, u32 dev_addr,
			 u32 reg_addr, u16 data)
{
	clause45_addr_phase(mac_base, phyaddr, dev_addr, reg_addr);
	writel(data, mac_base + R_MDIO_DATA);
	mdio_clause45_wait(mac_base);
}
u32 mdio_clause45_read(u32 mac_base, u32 phyaddr,
			    u32 dev_addr, u32 reg_addr)
{
	clause45_addr_phase(mac_base, phyaddr, dev_addr, reg_addr);
	writel(
			    (0x8000 | (phyaddr << 5) |
			     dev_addr),
	       mac_base + R_MDIO_COMMAND); // read command phase
	mdio_clause45_wait(mac_base);
	return readl(mac_base+R_MDIO_DATA);
}
/* Configure the MDIO controller for Clause 22 */
void mdio_config_c22(u8 mac_idx)
{
	u32 mac_base = XGMII_MAC_BASE + (mac_idx * MAC_CONFIG_BYTE_CNT);
	writel(
			    0x0b1c, mac_base + R_MDIO_CFG_STATUS); // Configure Clause 22
	mdio_wait(mac_idx);
	// mdelay(500);
}

/*
 * PHY power down bits:
 *  0_0.11  Copper Power Down
 *  16_0.2  Copper Power Down
 *  0_1.11  Fiber/SGMII Power Down
 */

void mdio_phy_power_cycle(u8 mac_idx, u8 phy_addr, u8 reg)
{
	mdio_set_phy_powerdown(mac_idx, phy_addr, reg);
	mdelay(100);
	mdio_set_phy_powerup(mac_idx, phy_addr, reg);
}

void mdio_set_phy_powerup(u8 mac_idx, u8 phy_addr, u8 reg)
{
	u32 regval;
	mdio_select_page(mac_idx, phy_addr, reg, 0); //select page 0

	regval = mdio_clause22_read(
		mac_idx, phy_addr,
		0); // Reg 0_0: Bit 11: 1 Power Down ; 0: Normal Operation
	regval &= ~(1 << 11); //Bit 11: 0 Power Up
	mdio_clause22_write(mac_idx, phy_addr, 0, regval); // Power Up

	regval = mdio_clause22_read(
		mac_idx, phy_addr,
		16); // Reg 16_0: Bit 2: 1 Power Down ; 0: Normal Operation
	regval &= ~(1 << 2); //Bit 2: 0 Power Up
	mdio_clause22_write(mac_idx, phy_addr, 16, regval); // Power Up Copper

	mdio_select_page(mac_idx, phy_addr, reg, 1); //select page 1
	regval = mdio_clause22_read(
		mac_idx, phy_addr,
		0); // Reg 0_1: Bit 11: 1 Power Down ; 0: Normal Operation
	regval &= ~(1 << 11); //Bit 11: 0 Power Up
	mdio_clause22_write(mac_idx, phy_addr, 0, regval); // Power Up Fiber/SGMII

	mdio_select_page(mac_idx, phy_addr, reg, 0); //return page 0
}

void mdio_set_phy_powerdown(u8 mac_idx, u8 phy_addr, u8 reg)
{
	u32 regval;
	mdio_select_page(mac_idx, phy_addr, reg, 0); //select page 0

	regval = mdio_clause22_read(
		mac_idx, phy_addr,
		0); // Reg 0_0: Bit 11: 1 Power Down ; 0: Normal Operation
	regval |= (1 << 11); //Bit 11: 1 Power Down
	mdio_clause22_write(mac_idx, phy_addr, 0, regval); // Power Down

	regval = mdio_clause22_read(
		mac_idx, phy_addr,
		16); // Reg 16_0: Bit 2: 1 Power Down ; 0: Normal Operation
	regval |= (1 << 2); //Bit 2: 1 Power Down
	mdio_clause22_write(mac_idx, phy_addr, 16, regval); // Power Down Copper

	mdio_select_page(mac_idx, phy_addr, reg, 1); //select page 1
	regval = mdio_clause22_read(
		mac_idx, phy_addr,
		0); // Reg 0_1: Bit 11: 1 Power Down ; 0: Normal Operation
	regval |= (1 << 11); //Bit 11: 1 Power Down
	mdio_clause22_write(mac_idx, phy_addr, 0, regval); // Power Down Fiber/SGMII

	mdio_select_page(mac_idx, phy_addr, reg, 0); //return page 0
}


/*
 * Find the PHY with MDIO C22 and read its ID.
 * Returns the PHY address, or INVALID_MAC_IDX if no PHY is found.
 */
u8 mdio_find_phy_id(u8 mac_idx, uint16_t *id1, uint16_t *id2)
{
    if (id1 == NULL || id2 == NULL) {
        return INVALID_MAC_IDX;
	}

	for (u8 phy_addr = 0; phy_addr < 32; phy_addr++) {
		*id1 = mdio_clause22_read(mac_idx, phy_addr, 2); // Read PHY ID1
		*id2 = mdio_clause22_read(mac_idx, phy_addr, 3); // Read PHY ID2

		if ((*id1 != 0xFFFF && *id1 != 0x0000) &&
		    (*id2 != 0xFFFF && *id2 != 0x0000)) {
			printf("MDIO Found PHY at addr: 0x%x\n", phy_addr);
			printf("ID1: 0x%x\n", *id1);
			printf("ID2: 0x%x\n", *id2);
			return phy_addr;
		}
	}
	return INVALID_MAC_IDX;
}
/*
 * Find the PHY with MDIO C45 and read its ID.
 * Returns the PHY address if a PHY is found.
 */
u8 xgmii_mdio_scan_phy_address(u32 mac_base)
{
	uint16_t id1, id2;
	for (u8 phyaddr = 0; phyaddr < 32; phyaddr++) {
		// Read ID1 (Device 1, reg 0x02)
		id1 = mdio_clause45_read(mac_base, phyaddr, 1, 0x02);
		// Read ID2 (Device 1, reg 0x03)
		id2 = mdio_clause45_read(mac_base, phyaddr, 1, 0x03);

		if ((id1 != 0xFFFF && id1 != 0x0000) ||
		    (id2 != 0xFFFF && id2 != 0x0000)) {
			printf("Found PHY at addr: %d\n", phyaddr);
			printf("PMA/PMD Device Identifier 1: %d\n", id1);
			printf("PMA/PMD Device Identifier 2: %d\n", id2);

			id1 = mdio_clause45_read(mac_base, phyaddr, 1, 0x0E);
			id2 = mdio_clause45_read(mac_base, phyaddr, 1, 0x0F);

			printf("PMA/PMD Package Identifier 1: %d\n", id1);
			printf("PMA/PMD Package Identifier 2: %d\n", id2);

			id1 = mdio_clause45_read(mac_base, phyaddr, 3, 0x02);
			id2 = mdio_clause45_read(mac_base, phyaddr, 3, 0x03);
			printf("PCS Device Identifier 1: %d\n", id1);
			printf("PCS Device Identifier 2: %d\n", id2);

			id1 = mdio_clause45_read(mac_base, phyaddr, 3, 0x0E);
			id2 = mdio_clause45_read(mac_base, phyaddr, 3, 0x0F);
			printf("PCS Package Identifier 1: %d\n", id1);
			printf("PCS Package Identifier 2: %d\n", id2);

			return phyaddr;
		}
	}	
	return INVALID_MAC_IDX;
}

/*
 * Setup PHY mode. Reg 18_20[2:0]
 * 000 = RGMII (System mode) to Copper
 * 001 = SGMII (System mode) to Copper
 * 010 = RGMII (System mode) to 1000BASE-X
 * 011 = RGMII (System mode) to 100BASE-FX
 * 100 = RGMII (System mode) to SGMII (Media mode)
 */
void mdio_setup_phy_mode(u8 mac_idx, u8 phy_addr, u8 reg)
{
	u32 regval;
	mdio_select_page(mac_idx, phy_addr, reg, 18); //select page 18
	regval = mdio_clause22_read(mac_idx, phy_addr, 20); //Control Reg
	regval &= ~0x7; // Clear MODE[2:0] bits
	regval |= 0x1; // Set MODE = 001 -> SGMII (System mode) to Copper
	mdio_clause22_write(mac_idx, phy_addr, 20, regval);

	// mdelay(100);

	//Must be followed by a software reset to take effect.
	regval = regval | (1 << 15); //Reset
	mdio_clause22_write(mac_idx, phy_addr, 20, regval);
	do {
		regval = mdio_clause22_read(mac_idx, phy_addr, 20);
	} while ((regval & (1 << 15)) != 0); // Wait until reset done
	mdio_select_page(mac_idx, phy_addr, reg, 0); //return page 0
}

/*
 * MDIO C22 select page. reg is the page register of the PHY:
 * - Marvell PHY (MARVELL_PHY = 22): Page Address register 22; page 0 gives
 *   access to the IEEE standard registers 0 to 15.
 * - Realtek PHY (REALTEK_PHY = 31): Page Select Register (PAGSR) 31; set it
 *   to 0 or 0xa42 (default) to access the IEEE standard registers 0 to 15.
 */
void mdio_select_page(u8 mac_idx, u8 phy_addr, u8 reg,
		      u8 page)
{
	mdio_clause22_write(mac_idx, phy_addr, reg, page);
	// mdelay(100);
}

/*
 * Default MAC interface speed
 * Reg 21_2: Bit 13: 0, Bit 6: 1 <=> 1000Mbps
 * Also used to set the speed of the MAC interface during MAC side loopback.
 * Both bits must be set and the speed forced to the same value in register 0.
 *
 * Select device speed for Realtek
 * Page 0 - Address 0x0: Bit 6: 1 and Bit 13: 0 to determine device speed as 1000Mbps
 * Note: changes to registers 0.12, 0.13, 0.6 and 0.8 take effect only after
 * a software reset (0.15), a Restart_AN (0.9), or a PWD (0.11) transition
 * from power-down to normal operation.
 */
static void mdio_set_phy_speed(u8 mac_idx, u8 phy_addr, u8 reg)
{
	u32 regval;
	u8 page = 0;

	if (reg == MARVELL_PHY)
		page = 2;
	else
		page = 0;
	mdio_select_page(mac_idx, phy_addr, reg, page);

	regval = mdio_clause22_read(mac_idx, phy_addr, 21);
	//val |= BIT(6) | BIT(13); as BIT13=0 anyway
	regval |= (1 << 6); //Set bit 6 = 1; Select speed 1000Mbps
	if (reg == REALTEK_PHY)
		regval &= ~(1 << 13);

	mdio_clause22_write(mac_idx, phy_addr, 21, regval);

	mdio_reset_phy(mac_idx, phy_addr, reg);

	page = 0; // select page 0
	mdio_select_page(mac_idx, phy_addr, reg, page);
}

/*
 * Setup Copper Control Register
 * Reg 0_0: Bit 8: Full duplex; Bit 6: 1000Mbps; Bit 12: AN enable
 * Bit 9: Restart Copper Auto-Negotiation
 * Bit 6 & 13: Speed Selection
 */
static void mdio_phy_config_pcs(u8 mac_idx, u8 phy_addr, u8 reg)
{
	u32 regval;
	u8 page = 0;
	mdio_select_page(mac_idx, phy_addr, reg, 0); //select page 0

	regval = mdio_clause22_read(mac_idx, phy_addr, 0);

	if (reg == MARVELL_PHY) {
		// Full duplex (bit 8), 1000Mbps
		regval |= (1 << 8) | (1 << 6);

		// Clear auto-negotiation enable (bit 12)
		regval &= ~(1 << 12); //Disable AN
	} else {
		// Full duplex (bit 8), 1000Mbps
		regval |= (1 << 6) | (1 << 8);
		regval &= ~(1 << 13);

		// Auto-Negotiation enable (bit 12)
		regval |= (1 << 12);
		regval |= (1 << 9);
	}

	mdio_clause22_write(mac_idx, phy_addr, 0, regval);
	// mdelay(10);

	mdio_reset_phy(mac_idx, phy_addr, reg);
	// mdelay(100);

	page = 0;
	mdio_select_page(mac_idx, phy_addr, reg, page);
}

/* Apply external PHY Configuration */
void mdio_phy_config(u8 mac_idx, u8 phy_addr, u8 reg)
{
	// mdio_set_phy_speed(mac_idx, phy_addr, reg);
	mdio_phy_config_pcs(mac_idx, phy_addr, reg);
}

/* PHY software reset: Reg 0_0[15], wait until it self-clears */
void mdio_reset_phy(u8 mac_idx, u8 phy_addr, u8 reg)
{
	u32 regval;

	mdio_select_page(mac_idx, phy_addr, reg, 0); //select page 0
	regval = mdio_clause22_read(mac_idx, phy_addr, 0); //Control Reg
	regval = regval | (1 << 15); //Reset PHY
	mdio_clause22_write(mac_idx, phy_addr, 0, regval);

	do {
		regval = mdio_clause22_read(mac_idx, phy_addr, 0);
	} while ((regval & (1 << 15)) != 0); // Wait until reset done
}

/*
 * Enable Auto Negotiation Process
 * Reg 0_1[12]:
 * 1 = Enable Auto-Negotiation Process
 * 0 = Disable Auto-Negotiation Process
 */
void mdio_set_phy_an(u8 mac_idx, u8 phy_addr, u8 reg)
{
	uint16_t regval;
	mdio_select_page(mac_idx, phy_addr, reg, 1); // Page 1
	regval = mdio_clause22_read(mac_idx, phy_addr,
				    0); // Fiber Control Register (0_1)
	regval |= (1 << 12); //Bit 12: enable AN
	mdio_clause22_write(mac_idx, phy_addr, 0, regval);
	mdio_select_page(mac_idx, phy_addr, reg, 0); // Page 0
}

/*
 * Setup the copper Auto Negotiation
 * Bit 12: Auto-Negotiation ; Bit 9: Restart Copper Auto-Negotiation
 */
void mdio_setup_copper_an(u8 mac_idx, u8 phy_addr, u8 reg)
{
	u32 regval;
	mdio_select_page(mac_idx, phy_addr, reg, 0); //select page 0
	regval = mdio_clause22_read(mac_idx, phy_addr, 0); //Control Reg
	regval = regval | (1 << 12); //12 Auto-Negotiation
	regval = regval | (1 << 9); // 9 Restart Copper Auto-Negotiation
	mdio_clause22_write(mac_idx, phy_addr, 0, regval);
}

/* Enable the external PHY loopback: Reg 0_0[14] */
void mdio_enable_exphy_lb(u8 mac_idx, u8 phy_addr, u8 reg)
{
	u32 regval;
	printf("Config PHY with PCS Loopback\n");
	mdio_select_page(mac_idx, phy_addr, reg, 0); //select page 0

	regval = mdio_clause22_read(mac_idx, phy_addr, 0);

	// Set loopback (bit 14)
	regval |= (1 << 14);

	mdio_clause22_write(mac_idx, phy_addr, 0, regval);
	// mdelay(100);
}

/*
 * Disable the external PHY loopback (Realtek and Marvell)
 * Page 0 - Address 0x0, Bit 14: 1 - enable PCS LB, 0 - disable PCS LB
 */
void mdio_disable_exphy_lb(u8 mac_idx, u8 phy_addr, u8 reg)
{
	u32 regval;
	mdio_select_page(mac_idx, phy_addr, reg, 0); //select page 0

	regval = mdio_clause22_read(mac_idx, phy_addr, 0);

	// De-assert loopback (bit 14)
	regval &= ~(1 << 14);

	mdio_clause22_write(mac_idx, phy_addr, 0, regval);
	mdelay(100);
}

/*
 * PHY <=> Copper | Linkup Status Check.
 * Reg 1_0
 * Check: Bit 5: Auto-Negotiation process && Bit 2: Copper Link
 */
int mdio_wait_copper_linkup(u8 mac_idx, u8 phy_addr,
				  u8 reg)
{
	int count = 500;
	uint16_t regval;
	int status = 0;

	mdio_select_page(mac_idx, phy_addr, reg, 0); // Page 0
	do {
		regval = mdio_clause22_read(mac_idx, phy_addr,
					    1); //Copper Status Register
		// Bit 5: Auto-Negotiation process complete && Bit 2: Copper Link
		if ((regval & (1 << 5)) && (regval & (1 << 2))) {
			status = 1;
			break;
		}
		mdelay(10);
	} while (--count);

	mdio_select_page(mac_idx, phy_addr, reg, 0); // Page 0
	return status; // Timeout
}

void mdio_set_advertise_1000base_t_full_duplex(u8 mac_idx,
					       u8 phy_addr)
{
	u32 regval;
	mdio_select_page(mac_idx, phy_addr, MARVELL_PHY, 0); //select page 0
	regval = mdio_clause22_read(mac_idx, phy_addr, 0); //Control Reg
	regval = regval | (1 << 15); //Reset PHY
	mdio_clause22_write(mac_idx, phy_addr, 0, regval);
}

/*
 * Enable bypass AN for the Marvell PHY
 * Serial Interface Auto-Negotiation bypass
 * Reg 26_1[6]
 */
void mdio_set_sgmii_an_bypass_mode(u8 mac_idx, u8 phy_addr)
{
	uint16_t regval;
	mdio_select_page(mac_idx, phy_addr, MARVELL_PHY, 1); // Page 1
	regval = mdio_clause22_read(mac_idx, phy_addr,
				    26); // Fiber Specific Control Register 2
	regval |= (1 << 6); //Bit 6: enable bypass
	mdio_clause22_write(mac_idx, phy_addr, 26, regval);
	mdio_select_page(mac_idx, phy_addr, MARVELL_PHY, 0); // Page 0
}

/*
 * SGMII <=> Marvell PHY Linkup Status Check.
 * Reg 17_1
 * Bit 5: Sync status && Bit 10: Link (real time) && Bit 11: Speed and Duplex Resolved
 */
static inline int mdio_wait_sgmii_link_up_from_marvell_phy(u8 mac_idx,
						   u8 phy_addr)
{
	int count = 20;
	uint16_t regval;
	int status = 0;

	mdio_select_page(mac_idx, phy_addr, MARVELL_PHY, 1); // Page 1

	do {
		regval = mdio_clause22_read(
			mac_idx, phy_addr,
			17); // Fiber Specific Status Register
		// Bit 5: Sync status && Bit 10: Link (real time) && Bit 11: Speed and Duplex Resolved
		if ((regval & (1 << 5)) && (regval & (1 << 10)) &&
		    (regval & (1 << 11))) {
			status = 1;
			printf("PHY STATUS!: 0x%x\n", regval);
			break;
		}
		mdelay(50);
	} while (--count);

	mdio_select_page(mac_idx, phy_addr, MARVELL_PHY, 0); // Page 0
	return status; // Timeout
}

/*
 * Page 6, reg 18: set bit 3 to enable the stub loopback. Bit 4 is set too:
 * it resets the CRC error counter (self-clearing).
 */
void mdio_set_stub_lb(u8 mac_idx, u8 phy_addr, u8 reg)
{
	u32 regval;
	mdio_select_page(mac_idx, phy_addr, reg, 6); //select page 6
	regval = mdio_clause22_read(mac_idx, phy_addr,
				    18); // Page 6, Register 18
	regval |= (1 << 4); //Reset CRC Counter
	regval |= (1 << 3); //Enable Stub Test
	mdio_clause22_write(mac_idx, phy_addr, 18, regval);
	mdio_select_page(mac_idx, phy_addr, reg, 0); //select page 0
}

/*
 * Page 6, reg 18: clear bit 3 to disable the stub loopback, then reset the
 * PHY before the next loopback test.
 */
void mdio_disable_stub_lb(u8 mac_idx, u8 phy_addr, u8 reg)
{
	u32 regval;
	mdio_select_page(mac_idx, phy_addr, reg, 6); //select page 6
	regval = mdio_clause22_read(mac_idx, phy_addr,
				    18); // Page 6, Register 18
	regval &= ~(1 << 3); //Disable Stub Test
	mdio_clause22_write(mac_idx, phy_addr, 18, regval);
	mdio_select_page(mac_idx, phy_addr, reg, 0); //select page 0

	mdio_reset_phy(mac_idx, phy_addr, reg);
	mdelay(10);
}

/* Start the Marvell PHY packet generator (page 6) over MDIO C22 */
void mdio_setup_start_pg(u8 mac_idx, u8 phy_addr, u8 reg)
{
	mdio_select_page(mac_idx, phy_addr, reg, 6); //select page 6

	/* Gap between packets of the PHY packet generator (copper) */
	mdio_clause22_write(mac_idx, phy_addr, 19, 0x0C); // IPG = 12 byte

	/* CRC counter reset */
	mdio_clause22_write(mac_idx, phy_addr, 18, 0x10); // Reg 18, bit 4 = 1
	mdelay(100);

	// Config Packet Generator:
	uint16_t pg_ctrl = (0xFE << 8) | // burst size = 0xFE packets
			   (0 << 7) | // Trigger send  (0: start)
			   (0 << 6) | // no self-clear
			   (1 << 4) | // Enable CRC checker
			   (1 << 3) | // Enable PG
			   (0 << 2) | // Payload pseudo-random
			   (0 << 1) | // Packet length: 64 byte
			   (0 << 0); // Don't check CRC

	mdio_clause22_write(mac_idx, phy_addr, 16,
			    pg_ctrl); // start send packet
	mdio_select_page(mac_idx, phy_addr, reg, 0); //return page 0
}

void realtek_conf(u8 mac_idx, int phy_addr)
{
    mdio_clause22_write(mac_idx, phy_addr, 0, 0x8000);  
    mdio_clause22_write(mac_idx, phy_addr, 0x1f, 0xd08);  
    // mdio_clause22_write(mac_idx, phy_addr, 0x11, 0xc);  
    mdio_clause22_write(mac_idx, phy_addr, 0x15, 0x11);  
    mdio_clause22_write(mac_idx, phy_addr, 0x1f, 0x0);  
    mdio_clause22_write(mac_idx, phy_addr, 0x1f, 0xd04);  
    // mdio_clause22_write(mac_idx, phy_addr, 0x10, 0x617f);
 
    mdio_clause22_write(mac_idx, phy_addr, 0x1f, 0x0);
    mdio_clause22_write(mac_idx, phy_addr, 0x9, 0x300);
    mdio_clause22_write(mac_idx, phy_addr, 0x0, 0x1240);
}
void mdio_setup_realtek_phy_autoneg(u8 mac_idx)
{
	u32 rval;
	u32 sgmii_base =
		SGMII0_BASE + ((mac_idx - 1) * SGMII_CONFIG_RANGE);
	rval = readl(sgmii_base + 0x1c00);
	rval = (rval | (0x1 << 12));
	writel(rval, sgmii_base + 0x1c00);
}
void mdio_disable_realtek_phy_autoneg(u8 mac_idx, u8 phy_addr,
					     u8 reg)
{
	u32 regval;
	//Select page 0
	mdio_select_page(mac_idx, phy_addr, reg, 0);
	// Read value of Address 0x00
	regval = mdio_clause22_read(mac_idx, phy_addr, 0);
	regval &= ~(1 << 12);

	mdio_clause22_write(mac_idx, phy_addr, 0, regval);
}

u32 wait_auto_neg_comple(u8 mac_idx, u8 phyaddr,
				     u8 reg)
{
	mdio_select_page(mac_idx, phyaddr, reg, 0);
	u32 regval, auto_neo_complete;
	u32 count = 8000;

	do {
		regval = mdio_clause22_read(mac_idx, phyaddr, 1);
		auto_neo_complete = (regval >> 5) & 0x1;
		mdelay(1);
	} while (!(auto_neo_complete) && --count);

	if (count <= 0) {
		printf("Auto-Negotiation External PHY Fail\n");

	} else {
		printf("Auto-Negotiation External PHY Complete\n");
	}	

	return (count > 0) ? 1 : 0;
}

/* Re-init the external PHY for the test mode in opt */
int eth_init_exPhy(u8 mac_idx, u8 phyaddr, eth_test_options opt,
			 u8 reg)
{
	int status = 0;

	//Reset PHY
	mdio_reset_phy(mac_idx, phyaddr, reg);

	//Setup PHY mode: SGMII (System mode) to Copper
	if (opt.exPhy_mode == ENA_EX_PHY_MODE) {
		mdio_setup_phy_mode(mac_idx, phyaddr, reg);
	}

	// Setup bypass AN for PHY
	if (opt.exPhy_an_bypass == ENA_EX_PHY_AN_BYBASS) {
		mdio_set_sgmii_an_bypass_mode(mac_idx, phyaddr);
	}

	//Advertise 1000base_t and setup AN for copper
	if (opt.eth_run_test_ops == EXTERNAL_PHY_RJ45_LOOPBACK ||
	    opt.eth_run_test_ops == EXTERNAL_PHY_COPPER) {
		if (reg == MARVELL_PHY)
			mdio_set_advertise_1000base_t_full_duplex(mac_idx,
								  phyaddr);
		mdio_setup_copper_an(mac_idx, phyaddr, reg);
	}

	mdio_phy_config(mac_idx, phyaddr, reg);

	if (reg == REALTEK_PHY)
		realtek_conf(mac_idx, phyaddr);

	if (opt.eth_run_test_ops == EXTERNAL_PHY_RJ45_LOOPBACK) {
		mdio_set_stub_lb(mac_idx, phyaddr, reg);
		status = mdio_wait_copper_linkup(mac_idx, phyaddr, reg);
		if (status == 1) {
			printf("Copper - Link Up!\n");
		} else {
			printf("Copper - Link Down!\n");
		}
	} else if (opt.eth_run_test_ops == EXTERNAL_PHY_COPPER) {
		status = mdio_wait_copper_linkup(mac_idx, phyaddr, reg);
		printf("Copper - Link %s!\n", status == 1 ? "Up" : "Down");
	} else if (opt.eth_run_test_ops == EXTERNAL_PHY_LOOPBACK) {
		mdio_enable_exphy_lb(mac_idx, phyaddr, reg);
	}

	// Setup bypass AN for PHY
	if (opt.exPhy_an_bypass == ENA_EX_PHY_AN_BYBASS) {
		mdio_set_phy_an(mac_idx, phyaddr, reg);
	}

	return status;
}

/*
 * T-Unit software reset.
 * 31.F001[13] - 0: normal; 1: reset
 */
void eth_xgmii_TUNIT_sw_reset(u32 phyaddr)
{
	u32 mac_base = XGMII_MAC_BASE;
	u32 reg_val = 0;

	reg_val = mdio_clause45_read(mac_base, phyaddr, 31, 0xF001);
	/* Bit 13: software reset the T Unit */
	reg_val |= (1 << 13);
	mdio_clause45_write(mac_base, phyaddr, 31, 0xF001, reg_val);
	while (reg_val & (1 << 13)) {
		reg_val = mdio_clause45_read(mac_base, phyaddr, 31, 0xF001);
		mdelay(100);
	}
	printf("Done SW Reset T Unit\n");
}
/*
 * Copper software reset.
 * PMA/PMD - 1.0000[15] - 0: normal; 1: reset
 * PCS     - 3.0000[15] - 0: normal; 1: reset
 */
void eth_xgmii_copper_sw_reset(u32 phyaddr)
{
	u32 mac_base = XGMII_MAC_BASE;
	u32 reg_val = 0;

	reg_val = mdio_clause45_read(mac_base, phyaddr, 1, 0x0000);
	/* Bit 15: PMA/PMD software reset */
	reg_val |= (1 << 15);
	mdio_clause45_write(mac_base, phyaddr, 1, 0x0000, reg_val);
	while (reg_val & (1 << 15)) {
		reg_val = mdio_clause45_read(mac_base, phyaddr, 1, 0x0000);
		mdelay(100);
	}
	printf("Done PMA/PMD SW Reset Copper\n");

	/* Bit 15: PCS software reset */
	reg_val = mdio_clause45_read(mac_base, phyaddr, 3, 0x0000);
	reg_val |= (1 << 15);
	mdio_clause45_write(mac_base, phyaddr, 3, 0x0000, reg_val);
	while (reg_val & (1 << 15)) {
		reg_val = mdio_clause45_read(mac_base, phyaddr, 3, 0x0000);
		mdelay(100);
	}
	printf("Done PCS SW Reset Copper\n");
}
/* Read the firmware revision */
static inline int eth_read_fw_revision(u32 phyaddr)
{
	u32 mac_base = XGMII_MAC_BASE;
	u32 revison_ab = 0, revison_cd = 0, revison = 0;
	int status;

	revison_ab = mdio_clause45_read(mac_base, phyaddr, 1, 0xc011);
	revison_cd = mdio_clause45_read(mac_base, phyaddr, 1, 0xc012);

	if ((revison_ab != 0) && (revison_ab != 0xffff) && (revison_cd != 0) &&
	    (revison_cd != 0xffff)) {
		revison = (revison_ab << 16) | revison_cd;
		printf("Found Firmware Revison: %d", revison);
		status = 1;
	} else {
		printf("Firmware not found");
		status = 0;
	}
	return status;
}
void xgmii_enable_phy_loopback(u32 mac_base, u32 phyaddr)
{
	u32 reg_val = 0;
	reg_val = mdio_clause45_read(mac_base, phyaddr, 0x3,
				     0x0); //Default 0xA040
	reg_val |= (1 << 14); //Enable PHY Loopback
	mdio_clause45_write(mac_base, phyaddr, 0x3, 0x0, reg_val); //Val 0xE040
	printf("LB Setting");
	mdelay(1000);
	reg_val = mdio_clause45_read(mac_base, phyaddr, 3, 0x0000);
}
void mdio_phy_88e1512_packet_64bytes_gen(uint8_t mac_idx, uint8_t phy_addr, uint8_t reg){
    mdio_select_page(mac_idx, phy_addr, reg, 6); //select page 6

    /* Gap between packets of the PHY packet generator (copper) */
    mdio_clause22_write(mac_idx, phy_addr, 19, 0x0C);  // IPG = 12 byte

    /* CRC counter reset */
    mdio_clause22_write(mac_idx, phy_addr, 18, 0x10);      // Reg 18, bit 4 = 1
    mdelay(100);

    // Config Packet Generator:
    uint16_t pg_ctrl =
        (ETH_PACKET_CRC_ERROR << 8) |  // burst = 1 packet
        (0 << 7)    |  // Trigger send  (0: start)
        (0 << 6)    |  // no self-clear
        (1 << 4)    |  // Enable CRC checker
        (1 << 3)    |  // Enable PG
        (1 << 2)    |  // Payload pseudo-random
        //(1 << 1)    |  // Packet length: 64 bytes | 1518 bytes
        (1 << 0);      // TX Packet with CRC Error

    mdio_clause22_write(mac_idx, phy_addr, 16, pg_ctrl);  // start send packet
    mdio_select_page(mac_idx, phy_addr, reg, 0); //return page 0
}

/* Disable the 88E1512 packet generator (page 6 reg 16) */
void mdio_phy_88e1512_packet_gen_stop(u8 mac_idx, u8 phy_addr, u8 reg)
{
	mdio_select_page(mac_idx, phy_addr, reg, 6); //select page 6
	mdio_clause22_write(mac_idx, phy_addr, 16, 0);
	mdio_select_page(mac_idx, phy_addr, reg, 0); //return page 0
}

/* Debug: 88E1512 CRC checker, page 18 reg 17: [15:8] = pkt, [7:0] = crc err */
void mdio_check_packet_gen_result(u8 mac_idx, u8 phy_addr, u8 reg)
{
	u32 regval;

	mdio_select_page(mac_idx, phy_addr, reg, 18); //select page 18
	regval = mdio_clause22_read(mac_idx, phy_addr, 17);
	printf("PHY - CRC Checker: %x\n", regval);
	printf("PHY - RX Packet: %x\n", (regval >> 8) & 0xFF);
	printf("PHY - CRC Error: %x\n", regval & 0xFF);
	regval = mdio_clause22_read(mac_idx, phy_addr, 18);
	printf("reg18:%x\n", regval);
	mdio_select_page(mac_idx, phy_addr, reg, 0); //return page 0
}
