#
# (C) Copyright 2000-2004
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# SPDX-License-Identifier:	GPL-2.0+
#

obj-y	:= pxa1928_ff.o ../common/ddr_tests.o
ifeq "$(TARGET_BUILD_VARIANT)" "user"
CFLAGS += -DBUILD_VARIANT_USER
endif
