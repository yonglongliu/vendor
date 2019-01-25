EFUSE_SRC_PATH := $(call my-dir)
include $(CLEAR_VARS)

EFUSE_TARGET_LIB := $(TARGET_OUT_INTERMEDIATES)/sec_bsp/libsprdefuse.a

.PHONY: efuse efuse_build $(EFUSE_TARGET_LIB)

efuse: $(EFUSE_TARGET_LIB)

efuse_build:
	$(info Building sprd module ...)
	$(MAKE) -C $(EFUSE_SRC_PATH)

$(EFUSE_TARGET_LIB): efuse_build
	$(hide_cmd) cp $(EFUSE_SRC_PATH)/libsprdefuse.a $@
	$(hide_cmd) rm $(EFUSE_SRC_PATH)/libsprdefuse.a
	$(info ### Done with libsprdefuse.a install ###)

ALL_DEFAULT_INSTALLED_MODULES += $(EFUSE_TARGET_LIB)
ALL_MODULES.$(LOCAL_MODULE).INSTALLED += $(EFUSE_TARGET_LIB)




