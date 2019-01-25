#Copyright(C)2008 The Android Open Source Project
#
#Licensed under the Apache License, Version 2.0(the "License");
#you may not use this file except in compliance with the License.
#You may obtain a copy of the License at
#
#http:  // www.apache.org/licenses/LICENSE-2.0
#
#Unless required by applicable law or agreed to in writing, software
#distributed under the License is distributed on an "AS IS" BASIS,
#WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#See the License for the specific language governing permissions and
#limitations under the License.

LOCAL_PATH := $(call my-dir)
ifeq ($(USE_SPRD_SENSOR_LIB),true)
ifneq ($(TARGET_SIMULATOR),true)
ifneq ($(USE_INVENSENSE_LIB),true)

include $(CLEAR_VARS)
LOCAL_SRC_FILES		:= mag/libcalmodule_akm_32.a
LOCAL_MODULE 		:= libSmartCompass_32
LOCAL_MULTILIB		:= 32
LOCAL_MODULE_CLASS 	:= STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX	:= .a
LOCAL_MODULE_TAGS 	:= optional
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_SRC_FILES		:= mag/libcalmodule_akm_64.a
LOCAL_MODULE 		:= libSmartCompass_64
LOCAL_MULTILIB		:= 64
LOCAL_MODULE_CLASS 	:= STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX	:= .a
LOCAL_MODULE_TAGS 	:= optional
include $(BUILD_PREBUILT)

#HAL module implemenation, not prelinked, and stored in
#hw / < SENSORS_HARDWARE_MODULE_ID >.< ro.product.board >.so
include $(CLEAR_VARS)

LOCAL_MODULE := sensors.$(TARGET_BOARD_PLATFORM)

LOCAL_MODULE_RELATIVE_PATH := hw

LOCAL_MODULE_TAGS := optional

LOCAL_PROPRIETARY_MODULE := true

LOCAL_CFLAGS := -DLOG_TAG=\"sensor\" \
                                -Wall

LOCAL_SRC_FILES := \
                        SensorBase.cpp \
                        SensorCoordinate.cpp \
                        InputEventReader.cpp \
                        sensors.cpp

#################################################################
#ACCELERATION
ifneq ($(BOARD_HAVE_ACC),NULL)
LOCAL_SRC_FILES += acc/Acc_$(BOARD_HAVE_ACC).cpp
LOCAL_CFLAGS += -DACC_INSTALL_$(BOARD_ACC_INSTALL)
else
LOCAL_CFLAGS += -DACC_NULL
endif

#################################################################
#MAGNETIC
ifneq ($(BOARD_HAVE_ORI),NULL)
LOCAL_SRC_FILES += mag/Ori_$(BOARD_HAVE_ORI).cpp
LOCAL_CFLAGS += -DORI_INSTALL_$(BOARD_ORI_INSTALL)
else
LOCAL_CFLAGS += -DORI_NULL
endif

#################################################################
#PROXIMITY & LIGHT Sensors ( PLS )
ifeq ($(BOARD_PLS_COMPATIBLE),true)
LOCAL_SRC_FILES += pls/PlsSensor.cpp
LOCAL_CFLAGS += -DPLS_COMPATIBLE
else

ifneq ($(BOARD_HAVE_PLS),NULL)
LOCAL_SRC_FILES += pls/Pls_$(BOARD_HAVE_PLS).cpp
else
LOCAL_CFLAGS += -DPLS_NULL
endif
endif

LOCAL_SHARED_LIBRARIES := liblog libcutils libdl
LOCAL_PRELINK_MODULE := false

ifdef TARGET_2ND_ARCH
LOCAL_STATIC_LIBRARIES_32 += libSmartCompass_32
LOCAL_STATIC_LIBRARIES_64 += libSmartCompass_64
endif

include $(BUILD_SHARED_LIBRARY)

endif # !USE_INVENSENSE_LIB
endif # !TARGET_SIMULATOR
endif # USE_SPRD_SENSOR_LIB
