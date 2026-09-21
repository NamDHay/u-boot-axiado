// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#include <dm.h>
#include <stdio.h>
#include <string.h>
#include <asm/io.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <time.h>
#include <linux/delay.h>
#include <malloc.h>
#include <asm/cache.h>
#include <cpu_func.h>
#include <display_options.h>

#include "ax_pcie.h"
#include "ax_pcie_conf.h"
#include "ax_nvme.h"

/*
 * DMA memory shared with the NVMe controller.
 *
 * Everything here is a *PCIe bus address* as seen by the SSD, so it MUST lie
 * inside the inbound (PCIe->AXI) window set up in axiado_pcie.c
 * (AXIADO_PCIE_DMA_PCI_BASE / _SIZE).  Make sure nothing else (U-Boot heap,
 * stack, relocated code) lives in this range.
 */
#define NVME_DMA_BASE            0x92000000ULL
#define NVME_ADMIN_SQ_ADDR       (NVME_DMA_BASE + 0x0000)
#define NVME_ADMIN_CQ_ADDR       (NVME_DMA_BASE + 0x1000)
#define NVME_IDENTIFY_BUF_ADDR   (NVME_DMA_BASE + 0x2000)
#define NVME_PRP_LIST_ADDR       (NVME_DMA_BASE + 0x3000)
#define NVME_IO_SQ_ADDR          (NVME_DMA_BASE + 0x4000)
#define NVME_IO_CQ_ADDR          (NVME_DMA_BASE + 0x5000)

#ifdef AX_NVME_DEBUG
#define ax_nvme_dbg(fmt, ...)    printf(fmt, ##__VA_ARGS__)
#else
#define ax_nvme_dbg(fmt, ...)    do { } while (0)
#endif

/* ------------------------------------------------------------------------
 * Cache helpers (ranges are widened to whole cache lines)
 * ------------------------------------------------------------------------ */

static void ax_nvme_cache_flush(const void *p, size_t len)
{
    ulong s = (ulong)p & ~((ulong)ARCH_DMA_MINALIGN - 1);
    ulong e = ALIGN((ulong)p + len, ARCH_DMA_MINALIGN);

    flush_dcache_range(s, e);
}

static void ax_nvme_cache_inv(const void *p, size_t len)
{
    ulong s = (ulong)p & ~((ulong)ARCH_DMA_MINALIGN - 1);
    ulong e = ALIGN((ulong)p + len, ARCH_DMA_MINALIGN);

    invalidate_dcache_range(s, e);
}

/* ------------------------------------------------------------------------
 * BAR0 register access (through the PCIe outbound memory window)
 * ------------------------------------------------------------------------ */

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

/*
 * 64-bit registers are accessed as two 32-bit accesses, low dword first,
 * which the NVMe spec explicitly allows.
 */
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

static void ax_nvme_dump_state(struct ax_nvme_dev *ndev)
{
    u32 cc = 0xffffffff, csts = 0xffffffff;

    ax_nvme_read32(ndev, NVME_REG_CC, &cc);
    ax_nvme_read32(ndev, NVME_REG_CSTS, &csts);
    printf("NVMe state: CC=0x%08x CSTS=0x%08x\n", cc, csts);
}

static int ax_nvme_wait_ready(struct ax_nvme_dev *ndev,
                              bool enabled)
{
    u32 csts;
    u32 timeout_ms;
    ulong start;
    int ret;

    /* CAP.TO is in units of 500 ms */
    timeout_ms = NVME_CAP_TO(ndev->cap) * 500;

    if (!timeout_ms)
        timeout_ms = 500;

    start = get_timer(0);

    while (get_timer(start) < timeout_ms) {
        ret = ax_nvme_read32(ndev, NVME_REG_CSTS, &csts);
        if (ret)
            return ret;

        /* all ones = MMIO is not reaching the device at all */
        if (csts == 0xffffffff) {
            printf("NVMe CSTS reads 0xffffffff: BAR/link not reachable\n");
            return -ENODEV;
        }

        if (csts & NVME_CSTS_CFS) {
            printf("NVMe fatal status: CSTS=0x%08x\n", csts);
            return -EIO;
        }

        if (!!(csts & NVME_CSTS_RDY) == enabled)
            return 0;

        udelay(100);
    }

    printf("Timeout waiting NVMe RDY=%d\n", enabled);
    ax_nvme_dump_state(ndev);

    return -ETIMEDOUT;
}

