CURRENT_PATH_TOS_SERVER := $(call my-dir)
include $(CLEAR_VARS)

TOS_SERVER_LIB := $(CURRENT_PATH_TOS_SERVER)/libsprdtos_server.a
TOS_SERVER_OUT := $(TARGET_OUT_INTERMEDIATES)/sec_bsp/tos_server
INSTALLED_TOS_SERVER_TARGET := $(TOS_SERVER_OUT)/libsprdtos_server.a

.PHONY: tos_server tos_server_build $(INSTALLED_TOS_SERVER_TARGET) 
tos_server: $(INSTALLED_TOS_SERVER_TARGET)

tos_server_build:
	$(info Building sprd tos server driver...)
	$(MAKE) -C $(CURRENT_PATH_TOS_SERVER)

$(INSTALLED_TOS_SERVER_TARGET): tos_server_build
	$(hide_cmd) cp $(TOS_SERVER_LIB) $@
	$(hide_cmd) rm $(TOS_SERVER_LIB)
	$(info ### Done with libsprdtos_server.a install ###)

ALL_DEFAULT_INSTALLED_MODULES += $(INSTALLED_TOS_SERVER_TARGET)
ALL_MODULES.$(LOCAL_MODULE).INSTALLED += $(INSTALLED_TOS_SERVER_TARGET)

