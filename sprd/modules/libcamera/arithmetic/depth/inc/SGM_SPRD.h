#ifndef __SGM_SPRD_H__
#define __SGM_SPRD_H__

#ifdef __cplusplus
extern "C" {
#endif

	typedef enum
	{
		YUV420_NV12=0,
		YUV422_YUYV,
	} ImageYUVFormat;
	typedef enum
	{
		MODE_PREVIEW,
		MODE_CAPTURE
	}depth_mode;

	typedef enum
	{
		MODE_DISPARITY,
		MODE_WEIGHTMAP
	}outFormat;

	typedef enum
	{
		DISTANCE_OK=0,
		DISTANCE_FAR,
		DISTANCE_CLOSE
	}distanceRet;

	typedef enum
	{
		DEPTH_NORMAL=0,
		DEPTH_STOP,
	}depth_stop_flag;

	struct depth_init_inputparam
	{
		int input_width_main;
		int input_height_main;
		int input_width_sub;
		int input_height_sub;
		int output_depthwidth;
		int output_depthheight;
		int online_depthwidth;
		int online_depthheight;
		int depth_threadNum;
		int online_threadNum;
		ImageYUVFormat imageFormat_main;
		ImageYUVFormat imageFormat_sub;
		void* potpbuf;
		int otpsize;
		char * config_param;

	};

	struct depth_init_outputparam
	{
		int  outputsize;
		int  calibration_width;
		int  calibration_height; 
	};

	typedef struct {
		int F_number; // 1 ~ 20
		int sel_x; /* The point which be touched */
		int sel_y; /* The point which be touched */
		unsigned char *DisparityImage;
	} weightmap_param;

	typedef struct DistanceTwoPointInfo
	{

		int x1_pos;
		int y1_pos;
		int x2_pos;
		int y2_pos;
	}DistanceTwoPointInfo;


	int sprd_depth_VersionInfo_Get(char a_acOutRetbuf[256], unsigned int a_udInSize);

	void sprd_depth_Set_Stopflag(void *handle,depth_stop_flag stop_flag);

	void* sprd_depth_Init(struct depth_init_inputparam *inparam , struct depth_init_outputparam *outputinfo,depth_mode mode,outFormat format);

	int sprd_depth_Run(void*handle , void * a_pOutDisparity, void * a_pOutMaptable,void* a_pInSub_YCC420NV21,void* a_pInMain_YCC420NV21,weightmap_param *wParams);

	int sprd_depth_Run_distance(void* handle , void * a_pOutDisparity, void * a_pOutMaptable, void* a_pInSub_YCC420NV21, void* a_pInMain_YCC420NV21,weightmap_param *wParams,distanceRet *distance);

	int sprd_depth_OnlineCalibration(void* handle , void * a_pOutMaptable, void* a_pInSub_YCC420NV21, void* a_pInMain_YCC420NV21);

	int sprd_depth_rotate(void * a_pOutDisparity,int width,int height,int angle);

	int sprd_depth_distancemeasurement(int *distance , void* disparity , DistanceTwoPointInfo *points_info);

	int sprd_depth_Close(void *handle);


#ifdef __cplusplus
}//extern C
#endif

#endif

