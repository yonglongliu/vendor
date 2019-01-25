/* Copyright (c) 2017, The Linux Foundataion. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ifndef SPRDCAMERA3BLUR_H_HEADER
#define SPRDCAMERA3BLUR_H_HEADER

#include <stdlib.h>
#include <dlfcn.h>
#include <utils/Log.h>
#include <utils/Errors.h>
#include <utils/List.h>
#include <utils/Mutex.h>
#include <utils/Thread.h>
#include <cutils/properties.h>
#include <hardware/camera3.h>
#include <hardware/camera.h>
#include <system/camera.h>
#include <sys/mman.h>
#include <sprd_ion.h>
#include <gralloc_priv.h>
#include <ui/GraphicBuffer.h>
#include "../SprdCamera3HWI.h"
#include "SprdMultiCam3Common.h"
//#include "ts_makeup_api.h"
#include "SprdCamera3MultiBase.h"
#include "../SprdCamera3Setting.h"
#include "SprdCamera3FaceBeautyBase.h"

namespace sprdcamera {

#define YUV_CONVERT_TO_JPEG
//#define ISP_SUPPORT_MICRODEPTH

#ifdef YUV_CONVERT_TO_JPEG
#define BLUR_LOCAL_CAPBUFF_NUM (4)
#else
#define BLUR_LOCAL_CAPBUFF_NUM 2
#endif

#ifdef ISP_SUPPORT_MICRODEPTH
#ifdef YUV_CONVERT_TO_JPEG
#define BLUR3_REFOCUS_COMMON_PARAM_NUM (27)
#define BLUR_REFOCUS_PARAM2_NUM (52)
#else
#define BLUR3_REFOCUS_COMMON_PARAM_NUM (25)
#define BLUR_REFOCUS_PARAM2_NUM (50)
#endif
#else
#ifdef YUV_CONVERT_TO_JPEG
#define BLUR3_REFOCUS_COMMON_PARAM_NUM (11)
#define BLUR_REFOCUS_PARAM2_NUM (11)
#else
#define BLUR3_REFOCUS_COMMON_PARAM_NUM (9)
#define BLUR_REFOCUS_PARAM2_NUM (9)
#endif
#endif

#define BLUR_REFOCUS_COMMON_PARAM_NUM (20)
#define BLUR_MAX_NUM_STREAMS (3)
#define BLUR_THREAD_TIMEOUT 50e6
#define BLUR_LIB_BOKEH_PREVIEW "libbokeh_gaussian.so"
#define BLUR_LIB_BOKEH_CAPTURE "libbokeh_gaussian_cap.so"
#define BLUR_LIB_BOKEH_CAPTURE2 "libBokeh2Frames.so"
#define BLUR_LIB_BOKEH_NUM (2)

#define BLUR_REFOCUS_2_PARAM_NUM (17)
#define BLUR_AF_WINDOW_NUM (9)
#define BLUR_MAX_ROI (10)
#define BLUR_CALI_SEQ_LEN (20)
#define BLUR_GET_SEQ_FROM_AF (0)
#define BLUR_REFOCUS_PARAM_NUM                                                 \
    (BLUR_AF_WINDOW_NUM + BLUR_REFOCUS_2_PARAM_NUM +                           \
     BLUR_REFOCUS_COMMON_PARAM_NUM + BLUR_MAX_ROI * 5 + BLUR_CALI_SEQ_LEN)

#define BLUR_CIRCLE_SIZE_SCALE (3)
#define BLUR_SMOOTH_SIZE_SCALE (8)
#define BLUR_CIRCLE_VALUE_MIN (20)

typedef struct {
    unsigned int af_peak_pos;
    unsigned int near_peak_pos;
    unsigned int far_peak_pos;
    unsigned int distance_reminder;
} blur_isp_info_t;

typedef struct {
    uint32_t frame_number;
    buffer_handle_t *buffer;
    camera3_stream_t *stream;
    camera3_stream_buffer_t *input_buffer;
} request_saved_blur_t;

typedef enum {
    BLUR_MSG_DATA_PROC = 1,
    BLUR_MSG_COMBAIN_PROC,
    BLUR_MSG_EXIT
} blurMsgType;

typedef struct {
    uint32_t frame_number;
    const camera3_stream_buffer_t *input_buffer;
    buffer_handle_t *buffer;
} buffer_combination_blur_t;

typedef struct {
    blurMsgType msg_type;
    buffer_combination_blur_t combo_buff;
} blur_queue_msg_t;

typedef struct {
    int width;                       // image width
    int height;                      // image height
    float min_slope;                 // 0.001~0.01, default is 0.005
    float max_slope;                 // 0.01~0.1, default is 0.05
    float findex2gamma_adjust_ratio; // 2~11, default is 6.0
    int box_filter_size;
} preview_init_params_t;

typedef struct {
    int roi_type;         // 0: circle 1:rectangle
    int f_number;         // 1 ~ 20
    unsigned short sel_x; /* The point which be touched */
    unsigned short sel_y; /* The point which be touched */
    int circle_size;      // for version 1.0 only
    // for rectangle
    int valid_roi;
    int x1[BLUR_MAX_ROI], y1[BLUR_MAX_ROI]; // left-top point of roi
    int x2[BLUR_MAX_ROI], y2[BLUR_MAX_ROI]; // right-bottom point of roi
    int flag[BLUR_MAX_ROI];                 // 0:face 1:body
} preview_weight_params_t;

