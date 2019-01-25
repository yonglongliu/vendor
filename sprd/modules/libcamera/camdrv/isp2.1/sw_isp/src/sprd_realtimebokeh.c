/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "sprd_realtimebokeh.h"
#include "swisp_log.h"
#include "thread.h"
#include "sw_isp_interface.h"
#include "iBokeh.h"
#include "SGM_SPRD.h"
#include "sprd_depth_configurable_param_sbs.h"
#include <utils/Timers.h>
#include <semaphore.h>
#include <cutils/properties.h>
#define DEPTH_OUTPUT_WIDTH 320
#define DEPTH_OUTPUT_HEIGHT 240

#define SAVE_SWISP_PROP "realbokeh.debugdump"
#define SAVE_PATH "/data/misc/cameraserver"
#define CLOSE_REALBOKEH "close.realbokeh"
#define BOKEHCALCULATE_DISANCE_OK 0
#define BOKEHCALCULATE_DISANCE_TOOFAR 1
#define BOKEHCALCULATE_DISANCE_TOONEAR 2
#define BOKEHCALCULATE_DISANCE_ERROR 3
static int framenum = 0;
static int bokeh_num = 0;
typedef enum bokeh_af_state
{
	BOKEH_AF_NORMAL,
	BOKEH_AF_FOCUSING,
	BOKEH_AF_RESUMSTATE1,
	BOKEH_AF_RESUMSTATE2
}bokeh_af_state_e;
typedef enum distance_info
{
	BOKEH_DISTANCE_OK,
	BOKEH_DISTANCE_TOONEAR,
	BOKEH_DISTANCE_TOOFAR,
	BOKEH_DISTANCE_ERROR
}distance_info_e;
typedef struct misc_thread_arg
{
	struct isp_img_frm raw;
	struct soft_isp_misc_img_frm m_yuv_pre;
	//struct soft_isp_frm m_yuv_depth;
	//struct img_frm m_yuv_bokeh;
	struct isp_img_frm s_yuv_depth;
	struct isp_img_frm s_yuv_sw_out;
	struct soft_isp_block_param isp_blockparam;
	uint32_t need_onlinecalb;
}misc_thread_arg_t;

typedef struct sw_isp_handle
{
	void* swisp_handle;
	void* bokeh_handle;
	void* depth_handle;
	struct soft_isp_init_param param;
	struct soft_isp_block_param isp_block_param;
	CThread_pool *threadpool; //swisp ynr depth thread , misc thread
	uint8_t misc_calculating; //misc thread processing?
	uint8_t misc_depthcalculating;
	uint8_t online_calbr_valid;
	uint8_t online_calbr_calculating;
	uint8_t online_calbr_forceinvalid;
	uint32_t online_calbinfo_size;
	uint32_t online_calbinfo_width;
	uint32_t online_calbinfo_height;
	misc_thread_arg_t misc_arg;
	uint16_t * depth_weight_ptr[3];
	uint8_t depth_weight_valid_index;
	uint8_t* otp_data;
	void *ponline_calbinfo;
	int32_t otp_size;
	WeightParams_t weight_param;
	uint8_t process_flag;
	uint8_t stopflag;
	pthread_cond_t control_misc_state;
	pthread_mutex_t control_misc_mutex;
	pthread_mutex_t control_mutex;
	int start_flag;
#ifdef YNR_CONTROL_INTERNAL
	//ynr_control_info_t ynr_control;
	sem_t ynr_sem;
#endif
	int32_t preview_width;
	int32_t preview_height;
	uint8_t af_state;
	uint8_t current_fnumber;
	uint8_t fnumber_changed;
	enum distance_info distance_state;
}sw_isp_handle_t;

static long long gTime;
static long long system_time()
{
	struct timespec t;
	t.tv_sec = t.tv_nsec = 0;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return (long long)(t.tv_sec)*1000000000LL + (long long)t.tv_nsec;
}

static void startTime()
{
	gTime = system_time();
}

static void endTime(const char *str)
{
	long long t = system_time() - gTime;
	double ds = ((double)t) / 1e9;
	SWISP_LOGI("Test: %s, %f s\n", str, ds);
}

static int update_param_to_internal(void*handle , struct soft_isp_block_param *isp_param)
{
	SWISP_LOGI("isp param blc bypass:%d , paramb:%d , isp_param is:%p" , isp_param->blc_info.bypass , isp_param->blc_info.b , isp_param);
	return sw_isp_update_param(handle , isp_param);
}

static int event_callback(void*handle , enum soft_isp_evt_id evt_id , void* raw , void* pre , void* vir_addr , int32_t size)
{
	struct soft_isp_cb_info cbinfo;
	sw_isp_handle_t *phandle = (sw_isp_handle_t*)handle;
	//        cbinfo.evt_id = evt_id;
	cbinfo.vir_addr = (cmr_uint)vir_addr;
	cbinfo.buf_size = size;
	if((NULL != raw) && (NULL != pre))
	{
		memcpy(&cbinfo.raw , raw , sizeof(struct isp_img_frm));
		memcpy(&cbinfo.m_yuv_pre , pre , sizeof(struct soft_isp_misc_img_frm));
	}
	if(phandle->param.evt_cb != NULL)
		phandle->param.evt_cb(evt_id , &cbinfo , phandle->param.isp_handle);
	return 0;
}

#if 1
static void set_stop_flag(void* handle , uint8_t flag)
{
	sw_isp_handle_t* phandle = (sw_isp_handle_t*)handle;
	pthread_mutex_lock(&phandle->control_mutex);
	phandle->stopflag = flag;
	SWISP_LOGI("set_stop flag:%d\n" , flag);
	pthread_mutex_unlock(&phandle->control_mutex);
}

#if 0
static void set_process_flag(void*handle , uint8_t flag)
{
	sw_isp_handle_t* phandle = (sw_isp_handle_t*)handle;
	pthread_mutex_lock(&phandle->control_mutex);
	phandle->process_flag = flag;
	//pthread_cond_signal(&phandle->control_state);
	pthread_mutex_unlock(&phandle->control_mutex);
}

