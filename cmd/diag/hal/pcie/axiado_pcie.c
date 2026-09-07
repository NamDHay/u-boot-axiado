// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#include <dm.h>
#include <stdio.h>
#include <asm/io.h>
#include <linux/printk.h>
#include <time.h>
#include <linux/delay.h>
#include <malloc.h>
#include <linux/log2.h>

#include "ax_diag.h"
#include "ax_pcie.h"
#include "axiado_pcie.h"
#include "ax_pcie_conf.h"

#define PCIE_LINKUP_TIMEOUT 10000
#define PCIE_MAX_GEN 0x4
#define LTSSM_MAX_NUM_STATE 27

const char ltssm_state_char[LTSSM_MAX_NUM_STATE][32] = {"DETECT.QUIET",
                                                        "DETECT.ACTIVE",
                                                        "POLLING.ACTIVE",
                                                        "POLLING.COMPLIANCE",
                                                        "POLLING.CFG",
                                                        "CFG.LINKWIDTHSTART",
                                                        "CFG.LINKWIDTHACCEPT",
                                                        "CFG.LANEN_WAIT",
                                                        "CFG.LANEN_ACCEPT",
                                                        "CFG.COMPLETE",
                                                        "CFG.IDLE",
                                                        "RECOVERY.RCV_LOCK",
                                                        "RECOVERY.EQ",
                                                        "RECOVERY.SPEED",
                                                        "RECOVERY.RCV_CFG",
                                                        "RECOVERY.IDLE",
                                                        "L0",
                                                        "L0S",
                                                        "L1.ENTRY",
                                                        "L1.IDLE",
                                                        "L2.IDLE/L2.TRANSMITWAKE",
                                                        "RESERVED",
                                                        "DISABLE",
                                                        "LOOPBACK.ENTRY",
                                                        "LOOPBACK.ACTIVE",
                                                        "LOOPBACK.EXIT",
                                                        "HOTRESET"};

static struct axiado_pcie pcie_x1 = {
    .pcie_x1 = true,
    .pcie_x2 = false,

    .lanes = 1,

    .csr = (void __iomem *)0x45200000,
    .phy = (void __iomem *)0x45300000,
    .cfg = (void __iomem *)0x45203000,
    .bridge = (void __iomem *)0x45202000,
    .mbx = (void __iomem *)0x45280000,
    .ext = (void __iomem *)0x45240000,
    .ecam = (void __iomem *)0x7E0000000,

    .atr = {
        { .base = (void __iomem *)0x700000000, .size = SZ_256M, .flags = PCI_BAR_MEM_TYPE_64 } ,
        { .base = (void __iomem *)0x740000000, .size = SZ_256M, .flags = PCI_BAR_PREFETCH } ,
    },

    .bar = {
        { .base = (void __iomem *)0x92000000, .size = SZ_256M, .flags = PCI_BAR_MEM_TYPE_64 } ,
        { .base = (void __iomem *)0xA2000000, .size = SZ_256M, .flags = PCI_BAR_PREFETCH } ,
    },
};

static struct axiado_pcie pcie_x2 = {
    .pcie_x1 = false,
    .pcie_x2 = true,
    
    .lanes = 2,

    .csr = (void __iomem *)0x45000000,
    .phy = (void __iomem *)0x45100000,
    .cfg = (void __iomem *)0x45003000,
    .bridge = (void __iomem *)0x45002000,
    .mbx = (void __iomem *)0x45080000,
    .ext = (void __iomem *)0x45040000,
    .ecam = (void __iomem *)0x5E0000000,

    .atr = {
        { .base = (void __iomem *)0x500000000, .size = SZ_256M, .flags = PCI_BAR_MEM_TYPE_64 } ,
        { .base = (void __iomem *)0x540000000, .size = SZ_256M, .flags = PCI_BAR_PREFETCH } ,
    },

    .bar = {
        { .base = (void __iomem *)0xA2000000, .size = SZ_256M, .flags = PCI_BAR_MEM_TYPE_64 } ,
        { .base = (void __iomem *)0xA3000000, .size = SZ_256M, .flags = PCI_BAR_PREFETCH } ,
    },
};

static struct axiado_pcie *axiado_pcie_get_port(u8 port)
{
    switch (port) {
    case 0x1:
        return &pcie_x1;

    case 0x2:
        return &pcie_x2;

    default:
        return NULL;
    }
}

static int is_port_pll_locked(struct axiado_pcie *pcie) {
    u32 offset, val;

    offset = pcie->pcie_x1 ?
        PCIE_X1_PLL_LOCK_OFFSET :
        PCIE_X2_PLL_LOCK_OFFSET;

    val = readl(pcie->phy + offset);
    return (val != 0);
}

