// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy namhuyngn03@gmail.com
 */

#ifndef __I2C_H
#define __I2C_H

struct ax_hdmi_ops {
    /* Basic */
    int (*reset)(void);
    int (*init)(void);
    int (*deinit)(void);

    /* Status */
    int (*get_hpd)(bool *connected);
    int (*get_status)(u32 *status);

    /* Clock / PHY */
    int (*clk_enable)(void);
    int (*clk_disable)(void);

    int (*phy_init)(void);
    int (*phy_power_on)(void);
    int (*phy_power_off)(void);
    int (*phy_get_lock)(bool *locked);

    /* Video */
    int (*set_mode)(u32 width, u32 height, u32 refresh);
    int (*video_enable)(void);
    int (*video_disable)(void);

    /* Test */
    int (*bist)(void);
    int (*vbist)(void);
};

extern struct ax_hdmi_ops hdmi;

static inline int ax_hdmi_reset(void) {  
    if (hdmi.reset == NULL) return -ENOSYS;

    return hdmi.reset(); 
}

static inline int ax_hdmi_bist(void) {  
    if (hdmi.bist == NULL) return -ENOSYS;

    return hdmi.bist(); 
}

static inline int ax_hdmi_vbist(void) {  
    if (hdmi.vbist == NULL) return -ENOSYS;

    return hdmi.vbist(); 
}

#endif /* __I2C_H */
