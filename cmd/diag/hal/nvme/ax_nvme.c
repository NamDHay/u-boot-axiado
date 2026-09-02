// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy namhuyngn03@gmail.com
 */

#include <dm.h>
#include <stdio.h>
#include <asm/io.h>
#include <linux/printk.h>
#include <time.h>
#include <linux/delay.h>
#include <malloc.h>
#include <asm/cache.h>
#include <display_options.h>

#include "ax_pcie.h"
#include "ax_pcie_conf.h"
#include "ax_nvme.h"

static int ax_nvme_read32(struct ax_nvme_dev *dev,
                          u64 reg, u32 *val)
{
    return ax_pcie_mem_read(dev->pcie_port, dev->bar0 + reg,
                            val, sizeof(*val));
}

static int ax_nvme_write32(struct ax_nvme_dev *dev,
                           u64 reg, u32 val)
{
    return ax_pcie_mem_write(dev->pcie_port, dev->bar0 + reg,
                             &val, sizeof(val));
}

static int ax_nvme_read64(struct ax_nvme_dev *dev,
                          u64 reg, u64 *val)
{
    return ax_pcie_mem_read(dev->pcie_port, dev->bar0 + reg,
                            val, sizeof(*val));
}

static int ax_nvme_write64(struct ax_nvme_dev *dev,
                           u64 reg, u64 val)
{
    return ax_pcie_mem_write(dev->pcie_port, dev->bar0 + reg,
                             &val, sizeof(val));
}

static int ax_nvme_wait_ready(struct ax_nvme_dev *ndev,
                              bool enabled)
{
    u32 csts;
    u32 timeout_ms;
    ulong start;
    int ret;

    /*
     * CAP.TO đơn vị 500 ms
     */
    timeout_ms = NVME_CAP_TO(ndev->cap) * 500;

    if (!timeout_ms)
        timeout_ms = 500;

    start = get_timer(0);

    while (get_timer(start) < timeout_ms) {
        ret = ax_nvme_read32(ndev,
                             NVME_REG_CSTS,
                             &csts);
        if (ret)
            return ret;

        if (csts & NVME_CSTS_CFS) {
            printf("NVMe fatal status: CSTS=0x%08x\n",
                   csts);
            return -EIO;
        }

        if (!!(csts & NVME_CSTS_RDY) == enabled)
            return 0;

        udelay(100);
    }

    printf("Timeout waiting NVMe RDY=%d\n", enabled);

    return -ETIMEDOUT;
}

static int ax_nvme_enable_pcie(struct ax_nvme_dev *ndev)
{
    u32 val;
    int ret;

    ret = ax_pcie_cfg_read(ndev->pcie_port,
                           ndev->bus,
                           ndev->dev,
                           ndev->func,
                           PCI_COMMAND,
                           &val);
    if (ret)
        return ret;

    val |= PCI_COMMAND_MEMORY;
    val |= PCI_COMMAND_MASTER;

    ret = ax_pcie_cfg_write(ndev->pcie_port,
                            ndev->bus,
                            ndev->dev,
                            ndev->func,
                            PCI_COMMAND,
                            val);
    if (ret)
        return ret;

    printf("PCI Command = 0x%08x\n", val);

    return 0;
}

static int ax_nvme_get_bar0(struct ax_nvme_dev *ndev)
{
    u32 bar_lo;
    u32 bar_hi;
    u64 bar;
    int ret;

    ret = ax_pcie_cfg_read(ndev->pcie_port,
                           ndev->bus,
                           ndev->dev,
                           ndev->func,
                           PCI_BASE_ADDRESS_0,
                           &bar_lo);
    if (ret)
        return ret;

    if ((bar_lo & 0x1) == 0) {
        u32 type;

        type = (bar_lo >> 1) & 0x3;

        bar = bar_lo & ~0xf;

        /*
         * 64-bit BAR
         */
        if (type == 0x2) {
            ret = ax_pcie_cfg_read(ndev->pcie_port,
                                   ndev->bus,
                                   ndev->dev,
                                   ndev->func,
                                   PCI_BASE_ADDRESS_0 + 4,
                                   &bar_hi);
            if (ret)
                return ret;

            bar |= ((u64)bar_hi << 32);
        }

        ndev->bar0 = bar;

        printf("NVMe BAR0 = 0x%llx\n", ndev->bar0);

        return 0;
    }

    return -EINVAL;
}

