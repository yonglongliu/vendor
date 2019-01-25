droid:gecko_vowifi
gecko_vowifi: gecko

LOCAL_PATH := $(call my-dir)

# symbol link the source code to gecko/dom
$(info $(shell $(LOCAL_PATH)/copy_to_gecko.sh))