static int get_ltssm_state(struct axiado_pcie *pcie) {
    u32 ltssm_state, val;

    if (!is_port_pll_locked(pcie)) {
        ltssm_state = 0;
    } else {
        val = readl(pcie->csr + LINK_STATUS_LOW_POWER_ADDR);
        ltssm_state = (val >> 8) & 0x1F;

        if (ltssm_state >= LTSSM_MAX_NUM_STATE) {
            ltssm_state = 0;
        }
    }

    return ltssm_state;
}


static unsigned int axiado_pcie_conf_offset(u8 bus, u8 dev, u8 func,
        unsigned int where)
{
    return (bus  << 20) |
        (dev  << 15) |
        (func << 12) |
        (where & (SZ_4K - 1) & ~3);
}

static int axiado_pcie_map_bus(struct axiado_pcie *pcie, u8 bus, u8 dev, u8 func,
        int where, unsigned long long *address)
{
    unsigned int offset, ltssm_state;

    ltssm_state = get_ltssm_state(pcie);

    if (!pcie->is_root_port)
        return -ENOTSUPP;

    if (bus > 2)
        return -ENODEV;

    /* Dont support targetting RP config space with */
    /* function number other than function 0 */
    if (func)
        return -ENOTSUPP;

    if (ltssm_state != S_L0) {
        *address = (unsigned long)pcie->cfg + (where & ~3);
    } else {
        offset = axiado_pcie_conf_offset(bus, dev, func, where);
        *address = (unsigned long long)pcie->ecam + offset;
    }

    return 0;
}

static int axiado_pcie_cfg_read(int port, u8 bus, u8 dev, u8 func,
        u16 offset, u32 *val)
{
    struct axiado_pcie *pcie;
    unsigned long long address;
    unsigned long long value;
    int err;

    pcie = axiado_pcie_get_port(port);
    if (!pcie)
        return -EINVAL;

    err = axiado_pcie_map_bus(pcie, bus, dev, func, offset, &address);
    if (err < 0) {
        value = 0xffffffff;
        return 0;
    }
    *val = readl(address);

    return 0;
}

static int axiado_pcie_cfg_write(int port, u8 bus, u8 dev, u8 func,
        u16 offset, u32 val)
{
    struct axiado_pcie *pcie;
    unsigned long long address;
    int err;

    pcie = axiado_pcie_get_port(port);
    if (!pcie)
        return -EINVAL;

    err = axiado_pcie_map_bus(pcie, bus, dev, func, offset, &address);
    if (err < 0)
        return 0;

    writel(val, address);

    return 0;
}

static int axiado_pcie_mem_read(u8 port, u64 offset,
                                void *buf, size_t len)
{
    struct axiado_pcie *pcie;
    void __iomem *addr;
    u8 *dst = buf;
    u32 val;

    if (!buf || !len)
        return -EINVAL;

    pcie = axiado_pcie_get_port(port);
    if (!pcie)
        return -EINVAL;

    addr = pcie->atr[0].base + offset;

    while (len >= sizeof(u32)) {
        val = readl(addr);
        memcpy(dst, &val, sizeof(val));

        addr += sizeof(u32);
        dst += sizeof(u32);
        len -= sizeof(u32);
    }

    while (len) {
        *dst++ = readb(addr++);
        len--;
    }

    return 0;
}

static int axiado_pcie_mem_write(u8 port, u64 offset,
                                 const void *buf, size_t len)
{
    struct axiado_pcie *pcie;
    void __iomem *addr;
    const u8 *src = buf;
    u32 val;

    if (!buf || !len)
        return -EINVAL;

    pcie = axiado_pcie_get_port(port);
    if (!pcie)
        return -EINVAL;

    addr = pcie->atr[0].base + offset;

    while (len >= sizeof(u32)) {
        memcpy(&val, src, sizeof(val));
        writel(val, addr);

        addr += sizeof(u32);
        src += sizeof(u32);
        len -= sizeof(u32);
    }

    while (len) {
        writeb(*src++, addr++);
        len--;
    }

    return 0;
}

static int axiado_pcie_release_reset(struct axiado_pcie *pcie)
{
    u32 value;

    printf("%s - x%d\n", __func__, pcie->lanes);

    if (pcie->pcie_x1) {
        value = readl(pcie->ext + REG_PCIE_X1_RESET_CTRL_ADRS_OFFSET);
    } else {
        value = readl(pcie->ext + REG_PCIE_X2_RESET_CTRL_ADRS_OFFSET);
    }    

    value = REG_PCIE_RESET_CTRL_PCIE_RSTN_SET(value, 0x1);
    value = REG_PCIE_RESET_CTRL_HOLD_PIPE_RST_SET(value, 0x1);

    if (pcie->pcie_x1) {
        writel(value, pcie->ext + REG_PCIE_X1_RESET_CTRL_ADRS_OFFSET);
    } else {
        writel(value, pcie->ext + REG_PCIE_X2_RESET_CTRL_ADRS_OFFSET);
    }    
    mdelay(100);

    return 0;
}

