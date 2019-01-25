#
# Spreadtrum WCN configuration
#

# Check envionment variables
ifeq ($(TARGET_BOARD),)
$(error WCN connectivity.mk should include after TARGET_BOARD)
endif

ifeq ($(BOARD_HAVE_SPRD_WCN_COMBO),)
$(error PRODUCT MK should have BOARD_HAVE_SPRD_WCN_COMBO config)
endif

# Config
# Help fix the board conflict without TARGET_BOARD
ifeq ($(SPRD_WCN_HW_CONFIG),)
SPRD_WCN_HW_CONFIG := $(TARGET_BOARD)
endif

# Legacy
# Help compatible with legacy project
ifeq ($(PLATFORM_VERSION),6.0)
SPRD_WCN_ETC_PATH := system/etc
endif

ifeq ($(PLATFORM_VERSION),4.4.4)
SPRD_WCN_ETC_PATH := system/etc
endif

SPRD_WCNBT_CHISET := $(BOARD_HAVE_SPRD_WCN_COMBO)
SPRD_WCN_HW_MODEL := $(BOARD_HAVE_SPRD_WCN_COMBO)

$(call inherit-product, vendor/sprd/modules/wcn/connconfig/$(SPRD_WCN_HW_MODEL)/connectivity.mk)
BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := vendor/sprd/modules/wcn/bt/libbt/conf/sprd/$(SPRD_WCNBT_CHISET)/include \
                                               vendor/sprd/modules/wcn/bt/libbt/include

PRODUCT_PACKAGES += \
    hcidump \
    libbqbbt \
    libbt-vendor \
    btools \
    android.hardware.bluetooth@1.0-impl \
    android.hardware.bluetooth@1.0-service \
    libbt-sprd_suite \
    libbt-sprd_eut

ifeq ($(strip $(SPRD_WCN_HW_MODEL)),marlin2)
PRODUCT_PACKAGES += libwifieut
else ifeq ($(strip $(SPRD_WCN_HW_MODEL)),pike2)
PRODUCT_PACKAGES += libwifieut
else ifeq ($(strip $(SPRD_WCN_HW_MODEL)),sharkle)
PRODUCT_PACKAGES += libwifieut
else ifeq ($(strip $(SPRD_WCN_HW_MODEL)),sharkl3)
PRODUCT_PACKAGES += libwifieut
else ifeq ($(strip $(SPRD_WCN_HW_MODEL)),sharklep)
PRODUCT_PACKAGES += libwifieut
endif
