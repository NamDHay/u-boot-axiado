# SPDX-License-Identifier: GPL-2.0
#
# Makefile for Axiado SHIM Driver
#

obj-$(CONFIG_AXIADO_HCP) += axiado_shim.o

axiado_shim-y := \
	mac_config.o \
	phy_mdio.o \
	shim_mac_init.o \
	shim_platform.o

ccflags-y += -I$(src)
ccflags-y += -I$(srctree)/drivers/net/axiado
ccflags-y += -I$(srctree)/drivers/net/axiado/hfifo