/* ------------------------------------------------------------------------
 * PCI side
 * ------------------------------------------------------------------------ */

static int ax_nvme_enable_pcie(struct ax_nvme_dev *ndev)
{
    u32 val;
    int ret;

    ret = ax_pcie_cfg_read(ndev->pcie_port, ndev->bus, ndev->dev,
                           ndev->func, PCI_CFG_COMMAND, &val);
    if (ret)
        return ret;

    /* keep only the command half: the upper half is RW1C status */
    val &= 0xffff;
    val |= PCI_COMMAND_MEMORY;
    val |= PCI_COMMAND_MASTER;
    val &= ~PCI_COMMAND_INTX_DISABLE;

    ret = ax_pcie_cfg_write(ndev->pcie_port, ndev->bus, ndev->dev,
                            ndev->func, PCI_CFG_COMMAND, val);
    if (ret)
        return ret;

    /* read back to be sure the write really reached the endpoint */
    ret = ax_pcie_cfg_read(ndev->pcie_port, ndev->bus, ndev->dev,
                           ndev->func, PCI_CFG_COMMAND, &val);
    if (ret)
        return ret;

    printf("PCI Command = 0x%04x\n", val & 0xffff);

    if (!(val & PCI_COMMAND_MEMORY) || !(val & PCI_COMMAND_MASTER)) {
        printf("Endpoint refused Memory/BusMaster enable\n");
        return -EIO;
    }

    return 0;
}

static int ax_nvme_get_bar0(struct ax_nvme_dev *ndev)
{
    u32 bar_lo;
    u32 bar_hi;
    u64 bar;
    int ret;

    ret = ax_pcie_cfg_read(ndev->pcie_port, ndev->bus, ndev->dev,
                           ndev->func, PCI_CFG_BAR0, &bar_lo);
    if (ret)
        return ret;

    if (bar_lo == 0xffffffff)
        return -ENODEV;

    /* NVMe registers must be in a memory BAR */
    if (bar_lo & PCI_BAR_IO)
        return -EINVAL;

    bar = bar_lo & PCI_BAR_ADDR_MASK;

    /* 64-bit BAR */
    if ((bar_lo & PCI_BAR_MEM_TYPE_MASK) == PCI_BAR_MEM_TYPE_64) {
        ret = ax_pcie_cfg_read(ndev->pcie_port, ndev->bus, ndev->dev,
                               ndev->func, PCI_CFG_BAR1, &bar_hi);
        if (ret)
            return ret;

        bar |= ((u64)bar_hi << 32);
    }

    /* not assigned by ax_pcie_init_ep_bars() yet */
    if (!bar)
        return -ENOENT;

    ndev->bar0 = bar;

    printf("NVMe BAR0 = 0x%llx\n", ndev->bar0);

    return 0;
}

/* ------------------------------------------------------------------------
 * Controller enable / disable
 * ------------------------------------------------------------------------ */

static int ax_nvme_disable_ctrl(struct ax_nvme_dev *ndev)
{
    u32 cc;
    int ret;

    ret = ax_nvme_read32(ndev, NVME_REG_CC, &cc);
    if (ret)
        return ret;

    cc &= ~NVME_CC_EN;

    ret = ax_nvme_write32(ndev, NVME_REG_CC, cc);
    if (ret)
        return ret;

    return ax_nvme_wait_ready(ndev, false);
}

static int ax_nvme_enable_ctrl(struct ax_nvme_dev *ndev)
{
    u32 cc;
    int ret;

    cc = ndev->ctrl_config | NVME_CC_EN;

    ret = ax_nvme_write32(ndev, NVME_REG_CC, cc);
    if (ret)
        return ret;

    return ax_nvme_wait_ready(ndev, true);
}

/* ------------------------------------------------------------------------
 * Doorbells
 * ------------------------------------------------------------------------ */

static u64 ax_nvme_sq_db_offset(struct ax_nvme_dev *ndev, u16 qid)
{
    return NVME_REG_DBS + ((u64)(2 * qid) * ndev->db_stride);
}

static u64 ax_nvme_cq_db_offset(struct ax_nvme_dev *ndev, u16 qid)
{
    return NVME_REG_DBS + ((u64)(2 * qid + 1) * ndev->db_stride);
}

