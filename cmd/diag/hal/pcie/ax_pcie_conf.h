/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy namhuyngn03@gmail.com
 */

#ifndef __PCIE_CONF_H
#define __PCIE_C_H

#include <linux/types.h>
#include <linux/bitops.h>

/*
 * PCI Configuration Space
 */
#define PCI_CFG_VENDOR_ID              0x00
#define PCI_CFG_DEVICE_ID              0x02
#define PCI_CFG_COMMAND                0x04
#define PCI_CFG_STATUS                 0x06
#define PCI_CFG_REVISION_ID            0x08
#define PCI_CFG_PROG_IF                0x09
#define PCI_CFG_SUBCLASS               0x0a
#define PCI_CFG_CLASS_CODE             0x0b
#define PCI_CFG_CACHE_LINE_SIZE        0x0c
#define PCI_CFG_LATENCY_TIMER          0x0d
#define PCI_CFG_HEADER_TYPE            0x0e
#define PCI_CFG_BIST                   0x0f

#define PCI_CFG_BAR0                   0x10
#define PCI_CFG_BAR1                   0x14
#define PCI_CFG_BAR2                   0x18
#define PCI_CFG_BAR3                   0x1c
#define PCI_CFG_BAR4                   0x20
#define PCI_CFG_BAR5                   0x24

#define PCI_CFG_CARDBUS_CIS            0x28
#define PCI_CFG_SUBSYS_VENDOR_ID       0x2c
#define PCI_CFG_SUBSYS_ID               0x2e
#define PCI_CFG_EXP_ROM                0x30
#define PCI_CFG_CAP_PTR                0x34
#define PCI_CFG_INT_LINE               0x3c
#define PCI_CFG_INT_PIN                0x3d
#define PCI_CFG_MIN_GNT                0x3e
#define PCI_CFG_MAX_LAT                0x3f

/*
 * Header Type
 */
#define PCI_HEADER_TYPE_MASK           0x7f
#define PCI_HEADER_TYPE_NORMAL         0x00
#define PCI_HEADER_TYPE_BRIDGE         0x01
#define PCI_HEADER_TYPE_CARDBUS        0x02
#define PCI_HEADER_TYPE_MULTI_FUNC     BIT(7)

/*
 * Command Register
 */
#define PCI_COMMAND_IO                 BIT(0)
#define PCI_COMMAND_MEMORY             BIT(1)
#define PCI_COMMAND_MASTER             BIT(2)
#define PCI_COMMAND_SPECIAL            BIT(3)
#define PCI_COMMAND_INVALIDATE         BIT(4)
#define PCI_COMMAND_VGA_PALETTE        BIT(5)
#define PCI_COMMAND_PARITY             BIT(6)
#define PCI_COMMAND_WAIT               BIT(7)
#define PCI_COMMAND_SERR               BIT(8)
#define PCI_COMMAND_FAST_BACK          BIT(9)
#define PCI_COMMAND_INTX_DISABLE       BIT(10)

/*
 * Status Register
 */
#define PCI_STATUS_CAP_LIST            BIT(4)
#define PCI_STATUS_66MHZ               BIT(5)
#define PCI_STATUS_FAST_BACK            BIT(7)
#define PCI_STATUS_PARITY              BIT(8)
#define PCI_STATUS_DEVSEL_MASK         (BIT(9) | BIT(10))
#define PCI_STATUS_SIG_TARGET_ABORT    BIT(11)
#define PCI_STATUS_REC_TARGET_ABORT    BIT(12)
#define PCI_STATUS_REC_MASTER_ABORT    BIT(13)
#define PCI_STATUS_SIG_SYSTEM_ERROR    BIT(14)
#define PCI_STATUS_DETECTED_PARITY     BIT(15)

/*
 * BAR
 */
#define PCI_BAR_IO                     BIT(0)
#define PCI_BAR_MEM_TYPE_MASK          (BIT(1) | BIT(2))
#define PCI_BAR_MEM_TYPE_32            0x0
#define PCI_BAR_MEM_TYPE_64            BIT(2)
#define PCI_BAR_PREFETCH               BIT(3)
#define PCI_BAR_ADDR_MASK              0xfffffff0
#define PCI_BAR_IO_ADDR_MASK           0xfffffffc

/*
 * Standard Capability header
 *
 * +0x00 Capability ID
 * +0x01 Next Capability Pointer
 */
#define PCI_CAP_ID                     0x00
#define PCI_CAP_NEXT                   0x01

#define PCI_CAP_ID_PM                  0x01
#define PCI_CAP_ID_MSI                 0x05
#define PCI_CAP_ID_PCIE                0x10
#define PCI_CAP_ID_MSIX                0x11

