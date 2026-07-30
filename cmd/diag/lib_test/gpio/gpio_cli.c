#include <command.h>
#include <dm.h>
#include <errno.h>
#include <linux/printk.h>
#include "diag.h"

#include "gpio_cli.h"
#include "gpio.h"

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
