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
CUR_DIR := otp_drv

LOCAL_SRC_DIR := $(LOCAL_PATH)/$(CUR_DIR)
LOCAL_C_INCLUDES += $(shell find $(LOCAL_SRC_DIR) -name '*.h' | sed -r 's/(.*)\//\1 /' | cut -d" " -f1 )

LOCAL_SRC_FILES += \
	otp_drv/otp_common.c

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),3)
LOCAL_SRC_FILES += \
	otp_drv/driver/imx258_truly/imx258_truly_otp_drv.c \
	otp_drv/driver/s5k3p8sm_truly/s5k3p8sm_truly_otp_drv.c
else
LOCAL_SRC_FILES += \
    otp_drv/driver/ov8856_shine/ov8856_shine_otp_drv.c \
	otp_drv/driver/gc2375_altek/gc2375_altek_otp_drv.c \
	otp_drv/driver/gc5024_common/gc5024_common_otp_drv.c \
	otp_drv/driver/imx258/imx258_otp_drv.c \
	otp_drv/driver/ov5675_sunny/ov5675_sunny_otp_drv.c \
	otp_drv/driver/ov13855/ov13855_otp_drv.c \
	otp_drv/driver/ov13855_altek/ov13855_altek_otp_drv.c \
	otp_drv/driver/ov13855_sunny/ov13855_sunny_otp_drv.c \
	otp_drv/driver/s5k3l8xxm3_qtech/s5k3l8xxm3_qtech_otp_drv.c \
	otp_drv/driver/s5k3l8xxm3_reachtech/s5k3l8xxm3_reachtech_otp_drv.c \
	otp_drv/driver/ov2680_cmk/ov2680_cmk_otp_drv.c \
	otp_drv/driver/ov8856_cmk/ov8856_cmk_otp_drv.c \
	otp_drv/driver/ov8858_cmk/ov8858_cmk_otp_drv.c \
	otp_drv/driver/sp8407/sp8407_otp_drv.c \
	otp_drv/driver/sp8407_cmk/sp8407_cmk_otp_drv.c \
	otp_drv/driver/s5k5e8yx_jd/s5k5e8yx_jd_otp_drv.c
endif
