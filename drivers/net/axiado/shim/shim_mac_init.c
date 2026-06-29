// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * shim_mac_init.c - Initialization routines for the Axiado SHIM driver.
 * Configures MACs and flow control.
 *
 * Copyright (c) 2022-2026 Axiado Corporation
 */

#include <miiphy.h>
#include <net.h>
#include <asm/io.h>

#include "mac_config.h"
#include "shim_mac.h"
#include "shim_platform.h"

/**
 * shim_mac_init - Master function for initializing all enabled MACs and SHIM.
 * @dev: The device structure for logging.
 *
 * This function orchestrates the initialization sequence:
 * 1. Basic initialization of all enabled MACs.
 * 2. Flow control configuration.
 *
 * Return: SHIM_STATUS_SUCCESS on success, or a negative error code on failure.
 */
enum AX_SHIM_STATUS shim_mac_init(void)
{
	enum AX_SHIM_STATUS ret;

	printf("Initializing shim and mac configuration\n");

	/* 1. Initialization of all MACs */
	ret = mac_basic_init();
	if (ret != SHIM_STATUS_SUCCESS) {
		printf("MAC initialization failed. ret: %d\n", ret);
		return ret;
	}
	printf("MAC basic init done\n");

	/* 2. MAC flow config */
	ret = mac_flow_config();
	if (ret != SHIM_STATUS_SUCCESS) {
		printf("MAC flow config failed. ret: %d\n", ret);
		return ret;
	}
	printf("Flow config done\n");

	printf("SHIM and MACs initialized successfully\n");

	return SHIM_STATUS_SUCCESS;
}
