#ifndef __CDNS_I3C_H
#define __CDNS_I3C_H

#define DEV_ID				0x0
#define DEV_ID_I3C_MASTER		0x5034

#define CONF_STATUS0			0x4
#define CONF_STATUS0_CMDR_DEPTH(x)	(4 << (((x) & GENMASK(31, 29)) >> 29))
#define CONF_STATUS0_ECC_CHK		BIT(28)
#define CONF_STATUS0_INTEG_CHK		BIT(27)
#define CONF_STATUS0_CSR_DAP_CHK	BIT(26)
#define CONF_STATUS0_TRANS_TOUT_CHK	BIT(25)
#define CONF_STATUS0_PROT_FAULTS_CHK	BIT(24)
#define CONF_STATUS0_GPO_NUM(x)		(((x) & GENMASK(23, 16)) >> 16)
#define CONF_STATUS0_GPI_NUM(x)		(((x) & GENMASK(15, 8)) >> 8)
#define CONF_STATUS0_IBIR_DEPTH(x)	(4 << (((x) & GENMASK(7, 6)) >> 7))
#define CONF_STATUS0_SUPPORTS_DDR	BIT(5)
#define CONF_STATUS0_SEC_MASTER		BIT(4)
#define CONF_STATUS0_DEVS_NUM(x)	((x) & GENMASK(3, 0))

#define CONF_STATUS1			0x8
#define CONF_STATUS1_IBI_HW_RES(x)	((((x) & GENMASK(31, 28)) >> 28) + 1)
#define CONF_STATUS1_CMD_DEPTH(x)	(4 << (((x) & GENMASK(27, 26)) >> 26))
#define CONF_STATUS1_SLVDDR_RX_DEPTH(x)	(8 << (((x) & GENMASK(25, 21)) >> 21))
#define CONF_STATUS1_SLVDDR_TX_DEPTH(x)	(8 << (((x) & GENMASK(20, 16)) >> 16))
#define CONF_STATUS1_IBI_DEPTH(x)	(2 << (((x) & GENMASK(12, 10)) >> 10))
#define CONF_STATUS1_RX_DEPTH(x)	(8 << (((x) & GENMASK(9, 5)) >> 5))
#define CONF_STATUS1_TX_DEPTH(x)	(8 << ((x) & GENMASK(4, 0)))

#define REV_ID				0xc
#define REV_ID_VID(id)			(((id) & GENMASK(31, 20)) >> 20)
#define REV_ID_PID(id)			(((id) & GENMASK(19, 8)) >> 8)
#define REV_ID_REV_MAJOR(id)		(((id) & GENMASK(7, 4)) >> 4)
#define REV_ID_REV_MINOR(id)		((id) & GENMASK(3, 0))

#define CTRL				0x10
#define CTRL_DEV_EN			BIT(31)
#define CTRL_HALT_EN			BIT(30)
#define CTRL_MCS			BIT(29)
#define CTRL_MCS_EN			BIT(28)
#define CTRL_THD_DELAY(x)		(((x) << 24) & GENMASK(25, 24))
#define CTRL_HJ_DISEC			BIT(8)
#define CTRL_MST_ACK			BIT(7)
#define CTRL_HJ_ACK			BIT(6)
#define CTRL_HJ_INIT			BIT(5)
#define CTRL_MST_INIT			BIT(4)
#define CTRL_AHDR_OPT			BIT(3)
#define CTRL_PURE_BUS_MODE		0
#define CTRL_MIXED_FAST_BUS_MODE	2
#define CTRL_MIXED_SLOW_BUS_MODE	3
#define CTRL_BUS_MODE_MASK		GENMASK(1, 0)
#define THD_DELAY_MAX			3

#define PRESCL_CTRL0			0x14
#define PRESCL_CTRL0_I2C(x)		((x) << 16)
#define PRESCL_CTRL0_I3C(x)		(x)
#define PRESCL_CTRL0_I3C_MAX		GENMASK(9, 0)
#define PRESCL_CTRL0_I2C_MAX		GENMASK(15, 0)

#define PRESCL_CTRL1			0x18
#define PRESCL_CTRL1_PP_LOW_MASK	GENMASK(15, 8)
#define PRESCL_CTRL1_PP_LOW(x)		((x) << 8)
#define PRESCL_CTRL1_OD_LOW_MASK	GENMASK(7, 0)
#define PRESCL_CTRL1_OD_LOW(x)		(x)