static int ax_nvme_disable_ctrl(struct ax_nvme_dev *ndev)
{
    u32 cc;
    int ret;

    ret = ax_nvme_read32(ndev,
                         NVME_REG_CC,
                         &cc);
    if (ret)
        return ret;

    cc &= ~NVME_CC_EN;

    ret = ax_nvme_write32(ndev,
                          NVME_REG_CC,
                          cc);
    if (ret)
        return ret;

    return ax_nvme_wait_ready(ndev, false);
}

static int ax_nvme_enable_ctrl(struct ax_nvme_dev *ndev)
{
    u32 cc;
    int ret;

    cc = ndev->ctrl_config | NVME_CC_EN;

    ret = ax_nvme_write32(ndev,
                          NVME_REG_CC,
                          cc);
    if (ret)
        return ret;

    return ax_nvme_wait_ready(ndev, true);
}

static u64 ax_nvme_sq_db_offset(struct ax_nvme_dev *ndev,
                                u16 qid)
{
    return NVME_REG_DBS +
           ((u64)(2 * qid) * ndev->db_stride);
}

static u64 ax_nvme_cq_db_offset(struct ax_nvme_dev *ndev,
                                u16 qid)
{
    return NVME_REG_DBS +
           ((u64)(2 * qid + 1) * ndev->db_stride);
}

static int ax_nvme_ring_sq(struct ax_nvme_dev *ndev,
                           struct ax_nvme_queue *q)
{
    return ax_nvme_write32(ndev,
                           ax_nvme_sq_db_offset(ndev, q->qid),
                           q->sq_tail);
}

static int ax_nvme_ring_cq(struct ax_nvme_dev *ndev,
                           struct ax_nvme_queue *q)
{
    return ax_nvme_write32(ndev,
                           ax_nvme_cq_db_offset(ndev, q->qid),
                           q->cq_head);
}

static int ax_nvme_alloc_queue(struct ax_nvme_dev *ndev,
                               struct ax_nvme_queue *q,
                               u16 qid,
                               u16 depth)
{
    size_t sq_size;
    size_t cq_size;

    sq_size = depth * sizeof(struct nvme_command);
    cq_size = depth * sizeof(struct nvme_completion);

    q->sq_cmds = memalign(ARCH_DMA_MINALIGN,
                          ALIGN(sq_size,
                                ARCH_DMA_MINALIGN));

    if (!q->sq_cmds)
        return -ENOMEM;

    q->cqes = memalign(ARCH_DMA_MINALIGN,
                       ALIGN(cq_size,
                             ARCH_DMA_MINALIGN));

    if (!q->cqes) {
        free(q->sq_cmds);
        q->sq_cmds = NULL;

        return -ENOMEM;
    }

    memset(q->sq_cmds, 0, sq_size);
    memset(q->cqes, 0, cq_size);

    q->qid = qid;
    q->q_depth = depth;

    q->sq_tail = 0;
    q->cq_head = 0;

    q->cq_phase = 1;
    q->cmd_id = 0;

    q->sq_dma = (u64)(uintptr_t)q->sq_cmds;
    q->cq_dma = (u64)(uintptr_t)q->cqes;

    flush_dcache_range((ulong)q->sq_cmds,
                       (ulong)q->sq_cmds +
                       ALIGN(sq_size, ARCH_DMA_MINALIGN));

    flush_dcache_range((ulong)q->cqes,
                       (ulong)q->cqes +
                       ALIGN(cq_size, ARCH_DMA_MINALIGN));

    return 0;
}

