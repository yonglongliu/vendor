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
LOCAL_PATH := $(call my-dir)

TUNING_PATH := $(TARGET_OUT)/lib/tuning
LOCAL_TUNING_PATH := parameters/tuning_bin

ifeq ($(strip $(TARGET_PRODUCT)),sp9861e_2h10_vmm)
LOCAL_TUNING_PATH := parameters/tuning_bin/truly
endif

include $(CLEAR_VARS)
LOCAL_MODULE := ov2680_mipi_raw_tuning.bin
LOCAL_SRC_FILES := $(LOCAL_TUNING_PATH)/$(LOCAL_MODULE)
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TUNING_PATH)
include $(BUILD_PREBUILT)