#define MST_IER				0x20
#define MST_IDR				0x24
#define MST_IMR				0x28
#define MST_ICR				0x2c
#define MST_ISR				0x30
#define MST_INT_HALTED			BIT(18)
#define MST_INT_MR_DONE			BIT(17)
#define MST_INT_IMM_COMP		BIT(16)
#define MST_INT_TX_THR			BIT(15)
#define MST_INT_TX_OVF			BIT(14)
#define MST_INT_IBID_THR		BIT(12)
#define MST_INT_IBID_UNF		BIT(11)
#define MST_INT_IBIR_THR		BIT(10)
#define MST_INT_IBIR_UNF		BIT(9)
#define MST_INT_IBIR_OVF		BIT(8)
#define MST_INT_RX_THR			BIT(7)
#define MST_INT_RX_UNF			BIT(6)
#define MST_INT_CMDD_EMP		BIT(5)
#define MST_INT_CMDD_THR		BIT(4)
#define MST_INT_CMDD_OVF		BIT(3)
#define MST_INT_CMDR_THR		BIT(2)
#define MST_INT_CMDR_UNF		BIT(1)
#define MST_INT_CMDR_OVF		BIT(0)

#define MST_STATUS0			0x34
#define MST_STATUS0_IDLE		BIT(18)
#define MST_STATUS0_HALTED		BIT(17)
#define MST_STATUS0_MASTER_MODE		BIT(16)
#define MST_STATUS0_TX_FULL		BIT(13)
#define MST_STATUS0_IBID_FULL		BIT(12)
#define MST_STATUS0_IBIR_FULL		BIT(11)
#define MST_STATUS0_RX_FULL		BIT(10)
#define MST_STATUS0_CMDD_FULL		BIT(9)
#define MST_STATUS0_CMDR_FULL		BIT(8)
#define MST_STATUS0_TX_EMP		BIT(5)
#define MST_STATUS0_IBID_EMP		BIT(4)
#define MST_STATUS0_IBIR_EMP		BIT(3)
#define MST_STATUS0_RX_EMP		BIT(2)
#define MST_STATUS0_CMDD_EMP		BIT(1)
#define MST_STATUS0_CMDR_EMP		BIT(0)

#define CMDR				0x38
#define CMDR_NO_ERROR			0
#define CMDR_DDR_PREAMBLE_ERROR		1
#define CMDR_DDR_PARITY_ERROR		2
#define CMDR_DDR_RX_FIFO_OVF		3
#define CMDR_DDR_TX_FIFO_UNF		4
#define CMDR_M0_ERROR			5
#define CMDR_M1_ERROR			6
#define CMDR_M2_ERROR			7
#define CMDR_MST_ABORT			8
#define CMDR_NACK_RESP			9
#define CMDR_INVALID_DA			10
#define CMDR_DDR_DROPPED		11
#define CMDR_ERROR(x)			(((x) & GENMASK(27, 24)) >> 24)
#define CMDR_XFER_BYTES(x)		(((x) & GENMASK(19, 8)) >> 8)
#define CMDR_CMDID_HJACK_DISEC		0xfe
#define CMDR_CMDID_HJACK_ENTDAA		0xff
#define CMDR_CMDID(x)			((x) & GENMASK(7, 0))

#define IBIR				0x3c
#define IBIR_ACKED			BIT(12)
#define IBIR_SLVID(x)			(((x) & GENMASK(11, 8)) >> 8)
#define IBIR_ERROR			BIT(7)
#define IBIR_XFER_BYTES(x)		(((x) & GENMASK(6, 2)) >> 2)
#define IBIR_TYPE_IBI			0
#define IBIR_TYPE_HJ			1
#define IBIR_TYPE_MR			2
#define IBIR_TYPE(x)			((x) & GENMASK(1, 0))