static int wait_condition_process(void* handle)
{
	sw_isp_handle_t* phandle = (sw_isp_handle_t*)handle;
	pthred_mutex_lock(&phandle->control_mutex);
	while(phandle->process_flag)
	 pthread_cond_wait(&phandle->control_state, &phandle->control_mutex);
	pthread_mutex_unlock(&phandle->control_mutex);
	return 0;
}
#endif

static void set_misc_flag(void* handle , uint8_t flag)
{
	sw_isp_handle_t* phandle = (sw_isp_handle_t*)handle;
	pthread_mutex_lock(&phandle->control_misc_mutex);
	phandle->misc_calculating = flag;
	pthread_cond_signal(&phandle->control_misc_state);
	SWISP_LOGI("---------set_misc_flag flag:%d\n" , flag);
	pthread_mutex_unlock(&phandle->control_misc_mutex);
}
static void set_misc_depthcalculating(void *handle , uint8_t flag)
{
	sw_isp_handle_t* phandle = (sw_isp_handle_t*)handle;
	phandle->misc_depthcalculating  = flag;
}
static int wait_condition_miscprocess(void* handle)
{
	sw_isp_handle_t* phandle = (sw_isp_handle_t*)handle;
	pthread_mutex_lock(&phandle->control_misc_mutex);
	SWISP_LOGI("---------wait cond miscprocess enter\n");
	while(phandle->misc_calculating)
		pthread_cond_wait(&phandle->control_misc_state, &phandle->control_misc_mutex);
	SWISP_LOGI("---------wait cond miscprocess exit\n");
	pthread_mutex_unlock(&phandle->control_misc_mutex);
	return 0;
}
#endif
#if 0
static int release_swisp_resource(void* handle)
{
	sw_isp_handle_t* phandle = (sw_isp_handle_t*)handle;
	if(phandle == NULL)
	{
		SWISP_LOGE("release input handle is null");
		return -1;
	}
	if(phandle->depth_weight_ptr[0] != NULL)
	{
		free(phandle->depth_weight_ptr[0]);
	}
:wq
	if(phandle->swisp_handle != NULL)
	{
		deinit_sw_isp(phandle->swisp_handle);
	}
	if(phandle->bokeh_handle != NULL)
	{
		iBokehDeinit(phandle->bokeh_handle);
	}
	if(phandle->depth_handle != NULL)
	{
		sprd_depth_Close(phandle->depth_handle);
	}
}
#endif
__attribute__ ((visibility("default"))) int sprd_realtimebokeh_init(struct soft_isp_init_param *param, void **handle)
{
	void* sw_isp_handle;
	int ret;
	sw_isp_handle_t* phandle = (sw_isp_handle_t*)malloc(sizeof(sw_isp_handle_t));
	if(phandle == NULL)
	{
		SWISP_LOGE("malloc swIisp_handle failed");
		return -1;
	}
	memset(phandle , 0 , sizeof(sw_isp_handle_t));
	pthread_cond_init(&phandle->control_misc_state , NULL);
	pthread_mutex_init(&phandle->control_misc_mutex , NULL);
	pthread_mutex_init(&phandle->control_mutex , NULL);
	memset(phandle , 0 , sizeof(sw_isp_handle_t));
	phandle->misc_calculating = 0;
	phandle->misc_depthcalculating = 0;
	phandle->online_calbr_valid = 0;
	phandle->online_calbr_forceinvalid = 1;
	phandle->online_calbr_calculating = 1;
	phandle->stopflag = 0;
	phandle->process_flag = 0;
	phandle->depth_weight_valid_index = 0;
	phandle->start_flag = 0;
	SWISP_LOGI("------->otp size in init is:%d, mainnum:%d, subnum:%d\n" , param->otp_size , param->swisp_mainthreadnum , param->swisp_subthreadnum);
	phandle->otp_data = (uint8_t*)malloc(param->otp_size);
	phandle->otp_size = param->otp_size;
	if(phandle->otp_data == NULL)
	{
		free(phandle);
		return -1;
	}
	memcpy(phandle->otp_data , param->potp_buf , param->otp_size);
	/*online calibration width height, buffer is width*height*8 */
	phandle->ponline_calbinfo = (void*)malloc(param->online_calb_width * param->online_calb_height * sizeof(int));
	if(NULL == phandle->ponline_calbinfo)
	{
		SWISP_LOGE("alloc ponline calbration info failed");
		free(phandle->otp_data);
		free(phandle);
		return -1;
	}
	SWISP_LOGI("yzl ad online_calbinfo_size w:%d , h:%d" , param->online_calb_width , param->online_calb_height);
	phandle->online_calbinfo_size = param->online_calb_width * param->online_calb_height * sizeof(int);
	phandle->online_calbinfo_width = param->online_calb_width;
	phandle->online_calbinfo_height = param->online_calb_height;

	//init sw_isp
	startTime();
	sw_isp_handle = init_sw_isp(/*SW_ISP_IMG_WIDTH , SW_ISP_IMG_HEIGHT , SW_ISP_IMG_WIDTH , */param);
	endTime("init_sw_isp cost");
	if(sw_isp_handle == NULL)
	{
		SWISP_LOGE("init sw isp error null handle");
		free(phandle->otp_data);
		free(phandle->ponline_calbinfo);
		free(phandle);
		return -1;
	}
	phandle->swisp_handle = sw_isp_handle;
	if(param != NULL)
	{
		memcpy(&phandle->param , param , sizeof(struct soft_isp_init_param));
	}
	else
	{
		SWISP_LOGW("input param is NULL");
	}
	//init swisp/ynr/depth thread
	ret = sprd_pool_init(&phandle->threadpool , 1);
	if(ret != 0)
	{
		SWISP_LOGE("sprd_pool_init failed ret:%d" , ret);
		sprd_realtimebokeh_deinit(phandle);
		*handle = NULL;
		return -1;
	}
#ifdef YNR_CONTROL_INTERNAL
	sem_init(&phandle->ynr_sem , 0, 0);
#endif
	phandle->af_state = BOKEH_AF_NORMAL;
	phandle->distance_state = BOKEH_DISTANCE_OK;
	phandle->current_fnumber = 1;
	phandle->fnumber_changed = 0;
	*handle = phandle;
	return 0;
}
/*ioctl must be called after sprd_realtimebokeh_stop*/
__attribute__ ((visibility("default"))) int sprd_realtimebokeh_ioctl(void *handle, sprd_realtimebokeh_cmd cmd , void *outinfo)
{
	int ret = 0;
	sw_isp_handle_t* phandle = (sw_isp_handle_t*) handle;
	switch(cmd)
	{
	case REALTIMEBOKEH_CMD_GET_ONLINECALBINFO:
	{
		struct isp_depth_cali_info *pinfo;
		pinfo = outinfo;
		uint8_t value = phandle->online_calbr_valid;
		if(phandle->online_calbr_forceinvalid == 0)
		{
			pinfo->calbinfo_valid = phandle->online_calbr_valid;
		}
		else
		{
			phandle->online_calbr_valid = 0;
			pinfo->calbinfo_valid = 0;
		}
		SWISP_LOGI("yzl add online_calbr_forceinvalid:%d , online_calbr_valid:%d,\
				not corrected online_calbr_valid:%d" , phandle->online_calbr_forceinvalid , phandle->online_calbr_valid, value);
		pinfo->buffer = phandle->ponline_calbinfo;
		pinfo->size = phandle->online_calbinfo_size;
		SWISP_LOGI("get online calibration info valid:%d, buffer:%p" , pinfo->calbinfo_valid , pinfo->buffer);
		break;
	}
	default:
		SWISP_LOGE("sprd_realtimebokeh_ioctl cmd error");
		ret = -1;
		break;
	}
	return ret;
}
static int save_onlinecalbinfo(char *filename , uint8_t *input , uint32_t size)
{
	char file_name[256];
	FILE *fp;
        time_t timep;
        struct tm *p;
        time(&timep);
        p = localtime(&timep);
	sprintf(file_name , "/data/misc/cameraserver/%04d%02d%02d%02d%02d%02d_%s" ,(1900+p->tm_year),
                        (1+p->tm_mon) , p->tm_mday , p->tm_hour, p->tm_min , p->tm_sec , filename);
	fp = fopen(file_name , "wb");
	if(fp)
	{
		fwrite(input , size ,1 , fp);
		fclose(fp);
		return 0;
	}
	return -1;
}
static int savefile(char* filename , uint8_t* ptr , int width , int height, int yuv)
{
	int i;
	//static int inum = 0;
	char file_name[256];
	time_t timep;
	struct tm *p;
	time(&timep);
	p = localtime(&timep);
	sprintf(file_name , "/data/misc/cameraserver/%04d%02d%02d%02d%02d%02d_%s" ,(1900+p->tm_year),
			(1+p->tm_mon) , p->tm_mday , p->tm_hour, p->tm_min , p->tm_sec , filename);
	//if(inum < 5)
	FILE* fp = fopen(file_name , "wb");
	if (fp) {
#if 1
		//yuv
		if(yuv == 1)
			fwrite(ptr , 1 , width*height*3/2 , fp);
		else if (0 == yuv) { //raw
			uint16_t* rawptr = (uint16_t*)ptr;
			for (i = 0; i < height; i++)
				fwrite(rawptr  + i*width*2, 1 ,width*sizeof(uint16_t) , fp);
		} else if(2 == yuv) { //weight table
			uint16_t* rawptr = (uint16_t*)ptr;
			fwrite(rawptr, 1 ,width*height*sizeof(uint16_t) , fp);
		}
		else
#endif
		fclose(fp);
	}
//}
//	inum++;
	return 0;
}

