ifneq ($(TARGET_SIMULATOR),true)
LOCAL_PATH:= $(call my-dir)
ifeq ($(strip $(CONFIG_64KERNEL_32FRAMEWORK)),true)
MIDGARD_ARCH_ := arm64
else
MIDGARD_ARCH_ := $(TARGET_ARCH)
endif

ifeq ($(strip $(MIDGARD_ARCH_)),x86_64)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libGLES_mali.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH_32 := $(TARGET_OUT_VENDOR)/lib/egl
LOCAL_MODULE_PATH_64 := $(TARGET_OUT_VENDOR)/lib64/egl
LOCAL_MULTILIB := both
LOCAL_SRC_FILES_32 :=  usr/sp9853i/libGLES_mali.so
LOCAL_SRC_FILES_64 :=  usr/sp9853i/libGLES_mali_64.so

# Symlink libOpenCL
LOCAL_POST_INSTALL_CMD = $(hide)\
        ln -sf egl/$(notdir $(LOCAL_INSTALLED_MODULE)) $(dir $(LOCAL_INSTALLED_MODULE))../libOpenCL.so.1.1;\
        ln -sf libOpenCL.so.1.1 $(dir $(LOCAL_INSTALLED_MODULE))../libOpenCL.so.1;\
        ln -sf libOpenCL.so.1 $(dir $(LOCAL_INSTALLED_MODULE))../libOpenCL.so;

include $(BUILD_PREBUILT)

#gralloc
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := gralloc.$(TARGET_BOARD_PLATFORM).so

LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH_32 := $(TARGET_OUT_VENDOR)/lib/hw
LOCAL_MODULE_PATH_64 := $(TARGET_OUT_VENDOR)/lib64/hw
LOCAL_MULTILIB := both

LOCAL_SRC_FILES_32 :=  usr/sp9853i/gralloc.midgard.so
LOCAL_SRC_FILES_64 :=  usr/sp9853i/gralloc.midgard_64.so

include $(BUILD_PREBUILT)

#OpenCL
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libRSDriverArm.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH_32 := $(TARGET_OUT_VENDOR)/lib
LOCAL_MODULE_PATH_64 := $(TARGET_OUT_VENDOR)/lib64
LOCAL_MULTILIB := both
LOCAL_SRC_FILES_32 :=  usr/sp9853i/libRSDriverArm.so
LOCAL_SRC_FILES_64 :=  usr/sp9853i/libRSDriverArm_64.so

include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libbccArm.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH_32 := $(TARGET_OUT_VENDOR)/lib
LOCAL_MODULE_PATH_64 := $(TARGET_OUT_VENDOR)/lib64
LOCAL_MULTILIB := both
LOCAL_SRC_FILES_32 :=  usr/sp9853i/libbccArm.so
LOCAL_SRC_FILES_64 :=  usr/sp9853i/libbccArm_64.so

include $(BUILD_PREBUILT)


include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libmalicore.bc
LOCAL_MODULE_OWNER := arm
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_PATH_32 := $(TARGET_OUT_VENDOR)/lib
LOCAL_MODULE_PATH_64 := $(TARGET_OUT_VENDOR)/lib64
LOCAL_MULTILIB := both
LOCAL_SRC_FILES_32 :=  usr/sp9853i/libmalicore.bc
LOCAL_SRC_FILES_64 :=  usr/sp9853i/libmalicore_64.bc
$(shell mkdir -p $(LOCAL_MODULE_PATH_64))
$(shell mkdir -p $(LOCAL_MODULE_PATH_32))
$(shell cp -rf $(LOCAL_PATH)/usr/sp9853i/libmalicore_64.bc $(LOCAL_MODULE_PATH_64)/libmalicore.bc)
$(shell cp -rf $(LOCAL_PATH)/usr/sp9853i/libmalicore.bc $(LOCAL_MODULE_PATH_32)/libmalicore.bc)

include $(BUILD_PREBUILT)




else





ifeq ($(strip $(MIDGARD_ARCH_)),arm64)

$(warning  "GPU: include $(LOCAL_PATH)/midgard_arm64.mk")
include  $(LOCAL_PATH)/midgard_arm64.mk

else ifeq ($(strip $(MALI_PLATFORM_NAME)),sharkle)

$(warning  "GPU sharkl2: include $(LOCAL_PATH)/midgard_arm64.mk")
include  $(LOCAL_PATH)/midgard_arm64.mk

else
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libGLES_mali.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/lib/egl
ifeq ($(strip $(MALI_PLATFORM_NAME)),sharkl2)
LOCAL_SRC_FILES := usr/sp9833/libGLES_mali.so
else
LOCAL_SRC_FILES := usr/libGLES_mali.so
endif