static int axiado_pcie_configure_controller(struct axiado_pcie *pcie) 
{
    uint32_t val;
    uint64_t csr_pcie_ext_ip_ctrl_off, reg_pcie_gen_settings_off;
    uint64_t csr_pcie_pex_dev_adrs_off;
    uint64_t csr_base_imask_local_off;

    printf("%s - x%d\n", __func__, pcie->lanes);

    if (pcie->pcie_x1){
        csr_pcie_ext_ip_ctrl_off = REG_PCIE_X1_IP_CTRL_ADRS_OFFSET;
        reg_pcie_gen_settings_off = REG_PCIE_GEN_SETTINGS_ADRS_OFFSET;
        csr_pcie_pex_dev_adrs_off = REG_PCIE_PEX_DEV_ADRS_OFFSET;
        csr_base_imask_local_off = REG_PCIE_IMASK_LOCAL_ADRS_OFFSET;
    } else if (pcie->pcie_x2) {
        csr_pcie_ext_ip_ctrl_off = REG_PCIE_X2_IP_CTRL_ADRS_OFFSET;
        reg_pcie_gen_settings_off = REG_PCIE_GEN_SETTINGS_ADRS_OFFSET;
        csr_pcie_pex_dev_adrs_off = REG_PCIE_PEX_DEV_ADRS_OFFSET;
        csr_base_imask_local_off = REG_PCIE_IMASK_LOCAL_ADRS_OFFSET;
    } else {
        return -EINVAL;
    }

    /* Core configuration should only be modified in reset mode */
    /* Set supported speeds and RP/EP in k_gen settings */
    printf("\tset tl_clk freq to %d MHz\n\r", AX_PCIE_TL_CLOCK_FREQ_MHZ);
    val = readl(pcie->ext + csr_pcie_ext_ip_ctrl_off);
    val = REG_PCIE_IP_CTRL_FREQ_SET(val, AX_PCIE_TL_CLOCK_FREQ_MHZ);
    if (pcie->is_root_port) {
        /* set GEN_SETTINGS BIT0 to set RP mode */
        val = REG_PCIE_IP_CTRL_RP_NEP_SET(val, 1);  // 1--RP
    } else {
        val = REG_PCIE_IP_CTRL_RP_NEP_SET(val, 0);  // 0--EP
    }
    val = REG_PCIE_IP_CTRL_TL_CLK_GATE_EN_SET(val, 0);
    writel(val, pcie->ext + csr_pcie_ext_ip_ctrl_off);

    /* KSET setting */
    val = readl(pcie->bridge + reg_pcie_gen_settings_off);
    /* 0--EP 1--RP */
    if (pcie->is_root_port) {
        val = PCIE_K_SET_PCIE_PORT_TYPE_SET(val, 1);
        /* Gen supported from Gen2 -> speed */
        val = PCIE_K_SET_LINK_SPEED_SET(val, ((0x1 << (pcie->speed - 1)) - 1));
    } else {
        /* Don't set target gen for PCIE_EP, keep EP configured at default gen */
        val = PCIE_K_SET_PCIE_PORT_TYPE_SET(val, 0);
    }

    if (pcie->pcie_x2) {
        val = PCIE_K_SET_LINK_WIDTH_X2_SET(val, 0x1);
    }

    if (IS_LANE_REVERSAL_EN) {
        val = PCIE_K_SET_LANE_REVERSAL_EN_SET(val, 0x1);
    }

    val = PCIE_K_SET_EXT_INTR_MODE_SET(val, 0x1);
    writel(val, pcie->bridge + reg_pcie_gen_settings_off);

    /* Setting max payload size */
    val = readl(pcie->bridge + csr_pcie_pex_dev_adrs_off);
    val = PCIE_PEX_DEV_MAX_PAYLOAD_SET(val, PAYLOAD_256_BYTE);
    writel(val, pcie->bridge + csr_pcie_pex_dev_adrs_off);
    return 0;
}

