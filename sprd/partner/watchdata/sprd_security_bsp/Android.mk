#only compiled when TEE configure to watchdata
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

#check target board and incude the proper version of bsp modules
#target board is for iwhale2
ifneq ($(findstring 9861e,$(TARGET_BOARD)),)
include $(LOCAL_PATH)/firewall/iwhale2/Android.mk
include $(LOCAL_PATH)/crypto/sprd/r2p0/Android.mk
include $(LOCAL_PATH)/efuse/iwhale2/Android.mk
include $(LOCAL_PATH)/factory/iwhale2/Android.mk
endif

#target board is for isharkl2
ifneq ($(findstring 9853i,$(TARGET_BOARD)),)
include $(LOCAL_PATH)/efuse/isharkl2/Android.mk
endif

#target board if for sharkl2
ifneq ($(findstring 9850ka,$(TARGET_BOARD)),)
include $(LOCAL_PATH)/crypto/sprd/r3p0/Android.mk
endif
