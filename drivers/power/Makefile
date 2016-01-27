#
# Copyright (c) 2009 Wind River Systems, Inc.
# Tom Rix <Tom.Rix at windriver.com>
#
# SPDX-License-Identifier:	GPL-2.0+
#

obj-$(CONFIG_EXYNOS_TMU)	+= exynos-tmu.o
obj-$(CONFIG_FTPMU010_POWER)	+= ftpmu010.o
obj-$(CONFIG_TPS6586X_POWER)	+= tps6586x.o
obj-$(CONFIG_TWL4030_POWER)	+= twl4030.o
obj-$(CONFIG_TWL6030_POWER)	+= twl6030.o
obj-$(CONFIG_PALMAS_POWER)	+= palmas.o
obj-$(CONFIG_PXA988_POWER)	+= pxa988_freq.o pxa988_power.o pxa_common.o
obj-$(CONFIG_HELAN2_POWER)	+= pxa1U88_freq.o helanx_28nmpll.o pxa1U88_power.o pxa_common.o helanx_hwdvc.o
obj-$(CONFIG_HELAN3_POWER)	+= pxa1936_freq.o helanx_28nmpll.o pxa1U88_power.o pxa_common.o helanx_hwdvc.o fuseinfo.o
obj-$(CONFIG_PXA1928_DFC)	+= pxa1928_freq.o
obj-$(CONFIG_PXA1928_COMM_D2)	+= pxa1928_comm_d2.o
obj-$(CONFIG_PXA1928_POWER)	+= pxa1928_power.o

obj-$(CONFIG_POWER) += power_core.o
obj-$(CONFIG_DIALOG_POWER) += power_dialog.o
obj-$(CONFIG_POWER_FSL) += power_fsl.o
obj-$(CONFIG_POWER_I2C) += power_i2c.o
obj-$(CONFIG_POWER_SPI) += power_spi.o

obj-$(CONFIG_PXA988_LPM_TEST) += mmp_idle.o
obj-$(CONFIG_PXA1908_LPM) += pxa1908_idle.o
obj-$(CONFIG_PXA1936_LPM) += pxa1936_idle.o
obj-$(CONFIG_PXA1928_LPM) += pxa1928_idle.o