static int axiado_pcie_config_eq_tunning(struct axiado_pcie *pcie)
{
    u8 lane;
    u32 val, val1, val2;
    u64 csr_base_int_phymac_offset;
    u64 csr_base_int_pcie_pex_spc;
    u32 gen3_rx_tx_preset;
    u32 gen4_rx_tx_preset;

    printf("%s - gen%d - x%d\n\r", __func__, pcie->speed, pcie->lanes);

    if (pcie->pcie_x1){
        csr_base_int_phymac_offset = REG_PCIE_PHYMAC_CFG_ADRS_OFFSET;
        csr_base_int_pcie_pex_spc = REG_PCIE_PEX_SPC_ADRS_OFFSET;
    } else if (pcie->pcie_x2) {
        csr_base_int_phymac_offset = REG_PCIE_PHYMAC_CFG_ADRS_OFFSET;
        csr_base_int_pcie_pex_spc = REG_PCIE_PEX_SPC_ADRS_OFFSET;
    } else {
        return -EINVAL;
    }

    // enable EQ PH2,3
    val = readl(pcie->bridge + csr_base_int_phymac_offset);
    val = PCIE_PHYMAC_CFG_PER_EQ_PHASE_2_3_SET(val, 0x1);
    val = PCIE_PHYMAC_CFG_RXELECIDLE_SET(val, 0x1);
    val = PCIE_PHYMAC_CFG_RX_VALID_FILTER_EN_SET(val, 0x1);
    val = PCIE_PHYMAC_CFG_ENHANCE_EQ_SET(val, PERFORM_EQ_ALL_SPEED_BEFORE_NRZ);
    if (IS_COMMON_SLOT_CLOCK_SET) {
        val = PCIE_PHYMAC_CFG_CLOCK_IS_COMMON_SET(val, 0x1);
    }
    writel(val, pcie->bridge + csr_base_int_phymac_offset);

    if (IS_COMMON_SLOT_CLOCK_SET) {
        val = readl(pcie->bridge + csr_base_int_pcie_pex_spc);
        val = PCIE_PEX_SPC_SLOT_REG_IMPL_SET(val, 0x1);
        val = PCIE_PEX_SPC_SLOT_CLK_CONFIG_SET(val,
                0x1);  // Slot clock configuration
                       // (0:independent, 1:refclk)
        writel(val, pcie->bridge + csr_base_int_pcie_pex_spc);
    }

    //  set preset value and max fine tuning attempts
    val1 = readl(pcie->bridge + REG_PCIE_EQ_TUNING_31_0_ADRS_OFFSET);
    val2 = readl(pcie->bridge + REG_PCIE_EQ_TUNING_63_32_ADRS_OFFSET);
    val = ((u64)val2 << 32) | ((u64)val1);

    val = PCIE_EQ_TUNING_63_0_PRESET_TUNING_SETTINGS_SET(val, BEST_PRESET_REAPPLIED_CONT);
    val = PCIE_EQ_TUNING_63_0_CONT_FINE_TUNE_EVEN_IF_NO_COEFF_SET(val, 0x1);
    val = PCIE_EQ_TUNING_63_0_GEN3_PRESET_SET(val, PCIE_GEN3_PRESET_VECTOR);
    val = PCIE_EQ_TUNING_63_0_GEN3_MAX_TUNING_ITER_SET(val, PCIE_GEN3_MAX_TUNING_ITER);

    val1 = ((u64)val) & 0xFFFFFFFF;
    val2 = ((u64)val >> 32) & 0xFFFFFFFF;
    writel(val1, pcie->bridge + REG_PCIE_EQ_TUNING_31_0_ADRS_OFFSET);
    writel(val2, pcie->bridge + REG_PCIE_EQ_TUNING_63_32_ADRS_OFFSET);

    gen3_rx_tx_preset =
        PCIE_GEN3_RX_TX_PRESET_GET(PCIE_GEN3_TX_PRESET, PCIE_GEN3_RX_PRESET);
    // 1 lane need 16-bit config
    // 2 lane need 2*8 = 32-bit config ==> 32-bit is the size of a singe register
    // access so 2 lane is set at a time
    for (lane = 0; lane < 16; lane += 2) {
        writel(gen3_rx_tx_preset, pcie->bridge + REG_PCIE_EQ_PRESET_8G_31_0_ADRS_OFFSET + sizeof(u32) * (lane / 2));
    }

    gen4_rx_tx_preset = PCIE_GEN4_RX_TX_PRESET_GET(PCIE_GEN4_TX_PRESET, PCIE_GEN4_RX_PRESET);
    // 1 lane need 8-bit config
    // 4 lane need 4*8 = 32-bit config ==> 32-bit is the size of a singe register
    // access so 4 lane is set at a time
    for (lane = 0; lane < 16; lane += 4) {
        writel(gen4_rx_tx_preset, pcie->bridge + REG_PCIE_EQ_PRESET_16G_31_0_ADRS_OFFSET + sizeof(u32) * (lane / 4));
    }

    val = readl(pcie->bridge + REG_PCIE_PEX_SPC_ADRS_OFFSET);
    val = val | 0x3000; 
    writel(val, pcie->bridge + REG_PCIE_PEX_SPC_ADRS_OFFSET);

    return 0;
}

