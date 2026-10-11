// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#ifndef __NVME_H
#define __NVME_H

#include <linux/types.h>
#include <linux/bitops.h>

/*
 * NVMe BAR register offsets
 */
#define NVME_REG_CAP            0x0000
#define NVME_REG_VS             0x0008
#define NVME_REG_INTMS          0x000c
#define NVME_REG_INTMC          0x0010
#define NVME_REG_CC             0x0014
#define NVME_REG_CSTS           0x001c
#define NVME_REG_NSSR           0x0020
#define NVME_REG_AQA            0x0024
#define NVME_REG_ASQ            0x0028
#define NVME_REG_ACQ            0x0030

#define NVME_REG_DBS            0x1000

/*
 * CAP
 */
#define NVME_CAP_MQES(cap)      ((u32)((cap) & 0xffff))
#define NVME_CAP_CQR(cap)       (((cap) >> 16) & 0x1)
#define NVME_CAP_AMS(cap)       (((cap) >> 17) & 0x3)
#define NVME_CAP_TO(cap)        (((cap) >> 24) & 0xff)
#define NVME_CAP_DSTRD(cap)     (((cap) >> 32) & 0xf)
#define NVME_CAP_MPSMIN(cap)    (((cap) >> 48) & 0xf)
#define NVME_CAP_MPSMAX(cap)    (((cap) >> 52) & 0xf)

/*
 * CC
 */
#define NVME_CC_EN              BIT(0)

#define NVME_CC_CSS_SHIFT       4
#define NVME_CC_MPS_SHIFT       7
#define NVME_CC_AMS_SHIFT       11
#define NVME_CC_SHN_SHIFT       14
#define NVME_CC_IOSQES_SHIFT    16
#define NVME_CC_IOCQES_SHIFT    20

#define NVME_CC_CSS_NVM         (0 << NVME_CC_CSS_SHIFT)
#define NVME_CC_AMS_RR          (0 << NVME_CC_AMS_SHIFT)
#define NVME_CC_SHN_NONE        (0 << NVME_CC_SHN_SHIFT)

#define NVME_CC_IOSQES          (6 << NVME_CC_IOSQES_SHIFT)
#define NVME_CC_IOCQES          (4 << NVME_CC_IOCQES_SHIFT)

/*
 * CSTS
 */
#define NVME_CSTS_RDY           BIT(0)
#define NVME_CSTS_CFS           BIT(1)

/*
 * AQA
 */
#define NVME_AQA_ASQS(x)        (((x) - 1) & 0xfff)
#define NVME_AQA_ACQS(x)        ((((x) - 1) & 0xfff) << 16)

/*
 * Queue
 */
#define NVME_ADMIN_QID          0
#define NVME_IO_QID             1

#define NVME_ADMIN_Q_DEPTH      2
#define NVME_IO_Q_DEPTH         16
#define NVME_MAX_XFER_BYTES     (256 * 1024)
#define NVME_CMD_TIMEOUT_MS     5000

/*
 * NVMe opcodes
 */
#define NVME_ADMIN_DELETE_SQ    0x00
#define NVME_ADMIN_CREATE_SQ    0x01
#define NVME_ADMIN_DELETE_CQ    0x04
#define NVME_ADMIN_CREATE_CQ    0x05
#define NVME_ADMIN_IDENTIFY     0x06
#define NVME_ADMIN_SET_FEATURES 0x09

#define NVME_FEAT_NUM_QUEUES    0x07

#define NVME_CMD_FLUSH          0x00
#define NVME_CMD_WRITE          0x01
#define NVME_CMD_READ           0x02

/*
 * Identify CNS
 */
#define NVME_ID_CNS_NS          0x00
#define NVME_ID_CNS_CTRL        0x01

/*
 * Queue flags
 */
#define NVME_QUEUE_PHYS_CONTIG  BIT(0)
#define NVME_CQ_IRQ_ENABLED     BIT(1)

#define NVME_SQ_PRIO_MEDIUM     (2 << 1)

/*
 * Submission queue entry
 */
struct nvme_command {
    u8      opcode;
    u8      flags;
    __le16  command_id;

    __le32  nsid;
    __le32  cdw2;
    __le32  cdw3;

    __le64  metadata;
    __le64  prp1;
    __le64  prp2;

    __le32  cdw10;
    __le32  cdw11;
    __le32  cdw12;
    __le32  cdw13;
    __le32  cdw14;
    __le32  cdw15;
} __packed;

/*
 * Completion queue entry
 */
struct nvme_completion {
    __le32  result;
    __le32  rsvd;
    __le16  sq_head;
    __le16  sq_id;
    __le16  command_id;
    __le16  status;
} __packed;

/*
 * Queue
 */
struct ax_nvme_queue {
    struct nvme_command *sq_cmds;
    struct nvme_completion *cqes;

    u64 sq_dma;
    u64 cq_dma;

    u16 q_depth;

    u16 sq_tail;
    u16 cq_head;

    u16 qid;
    u8 cq_phase;
    bool irq_enabled;          /* CQ requests an interrupt on completion */

    u16 cmd_id;
};

/*
 * NVMe device
 */
struct ax_nvme_dev {
    u8 pcie_port;

    /* PCIe MSI state; ISR only sets msi_pending. */
    volatile bool msi_pending;
    bool msi_enabled;
    struct ax_pcie_msi msi;

    u8 bus;
    u8 dev;
    u8 func;

    u64 bar0;

    /*
     * CAP
     */
    u64 cap;

    u32 page_size;
    u32 db_stride;

    u16 q_depth;            /* admin queue depth */

    u32 ctrl_config;

    /* Identify Controller */
    char sn[21];
    char mn[41];
    char fr[9];
    u32 max_xfer_bytes;     /* from MDTS, capped by NVME_MAX_XFER_BYTES */

    /* Identify Namespace (nsid) */
    u32 nsid;
    u64 nsze;               /* namespace size in blocks */
    u32 lba_shift;          /* log2(block size) */

    struct ax_nvme_queue adminq;
    struct ax_nvme_queue ioq;
};

/*
 * Public APIs
 */
int ax_nvme_probe(struct ax_nvme_dev *ndev,
                  u8 pcie_port,
                  u8 bus,
                  u8 dev,
                  u8 func);

int ax_nvme_identify_controller(struct ax_nvme_dev *ndev,
                                void *buf,
                                u64 dma_addr);

int ax_nvme_identify_namespace(struct ax_nvme_dev *ndev,
                               u32 nsid,
                               void *buf,
                               u64 dma_addr);

/*
 * Block I/O.  @buf is the CPU pointer, @dma_addr the PCIe bus address of the
 * same memory (identity mapped through the inbound ATR, so usually equal).
 * @buf/@dma_addr must be dword aligned; @buf should be cache-line aligned
 * and be reachable by the SSD's DMA.
 */
int ax_nvme_read(struct ax_nvme_dev *ndev,
                 u32 nsid,
                 u64 lba,
                 u32 blocks,
                 void *buf,
                 u64 dma_addr);

int ax_nvme_write(struct ax_nvme_dev *ndev,
                  u32 nsid,
                  u64 lba,
                  u32 blocks,
                  const void *buf,
                  u64 dma_addr);

#endif /* __NVME_H */