typedef struct {
    int width;                       // image width
    int height;                      // image height
    float min_slope;                 // 0.001~0.01, default is 0.005
    float max_slope;                 // 0.01~0.1, default is 0.05
    float findex2gamma_adjust_ratio; // 2~11, default is 6.0
    int Scalingratio;                // only support 2,4,6,8
    int SmoothWinSize;               // odd number
    int box_filter_size; // odd number, default: the same as SmoothWinSize
    // below for 2.0 only
    /* Register Parameters : depend on sensor module */
    unsigned short vcm_dac_up_bound;  /* Default value : 0 */
    unsigned short vcm_dac_low_bound; /* Default value : 0 */
    unsigned short *vcm_dac_info;     /* Default value : NULL (Resaved) */
    /* Register Parameters : For Tuning */
    unsigned char vcm_dac_gain;     /* Default value : 16 */
    unsigned char valid_depth_clip; /* The up bound of valid_depth */
    unsigned char method;           /* The depth method. (Resaved) */
    /* Register Parameters : depend on AF Scanning */
    /* The number of AF windows with row (i.e. vertical) */
    unsigned char row_num;
    /* The number of AF windows with row (i.e. horizontal) */
    unsigned char column_num;
    unsigned char boundary_ratio; /*  (Unit : Percentage) */
    /* Customer Parameter */
    /* Range: [0, 16] Default value : 0*/
    unsigned char sel_size;
    /* For Tuning Range : [0, 32], Default value : 4 */
    unsigned char valid_depth;
    unsigned short slope; /* For Tuning : Range : [0, 16] default is 8 */
    unsigned char valid_depth_up_bound;  /* For Tuning */
    unsigned char valid_depth_low_bound; /* For Tuning */
    unsigned short *cali_dist_seq;       /* Pointer */
    unsigned short *cali_dac_seq;        /* Pointer */
    unsigned short cali_seq_len;
} capture_init_params_t;

typedef struct {
    int version;                  // 1~2, 1: 1.0 bokeh; 2: 2.0 bokeh with AF
    int roi_type;                 // // 0: circle 1:rectangle 2:seg
    int f_number;                 // 1 ~ 20
    unsigned short sel_x;         /* The point which be touched */
    unsigned short sel_y;         /* The point which be touched */
    unsigned short *win_peak_pos; /* The seqence of peak position which be
                                     provided via struct isp_af_fullscan_info */
    int circle_size;              // for version 1.0 only
    // for rectangle
    int valid_roi;
    int total_roi;
    int x1[BLUR_MAX_ROI], y1[BLUR_MAX_ROI]; // left-top point of roi
    int x2[BLUR_MAX_ROI], y2[BLUR_MAX_ROI]; // right-bottom point of roi
    int flag[BLUR_MAX_ROI];                 // 0:face 1:body
    int rotate_angle; // counter clock-wise. 0:face up body down 90:face left
                      // body right 180:face down body up 270:face right body
                      // left  ->
    bool rear_cam_en; // 1:rear camera capture 0:front camera capture
} capture_weight_params_t;

typedef struct {
    void *handle;
    int (*iSmoothInit)(void **handle, int width, int height, float min_slope,
                       float max_slope, float findex2gamma_adjust_ratio,
                       int box_filter_size);
    int (*iSmoothCapInit)(void **handle, capture_init_params_t *params);
    int (*iSmoothCap_VersionInfo_Get)(void *a_pOutBuf, int a_dInBufMaxSize);
    int (*iSmoothDeinit)(void *handle);
    int (*iSmoothCreateWeightMap)(void *handle,
                                  preview_weight_params_t *params);
    int (*iSmoothCapCreateWeightMap)(void *handle,
                                     capture_weight_params_t *params,
                                     unsigned char *Src_YUV,
                                     unsigned short *outWeightMap);
    int (*iSmoothBlurImage)(void *handle, unsigned char *Src_YUV,
                            unsigned char *Output_YUV);
    int (*iSmoothCapBlurImage)(void *handle, unsigned char *Src_YUV,
                               unsigned short *inWeightMap,
                               capture_weight_params_t *params,
                               unsigned char *Output_YUV);
    void *mHandle;
} BlurAPI_t;

