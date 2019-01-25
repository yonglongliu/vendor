FACTORY_SRC_PATH := $(call my-dir)
include $(CLEAR_VARS)

FACTORY_TARGET_LIB := $(TARGET_OUT_INTERMEDIATES)/sec_bsp/libsprdfactory.a

.PHONY: factory factory_build $(FACTORY_TARGET_LIB)

factory: $(FACTORY_TARGET_LIB)

factory_build:
	$(info Building sprd module ...)
	$(MAKE) -C $(FACTORY_SRC_PATH)

$(FACTORY_TARGET_LIB): factory_build
	$(hide_cmd) cp $(FACTORY_SRC_PATH)/libsprdfactory.a $@
	$(hide_cmd) rm $(FACTORY_SRC_PATH)/libsprdfactory.a
	$(info ### Done with libsprdfactory.a install ###)

ALL_DEFAULT_INSTALLED_MODULES += $(FACTORY_TARGET_LIB)
ALL_MODULES.$(LOCAL_MODULE).INSTALLED += $(FACTORY_TARGET_LIB)




