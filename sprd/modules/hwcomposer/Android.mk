# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


LOCAL_PATH := $(call my-dir)



# HAL module implemenation stored in
# hw/<OVERLAY_HARDWARE_MODULE_ID>.<ro.product.board>.so

ifeq ($(strip $(USE_SPRD_HWCOMPOSER)),true)

include $(CLEAR_VARS)

LOCAL_MODULE_RELATIVE_PATH :=hw
LOCAL_SHARED_LIBRARIES := liblog       \
                          libEGL       \
                          libmemion    \
                          libutils     \
                          libcutils    \
                          libGLESv1_CM \
                          libGLESv2    \
                          libhardware  \
                          libui        \
                          libsync

LOCAL_SRC_FILES := SprdHWComposer.cpp \
		   AndroidFence.cpp \
		   SprdDisplayPlane.cpp \
		   SprdHWLayer.cpp \
		   SprdPrimaryDisplayDevice/SprdPrimaryDisplayDevice.cpp \
		   SprdPrimaryDisplayDevice/SprdHWLayerList.cpp \
		   SprdPrimaryDisplayDevice/SprdOverlayPlane.cpp \
		   SprdPrimaryDisplayDevice/SprdPrimaryPlane.cpp \
		   SprdVirtualDisplayDevice/SprdVirtualDisplayDevice.cpp \
		   SprdVirtualDisplayDevice/SprdVDLayerList.cpp \
		   SprdVirtualDisplayDevice/SprdVirtualPlane.cpp \
		   SprdVirtualDisplayDevice/SprdWIDIBlit.cpp \
		   SprdExternalDisplayDevice/SprdExternalDisplayDevice.cpp \
		   SprdUtil.cpp \
                   dump.cpp

LOCAL_C_INCLUDES := \
	$(TOP)/vendor/sprd/modules/libmemion \
	$(TOP)/vendor/sprd/external/kernel-headers \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL/usr/include/video \
        $(TARGET_OUT_INTERMEDIATES)/KERNEL/
#	$(LOCAL)/../gsp
#	$(LOCAL)/../dpu

LOCAL_PROPRIETARY_MODULE := true
ifeq ($(strip $(TARGET_BOARD_PLATFORM)),)
LOCAL_MODULE := hwcomposer.default
else
LOCAL_MODULE := hwcomposer.$(TARGET_BOARD_PLATFORM)
endif
LOCAL_CFLAGS:= -DLOG_TAG=\"SPRDHWComposer\"
LOCAL_CFLAGS += -DGL_GLEXT_PROTOTYPES -DEGL_EGLEXT_PROTOTYPES
#DPU_ENABLE := true
#ifeq ($(strip $(DPU_ENABLE)),true)
ifeq ($(strip $(MALI_PLATFORM_NAME)),sharkl2)
LOCAL_CFLAGS += -DACC_BY_DISPC
else ifeq ($(strip $(MALI_PLATFORM_NAME)),sharkle)
LOCAL_CFLAGS += -DACC_BY_DISPC
else ifeq ($(strip $(MALI_PLATFORM_NAME)),isharkl2)
LOCAL_CFLAGS += -DACC_BY_DISPC
endif
LOCAL_C_INCLUDES += $(TOP)/vendor/sprd/external/drivers/gpu
LOCAL_CFLAGS += -DTARGET_GPU_PLATFORM=$(TARGET_GPU_PLATFORM)

ifeq ($(strip $(LOWPOWER_DISPLAY_30FPS)) , true)
    LOCAL_CFLAGS += -DUPDATE_SYSTEM_FPS_FOR_POWER_SAVE
endif

ifeq ($(strip $(TARGET_ARCH)),x86_64)
LOCAL_CFLAGS += -DARCH_WITHOUT_NEON
endif

ifeq ($(strip $(TARGET_GPU_PLATFORM)),midgard)
LOCAL_CFLAGS += -DGRALLOC_MIDGARD
else ifeq ($(strip $(TARGET_GPU_PLATFORM)),rogue)
LOCAL_CFLAGS += -DGRALLOC_ROGUE
else ifeq ($(strip $(TARGET_GPU_PLATFORM)),soft)
LOCAL_CFLAGS += -DGRALLOC_MIDGARD
else
LOCAL_CFLAGS += -DGRALLOC_UTGARD
endif