static int ax_nvme_configure_admin_queue(struct ax_nvme_dev *ndev)
{
    struct ax_nvme_queue *q;
    u32 aqa;
    u32 page_shift;
    int ret;

    q = &ndev->adminq;

    /*
     * NVMe MPSMIN được tính từ 4 KiB.
     *
     * page_shift = 12 + CAP.MPSMIN
     */
    page_shift = 12 + NVME_CAP_MPSMIN(ndev->cap);

    ndev->page_size = 1 << page_shift;

    /*
     * U-Boot basic NVMe config
     */
    ndev->ctrl_config = NVME_CC_CSS_NVM;

    ndev->ctrl_config |=
        (page_shift - 12) << NVME_CC_MPS_SHIFT;

    ndev->ctrl_config |= NVME_CC_AMS_RR;
    ndev->ctrl_config |= NVME_CC_SHN_NONE;
    ndev->ctrl_config |= NVME_CC_IOSQES;
    ndev->ctrl_config |= NVME_CC_IOCQES;

    ret = ax_nvme_alloc_queue(ndev,
                              q,
                              NVME_ADMIN_QID,
                              ndev->q_depth);
    if (ret)
        return ret;

    aqa = NVME_AQA_ASQS(q->q_depth);
    aqa |= NVME_AQA_ACQS(q->q_depth);

    ret = ax_nvme_write32(ndev,
                          NVME_REG_AQA,
                          aqa);
    if (ret)
        return ret;

    ret = ax_nvme_write64(ndev,
                          NVME_REG_ASQ,
                          q->sq_dma);
    if (ret)
        return ret;

    ret = ax_nvme_write64(ndev,
                          NVME_REG_ACQ,
                          q->cq_dma);
    if (ret)
        return ret;

    printf("AQA = 0x%08x\n", aqa);
    printf("ASQ = 0x%016llx\n", q->sq_dma);
    printf("ACQ = 0x%016llx\n", q->cq_dma);

    return ax_nvme_enable_ctrl(ndev);
}

static int ax_nvme_submit_cmd(struct ax_nvme_dev *ndev,
                              struct ax_nvme_queue *q,
                              struct nvme_command *cmd)
{
    struct nvme_completion *cqe;
    u16 cmdid;
    u16 status;
    ulong start;
    int ret;

    cmdid = q->cmd_id++;

    cmd->command_id = cpu_to_le16(cmdid);

    /*
     * Copy command vào SQ
     */
    memcpy(&q->sq_cmds[q->sq_tail],
           cmd,
           sizeof(*cmd));

    flush_dcache_range(
        (ulong)&q->sq_cmds[q->sq_tail],
        (ulong)&q->sq_cmds[q->sq_tail] +
        ALIGN(sizeof(*cmd), ARCH_DMA_MINALIGN));

    /*
     * Advance SQ tail
     */
    q->sq_tail++;

    if (q->sq_tail == q->q_depth)
        q->sq_tail = 0;

    /*
     * Ring SQ doorbell
     */
    ret = ax_nvme_ring_sq(ndev, q);
    if (ret)
        return ret;

    /*
     * Poll CQ
     */
    start = get_timer(0);

    while (get_timer(start) < 30000) {
        cqe = &q->cqes[q->cq_head];

        invalidate_dcache_range(
            (ulong)cqe,
            (ulong)cqe +
            ALIGN(sizeof(*cqe),
                  ARCH_DMA_MINALIGN));

        /*
         * CQ phase bit = status bit0
         */
        status = le16_to_cpu(cqe->status);

        if ((status & 0x1) == q->cq_phase)
            break;

        udelay(10);
    }

    if (get_timer(start) >= 30000) {
        printf("NVMe command timeout\n");
        return -ETIMEDOUT;
    }

    /*
     * Validate command ID
     */
    if (le16_to_cpu(cqe->command_id) != cmdid) {
        printf("Unexpected completion cmdid=%u expected=%u\n",
               le16_to_cpu(cqe->command_id),
               cmdid);

        return -EIO;
    }

    status = le16_to_cpu(cqe->status);

    /*
     * NVMe completion status:
     * bits 15:1 = Status Code / Status Code Type
     * bit 0      = Phase
     */
    if ((status >> 1) != 0) {
        printf("NVMe command failed: status=0x%04x\n",
               status);

        return -EIO;
    }

    /*
     * Advance CQ head
     */
    q->cq_head++;

    if (q->cq_head == q->q_depth) {
        q->cq_head = 0;
        q->cq_phase ^= 1;
    }

    /*
     * Tell controller CQ entry consumed
     */
    ret = ax_nvme_ring_cq(ndev, q);

    return ret;
}