# Symlink libOpenCL
LOCAL_POST_INSTALL_CMD = $(hide)\
        ln -sf egl/$(notdir $(LOCAL_INSTALLED_MODULE)) $(dir $(LOCAL_INSTALLED_MODULE))../libOpenCL.so.1.1;\
        ln -sf libOpenCL.so.1.1 $(dir $(LOCAL_INSTALLED_MODULE))../libOpenCL.so.1;\
        ln -sf libOpenCL.so.1 $(dir $(LOCAL_INSTALLED_MODULE))../libOpenCL.so;


#Symlink Vulkan
#LOCAL_POST_INSTALL_CMD += mkdir -p $(dir $(LOCAL_INSTALLED_MODULE))../hw;\
#	ln -sf ../egl/$(notdir $(LOCAL_INSTALLED_MODULE)) $(dir $(LOCAL_INSTALLED_MODULE))../hw/vulkan.$(TARGET_BOARD_PLATFORM).so

include $(BUILD_PREBUILT)

#hw
include $(CLEAR_VARS)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_TAGS := optional
ifeq ($(strip $(TARGET_BOARD_PLATFORM)),)
LOCAL_MODULE := gralloc.default.so
else
LOCAL_MODULE := gralloc.$(TARGET_BOARD_PLATFORM).so
endif

LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MULTILIB := both
ifeq ($(strip $(MALI_PLATFORM_NAME)),sharkl2)
LOCAL_SRC_FILES :=  usr/sp9833/gralloc.midgard.so
else
LOCAL_SRC_FILES :=  usr/gralloc.midgard.so
endif

include $(BUILD_PREBUILT)

#OpenCL
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libRSDriverArm.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/lib
ifeq ($(strip $(MALI_PLATFORM_NAME)),sharkl2)
LOCAL_SRC_FILES :=  usr/sp9833/libRSDriverArm.so
else
LOCAL_SRC_FILES :=  usr/libRSDriverArm.so
endif

include $(BUILD_PREBUILT)


include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libbccArm.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/lib
ifeq ($(strip $(MALI_PLATFORM_NAME)),sharkl2)
LOCAL_SRC_FILES :=  usr/sp9833/libbccArm.so
else
LOCAL_SRC_FILES :=  usr/libbccArm.so
endif

include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libmalicore.bc
LOCAL_MODULE_OWNER := arm
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/lib
ifeq ($(strip $(MALI_PLATFORM_NAME)),sharkl2)
LOCAL_SRC_FILES :=  usr/sp9833/libmalicore.bc
else
LOCAL_SRC_FILES :=  usr/libmalicore.bc
endif
$(shell mkdir -p $(LOCAL_MODULE_PATH)/)
$(shell cp -rf $(LOCAL_PATH)/usr/libmalicore.bc $(LOCAL_MODULE_PATH)/libmalicore.bc)

include $(BUILD_PREBUILT)
endif



endif

#ko
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := mali.ko
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(PRODUCT_OUT)/root/lib/modules
LOCAL_STRIP_MODULE := keep_symbols
LOCAL_SRC_FILES := mali/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

ifeq ($(TARGET_BUILD_VARIANT),user)
  DEBUGMODE := BUILD=no
else
  DEBUGMODE := $(DEBUGMODE)
endif
  #MALI_PLATFORM_NAME variable was added at file :device/sprd/*platform*/common/BoardCommon.mk 

$(LOCAL_PATH)/mali/mali.ko: $(TARGET_PREBUILT_KERNEL)
	$(MAKE) -C $(shell dirname $@) ARCH=$(TARGET_KERNEL_ARCH) CROSS_COMPILE=$(KERNEL_CROSS_COMPILE) CONFIG_MALI_PLATFORM_THIRDPARTY_NAME=$(MALI_PLATFORM_NAME) CONFIG_MALI_BOARD_PLATFORM=$(TARGET_BOARD_PLATFORM) $(DEBUGMODE) CONFIG_MALI_VOLTAGE_LEVEL=$(VOLTAGE_CONTROLLER_LEVEL) KDIR=$(ANDROID_PRODUCT_OUT)/obj/KERNEL clean
	$(MAKE) -C $(shell dirname $@) ARCH=$(TARGET_KERNEL_ARCH) CROSS_COMPILE=$(KERNEL_CROSS_COMPILE)$(EXT_FLAGS) CONFIG_MALI_PLATFORM_THIRDPARTY_NAME=$(MALI_PLATFORM_NAME) CONFIG_MALI_BOARD_PLATFORM=$(TARGET_BOARD_PLATFORM) $(DEBUGMODE) CONFIG_MALI_VOLTAGE_LEVEL=$(VOLTAGE_CONTROLLER_LEVEL) KDIR=$(ANDROID_PRODUCT_OUT)/obj/KERNEL
	@-$(KERNEL_CROSS_COMPILE)strip -d --strip-unneeded $@


endif