#DEVICE_OVERLAYPLANE_BORROW_PRIMARYPLANE_BUFFER can make SprdPrimaryPlane
#share the plane buffer to SprdOverlayPlane,
#save 3 screen size YUV420 buffer memory.
#DEVICE_PRIMARYPLANE_USE_RGB565 make the SprdPrimaryPlane use
#RGB565 format buffer to display, also can save 4 screen size
#buffer memory, but it will reduce the image quality.

#ifeq ($(strip $(TARGET_BOARD_PLATFORM)),sc8830)
DEVICE_WITH_GSP := true

DEVICE_OVERLAYPLANE_BORROW_PRIMARYPLANE_BUFFER := true
DEVICE_USE_FB_HW_VSYNC := true
ifeq ($(strip $(TARGET_GPU_PLATFORM)),rogue)
DEVICE_DIRECT_DISPLAY_SINGLE_OSD_LAYER := false
else
DEVICE_DIRECT_DISPLAY_SINGLE_OSD_LAYER := true
endif
#endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),scx15)
DEVICE_WITH_GSP := true
DEVICE_OVERLAYPLANE_BORROW_PRIMARYPLANE_BUFFER := true
endif

ifeq ($(strip $(GET_FRAMEBUFFER_FORMAT_FROM_HWC)), true)
	LOCAL_CFLAGS += -DGET_FRAMEBUFFER_FORMAT_FROM_HWC
endif

ifeq ($(FPHONE_ENFORCE_RGB_565_FLAG),true)
	LOCAL_CFLAGS += -DEN_RGB_565
endif

TARGET_SUPPORT_ADF_DISPLAY := true
ifeq ($(strip $(TARGET_SUPPORT_ADF_DISPLAY)),true)

LOCAL_C_INCLUDES += $(TOP)/system/core/adf/libadf/include/  \
		    $(TOP)/system/core/adf/libadfhwc/include/ \
                    $(TOP)/vendor/sprd/external/kernel-headers/

LOCAL_SRC_FILES += SprdADFWrapper.cpp

LOCAL_STATIC_LIBRARIES += libadf libadfhwc
else
LOCAL_SRC_FILES += SprdFrameBufferDevice.cpp \
                  SprdVsyncEvent.cpp
endif

ifeq ($(strip $(DEVICE_USE_FB_HW_VSYNC)),true)
    LOCAL_CFLAGS += -DUSE_FB_HW_VSYNC
endif


ifeq ($(strip $(DEVICE_WITH_GSP)),true)
    LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libcamera/sc8830/inc    
    #LOCAL_CFLAGS += -DVIDEO_LAYER_USE_RGB
    # PROCESS_VIDEO_USE_GSP : protecting sc8830 code
    LOCAL_CFLAGS += -DPROCESS_VIDEO_USE_GSP
    LOCAL_CFLAGS += -DGSP_OUTPUT_USE_YUV420

# if GSP has not IOMMU, DIRECT_DISPLAY_SINGLE_OSD_LAYER need contiguous physcial address;
# if GSP has IOMMU, we can open DIRECT_DISPLAY_SINGLE_OSD_LAYER.
ifeq ($(strip $(DEVICE_DIRECT_DISPLAY_SINGLE_OSD_LAYER)),true)
        LOCAL_CFLAGS += -DDIRECT_DISPLAY_SINGLE_OSD_LAYER
endif

endif

ifeq ($(strip $(DEVICE_PRIMARYPLANE_USE_RGB565)), true)
    LOCAL_CFLAGS += -DPRIMARYPLANE_USE_RGB565
endif

ifeq ($(strip $(DEVICE_OVERLAYPLANE_BORROW_PRIMARYPLANE_BUFFER)), true)
    LOCAL_CFLAGS += -DBORROW_PRIMARYPLANE_BUFFER
