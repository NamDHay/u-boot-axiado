#include <command.h>
#include <dm.h>
#include <errno.h>
#include <linux/printk.h>

#include "ax_diag.h"
#include "ax_gpio.h"
#include "gpio_cli.h"

int gpio_diag_init(void) {
    static int is_init = 0;
    if (is_init)
        goto end;

end:
    is_init = 1;
    return 0;
};

int gpio_diag_test(unsigned int testid) {
    int ret;
    switch (testid) {
        case TOGGLE:
        case LOOPBACK:
            ret = 0;
            break;
        default:
            pr_err("testid:%d not found\n", testid);
            ret = -ESRCH;
            break;
    }

    return ret;
}

void gpio_diag_stat(void) {
    ;
}

int do_ax_gpio(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[]) 
{
    unsigned int gpio;
    int value;
    int ret;
    char *endp;

    if (argc < 2)
        return CMD_RET_USAGE;

    gpio = simple_strtol(argv[2], &endp, 10);

    ret = ax_gpio_request(gpio, "bypass");
    if (ret) {
        printf("GPIO: '%s' request failed\n", argv[2]);
        return -EINVAL;
    }

    if (!strncmp(argv[1], "i", 1)) {
        ax_gpio_set_direction(gpio, 1);
        value = ax_gpio_get_value(gpio);
    } else {
        ax_gpio_set_direction(gpio, 0);

        if (!strncmp(argv[1], "s", 1)) {
            value = 1;
        } else if (!strncmp(argv[1], "c", 1)) {
            value = 0;
        }
        ax_gpio_set_value(gpio, value);
    }

    if (!strncmp(argv[1], "t", 1)) {
        ax_gpio_set_direction(gpio, 1);
        value = ax_gpio_get_value(gpio);
        ax_gpio_set_direction(gpio, 0);
        ax_gpio_set_value(gpio, (value = value ? 0 : 1));
    }
	printf("gpio: pin %d value is %d\n", gpio, value);

    return 0;
}

U_BOOT_CMD(
        ax_gpio,	CONFIG_SYS_MAXARGS,	0,	do_ax_gpio,
        "perform axiado gpio diagnostics",
        "<input|set|clear|toggle> <pin>\n"
        );