int ax_nvme_identify_controller(struct ax_nvme_dev *ndev,
                                void *buf,
                                u64 dma_addr)
{
    struct nvme_command cmd;
    int ret;

    memset(&cmd, 0, sizeof(cmd));

    memset(buf, 0, 4096);

    flush_dcache_range((ulong)buf,
                       (ulong)buf + 4096);

    cmd.opcode = NVME_ADMIN_IDENTIFY;

    /*
     * PRP1 = địa chỉ host buffer mà NVMe device
     * sẽ DMA data vào
     */
    cmd.prp1 = cpu_to_le64(dma_addr);

    /*
     * CNS = Controller
     */
    cmd.cdw10 = cpu_to_le32(NVME_ID_CNS_CTRL);

    ret = ax_nvme_submit_cmd(ndev,
                             &ndev->adminq,
                             &cmd);
    if (ret)
        return ret;

    invalidate_dcache_range((ulong)buf,
                            (ulong)buf + 4096);

    return 0;
}

int ax_nvme_identify_namespace(struct ax_nvme_dev *ndev,
                               u32 nsid,
                               void *buf,
                               u64 dma_addr)
{
    struct nvme_command cmd;
    int ret;

    memset(&cmd, 0, sizeof(cmd));
    memset(buf, 0, 4096);

    flush_dcache_range((ulong)buf,
                       (ulong)buf + 4096);

    cmd.opcode = NVME_ADMIN_IDENTIFY;

    cmd.nsid = cpu_to_le32(nsid);

    cmd.prp1 = cpu_to_le64(dma_addr);

    cmd.cdw10 = cpu_to_le32(NVME_ID_CNS_NS);

    ret = ax_nvme_submit_cmd(ndev,
                             &ndev->adminq,
                             &cmd);
    if (ret)
        return ret;

    invalidate_dcache_range((ulong)buf,
                            (ulong)buf + 4096);

    return 0;
}

int ax_nvme_probe(struct ax_nvme_dev *ndev,
                  u8 pcie_port,
                  u8 bus,
                  u8 dev,
                  u8 func)
{
    u32 class_rev;
    u32 csts;
    int ret;

    memset(ndev, 0, sizeof(*ndev));

    ndev->pcie_port = pcie_port;

    ndev->bus = bus;
    ndev->dev = dev;
    ndev->func = func;

    /*
     * Check class code
     */
    ret = ax_pcie_cfg_read(pcie_port,
                           bus,
                           dev,
                           func,
                           PCI_CLASS_REVISION,
                           &class_rev);
    if (ret)
        return ret;

    printf("PCI class/rev = 0x%08x\n",
           class_rev);

    /*
     * Enable Memory Space + Bus Master
     */
    ret = ax_nvme_enable_pcie(ndev);
    if (ret)
        return ret;

    /*
     * Read BAR0
     */
    ret = ax_nvme_get_bar0(ndev);
    if (ret)
        return ret;

    /*
     * Test MMIO access
     */
    ret = ax_nvme_read32(ndev,
                         NVME_REG_CSTS,
                         &csts);
    if (ret)
        return ret;

    printf("NVMe CSTS = 0x%08x\n",
           csts);

    /*
     * Read CAP
     */
    ret = ax_nvme_read64(ndev,
                         NVME_REG_CAP,
                         &ndev->cap);
    if (ret)
        return ret;

    printf("NVMe CAP = 0x%016llx\n",
           ndev->cap);

    printf("MQES   = %u\n",
           NVME_CAP_MQES(ndev->cap));

    printf("DSTRD  = %u\n",
           NVME_CAP_DSTRD(ndev->cap));

    printf("MPSMIN = %u\n",
           NVME_CAP_MPSMIN(ndev->cap));

    /*
     * Doorbell stride
     *
     * Byte offset stride = 4 << DSTRD
     */
    ndev->db_stride =
        4 << NVME_CAP_DSTRD(ndev->cap);

    /*
     * Queue depth
     */
    ndev->q_depth = min_t(u16,
                          NVME_CAP_MQES(ndev->cap) + 1,
                          NVME_ADMIN_Q_DEPTH);

    if (ndev->q_depth < 2)
        return -EINVAL;

    /*
     * Controller must be disabled before
     * programming AQA/ASQ/ACQ.
     */
    ret = ax_nvme_disable_ctrl(ndev);
    if (ret)
        return ret;

    /*
     * Setup admin queue
     */
    ret = ax_nvme_configure_admin_queue(ndev);
    if (ret)
        return ret;

    printf("NVMe controller ready\n");

    return 0;
}