endif

ifeq ($(strip $(DEVICE_DYNAMIC_RELEASE_PLANEBUFFER)), true)
    LOCAL_CFLAGS += -DDYNAMIC_RELEASE_PLANEBUFFER
endif

ifeq ($(strip $(TARGET_FORCE_DISABLE_HWC_OVERLAY)), true)
    LOCAL_CFLAGS += -DFORCE_DISABLE_HWC_OVERLAY
endif

# For Virtual Display
# HWC need do the Hardware copy and format convertion
# FORCE_ADJUST_ACCELERATOR: for a better performance of Virtual Display,
# we forcibly make sure the GSP/GPP device to be used by Virtual Display.
# and disable the GSP/GPP on Primary Display.
ifeq ($(TARGET_FORCE_HWC_FOR_VIRTUAL_DISPLAYS),true)
    LOCAL_CFLAGS += -DFORCE_HWC_COPY_FOR_VIRTUAL_DISPLAYS
endif

# OVERLAY_COMPOSER_GPU_CONFIG: Enable or disable OVERLAY_COMPOSER_GPU
# Macro, OVERLAY_COMPOSER will do Hardware layer blending and then
# post the overlay buffer to OSD display plane.
# If you want to know how OVERLAY_COMPOSER use and work,
# Please see the OverlayComposer/OverlayComposer.h for more details.
ifeq ($(strip $(USE_OVERLAY_COMPOSER_GPU)),true)

ifeq ($(strip $(TARGET_GPU_PLATFORM)),midgard)
    LOCAL_CFLAGS += -DINVALIDATE_WINDOW_TARGET
endif
    LOCAL_CFLAGS += -DOVERLAY_COMPOSER_GPU

    LOCAL_SRC_FILES += OverlayComposer/OverlayComposer.cpp \
                       OverlayComposer/OverlayNativeWindow.cpp \
                       OverlayComposer/Layer.cpp \
                       OverlayComposer/Utility.cpp \
                       OverlayComposer/SyncThread.cpp

endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),sc8825)
    
    LOCAL_CFLAGS += -DSCAL_ROT_TMP_BUF

    LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libcamera/sc8825/inc

    LOCAL_SRC_FILES += sc8825/scale_rotate.c
    LOCAL_CFLAGS += -D_PROC_OSD_WITH_THREAD

    LOCAL_CFLAGS += -D_DMA_COPY_OSD_LAYER
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),sc8810)
    LOCAL_CFLAGS += -DSCAL_ROT_TMP_BUF
    LOCAL_SRC_FILES += sc8810/scale_rotate.c
    LOCAL_CFLAGS += -D_VSYNC_USE_SOFT_TIMER
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),sc7710)
    LOCAL_CFLAGS += -DSCAL_ROT_TMP_BUF
    LOCAL_SRC_FILES += sc8810/scale_rotate.c
    LOCAL_CFLAGS += -D_VSYNC_USE_SOFT_TIMER
endif

ifeq ($(strip $(USE_GPU_PROCESS_VIDEO)) , true)
    LOCAL_CFLAGS += -DTRANSFORM_USE_GPU
    LOCAL_SRC_FILES += gpu_transform.cpp
endif

USE_RGB_VIDEO_LAYER := true
ifeq ($(strip $(USE_RGB_VIDEO_LAYER)) , true)
    LOCAL_CFLAGS += -DVIDEO_LAYER_USE_RGB
endif

ifeq ($(strip $(USE_SPRD_DITHER)) , true)
    LOCAL_CFLAGS += -DSPRD_DITHER_ENABLE
endif

ifeq ($(strip $(SPRD_HWC_DEBUG_TRACE)), true)
    LOCAL_CFLAGS += -DHWC_DEBUG_TRACE
endif

ifeq ($(strip $(TARGET_DEBUG_CAMERA_SHAKE_TEST)), true)
    LOCAL_CFLAGS += -DHWC_DUMP_CAMERA_SHAKE_TEST
endif

LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)

endif

include $(call all-makefiles-under,$(LOCAL_PATH))





