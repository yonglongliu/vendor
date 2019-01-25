#include <jni.h>
#include <utils/String16.h>
#include <gui/Surface.h>
#include <utils/StrongPointer.h>
#include "CameraService.h"
#include "CameraHardwareInterface.h"
#include "hardware.h"

using namespace android;

#define CAMERA_ROTATION_CROP_PARAM 1
#define CAMERA_STOP_RECORDING_PARAM 2

typedef struct {
    int width;
    int height;
}Resolution;

typedef enum {
    INVALID_PARAM = 0,
    SET_FPS,
    SET_RESOLUTION
}eParamType;

typedef union {
    int fps;
    Resolution cameraResolution;
}CameraParams;

typedef struct {
    eParamType type;
    CameraParams params;
}CameraParamContainer;

typedef void (*ims_notify_callback)(int msgType,
                            int ext1,
                            int ext2,
                            void* user);

typedef void (*ims_custom_params_callback)(void* params);

typedef void (*ims_notify_callback)(int32_t msgType,
                            int32_t ext1,
                            int32_t ext2,
                            void* user);

typedef void (*ims_data_callback)(int32_t msgType,
                            const sp<IMemory> &dataPtr,
                            camera_frame_metadata_t *metadata,
                            void* user);

typedef void (*ims_data_callback_timestamp)(nsecs_t timestamp,
                            int32_t msgType,
                            const sp<IMemory> &dataPtr,
                            void *user);

typedef struct {
    void * cameraParam;
    int cameraMount;
    int cameraFacing;
    int* rotationOffset;
    int width;
    int height;
} CropRotationParam;

typedef struct {
    int type;

    union {
    CropRotationParam cropRotationParam;
    };

} CustomCallbackParam;

typedef enum {
    VCE_CAMERA_NO_ERR      = 0,
    VCE_CAMERA_TIME_OUT    = -1,
    VCE_CAMERA_FAILED      = -2,
    VCE_CAMERA_UNINITED    = -3,
} VCE_CAMERA_ERR;

#ifdef __cplusplus
extern "C" {
#endif

short imsCameraOpen(unsigned int cameraid);
short imsCameraRelease();
short imsCameraSetPreviewSurface(sp<IGraphicBufferProducer> *surface);
short imsCamerasetPreviewDisplayOrientation(unsigned int rotation);
short imsCameraStartPreview();
short imsCameraStopPreview();
short imsCameraStartRecording();
short imsCameraStopRecording();
short imsCameraSetParameter(CameraParamContainer setParams);
CameraParams getCameraParameter(eParamType paramKey);

void registerCameraCallbacks(ims_notify_callback notifyCb);

short registerCallbacks(ims_notify_callback notify_cb,
        ims_data_callback data_cb,
        ims_data_callback_timestamp data_cb_timestamp,
        ims_custom_params_callback custom_cb,
        void* user);
#ifdef __cplusplus

}
#endif