/*
 * PCI Power Management Capability
 */
#define PCI_PM_CAP                     0x02
#define PCI_PM_CTRL                    0x04
#define PCI_PM_STATUS                  0x06

#define PCI_PM_CAP_VERSION_MASK        0x0007
#define PCI_PM_CAP_PME_CLOCK           BIT(3)
#define PCI_PM_CAP_DSI                 BIT(5)
#define PCI_PM_CAP_D1                 BIT(9)
#define PCI_PM_CAP_D2                 BIT(10)
#define PCI_PM_CAP_PME_SUPPORT_MASK    0xf800

#define PCI_PM_CTRL_STATE_MASK         0x0003
#define PCI_PM_CTRL_NO_SOFT_RESET      BIT(3)
#define PCI_PM_CTRL_PME_ENABLE        BIT(8)
#define PCI_PM_CTRL_DATA_SEL_MASK      0x0f00
#define PCI_PM_CTRL_DATA_SCALE_MASK    0x1800
#define PCI_PM_CTRL_PME_STATUS         BIT(15)

/*
 * MSI Capability
 */
#define PCI_MSI_CTRL                   0x02
#define PCI_MSI_ADDR_LO                0x04
#define PCI_MSI_ADDR_HI                0x08
#define PCI_MSI_DATA_32                0x08
#define PCI_MSI_DATA_64                0x0c

#define PCI_MSI_CTRL_ENABLE            BIT(0)
#define PCI_MSI_CTRL_MMC_MASK          0x000e
#define PCI_MSI_CTRL_MME_MASK          0x0070
#define PCI_MSI_CTRL_64BIT             BIT(7)
#define PCI_MSI_CTRL_PER_VECTOR_MASK   BIT(8)

/*
 * MSI-X Capability
 */
#define PCI_MSIX_CTRL                  0x02
#define PCI_MSIX_TABLE                 0x04
#define PCI_MSIX_PBA                   0x08

#define PCI_MSIX_CTRL_TABLE_SIZE_MASK  0x07ff
#define PCI_MSIX_CTRL_FUNCTION_MASK    BIT(14)
#define PCI_MSIX_CTRL_ENABLE           BIT(15)

#define PCI_MSIX_BIR_MASK              0x7
#define PCI_MSIX_OFFSET_MASK           0xfffffff8

/*
 * PCI Express Capability
 *
 * Capability base + offset
 */
#define PCI_EXP_CAP                    0x02
#define PCI_EXP_DEVCAP                 0x04
#define PCI_EXP_DEVCTL                 0x08
#define PCI_EXP_DEVSTA                 0x0a
#define PCI_EXP_LNKCAP                 0x0c
#define PCI_EXP_LNKCTL                 0x10
#define PCI_EXP_LNKSTA                 0x12
#define PCI_EXP_SLT_CAP                0x14
#define PCI_EXP_SLT_CTL                0x18
#define PCI_EXP_SLT_STA                0x1a
#define PCI_EXP_RTCTL                  0x1c
#define PCI_EXP_RTSTA                  0x20
#define PCI_EXP_DEVCAP2                0x24
#define PCI_EXP_DEVCTL2                0x28
#define PCI_EXP_LNKCAP2                0x2c
#define PCI_EXP_LNKCTL2                0x30
#define PCI_EXP_LNKSTA2                0x32

/*
 * PCI Express Capability register
 */
#define PCI_EXP_CAP_VERSION_MASK       0x000f
#define PCI_EXP_CAP_TYPE_MASK          0x00f0
#define PCI_EXP_CAP_TYPE_SHIFT         4

#define PCI_EXP_TYPE_ENDPOINT          0x0
#define PCI_EXP_TYPE_LEGACY_ENDPOINT   0x1
#define PCI_EXP_TYPE_ROOT_PORT         0x4
#define PCI_EXP_TYPE_UPSTREAM_PORT      0x5
#define PCI_EXP_TYPE_DOWNSTREAM_PORT    0x6
#define PCI_EXP_TYPE_PCIE_BRIDGE        0x7
#define PCI_EXP_TYPE_RC_ENDPOINT        0x9
#define PCI_EXP_TYPE_RC_EVENT_COLLECTOR 0xa

/*
 * Device Capability
 */
#define PCI_EXP_DEVCAP_MPS_MASK        0x00000007

#define PCI_EXP_DEVCAP_PHANTOM_MASK    0x00000018
#define PCI_EXP_DEVCAP_PHANTOM_SHIFT   3

#define PCI_EXP_DEVCAP_EXT_TAG         BIT(5)

