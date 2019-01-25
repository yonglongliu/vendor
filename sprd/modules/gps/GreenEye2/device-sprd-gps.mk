TARGET_VARIANTS := x86 x86_64
ifdef TARGET_ARCH
SPRD_GNSS_ARCH :=$(TARGET_ARCH)
endif

ifeq ($(strip $(SPRD_GNSS_SHARKLE_PIKL2)),true)
$(warning shell echo "it should sharkle or pik2!")
PRODUCT_COPY_FILES += \
	vendor/sprd/modules/gps/GreenEye2/agnss/spirentroot.cer:/system/etc/spirentroot.cer \
	vendor/sprd/modules/gps/GreenEye2/ge2_libgps/supl.xml:/system/etc/supl.xml \
	vendor/sprd/modules/gps/GreenEye2/ge2_libgps/config.xml:/system/etc/config.xml
else
$(warning shell echo "it should normal GE2 version")
PRODUCT_COPY_FILES += \
	vendor/sprd/modules/gps/GreenEye2/bin/gnssmodem.bin:/vendor/etc/gnssmodem.bin \
	vendor/sprd/modules/gps/GreenEye2/bin/gnssbdmodem.bin:/vendor/etc/gnssbdmodem.bin \
	vendor/sprd/modules/gps/GreenEye2/bin/gnssfdl.bin:/vendor/etc/gnssfdl.bin \
	vendor/sprd/modules/gps/GreenEye2/agnss/spirentroot.cer:/vendor/etc/spirentroot.cer \
	vendor/sprd/modules/gps/GreenEye2/ge2_libgps/supl.xml:/vendor/etc/supl.xml \
	vendor/sprd/modules/gps/GreenEye2/ge2_libgps/config.xml:/vendor/etc/config.xml
endif

ifeq ($(SPRD_GNSS_ARCH), $(filter $(SPRD_GNSS_ARCH),$(TARGET_VARIANTS)))
PRODUCT_COPY_FILES += \
	vendor/sprd/modules/gps/GreenEye2/agnss/android80i/android80i_32bit/libsupl.so:/vendor/lib/libsupl.so \
	vendor/sprd/modules/gps/GreenEye2/agnss/android80i/android80i_32bit/liblcsagent.so:/vendor/lib/liblcsagent.so \
	vendor/sprd/modules/gps/GreenEye2/agnss/android80i/android80i_32bit/liblcscp.so:/vendor/lib/liblcscp.so \
	vendor/sprd/modules/gps/GreenEye2/agnss/android80i/android80i_32bit/liblcswbxml2.so:/vendor/lib/liblcswbxml2.so \
    vendor/sprd/modules/gps/GreenEye2/agnss/android80i/android80i_32bit/liblcsmgt.so:/vendor/lib/liblcsmgt.so \
    vendor/sprd/modules/gps/GreenEye2/agnss/android80i/android80i_64bit/libsupl.so:/vendor/lib64/libsupl.so \
	vendor/sprd/modules/gps/GreenEye2/agnss/android80i/android80i_64bit/liblcsagent.so:/vendor/lib64/liblcsagent.so \
	vendor/sprd/modules/gps/GreenEye2/agnss/android80i/android80i_64bit/liblcscp.so:/vendor/lib64/liblcscp.so \
	vendor/sprd/modules/gps/GreenEye2/agnss/android80i/android80i_64bit/liblcswbxml2.so:/vendor/lib64/liblcswbxml2.so \
	vendor/sprd/modules/gps/GreenEye2/agnss/android80i/android80i_64bit/liblcsmgt.so:/vendor/lib64/liblcsmgt.so
else #arm arch
PRODUCT_COPY_FILES += \
	vendor/sprd/modules/gps/GreenEye2/agnss/android60/android60_32bit/libsupl.so:/system/lib/libsupl.so \
	vendor/sprd/modules/gps/GreenEye2/agnss/android60/android60_32bit/liblcsagent.so:/system/lib/liblcsagent.so \
	vendor/sprd/modules/gps/GreenEye2/agnss/android60/android60_32bit/liblcscp.so:/system/lib/liblcscp.so \
	vendor/sprd/modules/gps/GreenEye2/agnss/android60/android60_32bit/liblcswbxml2.so:/system/lib/liblcswbxml2.so \
        vendor/sprd/modules/gps/GreenEye2/agnss/android60/android60_32bit/liblcsmgt.so:/system/lib/liblcsmgt.so \
        vendor/sprd/modules/gps/GreenEye2/agnss/android60/android60_64bit/libsupl.so:/system/lib64/libsupl.so \
	vendor/sprd/modules/gps/GreenEye2/agnss/android60/android60_64bit/liblcsagent.so:/system/lib64/liblcsagent.so \
	vendor/sprd/modules/gps/GreenEye2/agnss/android60/android60_64bit/liblcscp.so:/system/lib64/liblcscp.so \
	vendor/sprd/modules/gps/GreenEye2/agnss/android60/android60_64bit/liblcswbxml2.so:/system/lib64/liblcswbxml2.so \
	vendor/sprd/modules/gps/GreenEye2/agnss/android60/android60_64bit/liblcsmgt.so:/system/lib64/liblcsmgt.so
endif

PRODUCT_PACKAGES += \
		gpsd
