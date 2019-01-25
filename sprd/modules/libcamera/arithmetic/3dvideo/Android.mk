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
ifneq ($(PLATFORM_VERSION),4.4.4)
#ifeq (true, $(filter $(strip $(TARGET_BOARD_STEREOVIDEO_SUPPORT)) $(strip $(TARGET_BOARD_STEREOPREVIEW_SUPPORT)) $(strip $(TARGET_BOARD_STEREOCAPTURE_SUPPORT)), true))
ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), arm arm64))
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libimagestitcher
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := $(LOCAL_MODULE).so
LOCAL_MODULE_STEM_64 := $(LOCAL_MODULE).so
LOCAL_SRC_FILES_32 := lib/$(LOCAL_MODULE).so
LOCAL_SRC_FILES_64 := lib64/$(LOCAL_MODULE).so

ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_PREBUILT)
endif
endif