#define PCI_EXP_DEVCAP_L0S_MASK        0x000001c0
#define PCI_EXP_DEVCAP_L0S_SHIFT       6

#define PCI_EXP_DEVCAP_L1_MASK         0x00000e00
#define PCI_EXP_DEVCAP_L1_SHIFT        9

#define PCI_EXP_DEVCAP_ATN_BUT         BIT(10)
#define PCI_EXP_DEVCAP_ATN_IND         BIT(11)
#define PCI_EXP_DEVCAP_PWR_IND         BIT(12)

/*
 * Device Control
 */
#define PCI_EXP_DEVCTL_CERE            BIT(0)
#define PCI_EXP_DEVCTL_NFERE           BIT(1)
#define PCI_EXP_DEVCTL_FERE            BIT(2)
#define PCI_EXP_DEVCTL_URRE            BIT(3)
#define PCI_EXP_DEVCTL_RELAX           BIT(4)

#define PCI_EXP_DEVCTL_MPS_MASK        0x00e0
#define PCI_EXP_DEVCTL_MPS_SHIFT       5

#define PCI_EXP_DEVCTL_EXT_TAG         BIT(8)

#define PCI_EXP_DEVCTL_PHANTOM_MASK    0x0e00
#define PCI_EXP_DEVCTL_PHANTOM_SHIFT   9

#define PCI_EXP_DEVCTL_AUX_PME         BIT(11)
#define PCI_EXP_DEVCTL_NOSNOOP         BIT(11)

#define PCI_EXP_DEVCTL_READRQ_MASK     0x7000
#define PCI_EXP_DEVCTL_READRQ_SHIFT    12

#define PCI_EXP_DEVCTL_BCR_FLR         BIT(15)

/*
 * Device Status
 */
#define PCI_EXP_DEVSTA_CED             BIT(0)
#define PCI_EXP_DEVSTA_NFED            BIT(1)
#define PCI_EXP_DEVSTA_FED             BIT(2)
#define PCI_EXP_DEVSTA_URD             BIT(3)
#define PCI_EXP_DEVSTA_AUXPD           BIT(4)
#define PCI_EXP_DEVSTA_TRPND           BIT(5)

/*
 * Link Capability
 */
#define PCI_EXP_LNKCAP_SPEED_MASK      0x0000000f

#define PCI_EXP_LNKCAP_WIDTH_MASK      0x000003f0
#define PCI_EXP_LNKCAP_WIDTH_SHIFT     4

#define PCI_EXP_LNKCAP_ASPM_MASK       0x00000c00

#define PCI_EXP_LNKCAP_L0S_MASK        0x00007000
#define PCI_EXP_LNKCAP_L0S_SHIFT       12

#define PCI_EXP_LNKCAP_L1_MASK         0x00038000
#define PCI_EXP_LNKCAP_L1_SHIFT        15

#define PCI_EXP_LNKCAP_CLOCK_PM        BIT(18)
#define PCI_EXP_LNKCAP_SURPRISE_DOWN    BIT(19)
#define PCI_EXP_LNKCAP_DLL_ACTIVE      BIT(20)

/*
 * Link Control
 */
#define PCI_EXP_LNKCTL_ASPM_MASK       0x0003
#define PCI_EXP_LNKCTL_RCB             BIT(3)
#define PCI_EXP_LNKCTL_LINK_DISABLE    BIT(4)
#define PCI_EXP_LNKCTL_RETRAIN         BIT(5)
#define PCI_EXP_LNKCTL_COMMON_CLK      BIT(6)
#define PCI_EXP_LNKCTL_EXT_SYNC        BIT(7)
#define PCI_EXP_LNKCTL_CLK_PM          BIT(8)
#define PCI_EXP_LNKCTL_HAWD            BIT(9)
#define PCI_EXP_LNKCTL_BWM_INT         BIT(10)
#define PCI_EXP_LNKCTL_AUT_BW_INT      BIT(11)

/*
 * Link Status
 */
#define PCI_EXP_LNKSTA_SPEED_MASK      0x000f

#define PCI_EXP_LNKSTA_WIDTH_MASK      0x03f0
#define PCI_EXP_LNKSTA_WIDTH_SHIFT     4

#define PCI_EXP_LNKSTA_TRAINING        BIT(11)
#define PCI_EXP_LNKSTA_CLOCK_PM        BIT(12)
#define PCI_EXP_LNKSTA_DLL_ACTIVE      BIT(13)
#define PCI_EXP_LNKSTA_BWM_STATUS      BIT(14)
#define PCI_EXP_LNKSTA_AUT_BW_STATUS   BIT(15)

