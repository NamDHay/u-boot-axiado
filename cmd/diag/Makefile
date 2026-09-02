ccflags-$(CONFIG_CMD_AX_DIAG) += \
	$(addprefix -I,$(shell find $(srctree)/cmd/diag -type d))

obj-$(CONFIG_CMD_AX_DIAG) += diag_common.o

obj-$(CONFIG_CMD_AX_DIAG) += hal/
obj-$(CONFIG_CMD_AX_DIAG) += lib_test/
