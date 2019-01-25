SUFFIX_NAME := .ini
CALIBRATION_NAME := connectivity_calibration
CONFIGURE_NAME := connectivity_configure

CALIBRATION_SRC := $(LOCAL_PATH)/$(SPRD_WCN_HW_CONFIG)/$(addsuffix $(SUFFIX_NAME),$(basename $(CALIBRATION_NAME)))

ifeq ($(strip $(WCN_EXTENSION)),)
    WCN_EXTENSION := true
endif

ifeq (,$(wildcard $(CALIBRATION_SRC)))
# configuration file does not exist. Use default one
CALIBRATION_SRC := $(LOCAL_PATH)/default/$(CALIBRATION_NAME)$(SUFFIX_NAME)
endif

CONFIGURE_SRC := $(LOCAL_PATH)/$(SPRD_WCN_HW_CONFIG)/$(addsuffix $(SUFFIX_NAME),$(basename $(CONFIGURE_NAME)))

ifeq (,$(wildcard $(CONFIGURE_SRC)))
# configuration file does not exist. Use default one
CONFIGURE_SRC := $(LOCAL_PATH)/default/$(CONFIGURE_NAME)$(SUFFIX_NAME)
endif

SPRD_WCN_ETC_PATH ?= vendor/etc

PRODUCT_COPY_FILES += \
    $(CONFIGURE_SRC):$(SPRD_WCN_ETC_PATH)/connectivity_configure.ini        \
    $(CALIBRATION_SRC):$(SPRD_WCN_ETC_PATH)/connectivity_calibration.ini    \
    frameworks/native/data/etc/android.hardware.bluetooth_le.xml:vendor/etc/permissions/android.hardware.bluetooth_le.xml

PRODUCT_PROPERTY_OVERRIDES += \
    ro.wcn.hardware.product=$(SPRD_WCN_HW_MODEL) \
    ro.wcn.hardware.etcpath=/$(SPRD_WCN_ETC_PATH) \
    ro.bt.bdaddr_path="/data/misc/bluedroid/btmac.txt"
