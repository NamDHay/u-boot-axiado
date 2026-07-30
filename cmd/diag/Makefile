ccflags-$(CONFIG_CMD_AX_DIAG) += \
	$(addprefix -I,$(shell find $(srctree)/cmd/diag -type d))

diag-objs := $(patsubst $(src)/%.c,%.o,\
              $(shell find $(src) -name '*.c'))

obj-$(CONFIG_CMD_AX_DIAG) += diag.o
