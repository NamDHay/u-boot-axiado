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
#include <display_options.h>

#include "ax_diag.h"
#include "ax_pcie.h"
#include "axiado_pcie.h"
#include "ax_pcie_conf.h"

#define AXIADO_PCIE_NO_RP_INBOUND_ATR 1

#define PCIE_LINKUP_TIMEOUT 10000
#define PCIE_MAX_GEN 0x4
#define LTSSM_MAX_NUM_STATE 27

#define MAX_STACK_SIZE 10  // Define the maximum size of your stack

// The static array and the top index tracking variable
static uint8_t axi_pf_arr_fifo[MAX_STACK_SIZE];
static uint8_t axi_vf_arr_fifo[MAX_STACK_SIZE];
static int pf_top = -1;  // -1 indicates the stack is currently empty
static int vf_top = -1;  // -1 indicates the stack is currently empty

const char ltssm_state_char[LTSSM_MAX_NUM_STATE][32] = {
    "DETECT.QUIET",
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
        { 
            .bus_start = (void __iomem *)0x00000000, 
            .phys_start = (void __iomem *)0x700000000, 
            .size = SZ_1G, 
            .flags = PCI_BAR_MEM_TYPE_64 
        } ,
        { 
            .bus_start = (void __iomem *)0x00000000 + SZ_1G, 
            .phys_start = (void __iomem *)0x700000000 + SZ_1G, 
            .size = SZ_1G, 
            .flags = PCI_BAR_MEM_TYPE_64 
        } ,
    },

    .bar = {
        { 
            .bus_start = (void __iomem *)0x40200000, 
            .phys_start = (void __iomem *)0x40200000, 
            .size = SZ_1M, 
            .flags = PCI_BAR_MEM_TYPE_64 
        } , /* OCM Base */
        { 
            .bus_start = (void __iomem *)0x90000000, 
            .phys_start = (void __iomem *)0x90000000, 
            .size = SZ_64M, 
            .flags = PCI_BAR_PREFETCH | PCI_BAR_MEM_TYPE_64 
        } , /* DDR Base */
        { 
            .bus_start = (void __iomem *)0x45280000, 
            .phys_start = (void __iomem *)0x45280000, 
            .size = SZ_256K, 
            .flags = PCI_BAR_MEM_TYPE_64 
        } , /* MBX/MSIX Base */
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
        { 
            .bus_start = (void __iomem *)0x00000000, 
            .phys_start = (void __iomem *)0x500000000, 
            .size = SZ_1G, 
            .flags = PCI_BAR_MEM_TYPE_64 
        } ,
        { 
            .bus_start = (void __iomem *)0x00000000 + SZ_1G, 
            .phys_start = (void __iomem *)0x500000000 + SZ_1G, 
            .size = SZ_1G, 
            .flags = PCI_BAR_PREFETCH | PCI_BAR_MEM_TYPE_64 
        },
    },

    .bar = {
        { 
            .bus_start = (void __iomem *)0x40200000, 
            .phys_start = (void __iomem *)0x40200000, 
            .size = SZ_1M, 
            .flags = PCI_BAR_MEM_TYPE_64 
        } , /* OCM Base */
        { 
            .bus_start = (void __iomem *)0x90000000, 
            .phys_start = (void __iomem *)0x90000000, 
            .size = SZ_64M, 
            .flags = PCI_BAR_PREFETCH | PCI_BAR_MEM_TYPE_64 
        } , /* DDR Base */
        { 
            .bus_start = (void __iomem *)0x45080000, 
            .phys_start = (void __iomem *)0x45080000, 
            .size = SZ_256K, 
            .flags = PCI_BAR_MEM_TYPE_64 
        } , /* MBX/MSIX Base */
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

/*
 * LTSSM says the link is usable for config TLPs (Recovery / L0 / L0s / L1).
 * Testing only S_L0 is wrong: as soon as the link goes to L0s/L1 or a speed
 * change enters Recovery the access would fall back to the local RP config
 * space and return the root port's own registers.
 */
static bool axiado_pcie_link_active(struct axiado_pcie *pcie)
{
    u32 state = get_ltssm_state(pcie);

    return state >= S_RECOVERY_RCV_LOCK && state <= S_L1_IDLE;
}

static int axiado_pcie_map_bus(struct axiado_pcie *pcie, u8 bus, u8 dev, u8 func,
        int where, void __iomem **address)
{
    if (!pcie->is_root_port)
        return -ENOTSUPP;

    /* Bus 0 is the root port itself: always the local config space */
    if (bus == 0) {
        if (dev || func)
            return -ENODEV;

        *address = pcie->cfg + (where & (SZ_4K - 1) & ~3);
        return 0;
    }

    if (bus > 2)
        return -ENODEV;

    /* Point-to-point link: only device 0 exists right below the root port */
    if (bus == 1 && dev)
        return -ENODEV;

    /* No link => no config TLPs, never fall back to the RP registers */
    if (!axiado_pcie_link_active(pcie))
        return -ENODEV;

    *address = pcie->ecam + axiado_pcie_conf_offset(bus, dev, func, where);
    return 0;
}

static uint8_t pf_arr_push(uint8_t value) 
{
    if (pf_top >= (MAX_STACK_SIZE - 1)) {
        printf("Error: Stack Overflow!\n");
        return -EINVAL;
    }

    pf_top++;                         // Increment the top index
    axi_pf_arr_fifo[pf_top] = value;  // Store the value
    return 0;
}

static uint8_t pf_arr_pop(uint8_t *popped_value) 
{
    if (pf_top < 0) {
        printf("Error: Stack Underflow!\n");
        return -EINVAL;
    }

    *popped_value = axi_pf_arr_fifo[pf_top];  // Retrieve the value at the top
    pf_top--;                                 // Decrement the top index
    return 0;
}

static uint8_t vf_arr_push(uint8_t value) 
{
    if (vf_top >= (MAX_STACK_SIZE - 1)) {
        printf("Error: Stack Overflow!\n");
        return -EINVAL;
    }

    vf_top++;                         // Increment the top index
    axi_vf_arr_fifo[vf_top] = value;  // Store the value
    return 0;
}

static uint8_t vf_arr_pop(uint8_t *popped_value) 
{
    if (vf_top < 0) {
        printf("Error: Stack Underflow!\n");
        return -EINVAL;
    }

    *popped_value = axi_vf_arr_fifo[vf_top];  // Retrieve the value at the top
    vf_top--;                                 // Decrement the top index
    return 0;
}

void axiado_pcie_axi_set_pf(struct axiado_pcie *pcie, uint8_t pf_num) 
{
    uint8_t set_pf;
    uint32_t cur_pf;
    uint32_t cur_vf;
    uint32_t val;

    cur_vf = 0;
    if (pcie->pcie_x1) {
        set_pf = 0;
    } else {
        set_pf = pf_num;
    }

    val = readl(pcie->csr + AXI_FUNC_ADDR);
    cur_pf = PCIE_XPA_AXI_FUNC_READ_PF_GET(val);
    cur_pf = PCIE_XPA_AXI_FUNC_READ_VF_GET(val);
    pf_arr_push(cur_pf);
    vf_arr_push(cur_vf);

    val = readl(pcie->csr + AXI_FUNC_ADDR);
    val = PCIE_XPA_AXI_FUNC_READ_PF_SET(val, set_pf);
    val = PCIE_XPA_AXI_FUNC_WRITE_PF_SET(val, set_pf);

    // VF = 0 indicates that the physical function has initiated the request
    val = PCIE_XPA_AXI_FUNC_READ_VF_SET(val, 0x0);
    val = PCIE_XPA_AXI_FUNC_WRITE_VF_SET(val, 0x0);
    writel(val, pcie->csr + AXI_FUNC_ADDR);
}

void axiado_pcie_axi_clr(struct axiado_pcie *pcie) 
{
    uint8_t pre_pf;
    uint8_t pre_vf;
    uint32_t val;

    // If fifo pop fail
    if (!pf_arr_pop(&pre_pf)) {
        pre_pf = 0;
    }
    if (!vf_arr_pop(&pre_vf)) {
        pre_vf = 0;
    }

    val = readl(pcie->csr + AXI_FUNC_ADDR);
    val = PCIE_XPA_AXI_FUNC_READ_PF_SET(val, pre_pf);
    val = PCIE_XPA_AXI_FUNC_WRITE_PF_SET(val, pre_pf);
    val = PCIE_XPA_AXI_FUNC_READ_VF_SET(val, pre_vf);
    val = PCIE_XPA_AXI_FUNC_WRITE_VF_SET(val, pre_vf);
    writel(val, pcie->csr + AXI_FUNC_ADDR);
}

void axiado_pcie_axi_set_vf(struct axiado_pcie *pcie, uint8_t pf_num, uint8_t vf_num) 
{
    uint8_t set_pf;
    uint8_t set_vf;
    uint32_t cur_pf;
    uint32_t cur_vf;
    uint32_t val;

    if (pcie->pcie_x1) {
        set_pf = 0;
    } else {
        set_pf = pf_num;
    }

    if (vf_num == 0) {
        // SHOULD NOT REACH HERE
        // VF = 0 indicates that the physical function has initiated the request
        set_vf = 1;
    } else {
        set_vf = vf_num;
    }

    val = readl(pcie->csr + AXI_FUNC_ADDR);
    cur_pf = PCIE_XPA_AXI_FUNC_READ_PF_GET(val);
    cur_pf = PCIE_XPA_AXI_FUNC_READ_VF_GET(val);

    pf_arr_push(cur_pf);
    vf_arr_push(cur_vf);

    val = readl(pcie->csr + AXI_FUNC_ADDR);
    val = PCIE_XPA_AXI_FUNC_READ_PF_SET(val, set_pf);
    val = PCIE_XPA_AXI_FUNC_WRITE_PF_SET(val, set_pf);

    val = PCIE_XPA_AXI_FUNC_READ_VF_SET(val, set_vf);
    val = PCIE_XPA_AXI_FUNC_WRITE_VF_SET(val, set_vf);
    writel(val, pcie->csr + AXI_FUNC_ADDR);
}

uint8_t axiado_pcie_get_max_pf_num(struct axiado_pcie *pcie) 
{
    if (pcie->is_root_port) {
        return PCIE_PF_NUM_1;
    } else if (pcie->pcie_x1) {
        return PCIE_PF_NUM_1;
    } else {
        return PCIE_MAX_PF_NUM;
    }
}


static int axiado_pcie_cfg_read(int port, u8 bus, u8 dev, u8 func,
        u16 offset, u32 *val)
{
    struct axiado_pcie *pcie;
    void __iomem *address;
    int err;

    pcie = axiado_pcie_get_port(port);
    if (!pcie)
        return -EINVAL;

    err = axiado_pcie_map_bus(pcie, bus, dev, func, offset, &address);
    if (err < 0) {
        /* PCI convention for "no device": all ones (was: *val untouched) */
        *val = 0xffffffff;
        return 0;
    }

    *val = readl(address);

    return 0;
}

static int axiado_pcie_cfg_write(int port, u8 bus, u8 dev, u8 func,
        u16 offset, u32 val)
{
    struct axiado_pcie *pcie;
    void __iomem *address;
    int err;

    pcie = axiado_pcie_get_port(port);
    if (!pcie)
        return -EINVAL;

    err = axiado_pcie_map_bus(pcie, bus, dev, func, offset, &address);
    if (err < 0)
        return 0;   /* write to a non-existent device is dropped */

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
    size_t size = len;

    if (!buf || !len)
        return -EINVAL;

    pcie = axiado_pcie_get_port(port);
    if (!pcie)
        return -EINVAL;

    if ((offset + len) > (pcie->atr[0].phys_start + pcie->atr[0].size)) {
        printf("PCIe mem access out of window: off=0x%llx len=%zu\n",
                (u64)offset, len);
        return -EINVAL;
    }

    addr = (void __iomem *)(pcie->atr[0].phys_start + offset);

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

    if ((offset + len) > (pcie->atr[0].phys_start + pcie->atr[0].size)) {
        printf("PCIe mem access out of window: off=0x%llx len=%zu\n",
                (u64)offset, len);
        return -EINVAL;
    }

    addr = (void __iomem *)(pcie->atr[0].phys_start + offset);
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
    u64 tun;

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
    tun = ((u64)val2 << 32) | ((u64)val1);

    tun = PCIE_EQ_TUNING_63_0_PRESET_TUNING_SETTINGS_SET(tun, BEST_PRESET_REAPPLIED_CONT);
    tun = PCIE_EQ_TUNING_63_0_CONT_FINE_TUNE_EVEN_IF_NO_COEFF_SET(tun, 0x1);
    tun = PCIE_EQ_TUNING_63_0_GEN3_PRESET_SET(tun, PCIE_GEN3_PRESET_VECTOR);
    tun = PCIE_EQ_TUNING_63_0_GEN3_MAX_TUNING_ITER_SET(tun, PCIE_GEN3_MAX_TUNING_ITER);
    tun = PCIE_EQ_TUNING_63_0_GEN4_PRESET_SET(tun, PCIE_GEN4_PRESET_VECTOR);
    tun = PCIE_EQ_TUNING_63_0_GEN4_MAX_TUNING_ITER_SET(tun, PCIE_GEN4_MAX_TUNING_ITER);

    val1 = (u32)(tun & 0xFFFFFFFF);
    val2 = (u32)((tun >> 32) & 0xFFFFFFFF);
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
        /* Change class to Trust Services Device (PCIe End-Point) */
        axiado_pcie_axi_set_pf(pcie, PCIE_PF_NUM_0);
        writel(PCIE_TRUSTEDS_CLASS_CODE, pcie->bridge +
                REG_PCIE_PCI_IDS_63_32_ADRS_OFFSET);
        axiado_pcie_axi_clr(pcie);
        if (pcie->pcie_x2) {
            axiado_pcie_axi_set_pf(pcie, PCIE_PF_NUM_1);
            writel(PCIE_ETHERNET_CLASS_CODE, pcie->bridge +
                    REG_PCIE_PCI_IDS_63_32_ADRS_OFFSET);
            axiado_pcie_axi_clr(pcie);

            axiado_pcie_axi_set_pf(pcie, PCIE_PF_NUM_2);
            writel(PCIE_USB_CLASS_CODE, pcie->bridge +
                    REG_PCIE_PCI_IDS_63_32_ADRS_OFFSET);
            axiado_pcie_axi_clr(pcie);

            axiado_pcie_axi_set_pf(pcie, PCIE_PF_NUM_3);
            writel(PCIE_VGA_CLASS_CODE, pcie->bridge +
                    REG_PCIE_PCI_IDS_63_32_ADRS_OFFSET);
            axiado_pcie_axi_clr(pcie);

            axiado_pcie_axi_set_pf(pcie, PCIE_PF_NUM_4);
            writel(PCIE_STORAGE_CLASS_CODE, pcie->bridge +
                    REG_PCIE_PCI_IDS_63_32_ADRS_OFFSET);
            axiado_pcie_axi_clr(pcie);
        }
    }

    return 0;
}

