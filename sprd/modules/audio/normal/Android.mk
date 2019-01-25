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

ifeq ($(strip $(BOARD_USES_TINYALSA_AUDIO)),true)
ifneq ($(strip $(USE_AUDIO_WHALE_HAL)),true)
LOCAL_PATH := $(call my-dir)


#TinyAlsa audio

include $(CLEAR_VARS)

LOCAL_MODULE := audio.primary.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_CFLAGS := -D_POSIX_SOURCE -Wno-multichar -g

ifeq ($(strip $(BOARD_USES_LINE_CALL)), true)
LOCAL_CFLAGS += -D_VOICE_CALL_VIA_LINEIN
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),sc8830)
LOCAL_CFLAGS += -DAUDIO_SPIPE_TD
LOCAL_CFLAGS += -D_LPA_IRAM
endif

LOCAL_CFLAGS += -DAUDIO_DEBUG

ifeq ($(strip $(SPRD_VBC_DEEPBUFFER_MIXER)),true)
LOCAL_CFLAGS += -DAUDIO_VBC_DEEPBUFFER_MIXER
endif

ifeq ($(strip $(SPRD_AUDIO_VOIP_VERSION)),v2)
LOCAL_CFLAGS += -DAUDIO_VOIP_VERSION_2
endif

ifneq ($(filter scx35_sc9620referphone scx35_sc9620openphone scx35_sc9620openphone_zt, $(TARGET_BOARD)),)
LOCAL_CFLAGS += -DVB_CONTROL_PARAMETER_V2
endif

ifeq ($(strip $(AUDIO_CONTROL_PARAMETER_V2)), true)
LOCAL_CFLAGS += -DVB_CONTROL_PARAMETER_V2
endif
ifeq ($(strip $(TARGET_BOARD_PLATFORM)),scx15)
LOCAL_CFLAGS += -DAUDIO_SPIPE_TD
LOCAL_CFLAGS += -D_LPA_IRAM
endif

ifeq ($(strip $(BOARD_USES_SS_VOIP)), true)
# Default case, Nothing to do.
else
LOCAL_CFLAGS += -DVOIP_DSP_PROCESS
endif

ifeq ($(strip $(BOARD_AUDIO_OLD_MODEM)), true)
LOCAL_CFLAGS += -DAUDIO_OLD_MODEM
endif

ifeq ($(strip $(USE_PCM_IRQ_MODE)), true)
LOCAL_CFLAGS += -DUSE_PCM_IRQ_MODE
endif

ifeq ($(strip $(ENABLE_DEVICES_CTL_ON)), true)
LOCAL_CFLAGS += -DENABLE_DEVICES_CTL_ON
endif

LOCAL_C_INCLUDES += \
    external/tinyalsa/include \
    external/tinycompress/include \
    external/expat/lib \
    system/media/audio_utils/include \
    system/media/audio_effects/include \
    $(LOCAL_PATH)/vb_pga \
    $(LOCAL_PATH)/record_process \
    $(LOCAL_PATH)/include \
    $(LOCAL_PATH)/DumpData \
    $(LOCAL_PATH)/audiotest \
    $(LOCAL_PATH)/record_nr \
    $(LOCAL_PATH)/skd \
    $(LOCAL_PATH)/resample_api\
    $(LOCAL_PATH)/custom_mmi \
    $(LOCAL_PATH)/HuaWeiLaoHua \
    vendor/sprd/modules/resampler \
    $(LOCAL_PATH)/smart_amp/inc

ifneq ($(filter $(strip $(PLATFORM_VERSION)),4.4.4),)
LOCAL_CFLAGS += -DANDROID4X
LOCAL_C_INCLUDES += \
    vendor/sprd/modules/libatci \
    vendor/sprd/proprietories-source/engmode
USE_ALSA_PCM_UTIL_FILE := true
else

ifneq ($(filter $(strip $(PLATFORM_VERSION)),5.0 5.1),)
LOCAL_C_INCLUDES += \
    vendor/sprd/open-source/libs/libatchannel

LOCAL_CFLAGS += -DANDROID5X
LOCAL_CFLAGS += -DUSE_RESAMPLER_44_1_K
else

ifneq ($(filter $(strip $(PLATFORM_VERSION)),6.0 6.0.1),)
ifneq ($(filter $(strip $(TARGET_PROVIDES_B2G_INIT_RC)),true),)
#for KAIOS
LOCAL_C_INCLUDES += \
    vendor/sprd/modules/libatci \
    vendor/sprd/proprietories-source/engmode
else
#for Android
LOCAL_C_INCLUDES += \
    vendor/sprd/open-source/libs/libatci \
    vendor/sprd/open-source/apps/engmode
endif
LOCAL_CFLAGS += -DANDROID6X
LOCAL_CFLAGS += -DUSE_RESAMPLER_44_1_K
else
LOCAL_C_INCLUDES += \
    vendor/sprd/modules/libatci \
    vendor/sprd/proprietories-source/engmode

