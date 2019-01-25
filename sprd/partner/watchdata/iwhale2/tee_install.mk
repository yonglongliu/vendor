
LOCAL_PATH := vendor/sprd/partner/watchdata/iwhale2

############################################################
## Install sml and tos image
############################################################
ifeq ($(TARGET_BOARD), sp9861e_1h10)
#    PRODUCT_COPY_FILES += \
#        $(LOCAL_PATH)/tos/secvm-sign.bin:secvm-sign.bin
#else
#   PRODUCT_COPY_FILES += \
#        $(LOCAL_PATH)/tos/9860/tos.bin:tos.bin
endif

############################################################
## TEE linux driver
############################################################
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/linux/driver/wdtee_armtz.ko:root/lib/modules/wdtee_armtz.ko \
    $(LOCAL_PATH)/TAs/77617463-6801-226B-65796D6173746572.secta:cache/data/tee/77617463-6801-226B-65796D6173746572.secta \
    $(LOCAL_PATH)/TAs/77617463-6802-6761-74656B6565706572.secta:cache/data/tee/77617463-6802-6761-74656B6565706572.secta \
    $(LOCAL_PATH)/TAs/gpapi_driver.bin:cache/data/tee/gpapi_driver.bin  


############################################################
## gatekeeper HAL and CA
############################################################
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/gatekeeper/lib/gatekeeper.default.so:system/lib/hw/gatekeeper.default.so \
    $(LOCAL_PATH)/gatekeeper/lib64/gatekeeper.default.so:system/lib64/hw/gatekeeper.default.so

############################################################
## keymaster HAL and CA
############################################################
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/keymaster/lib/libkeymaster.so:system/lib/libkeymaster.so \
    $(LOCAL_PATH)/keymaster/lib64/libkeymaster.so:system/lib64/libkeymaster.so
############################################################
## ifaa HAL and CA
############################################################
#PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/ifaa/lib/libifaaclient.so:system/lib/libifaaclient.so \
    $(LOCAL_PATH)/ifaa/lib/libteeclientjni.so:system/lib/libteeclientjni.so \
    $(LOCAL_PATH)/ifaa/lib64/libifaaclient.so:system/lib64/libifaaclient.so \
    $(LOCAL_PATH)/ifaa/lib64/libteeclientjni.so:system/lib64/libteeclientjni.so \
    $(LOCAL_PATH)/ifaa/bin/ifaaservice:system/bin/ifaaservice
############################################################
## iris HAL and CA
############################################################
IRIS_SUPPORT:= false
ifeq ($(IRIS_SUPPORT), true)

PRODUCT_COPY_FILES += \
#    $(LOCAL_PATH)/iris/lib/hw/iris.default.so:system/lib/hw/iris.default.so \
    $(LOCAL_PATH)/iris/lib64/hw/iris.default.so:system/lib64/hw/iris.default.so

endif

############################################################
## TEE=dependent libraries and daemons
############################################################
PRODUCT_PACKAGES +=       \
    wdtee_listener.bin	  \
    wdtee_deamon.bin  \
    wdtee_log.bin  \
    libteec.so \
    libteeproduction.so 
	

############################################################
## Install trusted application binary
############################################################
LOCAL_TA_NAME_SUFFIX := .*
LOCAL_TA_INSTALL_PATH := system/teetz

define all-ta-files-under
  $(patsubst ./%,%, \
    $(shell cd $(LOCAL_PATH) ; \
        find -L $(1) -name "*$(LOCAL_TA_NAME_SUFFIX)") \
  )
endef

define tee-copy-file
  PRODUCT_COPY_FILES += $(1):$(2)
endef

# arg1:dest path, arg2:src TA files
define tee-copy-files
  $(foreach src,$(2), $(eval $(call tee-copy-file,$(src),$(1)/$(notdir $(src)))))
endef

$(call tee-copy-files,$(LOCAL_TA_INSTALL_PATH),\
    $(addprefix $(LOCAL_PATH)/,$(call all-ta-files-under,TAs)))
