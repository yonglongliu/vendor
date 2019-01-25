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
ifeq ($(strip $(TARGET_BOARD_SBS_MODE_SENSOR)),true)
LOCAL_PATH := $(call my-dir)
ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), arm arm64))
LIB_PATH := lib/lib
else ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), x86 x86_64))
LIB_PATH := lib/x86_lib
endif
include $(CLEAR_VARS)
LOCAL_MODULE := libsensor_sbs
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libsensor_sbs.a
LOCAL_MODULE_STEM_64 := libsensor_sbs.a
LOCAL_SRC_FILES_32 := $(LIB_PATH)/libsensor_sbs.a
LOCAL_SRC_FILES_64 := $(LIB_PATH)64/libsensor_sbs.a
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)
endif
