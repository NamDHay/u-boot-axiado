#ifndef __GPIO_H
#define __GPIO_H

int ax_gpio_request(unsigned pin, const char *label);
int ax_gpio_set_direction(unsigned pin, unsigned dir);
int ax_gpio_set_value(unsigned pin, unsigned value);
int ax_gpio_get_value(unsigned pin);
int ax_gpio_free(unsigned pin);

#endif /* __GPIO_H */
