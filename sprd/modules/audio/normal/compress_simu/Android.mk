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


LOCAL_MODULE := libcompresssimu
LOCAL_32_BIT_ONLY := true
LOCAL_CFLAGS := -D_POSIX_SOURCE -Wno-multichar -g

LOCAL_CFLAGS += -DLOCAL_SOCKET_SERVER


LOCAL_SRC_FILES := compress_simu.c ringbuffer.c mp3_dec.c

LOCAL_SHARED_LIBRARIES := \
			libutils \
			libtinyalsa \
			libcutils  \
			libaudioutils \
			libdl \
 			liblog

LOCAL_C_INCLUDES += \
    external/tinyalsa/include \
    external/expat/lib \
    external/tinycompress/include \
    vendor/sprd/modules/audio/normal/DumpData \
    system/media/audio_utils/include

ifeq ($(strip $(AUDIOSERVER_MULTILIB)),)
LOCAL_MULTILIB := 32
else
LOCAL_MULTILIB := $(AUDIOSERVER_MULTILIB)
LOCAL_CFLAGS += -DSET_OFFLOAD_AFFINITY
endif
LOCAL_MODULE_TAGS := optional

ifneq ($(filter $(strip $(PLATFORM_VERSION)),O 8.0.0 8.1.0),)
LOCAL_CFLAGS += -DFLAG_VENDOR_ETC
LOCAL_PROPRIETARY_MODULE := true
endif

ifneq ($(filter $(strip $(PLATFORM_VERSION)),4.4.4),)
LOCAL_CFLAGS += -DANDROID4X
endif

include $(BUILD_SHARED_LIBRARY)