static int axiado_pcie_config_vendor_dev_id(struct axiado_pcie *pcie)
{
    u32 val;

    axiado_pcie_axi_set_pf(pcie, PCIE_PF_NUM_0);
    val = readl(pcie->bridge + REG_PCIE_PCI_IDS_31_0_ADRS_OFFSET);
    writel((PCIE_PCI_IDS_31_0_VENDOR_ID_SET(val, 0x1e6b) |
                PCIE_PCI_IDS_31_0_DEVICE_ID_SET(val, pcie->lanes)), 
            pcie->bridge + REG_PCIE_PCI_IDS_31_0_ADRS_OFFSET);
    axiado_pcie_axi_clr(pcie);

    if (pcie->pcie_x2) {
        axiado_pcie_axi_set_pf(pcie, PCIE_PF_NUM_1);
        val = readl(pcie->bridge + REG_PCIE_PCI_IDS_31_0_ADRS_OFFSET);
        writel((PCIE_PCI_IDS_31_0_VENDOR_ID_SET(val, 0x1e6b) |
                    PCIE_PCI_IDS_31_0_DEVICE_ID_SET(val, pcie->lanes)), 
                pcie->bridge + REG_PCIE_PCI_IDS_31_0_ADRS_OFFSET);
        axiado_pcie_axi_clr(pcie);

        axiado_pcie_axi_set_pf(pcie, PCIE_PF_NUM_2);
        val = readl(pcie->bridge + REG_PCIE_PCI_IDS_31_0_ADRS_OFFSET);
        writel((PCIE_PCI_IDS_31_0_VENDOR_ID_SET(val, 0x1e6b) |
                    PCIE_PCI_IDS_31_0_DEVICE_ID_SET(val, pcie->lanes)), 
                pcie->bridge + REG_PCIE_PCI_IDS_31_0_ADRS_OFFSET);
        axiado_pcie_axi_clr(pcie);

        axiado_pcie_axi_set_pf(pcie, PCIE_PF_NUM_3);
        val = readl(pcie->bridge + REG_PCIE_PCI_IDS_31_0_ADRS_OFFSET);
        writel((PCIE_PCI_IDS_31_0_VENDOR_ID_SET(val, 0x1e6b) |
                    PCIE_PCI_IDS_31_0_DEVICE_ID_SET(val, pcie->lanes)), 
                pcie->bridge + REG_PCIE_PCI_IDS_31_0_ADRS_OFFSET);
        axiado_pcie_axi_clr(pcie);

        axiado_pcie_axi_set_pf(pcie, PCIE_PF_NUM_4);
        val = readl(pcie->bridge + REG_PCIE_PCI_IDS_31_0_ADRS_OFFSET);
        writel((PCIE_PCI_IDS_31_0_VENDOR_ID_SET(val, 0x1e6b) |
                    PCIE_PCI_IDS_31_0_DEVICE_ID_SET(val, pcie->lanes)), 
                pcie->bridge + REG_PCIE_PCI_IDS_31_0_ADRS_OFFSET);
        axiado_pcie_axi_clr(pcie);
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
        udelay(100);
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
    u32 val, ltssm_state = 0, cur_speed = 0, cur_width = 0;
    bool l0 = false;

    if (!port) {
        printf("Invalid Arguments passed for Link status check\n");
        return -EINVAL;
    }

    /* wait for L0 - accept whatever speed/width was negotiated */
    start = get_timer(0);
    while (get_timer(start) < timeout_ms) {
        val = readl(port->bridge + REG_PCIE_BASIC_STATUS_ADRS_OFFSET);
        cur_width = (val & 0xFF);
        cur_speed = (val >> 8) & 0x1F;

        val = readl(port->csr + LINK_STATUS_LOW_POWER_ADDR);
        ltssm_state = (val >> 8) & 0x1F;
        if (ltssm_state == S_L0 && cur_width) {
            l0 = true;
            break;
        }
        mdelay(1);
    }

    if (!l0) {
        printf("Link x%u DOWN (LTSSM=%s), ignoring\n", port->lanes,
                ltssm_state < LTSSM_MAX_NUM_STATE ?
                ltssm_state_char[ltssm_state] : "?");
        return -ENODEV;
    }

    /*
     * Links always come up at Gen1 first and are then retrained (Recovery)
     * to the target speed.  Give that some time, but do not fail if the
     * partner (e.g. a Gen3 NVMe SSD on a Gen4 port) settles lower.
     */
    start = get_timer(0);
    while (cur_speed < port->speed && get_timer(start) < 1000) {
        mdelay(10);
        val = readl(port->bridge + REG_PCIE_BASIC_STATUS_ADRS_OFFSET);
        cur_width = (val & 0xFF);
        cur_speed = (val >> 8) & 0x1F;
    }

    ltssm_state = get_ltssm_state(port);
    printf("Linkup %s - Gen%dx%d (requested Gen%d x%d)\n",
            ltssm_state_char[ltssm_state], cur_speed, cur_width,
            port->speed, port->lanes);
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
    writel((lower_32_bits(src_addr) & PCIE_ATR_SRC_ADDR_MASK) |
            ((ilog2(window_size) - 1) << PCIE_ATR_SRC_WIN_SIZE_SHIFT) | 
            1,
            pcie->bridge + (offset + PCIE_ATR_SRC_ADDR_LOW));

    writel(upper_32_bits(src_addr),
            pcie->bridge + (offset + PCIE_ATR_SRC_ADDR_HIGH));

    writel((lower_32_bits(trsl_addr) & PCIE_ATR_TRSL_ADDR_MASK),
            pcie->bridge + (offset + PCIE_ATR_TRSL_ADDR_LOW));

    writel(upper_32_bits(trsl_addr),
            pcie->bridge + (offset + PCIE_ATR_TRSL_ADDR_HIGH));

    writel(trsl_param, pcie->bridge + (offset+PCIE_ATR_TRSL_PARAM));

    /* trsl_mask = ~((u64)window_size - 1); */
    /* writel((uint32_t)(trsl_mask & 0xFFFFFFFF), pcie->bridge + (offset + PCIE_ATR_TRSL_MASK_OFFSET)); */
    /* writel((uint32_t)(((uint64_t)trsl_mask >> 32) & 0xFFFFFFFF), */ 
    /*         pcie->bridge + (offset + PCIE_ATR_TRSL_MASK_OFFSET + sizeof(uint32_t))); */

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
    writel((lower_32_bits(src_addr) & PCIE_ATR_SRC_ADDR_MASK) |
            ((ilog2(window_size) - 1) << PCIE_ATR_SRC_WIN_SIZE_SHIFT) | 
            1,
            pcie->bridge + (offset + PCIE_ATR_SRC_ADDR_LOW));

    writel(upper_32_bits(src_addr),
            pcie->bridge + (offset + PCIE_ATR_SRC_ADDR_HIGH));

    writel((lower_32_bits(trsl_addr) & PCIE_ATR_TRSL_ADDR_MASK),
            pcie->bridge + (offset + PCIE_ATR_TRSL_ADDR_LOW));

    writel(upper_32_bits(trsl_addr),
            pcie->bridge + (offset + PCIE_ATR_TRSL_ADDR_HIGH));

    writel(trsl_param, pcie->bridge + (offset+PCIE_ATR_TRSL_PARAM));

    /* trsl_mask = ~((u64)window_size - 1); */
    /* writel((uint32_t)(trsl_mask & 0xFFFFFFFF), pcie->bridge + (offset + PCIE_ATR_TRSL_MASK_OFFSET)); */
    /* writel((uint32_t)(((uint64_t)trsl_mask >> 32) & 0xFFFFFFFF), */ 
    /*         pcie->bridge + (offset + PCIE_ATR_TRSL_MASK_OFFSET + sizeof(uint32_t))); */

    printf("P2A entry [table: %d]: 0x%010llx %s 0x%010llx [0x%010llx] (param: 0x%06x)\n",
            table_num, src_addr, (trsl_param & PCIE_ATR_TRSL_DIR) ? "<-" : "->",
            trsl_addr, (u32)window_size, trsl_param);

    return 0;
}

static void axiado_pcie_setup_windows(struct axiado_pcie *pcie)
{
    int table;
    int bar_lo, bar_hi;
    int ret;

    if (pcie->is_root_port) {
        for (table = 0; table < ARRAY_SIZE(pcie->atr); table++) {
            axiado_pcie_setup_a2p_atr(pcie, table,
                    (u64)pcie->atr[table].bus_start,
                    (u64)pcie->atr[table].bus_start,
                    pcie->atr[table].size,
                    PCIE_TX_TX_INF);
        }
    }

    if (!pcie->is_root_port) {
        /*
         * BAR registers in bridge space encode both size mask
         * and type.  Size mask = ~(size - 1) & 0xFFFFFFF0,
         * type goes in bits [3:0].  BAR1/BAR3 are upper 32
         * bits for 64-bit BARs.
         *
         * Must be configured while IP is in reset.
         */
        if (pcie->bar[0].size) {
            /* BAR0: size + type */
            bar_lo = (~(pcie->bar[0].size - 1) & 0xFFFFFFF0) |
                pcie->bar[0].flags;

            /* BAR1: size + type */
            bar_hi = 0xFFFFFFFF;

            axiado_pcie_bar_init(pcie, BAR_01, (u64)pcie->bar[0].phys_start, pcie->bar[0].size);
            axiado_pcie_setup_p2a_atr(pcie, 0, 
                    (u64)0x0,
                    (u64)pcie->bar[0].phys_start & 0xFFFFFFFF,
                    (u64)pcie->bar[0].size,
                    AXI4_MSTR_NUM_0);

        } else {
            bar_lo = 0x0;
            bar_hi = 0x0;
        }
        writel(bar_lo, pcie->bridge + PCIE_BAR_01_OFFSET);
        writel(bar_hi, pcie->bridge + PCIE_BAR_01_OFFSET + 4);

        if (pcie->bar[1].size) {
            /* BAR2: size + type */
            bar_lo = (~(pcie->bar[1].size - 1) & 0xFFFFFFF0) |
                pcie->bar[1].flags;

            /* BAR3: size + type */
            bar_hi = 0xFFFFFFFF;

            axiado_pcie_bar_init(pcie, BAR_23, (u64)pcie->bar[1].phys_start, pcie->bar[1].size);

            axiado_pcie_setup_p2a_atr(pcie, 2, 
                    (u64)0x0,
                    (u64)pcie->bar[1].phys_start & 0xFFFFFFFF,
                    (u64)pcie->bar[1].size,
                    AXI4_MSTR_NUM_0);
        } else {
            bar_lo = 0x0;
            bar_hi = 0x0;
        }
        writel(bar_lo, pcie->bridge + PCIE_BAR_23_OFFSET);
        writel(bar_hi, pcie->bridge + PCIE_BAR_23_OFFSET + 4);

        if (pcie->bar[2].size) {
            /* BAR2: size + type */
            bar_lo = (~(pcie->bar[2].size - 1) & 0xFFFFFFF0) |
                pcie->bar[2].flags;

            /* BAR3: size + type */
            bar_hi = 0xFFFFFFFF;

            axiado_pcie_bar_init(pcie, BAR_45, (u64)pcie->bar[2].phys_start, pcie->bar[2].size);

            axiado_pcie_setup_p2a_atr(pcie, 4, 
                    (u64)0x0,
                    (u64)pcie->bar[2].phys_start & 0xFFFFFFFF,
                    (u64)pcie->bar[2].size,
                    PCIE_TX_TX_INF);
        } else {
            bar_lo = 0x0;
            bar_hi = 0x0;
        }
        writel(bar_lo, pcie->bridge + PCIE_BAR_45_OFFSET);
        writel(bar_hi, pcie->bridge + PCIE_BAR_45_OFFSET + 4);
    }
}

/////////////////////////////////////////////
/////////////////// MSI-X ///////////////////
/////////////////////////////////////////////
// 1. MSI-X supports up to 2048 vectors per function (PF and VF)
// 2. Each MSI-X vector gets its own independent 64 bit target
//    memory addres and its own 32 bit data value. No more share address
// 3. MSI-X mandates support for per vector masking. each of those
//    up to 2048 vectors can be individually masked or unmasked
//    by the OS. This gives the OS the fine grained controle over
//    the interrupt delivery and adding the flexibility
//
// MSI-X work flow
//  1. Interrupt event happend
//  2. Device identifies the associated MSI-X vector number
//  3. Device check the per vector mask bit in the MSI-X table entry
//     for that vector, and it checks the global function mask bit
//     in the capability structure
//  4.
//      a. If both are unmasked, clear, usually 0, the device reads
//         the message address and message data from the vector's table entry
//      b. It then generates and send a memory write TLP with that specific
//         address and data onto the PCI Bus
//  5.
//      a. If either mask bit is set, masked value usually 1, the device is
//         not send the TLP
//      b. Instead, it just set the corresponding bit in the pending bit array (PBA)
//  6. If a TLP was sent, the root complex chipset see this memory write to a
//     special addres range, recognizes it as and MSI-X interrupt, and routes it
//     to the correct CPU core based on the message address programmed by the OS
//
// MSI-X discorer and initialization.
//  - The OS should set the whole MSI-X table addresses data, initial masks during
//    device initialization
//  - The OS discovers the MSI-X capability, allocates memory for the table and PBA
//    mapping the BIR, then programs each enabled table entry
//  - Drivers register ISR for a specific vectos the OS assign them
//  - Device provide the hardware structures in its memory. OS populates them to
//    control interrupt delivery . This granularity per vector address data mask is perfect
//    for complex

int axiado_pcie_msix_ep_init_config(struct axiado_pcie *pcie) {
    uint8_t max_supported_pf_num;
    uint8_t pf_num;
    uint32_t val;

    if (pcie->pcie_x1) {
        return -ENOSYS;
    }

    val = readl(pcie->ext + REG_PCIE_X2_MSIX_PIN_INTR_MASK_ADRS_OFFSET);
    val = BIT_SET_VAL(31, 0, val, 0x0);  // Unmask all interrupt pin
    writel(val, pcie->ext + REG_PCIE_X2_MSIX_PIN_INTR_MASK_ADRS_OFFSET);

    // Kirkwood System Address where MSI-X TLPs are sent. PCIe
    // AXI-to-PCIe tables are setup send the TLP as MSIX transactions.
    val = readl(pcie->ext + REG_PCIE_X2_MSIX_KW_ADRS_ADRS_OFFSET);
    val = BIT_SET_VAL(27, 0, val, (AX3000_MEM_BASE_ADRS_PCIE_X2_1 >> 12));
    writel(val, pcie->ext + REG_PCIE_X2_MSIX_KW_ADRS_ADRS_OFFSET);

    val = readl(pcie->ext + REG_PCIE_X2_WR_MON_CTRL_STS_ADRS_OFFSET);
    val = EXT_WR_MON_CTRL_EN_SET(val, 0x0);
    val = EXT_WR_MON_CTRL_MASK_SET(val, 0xFF);
    writel(val, pcie->ext + REG_PCIE_X2_WR_MON_CTRL_STS_ADRS_OFFSET);

    max_supported_pf_num = axiado_pcie_get_max_pf_num(pcie);
    // XPressRICH 5.0, 12.6.3
    // MSI and MSI-X should never be enabled simultaneously; this is a configuration error.
    for (pf_num = PCIE_PF_NUM_0; pf_num < max_supported_pf_num; pf_num++) {
        axiado_pcie_axi_set_pf(pcie, pf_num);

        // the MSI-X Table Size N, which is encoded as N-1.
        // For example, a register returned value of 0011b indicates a table size of 4.
        val = readl(pcie->ext + REG_PCIE_X2_PCIE_PCI_IRQ_PASID_MSIX_15_0_ADRS_OFFSET);
        val = INT_PCIE_PCI_IRQ_INTR_PIN_SET(val, 0x0);  // Interrupt Pin = None
        val = INT_PCIE_PCI_IRQ_NUM_MSI_MSG_SET(val, PCIE_MSI_SINGLE_PF_MSI_VEC_ALLOC);
        val = INT_PCIE_PCI_IRQ_DISABLE_MSI_SET(val, 0x1);             // Disable MSI
        val = INT_PCIE_PCI_MSIX_15_0_IMPL_MSIX_CAP_PF_SET(val, 0x1);  // Enable MSI-X
        val = INT_PCIE_PCI_MSIX_15_0_TABLE_SIZE_SET(val, (PCIE_MSIX_PER_FUNCTION_DEFAULT_TABLE_SIZE - 1));
        writel(val, pcie->ext + REG_PCIE_X2_PCIE_PCI_IRQ_PASID_MSIX_15_0_ADRS_OFFSET);

        val = readl(pcie->ext + REG_PCIE_X2_PCIE_PCI_MSIX_47_16_ADRS_OFFSET);
        val = INT_PCIE_PCI_MSIX_47_16_TABLE_BIR_SET(val, BAR_45);
        val = INT_PCIE_PCI_MSIX_47_16_TABLE_OFFSET_SET(val, PCIE_MSIX_MBX_X2_PER_PF_TABLE_OFFSET);
        writel(val, pcie->ext + REG_PCIE_X2_PCIE_PCI_MSIX_47_16_ADRS_OFFSET);

        val = readl(pcie->ext + REG_PCIE_X2_PCIE_PCI_MSIX_79_48_ADRS_OFFSET);
        val = INT_PCIE_PCI_MSIX_79_48_PBA_BIR_SET(val, BAR_45);
        val = INT_PCIE_PCI_MSIX_79_48_PBA_OFFSET_SET(val, PCIE_MSIX_MBX_X2_PER_PF_PBA_OFFSET);
        writel(val, pcie->ext + REG_PCIE_X2_PCIE_PCI_MSIX_79_48_ADRS_OFFSET);

        axiado_pcie_axi_clr(pcie);
    }

    return 0;
}

/* release=0: assert PERST#, release=1: de-assert (matches the 0x5 written in init) */
static void axiado_pcie_set_perst(struct axiado_pcie *pcie, u32 release)
{
    void __iomem *reg = pcie->ext + (pcie->pcie_x1 ?
            REG_PCIE_X1_RESET_CTRL_ADRS_OFFSET :
            REG_PCIE_X2_RESET_CTRL_ADRS_OFFSET);
    u32 val = readl(reg);

    val = REG_PCIE_RESET_CTRL_PCIE_RP_PERST_SET(val, release);
    writel(val, reg);
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
        /* Bus master + memory space enable in EP config space */
        udelay(10);
        axiado_pcie_cfg_read(port, 0, 0, 0, PCI_CFG_COMMAND, &val);
        val = (val & 0xffff) | PCI_COMMAND_MASTER;
        axiado_pcie_cfg_write(port, 0, 0, 0, PCI_CFG_COMMAND, val);

        axiado_pcie_setup_windows(pcie);

        axiado_pcie_config_class_code(pcie);
        axiado_pcie_config_vendor_dev_id(pcie);

        axiado_pcie_msix_ep_init_config(pcie);
    }

    /* release PIPE_RST_N for PCIe */
    if (pcie->pcie_x1)
        writel(0x5, pcie->ext + REG_PCIE_X1_RESET_CTRL_ADRS_OFFSET);
    else if (pcie->pcie_x2)
        writel(0x5, pcie->ext + REG_PCIE_X2_RESET_CTRL_ADRS_OFFSET);

    mdelay(100);

    axiado_pcie_enable_ltssm(pcie);

    if (pcie->is_root_port){
        u32 vendor_id, class_code;
        u32 dlf_cap;

        /*
         * PERST# must be held asserted for >= 100us (we use 100 ms) and only
         * then released.  
         */
        axiado_pcie_set_perst(pcie, 0);
        mdelay(PCIE_PERST_ASSERT_MS);
        axiado_pcie_set_perst(pcie, 1);

        ret = axiado_pcie_port_check_link(pcie,
                PCIE_LINKUP_TIMEOUT);
        if (ret < 0)
            return ret;

        /* PCIe spec: wait 100 ms after link-up before the first config request */
        mdelay(PCIE_POST_LINK_DELAY_MS);

        axiado_pcie_setup_windows(pcie);

        /* Bus master enable */
        axiado_pcie_cfg_read(port, 0, 0, 0, PCI_CFG_COMMAND, &val);
        val = (val & 0xffff) | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;
        axiado_pcie_cfg_write(port, 0, 0, 0, PCI_CFG_COMMAND, val);

        /* Enumeration bus number to access Endpoint on the other side of the link */
        axiado_pcie_cfg_read(port, 0, 0, 0, PCIE_TYPE1_TIMER_SUB_SEC_PRI_OFF, &val);
        val = PCIE_TYPE1_PRI_BUS_SET(val, 0x0);   // Primary bus num
        val = PCIE_TYPE1_SEC_BUS_SET(val, 0x1);   // Secondary bus num
        val = PCIE_TYPE1_SUB_BUS_SET(val, 0xFF);  // Subordinary bus num
        axiado_pcie_cfg_write(port, 0, 0, 0, PCIE_TYPE1_TIMER_SUB_SEC_PRI_OFF, val);

        /* SSDs answer CRS / all-ones for a while: poll until they respond */
        ret = ax_pcie_wait_device_ready(port, 0x1, 0x0, 0x0,
                PCIE_EP_READY_TIMEOUT_MS);
        if (ret) {
            printf("Device behind port x%d does not answer config reads (%d)\n",
                    pcie->lanes, ret);
            return -ENODEV;
        }

        axiado_pcie_cfg_read(port, 0x1, 0x0, 0x0, PCI_CFG_CLASS_CODE, &class_code);
        axiado_pcie_cfg_read(port, 0x1, 0x0, 0x0, PCI_CFG_VENDOR_ID, &vendor_id);
        printf("EP Vendor/Device Id: 0x%08x - Class/Rev: 0x%08x\n\r", vendor_id, class_code);
        axiado_pcie_cfg_read(port, 0x1, 0x0, 0x0, PCI_CFG_COMMAND, &val);
        val = (val & 0xffff) | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;
        axiado_pcie_cfg_write(port, 0x1, 0x0, 0x0, PCI_CFG_COMMAND, val);
    }

    return 0;
}

