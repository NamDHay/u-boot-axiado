// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#ifndef __EMMC_H
#define __EMMC_H

#include <linux/types.h>

/* @EMMC_SPEED_MODE */
typedef enum {
	EMMC_MODE_NORMAL = 0, // Default speed (up to 25 MHz)
	EMMC_MODE_HIGH_SPEED, // High Speed (up to 50 MHz)
	EMMC_MODE_HS200, // HS200 (up to 200 MHz SDR)
	EMMC_MODE_HS400, // HS400 (up to 400 MHz DDR)
	EMMC_MODE_DDR50
} emmc_speed_mode_t;

/* @BUS_WIDTH */
typedef enum {
	EMMC_BUS_1BIT = 0, // 1-bit bus width
	EMMC_BUS_4BIT, // 4-bit bus width
	EMMC_BUS_8BIT, // 8-bit bus width
	EMMC_BUS_4BIT_DDR, // 4-bit bus width, dual data rate
	EMMC_BUS_8BIT_DDR // 8-bit bus width, dual data rate
} emmc_bus_width_t;

struct ax_emmc_ops {
	/* Controller initialization */
	int (*init)(void);

	/* Controller reset */
	int (*reset)(void);

	/* Set bus clock frequency */
	int (*set_clock)(uint32_t hz);

	/* Set bus width: 1, 4 or 8 bits */
	int (*set_bus_width)(uint32_t width);

	/* Read blocks from eMMC */
	int (*read)(uint64_t start, uint32_t blkcnt, void *dst);

	/* Write blocks to eMMC */
	int (*write)(uint64_t start, uint32_t blkcnt, const void *src);

	/* Erase blocks */
	int (*erase)(uint64_t start, uint32_t blkcnt);

	/* Get block size */
	uint32_t (*get_block_size)(void);

	/* Get total number of blocks */
	uint64_t (*get_block_count)(void);
};

extern struct ax_emmc_ops emmc;

static inline int ax_emmc_init(void)
{
	if (emmc.init == NULL)
		return -ENOSYS;

	return emmc.init();
}

static inline int ax_emmc_reset(void)
{
	if (emmc.reset == NULL)
		return -ENOSYS;

	return emmc.reset();
}

static inline int ax_emmc_set_clock(uint32_t hz)
{
	if (emmc.set_clock == NULL)
		return -ENOSYS;

	return emmc.set_clock(hz);
}

static inline int ax_emmc_set_bus_width(uint32_t width)
{
	if (emmc.set_bus_width == NULL)
		return -ENOSYS;

	return emmc.set_bus_width(width);
}

static inline int ax_emmc_read(uint64_t start, uint32_t blkcnt,
			       void *dst)
{
	if (emmc.read == NULL)
		return -ENOSYS;

	return emmc.read(start, blkcnt, dst);
}

static inline int ax_emmc_write(uint64_t start, uint32_t blkcnt,
				const void *src)
{
	if (emmc.write == NULL)
		return -ENOSYS;

	return emmc.write(start, blkcnt, src);
}

static inline int ax_emmc_erase(uint64_t start, uint32_t blkcnt)
{
	if (emmc.erase == NULL)
		return -ENOSYS;

	return emmc.erase(start, blkcnt);
}

static inline uint32_t ax_emmc_get_block_size(void)
{
	if (emmc.get_block_size == NULL)
		return 0;

	return emmc.get_block_size();
}

static inline uint64_t ax_emmc_get_block_count(void)
{
	if (emmc.get_block_count == NULL)
		return 0;

	return emmc.get_block_count();
}

#endif /* __EMMC_H */
