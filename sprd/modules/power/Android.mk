# Copyright (C) 2012 The Android Open Source Project
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

ifneq ($(BOARD_POWERHINT_HAL),)
LOCAL_PATH := $(call my-dir)

# HAL module implemenation stored in
# hw/<POWERS_HARDWARE_MODULE_ID>.<ro.hardware>.so
include $(CLEAR_VARS)

LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_C_INCLUDES := \
    external/libxml2/include \
    external/icu/icu4c/source/common

LOCAL_SHARED_LIBRARIES := liblog libcutils libicuuc
LOCAL_STATIC_LIBRARIES := libxml2

LOCAL_SRC_FILES := \
    common.c \
    sprd_power.c \
    config.c \
    devfreq.c \
    cpufreq.c \
    pm_qos.c \
    utils.c

LOCAL_MODULE := power.$(TARGET_BOARD_PLATFORM)
LOCAL_VENDOR_MODULE := true
LOCAL_CFLAGS := -DDEBUG=1 -DDEBUG_V=1
LOCAL_CFLAGS += -DBOOST_SPECIFICED -std=c99
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := power_scene_config.xml
LOCAL_MODULE_CLASS := ETC
LOCAL_VENDOR_MODULE := true

ifeq ($(POWERHINT_PRODUCT_CONFIG),)
LOCAL_SRC_FILES := config_files/scene_config_$(BOARD_POWERHINT_HAL).xml
else
LOCAL_SRC_FILES := config_files/$(POWERHINT_PRODUCT_CONFIG)/power_scene_config.xml
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := power_resource_file_info.xml
LOCAL_MODULE_CLASS := ETC
LOCAL_VENDOR_MODULE := true

ifeq ($(POWERHINT_PRODUCT_CONFIG),)
LOCAL_SRC_FILES := config_files/resource_file_info_$(BOARD_POWERHINT_HAL).xml
else
LOCAL_SRC_FILES := config_files/$(POWERHINT_PRODUCT_CONFIG)/power_resource_file_info.xml
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := power_scene_id_define.txt
LOCAL_MODULE_CLASS := ETC
LOCAL_VENDOR_MODULE := true

LOCAL_SRC_FILES := config_files/power_scene_id_define.txt
include $(BUILD_PREBUILT)

endif