#ifdef YNR_CONTROL_INTERNAL
static int ynr_process(sw_isp_handle_t *phandle)
{
	int ret;
	ynr_param_info_t ynrparam;
	ynrparam.src_width = phandle->misc_arg.s_yuv_sw_out.img_size.w;
	ynrparam.src_height = phandle->misc_arg.s_yuv_sw_out.img_size.h;
	ynrparam.dst_width = phandle->misc_arg.s_yuv_depth.img_size.w;
	ynrparam.dst_height = phandle->misc_arg.s_yuv_depth.img_size.h;
	ynrparam.src_buf_fd = phandle->misc_arg.s_yuv_sw_out.img_fd.y;
	ynrparam.dst_buf_fd = phandle->misc_arg.s_yuv_depth.img_fd.y;
	ret =  phandle->param.ynr_control(phandle->param.isp_handle , &ynrparam);
	if(ret != 0)
	{
		SWISP_LOGE("ynr_process ynr_control.isp_ioctl failed ret:%d",ret);
		return -1;
	}
	//wait isp ynr completed
	sem_wait(&phandle->ynr_sem);
	{
		char value[128];
		property_get(SAVE_SWISP_PROP , value , "no");
		if(0 == strcmp(value , "yes"))
		{
			char filename[256];
			sprintf(filename , "%dx%d_afterynr_index%d.yuv" , ynrparam.dst_width , ynrparam.dst_height , framenum);
			savefile(filename , (uint8_t*)phandle->misc_arg.s_yuv_depth.img_addr_vir.chn0 , ynrparam.dst_width , ynrparam.dst_height , 1);
			sprintf(filename , "%dx%d_beforeynr_index%d.yuv" , ynrparam.src_width , ynrparam.src_height , framenum);
			savefile(filename , (uint8_t*)phandle->misc_arg.s_yuv_sw_out.img_addr_vir.chn0 , ynrparam.src_width , ynrparam.src_height, 1);
		}
	}
	return 0;
}
#endif