static int axiado_pcie_config_class_code(struct axiado_pcie *pcie)
{
    if (pcie->is_root_port) {
        /* Change class to PCIe Bridge */
        writel(PCIE_BRIDGE_CLASS_CODE, pcie->bridge +
                REG_PCIE_PCI_IDS_63_32_ADRS_OFFSET);
    } else {
        /* Change class to Trusted Services (PCIe End-Point) */
        writel(PCIE_TRUSTEDS_CLASS_CODE, pcie->bridge +
                REG_PCIE_PCI_IDS_63_32_ADRS_OFFSET);
    }

    return 0;
}

static int axiado_pcie_pll_wait(struct axiado_pcie *pcie, unsigned long timeout_ms)
{
    u32 val;
    u32 offset;
    ulong start;

    if (!pcie)
        return -EINVAL;

    offset = pcie->pcie_x1 ?
        PCIE_X1_PLL_LOCK_OFFSET :
        PCIE_X2_PLL_LOCK_OFFSET;

    start = get_timer(0);
    while (get_timer(start) < timeout_ms) {
        val = readl(pcie->phy + offset);
        if (val) {
            return 0;
        }
    }

    return -ETIMEDOUT;
}

/*
 * If there are no PCIe cards attached, then calling this function
 * can result in the increase of the bootup time as there are big timeout
 * loops.
 */
static int axiado_pcie_port_check_link(struct axiado_pcie *port, unsigned long timeout_ms)
{
    ulong start;
    u32 val, ltssm_state, cur_speed, cur_width;

    if (!port) {
        printf("Invalid Arguments passed for Link status check\n");
        return -EINVAL;
    }

    /* wait for LinkUp(at Gen1) */
    start = get_timer(0);
    while (get_timer(start) < timeout_ms) {
        val = readl(port->bridge + REG_PCIE_BASIC_STATUS_ADRS_OFFSET);
        cur_width = (val & 0xFF);
        cur_speed = (val >> 8) & 0x1F;

        val = readl(port->csr + LINK_STATUS_LOW_POWER_ADDR);
        ltssm_state = (val >> 8) & 0x1F;
        if ((ltssm_state == S_L0) && (cur_speed == port->speed) && (cur_width == port->lanes)) {
            printf("\tltssm_state = 0x%x\n\r", ltssm_state);
            printf("\tcur_speed = %d\n\r", cur_speed);
            printf("\tcur_width = %d\n\r", cur_width);
            break;
        }
    }

    if (get_timer(start) < timeout_ms) {
        printf("Link successfully reach ");
    } else if (ltssm_state == S_L0) {
        printf("Polling timeout but link reach ");
    } else {
        printf("Link %u DOWN, ignoring\n", port->lanes);
        return -ENODEV;
    }

    val = readl(port->bridge + REG_PCIE_BASIC_STATUS_ADRS_OFFSET);
    cur_width = (val & 0xFF);
    cur_speed = (val >> 8) & 0x1F;

    ltssm_state = get_ltssm_state(port);

    printf("Linkup %s - Gen%dx%d\n\r", ltssm_state_char[ltssm_state], cur_speed, cur_width);
    return 0;
}

static int axiado_pcie_enable_ltssm(struct axiado_pcie *pcie)
{
    u32 val;

    /* enable LTSSM — clear DISABLE_LTSSM (bit 2) in CFGCTRL */
    val = readl(pcie->bridge + REG_PCIE_CFGCTRL_ADRS_OFFSET);
    printf("LTSSM enable [0x%llx]: 0x%x -> 0x%x\n",
            (u64)pcie->bridge + REG_PCIE_CFGCTRL_ADRS_OFFSET,
            val, val & PCIE_PHY_CONF_CTRL_LTSSM);
    writel(val & PCIE_PHY_CONF_CTRL_LTSSM, pcie->bridge + REG_PCIE_CFGCTRL_ADRS_OFFSET);

    mdelay(100);

    return 0;
}

