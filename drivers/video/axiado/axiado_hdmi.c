// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2025 Axiado Corporation.
 *
 * Based on inno_hdmi.c
 *   Copyright (c) Fuzhou Rockchip Electronics Co. Ltd.
 *     Zheng Yang <zhengyang@rock-chips.com>
 *     Yakir Yang <ykk@rock-chips.com>
 */

#include <clk.h>
#include <display.h>
#include <dm.h>
#include <dw_hdmi.h>
#include <edid.h>
#include <log.h>
#include <malloc.h>
#include <regmap.h>
#include <syscon.h>
#include <asm/gpio.h>
#include <power/regulator.h>
#include "axiado_hdmi.h"

static int axiado_hdmi_probe(struct udevice *dev)
{
    struct axiado_hdmi *hdmi = dev_get_priv(dev);
    ofnode np = dev_ofnode(dev);
    int ret;

    hdmi = malloc(sizeof(struct axiado_hdmi));
    return ret;
}


static const struct dm_display_ops axiado_hdmi_ops = {
	/* .read_edid = axiado_hdmi_read_edid, */
	/* .enable = axiado_hdmi_enable, */
};

static const struct udevice_id axiado_hdmi_ids[] = {
	{
		.compatible = "axiado,ax3000-hdmi",
	},
	{},
};

U_BOOT_DRIVER(axiado_hdmi) = {
	.name = "axiado-hdmi",
	.id = UCLASS_DISPLAY,
	.of_match = axiado_hdmi_ids,
	.ops = &axiado_hdmi_ops,
	.probe = axiado_hdmi_probe,
	.priv_auto	= sizeof(struct axiado_hdmi),
};

