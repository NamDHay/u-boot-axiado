ccflags-$(CONFIG_CMD_AX_DIAG) += \
	$(addprefix -I,$(shell find $(srctree)/cmd/diag -type d))

obj-y						+= adc/ax_adc.o
obj-$(CONFIG_INNO_ADC) 		+= adc/inno_saradc.o

# obj-y						+= can/ax_can.o

obj-$(CONFIG_ARASAN_SDHCI) 	+= emmc/arasan_sdhci.o emmc/arasan_sdhci_hs.o

obj-$(CONFIG_AXIADO_HCP_ETH) 	+= ethernet/hcp/hcp_main.o \
				   ethernet/hcp/eth_common_api/eth_common_api.o \
				   ethernet/hcp/mac/eth_mac.o \
				   ethernet/hcp/mdio/eth_mdio.o \
				   ethernet/hcp/mdio/eth_phy_table.o \
				   ethernet/hcp/net/eth_net.o

obj-y						+= gpio/ax_gpio.o
obj-$(CONFIG_CDNS_GPIO) 	+= gpio/cdns_gpio.o

obj-$(CONFIG_INNO_HDMI) 	+= hdmi/inno_hdmi.o

obj-y 						+= i2c/ax_i2c.o
obj-$(CONFIG_CDNS_I2C) 		+= i2c/cdns_i2c.o

obj-y 						+= i3c/ax_i3c.o
obj-$(CONFIG_CDNS_I3C) 		+= i3c/cdns_i3c.o

obj-$(CONFIG_AXIADO_JTAG) 	+= jtag/axiado_jtag.o

obj-y 						+= jtag/axiado_jtag.o
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

obj-$(CONFIG_AXIADO_XHCI) 	+= usb/host/ax_xhci_pci.o usb/host/ax_usb.o usb/host/xhci.o usb/host/xhci_ring.o usb/host/xhci_msc.o usb/host/xhci_hw.o

obj-$(CONFIG_CRG_XHCI) 		+= usb/gadget/crg_udc.o

obj-y 						+= wdt/ax_wdt.o
obj-$(CONFIG_CDNS_WDT) 		+= wdt/cdns_wdt.o
obj-$(CONFIG_AXIADO_WDT) 	+= wdt/axiado_wdt.o