static void* misc_process(void* handle)
{
	int ret;
	char filename[256];
	char value[128];
	sprintf(filename , "/data/misc/cameraserver/%dx%d_afterswisp.yuv" , 2104,1560);
	sw_isp_handle_t* phandle = (sw_isp_handle_t*)handle;
	void* aem_addr;
	int32_t aem_size;
	//uint16_t *ptemp = (uint16_t*)phandle->misc_arg.raw.img_addr_vir.chn0;
//	printf("---------micro raw data addr:%p , data:%hx,%hx,%hx,%hx\n" , ptemp , *ptemp ,*(ptemp+1),*(ptemp+2), *(ptemp+3));
	framenum++;
//	startTime();
	long long time = systemTime(CLOCK_MONOTONIC);
	long long timestart = time;
	update_param_to_internal(phandle->swisp_handle , &(phandle->misc_arg.isp_blockparam));

	ret = sw_isp_process(phandle->swisp_handle , (uint16_t*)phandle->misc_arg.raw.img_addr_vir.chn0+
		phandle->param.swisp_inputwidth , (uint8_t*)phandle->misc_arg.s_yuv_sw_out.img_addr_vir.chn0 ,
		&aem_addr , &aem_size);
//	endTime("sw_isp_process cost");
	SWISP_LOGI("----yzl add systemTime sw_isp_process cost time:%zd ms ,ret:%d\n" , (systemTime(CLOCK_MONOTONIC) - time)/1000000 , ret);
	if(ret != 0)
	{
		SWISP_LOGE("sw_isp_process failed ret:%d" , ret);
		event_callback(phandle , SOFT_BUFFER_RELEASE , &phandle->misc_arg.raw ,&phandle->misc_arg.m_yuv_pre , 0,0);
		set_misc_depthcalculating(handle , 0);
		set_misc_flag(phandle , 0);
		return NULL;
	}
	else
	{
		event_callback(phandle , SOFT_IRQ_AEM_STATIS , NULL ,NULL , aem_addr , aem_size);
		property_get(SAVE_SWISP_PROP , value , "no");
		if(0 == strcmp(value , "yes"))
		{
			sprintf(filename , "%dx%d_afterswisp_index%d.yuv"  , phandle->param.swisp_inputwidth/2 ,
				phandle->param.swisp_inputheight/2 , framenum);
			savefile(filename , (uint8_t*)phandle->misc_arg.s_yuv_sw_out.img_addr_vir.chn0 ,
				 phandle->param.swisp_inputwidth/2 , phandle->param.swisp_inputheight/2 , 1);
			sprintf(filename , "%dx%d_inpputraw_index%d.raw" ,phandle->param.swisp_inputwidth  ,
				phandle->param.swisp_inputheight , framenum);
			savefile(filename ,(void*)((uint16_t*)phandle->misc_arg.raw.img_addr_vir.chn0+
						   phandle->param.swisp_inputwidth) , phandle->param.swisp_inputwidth,
				 phandle->param.swisp_inputheight , 0);
			sprintf(filename , "%dx%d_inputpreview_index%d.yuv" ,phandle->preview_width  ,
				phandle->preview_height , framenum);
			savefile(filename ,(uint8_t*)phandle->misc_arg.m_yuv_pre.cpu_frminfo.img_addr_vir.chn0 , phandle->preview_width,
				 phandle->preview_height , 1);
		}
	}
	//check whether stop flag
	if(phandle->stopflag)
	{
		SWISP_LOGI("after sw_isp_process detect stop flag\n");
		event_callback(phandle , SOFT_BUFFER_RELEASE , &phandle->misc_arg.raw ,&phandle->misc_arg.m_yuv_pre , 0,0);
		set_misc_depthcalculating(handle , 0);
		set_misc_flag(phandle , 0);
		return NULL;
	}
	//ynr
#ifdef YNR_CONTROL_INTERNAL
	{
		ret = ynr_process(phandle);
		if(ret != 0)
		{
			SWISP_LOGE("ynr process failed ret:%d" , ret);
			event_callback(phandle , SOFT_BUFFER_RELEASE , &phandle->misc_arg.raw ,&phandle->misc_arg.m_yuv_pre , 0,0);
			set_misc_depthcalculating(handle , 0);
			set_misc_flag(phandle , 0);
			return NULL;
		}

		property_get("realbokeh.dump" , value, "no");
		if(strcmp(value , "yes") == 0)
		{
			sprintf(filename , "%dx%d_mainpreview_index%d.yuv" ,phandle->preview_width  ,
				phandle->preview_height , framenum);
			savefile(filename , (uint8_t*)phandle->misc_arg.m_yuv_pre.cpu_frminfo.img_addr_vir.chn0 , phandle->preview_width,
				 phandle->preview_height , 1);
			sprintf(filename , "%dx%d_subpreview_index%d.yuv" ,phandle->preview_width  ,
				phandle->preview_height , framenum);
			savefile(filename , (uint8_t*) phandle->misc_arg.s_yuv_depth.img_addr_vir.chn0 , phandle->misc_arg.s_yuv_depth.img_size.w,
				 phandle->misc_arg.s_yuv_depth.img_size.h , 1);
		}

	}
	if(phandle->stopflag)
	{
		SWISP_LOGE("after ynr process detect stop flag");
		event_callback(phandle , SOFT_BUFFER_RELEASE , &phandle->misc_arg.raw ,&phandle->misc_arg.m_yuv_pre , 0,0);
		set_misc_depthcalculating(handle , 0);
		set_misc_flag(phandle , 0);
		return NULL;
	}
#else

#endif
	//depth
	weightmap_param wParams;
	wParams.F_number = phandle->weight_param.F_number;
	wParams.sel_x = phandle->weight_param.sel_x;
	wParams.sel_y = phandle->weight_param.sel_y;
//	startTime();
	distanceRet distance;
	time = systemTime(CLOCK_MONOTONIC);
	SWISP_LOGI("----before sprd_depth_Run handle:%p , fnumber:%d ,x:%d,y:%d , depthweightptr:%p ,addr1:%p ,addr2:%p,%p\n" , phandle->depth_handle , wParams.F_number ,  wParams.sel_x ,
	wParams.sel_y , phandle->depth_weight_ptr[(phandle->depth_weight_valid_index+1)%2] , (void*)phandle->misc_arg.s_yuv_depth.img_addr_vir.chn0,
		(void*)phandle->misc_arg.m_yuv_pre.cpu_frminfo.img_addr_vir.chn0 , (void*)phandle->misc_arg.m_yuv_pre.graphicbuffer);



	// input: phandle->misc_arg.m_yuv_pre +  phandle->misc_arg.s_yuv_depth ; output:phandle->misc_arg.m_yuv_depth;
	ret = sprd_depth_Run_distance(phandle->depth_handle , (void*)phandle->depth_weight_ptr[(phandle->depth_weight_valid_index+1)%2] , NULL ,
					(void*)phandle->misc_arg.s_yuv_depth.img_addr_vir.chn0 , (void*)phandle->misc_arg.m_yuv_pre.cpu_frminfo.img_addr_vir.chn0 ,&wParams , &distance);
	distance = DISTANCE_OK;
//	memset(phandle->depth_weight_ptr[(phandle->depth_weight_valid_index+1)%2] , 0 , 960*720);
//	memset((void*)phandle->depth_weight_ptr[(phandle->depth_weight_valid_index+1)%2] , 0 , 960*720);
	SWISP_LOGI("-------micro process after Depth calculate use output index:%d , depthrun costtime:%zd ms, distance:%d , ret:%d\n" , (phandle->depth_weight_valid_index+1)%2 ,
		   (systemTime(CLOCK_MONOTONIC)-time)/1000000 , distance , ret);
	if(ret != 0)
	{
		SWISP_LOGW("sprd_depth_Run return error:%d" , ret);
	}
	else
	{
		char value[128];
		property_get(SAVE_SWISP_PROP , value , "no");
		if(0 == strcmp(value , "yes"))
		{
			char filename[256];
			sprintf(filename , "%dx%d_weight_table_index%d.bin" , phandle->preview_width  ,phandle->preview_height ,
				framenum);
			savefile(filename , (uint8_t *)phandle->depth_weight_ptr[(phandle->depth_weight_valid_index+1)%2] ,
				 phandle->preview_width , phandle->preview_height , 2);
		}
	}
//	endTime("depth Run");
//	pthread_mutex_lock(&phandle->control_mutex);
	phandle->depth_weight_valid_index = (phandle->depth_weight_valid_index+1)%2;
//	 pthread_mutex_unlock(&phandle->control_mutex);
	switch(distance)
	{
	case DISTANCE_OK:
		phandle->distance_state = BOKEH_DISTANCE_OK;
		break;
	case DISTANCE_FAR:
		phandle->distance_state = BOKEH_DISTANCE_TOOFAR;
		break;
	case DISTANCE_CLOSE:
		phandle->distance_state = BOKEH_DISTANCE_TOONEAR;
		break;
	default:
		phandle->distance_state = BOKEH_DISTANCE_ERROR;
		break;
	}
	set_misc_depthcalculating(handle , 0);

	//add depth online calibration
	if(1 == phandle->misc_arg.need_onlinecalb)
	{
		time = systemTime(CLOCK_MONOTONIC);
		ret = sprd_depth_OnlineCalibration(phandle->depth_handle , phandle->ponline_calbinfo , (void*)phandle->misc_arg.s_yuv_depth.img_addr_vir.chn0 , 
		(void*)phandle->misc_arg.m_yuv_pre.cpu_frminfo.img_addr_vir.chn0);
		SWISP_LOGI("online calibration info cost time:%lld ms" , (systemTime(CLOCK_MONOTONIC) - time)/1000000);
		if(ret == 0)
		{
			char value[128];
			char filename[128];
			static int index = 0;
			//set misc_calculating flag true;
			phandle->online_calbr_valid = 1;
			SWISP_LOGI("yzl add set phandle->online_calbr_valid 1");
			phandle->online_calbr_calculating = 0;
			property_get("save_onlinecalbinfo" , value , "no");
			if(!strcmp(value , "yes"))
			{
				sprintf(filename , "online_calibration_info_%d.dump" , index);
				save_onlinecalbinfo(filename , phandle->ponline_calbinfo , phandle->online_calbinfo_size);
				sprintf(filename , "%dx%d_online_inputslave_index%d.yuv" , phandle->misc_arg.s_yuv_depth.img_size.w , phandle->misc_arg.s_yuv_depth.img_size.h , index);
				savefile(filename , (void*)phandle->misc_arg.s_yuv_depth.img_addr_vir.chn0 , phandle->misc_arg.s_yuv_depth.img_size.w , phandle->misc_arg.s_yuv_depth.img_size.h , 1);
				sprintf(filename , "%dx%d_online_inputmain_index%d.yuv" , phandle->misc_arg.m_yuv_pre.cpu_frminfo.img_size.w , phandle->misc_arg.m_yuv_pre.cpu_frminfo.img_size.h , index);
				savefile(filename , (void*)phandle->misc_arg.m_yuv_pre.cpu_frminfo.img_addr_vir.chn0 ,phandle->misc_arg.m_yuv_pre.cpu_frminfo.img_size.w , phandle->misc_arg.m_yuv_pre.cpu_frminfo.img_size.h , 1);
				index++;
			}
		}
		else{
			SWISP_LOGE("calculate online calibration error");
		}
	}
	set_misc_flag(phandle , 0);
	event_callback(phandle , SOFT_BUFFER_RELEASE , &phandle->misc_arg.raw ,&phandle->misc_arg.m_yuv_pre , 0,0);
	SWISP_LOGI("misc_process cost time:%zd ms" , (systemTime(CLOCK_MONOTONIC) - timestart)/1000000);
	return NULL;
}

