ccflags-$(CONFIG_CMD_AX_DIAG) += \
	$(addprefix -I,$(shell find $(srctree)/cmd/diag -type d))

obj-$(CONFIG_INNO_ADC) 		+= adc/inno_saradc.o

obj-$(CONFIG_ARASAN_SDHCI) 	+= emmc/arasan_sdhci.o emmc/arasan_sdhci_hs.o

obj-$(CONFIG_CDNS_GPIO) 	+= gpio/cdns_gpio.o

obj-$(CONFIG_INNO_HDMI) 	+= hdmi/inno_hdmi.o

obj-y 						+= i2c/ax_i2c.o
obj-$(CONFIG_CDNS_I2C) 		+= i2c/cdns_i2c.o
obj-$(CONFIG_CDNS_I3C) 		+= i3c/cdns_i3c.o

obj-$(CONFIG_AXIADO_JTAG) 	+= jtag/axiado_jtag.o

obj-$(CONFIG_AXIADO_LTPI) 	+= ltpi/axiado_ltpi.o

obj-$(CONFIG_AXIADO_MBOX) 	+= mailbox/axiado_mailbox.o

obj-$(CONFIG_CMD_AX_PCIE) 	+= pcie/ax_pcie.o
obj-$(CONFIG_INNO_PCIE) 	+= pcie/axiado_pcie.o pcie/axiado_pcie_ep.o

obj-$(CONFIG_CMD_AX_NVME) 	+= nvme/ax_nvme.o

obj-$(CONFIG_AXIADO_PINMUX) += pinmux/axiado_pinmux.o

obj-$(CONFIG_CDNS_PWM) 		+= pwm/cdns_pwm.o

obj-$(CONFIG_CDNS_RTC) 		+= rtc/cdns_rtc.o

obj-$(CONFIG_AXIADO_SGPIO) 	+= sgpio/axiado_sgpio.o

obj-y 						+= spi/ax_spi.o
obj-$(CONFIG_DWC_SPI) 		+= spi/dwc_spi.o
obj-$(CONFIG_CMD_AX_FLASH) 	+= flash/ax_flash.o

obj-$(CONFIG_OPENTITAN_TPM)	+= tpm/opentitan_tpm.o

obj-$(CONFIG_CDNS_UART) 	+= uart/cdns_uart.o

obj-$(CONFIG_CDNS_WDT) 		+= wdt/cdns_wdt.o
obj-$(CONFIG_AXIADO_WDT) 	+= wdt/axiado_wdt.o

obj-$(CONFIG_AXIADO_USB) 	+= usb/ax_xhci_pci.o usb/ax_usb.o usb/xhci.o usb/xhci_ring.o usb/xhci_msc.o usb/xhci_hw.o
