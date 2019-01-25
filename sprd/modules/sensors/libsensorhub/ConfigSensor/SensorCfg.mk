
LOCAL_SRC_FILES := \
		SensorHubOpCodeExtrator.cpp

LOCAL_CFLAGS := -DLOG_TAG=\"SensorHubDrvConfig\"

ifneq ($(strip $(SENSOR_HUB_ACCELEROMETER)), $(filter $(SENSOR_HUB_ACCELEROMETER), null))
LOCAL_SRC_FILES += \
		accelerometer/accelerometer_$(SENSOR_HUB_ACCELEROMETER).cpp

LOCAL_CFLAGS += -DSENSORHUB_WITH_ACCELEROMETER
endif

ifneq ($(strip $(SENSOR_HUB_GYROSCOPE)), $(filter $(SENSOR_HUB_GYROSCOPE), null))
LOCAL_SRC_FILES += \
		gyroscope/gyroscope_$(SENSOR_HUB_GYROSCOPE).cpp

LOCAL_CFLAGS += -DSENSORHUB_WITH_GYROSCOPE
endif

ifneq ($(strip $(SENSOR_HUB_LIGHT)), $(filter $(SENSOR_HUB_LIGHT), null))
LOCAL_SRC_FILES += \
		light/light_$(SENSOR_HUB_LIGHT).cpp

LOCAL_CFLAGS += -DSENSORHUB_WITH_LIGHT
endif

ifneq ($(strip $(SENSOR_HUB_MAGNETIC)), $(filter $(SENSOR_HUB_MAGNETIC), null))
LOCAL_SRC_FILES += \
		magnetic/magnetic_$(SENSOR_HUB_MAGNETIC).cpp

LOCAL_CFLAGS += -DSENSORHUB_WITH_MAGNETIC
endif

ifneq ($(strip $(SENSOR_HUB_PROXIMITY)), $(filter $(SENSOR_HUB_PROXIMITY), null))
LOCAL_SRC_FILES += \
		proximity/proximity_$(SENSOR_HUB_PROXIMITY).cpp

LOCAL_CFLAGS += -DSENSORHUB_WITH_PROXIMITY
endif

ifneq ($(strip $(SENSOR_HUB_PRESSURE)), $(filter $(SENSOR_HUB_PRESSURE), null))
LOCAL_SRC_FILES += \
		pressure/pressure_$(SENSOR_HUB_PRESSURE).cpp

LOCAL_CFLAGS += -DSENSORHUB_WITH_PRESSURE
endif

ifneq ($(strip $(SENSOR_HUB_CALIBRATION)), $(filter $(SENSOR_HUB_CALIBRATION), null))
LOCAL_SRC_FILES += \
		calibration/calibration_$(SENSOR_HUB_CALIBRATION).cpp
LOCAL_CFLAGS += -DSENSORHUB_WITH_CALIBRATION
endif

