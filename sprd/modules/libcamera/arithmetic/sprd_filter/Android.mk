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

ifeq ($(strip $(TARGET_BOARD_CAMERA_FILTER_VERSION)),0)
LOCAL_PATH := $(call my-dir)
ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), arm arm64))
LIB_PATH := lib
else ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), x86 x86_64))
LIB_PATH := x86_lib
endif

#Sprd filter capture lib
include $(CLEAR_VARS)
LOCAL_MODULE := libSprdImageFilter
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libSprdImageFilter.so
LOCAL_MODULE_STEM_64 := libSprdImageFilter.so
LOCAL_SRC_FILES_32 := $(LIB_PATH)/libSprdImageFilter.so
LOCAL_SRC_FILES_64 := $(LIB_PATH)64/libSprdImageFilter.so
LOCAL_MODULE_TAGS := optional

ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_PREBUILT)

endif