typedef struct {
    cmr_u32 enable;
    cmr_u32 fir_mode;
    cmr_u32 fir_len;
    cmr_s32 hfir_coeff[7];
    cmr_s32 vfir_coeff[7];
    cmr_u32 fir_channel;
    cmr_u32 fir_cal_mode;
    cmr_s32 fir_edge_factor;
    cmr_u32 depth_mode;
    cmr_u32 smooth_thr;
    cmr_u32 touch_factor;
    cmr_u32 scale_factor;
    cmr_u32 refer_len;
    cmr_u32 merge_factor;
    cmr_u32 similar_factor;
    cmr_u32 similar_coeff[3];
    cmr_u32 tmp_mode;
    cmr_s32 tmp_coeff[8];
    cmr_u32 tmp_thr;
} capture2_init_params_t;

#ifdef ISP_SUPPORT_MICRODEPTH
typedef struct {
    cmr_u8 *microdepth_buffer;
    cmr_u32 microdepth_size;
} MicrodepthBoke2Frames;
#endif

typedef struct {
    int f_number;         // 1 ~ 20
    unsigned short sel_x; /* The point which be touched */
    unsigned short sel_y; /* The point which be touched */
} capture2_weight_params_t;

typedef struct {
    void *handle;
    int (*BokehFrames_VersionInfo_Get)(char a_acOutRetbuf[256],
                                       unsigned int a_udInSize);
    int (*BokehFrames_Init)(void **handle, int width, int height,
                            capture2_init_params_t *params);
    int (*BokehFrames_WeightMap)(void *img0_src, void *img1_src, void *dis_map,
                                 void *handle);
    int (*Bokeh2Frames_Process)(void *img0_src, void *img_rslt, void *dis_map,
                                void *handle, capture2_weight_params_t *params);
#ifdef ISP_SUPPORT_MICRODEPTH
    int (*BokehFrames_ParamInfo_Get)(void *handle,
                                     MicrodepthBoke2Frames **microdepthInfo);
#endif
    int (*BokehFrames_Deinit)(void *handle);
    void *mHandle;
} BlurAPI2_t;

class SprdCamera3Blur : SprdCamera3MultiBase, SprdCamera3FaceBeautyBase {
  public:
    static void getCameraBlur(SprdCamera3Blur **pBlur);
    static int camera_device_open(__unused const struct hw_module_t *module,
                                  const char *id,
                                  struct hw_device_t **hw_device);
    static int close_camera_device(struct hw_device_t *device);
    static int get_camera_info(int camera_id, struct camera_info *info);
    static int initialize(const struct camera3_device *device,
                          const camera3_callback_ops_t *ops);
    static int configure_streams(const struct camera3_device *device,
                                 camera3_stream_configuration_t *stream_list);
    static const camera_metadata_t *
    construct_default_request_settings(const struct camera3_device *, int type);
    static int process_capture_request(const struct camera3_device *device,
                                       camera3_capture_request_t *request);
    static void notifyMain(const struct camera3_callback_ops *ops,
                           const camera3_notify_msg_t *msg);
    static void
    process_capture_result_main(const struct camera3_callback_ops *ops,
                                const camera3_capture_result_t *result);
    static void dump(const struct camera3_device *device, int fd);
    static int flush(const struct camera3_device *device);
    static camera3_device_ops_t mCameraCaptureOps;
    static camera3_callback_ops callback_ops_main;
    static void
    process_capture_result_aux(const struct camera3_callback_ops *ops,
                               const camera3_capture_result_t *result);
    static void notifyAux(const struct camera3_callback_ops *ops,
                          const camera3_notify_msg_t *msg);
    static camera3_callback_ops callback_ops_aux;

  private:
    sprdcamera_physical_descriptor_t *m_pPhyCamera;
    sprd_virtual_camera_t m_VirtualCamera;
    uint8_t m_nPhyCameras;
    Mutex mLock;
    camera_metadata_t *mStaticMetadata;
    int mCaptureWidth;
    int mCaptureHeight;
    new_mem_t mLocalCapBuffer[BLUR_LOCAL_CAPBUFF_NUM];
    bool mFlushing;
    List<request_saved_blur_t> mSavedRequestList;
    camera3_stream_t *mSavedReqStreams[BLUR_MAX_NUM_STREAMS];
    int mPreviewStreamsNum;
    Mutex mRequestLock;
    int mjpegSize;
    void *m_pNearYuvBuffer;
    void *m_pFarYuvBuffer;
#ifdef YUV_CONVERT_TO_JPEG
    int mNearJpegSize;
    int mFarJpegSize;
    buffer_handle_t *m_pNearJpegBuffer;
    buffer_handle_t *m_pFarJpegBuffer;
#endif
    void *weight_map;
    uint8_t mCameraId;
    int32_t mPerfectskinlevel;
    int mCoverValue;
    face_beauty_levels fbLevels;
    int cameraDeviceOpen(int camera_id, struct hw_device_t **hw_device);
    int setupPhysicalCameras();
    int getCameraInfo(int blur_camera_id, struct camera_info *info);
    void freeLocalCapBuffer();
    void saveRequest(camera3_capture_request_t *request);

