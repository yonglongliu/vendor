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
ifeq ($(strip $(TARGET_BOARD_CAMERA_3DNR_CAPTURE)),true)
LOCAL_PATH:= $(call my-dir)

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),sp9832e)

ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), arm arm64))
LIB_PATH := blacksesame/mv_lib/lib
endif

include $(CLEAR_VARS)
LOCAL_MODULE := libtdnsTest
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := $(LOCAL_MODULE).so
LOCAL_MODULE_STEM_64 := $(LOCAL_MODULE).so
LOCAL_SRC_FILES_32 := $(LIB_PATH)/$(LOCAL_MODULE).so
LOCAL_SRC_FILES_64 := $(LIB_PATH)64/$(LOCAL_MODULE).so
LOCAL_MODULE_TAGS := optional
ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_PREBUILT)

else

ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), arm arm64))
LIB_PATH := sprd/mv_lib/lib
else ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), x86 x86_64))
LIB_PATH := sprd/mv_lib/x86_lib
endif

include $(CLEAR_VARS)
LOCAL_MODULE := libsprd3dnrmv
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := $(LOCAL_MODULE).so
LOCAL_MODULE_STEM_64 := $(LOCAL_MODULE).so
LOCAL_SRC_FILES_32 := $(LIB_PATH)/$(LOCAL_MODULE).so
LOCAL_SRC_FILES_64 := $(LIB_PATH)64/$(LOCAL_MODULE).so
LOCAL_MODULE_TAGS := optional
ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := sprd/src/3dnr_module.cpp
LOCAL_MODULE :=libsprd3dnr

LOCAL_SHARED_LIBRARIES :=libcutils libsprd3dnrmv
ifneq ($(filter $(strip $(PLATFORM_VERSION)),O 8.0.0 8.1.0),)
LOCAL_SHARED_LIBRARIES +=liblog
endif
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS :=  -O3 -fno-strict-aliasing -fPIC -fvisibility=hidden -Wunused-variable -Wunused-function  -Werror

LOCAL_C_INCLUDES := \
         $(TARGET_OUT_INTERMEDIATES)/KERNEL/usr/include/video \
         $(LOCAL_PATH)/sprd/inc \
         $(LOCAL_PATH)/../../common/inc \
         $(LOCAL_PATH)/../libgralloc_mali \
         $(TOP)/system/core/include/cutils/

ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)
endif
endif