static int ax_nvme_ring_sq(struct ax_nvme_dev *ndev, struct ax_nvme_queue *q)
{
    ax_nvme_dbg("Ring SQ%u DB: off=0x%llx val=%u\n", q->qid,
                ax_nvme_sq_db_offset(ndev, q->qid), q->sq_tail);

    return ax_nvme_write32(ndev, ax_nvme_sq_db_offset(ndev, q->qid),
                           q->sq_tail);
}

static int ax_nvme_ring_cq(struct ax_nvme_dev *ndev, struct ax_nvme_queue *q)
{
    return ax_nvme_write32(ndev, ax_nvme_cq_db_offset(ndev, q->qid),
                           q->cq_head);
}

/* ------------------------------------------------------------------------
 * Queues
 * ------------------------------------------------------------------------ */

static void ax_nvme_init_queue(struct ax_nvme_queue *q, u16 qid, u16 depth,
                               u64 sq_addr, u64 cq_addr)
{
    memset((void *)(ulong)sq_addr, 0, 4096);
    memset((void *)(ulong)cq_addr, 0, 4096);

    q->sq_cmds = (struct nvme_command *)(ulong)sq_addr;
    q->cqes = (struct nvme_completion *)(ulong)cq_addr;

    q->sq_dma = sq_addr;
    q->cq_dma = cq_addr;

    q->qid = qid;
    q->q_depth = depth;

    q->sq_tail = 0;
    q->cq_head = 0;
    q->cq_phase = 1;
    q->cmd_id = 0;

    ax_nvme_cache_flush(q->sq_cmds, 4096);
    ax_nvme_cache_flush(q->cqes, 4096);
}

static int ax_nvme_configure_admin_queue(struct ax_nvme_dev *ndev)
{
    struct ax_nvme_queue *q = &ndev->adminq;
    u32 aqa;
    u32 page_shift;
    int ret;

    /* page_shift = 12 + CAP.MPSMIN */
    page_shift = 12 + NVME_CAP_MPSMIN(ndev->cap);
    ndev->page_size = 1 << page_shift;

    ndev->ctrl_config = NVME_CC_CSS_NVM;
    ndev->ctrl_config |= (page_shift - 12) << NVME_CC_MPS_SHIFT;
    ndev->ctrl_config |= NVME_CC_AMS_RR;
    ndev->ctrl_config |= NVME_CC_SHN_NONE;
    ndev->ctrl_config |= NVME_CC_IOSQES;
    ndev->ctrl_config |= NVME_CC_IOCQES;

    ax_nvme_init_queue(q, NVME_ADMIN_QID, ndev->q_depth,
                       NVME_ADMIN_SQ_ADDR, NVME_ADMIN_CQ_ADDR);

    aqa = NVME_AQA_ASQS(q->q_depth);
    aqa |= NVME_AQA_ACQS(q->q_depth);

    ret = ax_nvme_write32(ndev, NVME_REG_AQA, aqa);
    if (ret)
        return ret;

    ret = ax_nvme_write64(ndev, NVME_REG_ASQ, q->sq_dma);
    if (ret)
        return ret;

    ret = ax_nvme_write64(ndev, NVME_REG_ACQ, q->cq_dma);
    if (ret)
        return ret;

    printf("AQA = 0x%08x ASQ = 0x%016llx ACQ = 0x%016llx\n",
           aqa, q->sq_dma, q->cq_dma);

    return ax_nvme_enable_ctrl(ndev);
}

/* ------------------------------------------------------------------------
 * Command submission (works for admin and I/O queues)
 * ------------------------------------------------------------------------ */