/*
 * Raw dump of the bridge ATR tables, local error status and RP type-1
 * windows.  ATR table = 8 dwords: SRC_L SRC_H TRSL_L TRSL_H PARAM ? MASK_L MASK_H
 * Compare with the same registers read in Linux (devmem).
 */
static void axiado_pcie_dump_atr(struct axiado_pcie *pcie)
{
    static const struct { const char *name; u32 base; } blk[] = {
        { "A2P (CPU->PCIe)", PCIE_ATR_AXI4_SLV0 },
        { "P2A (PCIe->AXI)", PCIE_ATR_PCIE_WIN0 },
    };
    u32 st;
    int b, t, i;

    for (b = 0; b < ARRAY_SIZE(blk); b++) {
        for (t = 0; t < 4; t++) {
            u32 off = blk[b].base + t * PCIE_ATR_TABLE_OFFSET;

            printf("  %s[%d] @%8p:", blk[b].name, t, pcie->bridge + off);
            for (i = 0; i < 8; i++)
                printf(" %08x", readl(pcie->bridge + off + i * 4));
            printf("\n");
        }
    }

    /* bridge local status: bits 16-18 AXI-side errors, 20-22 PCIe-side errors */
    st = readl(pcie->bridge + ISTATUS_LOCAL);
    printf("  ISTATUS_LOCAL=%08x%s%s%s%s%s%s\n", st,
            (st & INT_AXI_POST_ERROR)     ? " AXI_POST_ERR"     : "",
            (st & INT_AXI_FETCH_ERROR)    ? " AXI_FETCH_ERR"    : "",
            (st & INT_AXI_DISCARD_ERROR)  ? " AXI_DISCARD_ERR"  : "",
            (st & INT_PCIE_POST_ERROR)    ? " PCIE_POST_ERR"    : "",
            (st & INT_PCIE_FETCH_ERROR)   ? " PCIE_FETCH_ERR"   : "",
            (st & INT_PCIE_DISCARD_ERROR) ? " PCIE_DISCARD_ERR" : "");

    if (pcie->is_root_port)
        printf("  RP cfg 0x04=%08x 0x18=%08x 0x20=%08x 0x24=%08x 0x28=%08x 0x2c=%08x\n",
                readl(pcie->cfg + 0x04), readl(pcie->cfg + 0x18),
                readl(pcie->cfg + 0x20), readl(pcie->cfg + 0x24),
                readl(pcie->cfg + 0x28), readl(pcie->cfg + 0x2c));
}