__attribute__ ((visibility("default"))) int sprd_realtimebokeh_stop(void* handle)
{
//	sw_isp_handle_t *phandle = (sw_isp_handle_t*)handle;
	SWISP_LOGI("---------sprd_realtimebokeh_stop enter\n");
	set_stop_flag(handle , 1);
	//check misc calculating?
	SWISP_LOGI("---------sprd_realtimebokeh_stop before wait_condition_miscprocess\n");
	wait_condition_miscprocess(handle);
	SWISP_LOGI("---------sprd_realtimebokeh_stop exit\n");
	return 0;
}

__attribute__ ((visibility("default"))) int sprd_realtimebokeh_start(void* handle , soft_isp_startparam_t* startparam)
{
	sw_isp_handle_t *phandle = (sw_isp_handle_t*)handle;
	int ret;
	if((phandle->start_flag == 0) && startparam != NULL)
	{
		framenum = 0;
		bokeh_num = 0;
		phandle->preview_width = startparam->preview_width;
		phandle->preview_height = startparam->preview_height;
		SWISP_LOGI("enter first time init depth/bokeh , width:%d , height:%d" , startparam->preview_width , startparam->preview_height);
		phandle->depth_weight_ptr[0] = (uint16_t*) malloc(startparam->preview_width*startparam->preview_height*sizeof(uint16_t)*3);
		phandle->depth_weight_ptr[1] = phandle->depth_weight_ptr[0] + startparam->preview_width*startparam->preview_height;
		phandle->depth_weight_ptr[2] = phandle->depth_weight_ptr[1] + startparam->preview_width*startparam->preview_height;
		if(phandle->depth_weight_ptr[0] == NULL)
		{
			SWISP_LOGE("malloc depth_weight_ptr failed");
			sprd_realtimebokeh_deinit(phandle);
			return -1;
		}
		memset(phandle->depth_weight_ptr[0] , 0  , startparam->preview_width*startparam->preview_height*sizeof(uint16_t)*3);
		//bokeh init
		InitParams bokehparams;
		bokehparams.width = startparam->preview_width;
		bokehparams.height = startparam->preview_height;
		bokehparams.depth_width = 0;  //weight map mode not used
		bokehparams.depth_height = 0; //weight map mode not used
		bokehparams.SmoothWinSize = 7;
		bokehparams.ClipRatio = 50;
		bokehparams.Scalingratio = 2;
		bokehparams.DisparitySmoothWinSize = 11;
		SWISP_LOGI("-----------bokeh preview width:%d ,height:%d\n" , bokehparams.width , bokehparams.height);
		ret = iBokehInit(&phandle->bokeh_handle, &bokehparams);
		//  	ret = 0;
		if(ret != 0)
		{
			SWISP_LOGE("iBokehInit failed ret:%d , need deinit swisp" , ret);
			sprd_realtimebokeh_deinit(phandle);
			return -1;
		}
		//depth init
		struct depth_init_inputparam inparam;
		inparam.input_width_main = startparam->preview_width;//160;
		inparam.input_height_main = startparam->preview_height;//120;
		inparam.input_width_sub = startparam->preview_width;//160;
		inparam.input_height_sub = startparam->preview_height;//120;
		inparam.output_depthwidth = DEPTH_OUTPUT_WIDTH;//PREVIEW_WIDTH;
		inparam.output_depthheight = DEPTH_OUTPUT_HEIGHT;//PREVIEW_HEIGHT;
		inparam.imageFormat_main = YUV420_NV12;
		inparam.imageFormat_sub = YUV420_NV12;
		inparam.depth_threadNum = 1;
		inparam.online_depthwidth =  phandle->online_calbinfo_width;
		inparam.online_depthheight = phandle->online_calbinfo_height;
		inparam.online_threadNum = 2;
		inparam.potpbuf = phandle->otp_data;//(uint8_t*)malloc(256); //may change later
		inparam.otpsize = phandle->otp_size;
		//		inparam.potpbuf = (uint8_t*)malloc(256);
		//		inparam.otpsize = 232;
		//      	memset(inparam.potpbuf , 0x23 , 232);
		inparam.config_param = (char*)(&sprd_depth_config_para);
		struct depth_init_outputparam outparam;
		SWISP_LOGI("-----depth init param otp:%p , size:%d , prreview width:%d ,height:%d , depth:%d,%d, thread:%d \
				, online threadnum:%d, online depth w:%d, h:%d\n" , inparam.potpbuf , inparam.otpsize ,
			   inparam.input_width_main , inparam.input_height_main , inparam.output_depthwidth , inparam.output_depthheight , inparam.depth_threadNum,
				inparam.online_threadNum , inparam.online_depthwidth , inparam.online_depthheight);
		phandle->depth_handle = sprd_depth_Init(&inparam , &outparam , MODE_CAPTURE , MODE_WEIGHTMAP);
		if(phandle->depth_handle == NULL)
		{
			SWISP_LOGE("sprd_depth_Init failed ret:%d, deinit swisp and bokeh" , ret);
			sprd_realtimebokeh_deinit(phandle);
			return -1;
		}
		//first start input s_yuv_depth&s_yuv_sw_out
		memcpy(&phandle->misc_arg.s_yuv_depth , &startparam->s_yuv_depth , sizeof(startparam->s_yuv_depth));
		memcpy(&phandle->misc_arg.s_yuv_sw_out , &startparam->s_yuv_sw_out , sizeof(startparam->s_yuv_sw_out));

		phandle->start_flag = 1;
	}
#ifdef YNR_CONTROL_INTERNAL
	sem_init(&phandle->ynr_sem , 0, 0);
#endif
	set_stop_flag(phandle , 0);
	return 0;
}

