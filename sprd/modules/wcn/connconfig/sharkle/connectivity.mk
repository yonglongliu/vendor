CONNECTIVITY_OWN_FILES := \
    connectivity_calibration.ini \
    connectivity_configure.ini

SUFFIX_AD_NAME := .ad.ini
SPRD_WCN_ETC_PATH ?= vendor/etc

SPRD_WCN_ETC_AD_PATH := $(SPRD_WCN_ETC_PATH)/wcn

GENERATE_WCN_PRODUCT_COPY_FILES += $(foreach own, $(CONNECTIVITY_OWN_FILES), \
    $(if $(wildcard $(LOCAL_PATH)/$(SPRD_WCN_HW_CONFIG)/$(own)), \
        $(LOCAL_PATH)/$(SPRD_WCN_HW_CONFIG)/$(own):$(SPRD_WCN_ETC_PATH)/$(own), \
        $(LOCAL_PATH)/default/$(own):$(SPRD_WCN_ETC_PATH)/$(own)))

GENERATE_WCN_PRODUCT_COPY_AD_FILE += $(foreach own, $(CONNECTIVITY_OWN_FILES), \
    $(if $(wildcard $(LOCAL_PATH)/$(SPRD_WCN_HW_CONFIG)/$(addsuffix $(SUFFIX_AD_NAME), $(basename $(own)))), \
        $(LOCAL_PATH)/$(SPRD_WCN_HW_CONFIG)/$(addsuffix $(SUFFIX_AD_NAME), $(basename $(own))):$(SPRD_WCN_ETC_AD_PATH)/$(own), \
        $(LOCAL_PATH)/default/$(own):$(SPRD_WCN_ETC_AD_PATH)/$(own)))

PRODUCT_COPY_FILES += \
    $(GENERATE_WCN_PRODUCT_COPY_FILES) \
    $(GENERATE_WCN_PRODUCT_COPY_AD_FILE) \
    frameworks/native/data/etc/android.hardware.bluetooth_le.xml:vendor/etc/permissions/android.hardware.bluetooth_le.xml

PRODUCT_PROPERTY_OVERRIDES += \
    ro.wcn.hardware.product=$(SPRD_WCN_HW_MODEL) \
    ro.wcn.hardware.etcpath=/$(SPRD_WCN_ETC_PATH) \
    ro.bt.bdaddr_path="/data/misc/bluedroid/btmac.txt"

PRODUCT_PACKAGES += \
    sprdbt_tty.ko \
    sprd_fm.ko
