ccflags-$(CONFIG_CMD_AX_DIAG) += \
	$(addprefix -I,$(shell find $(srctree)/cmd/diag -type d))

obj-$(CONFIG_INNO_ADC) 		+= adc/inno_saradc.o

obj-$(CONFIG_CDNS_GPIO) 	+= gpio/cdns_gpio.o

obj-$(CONFIG_CDNS_I2C) 	+= i2c/cdns_i2c.o
obj-$(CONFIG_CDNS_I3C) 	+= i3c/cdns_i3c.o

obj-$(CONFIG_CMD_AX_PCIE) 	+= pcie/ax_pcie.o
obj-$(CONFIG_INNO_PCIE) 	+= pcie/axiado_pcie.o

obj-$(CONFIG_CMD_AX_NVME) 	+= nvme/ax_nvme.o

obj-$(CONFIG_AXIADO_PINMUX) 	+= pinmux/axiado_pinmux.o

obj-$(CONFIG_CDNS_PWM) 	+= pwm/cdns_pwm.o

obj-$(CONFIG_CDNS_RTC) 	+= rtc/cdns_rtc.o

obj-$(CONFIG_CDNS_UART) 	+= uart/cdns_uart.o

