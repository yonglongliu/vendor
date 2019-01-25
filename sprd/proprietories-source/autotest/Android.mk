#
#
#autotest makefile
#

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(AUTOTST_DEVLP_DBG),true)
LOCAL_CPPFLAGS += -DAUTOTST_DEVLP_DBG
endif

LOCAL_CFLAGS  += -lpthread
ifeq ($(BOARD_SPRD_WCNBT_MARLIN),true)
LOCAL_CFLAGS  += -DSPRD_WCNBT_MARLIN
endif

ifeq ($(USE_BBAT_WHALE),true)
LOCAL_CFLAGS  += -DUSE_BBAT_WHALE
endif

ifeq ($(BOARD_SPRD_WCNBT_SR2351),true)
LOCAL_CFLAGS += -DSPRD_WCNBT_SR2351
endif
# brcm android7.0 not support temporary
ifeq ($(BOARD_HAVE_BLUETOOTH_BCM),true)
LOCAL_CFLAGS += -DBOARD_HAVE_FM_BCM_70
endif

LOCAL_CFLAGS += -DGOOGLE_FM_INCLUDED

LOCAL_MODULE_TAGS:= optional

LOCAL_MODULE:= autotest

LOCAL_INIT_RC := autotest.rc

ifeq ($(SHARKLJ1_BBAT_BIT64), true)
LOCAL_32_BIT_ONLY := false
LOCAL_CFLAGS += -DSHARKLJ1_BBAT_BIT64
else
LOCAL_32_BIT_ONLY := true
LOCAL_CFLAGS += -DSHARKLJ1_BBAT_BIT32
endif

#LOCAL_SRC_FILES:= \
    atv.cpp \
    audio.cpp \
    bt_fm_mixed.cpp \
    fm.cpp \
    bt.cpp \
    perm.cpp \
    sensor.cpp

LOCAL_SRC_FILES:= \
    autotest_modules.cpp \
    otg.cpp \
    fm.cpp  \
    sensor.cpp \
    bt.cpp \
    battery.cpp  \
    camera.cpp   \
    channel.cpp  \
    cmmb.cpp     \
    debug.cpp  \
    diag.cpp   \
    driver.cpp \
    gps.cpp    \
    input.cpp  \
    tp.cpp    \
    lcd.cpp    \
    light.cpp  \
    sim.cpp    \
    tcard.cpp  \
    tester.cpp \
    tester_main.cpp \
    tester_dbg.cpp  \
    util.cpp        \
    ver.cpp         \
    vibrator.cpp    \
    wifi.cpp        \
    power.cpp       \
    ui.c  \
    mipi_lcd.c   \
    key_common.cpp	\
    events.c	\
    gsensor.cpp   \
    msensor.cpp   \
    psensor.cpp   \
    asensor.cpp   \
    lsensor.cpp   \
    testcomm.c  \
    main.cpp


LOCAL_C_INCLUDES:= \
    external/bluetooth/bluez/lib \
    external/bluetooth/bluez/src \
    frameworks/base/include \
    frameworks/av/include \
    system/bluetooth/bluedroid/include \
    system/core/include \
    hardware/libhardware/include \
    hardware/libhardware_legacy/include \
    external/bluetooth/bluedroid/btif/include \
    external/bluetooth/bluedroid/gki/ulinux \
    external/bluetooth/bluedroid/stack/include \
    external/bluetooth/bluedroid/stack/btm \
    $(TARGET_OUT_INTERMEDIATES)/KERNEL/usr/include/video
LOCAL_C_INCLUDES    +=  vendor/sprd/proprietories-source/autotest/minui \
			vendor/sprd/proprietories-source/autotest/res \
          vendor/sprd/proprietories-source/autotest/ion \
          system/core/libpixelflinger/include \
	    vendor/sprd/modules/libatci
#    libbluedroid \
#    libbluetooth \
#    libbluetoothd\
LOCAL_FORCE_STATIC_EXECUTABLE := true

#support:NXP, SAMSUNG
BOARD_NFC_SUPPORT := SAMSUNG

ifeq ($(BOARD_NFC_SUPPORT),NXP)
    LOCAL_SRC_FILES += nfc/nxp/nfc.cpp
    #copy NXP tools and script
    $(shell mkdir -p $(TARGET_OUT_ETC)/nfc)
    $(shell cp -ur "system/mmitest/nfc_test/Antenna Evaluation Scripts/Reader_mode.txt" $(TARGET_OUT_ETC)/nfc/)
    $(shell cp -ur "system/mmitest/nfc_test/pnscr" $(TARGET_OUT_EXECUTABLES)/)
endif

ifeq ($(BOARD_NFC_SUPPORT),SAMSUNG)
    LOCAL_SRC_FILES += nfc/samsung/nfc.cpp
endif

LOCAL_SHARED_LIBRARIES:= \
    libbinder \
    libatci \
    liblog \
    libcamera_client \
    libcutils    \
    libdl        \
    libgui       \
    libhardware  \
    libhardware_legacy \
    libmedia \
    libui    \
    libutils \
    libpixelflinger

LOCAL_STATIC_LIBRARIES:= \
    libatminui \
    libpng \
    libcutils \
    libz \
    liblog \
    libm \
    libmtdutils
include $(BUILD_EXECUTABLE)
include $(call all-makefiles-under,$(LOCAL_PATH))