static int axiado_pcie_setup_a2p_atr(struct axiado_pcie *pcie, u8 table_num, u64 src_addr,
        u64 trsl_addr, size_t window_size, int trsl_param)
{
    u32 offset = PCIE_ATR_AXI4_SLV0;
    u64 trsl_mask;

    if (table_num >= TABLE_MAX) {
        printf("%s - Invalid table %d\n", __func__, table_num);
        return -EINVAL;
    }

    offset +=  PCIE_ATR_TABLE_OFFSET * table_num;

    /* PCIE_ATR_SRC_ADDR_LOW:
     *   - bit 0: enable entry,
     *   - bits 1-6: ATR window size: total size in bytes: 2^(ATR_WSIZE + 1)
     *   - bits 7-11: reserved
     *   - bits 12-31: start of source address
     */
    writel((((src_addr & 0xFFFFF000) >> 12) & PCIE_ATR_SRC_ADDR_MASK) |
            (ilog2(window_size) - 1) << PCIE_ATR_SRC_WIN_SIZE_SHIFT | 1,
            pcie->bridge + (offset + PCIE_ATR_SRC_ADDR_LOW));

    writel(((src_addr >> 32) & 0xFFFFFFFF),
            pcie->bridge + (offset + PCIE_ATR_SRC_ADDR_HIGH));

    writel((((trsl_addr & 0xFFFFF000) >> 12) & PCIE_ATR_TRSL_ADDR_MASK),
            pcie->bridge + (offset + PCIE_ATR_TRSL_ADDR_LOW));

    writel(((trsl_addr >> 32) & 0xFFFFFFFF),
            pcie->bridge + (offset + PCIE_ATR_TRSL_ADDR_HIGH));

    writel(trsl_param, pcie->bridge + (offset+PCIE_ATR_TRSL_PARAM));

    trsl_mask = (uint64_t)(~((uint64_t)(ilog2(window_size)) - 1));
    writel((uint32_t)(trsl_mask & 0xFFFFFFFF), pcie->bridge + (offset + PCIE_ATR_TRSL_MASK_OFFSET));
    writel((uint32_t)((trsl_mask >> 32) & 0xFFFFFFFF), pcie->bridge + (offset + PCIE_ATR_TRSL_MASK_OFFSET + sizeof(uint32_t)));

    printf("A2P entry [table: %d]: 0x%010llx %s 0x%010llx [0x%010llx] (param: 0x%06x)\n",
            table_num, src_addr, (trsl_param & PCIE_ATR_TRSL_DIR) ? "<-" : "->",
            trsl_addr, (u64)window_size, trsl_param);
    return 0;
}

static int axiado_pcie_setup_p2a_atr(struct axiado_pcie *pcie, u8 table_num, u64 src_addr,
        u64 trsl_addr, size_t window_size, int trsl_param)
{
    u32 offset = PCIE_ATR_PCIE_WIN0;
    u64 trsl_mask;

    if (table_num >= TABLE_MAX) {
        printf("%s - Invalid table %d\n", __func__, table_num);
        return -EINVAL;
    }

    offset +=  PCIE_ATR_TABLE_OFFSET * table_num;

    /* PCIE_ATR_SRC_ADDR_LOW:
     *   - bit 0: enable entry,
     *   - bits 1-6: ATR window size: total size in bytes: 2^(ATR_WSIZE + 1)
     *   - bits 7-11: reserved
     *   - bits 12-31: start of source address
     */
    writel((((src_addr & 0xFFFFF000) >> 12) & PCIE_ATR_SRC_ADDR_MASK) |
            (ilog2(window_size) - 1) << PCIE_ATR_SRC_WIN_SIZE_SHIFT | 1,
            pcie->bridge + (offset + PCIE_ATR_SRC_ADDR_LOW));

    writel(((src_addr >> 32) & 0xFFFFFFFF),
            pcie->bridge + (offset + PCIE_ATR_SRC_ADDR_HIGH));

    writel((((trsl_addr & 0xFFFFF000) >> 12) & PCIE_ATR_TRSL_ADDR_MASK),
            pcie->bridge + (offset + PCIE_ATR_TRSL_ADDR_LOW));

    writel(((trsl_addr >> 32) & 0xFFFFFFFF),
            pcie->bridge + (offset + PCIE_ATR_TRSL_ADDR_HIGH));

    writel(trsl_param, pcie->bridge + (offset+PCIE_ATR_TRSL_PARAM));

    trsl_mask = (uint64_t)(~((uint64_t)(ilog2(window_size)) - 1));
    writel((uint32_t)(trsl_mask & 0xFFFFFFFF), pcie->bridge + (offset + PCIE_ATR_TRSL_MASK_OFFSET));
    writel((uint32_t)((trsl_mask >> 32) & 0xFFFFFFFF), pcie->bridge + (offset + PCIE_ATR_TRSL_MASK_OFFSET + sizeof(uint32_t)));

    printf("P2A entry [table: %d]: 0x%010llx %s 0x%010llx [0x%010llx] (param: 0x%06x)\n",
            table_num, src_addr, (trsl_param & PCIE_ATR_TRSL_DIR) ? "<-" : "->",
            trsl_addr, (u64)window_size, trsl_param);

    return 0;
}

static void axiado_pcie_setup_windows(struct axiado_pcie *pcie)
{
    int table;
    for (table = 0; table < ARRAY_SIZE(pcie->atr); table++) {
        if (pcie->is_root_port) {
            axiado_pcie_setup_a2p_atr(pcie, table, 
                    (u64)pcie->atr[table].base & 0xFFFFFFFF,
                    0x0,
                    pcie->atr[table].size,
                    PCIE_ATR_TRSLID_PCIE_MEMORY);
            axiado_pcie_setup_p2a_atr(pcie, table, 
                    0x0,
                    (u64)pcie->bar[table].base & 0xFFFFFFFF,
                    pcie->bar[table].size,
                    PCIE_ATR_TRSLID_AXI4_MASTER_0);
        } else {
            axiado_pcie_setup_p2a_atr(pcie, table, 
                    0x0,
                    (u64)pcie->bar[table].base,
                    pcie->bar[table].size,
                    PCIE_ATR_TRSLID_AXI4_MASTER_0);
        }
    }
}