#define SLV_IER				0x40
#define SLV_IDR				0x44
#define SLV_IMR				0x48
#define SLV_ICR				0x4c
#define SLV_ISR				0x50
#define SLV_INT_TM			BIT(20)
#define SLV_INT_ERROR			BIT(19)
#define SLV_INT_EVENT_UP		BIT(18)
#define SLV_INT_HJ_DONE			BIT(17)
#define SLV_INT_MR_DONE			BIT(16)
#define SLV_INT_DA_UPD			BIT(15)
#define SLV_INT_SDR_FAIL		BIT(14)
#define SLV_INT_DDR_FAIL		BIT(13)
#define SLV_INT_M_RD_ABORT		BIT(12)
#define SLV_INT_DDR_RX_THR		BIT(11)
#define SLV_INT_DDR_TX_THR		BIT(10)
#define SLV_INT_SDR_RX_THR		BIT(9)
#define SLV_INT_SDR_TX_THR		BIT(8)
#define SLV_INT_DDR_RX_UNF		BIT(7)
#define SLV_INT_DDR_TX_OVF		BIT(6)
#define SLV_INT_SDR_RX_UNF		BIT(5)
#define SLV_INT_SDR_TX_OVF		BIT(4)
#define SLV_INT_DDR_RD_COMP		BIT(3)
#define SLV_INT_DDR_WR_COMP		BIT(2)
#define SLV_INT_SDR_RD_COMP		BIT(1)
#define SLV_INT_SDR_WR_COMP		BIT(0)

#define SLV_STATUS0			0x54
#define SLV_STATUS0_REG_ADDR(s)		(((s) & GENMASK(23, 16)) >> 16)
#define SLV_STATUS0_XFRD_BYTES(s)	((s) & GENMASK(15, 0))

#define SLV_STATUS1			0x58
#define SLV_STATUS1_AS(s)		(((s) & GENMASK(21, 20)) >> 20)
#define SLV_STATUS1_VEN_TM		BIT(19)
#define SLV_STATUS1_HJ_DIS		BIT(18)
#define SLV_STATUS1_MR_DIS		BIT(17)
#define SLV_STATUS1_PROT_ERR		BIT(16)
#define SLV_STATUS1_DA(s)		(((s) & GENMASK(15, 9)) >> 9)
#define SLV_STATUS1_HAS_DA		BIT(8)
#define SLV_STATUS1_DDR_RX_FULL		BIT(7)
#define SLV_STATUS1_DDR_TX_FULL		BIT(6)
#define SLV_STATUS1_DDR_RX_EMPTY	BIT(5)
#define SLV_STATUS1_DDR_TX_EMPTY	BIT(4)
#define SLV_STATUS1_SDR_RX_FULL		BIT(3)
#define SLV_STATUS1_SDR_TX_FULL		BIT(2)
#define SLV_STATUS1_SDR_RX_EMPTY	BIT(1)
#define SLV_STATUS1_SDR_TX_EMPTY	BIT(0)

#define CMD0_FIFO			0x60
#define CMD0_FIFO_IS_DDR		BIT(31)
#define CMD0_FIFO_IS_CCC		BIT(30)
#define CMD0_FIFO_BCH			BIT(29)
#define XMIT_BURST_STATIC_SUBADDR	0
#define XMIT_SINGLE_INC_SUBADDR		1
#define XMIT_SINGLE_STATIC_SUBADDR	2
#define XMIT_BURST_WITHOUT_SUBADDR	3
#define CMD0_FIFO_PRIV_XMIT_MODE(m)	((m) << 27)
#define CMD0_FIFO_SBCA			BIT(26)
#define CMD0_FIFO_RSBC			BIT(25)
#define CMD0_FIFO_IS_10B		BIT(24)
#define CMD0_FIFO_PL_LEN(l)		((l) << 12)
#define CMD0_FIFO_PL_LEN_MAX		4095
#define CMD0_FIFO_DEV_ADDR(a)		((a) << 1)
#define CMD0_FIFO_RNW			BIT(0)

#define CMD1_FIFO			0x64
#define CMD1_FIFO_CMDID(id)		((id) << 24)
#define CMD1_FIFO_CSRADDR(a)		(a)
#define CMD1_FIFO_CCC(id)		(id)

#define TX_FIFO				0x68

#define IMD_CMD0			0x70
#define IMD_CMD0_PL_LEN(l)		((l) << 12)
#define IMD_CMD0_DEV_ADDR(a)		((a) << 1)
#define IMD_CMD0_RNW			BIT(0)

#define IMD_CMD1			0x74
#define IMD_CMD1_CCC(id)		(id)

#define IMD_DATA			0x78
#define RX_FIFO				0x80
#define IBI_DATA_FIFO			0x84
#define SLV_DDR_TX_FIFO			0x88
#define SLV_DDR_RX_FIFO			0x8c

