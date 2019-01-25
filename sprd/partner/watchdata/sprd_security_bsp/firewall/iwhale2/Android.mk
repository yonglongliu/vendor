# Build sprd firewall
SEC_MK_DIR := $(call my-dir)

include $(CLEAR_VARS)

SEC_LIB := $(SEC_MK_DIR)/sprd_firewall.a

SEC_BSP_OUT := $(TARGET_OUT_INTERMEDIATES)/sec_bsp

SEC_FW_OUT := $(SEC_BSP_OUT)/firewall

INSTALLED_SEC_TARGET := $(SEC_FW_OUT)/sprd_firewall.a

.PHONY: firewall sec_build $(INSTALLED_SEC_TARGET)
firewall: $(INSTALLED_SEC_TARGET)

sec_build:
	$(info Building sprd secure bsp ...)
	$(MAKE) -C $(SEC_MK_DIR)

$(INSTALLED_SEC_TARGET): sec_build
	$(hide_cmd) cp $(SEC_LIB) $@
	$(hide_cmd) rm $(SEC_LIB)
	$(info ### Done with secure bsp install ###)

ALL_DEFAULT_INSTALLED_MODULES += $(INSTALLED_SEC_TARGET)
ALL_MODULES.$(LOCAL_MODULE).INSTALLED += $(INSTALLED_SEC_TARGET)

