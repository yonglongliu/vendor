VOWIFI_KAIOS_PRODUCT := true

MY_PATH := vendor/sprd/sprd_vowifi
OUT_PATH := /system/etc/com.spreadtrum.vowifi

$(shell mkdir -p $(OUT)/$(OUT_PATH))

# add for vowifi source code module

#ims bridge daemon
PRODUCT_PACKAGES += \
	    ims_bridged \
	    libimsbrd

#ip monitor
#PRODUCT_PACKAGES += ip_monitor.sh

# add for vowifi bin module
PRODUCT_PACKAGES += libavatar \
	   libzmf \
	   libCamdrv23 \
	   liblemon \
	   libmme_jrtc \
	   libvowifiservice \
	   libvowifiwrapper \
	   vowifisrvd \
	   ju_ipsec_server

# enable the Wi-Fi calling menu in settings.
PRODUCT_PROPERTY_OVERRIDES += persist.dbg.wfc_avail_ovr=1
# vowifi voice engine in ap/cp
PRODUCT_PROPERTY_OVERRIDES += persist.sys.vowifi.voice=cp

PRODUCT_COPY_FILES += \
	$(call find-copy-subdir-files,*,$(MY_PATH)/res,$(OUT_PATH)/conf)