static int ax_nvme_submit_cmd(struct ax_nvme_dev *ndev,
                              struct ax_nvme_queue *q,
                              struct nvme_command *cmd,
                              u32 *result)
{
    struct nvme_completion *cqe;
    struct nvme_command *sqe;
    u16 cmdid;
    u16 status;
    u16 cqe_cid;
    bool done = false;
    ulong start;
    int ret;

    cmdid = q->cmd_id++;
    cmd->command_id = cpu_to_le16(cmdid);

    sqe = &q->sq_cmds[q->sq_tail];

    ax_nvme_dbg("NVMe submit: q%u slot %u cid %u opc 0x%02x\n",
                q->qid, q->sq_tail, cmdid, cmd->opcode);

    memcpy(sqe, cmd, sizeof(*cmd));
    ax_nvme_cache_flush(sqe, sizeof(*sqe));

    q->sq_tail = (q->sq_tail + 1) % q->q_depth;

    /* flush_dcache_range() ends with a barrier: SQE is visible before the bell */
    ret = ax_nvme_ring_sq(ndev, q);
    if (ret)
        return ret;

    /* Poll the CQ: an entry is new when its phase bit == expected phase */
    start = get_timer(0);
    do {
        cqe = &q->cqes[q->cq_head];
        ax_nvme_cache_inv(cqe, sizeof(*cqe));

        status = le16_to_cpu(cqe->status);
        if ((status & 0x1) == q->cq_phase) {
            done = true;
            break;
        }

        udelay(10);
    } while (get_timer(start) < NVME_CMD_TIMEOUT_MS);

    if (!done) {
        printf("NVMe command timeout (q%u opc 0x%02x)\n", q->qid,
               cmd->opcode);
        ax_nvme_dump_state(ndev);
        return -ETIMEDOUT;
    }

    cqe_cid = le16_to_cpu(cqe->command_id);
    if (result)
        *result = le32_to_cpu(cqe->result);

    /*
     * Consume the entry and tell the controller BEFORE looking at the
     * status: an error completion still occupies a CQ slot, otherwise the
     * queue state gets out of sync with the controller after the first error.
     */
    q->cq_head++;
    if (q->cq_head == q->q_depth) {
        q->cq_head = 0;
        q->cq_phase ^= 1;
    }

    ret = ax_nvme_ring_cq(ndev, q);
    if (ret)
        return ret;

    if (cqe_cid != cmdid) {
        printf("Unexpected completion cmdid=%u expected=%u\n", cqe_cid, cmdid);
        return -EIO;
    }

    if ((status >> 1) != 0) {
        printf("NVMe command failed: opc=0x%02x SCT=%u SC=0x%02x (status=0x%04x)\n",
               cmd->opcode, (status >> 9) & 0x7, (status >> 1) & 0xff, status);
        return -EIO;
    }

    return 0;
}

/* ------------------------------------------------------------------------
 * Admin commands
 * ------------------------------------------------------------------------ */

static void ax_nvme_trim(char *s)
{
    int n = strlen(s);

    while (n > 0 && (s[n - 1] == ' ' || s[n - 1] == '\0'))
        s[--n] = '\0';
}

int ax_nvme_identify_controller(struct ax_nvme_dev *ndev,
                                void *buf, u64 dma_addr)
{
    struct nvme_command cmd;
    int ret;

    memset(&cmd, 0, sizeof(cmd));
    memset(buf, 0, 4096);
    ax_nvme_cache_flush(buf, 4096);

    cmd.opcode = NVME_ADMIN_IDENTIFY;
    cmd.prp1 = cpu_to_le64(dma_addr);
    cmd.cdw10 = cpu_to_le32(NVME_ID_CNS_CTRL);

    ret = ax_nvme_submit_cmd(ndev, &ndev->adminq, &cmd, NULL);
    if (ret)
        return ret;

    ax_nvme_cache_inv(buf, 4096);

    return 0;
}

int ax_nvme_identify_namespace(struct ax_nvme_dev *ndev, u32 nsid,
                               void *buf, u64 dma_addr)
{
    struct nvme_command cmd;
    int ret;

    memset(&cmd, 0, sizeof(cmd));
    memset(buf, 0, 4096);
    ax_nvme_cache_flush(buf, 4096);

    cmd.opcode = NVME_ADMIN_IDENTIFY;
    cmd.nsid = cpu_to_le32(nsid);
    cmd.prp1 = cpu_to_le64(dma_addr);
    cmd.cdw10 = cpu_to_le32(NVME_ID_CNS_NS);

    ret = ax_nvme_submit_cmd(ndev, &ndev->adminq, &cmd, NULL);
    if (ret)
        return ret;

    ax_nvme_cache_inv(buf, 4096);

    return 0;
}