__attribute__ ((visibility("default"))) int sprd_realtimebokeh_process(void* handle , struct soft_isp_proc_param* isp_param)
{
	sw_isp_handle_t *phandle = (sw_isp_handle_t*)handle;
	int ret;
	int distancestate;
	char value[128];
	//set_process_flag(1);
	pthread_mutex_lock(&phandle->control_mutex);
	isp_param->bokeh_status = BOKEHCALCULATE_DISANCE_ERROR; //invalid
	SWISP_LOGI("-------- sprd_realtimebokeh_process enter , misc_calculating:%d , misc_depthcalculating:%d , afstatus:%d\n" , phandle->misc_calculating,
		   phandle->misc_depthcalculating , isp_param->af_status);
	if(isp_param->af_status == 1)
	{
		phandle->online_calbr_valid = 0;
		phandle->online_calbr_forceinvalid = 1;
	}
	if((phandle->stopflag == 1) || phandle->start_flag != 1)
	{
		SWISP_LOGI("-----------sprd_realtimebokeh_process stop flag return\n");
		pthread_mutex_unlock(&phandle->control_mutex);
		return -1;
	}

	property_get(CLOSE_REALBOKEH , value , "no");
	if(0 == strcmp(value , "yes"))
	{
		memcpy((void*)isp_param->m_yuv_bokeh.cpu_frminfo.img_addr_vir.chn0 ,(void*)isp_param->m_yuv_pre.cpu_frminfo.img_addr_vir.chn0 ,
		       isp_param->m_yuv_pre.cpu_frminfo.img_size.h*isp_param->m_yuv_pre.cpu_frminfo.img_size.w*3/2);
		pthread_mutex_unlock(&phandle->control_mutex);
		return 0;
	}
	if(phandle->current_fnumber != isp_param->weightparam.F_number)
	{
		phandle->fnumber_changed = 1;
		phandle->current_fnumber = isp_param->weightparam.F_number;
	}
	if(isp_param->af_status == 1)
	{
		phandle->af_state = BOKEH_AF_FOCUSING;
//		phandle->online_calbr_valid = 0;
		memcpy((void*)isp_param->m_yuv_bokeh.cpu_frminfo.img_addr_vir.chn0 ,(void*)isp_param->m_yuv_pre.cpu_frminfo.img_addr_vir.chn0 ,
		       isp_param->m_yuv_pre.cpu_frminfo.img_size.h*isp_param->m_yuv_pre.cpu_frminfo.img_size.w*3/2);
		event_callback(phandle , SOFT_BUFFER_RELEASE , &isp_param->raw ,&isp_param->m_yuv_pre , 0,0);
		pthread_mutex_unlock(&phandle->control_mutex);
		//	*status = REALTIMEBOKEH_STATUS_FOCUSING;
		return 0;
	}
	if(phandle->af_state == BOKEH_AF_FOCUSING)
	{
		phandle->af_state = BOKEH_AF_RESUMSTATE1;
		SWISP_LOGI("af_state from FOCUSING TO STATE1");
	}
	if((phandle->misc_depthcalculating == 0) && (phandle->af_state == BOKEH_AF_RESUMSTATE2))
	{
		phandle->af_state = BOKEH_AF_NORMAL;
		SWISP_LOGI("af_state from STATE2 to NORMAL");
	}
	//check whether last depth calcuate over
	if(((phandle->misc_calculating == 0) && (phandle->af_state != BOKEH_AF_NORMAL)) ||\
	   ((phandle->misc_calculating == 0) && (phandle->fnumber_changed == 1)))
	{
		//update depth data
		//start sw isp
		SWISP_LOGI("----------afstate:%d , fnumber_changed:%d" , phandle->af_state , phandle->fnumber_changed);
		phandle->misc_calculating = 1;
		phandle->misc_depthcalculating = 1;
		phandle->fnumber_changed = 0;
		if(phandle->af_state == BOKEH_AF_RESUMSTATE1)
		{
			phandle->af_state = BOKEH_AF_RESUMSTATE2;
			phandle->misc_arg.need_onlinecalb = 1;
			phandle->online_calbr_calculating = 1;
			phandle->online_calbr_forceinvalid = 0;
			SWISP_LOGI("af_state from STATE1 to STATE2");
		}
		else
		{
			phandle->misc_arg.need_onlinecalb = 0;
		}
		SWISP_LOGI("--------------sprd_sw_isp_process misc calculate over af_state:%d\n" , phandle->af_state);
		//memcpy(&phandle->isp_block_param , &isp_param->block , sizeof(struct soft_isp_block_param));
#if 0
		update_param_to_internal(phandle->swisp_handle , isp_param);
#endif
		phandle->weight_param.F_number = isp_param->weightparam.F_number;
		phandle->weight_param.sel_x = isp_param->weightparam.sel_x;
		phandle->weight_param.sel_y = isp_param->weightparam.sel_y;
		//		weightparam.DisparityImage =
		memcpy(&phandle->misc_arg.raw , &isp_param->raw , sizeof(isp_param->raw));
		memcpy(&phandle->misc_arg.m_yuv_pre , &isp_param->m_yuv_pre , sizeof(isp_param->m_yuv_pre));
		memcpy(&phandle->misc_arg.isp_blockparam , &isp_param->block , sizeof(isp_param->block));
		SWISP_LOGI("-----------isp_param raw addr:%p , misc_arg raw_addr:%p\n" , (void *)isp_param->raw.img_addr_vir.chn0 , (void *)phandle->misc_arg.raw.img_addr_vir.chn0);
		//phandle->depth_weight_valid_index = (phandle->depth_weight_valid_index+1)%2;
		//		SWISP_LOGI("ptemp_raw size:%dx%d , ptemp_pre:%dx%d" , phandle->param.swisp_inputwidth, phandle->param.swisp_inputheight,
		//					phandle->preview_width , phandle->preview_height);
		sprd_pool_add_worker(phandle->threadpool , misc_process , phandle);
		isp_param->release_status = 1;

	}
#if 1
	//startTime();
	long long time = systemTime(CLOCK_MONOTONIC);
	//bokeh input
	distancestate = phandle->distance_state;
	if((phandle->af_state == BOKEH_AF_NORMAL) && (distancestate == BOKEH_DISTANCE_OK))
	{
		isp_param->weightparam.DisparityImage = (uint8_t*)phandle->depth_weight_ptr[phandle->depth_weight_valid_index];//use last weitmap calculated
		SWISP_LOGI("---------before calculate weight map use index:%d , disparityImage:%p , af_state:%d\n" , phandle->depth_weight_valid_index , phandle->depth_weight_ptr[phandle->depth_weight_valid_index] , phandle->af_state);
		ret = iBokehCreateWeightMap(phandle->bokeh_handle , /*&weightparam*/&isp_param->weightparam);
		SWISP_LOGI("---------calculate weight map use index:%d\n" , phandle->depth_weight_valid_index);
		if(ret != 0)
		{
			SWISP_LOGE("iBokehCreateWeightMap falied ret:%d" , ret);
			pthread_mutex_unlock(&phandle->control_mutex);
			return -1;
		}
		ret = iBokehBlurImage(phandle->bokeh_handle , (void*)isp_param->m_yuv_pre.graphicbuffer ,
				      (void*)isp_param->m_yuv_bokeh.graphicbuffer);
		SWISP_LOGI("bokeh cost time:%lld ms ret:%d\n" , (systemTime(CLOCK_MONOTONIC)-time)/1000000 , ret);
		if(ret != 0)
		{
			SWISP_LOGE("iBokehBlurImage failed ret:%d" , ret);
			pthread_mutex_unlock(&phandle->control_mutex);
			return -1;
		}
		isp_param->bokeh_status = BOKEHCALCULATE_DISANCE_OK;
#if 0
		if(phandle->online_calbr_calculating == 0)
			phandle->online_calbr_valid = 1;
#endif
	}
	else
	{
		memcpy((void*)isp_param->m_yuv_bokeh.cpu_frminfo.img_addr_vir.chn0 ,(void*)isp_param->m_yuv_pre.cpu_frminfo.img_addr_vir.chn0 ,
		       isp_param->m_yuv_pre.cpu_frminfo.img_size.h*isp_param->m_yuv_pre.cpu_frminfo.img_size.w*3/2);
		if(phandle->af_state == BOKEH_AF_NORMAL)
		{
			switch(distancestate)
			{
			case BOKEH_DISTANCE_TOOFAR:
				SWISP_LOGI("distance is too far");
				isp_param->bokeh_status = BOKEHCALCULATE_DISANCE_TOOFAR; //1 is too far
				break;
			case BOKEH_DISTANCE_TOONEAR:
				SWISP_LOGI("distance is too near");
				isp_param->bokeh_status = BOKEHCALCULATE_DISANCE_TOONEAR; //2 is too near
				break;
			case BOKEH_DISTANCE_ERROR:
				SWISP_LOGI("distance calculate is error");
				isp_param->bokeh_status = BOKEHCALCULATE_DISANCE_ERROR; //3 is error
				break;
			default:
				isp_param->bokeh_status = BOKEHCALCULATE_DISANCE_ERROR;
				SWISP_LOGW("distance is other value:%d" , phandle->distance_state);
				break;
			}
		}
		SWISP_LOGI("bokeh use memcpy cost time:%zd ms\n" , (systemTime(CLOCK_MONOTONIC)-time)/1000000);
	}

	{
		char value[128];
		property_get("realbokeh.dump" , value , "no");
		if(0 == strcmp(value , "yes"))
		{
			char filename[256];
			sprintf(filename , "%dx%d_bokehresult_index%d.yuv" , isp_param->m_yuv_bokeh.cpu_frminfo.img_size.w , isp_param->m_yuv_bokeh.cpu_frminfo.img_size.h , bokeh_num);
			savefile(filename , (uint8_t*)isp_param->m_yuv_bokeh.cpu_frminfo.img_addr_vir.chn0 ,isp_param->m_yuv_bokeh.cpu_frminfo.img_size.w ,isp_param->m_yuv_bokeh.cpu_frminfo.img_size.h , 1);
			sprintf(filename , "%dx%d_bokehinput_index%d.yuv" , isp_param->m_yuv_pre.cpu_frminfo.img_size.w , isp_param->m_yuv_pre.cpu_frminfo.img_size.h , bokeh_num);
			savefile(filename , (uint8_t*)isp_param->m_yuv_pre.cpu_frminfo.img_addr_vir.chn0 ,isp_param->m_yuv_pre.cpu_frminfo.img_size.w ,isp_param->m_yuv_pre.cpu_frminfo.img_size.h , 1);
			sprintf(filename , "bokehweighttable_index%d.bin" , bokeh_num);
			savefile(filename , isp_param->weightparam.DisparityImage , isp_param->m_yuv_pre.cpu_frminfo.img_size.w,
				 isp_param->m_yuv_pre.cpu_frminfo.img_size.h , 2);
		}
	}
#endif
	bokeh_num++;
	pthread_mutex_unlock(&phandle->control_mutex);
	return 0;
}

