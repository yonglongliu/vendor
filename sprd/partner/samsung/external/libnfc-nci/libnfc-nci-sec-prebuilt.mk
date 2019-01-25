# Static library libnfc-nci-sec.a contains DTA.
# This source code shall not be released to customer,
# so they are put in this static lib.

# Developer must copy the prebuilt libnfc-nci-sec.a into
# external/libnfc-nci/

include $(CLEAR_VARS)
LOCAL_MODULE := libnfc-nci-sec-dta-32
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX := .a

########################################
# SAMSUNG DTA Configuration
ifeq ($(BOARD_USES_SAMSUNG_NFC_DTA), true)
ifeq ($(BOARD_USES_SAMSUNG_NFC_DTA_64), true)
LOCAL_SRC_FILES := libnfc-nci-sec-dta-64.a
else
LOCAL_SRC_FILES := libnfc-nci-sec-dta-32.a
endif
endif

include $(BUILD_PREBUILT)