static int ax_nvme_identify(struct ax_nvme_dev *ndev)
{
    u8 *id = (u8 *)(ulong)NVME_IDENTIFY_BUF_ADDR;
    u32 lbaf;
    u64 nsze;
    u8 mdts;
    int ret;

    printf("Identify buffer = 0x%llx\n", (u64)NVME_IDENTIFY_BUF_ADDR);

    /* ---- controller ---- */
    ret = ax_nvme_identify_controller(ndev, id, NVME_IDENTIFY_BUF_ADDR);
    if (ret) {
        printf("Identify Controller failed: %d\n", ret);
        return ret;
    }

    memcpy(ndev->sn, id + 4, 20);   ndev->sn[20] = '\0';
    memcpy(ndev->mn, id + 24, 40);  ndev->mn[40] = '\0';
    memcpy(ndev->fr, id + 64, 8);   ndev->fr[8] = '\0';
    ax_nvme_trim(ndev->sn);
    ax_nvme_trim(ndev->mn);
    ax_nvme_trim(ndev->fr);

    mdts = id[77];
    ndev->max_xfer_bytes = mdts ? (ndev->page_size << mdts) :
                                  NVME_MAX_XFER_BYTES;
    if (ndev->max_xfer_bytes > NVME_MAX_XFER_BYTES)
        ndev->max_xfer_bytes = NVME_MAX_XFER_BYTES;

    printf("NVMe: model '%s' serial '%s' fw '%s' max xfer %u KiB\n",
           ndev->mn, ndev->sn, ndev->fr, ndev->max_xfer_bytes / 1024);

    /* ---- namespace 1 ---- */
    ndev->nsid = 1;
    ret = ax_nvme_identify_namespace(ndev, ndev->nsid, id,
                                     NVME_IDENTIFY_BUF_ADDR);
    if (ret) {
        printf("Identify Namespace %u failed: %d\n", ndev->nsid, ret);
        return ret;
    }

    memcpy(&nsze, id, sizeof(nsze));
    ndev->nsze = le64_to_cpu(nsze);

    /* FLBAS[3:0] selects the LBA format, LBAF[n] at 128 + 4n, LBADS = [23:16] */
    memcpy(&lbaf, id + 128 + 4 * (id[26] & 0xf), sizeof(lbaf));
    ndev->lba_shift = (le32_to_cpu(lbaf) >> 16) & 0xff;

    if (ndev->lba_shift < 9 || ndev->lba_shift > 16) {
        printf("Unsupported LBA size shift %u\n", ndev->lba_shift);
        return -EINVAL;
    }

    printf("NVMe: ns %u = %llu blocks of %u bytes\n", ndev->nsid,
           ndev->nsze, 1U << ndev->lba_shift);

    return 0;
}

static int ax_nvme_set_num_queues(struct ax_nvme_dev *ndev)
{
    struct nvme_command cmd;
    u32 res = 0;
    int ret;

    memset(&cmd, 0, sizeof(cmd));
    cmd.opcode = NVME_ADMIN_SET_FEATURES;
    cmd.cdw10 = cpu_to_le32(NVME_FEAT_NUM_QUEUES);
    cmd.cdw11 = cpu_to_le32(0);     /* 1 SQ + 1 CQ (0-based) */

    ret = ax_nvme_submit_cmd(ndev, &ndev->adminq, &cmd, &res);
    if (ret)
        return ret;

    ax_nvme_dbg("NVMe queues granted: 0x%08x\n", res);

    return 0;
}

