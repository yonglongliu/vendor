EXTN_SAMSUNG_PATH:= $(call my-dir)

LOCAL_C_INCLUDES += \
    $(EXTN_SAMSUNG_PATH)/inc \
    $(EXTN_SAMSUNG_PATH)/src/common \
    $(EXTN_SAMSUNG_PATH)/src/log \
    $(EXTN_SAMSUNG_PATH)/src/mifare \
    $(EXTN_SAMSUNG_PATH)/src/utils

#LOCAL_CFLAGS += -DNXP_UICC_ENABLE

