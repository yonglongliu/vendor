CONNECTIVITY_OWN_FILES := \
    connectivity_calibration.ini \
    connectivity_configure.ini

SPRD_WCN_ETC_PATH ?= vendor/etc

GENERATE_WCN_PRODUCT_COPY_FILES += $(foreach own, $(CONNECTIVITY_OWN_FILES), \
    $(if $(wildcard $(LOCAL_PATH)/$(SPRD_WCN_HW_CONFIG)/$(own)), \
        $(LOCAL_PATH)/$(SPRD_WCN_HW_CONFIG)/$(own):$(SPRD_WCN_ETC_PATH)/$(own), \
        $(LOCAL_PATH)/default/$(own):$(SPRD_WCN_ETC_PATH)/$(own)))

PRODUCT_COPY_FILES += \
    $(GENERATE_WCN_PRODUCT_COPY_FILES) \
    frameworks/native/data/etc/android.hardware.bluetooth_le.xml:vendor/etc/permissions/android.hardware.bluetooth_le.xml


PRODUCT_PROPERTY_OVERRIDES += \
    ro.wcn.hardware.product=$(SPRD_WCN_HW_MODEL) \
    ro.wcn.hardware.etcpath=/$(SPRD_WCN_ETC_PATH) \
    ro.bt.bdaddr_path="/data/misc/bluedroid/btmac.txt"

PRODUCT_PACKAGES += \
    sprdbt_tty.ko \
    sprd_fm.ko
