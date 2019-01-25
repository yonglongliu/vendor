CURRENT_PATH_R2P0 := $(call my-dir)
include $(CLEAR_VARS)

CE_LIB_R2P0 := $(CURRENT_PATH_R2P0)/libsprdce.a
CE_OUT_R2P0 := $(TARGET_OUT_INTERMEDIATES)/sec_bsp/ce
INSTALLED_CE_TARGET_R2P0 := $(CE_OUT_R2P0)/libsprdce.a

.PHONY: ce ce_build $(INSTALLED_CE_TARGET_R2P0)

ce: $(INSTALLED_CE_TARGET_R2P0)

ce_build:
	$(info Building sprd ce r2p0 driver...)
	$(MAKE) -C $(CURRENT_PATH_R2P0)

$(INSTALLED_CE_TARGET_R2P0): ce_build
	$(hide_cmd) cp $(CE_LIB_R2P0) $@
	$(hide_cmd) rm $(CE_LIB_R2P0)
	$(info ### Done with libsprdce.a install ###)

ALL_DEFAULT_INSTALLED_MODULES += $(INSTALLED_CE_TARGET_R2P0)
ALL_MODULES.$(LOCAL_MODULE).INSTALLED += $(INSTALLED_CE_TARGET_R2P0)

