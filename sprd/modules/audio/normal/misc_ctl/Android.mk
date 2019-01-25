# Copyright (C) 2011 The Android Open Source Project
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

#TinyAlsa audio

include $(CLEAR_VARS)


LOCAL_MODULE := libaudiomiscctl
ifeq ($(strip $(AUDIOSERVER_MULTILIB)),)
LOCAL_32_BIT_ONLY := true
else
LOCAL_MULTILIB := $(AUDIOSERVER_MULTILIB)
endif
LOCAL_CFLAGS := -D_POSIX_SOURCE -Wno-multichar -g

LOCAL_CFLAGS += -DLOCAL_SOCKET_SERVER

ifneq ($(filter $(strip $(PLATFORM_VERSION)),4.4.4 5.0 5.1 6.0 6.0.1),)
LOCAL_CFLAGS += -DNO_POWER_SAVING_MODE
endif

LOCAL_SRC_FILES := misc_ctl.cpp

LOCAL_SHARED_LIBRARIES := \
			libutils \
			libcutils  \
			libbinder  \
			libpowermanager  \
			liblog 

LOCAL_C_INCLUDES += \
    external/tinyalsa/include \
    external/expat/lib \
    external/tinycompress/include \
    system/media/audio_utils/include


ifneq ($(filter $(strip $(PLATFORM_VERSION)),O 8.0.0 8.1.0),)
LOCAL_CFLAGS += -DVERSION_IS_ANDROID_O
LOCAL_CFLAGS += -DFLAG_VENDOR_ETC
LOCAL_PROPRIETARY_MODULE := true
endif

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)


