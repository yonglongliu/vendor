# Spreadtrum efuse hardware layer

LOCAL_PATH:= $(call my-dir)

SPECIAL_EFUSE_LIST := 9838 7720 9832a 7731

TEE_EFUSE_LIST := 9860

X86_TEE_EFUSE_LIST := 9861e
ARM_TEE_EFUSE_LIST := 9850ka 9820e

include $(CLEAR_VARS)

ifneq ($(strip $(foreach var, $(SPECIAL_EFUSE_LIST), $(strip $(findstring $(var), $(TARGET_BOARD))))),)
LOCAL_SRC_FILES:= sprd_efuse_hw.c
else ifneq ($(strip $(foreach var, $(TEE_EFUSE_LIST), $(strip $(findstring $(var), $(TARGET_BOARD))))),)
LOCAL_SRC_FILES:= sprd_whalex_efuse_hw.c
else ifneq ($(strip $(foreach var, $(X86_TEE_EFUSE_LIST), $(strip $(findstring $(var), $(TARGET_BOARD))))),)
LOCAL_CFLAGS += -DX86_TEE_EFUSE_CONFIG
LOCAL_SRC_FILES:= sprd_iwhalex_efuse_hw.c
else ifneq ($(strip $(foreach var, $(ARM_TEE_EFUSE_LIST), $(strip $(findstring $(var), $(TARGET_BOARD))))),)
LOCAL_CFLAGS += -DARM_TEE_EFUSE_CONFIG
LOCAL_SRC_FILES:= sprd_iwhalex_efuse_hw.c
else
LOCAL_SRC_FILES:= sprd_tshark_efuse_hw.c
endif

LOCAL_MODULE:= libefuse

LOCAL_SHARED_LIBRARIES:= liblog libc libcutils

ifneq ($(BOARD_TEE_CONFIG),)
ifneq ($(strip $(foreach var, $(X86_TEE_EFUSE_LIST), $(strip $(findstring $(var), $(TARGET_BOARD))))),)
LOCAL_CFLAGS += -DCONFIG_SPRD_SECBOOT_TEE
LOCAL_SHARED_LIBRARIES  += libteeproduction
else ifneq ($(strip $(foreach var, $(ARM_TEE_EFUSE_LIST), $(strip $(findstring $(var), $(TARGET_BOARD))))),)
LOCAL_CFLAGS += -DCONFIG_SPRD_SECBOOT_TEE
LOCAL_SHARED_LIBRARIES  += libteeproduction
endif
endif

LOCAL_C_INCLUDES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include

LOCAL_MODULE_TAGS:= optional
LOCAL_MULTILIB := both
include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)

ifneq ($(strip $(foreach var, $(SPECIAL_EFUSE_LIST), $(strip $(findstring $(var), $(TARGET_BOARD))))),)
LOCAL_SRC_FILES:= sprd_efuse_hw.c
else ifneq ($(strip $(foreach var, $(TEE_EFUSE_LIST), $(strip $(findstring $(var), $(TARGET_BOARD))))),)
LOCAL_SRC_FILES:= sprd_whalex_efuse_hw.c
else ifneq ($(strip $(foreach var, $(X86_TEE_EFUSE_LIST), $(strip $(findstring $(var), $(TARGET_BOARD))))),)
LOCAL_CFLAGS += -DX86_TEE_EFUSE_CONFIG
LOCAL_SRC_FILES:= sprd_iwhalex_efuse_hw.c
else ifneq ($(strip $(foreach var, $(ARM_TEE_EFUSE_LIST), $(strip $(findstring $(var), $(TARGET_BOARD))))),)
LOCAL_CFLAGS += -DARM_TEE_EFUSE_CONFIG
LOCAL_SRC_FILES:= sprd_iwhalex_efuse_hw.c
else
LOCAL_SRC_FILES:= sprd_tshark_efuse_hw.c
endif

LOCAL_MODULE:= libefuse

LOCAL_SHARED_LIBRARIES:= liblog libc libcutils

ifneq ($(BOARD_TEE_CONFIG),)
ifneq ($(strip $(foreach var, $(X86_TEE_EFUSE_LIST), $(strip $(findstring $(var), $(TARGET_BOARD))))),)
LOCAL_CFLAGS += -DCONFIG_SPRD_SECBOOT_TEE
LOCAL_SHARED_LIBRARIES  += libteeproduction
else ifneq ($(strip $(foreach var, $(ARM_TEE_EFUSE_LIST), $(strip $(findstring $(var), $(TARGET_BOARD))))),)
LOCAL_CFLAGS += -DCONFIG_SPRD_SECBOOT_TEE
LOCAL_SHARED_LIBRARIES  += libteeproduction
endif
endif

LOCAL_C_INCLUDES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)

LOCAL_MODULE_TAGS:= optional
LOCAL_MULTILIB := both
include $(BUILD_STATIC_LIBRARY)

ifeq ($(strip $(HAS_GETUID_LIB)),true)
include $(call all-makefiles-under,$(LOCAL_PATH))
endif

