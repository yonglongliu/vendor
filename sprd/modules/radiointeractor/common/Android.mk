LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-java-files-under, $(src_dirs)) \
                   src/com/android/sprd/telephony/IRadioInteractor.aidl \
                   src/com/android/sprd/telephony/IRadioInteractorCallback.aidl \
                   src/com/android/sprd/telephony/IRadioInteractorlService.aidl

LOCAL_STATIC_JAVA_LIBRARIES := android.hardware.radio-V1.0-java-static \
    android.hardware.radio.deprecated-V1.0-java-static \
    vendor.sprd.hardware.radio-V1.0-java-static

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := radio_interactor_common
LOCAL_PROGUARD_ENABLED := disabled

include $(BUILD_JAVA_LIBRARY)
