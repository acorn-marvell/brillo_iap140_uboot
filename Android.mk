#Android makefile to build uboot as a part of Android Build

LOCAL_PATH := $(call my-dir)

.PHONY: build-uboot clean-uboot

UBOOT_SRC := $(abspath $(LOCAL_PATH))
UBOOT_BUILD :=  $(abspath $(TARGET_OUT_INTERMEDIATES)/u-boot)

SECURITY_REGION_SIZE_MB ?= 8

UBOOT_ARCH ?= arm64
ifeq ($(UBOOT_ARCH),arm64)
UBOOT_CROSS_COMPILE := $(ANDROID_BUILD_TOP)/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/aarch64-linux-android-
else
UBOOT_CROSS_COMPILE := $(ANDROID_BUILD_TOP)/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/bin/arm-linux-androideabi-
endif

UBOOT_DEFCONFIG ?= ulc1_dkb_config

PRIVATE_UBOOT_ARGS := -C $(UBOOT_SRC) ARCH=arm CROSS_COMPILE=$(UBOOT_CROSS_COMPILE) \
	SECURITY_REGION_SIZE_MB=$(SECURITY_REGION_SIZE_MB)

ifneq ($(UBOOT_BUILD),)
PRIVATE_UBOOT_ARGS += O=$(abspath $(UBOOT_BUILD))
endif

# Include uboot in the Android build system
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ./../../$(TARGET_OUT_INTERMEDIATES)/u-boot/u-boot.bin
LOCAL_MODULE := u-boot.bin
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(PRODUCT_OUT)

include $(BUILD_PREBUILT)

build-uboot:
ifneq ($(UBOOT_BUILD),)
	@mkdir -p $(UBOOT_BUILD)
endif

ifeq ($(UBOOT_DEFCONFIG),local)
	@echo Skipping uboot configuration, UBOOT_DEFCONFIG set to local
else
	$(MAKE) $(PRIVATE_UBOOT_ARGS) $(UBOOT_DEFCONFIG)
endif
	$(MAKE) $(PRIVATE_UBOOT_ARGS)

ifeq ($(UBOOT_ARCH),arm64)
build-kernel: build-uboot
endif

$(TARGET_OUT_INTERMEDIATES)/u-boot/u-boot.bin : build-uboot

clean clobber : clean-uboot

clean-uboot:
	$(MAKE) $(PRIVATE_UBOOT_ARGS) distclean