#define CMD_IBI_THR_CTRL		0x90
#define IBIR_THR(t)			((t) << 24)
#define CMDR_THR(t)			((t) << 16)
#define IBI_THR(t)			((t) << 8)
#define CMD_THR(t)			(t)

#define TX_RX_THR_CTRL			0x94
#define RX_THR(t)			((t) << 16)
#define TX_THR(t)			(t)

#define SLV_DDR_TX_RX_THR_CTRL		0x98
#define SLV_DDR_RX_THR(t)		((t) << 16)
#define SLV_DDR_TX_THR(t)		(t)

#define FLUSH_CTRL			0x9c
#define FLUSH_IBI_RESP			BIT(23)
#define FLUSH_CMD_RESP			BIT(22)
#define FLUSH_SLV_DDR_RX_FIFO		BIT(22)
#define FLUSH_SLV_DDR_TX_FIFO		BIT(21)
#define FLUSH_IMM_FIFO			BIT(20)
#define FLUSH_IBI_FIFO			BIT(19)
#define FLUSH_RX_FIFO			BIT(18)
#define FLUSH_TX_FIFO			BIT(17)
#define FLUSH_CMD_FIFO			BIT(16)

#define TTO_PRESCL_CTRL0		0xb0
#define TTO_PRESCL_CTRL0_DIVB(x)	((x) << 16)
#define TTO_PRESCL_CTRL0_DIVA(x)	(x)

#define TTO_PRESCL_CTRL1		0xb4
#define TTO_PRESCL_CTRL1_DIVB(x)	((x) << 16)
#define TTO_PRESCL_CTRL1_DIVA(x)	(x)

#define DEVS_CTRL			0xb8
#define DEVS_CTRL_DEV_CLR_SHIFT		16
#define DEVS_CTRL_DEV_CLR_ALL		GENMASK(31, 16)
#define DEVS_CTRL_DEV_CLR(dev)		BIT(16 + (dev))
#define DEVS_CTRL_DEV_ACTIVE(dev)	BIT(dev)
#define DEVS_CTRL_DEVS_ACTIVE_MASK	GENMASK(15, 0)
#define MAX_DEVS			16

#define DEV_ID_RR0(d)			(0xc0 + ((d) * 0x10))
#define DEV_ID_RR0_LVR_EXT_ADDR		BIT(11)
#define DEV_ID_RR0_HDR_CAP		BIT(10)
#define DEV_ID_RR0_IS_I3C		BIT(9)
#define DEV_ID_RR0_DEV_ADDR_MASK	(GENMASK(6, 0) | GENMASK(15, 13))
#define DEV_ID_RR0_SET_DEV_ADDR(a)	(((a) & GENMASK(6, 0)) |	\
					 (((a) & GENMASK(9, 7)) << 6))
#define DEV_ID_RR0_GET_DEV_ADDR(x)	((((x) >> 1) & GENMASK(6, 0)) |	\
					 (((x) >> 6) & GENMASK(9, 7)))

#define DEV_ID_RR1(d)			(0xc4 + ((d) * 0x10))
#define DEV_ID_RR1_PID_MSB(pid)		(pid)

#define DEV_ID_RR2(d)			(0xc8 + ((d) * 0x10))
#define DEV_ID_RR2_PID_LSB(pid)		((pid) << 16)
#define DEV_ID_RR2_BCR(bcr)		((bcr) << 8)
#define DEV_ID_RR2_DCR(dcr)		(dcr)
#define DEV_ID_RR2_LVR(lvr)		(lvr)

#define SIR_MAP(x)			(0x180 + ((x) * 4))
#define SIR_MAP_DEV_REG(d)		SIR_MAP((d) / 2)
#define SIR_MAP_DEV_SHIFT(d, fs)	((fs) + (((d) % 2) ? 16 : 0))
#define SIR_MAP_DEV_CONF_MASK(d)	(GENMASK(15, 0) << (((d) % 2) ? 16 : 0))
#define SIR_MAP_DEV_CONF(d, c)		((c) << (((d) % 2) ? 16 : 0))
#define DEV_ROLE_SLAVE			0
#define DEV_ROLE_MASTER			1
#define SIR_MAP_DEV_ROLE(role)		((role) << 14)
#define SIR_MAP_DEV_SLOW		BIT(13)
#define SIR_MAP_DEV_PL(l)		((l) << 8)
#define SIR_MAP_PL_MAX			GENMASK(4, 0)
#define SIR_MAP_DEV_DA(a)		((a) << 1)
#define SIR_MAP_DEV_ACK			BIT(0)