static int axiado_pcie_init(u8 port, u8 controller_mode, u8 speed)
{
    struct axiado_pcie *pcie;
    int ret;
    u32 val;

    pcie = axiado_pcie_get_port(port);
    if (!pcie)
        return -EINVAL;

    if (speed > PCIE_MAX_GEN) {
        printf("Invalid PCIe Gen\n");
        return -EINVAL;
    }

    pcie->is_root_port = controller_mode;
    pcie->speed = speed;

    printf("%s - ", __func__);
    printf("%s - ", pcie->is_root_port == PCIE_RP ? "Rootport": "Endpoint");
    printf("gen%d - x%d\n", pcie->speed, pcie->lanes);

    ret = axiado_pcie_release_reset(pcie);
    if (ret < 0) {
        printf("pcie release reset failed\n");
        return ret;
    }

    ret = axiado_pcie_configure_controller(pcie);
    if (ret < 0) {
        printf("pcie set controller mode failed\n");
        return ret;
    }

    /* In case we need PHY tuning */
    ret = axiado_pcie_config_eq_tunning(pcie);
    if (ret < 0) {
        printf("pcie PHY tunning failed\n");
        return ret;
    }

    mdelay(100);

    axiado_pcie_config_class_code(pcie);
    if (pcie->is_root_port) {
        /* wait for PHY PLL lock -- Inno PCIe PHY doc says
         * PLL lock status is at offset 14'h23C0? but paddr
         * for PHY model is 12:0
         */
        ret = axiado_pcie_pll_wait(pcie, PCIE_LINKUP_TIMEOUT);
        if (ret < 0) {
            printf("PLL is not locked for port x%d, ignoring\n",
                    pcie->lanes);
            return ret;
        }
        printf("PLL Locked for port x%d\n", pcie->lanes);
    } else {

        /*
         * BAR registers in bridge space encode both size mask
         * and type.  Size mask = ~(size - 1) & 0xFFFFFFF0,
         * type goes in bits [3:0].  BAR1/BAR3 are upper 32
         * bits for 64-bit BARs.
         *
         * Must be configured while IP is in reset.
         */

        /* BAR0: size + type from DTS ranges flags */
        val = (~(pcie->bar[0].size - 1) & 0xFFFFFFF0) |
            pcie->bar[0].flags;
        writel(val, pcie->bridge + PCIE_BAR_01_OFFSET);
        /* BAR1: upper 32 bits for 64-bit BAR0 */
        writel(0xFFFFFFFF, pcie->bridge + PCIE_BAR_01_OFFSET + 4);

        /* BAR2: size + type (NOTE: HAL marks 0xEC as RO) */
        val = (~(pcie->bar[1].size - 1) & 0xFFFFFFF0) |
            pcie->bar[1].flags;
        writel(val, pcie->bridge + PCIE_BAR_23_OFFSET);
        /* BAR3: upper 32 bits for 64-bit BAR2 */
        writel(0xFFFFFFFF, pcie->bridge + PCIE_BAR_23_OFFSET + 4);

        /* Disable BAR4/5 — not used, clear default 4KB */
        writel(0x0, pcie->bridge + PCIE_BAR_45_OFFSET);
        writel(0x0, pcie->bridge + PCIE_BAR_45_OFFSET + 4);
    }

    /* release PIPE_RST_N for PCIe */
    if (pcie->pcie_x1)
        writel(0x5, pcie->ext + REG_PCIE_X1_RESET_CTRL_ADRS_OFFSET);
    else if (pcie->pcie_x2)
        writel(0x5, pcie->ext + REG_PCIE_X2_RESET_CTRL_ADRS_OFFSET);

    if (pcie->is_root_port) {
        axiado_pcie_setup_windows(pcie);

        /* Bus master enable */
        axiado_pcie_cfg_read(port, 0, 0, 0, PCI_CFG_COMMAND, &val);
        val |= PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;
        axiado_pcie_cfg_write(port, 0, 0, 0, PCI_CFG_COMMAND, val);
        
        /* Enumeration bus number to access Endpoint on the other side of the link */
        axiado_pcie_cfg_read(port, 0, 0, 0, PCIE_TYPE1_TIMER_SUB_SEC_PRI_OFF, &val);
        val = PCIE_TYPE1_PRI_BUS_SET(val, 0x0);   // Primary bus num
        val = PCIE_TYPE1_SEC_BUS_SET(val, 0x1);   // Secondary bus num
        val = PCIE_TYPE1_SUB_BUS_SET(val, 0xFF);  // Subordinary bus num
        axiado_pcie_cfg_write(port, 0, 0, 0, PCIE_TYPE1_TIMER_SUB_SEC_PRI_OFF, val);
    } else {
        printf("BAR01[%#llx]=%#x/%#x \nBAR23[%#llx]=%#x/%#x \nBAR45[%#llx]=%#x/%#x\n",
                (u64)pcie->bridge + PCIE_BAR_01_OFFSET,
                readl(pcie->bridge + PCIE_BAR_01_OFFSET),
                readl(pcie->bridge + PCIE_BAR_01_OFFSET + 4),
                (u64)pcie->bridge + PCIE_BAR_23_OFFSET,
                readl(pcie->bridge + PCIE_BAR_23_OFFSET),
                readl(pcie->bridge + PCIE_BAR_23_OFFSET + 4),
                (u64)pcie->bridge + PCIE_BAR_45_OFFSET,
                readl(pcie->bridge + PCIE_BAR_45_OFFSET),
                readl(pcie->bridge + PCIE_BAR_45_OFFSET + 4));
        printf("CFGCTRL[%#llx]=%#x GEN[%#llx]=%#x\n",
                (u64)pcie->bridge + 0x84,
                readl(pcie->bridge + 0x84),
                (u64)pcie->bridge + 0x80,
                readl(pcie->bridge + 0x80));

        /* Bus master + memory space enable in EP config space */
        udelay(10);
        axiado_pcie_cfg_read(port, 0, 0, 0, PCI_CFG_COMMAND, &val);
        val |= PCI_COMMAND_MASTER;
        axiado_pcie_cfg_write(port, 0, 0, 0, PCI_CFG_COMMAND, val);

        /* P2A ATR will be configured by EP link monitor after
         * host enumerates and assigns BAR addresses.
         * Pre-program translation targets now, source addresses
         * will be set once we read host-assigned BARs.
         */
        axiado_pcie_setup_windows(pcie);
        printf("P2A ATR pre-configured (src=0, will update after host enum)\n");
    }

    axiado_pcie_enable_ltssm(pcie);

    if (pcie->is_root_port){
        u32 vendor_id, class_code;

        /* TODO: testing PERST, not sure this is correct or not? */
        if (pcie->pcie_x1)
            val = readl(pcie->ext + REG_PCIE_X1_RESET_CTRL_ADRS_OFFSET);
        else if (pcie->pcie_x2)
            val = readl(pcie->ext + REG_PCIE_X2_RESET_CTRL_ADRS_OFFSET);

        val = REG_PCIE_RESET_CTRL_PCIE_RP_PERST_SET(val, 0);

        if (pcie->pcie_x1)
            writel(val, pcie->ext + REG_PCIE_X1_RESET_CTRL_ADRS_OFFSET);
        else if (pcie->pcie_x2)
            writel(val, pcie->ext + REG_PCIE_X2_RESET_CTRL_ADRS_OFFSET);

        if (pcie->pcie_x1)
            val = readl(pcie->ext + REG_PCIE_X1_RESET_CTRL_ADRS_OFFSET);
        else if (pcie->pcie_x2)
            val = readl(pcie->ext + REG_PCIE_X2_RESET_CTRL_ADRS_OFFSET);

        val = REG_PCIE_RESET_CTRL_PCIE_RP_PERST_SET(val, 1);

        if (pcie->pcie_x1)
            writel(val, pcie->ext + REG_PCIE_X1_RESET_CTRL_ADRS_OFFSET);
        else if (pcie->pcie_x2)
            writel(val, pcie->ext + REG_PCIE_X2_RESET_CTRL_ADRS_OFFSET);

        ret = axiado_pcie_port_check_link(pcie,
                PCIE_LINKUP_TIMEOUT);
        if (ret < 0)
            return ret;

        axiado_pcie_cfg_read(port, 0x1, 0x0, 0x0, PCI_CFG_CLASS_CODE, &class_code);
        axiado_pcie_cfg_read(port, 0x1, 0x0, 0x0, PCI_CFG_VENDOR_ID, &vendor_id);
        printf("BFM Vendor Id: 0x%x - Class Code: 0x%x\n\r", vendor_id, class_code);
        axiado_pcie_cfg_read(port, 0x1, 0x0, 0x0, PCI_CFG_COMMAND, &val);
        val |= PCI_COMMAND_MEMORY | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;
        axiado_pcie_cfg_write(port, 0x1, 0x0, 0x0, PCI_CFG_COMMAND, val);
    }

    return 0;
}

struct ax_pcie_ops pcie = {
    .init = axiado_pcie_init,
    .cfg_read = axiado_pcie_cfg_read,
    .cfg_write = axiado_pcie_cfg_write,
    .mem_read = axiado_pcie_mem_read,
    .mem_write = axiado_pcie_mem_write,
};