USE_ALSA_PCM_UTIL_FILE := true
LOCAL_CFLAGS += -DAUDIO_HAL_ANDROID_N_API
endif

endif

endif

BOARD_EQ_DIR := v2

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),sc8830)
BOARD_EQ_DIR := v2
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),scx15)
BOARD_EQ_DIR := v2
endif

LOCAL_C_INCLUDES += $(LOCAL_PATH)/vb_effect/$(BOARD_EQ_DIR)

ifeq ($(strip $(AUDIO_SMART_PA_TYPE)), NXP)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/nxppa
endif

LOCAL_SRC_FILES := \
    audio_hw.c \
    tinyalsa_util.c \
    audio_pga.c \
    record_process/aud_proc_config.c.arm \
    record_process/aud_filter_calc.c.arm \
    record_process/record_nr_api.c \
    skd/ring_buffer.c \
    sprd_cp_control.c

ifeq ($(strip $(AUDIO_MUX_PIPE)), true)
LOCAL_SRC_FILES  += audio_mux_pcm.c
LOCAL_CFLAGS += -DAUDIO_MUX_PCM
endif

ifeq ($(strip $(USE_ALSA_PCM_UTIL_FILE)), true)
LOCAL_SRC_FILES  += \
    alsa_pcm_util.c
endif



ifeq ($(strip $(AUDIO_SMART_PA_TYPE)), NXP)
LOCAL_CFLAGS += -DNXP_SMART_PA
endif

ifeq ($(strip $(AUDIO_SMART_PA_RE_CALIB)), true)
LOCAL_CFLAGS += -DNXP_SMART_PA_RE_CALIB
endif

ifeq ($(strip $(AUDIO_RECORD_NR)),true)
LOCAL_CFLAGS += -DRECORD_NR
endif

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libcutils \
    libtinyalsa \
    libaudioutils \
    libexpat \
    libdl \
    libvbeffect \
    libvbpga \
    libnvexchange \
    libdumpdata\
    libhardware_legacy \
    libutils \
    libFMHalSource \
    libAudioCustomMmi \
    libresample48kto44k \
    libcompresssimu    \
    libaudiomiscctl  \
    libsmartamp

ifneq ($(filter $(strip $(PLATFORM_VERSION)),O 8.0.0 8.1.0),)
SUPPORT_OLD_AUDIO_TEST_INTERFACE :=false
SPRD_AUDIO_TEST_INTERFACE := v2
else
ifneq ($(filter $(strip $(PLATFORM_VERSION)),4.4.4),)
SUPPORT_OLD_AUDIO_TEST_INTERFACE :=false
SPRD_AUDIO_TEST_INTERFACE := v2
else
SUPPORT_OLD_AUDIO_TEST_INTERFACE :=true
LOCAL_CFLAGS += -DSUPPORT_OLD_AUDIO_TEST_INTERFACE
endif
endif

ifeq ($(strip $(SPRD_AUDIO_TEST_INTERFACE)),v2)
LOCAL_CFLAGS += -DAUDIO_TEST_INTERFACE_V2
endif

ifneq ($(filter $(strip $(PLATFORM_VERSION)),5.0 5.1),)
LOCAL_SHARED_LIBRARIES += \
    libatchannel libmedia

LOCAL_SRC_FILES += \
    audiotest/auto_audio.cpp

else
LOCAL_SHARED_LIBRARIES += \
    libatci

ifeq ($(strip $(SUPPORT_OLD_AUDIO_TEST_INTERFACE)),true)
LOCAL_SHARED_LIBRARIES += libmedia
LOCAL_SRC_FILES += \
    audiotest/auto_audio_v2.cpp
endif

ifneq ($(filter $(strip $(PLATFORM_VERSION)),O 8.0.0 8.1.0),)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_CFLAGS += -DFLAG_VENDOR_ETC
endif



#when use google fm solution to set below the macro
#LOCAL_CFLAGS += -DFM_VERSION_IS_GOOGLE
#### android 6.x Version
endif

ifeq ($(strip $(SPRD_VBC_NOT_USE_AD23)),true)
LOCAL_CFLAGS += -DVBC_NOT_SUPPORT_AD23
endif

ifeq ($(strip $(AUDIOSERVER_MULTILIB)),)
LOCAL_32_BIT_ONLY := true
else
LOCAL_MULTILIB := $(AUDIOSERVER_MULTILIB)
#LOCAL_CFLAGS += -DSUPPORT_64BIT_HAL
endif
LOCAL_LDFLAGS += -Wl,--no-warn-shared-textrel
LOCAL_STATIC_LIBRARIES := \
    libSprdRecordNrProcess \
    HuaweiApkAudioTest

ifeq ($(strip $(AUDIO_SMART_PA_TYPE)), NXP)
LOCAL_SHARED_LIBRARIES += \
    libnxppa \
    libpaFmTrack \
    libfminterface \
    libFMHalSource
endif

LOCAL_MODULE_TAGS := optional
ifneq ($(filter $(strip $(PLATFORM_VERSION)),4.4.4),)
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
endif
include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))
endif
endif
