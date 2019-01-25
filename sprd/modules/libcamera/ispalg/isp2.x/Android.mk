#
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
#

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS += -fno-strict-aliasing -Wunused-variable -Werror


# ************************************************
# external header file
# ************************************************

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),2.1)
ifneq ($(filter $(strip $(TARGET_BOARD_PLATFORM)),$(strip $(PLATFORM_VERSION_FILTER))),)
ISP_HW_VER = 2v1a
else
ISP_HW_VER = 2v1
endif

ISP_DIR := ../../camdrv/isp2.1
endif
ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),2.2)
ISP_DIR := ../../camdrv/isp2.2
endif
ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),2.3)
ISP_DIR := ../../camdrv/isp2.3
endif
ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),2.4)
ISP_DIR := ../../camdrv/isp2.4
ISP_HW_VER = 2v4
endif
ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),2.5)
ISP_DIR := ../../camdrv/isp2.5
endif


LOCAL_C_INCLUDES := \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL/usr/include/video \
	$(LOCAL_PATH)/../../common/inc \
	$(LOCAL_PATH)/../../oem$(ISP_HW_VER)/inc \
	$(LOCAL_PATH)/../../oem$(ISP_HW_VER)/isp_calibration/inc \
	$(LOCAL_PATH)/../../jpeg/inc \
	$(LOCAL_PATH)/../../vsp/inc \
	$(LOCAL_PATH)/../../tool/mtrace

# ************************************************
# internal header file
# ************************************************
ISP_ALGO_DIR := .

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/$(ISP_DIR)/middleware/inc \
	$(LOCAL_PATH)/$(ISP_DIR)/isp_tune \
	$(LOCAL_PATH)/$(ISP_DIR)/calibration \
	$(LOCAL_PATH)/$(ISP_DIR)/driver/inc \
	$(LOCAL_PATH)/$(ISP_DIR)/param_manager \
	$(LOCAL_PATH)/$(ISP_DIR)/bridge \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/ae/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/ae/sprd/ae/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/ae/sprd/flash/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/atm/sprd/inc\
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/awb/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/awb/alc_awb/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/awb/sprd/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/af/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/af/sprd/afv1/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/af/sprd/aft/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/af/sft_af/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/af/alc_af/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/pdaf/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/pdaf/sprd/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/lsc/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/common/inc/ \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/afl/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/smart \
	$(LOCAL_PATH)/$(ISP_DIR)/utility \
	$(LOCAL_PATH)/$(ISP_DIR)/calibration/inc \
	$(LOCAL_PATH)/../../sensor/inc

LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL/usr

# don't modify this code
LOCAL_SRC_FILES := $(shell find $(LOCAL_PATH) -name '*.c' | sed s:^$(LOCAL_PATH)/::g)

include $(LOCAL_PATH)/../../SprdCtrl.mk

LOCAL_MODULE := libispalg

LOCAL_MODULE_TAGS := optional


ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_SHARED_LIBRARIES += liblog
LOCAL_PROPRIETARY_MODULE := true
endif

LOCAL_SHARED_LIBRARIES += libcampm
LOCAL_SHARED_LIBRARIES += libcutils libutils libdl libcamcommon

LOCAL_SHARED_LIBRARIES += libcamsensor

LOCAL_SHARED_LIBRARIES += libdeflicker

LOCAL_SHARED_LIBRARIES += libae libflash
LOCAL_SHARED_LIBRARIES += libawb1
LOCAL_SHARED_LIBRARIES += liblsc libsprdlsc
LOCAL_SHARED_LIBRARIES += libatm
LOCAL_SHARED_LIBRARIES += libSprdPdAlgo

LOCAL_SHARED_LIBRARIES += libspcaftrigger
ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),2.3)
LOCAL_SHARED_LIBRARIES += libspafv1_le
else
LOCAL_SHARED_LIBRARIES += libspafv1
endif



include $(BUILD_SHARED_LIBRARY)

include $(call first-makefiles-under,$(LOCAL_PATH))

