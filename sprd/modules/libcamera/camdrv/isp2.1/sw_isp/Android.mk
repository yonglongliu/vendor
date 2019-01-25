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

ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), arm arm64))
LIB_PATH := lib/lib
else ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), x86 x86_64))
LIB_PATH := lib/x86_lib
endif

include $(CLEAR_VARS)

LOCAL_CFLAGS += -fno-strict-aliasing -Werror

LOCAL_MODULE := libsprdswisp
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := $(LOCAL_MODULE).so
LOCAL_MODULE_STEM_64 := $(LOCAL_MODULE).so
LOCAL_SRC_FILES_32 := $(LIB_PATH)/$(LOCAL_MODULE).so
LOCAL_SRC_FILES_64 := $(LIB_PATH)64/$(LOCAL_MODULE).so

ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_PREBUILT)

include $(CLEAR_VARS)

LOCAL_CFLAGS += -fno-strict-aliasing -Werror -fvisibility=hidden

# ************************************************
# external header file
# ************************************************
LOCAL_C_INCLUDES := \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL/usr/include/video \
	$(LOCAL_PATH)/../../../common/inc

# ************************************************
# internal header file
# ************************************************
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../../../ispalg/isp2.x/smart \
	$(LOCAL_PATH)/../../../ispalg/isp2.x/awb/inc \
	$(LOCAL_PATH)/../../../ispalg/isp2.x/ae/inc \
	$(LOCAL_PATH)/../../../ispalg/isp2.x/ae/sprd/ae/inc \
	$(LOCAL_PATH)/../../../ispalg/isp2.x/common/inc/ \
	$(LOCAL_PATH)/../middleware/inc \
	$(LOCAL_PATH)/../param_manager \
	$(LOCAL_PATH)/../driver/inc \
	$(LOCAL_PATH)/../../../arithmetic/depth/inc \
	$(LOCAL_PATH)/inc

LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL/usr

LOCAL_SRC_FILES += $(call all-c-files-under, .)

LOCAL_MODULE := libcamrt

LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := libcutils libutils libdl liblog
LOCAL_SHARED_LIBRARIES += libbokeh_depth libsprddepth libsprdswisp

ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)
