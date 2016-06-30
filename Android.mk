#Android makefile to build uboot as a part of Android Build

.PHONY: build-uboot clean-uboot

LOCAL_PATH := $(call my-dir)
UBOOT_PATH := $(ANDROID_BUILD_TOP)/bootable/brillo_iap140_uboot
UBOOT_OUTPUT :=  $(abspath $(PRODUCT_OUT)/obj/uboot)

SECURITY_REGION_SIZE_MB ?= 8

UBOOT_ARCH ?= arm64
ifeq ($(UBOOT_ARCH),arm64)
#export PATH:=$(ANDROID_BUILD_TOP)/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin:$(ANDROID_BUILD_TOP)/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.8/bin:$(PATH)
UBOOT_CROSS_COMPILE := $(ANDROID_BUILD_TOP)/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/aarch64-linux-android-
else
UBOOT_CROSS_COMPILE := $(ANDROID_BUILD_TOP)/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/bin/arm-linux-androideabi-
endif

UBOOT_DEFCONFIG ?= ulc1_dkb_config

PRIVATE_UBOOT_ARGS := -C $(UBOOT_PATH) ARCH=arm CROSS_COMPILE=$(UBOOT_CROSS_COMPILE) \
	SECURITY_REGION_SIZE_MB=$(SECURITY_REGION_SIZE_MB)

#ifneq ($(UBOOT_OUTPUT),)
#PRIVATE_UBOOT_ARGS += O=$(abspath $(UBOOT_OUTPUT))
#endif

# Include uboot in the Android build system
include $(CLEAR_VARS)

#LOCAL_PATH := $(UBOOT_OUTPUT)
LOCAL_SRC_FILES := u-boot.bin
LOCAL_MODULE := $(LOCAL_SRC_FILES)
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(PRODUCT_OUT)

include $(BUILD_PREBUILT)

build-uboot :
#ifneq ($(UBOOT_OUTPUT),)
#	@mkdir -p $(UBOOT_OUTPUT)
#endif

ifeq ($(UBOOT_DEFCONFIG),local)
	@echo Skipping uboot configuration, UBOOT_DEFCONFIG set to local
else
	$(MAKE) $(PRIVATE_UBOOT_ARGS) $(UBOOT_DEFCONFIG)
endif
	$(MAKE) $(PRIVATE_UBOOT_ARGS)

ifeq ($(UBOOT_ARCH),arm64)
build-kernel: build-uboot
endif
#droidcore : u-boot.bin
#$(UBOOT_OUTPUT)/u-boot.bin : build-uboot
$(LOCAL_PATH)/u-boot.bin : build-uboot


clean clobber : clean-uboot

clean-uboot:
	$(MAKE) $(PRIVATE_UBOOT_ARGS) distclean

