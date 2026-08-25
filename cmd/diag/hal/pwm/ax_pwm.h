// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy namhuyngn03@gmail.com
 */

#ifndef __PWM_H
#define __PWM_H

struct ax_pwm_ops {
    /**
	 * set_config() - Set the PWM configuration
	 *
	 * Change both the PWM device's period and it's duty period if
	 * possible. Otherwise, set an appropriate duty period that best
	 * matches the given period_ns / duty_ns ratio for the device.
	 *
	 * @channel:	PWM channel to update
	 * @period_ns:	PWM period in nanoseconds
	 * @duty_ns:	PWM duty period in nanoseconds
	 * @return 0 if OK, -ve on error
	 */
	int (*set_config)(uint channel, uint period_ns, uint duty_ns);

	/**
	 * set_enable() - Enable or disable the PWM
	 *
	 * @channel:	PWM channel to update
	 * @enable:	true to enable, false to disable
	 * @return 0 if OK, -ve on error
	 */
	int (*set_enable)(uint channel, bool enable);
	/**
	 * set_invert() - Set the PWM invert
	 *
	 * @channel:    PWM channel to update
	 * @polarity:   true to invert, false to keep normal polarity
	 * @return 0 if OK, -ve on error
	 */
	int (*set_invert)(uint channel, bool polarity);
};

extern struct ax_pwm_ops pwm;

/**
 * pwm_set_config() - Set the PWM configuration
 *
 * Change both the PWM device's period and it's duty period if
 * possible. Otherwise, set an appropriate duty period that best
 * matches the given period_ns / duty_ns ratio for the device.
 *
 * @channel:	PWM channel to update
 * @period_ns:	PWM period in nanoseconds
 * @duty_ns:	PWM duty period in nanoseconds
 * Return: 0 if OK, -ve on error
 */
static inline int ax_pwm_set_config(uint channel, uint period_ns,
		   uint duty_ns) {
    if (pwm.set_config == NULL) return -ENOSYS;

    return pwm.set_config(channel, period_ns, duty_ns);
}

/**
 * pwm_set_enable() - Enable or disable the PWM
 *
 * @channel:	PWM channel to update
 * @enable:	true to enable, false to disable
 * Return: 0 if OK, -ve on error
 */
static inline int ax_pwm_set_enable(uint channel, bool enable) {
    if (pwm.set_enable == NULL) return -ENOSYS;

    return pwm.set_enable(channel, enable);
}

/**
 * pwm_set_invert() - Set pwm default polarity
 *
 * @channel:	PWM channel to update
 * @polarity:	true to invert, false to keep normal polarity
 * Return: 0 if OK, -ve on error
 */
static inline int ax_pwm_set_invert(uint channel, bool polarity) {
    if (pwm.set_invert == NULL) return -ENOSYS;

    return pwm.set_invert(channel, polarity);
}

#endif /* __PWM_H */