__attribute__ ((visibility("default"))) int sprd_realtimebokeh_deinit(void* handle)
{
	sw_isp_handle_t* phandle = (sw_isp_handle_t*)handle;
	//threadpool must be destroy first for it using depth handle and some other resources
	if(phandle->threadpool)
	{
		SWISP_LOGI("free threadpool");
		sprd_pool_destroy(phandle->threadpool);
	}
	if(phandle->depth_weight_ptr[0])
	{
		SWISP_LOGI("free depth_weight_ptr");
		free(phandle->depth_weight_ptr[0]);
	}
	if(phandle->swisp_handle)
	{
		SWISP_LOGI("free swisp_hanle");
		deinit_sw_isp(phandle->swisp_handle);
	}
	if(phandle->bokeh_handle)
	{
		SWISP_LOGI("free bokeh_handle");
		iBokehDeinit(phandle->bokeh_handle);
	}
	if(phandle->depth_handle)
	{
		SWISP_LOGI("free depth_handle");
		sprd_depth_Close(phandle->depth_handle);
	}
	if(phandle->otp_data)
	{
		SWISP_LOGI("free otp data");
		free(phandle->otp_data);
	}
	if(phandle->ponline_calbinfo)
	{
		SWISP_LOGI("free ponline calibration info");
		free(phandle->ponline_calbinfo);
	}
#ifdef YNR_CONTROL_INTERNAL
	sem_destroy(&phandle->ynr_sem);
#endif
	pthread_cond_destroy(&phandle->control_misc_state);
	pthread_mutex_destroy(&phandle->control_misc_mutex);
	pthread_mutex_destroy(&phandle->control_mutex);
	free(phandle);
	return 0;
}
#ifdef YNR_CONTROL_INTERNAL
__attribute__ ((visibility("default"))) void sprd_realtimebokeh_ynr_callback(void* handle)
{
	sw_isp_handle_t* phandle = (sw_isp_handle_t*)handle;
	sem_post(&phandle->ynr_sem);
}
#endif