/*
 * Slot Capability
 */
#define PCI_EXP_SLT_CAP_NUM_MASK       0x0000001f
#define PCI_EXP_SLT_CAP_ATT_BUTTON     BIT(6)
#define PCI_EXP_SLT_CAP_PWR_CTRL       BIT(7)
#define PCI_EXP_SLT_CAP_MRL_SENSOR     BIT(8)
#define PCI_EXP_SLT_CAP_PRES_DETECT    BIT(9)
#define PCI_EXP_SLT_CAP_PWR_LIMIT      0x00007c00
#define PCI_EXP_SLT_CAP_PWR_LIMIT_SHIFT 10
#define PCI_EXP_SLT_CAP_NO_CMD_CPL     BIT(18)
#define PCI_EXP_SLT_CAP_PHYSICAL       BIT(19)
#define PCI_EXP_SLT_CAP_ATT_IND        0x000c0000
#define PCI_EXP_SLT_CAP_PWR_IND        0x00300000

/*
 * Slot Control
 */
#define PCI_EXP_SLT_CTL_ABP_ENABLE     BIT(0)
#define PCI_EXP_SLT_CTL_PFD_ENABLE     BIT(1)
#define PCI_EXP_SLT_CTL_MRLCE_ENABLE   BIT(2)
#define PCI_EXP_SLT_CTL_PDC_ENABLE     BIT(3)
#define PCI_EXP_SLT_CTL_CCIE_ENABLE    BIT(4)
#define PCI_EXP_SLT_CTL_HPIE_ENABLE    BIT(5)
#define PCI_EXP_SLT_CTL_AIC            0x00c0
#define PCI_EXP_SLT_CTL_PWR_LIMIT      0x1f00
#define PCI_EXP_SLT_CTL_PWR_LIMIT_SHIFT 7
#define PCI_EXP_SLT_CTL_ATT_IND        0x0300
#define PCI_EXP_SLT_CTL_PWR_IND        0x0c00

/*
 * Slot Status
 */
#define PCI_EXP_SLT_STA_ABP            BIT(0)
#define PCI_EXP_SLT_STA_PFD            BIT(1)
#define PCI_EXP_SLT_STA_MRLSC          BIT(2)
#define PCI_EXP_SLT_STA_PDC            BIT(3)
#define PCI_EXP_SLT_STA_CCI            BIT(4)
#define PCI_EXP_SLT_STA_MRL            BIT(5)
#define PCI_EXP_SLT_STA_PDS            BIT(6)
#define PCI_EXP_SLT_STA_EIS            BIT(7)
#define PCI_EXP_SLT_STA_DLLSC          BIT(8)

/*
 * Root Control
 */
#define PCI_EXP_RTCTL_SECEE            BIT(0)
#define PCI_EXP_RTCTL_SENFEE           BIT(1)
#define PCI_EXP_RTCTL_SEFEE            BIT(2)
#define PCI_EXP_RTCTL_PMEIE            BIT(3)
#define PCI_EXP_RTCTL_CRSVIS           BIT(4)

/*
 * Root Status
 */
#define PCI_EXP_RTSTA_PME_REQ_ID_MASK  0x0000ffff
#define PCI_EXP_RTSTA_PME_STATUS       BIT(16)
#define PCI_EXP_RTSTA_PME_PENDING      BIT(17)

/*
 * PCI Express Link Capability 2
 */
#define PCI_EXP_LNKCAP2_SPEED_MASK     0x0000007f

/*
 * PCI Express Link Control 2
 */
#define PCI_EXP_LNKCTL2_SPEED_MASK     0x000f
#define PCI_EXP_LNKCTL2_ENTER_COMP     BIT(4)
#define PCI_EXP_LNKCTL2_HW_AUT_SPEED   BIT(5)
#define PCI_EXP_LNKCTL2_DEEMPHASIS     BIT(6)
#define PCI_EXP_LNKCTL2_TX_MARGIN      0x0380
#define PCI_EXP_LNKCTL2_HASD           BIT(9)
#define PCI_EXP_LNKCTL2_COMPLIANCE     BIT(4)

/*
 * PCI Express Extended Capability
 *
 * Header:
 *   bits 15:0   Capability ID
 *   bits 19:16  Capability Version
 *   bits 31:20  Next Capability Offset
 */
#define PCI_EXT_CAP_ID_MASK            0x0000ffff
#define PCI_EXT_CAP_VER_MASK           0x000f0000
#define PCI_EXT_CAP_VER_SHIFT          16
#define PCI_EXT_CAP_NEXT_MASK          0xfff00000
#define PCI_EXT_CAP_NEXT_SHIFT         20

