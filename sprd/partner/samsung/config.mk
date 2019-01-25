BOARD_USES_SAMSUNG_NFC := true
BOARD_USES_SAMSUNG_NFC_DTA := true
BOARD_USES_SAMSUNG_NFC_DTA_64 := false

BOARD_SEPOLICY_DIRS  += \
    vendor/sprd/partner/samsung/device/samsung/sepolicy

#NFC packages
PRODUCT_PACKAGES += \
    libnfc-sec \
    libnfc_sec_jni \
    nfc_nci.sec \
    NfcSec \
    Tag \
    com.android.nfc_extras \
    com.android.secnfc

#NFCEE access control
ifeq ($(TARGET_BUILD_VARIANT),user)
    NFCEE_ACCESS_PATH := vendor/sprd/partner/samsung/device/samsung/nfc/nfcee_access.xml
else
    NFCEE_ACCESS_PATH := vendor/sprd/partner/samsung/device/samsung/nfc/nfcee_access_debug.xml
endif

#NFC access control + feature files + configuration
PRODUCT_COPY_FILES += \
    $(NFCEE_ACCESS_PATH):system/etc/nfcee_access.xml \
    frameworks/native/data/etc/com.android.nfc_extras.xml:system/etc/permissions/com.android.nfc_extras.xml \
    frameworks/native/data/etc/android.hardware.nfc.xml:system/etc/permissions/android.hardware.nfc.xml \
    frameworks/native/data/etc/android.hardware.nfc.hce.xml:system/etc/permissions/android.hardware.nfc.hce.xml \
    frameworks/base/secnfc/com.android.secnfc.xml:system/etc/permissions/com.android.secnfc.xml \
    vendor/sprd/partner/samsung/device/samsung/nfc/libnfc-sec.conf:system/etc/libnfc-sec.conf \
    vendor/sprd/partner/samsung/device/samsung/nfc/libnfc-sec-hal.conf:system/etc/libnfc-sec-hal.conf \
    vendor/sprd/partner/samsung/device/samsung/nfc/sec_s3nrn81_firmware.bin:system/vendor/firmware/sec_s3nrn81_firmware.bin \
    vendor/sprd/partner/samsung/device/samsung/nfc/sec_s3nrn81_rfreg.bin:system/etc/sec_s3nrn81_rfreg.bin
