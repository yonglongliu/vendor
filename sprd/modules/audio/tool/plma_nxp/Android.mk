ifeq ($(strip $(AUDIO_SMART_PA_TYPE)), NXP)
LOCAL_PATH:= $(call my-dir)

#NOTE: for now we use static lib

############################## libtfa98xx
include $(CLEAR_VARS)

LOCAL_C_INCLUDES:= 	${LOCAL_PATH}/tfa/inc \
			${LOCAL_PATH}/tfa/linux-user-inc \
			${LOCAL_PATH}/tfa/src \
			${LOCAL_PATH}/utl/inc \
			${LOCAL_PATH}/hal/inc \
			${LOCAL_PATH}/hal/src \
			${LOCAL_PATH}/hal/src/lxScribo \
			${LOCAL_PATH}/srv/inc \
			${LOCAL_PATH}/srv/src/iniFile

LOCAL_SRC_FILES:= 	hal/src/NXP_I2C.c  \
			hal/src/lxScribo/hal_utils.c  \
			hal/src/lxScribo/lxDummy.c  \
			hal/src/lxScribo/lxScribo.c \
			hal/src/lxScribo/lxScriboProtocol.c  \
			hal/src/lxScribo/lxScriboProtocolUdp.c  \
			hal/src/lxScribo/lxScriboSerial.c  \
			hal/src/lxScribo/lxScriboSocket.c\
			hal/src/lxScribo/udp_sockets.c  \
			hal/src/lxScribo/lxI2c.c \
			hal/src/lxScribo/lxSysfs.c \
			tfa/src/tfa_debug.c \
			tfa/src/tfa_hal.c \
			tfa/src/tfa_osal.c \
			tfa/src/tfa_dsp.c \
			tfa/src/tfa_container.c \
			tfa/src/tfa_container_crc32.c \
			tfa/src/tfa_init.c \
			utl/src/tfaOsal.c \
			srv/src/tfaContainerWrite.c \
			srv/src/tfaContUtil.c \
			srv/src/iniFile/minIni.c \
			srv/src/Tfa98API.c \
			srv/src/tfa98xxDRC.c \
			srv/src/tfa98xxFilterCalculations.c \
			srv/src/tfa98xxCalculations.c \
			srv/src/tfa98xxCalibration.c \
			srv/src/tfa98xxLiveData.c \
			srv/src/tfa98xxDiagnostics.c

LOCAL_MODULE := libtfa98xx
LOCAL_SHARED_LIBRARIES:= libcutils libutils libdl
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false
LOCAL_CFLAGS := -D__ANDROID__ -DTFA_DEBUG -DLXSCRIBO -DHAL_SYSFS
LOCAL_CFLAGS += -DTFA98XX_GIT_VERSIONS=\"$(GIT_VERSION)\"

include $(BUILD_STATIC_LIBRARY)

############################## cli app
include $(CLEAR_VARS)

LOCAL_C_INCLUDES:= 	${LOCAL_PATH}/app/climax/src/cli \
			${LOCAL_PATH}/app/climax/inc \
			${LOCAL_PATH}/srv/inc \
			${LOCAL_PATH}/tfa/inc \
			${LOCAL_PATH}/tfa/linux-user-inc \
			${LOCAL_PATH}/utl/inc \
			${LOCAL_PATH}/hal/inc \
			${LOCAL_PATH}/hal/src \
			${LOCAL_PATH}/hal/src/lxScribo/scribosrv \
			${LOCAL_PATH}/hal/src/lxScribo \

LOCAL_SRC_FILES:= 	app/climax/src/climax.c \
			app/climax/src/cliCommands.c \
			app/climax/src/cli/cmdline.c \
			hal/src/lxScribo/scribosrv/i2cserver.c \
			hal/src/lxScribo/scribosrv/cmd.c

LOCAL_MODULE := climax
LOCAL_SHARED_LIBRARIES:= libcutils libutils libdl
LOCAL_STATIC_LIBRARIES:= libtfa98xx  
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false
LOCAL_CFLAGS := -D__ANDROID__
LOCAL_CFLAGS += -DTFA98XX_GIT_VERSIONS=\"$(GIT_VERSION)\"

include $(BUILD_EXECUTABLE)

endif