static int ax_nvme_create_io_queues(struct ax_nvme_dev *ndev)
{
    struct ax_nvme_queue *q = &ndev->ioq;
    struct nvme_command cmd;
    u16 depth;
    int ret;

    depth = min_t(u32, (u32)NVME_CAP_MQES(ndev->cap) + 1, NVME_IO_Q_DEPTH);
    if (depth < 2)
        return -EINVAL;

    ax_nvme_init_queue(q, NVME_IO_QID, depth, NVME_IO_SQ_ADDR, NVME_IO_CQ_ADDR);

    /* Create I/O Completion Queue (polled: no IEN) */
    memset(&cmd, 0, sizeof(cmd));
    cmd.opcode = NVME_ADMIN_CREATE_CQ;
    cmd.prp1 = cpu_to_le64(q->cq_dma);
    cmd.cdw10 = cpu_to_le32(((u32)(depth - 1) << 16) | q->qid);
    cmd.cdw11 = cpu_to_le32(NVME_QUEUE_PHYS_CONTIG);

    ret = ax_nvme_submit_cmd(ndev, &ndev->adminq, &cmd, NULL);
    if (ret) {
        printf("Create I/O CQ failed: %d\n", ret);
        return ret;
    }

    /* Create I/O Submission Queue bound to that CQ */
    memset(&cmd, 0, sizeof(cmd));
    cmd.opcode = NVME_ADMIN_CREATE_SQ;
    cmd.prp1 = cpu_to_le64(q->sq_dma);
    cmd.cdw10 = cpu_to_le32(((u32)(depth - 1) << 16) | q->qid);
    cmd.cdw11 = cpu_to_le32(((u32)q->qid << 16) |
                            NVME_QUEUE_PHYS_CONTIG | NVME_SQ_PRIO_MEDIUM);

    ret = ax_nvme_submit_cmd(ndev, &ndev->adminq, &cmd, NULL);
    if (ret) {
        printf("Create I/O SQ failed: %d\n", ret);
        return ret;
    }

    printf("NVMe I/O queue 1 created, depth %u\n", depth);

    return 0;
}

/* ------------------------------------------------------------------------
 * Block I/O
 * ------------------------------------------------------------------------ */

static int ax_nvme_rw(struct ax_nvme_dev *ndev, u8 opcode, u32 nsid, u64 lba,
                      u32 blocks, void *buf, u64 dma_addr)
{
    u32 bsize = 1U << ndev->lba_shift;
    u32 max_blocks = ndev->max_xfer_bytes >> ndev->lba_shift;
    bool is_read = (opcode == NVME_CMD_READ);
    u8 *cbuf = buf;
    int ret;

    if (!ndev->lba_shift || !ndev->ioq.q_depth)
        return -ENODEV;

    if (!blocks || (dma_addr & 0x3))
        return -EINVAL;

    if (lba + blocks > ndev->nsze)
        return -ERANGE;

    if (!max_blocks)
        max_blocks = 1;

    while (blocks) {
        struct nvme_command cmd;
        u32 n = min(blocks, max_blocks);
        u32 len = n * bsize;
        u32 off = dma_addr & (ndev->page_size - 1);
        u32 first = ndev->page_size - off;
        u64 prp1 = dma_addr;
        u64 prp2 = 0;

        if (len > first) {
            u64 next = dma_addr + first;    /* page aligned */

            if (len <= first + ndev->page_size) {
                prp2 = next;
            } else {
                /* more than 2 pages: PRP2 points at a list of page addresses */
                __le64 *list = (__le64 *)(ulong)NVME_PRP_LIST_ADDR;
                u32 entries = DIV_ROUND_UP(len - first, ndev->page_size);
                u32 i;

                for (i = 0; i < entries; i++)
                    list[i] = cpu_to_le64(next + (u64)i * ndev->page_size);

                ax_nvme_cache_flush(list, entries * sizeof(*list));
                prp2 = NVME_PRP_LIST_ADDR;
            }
        }

        if (is_read)
            ax_nvme_cache_inv(cbuf, len);   /* drop stale/dirty lines first */
        else
            ax_nvme_cache_flush(cbuf, len);

        memset(&cmd, 0, sizeof(cmd));
        cmd.opcode = opcode;
        cmd.nsid = cpu_to_le32(nsid);
        cmd.prp1 = cpu_to_le64(prp1);
        cmd.prp2 = cpu_to_le64(prp2);
        cmd.cdw10 = cpu_to_le32(lower_32_bits(lba));
        cmd.cdw11 = cpu_to_le32(upper_32_bits(lba));
        cmd.cdw12 = cpu_to_le32((n - 1) & 0xffff);   /* 0-based */

        ret = ax_nvme_submit_cmd(ndev, &ndev->ioq, &cmd, NULL);
        if (ret)
            return ret;

        if (is_read)
            ax_nvme_cache_inv(cbuf, len);

        lba += n;
        blocks -= n;
        cbuf += len;
        dma_addr += len;
    }

    return 0;
}

int ax_nvme_read(struct ax_nvme_dev *ndev, u32 nsid, u64 lba, u32 blocks,
                 void *buf, u64 dma_addr)
{
    return ax_nvme_rw(ndev, NVME_CMD_READ, nsid, lba, blocks, buf, dma_addr);
}

