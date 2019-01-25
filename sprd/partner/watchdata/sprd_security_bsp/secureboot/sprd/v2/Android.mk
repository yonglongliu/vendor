CURRENT_PATH_SECBOOT := $(call my-dir)
include $(CLEAR_VARS)

SECBOOT_LIB_V2 := $(CURRENT_PATH_SECBOOT)/libsprdsecboot.a
SECBOOT_OUT_V2 := $(TARGET_OUT_INTERMEDIATES)/sec_bsp/secboot
INSTALLED_SECBOOT_TARGET_V2 := $(SECBOOT_OUT_V2)/libsprdsecboot.a

.PHONY: secboot secboot_build $(INSTALLED_SECBOOT_TARGET_V2)

secboot: $(INSTALLED_SECBOOT_TARGET_V2)

secboot_build:
	$(info Building sprd  secureboot v2 version...)
	$(MAKE) -C $(CURRENT_PATH_SECBOOT)

$(INSTALLED_SECBOOT_TARGET_V2): secboot_build
	$(hide_cmd) cp $(SECBOOT_LIB_V2) $@
	$(hide_cmd) rm $(SECBOOT_LIB_V2)
	$(info ### Done with libsprdsecboot.a install ###)

ALL_DEFAULT_INSTALLED_MODULES += $(INSTALLED_SECBOOT_TARGET_V2)
ALL_MODULES.$(LOCAL_MODULE).INSTALLED += $(INSTALLED_SECBOOT_TARGET_V2) 