static int axiado_pcie_debug_print(u8 port)
{
    struct axiado_pcie *pcie;
    u32 val;
    u32 vendor_id, class_code;
    int ret;

    pcie = axiado_pcie_get_port(port);
    if (!pcie)
        return -EINVAL;

    axiado_pcie_dump_atr(pcie);

    if (!is_port_pll_locked(pcie)) {
        printf("PLL is not locked for port x%d, ignoring\n",
                pcie->lanes);
        return -ENOLINK;
    }
    printf("PLL Locked for port x%d\n", pcie->lanes);

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

    return 0;
}

int axiado_pcie_get_regions(u8 port,
        struct ax_pci_region **regions,
        int *region_cnt)
{
    struct axiado_pcie *pcie;

    if (!regions || !region_cnt)
        return -EINVAL;

    pcie = axiado_pcie_get_port(port);
    if (!pcie)
        return -EINVAL;

    *regions = pcie->atr;
    *region_cnt = ARRAY_SIZE(pcie->atr);

    return 0;
}

struct ax_pcie_ops pcie = {
    .init = axiado_pcie_init,
    .get_regions = axiado_pcie_get_regions,
    .cfg_read = axiado_pcie_cfg_read,
    .cfg_write = axiado_pcie_cfg_write,
    .mem_read = axiado_pcie_mem_read,
    .mem_write = axiado_pcie_mem_write,
    .debug_print = axiado_pcie_debug_print,
};