#define GPIR_WORD(x)			(0x200 + ((x) * 4))
#define GPI_REG(val, id)		\
	(((val) >> (((id) % 4) * 8)) & GENMASK(7, 0))

#define GPOR_WORD(x)			(0x220 + ((x) * 4))
#define GPO_REG(val, id)		\
	(((val) >> (((id) % 4) * 8)) & GENMASK(7, 0))

#define ASF_INT_STATUS			0x300
#define ASF_INT_RAW_STATUS		0x304
#define ASF_INT_MASK			0x308
#define ASF_INT_TEST			0x30c
#define ASF_INT_FATAL_SELECT		0x310
#define ASF_INTEGRITY_ERR		BIT(6)
#define ASF_PROTOCOL_ERR		BIT(5)
#define ASF_TRANS_TIMEOUT_ERR		BIT(4)
#define ASF_CSR_ERR			BIT(3)
#define ASF_DAP_ERR			BIT(2)
#define ASF_SRAM_UNCORR_ERR		BIT(1)
#define ASF_SRAM_CORR_ERR		BIT(0)

#define ASF_SRAM_CORR_FAULT_STATUS	0x320
#define ASF_SRAM_UNCORR_FAULT_STATUS	0x324
#define ASF_SRAM_CORR_FAULT_INSTANCE(x)	((x) >> 24)
#define ASF_SRAM_CORR_FAULT_ADDR(x)	((x) & GENMASK(23, 0))

#define ASF_SRAM_FAULT_STATS		0x328
#define ASF_SRAM_FAULT_UNCORR_STATS(x)	((x) >> 16)
#define ASF_SRAM_FAULT_CORR_STATS(x)	((x) & GENMASK(15, 0))

#define ASF_TRANS_TOUT_CTRL		0x330
#define ASF_TRANS_TOUT_EN		BIT(31)
#define ASF_TRANS_TOUT_VAL(x)	(x)

#define ASF_TRANS_TOUT_FAULT_MASK	0x334
#define ASF_TRANS_TOUT_FAULT_STATUS	0x338
#define ASF_TRANS_TOUT_FAULT_APB	BIT(3)
#define ASF_TRANS_TOUT_FAULT_SCL_LOW	BIT(2)
#define ASF_TRANS_TOUT_FAULT_SCL_HIGH	BIT(1)
#define ASF_TRANS_TOUT_FAULT_FSCL_HIGH	BIT(0)

#define ASF_PROTO_FAULT_MASK		0x340
#define ASF_PROTO_FAULT_STATUS		0x344
#define ASF_PROTO_FAULT_SLVSDR_RD_ABORT	BIT(31)
#define ASF_PROTO_FAULT_SLVDDR_FAIL	BIT(30)
#define ASF_PROTO_FAULT_S(x)		BIT(16 + (x))
#define ASF_PROTO_FAULT_MSTSDR_RD_ABORT	BIT(15)
#define ASF_PROTO_FAULT_MSTDDR_FAIL	BIT(14)
#define ASF_PROTO_FAULT_M(x)		BIT(x)

#define CDNS_I3C_QUIRKS_DISABLE_WARN_ON_INVALID_ID BIT(0)
#define CDNS_I3C_QUIRKS_ENABLE_REPEATED_START BIT(1)
#define CDNS_I3C_QUIRKS_DISABLE_IBIR_HANDLING BIT(2)
#define CDNS_I3C_QUIRKS_DISABLE_IBIR_ERROR BIT(3)
#define CDNS_I3C_QUIRKS_DISABLE_CTRL_HALT BIT(4)
#define CDNS_I3C_QUIRKS_DISABLE_MST_ABORT BIT(5)
#define CDNS_I3C_QUIRKS_SKIP_PAYLOAD_INC BIT(6)

#define MST_IDR_SET 0xFFFFFFFF /**< Master mode Interrupt disable value */
#define SLV_IDR_SET 0xFFFFFFFF /**< Slave mode Interrupt disable value */
#define CMD_IBI_THR_CTRL_SET \
	0x01000000 /**< Threshold configuration value for Command Queue. */
#define MST_IER_ENABLE 0xFFFFFFFF /**< Master mode Interrupt enable value */

#endif /* __CDNS_I3C_H */
