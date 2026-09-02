ccflags-$(CONFIG_CMD_AX_DIAG) += \
	$(addprefix -I,$(shell find $(srctree)/cmd/diag -type d))

obj-$(CONFIG_CMD_AX_ADC) 	+= adc/adc_cli.o

obj-$(CONFIG_CMD_AX_GPIO) 	+= gpio/gpio_cli.o

obj-$(CONFIG_CMD_AX_I2C) 	+= i2c/i2c_cli.o
obj-$(CONFIG_CMD_AX_I3C) 	+= i3c/i3c_cli.o

obj-$(CONFIG_CMD_AX_MEM) 	+= mem/mem_cli.o

obj-$(CONFIG_CMD_AX_PCIE) 	+= pcie/pcie_cli.o
obj-$(CONFIG_CMD_AX_NVME) 	+= nvme/nvme_cli.o

obj-$(CONFIG_CMD_AX_PWM) 	+= pwm/pwm_cli.o

obj-$(CONFIG_CMD_AX_RTC) 	+= rtc/rtc_cli.o

obj-$(CONFIG_CMD_AX_UART) 	+= uart/uart_cli.o
