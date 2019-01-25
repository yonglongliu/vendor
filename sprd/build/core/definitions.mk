# Nothing yet, but will be more infuture
SPRD_BUILD_SYSTEM := vendor/sprd/build/core

include $(wildcard $(SPRD_BUILD_SYSTEM)/config.mk)

all-java-files-under += $(eval LOCAL_JAVA_FILES_PATH := $(1) $(filter-out $(1),$(LOCAL_JAVA_FILES_PATH)))

find-other-java-files += $(eval LOCAL_JAVA_FILES_PATH := $(1) $(filter-out $(1),$(LOCAL_JAVA_FILES_PATH)))

all-c-files-under += $(eval LOCAL_C_FILES_PATH := $(1) $(filter-out $(1),$(LOCAL_C_FILES_PATH)))

all-cpp-files-under += $(eval LOCAL_C_FILES_PATH := $(1) $(filter-out $(1),$(LOCAL_C_FILES_PATH)))

define all-c-cpp-files-under
$(strip \
 $(sort $(patsubst ./%,%, \
   $(shell cd $(LOCAL_PATH) ; \
           find -L $(1) -name "*$(or $(LOCAL_CPP_EXTENSION),.cpp)" -and -not -name ".*") \
   $(strip $(call all-named-files-under,*.c, $(1))) \
  )) \
)
endef
