#
# Copyright (C) 2015 The Android Open-Source Project
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

# WARNING: Everything listed here will be built on ALL platforms,
# including x86, the emulator, and the SDK.  Modules must be uniquely
# named (liblights.panda), and must build everywhere, or limit themselves
# to only building on ARM if they include assembly. Individual makefiles
# are responsible for having their own logic, for fine-grained control.

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

###
# keystore.trusty is the HAL used by keystore on Trusty devices.
##
ifneq (6.0,$(filter 6.0,$(PLATFORM_VERSION)))

ifeq (7.0,$(filter 7.0,$(PLATFORM_VERSION)))
CONFIG_KEYMASTER_VERSION := 1
else
CONFIG_KEYMASTER_VERSION := 2
endif

ifeq ($(strip $(BOARD_TEE_CONFIG)), trusty)
include $(CLEAR_VARS)
LOCAL_MODULE := keystore.sprdtrusty
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SRC_FILES := \
	module.cpp \
	trusty_keymaster_ipc.c \
	trusty_keymaster$(CONFIG_KEYMASTER_VERSION)_device.cpp
LOCAL_CLFAGS += -fvisibility=hidden -Wall -Werror
LOCAL_CFLAGS += -DTRUSTY_KEYMASTER_VERSION=$(CONFIG_KEYMASTER_VERSION)
LOCAL_SHARED_LIBRARIES := \
	libcrypto \
	libkeymaster_messages \
	libtrusty \
	liblog \
	libcutils
LOCAL_MODULE_TAGS := optional
LOCAL_POST_INSTALL_CMD = \
    $(hide) ln -sf $(notdir $(LOCAL_INSTALLED_MODULE)) $(dir $(LOCAL_INSTALLED_MODULE))keystore.$(TARGET_BOARD_PLATFORM).so
include $(BUILD_SHARED_LIBRARY)

include $(LOCAL_PATH)/tests/Android.mk
endif

endif