  public:
    SprdCamera3Blur();
    virtual ~SprdCamera3Blur();

    class CaptureThread : public Thread {
      public:
        CaptureThread();
        ~CaptureThread();
        virtual bool threadLoop();
        virtual void requestExit();
        int loadBlurApi();
        void unLoadBlurApi();
        int initBlur20Params();
        int initBlurInitParams();
        void initBlurWeightParams();
        bool isBlurInitParamsChanged();
        void updateBlurWeightParams(CameraMetadata metaSettings, int type);
        void saveCaptureBlurParams(buffer_handle_t *result_buff,
                                   uint32_t jpeg_size);
        //        void getOutWeightMap(buffer_handle_t *output_buffer);
        void dumpSaveImages(void *result_buff, uint32_t use_size,
                            uint32_t jpeg_size);
        uint8_t getIspAfFullscanInfo();
        int blurHandle(buffer_handle_t *input1, void *input2,
                       buffer_handle_t *output);
        // This queue stores matched buffer as frame_matched_info_t
        List<blur_queue_msg_t> mCaptureMsgList;
        Mutex mMergequeueMutex;
        Condition mMergequeueSignal;
        const camera3_callback_ops_t *mCallbackOps;
        sprdcamera_physical_descriptor_t *mDevMain;
        buffer_handle_t *mSavedResultBuff;
        camera3_capture_request_t mSavedCapRequest;
        camera3_stream_buffer_t mSavedCapReqstreambuff;
        camera_metadata_t *mSavedCapReqsettings;
        camera3_stream_t mMainStreams[BLUR_MAX_NUM_STREAMS];
        uint8_t mCaptureStreamsNum;
        BlurAPI_t *mBlurApi[BLUR_LIB_BOKEH_NUM];
        BlurAPI2_t *mBlurApi2;
        int mLastMinScope;
        int mLastMaxScope;
        int mLastAdjustRati;
        int mCircleSizeScale;
        bool mFirstCapture;
        bool mFirstPreview;
        bool mUpdateCaptureWeightParams;
        bool mUpdatePreviewWeightParams;
        uint8_t mLastFaceNum;
        uint8_t mSkipFaceNum;
        unsigned short mWinPeakPos[BLUR_AF_WINDOW_NUM];
        preview_init_params_t mPreviewInitParams;
        preview_weight_params_t mPreviewWeightParams;
        capture_init_params_t mCaptureInitParams;
        capture_weight_params_t mCaptureWeightParams;
        capture2_init_params_t mCapture2InitParams;
        capture2_weight_params_t mCapture2WeightParams;
#ifdef ISP_SUPPORT_MICRODEPTH
        bokeh_micro_depth_tune_param mIspCapture2InitParams;
#endif
        int32_t mFaceInfo[4];
        uint32_t mRotation;
        int32_t mLastTouchX;
        int32_t mLastTouchY;
        bool mBlurBody;
        bool mUpdataTouch;
        int mVersion;
        bool mIsGalleryBlur;
        bool mIsBlurAlways;
        blur_isp_info_t mIspInfo;
        unsigned short *mOutWeightBuff;
//        unsigned short *mOutWeightMap;
#ifdef ISP_SUPPORT_MICRODEPTH
        MicrodepthBoke2Frames *mMicrodepthInfo;
#endif
      private:
        void waitMsgAvailable();
        void BlurFaceMakeup(buffer_handle_t *buffer_handle, void *buffer_addr);
    };
    sp<CaptureThread> mCaptureThread;
    Mutex mMergequeueFinishMutex;
    Condition mMergequeueFinishSignal;

    int initialize(const camera3_callback_ops_t *callback_ops);
    int configureStreams(const struct camera3_device *device,
                         camera3_stream_configuration_t *stream_list);
    int processCaptureRequest(const struct camera3_device *device,
                              camera3_capture_request_t *request);
    void notifyMain(const camera3_notify_msg_t *msg);
    void processCaptureResultMain(camera3_capture_result_t *result);
    const camera_metadata_t *
    constructDefaultRequestSettings(const struct camera3_device *device,
                                    int type);
    void _dump(const struct camera3_device *device, int fd);

    int _flush(const struct camera3_device *device);
    int closeCameraDevice();
};
};

#endif /* SPRDCAMERAMU*/