#define PCI_EXT_CAP_ID_AER             0x0001
#define PCI_EXT_CAP_ID_VC              0x0002
#define PCI_EXT_CAP_ID_DSN             0x0003
#define PCI_EXT_CAP_ID_RCLD            0x0005
#define PCI_EXT_CAP_ID_RBER            0x0007
#define PCI_EXT_CAP_ID_VC2             0x0009
#define PCI_EXT_CAP_ID_RCRB            0x000a
#define PCI_EXT_CAP_ID_VNDR            0x000b
#define PCI_EXT_CAP_ID_ACS             0x000d
#define PCI_EXT_CAP_ID_ARI             0x000e
#define PCI_EXT_CAP_ID_ATS             0x000f
#define PCI_EXT_CAP_ID_SRIOV           0x0010
#define PCI_EXT_CAP_ID_PRI             0x0013
#define PCI_EXT_CAP_ID_REBAR           0x0015
#define PCI_EXT_CAP_ID_DPA             0x0016
#define PCI_EXT_CAP_ID_TPH             0x0017
#define PCI_EXT_CAP_ID_LTR             0x0018
#define PCI_EXT_CAP_ID_SECPCI          0x0019
#define PCI_EXT_CAP_ID_PMUX            0x001a
#define PCI_EXT_CAP_ID_DPC             0x001d
#define PCI_EXT_CAP_ID_L1SS            0x001e
#define PCI_EXT_CAP_ID_PTM             0x001f
#define PCI_EXT_CAP_ID_DVSEC           0x0023
#define PCI_EXT_CAP_ID_DLF             0x0025
#define PCI_EXT_CAP_ID_IDE             0x0030

/*
 * AER Extended Capability
 */
#define PCI_AER_UNCORR_STATUS          0x04
#define PCI_AER_UNCORR_MASK            0x08
#define PCI_AER_UNCORR_SEVERITY        0x0c
#define PCI_AER_CORR_STATUS            0x10
#define PCI_AER_CORR_MASK              0x14
#define PCI_AER_CAP_CONTROL            0x18
#define PCI_AER_HEADER_LOG             0x1c

/*
 * ACS Extended Capability
 */
#define PCI_ACS_CAP                    0x04
#define PCI_ACS_CTRL                   0x06
#define PCI_ACS_STATUS                 0x08

/*
 * SR-IOV Extended Capability
 */
#define PCI_SRIOV_CAP                  0x04
#define PCI_SRIOV_CTRL                 0x08
#define PCI_SRIOV_STATUS               0x0a
#define PCI_SRIOV_INITIAL_VF           0x0c
#define PCI_SRIOV_TOTAL_VF             0x0e
#define PCI_SRIOV_NUM_VF               0x10
#define PCI_SRIOV_FUNC_DEP_LINK        0x12
#define PCI_SRIOV_FIRST_VF             0x14
#define PCI_SRIOV_VF_OFFSET            0x16
#define PCI_SRIOV_VF_STRIDE            0x18
#define PCI_SRIOV_VF_DID               0x1a
#define PCI_SRIOV_SUP_PAGE_SIZE        0x1c
#define PCI_SRIOV_SYS_PAGE_SIZE        0x20
#define PCI_SRIOV_BAR0                 0x24
#define PCI_SRIOV_BAR1                 0x28
#define PCI_SRIOV_BAR2                 0x2c
#define PCI_SRIOV_BAR3                 0x30
#define PCI_SRIOV_BAR4                 0x34
#define PCI_SRIOV_BAR5                 0x38

int ax_pcie_check_cap(u8 port, u8 bus, u8 dev, u8 func,
                          int cap_type, u16 cap_id);

int ax_pcie_init_ep_bars(int port, u8 bus, u8 dev, u8 func);

void ax_pcie_print_header(int port, u8 bus, u8 dev, u8 func);
void ax_pcie_print_capabilities(int port, u8 bus, u8 dev, u8 func);

void ax_pcie_print_capability(int port,
				     u8 bus, u8 dev, u8 func,
				     u8 offset);

void ax_pcie_print_pcie_cap(int port,
				   u8 bus, u8 dev, u8 func,
				   u16 offset);

void ax_pcie_print_msi_cap(int port,
				  u8 bus, u8 dev, u8 func,
				  u16 offset);

void ax_pcie_print_msix_cap(int port,
				   u8 bus, u8 dev, u8 func,
				   u16 offset);

void ax_pcie_print_pm_cap(int port,
				 u8 bus, u8 dev, u8 func,
				 u16 offset);

#endif /* __PCIE_CONF_H */
