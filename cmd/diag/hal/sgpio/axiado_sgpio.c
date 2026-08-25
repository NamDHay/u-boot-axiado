// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy namhuyngn03@gmail.com
 */

#include <dm.h>
#include <asm/io.h>
#include <stdio.h>

#include "ax3000_base_adrs.h"
#include "ax3000_slo_base_adrs.h"

#include "ax_sgpio.h"
#include "ax_pinmux.h"

static int axiado_sgpio_request(u8 bus, u16 total_pin) {  
    return 0;
}

static int axiado_sgpio_set_value(u16 pin, u8 value) {
    return 0;
}

static int axiado_sgpio_get_value(u16 pin) {
    return 0;
}

struct ax_sgpio_ops sgpio = {
    .request = axiado_sgpio_request,
    .set_value = axiado_sgpio_set_value,
    .get_value = axiado_sgpio_get_value,
};
