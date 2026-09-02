#include <command.h>
#include <dm.h>
#include <errno.h>
#include <linux/printk.h>

#include "ax_pcie.h"
#include "ax_nvme.h"

/*
 * ax_nvme init <port>
 */
static int ax_nvme_cmd_init(struct cmd_tbl *cmdtp, int flag,
        int argc, char *const argv[])
{
    static struct ax_nvme_dev ndev;
    int ret;
    unsigned int port;
    char *endp;

    port = simple_strtol(argv[1], &endp, 16);

    ret = ax_nvme_probe(&ndev,
                        port,      /* PCIe port */
                        1,      /* bus */
                        0,      /* device */
                        0);     /* function */
    if (ret) {
        printf("NVME_%d init failed: %d\n", port, ret);
        return CMD_RET_FAILURE;
    }

    printf("NVME_%d init successed\n", port);

    return CMD_RET_SUCCESS;
}

int do_ax_nvme(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[]) 
{
    if (argc < 3)
        return CMD_RET_USAGE;

    if (!strcmp(argv[1], "init"))
        return ax_nvme_cmd_init(cmdtp, flag, argc - 1, &argv[1]);

    return 0;
}

U_BOOT_CMD(
        ax_nvme,
        8,
        1,
        do_ax_nvme,
        "Axiado PCIe diagnostic command",
        "init <port>\n"
        "    - Initialize NVME controller\n"
        "\n"
);