int ax_nvme_write(struct ax_nvme_dev *ndev, u32 nsid, u64 lba, u32 blocks,
                  const void *buf, u64 dma_addr)
{
    return ax_nvme_rw(ndev, NVME_CMD_WRITE, nsid, lba, blocks,
                      (void *)buf, dma_addr);
}

/* ------------------------------------------------------------------------
 * Probe
 * ------------------------------------------------------------------------ */

int ax_nvme_probe(struct ax_nvme_dev *ndev,
                  u8 pcie_port, u8 bus, u8 dev, u8 func)
{
    u32 class_rev;
    u32 csts;
    u32 max_q_entries;
    int ret;

    memset(ndev, 0, sizeof(*ndev));

    ndev->pcie_port = pcie_port;
    ndev->bus = bus;
    ndev->dev = dev;
    ndev->func = func;

    /* The SSD may still answer CRS / all-ones right after link-up */
    ret = ax_pcie_wait_device_ready(pcie_port, bus, dev, func, 5000);
    if (ret)
        return ret;

    ret = ax_pcie_cfg_read(pcie_port, bus, dev, func,
                           PCI_CFG_REVISION_ID, &class_rev);
    if (ret)
        return ret;

    printf("PCI class/rev = 0x%08x\n", class_rev);

    /* 0x010802 = Mass storage / NVM / NVMe */
    if ((class_rev >> 8) != 0x010802)
        printf("Warning: class 0x%06x is not NVMe (0x010802)\n",
               class_rev >> 8);

    /* Make sure BAR0 has an address (programs all BARs if not) */
    ret = ax_nvme_get_bar0(ndev);
    if (ret == -ENOENT) {
        printf("BAR0 not assigned yet, running BAR init\n");

        ret = ax_pcie_init_ep_bars(pcie_port, bus, dev, func);
        if (ret)
            return ret;

        ret = ax_nvme_get_bar0(ndev);
    }
    if (ret) {
        printf("Cannot get NVMe BAR0: %d\n", ret);
        return ret;
    }

    /* Memory Space + Bus Master (BM is needed: the SSD DMAs the queues) */
    ret = ax_nvme_enable_pcie(ndev);
    if (ret)
        return ret;

    /* Test MMIO access */
    ret = ax_nvme_read32(ndev, NVME_REG_CSTS, &csts);
    if (ret)
        return ret;

    printf("NVMe CSTS = 0x%08x\n", csts);

    if (csts == 0xffffffff) {
        printf("MMIO read returned all ones: check outbound ATR / BAR / link\n");
        return -ENODEV;
    }

    ret = ax_nvme_read64(ndev, NVME_REG_CAP, &ndev->cap);
    if (ret)
        return ret;

    printf("NVMe CAP = 0x%016llx\n", ndev->cap);
    printf("MQES   = %u\n", NVME_CAP_MQES(ndev->cap));
    printf("DSTRD  = %u\n", (u32)NVME_CAP_DSTRD(ndev->cap));
    printf("MPSMIN = %u\n", (u32)NVME_CAP_MPSMIN(ndev->cap));

    if (!ndev->cap || ndev->cap == ~0ULL) {
        printf("Invalid CAP\n");
        return -ENODEV;
    }

    /* Doorbell stride in bytes = 4 << CAP.DSTRD */
    ndev->db_stride = 4 << NVME_CAP_DSTRD(ndev->cap);

    max_q_entries = (u32)NVME_CAP_MQES(ndev->cap) + 1;
    ndev->q_depth = min_t(u32, max_q_entries, NVME_ADMIN_Q_DEPTH);
    if (ndev->q_depth < 2)
        return -EINVAL;

    /* Controller must be disabled before programming AQA/ASQ/ACQ */
    ret = ax_nvme_disable_ctrl(ndev);
    if (ret)
        return ret;

    ret = ax_nvme_configure_admin_queue(ndev);
    if (ret)
        return ret;

    ret = ax_nvme_identify(ndev);
    if (ret)
        return ret;

    ret = ax_nvme_set_num_queues(ndev);
    if (ret)
        return ret;

    ret = ax_nvme_create_io_queues(ndev);
    if (ret)
        return ret;

    printf("NVMe controller ready\n");

    return 0;
}
