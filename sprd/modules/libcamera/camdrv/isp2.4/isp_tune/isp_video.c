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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdint.h>
#include <sys/types.h>
#include <semaphore.h>
#include <pthread.h>
#define LOG_TAG "isp_video"
#include <cutils/log.h>
#include "isp_param_tune_com.h"
#include "isp_type.h"
#include "isp_video.h"
#include "sensor_raw.h"
#define DATA_CMD_LENGTH 32
#define PACKET_MSG_HEADER 8
#define ISP_READ_MODE_ID_MAX 13

#define ISP_NR_BLOCK_MIN 0
#define ISP_ISO_NUM_MAX 7
#define ISP_ISO_NUM_MIN 0
#define ISP_AE_WEIGHT_TYPE_MAX 2
#define ISP_AE_WEIGHT_TYPE_MIN 0
#define ISP_SCENE_NUM_MAX 7
#define ISP_SCENE_NUM_MIN 0
#define ISP_ANTIFLICKER_MIN 0
#define ISP_ANTIFLICKER_MAX 1
#define ISP_LNC_TAB_MAX 8
#define ISP_LNC_TAB_MIN 0
#define ISP_SIMULATOR_MAX 0xffff
#define ISP_SIMULATOR_MIN 0x0000
enum {
	CMD_START_PREVIEW = 1,
	CMD_STOP_PREVIEW,
	CMD_READ_ISP_PARAM,
	CMD_WRITE_ISP_PARAM,
	CMD_GET_PREVIEW_PICTURE,
	CMD_AUTO_UPLOAD,
	CMD_UPLOAD_MAIN_INFO,
	CMD_TAKE_PICTURE,
	CMD_ISP_LEVEL,
	CMD_READ_SENSOR_REG,
	CMD_WRITE_SENSOR_REG,
	CMD_GET_INFO,
	CMD_OTP_WRITE,
	CMD_OTP_READ,
	CMD_READ_ISP_PARAM_V1,
	CMD_WRITE_ISP_PARAM_V1,
	CMD_DOWNLOAD_RAW_PIC,
	CMD_WRTIE_SCENE_PARAM,
	CMD_START_SIMULATION,

	CMD_SFT_READ = 200,
	CMD_SFT_WRITE,
	CMD_SFT_TRIG,
	CMD_SFT_SET_POS,
	CMD_SFT_GET_POS,
	CMD_SFT_TAKE_PICTURE,
	CMD_SFT_TAKE_PICTURE_NEW,
	CMD_SFT_OPEN_FILTER,
	CMD_SFT_GET_AF_VALUE,

	CMD_ASIC_TAKE_PICTURE,
	CMD_ASIC_TAKE_PICTURE_NEW
};

typedef enum {
	SCENEINFO = 0x00,
	BDN,
	BPC,
	UVDIV,
	CFA,
	EDGE,
	FLAT_OFFSET,
	GRGB,
	IIRCNR_YRANDOM,
	IIRCNR_IIR,
	IVST,
	NLM,
	PREF,
	PWD,
	RGB_PRECDN,
	VST,
	YIQ_AFM,
	UV_CDN,
	UV_POSTCDN,
	YUV_PRECDN,
	V21PPI = 0x14,
	V21BAYER_NR,
	V21VST,
	V21IVST,
	V21DITHER,
	V21BPC,
	V21GRGB,
	V21CFA,
	V21RGB_AFM,
	V21UVDIV,
	V21PRE3DNR,
	V21CAP3DNR,
	V21EDGE,
	V21PRECDN,
	V21Y_NR,
	V21CDN,
	V21POSTCDN,
	V21IIRCNR,
	V21NOISEFILTER,
	PIKE2NLM = 0x27,
	PIKE2BPC = 0x28,
	PIKE2RGBPRECDN = 0x29,
	PIKE2RGBAFM = 0x30,
	PIKE2FLATOFFSET = 0x31,
	PIKE2IVST = 0x32,
	PIKE2VST = 0x33,
	FILE_NAME_MAX
} DENOISE_DATA_NAME;

typedef enum {
	NORMAL = 0x00,
	NIGHT,
	SPORT,
	PORTRAIT,
	LANDSPACE,
	PANORAMA,
	USER0,
	USER1,
	USER2,
	USER3,
	USER4,
	USER5,
	USER6,
	USER7,
	USER8,
	USER9,
	SCENE_MODE_MAX
} DENOISE_SCENE_NAME;

enum {
	MODE_DUMP_DATA,
	MODE_NR_DATA,
	MODE_ISP_ID = 0x02,
	MODE_TUNE_INFO = 0x03,
	MODE_AE_TABLE,
	MODE_AE_WEIGHT_TABLE,
	MODE_AE_SCENE_TABLE,
	MODE_AE_AUTO_ISO_TABLE,
	MODE_LNC_DATA,
	MODE_AWB_DATA,
	MODE_NOTE_DATA,
	MODE_LIB_INFO_DATA,
	MODE_MAX
};

enum {
	/* 'FIVE' is 50Hz,'SIX' is 60Hz;
	 * 'ONE/TWO/FOUR....' is ISO100/ISO200/ISO400....
	 */
	AE_TABLE_FIVE_AUTO = 0x00,
	AE_TABLE_FIVE_ONE,
	AE_TABLE_FIVE_TWO,
	AE_TABLE_FIVE_FOUR,
	AE_TABLE_FIVE_AEIGHT,
	AE_TABLE_FIVE_SIXTEEN,
	AE_TABLE_SIX_AUTO = 0x10,
	AE_TABLE_SIX_ONE,
	AE_TABLE_SIX_TWO,
	AE_TABLE_SIX_FOUR,
	AE_TABLE_SIX_AEIGHT,
	AE_TABLE_SIX_SIXTEEN,
	AE_TABLE_MAX
};

enum {
	AE_WEIGHT_AVG = 0X00,
	AE_WEIGHT_CENTER,
	AE_WEIGHT_SPOT,
	AE_WEIGHT_MAX
};

enum {
	AE_SCENE_ZERO_FIVE = 0x00,
	AE_SCENE_ZERO_SIX,
	AE_SCENE_ONE_FIVE = 0x10,
	AE_SCENE_ONE_SIX,
	AE_SCENE_TWO_FIVE = 0x20,
	AE_SCENE_TWO_SIX,
	AE_SCENE_THREE_FIVE = 0x30,
	AE_SCENE_THREE_SIX,
	AE_SCENE_FOUR_FIVE = 0x40,
	AE_SCENE_FOUR_SIX,
	AE_SCENE_FIVE_FIVE = 0x50,
	AE_SCENE_FIVE_SIX,
	AE_SCENE_SIX_FIVE = 0x60,
	AE_SCENE_SIX_SIX,
	AE_SCENE_SEVEN_FIVE = 0x70,
	AE_SCENE_SEVEN_SIX,
	AE_SCENE_MAX
};

enum {
	AE_AUTO_ISO_ONE = 0x00,
	AE_AUTO_ISO_TWO,
	AE_AUTO_ISO_MAX
};

enum {
	LNC_CT_ZERO = 0x00,
	LNC_CT_ONE,
	LNC_CT_TWO,
	LNC_CT_THREE,
	LNC_CT_FOUR,
	LNC_CT_FIVE,
	LNC_CT_SIX,
	LNC_CT_SEVEN,
	LNC_CT_EIGHT,
	LNC_CT_MAX
};

enum {
	AWB_WIN_MAP = 0x00,
	AWB_WEIGHT,
	AWB_MAX
};

typedef struct msg_head_tag {
	cmr_u32 seq_num;	// Message sequence number, used for flow control
	cmr_u16 len;		// The totoal size of the packet "sizeof(MSG_HEAD_T)
	cmr_u8 type;		// Main command type
	cmr_u8 subtype;		// Sub command type
} __attribute__ ((packed)) MSG_HEAD_T;

typedef struct {
	cmr_u32 headlen;
	cmr_u32 img_format;
	cmr_u32 img_size;
	cmr_u32 totalpacket;
	cmr_u32 packetsn;
} ISP_IMAGE_HEADER_T;

typedef struct {
	cmr_u32 totalpacket;
	cmr_u32 packetsn;
	cmr_u32 reserve[3];
} ISP_DATA_HEADER_T;

struct isp_data_header_read {
	cmr_u16 main_type;
	cmr_u16 sub_type;
	cmr_u8 isp_mode;
	cmr_u8 nr_mode;
	cmr_u8 reserved[26];
} __attribute__ ((packed));

struct isp_data_header_normal {
	cmr_u32 data_total_len;
	cmr_u16 main_type;
	cmr_u16 sub_type;
	cmr_u8 isp_mode;
	cmr_u8 packet_status;
	cmr_u8 nr_mode;
	cmr_u8 reserved[21];
} __attribute__ ((packed));

struct isp_check_cmd_valid {
	cmr_u16 main_type;
	cmr_u16 sub_type;
	cmr_u8 isp_mode;
} __attribute__ ((packed));

struct camera_func {
	cmr_s32(*start_preview) (cmr_u32 param1, cmr_u32 param2);
	cmr_s32(*stop_preview) (cmr_u32 param1, cmr_u32 param2);
	cmr_s32(*take_picture) (cmr_u32 param1, cmr_u32 param2);
	cmr_s32(*set_capture_size) (cmr_u32 width, cmr_u32 height);
};

#define PREVIEW_MAX_WIDTH 640
#define PREVIEW_MAX_HEIGHT 480

#define CMD_BUF_SIZE  65536
#define SEND_IMAGE_SIZE 64512
#define SEND_DATA_SIZE 64512
#define DATA_BUF_SIZE 65536
#define PORT_NUM 16666		/* Port number for server */
#define BACKLOG 5
#define ISP_CMD_SUCCESS             0x0000
#define ISP_CMD_FAIL                0x0001
#define IMAGE_RAW_TYPE 0
#define IMAGE_YUV_TYPE 1

#define CLIENT_DEBUG
#ifdef CLIENT_DEBUG
#define DBG ISP_LOGE
#endif
static cmr_u8 *preview_buf_ptr = 0;
static cmr_u8 diag_rx_buf[CMD_BUF_SIZE];
static cmr_u8 temp_rx_buf[CMD_BUF_SIZE];
static cmr_u8 diag_cmd_buf[CMD_BUF_SIZE];
static cmr_u8 eng_rsp_diag[DATA_BUF_SIZE];
static cmr_s32 preview_flag = 0;
static cmr_s32 preview_img_end_flag = 1;
static cmr_s32 capture_img_end_flag = 1;
static cmr_s32 capture_format = 1;		// 1: start preview
static cmr_s32 capture_flag = 0;		// 1: call get pic
char raw_filename[200] = { 0 };

cmr_u32 tool_fmt_pattern = INVALID_FORMAT_PATTERN;
static FILE *raw_fp = NULL;
static struct isptool_scene_param scene_param = { 0, 0, 0, 0, 0, 0, 0, 0 };

static sem_t preview_sem_lock;
static sem_t capture_sem_lock;
static pthread_mutex_t ispstream_lock;
static cmr_s32 wire_connected = 0;
static cmr_s32 sockfd = 0;
static cmr_s32 sock_fd = 0;
static cmr_s32 rx_packet_len = 0;
static cmr_s32 rx_packet_total_len = 0;
cmr_s32 sequence_num = 0;
struct camera_func s_camera_fun = { PNULL, PNULL, PNULL, PNULL };

struct camera_func *s_camera_fun_ptr = &s_camera_fun;
static void *isp_handler;

static cmr_u32 g_af_pos = 0;
static cmr_u32 g_command = CMD_TAKE_PICTURE;
struct denoise_param_update nr_update_param;

struct camera_func *ispvideo_GetCameraFunc(void)
{
	return s_camera_fun_ptr;
}

cmr_s32 ispvideo_SetreTurnValue(cmr_u8 * rtn_ptr, cmr_u32 rtn)
{
	cmr_u32 *rtn_value = (cmr_u32 *) rtn_ptr;

	*rtn_value = rtn;

	return 0x04;
}

cmr_u32 ispvideo_GetIspParamLenFromSt(cmr_u8 * dig_ptr)
{
	cmr_u32 data_len = 0x00;
	cmr_u16 *data_ptr = (cmr_u16 *) (dig_ptr + 0x05);

	if (NULL != dig_ptr) {
		data_len = data_ptr[0];
		data_len -= 0x08;
	}

	return data_len;
}

cmr_u32 ispvideo_GetImgDataLen(cmr_u8 * dig_ptr)
{
	cmr_u32 data_len = 0x00;
	cmr_u16 *data_ptr = (cmr_u16 *) (dig_ptr + 0x05);

	if (NULL != dig_ptr) {
		data_len = data_ptr[0];
		data_len -= 0x08;
		data_len -= sizeof(ISP_IMAGE_HEADER_T);
	}

	return data_len;
}

cmr_u8 *ispvideo_GetImgDataInfo(cmr_u8 * dig_ptr, cmr_u32 * packet_sn, cmr_u32 * total_pack, cmr_u32 * img_width, cmr_u32 * img_height, cmr_u32 * img_headlen)
{
	cmr_u8 *data_ptr = NULL;
	char *tmp_ptr;
	ISP_IMAGE_HEADER_T img_info;

	if (!dig_ptr || !packet_sn || !total_pack || !img_width || !img_height) {
		ISP_LOGE("fail to check param");
		return data_ptr;
	}

	tmp_ptr = (char *)dig_ptr + 1 + sizeof(MSG_HEAD_T);

	memcpy((char *)&img_info, (char *)tmp_ptr, sizeof(ISP_IMAGE_HEADER_T));
	*img_headlen = img_info.headlen;
	*packet_sn = img_info.packetsn;
	*total_pack = img_info.totalpacket;
	*img_width = (img_info.img_size >> 0x10) & 0xFFFF;
	*img_height = img_info.img_size & 0xFFFF;
	ISP_LOGV("ImageInfo headlen %d, packetsn %d, total_packet %d, img_width %d, img_height %d", *img_headlen, *packet_sn, *total_pack, *img_width, *img_height);

	data_ptr = dig_ptr + 1 + sizeof(MSG_HEAD_T) + sizeof(ISP_IMAGE_HEADER_T);
	return data_ptr;
}

cmr_s32 ispvideo_GetSceneInfo(cmr_u8 * dig_ptr, struct isptool_scene_param * scene_parm)
{
	cmr_u8 *data_ptr = NULL;
	cmr_s32 rtn = 0;

	if (!dig_ptr || !scene_parm) {
		ISP_LOGE("fail to check param");
		return -1;
	}

	data_ptr = dig_ptr + 1 + sizeof(MSG_HEAD_T);
	/* little endian */
	memcpy(((char *)scene_parm) + 8, data_ptr, sizeof(struct isptool_scene_param) - 8);

	return rtn;
}

cmr_s32 ispvideo_GetIspParamFromSt(cmr_u8 * dig_ptr, struct isp_parser_buf_rtn * isp_ptr)
{
	cmr_s32 rtn = 0x00;

	if ((NULL != dig_ptr)
	    && (NULL != (void *)(isp_ptr->buf_addr))
	    && (0x00 != isp_ptr->buf_len)) {
		memcpy((void *)isp_ptr->buf_addr, (void *)dig_ptr, isp_ptr->buf_len);
	}

	return rtn;
}

void *ispvideo_GetIspHandle(void)
{
	return isp_handler;
}

cmr_u32 ispvideo_SetIspParamToSt(cmr_u8 * dig_ptr, struct isp_parser_buf_in * isp_ptr)
{
	cmr_u32 buf_len = 0x00;

	if ((NULL != dig_ptr)
	    && (NULL != (void *)(isp_ptr->buf_addr))
	    && (0x00 != isp_ptr->buf_len)) {
		memcpy((void *)dig_ptr, (void *)isp_ptr->buf_addr, isp_ptr->buf_len);

		buf_len = isp_ptr->buf_len;
	}

	return buf_len;
}

static cmr_s32 handle_img_data(cmr_u32 format, cmr_u32 width, cmr_u32 height, char *ch0_ptr, cmr_s32 ch0_len, char *ch1_ptr, cmr_s32 ch1_len, char *ch2_ptr, cmr_s32 ch2_len)
{
	cmr_s32 i, res, total_number;
	cmr_s32 send_number = 0;
	cmr_s32 chn0_number, chn1_number, chn2_number;
	cmr_s32 len = 0, rsp_len = 0;
	MSG_HEAD_T *msg_ret;
	ISP_IMAGE_HEADER_T isp_msg;

	chn0_number = (ch0_len + SEND_IMAGE_SIZE - 1) / SEND_IMAGE_SIZE;
	chn1_number = (ch1_len + SEND_IMAGE_SIZE - 1) / SEND_IMAGE_SIZE;
	chn2_number = (ch2_len + SEND_IMAGE_SIZE - 1) / SEND_IMAGE_SIZE;

	total_number = chn0_number + chn1_number + chn2_number;

	msg_ret = (MSG_HEAD_T *) (eng_rsp_diag + 1);

	for (i = 0; i < chn0_number; i++, send_number++) {
		if (i < chn0_number - 1)
			len = SEND_IMAGE_SIZE;
		else
			len = ch0_len - SEND_IMAGE_SIZE * i;
		rsp_len = sizeof(MSG_HEAD_T) + 1;

		isp_msg.headlen = 20;
		isp_msg.img_format = format;
		isp_msg.img_size = (width << 0x10) | height;
		isp_msg.totalpacket = total_number;
		isp_msg.packetsn = send_number + 1;

		memcpy(eng_rsp_diag + rsp_len, (char *)&isp_msg, sizeof(ISP_IMAGE_HEADER_T));
		rsp_len += sizeof(ISP_IMAGE_HEADER_T);

		memcpy(eng_rsp_diag + rsp_len, (char *)ch0_ptr + i * SEND_IMAGE_SIZE, len);
		rsp_len += len;

		eng_rsp_diag[rsp_len] = 0x7e;
		msg_ret->len = rsp_len - 1;
		msg_ret->seq_num = sequence_num++;
		res = send(sockfd, eng_rsp_diag, rsp_len + 1, 0);

	}

	for (i = 0; i < chn1_number; i++, send_number++) {
		if (i < chn1_number - 1)
			len = SEND_IMAGE_SIZE;
		else
			len = ch1_len - SEND_IMAGE_SIZE * i;
		rsp_len = sizeof(MSG_HEAD_T) + 1;

		isp_msg.headlen = 20;
		isp_msg.img_format = format;
		isp_msg.img_size = (width << 0x10) | height;
		isp_msg.totalpacket = total_number;
		isp_msg.packetsn = send_number + 1;

		memcpy(eng_rsp_diag + rsp_len, (char *)&isp_msg, sizeof(ISP_IMAGE_HEADER_T));
		rsp_len += sizeof(ISP_IMAGE_HEADER_T);

		memcpy(eng_rsp_diag + rsp_len, (char *)ch1_ptr + i * SEND_IMAGE_SIZE, len);
		rsp_len += len;

		eng_rsp_diag[rsp_len] = 0x7e;
		msg_ret->len = rsp_len - 1;
		msg_ret->seq_num = sequence_num++;
		res = send(sockfd, eng_rsp_diag, rsp_len + 1, 0);

	}

	for (i = 0; i < chn2_number; i++, send_number++) {
		if (i < chn2_number - 1)
			len = SEND_IMAGE_SIZE;
		else
			len = ch2_len - SEND_IMAGE_SIZE * i;
		rsp_len = sizeof(MSG_HEAD_T) + 1;

		isp_msg.headlen = 20;
		isp_msg.img_format = format;
		isp_msg.img_size = (width << 0x10) | height;
		isp_msg.totalpacket = total_number;
		isp_msg.packetsn = send_number + 1;

		memcpy(eng_rsp_diag + rsp_len, (char *)&isp_msg, sizeof(ISP_IMAGE_HEADER_T));
		rsp_len += sizeof(ISP_IMAGE_HEADER_T);

		memcpy(eng_rsp_diag + rsp_len, (char *)ch2_ptr + i * SEND_IMAGE_SIZE, len);
		rsp_len += len;

		eng_rsp_diag[rsp_len] = 0x7e;
		msg_ret->len = rsp_len - 1;
		msg_ret->seq_num = sequence_num++;
		res = send(sockfd, eng_rsp_diag, rsp_len + 1, 0);
	}

	return 0;
}

static cmr_s32 handle_img_data_asic(cmr_u32 format, cmr_u32 width, cmr_u32 height, char *ch0_ptr, cmr_s32 ch0_len, char *ch1_ptr, cmr_s32 ch1_len, char *ch2_ptr, cmr_s32 ch2_len)
{
	UNUSED(format);
	UNUSED(width);
	UNUSED(height);
	char name[100] = { '\0' };
	FILE *fp = NULL;
	switch (capture_format) {
	case 8:
		sprintf(name, "/data/internal-asec/the_pic_%u.mipi_raw", g_af_pos);
		break;
	case 16:
		sprintf(name, "/data/internal-asec/the_pic_%u.jpg", g_af_pos);
		break;
	}
	fp = fopen(name, "wb");

	if (fp == NULL) {
		ISP_LOGV("fail to create file");
		return 0;
	}

	fwrite(ch0_ptr, 1, ch0_len, fp);

	fwrite(ch1_ptr, 1, ch1_len, fp);

	fwrite(ch2_ptr, 1, ch2_len, fp);
	fclose(fp);
	ISP_LOGV("writing one pic succeeds");
	return 0;
}

cmr_s32 isp_sft_read(cmr_handle handler, cmr_u8 * data_buf, cmr_u32 * data_size)
{
	cmr_s32 ret = 0;
	struct isp_parser_buf_rtn rtn_param = { 0x00, 0x00 };

	rtn_param.buf_addr = (cmr_uint) data_buf;

	ret = isp_ioctl(handler, ISP_CTRL_SFT_READ, (void *)&rtn_param);	// get af info after auto focus

	*data_size = rtn_param.buf_len;

	return ret;
}

cmr_s32 isp_sft_write(cmr_handle handler, cmr_u8 * data_buf, cmr_u32 * data_size)
{
	cmr_s32 ret = 0;
	struct isp_parser_buf_rtn rtn_param = { 0x00, 0x00 };
	rtn_param.buf_addr = (cmr_uint) data_buf;
	rtn_param.buf_len = *data_size;
	ret = isp_ioctl(handler, ISP_CTRL_SFT_WRITE, (void *)&rtn_param);	// set af info after auto focus

	return ret;
}

static cmr_s32 isp_tool_calc_nr_addr_offset(cmr_u8 isp_mode, cmr_u8 nr_mode, cmr_u32 * one_multi_mode_ptr, cmr_u32 * offset_units_ptr)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 offset_units = 0;
	cmr_u32 i = 0, j = 0;

	if (isp_mode > (MAX_MODE_NUM - 1))
		return ISP_ERROR;
	if (nr_mode > (MAX_SCENEMODE_NUM - 1))
		return ISP_ERROR;
	if (!one_multi_mode_ptr)
		return ISP_ERROR;

	for (i = 0; i < isp_mode; i++) {
		if (one_multi_mode_ptr[i]) {
			for (j = 0; j < MAX_SCENEMODE_NUM; j++) {
				offset_units += (one_multi_mode_ptr[i] >> j) & 0x01;
			}
		}
	}

	for (j = 0; j < nr_mode; j++) {
		offset_units += (one_multi_mode_ptr[isp_mode] >> j) & 0x01;
	}
	*offset_units_ptr = offset_units;
	return rtn;
}


unsigned long nr_blk_level_size[ISP_BLK_TYPE_MAX] = {
	/*struct sensor_pwd_level *pwd_level_ptr;*/
	/*for some reason, pwd blok not removed in ISP_BLK_TYPE enum*/
	/*SO add zero here to adapt to it*/
	0,/*sizeof(struct sensor_pwd_level),*/
	sizeof(struct sensor_bpc_level),
	sizeof(struct sensor_bdn_level),
	/*struct sensor_grgb_level *grgb_v1_level_ptr; sharkls */
	sizeof(struct sensor_grgb_level),
	sizeof(struct sensor_nlm_level),
	sizeof(struct sensor_vst_level),
	sizeof(struct sensor_ivst_level),
	sizeof(struct sensor_flat_offset_level),
	sizeof(struct sensor_cfae_level),
	sizeof(struct sensor_rgb_precdn_level),
	sizeof(struct sensor_yuv_precdn_level),
	sizeof(struct sensor_prfy_level),
	sizeof(struct sensor_uv_cdn_level),
	sizeof(struct sensor_ee_level),
	sizeof(struct sensor_uv_postcdn_level),
	sizeof(struct sensor_iircnr_level),
	sizeof(struct sensor_iircnr_yrandom_level),
	sizeof(struct sensor_cce_uvdiv_level),
	sizeof(struct sensor_rgb_afm_level)
};

unsigned int get_nr_block_id_by_sub_type(cmr_u16 sub_type)
{
	ISP_LOGE("sub_type is %u", sub_type);
	switch (sub_type) {
	case BDN:
		return ISP_BLK_BDN_T;
	case PIKE2BPC:
		return ISP_BLK_BPC_T;
	case UVDIV:
		return ISP_BLK_UVDIV_T;
	case CFA:
		return ISP_BLK_CFA_T;
	case EDGE:
		return ISP_BLK_EDGE_T;
	case PIKE2FLATOFFSET:
		return ISP_BLK_NLM_FLAT_OFFSET_T;
	case GRGB:
		return ISP_BLK_GRGB_T;
	case IIRCNR_YRANDOM:
		return ISP_BLK_IIRCNR_YRANDOM_T;
	case IIRCNR_IIR:
		return ISP_BLK_IIRCNR_IIR_T;
	case PIKE2IVST:
		return ISP_BLK_NLM_IVST_T;
	case PIKE2NLM:
		return ISP_BLK_NLM_T;
	case PREF:
		return ISP_BLK_PREF_T;
	case PWD:
		return ISP_BLK_PRE_WAVELET_T;
	case PIKE2RGBPRECDN:
		return ISP_BLK_RGB_PRECDN_T;
	case PIKE2VST:
		return ISP_BLK_NLM_VST_T;
	case PIKE2RGBAFM:
		return ISP_BLK_RGB_AFM_T;
	case UV_CDN:
		return ISP_BLK_UV_CDN_T;
	case UV_POSTCDN:
		return ISP_BLK_UV_POSTCDN_T;
	case YUV_PRECDN:
		return ISP_BLK_YUV_PRECDN_T;
	default:
		return ISP_BLK_TYPE_MAX;
	}
	return ISP_BLK_TYPE_MAX;
};

unsigned long get_nr_tool_flag_index(unsigned long nr_blk_id)
{
#define NR_TOOL_FLAG_MAX 17
	switch (nr_blk_id) {
	case BDN:
		return 2;
	case BPC:
		return 2;
	case UVDIV:
		return 12;
	case CFA:
		return 3;
	case EDGE:
		return 4;
	case FLAT_OFFSET:
		return 7;
	case GRGB:
		return 5;
	case IIRCNR_YRANDOM:
		return 6;
	case IIRCNR_IIR:
		return 6;
	case IVST:
		return 7;
	case NLM:
		return 7;
	case PREF:
		return 10;
	case RGB_PRECDN:
		return 16;
	case VST:
		return 7;
	case YIQ_AFM:
		return 9;
	case UV_CDN:
		return 11;
	case UV_POSTCDN:
		return 13;
	case YUV_PRECDN:
		return 16;
	default:
		return NR_TOOL_FLAG_MAX;
	}
	return NR_TOOL_FLAG_MAX;
};

void *get_nr_block_level_ptr(struct denoise_param_update *base,
						unsigned int nr_block_id)
{
	switch (nr_block_id) {
	case ISP_BLK_BPC_T:
		return base->bpc_level_ptr;
	case ISP_BLK_BDN_T:
		return base->bdn_level_ptr;
	case ISP_BLK_GRGB_T:
		return base->grgb_level_ptr;
	case ISP_BLK_NLM_T:
		return base->nlm_level_ptr;
	case ISP_BLK_NLM_VST_T:
		return base->vst_level_ptr;
	case ISP_BLK_NLM_IVST_T:
		return base->ivst_level_ptr;
	case ISP_BLK_NLM_FLAT_OFFSET_T:
		return base->flat_offset_level_ptr;
	case ISP_BLK_CFA_T:
		return base->cfae_level_ptr;
	case ISP_BLK_RGB_PRECDN_T:
		return base->rgb_precdn_level_ptr;
	case ISP_BLK_YUV_PRECDN_T:
		return base->yuv_precdn_level_ptr;
	case ISP_BLK_PREF_T:
		return base->prfy_level_ptr;
	case ISP_BLK_UV_CDN_T:
		return base->uv_cdn_level_ptr;
	case ISP_BLK_EDGE_T:
		return base->ee_level_ptr;
	case ISP_BLK_UV_POSTCDN_T:
		return base->uv_postcdn_level_ptr;
	case ISP_BLK_IIRCNR_IIR_T:
		return base->iircnr_level_ptr;
	case ISP_BLK_IIRCNR_YRANDOM_T:
		return base->iircnr_yrandom_level_ptr;
	case ISP_BLK_UVDIV_T:
		return base->cce_uvdiv_level_ptr;
	case ISP_BLK_RGB_AFM_T:
		return base->rgb_afm_level_ptr;
	}
	return NULL;
};

cmr_s32 isp_denoise_write(cmr_u8 * data_buf, cmr_u32 * data_size)
{
	cmr_s32 ret = ISP_SUCCESS;
	cmr_u8 *data_actual_ptr;
	cmr_u32 data_actual_len;
	cmr_u8 isp_mode = 0;
	cmr_u8 nr_mode = 0;
	cmr_u32 offset_units = 0;
	cmr_u32 nr_offset_addr = 0;
	cmr_u8 *nr_blk_level_cur_addr = NULL;
	struct sensor_nr_scene_map_param *multi_nr_scene_map_ptr = PNULL;
	struct sensor_nr_level_map_param *multi_nr_level_map_ptr = PNULL;
	struct sensor_nr_level_map_param *multi_nr_default_level_map_ptr = PNULL;

	if ((NULL == data_buf) || (NULL == data_size)) {
		ISP_LOGE("fail to check param");
		return ISP_PARAM_NULL;
	}
	struct isp_data_header_normal *data_head = (struct isp_data_header_normal *)data_buf;

	data_actual_ptr = data_buf + sizeof(struct isp_data_header_normal);
	data_actual_len = *data_size - sizeof(struct isp_data_header_normal);

	if (nr_update_param.multi_nr_flag != SENSOR_MULTI_MODE_FLAG) {
		isp_mode = 0;
		nr_mode = 0;
	} else {
		isp_mode = data_head->isp_mode;
		nr_mode = data_head->nr_mode;
		multi_nr_scene_map_ptr = (struct sensor_nr_scene_map_param *)nr_update_param.nr_scene_map_ptr;
		multi_nr_level_map_ptr = (struct sensor_nr_level_map_param *)nr_update_param.nr_level_number_map_ptr;
		multi_nr_default_level_map_ptr = (struct sensor_nr_level_map_param *)nr_update_param.nr_level_number_map_ptr;
	}

	static cmr_u32 nr_blk_ptr_offset[ISP_BLK_TYPE_MAX];
	unsigned long nr_blk_id = get_nr_block_id_by_sub_type(data_head->sub_type);

	if (nr_blk_id == ISP_BLK_TYPE_MAX)
		return ret;
	isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (cmr_u32 *)&multi_nr_scene_map_ptr->nr_scene_map[0], &offset_units);
	nr_offset_addr = offset_units * nr_blk_level_size[nr_blk_id] * multi_nr_level_map_ptr->nr_level_map[nr_blk_id];
	nr_blk_level_cur_addr = (cmr_u8 *)(get_nr_block_level_ptr(&nr_update_param, nr_blk_id))
		+ nr_offset_addr + nr_blk_ptr_offset[nr_blk_id];
	memcpy(nr_blk_level_cur_addr, (cmr_u8 *)data_actual_ptr, data_actual_len);
	if (0x01 != data_head->packet_status)
		nr_blk_ptr_offset[nr_blk_id] += data_actual_len;
	else
		nr_blk_ptr_offset[nr_blk_id] = 0;
	nr_tool_flag[get_nr_tool_flag_index(nr_blk_id)] = 1;
	return ret;
}

cmr_s32 denoise_param_send(cmr_u8 * tx_buf, cmr_u32 valid_len, void *src_ptr, cmr_u32 src_size, cmr_u8 * data_ptr,  cmr_u8 * data_status)
{
	cmr_s32 ret = ISP_SUCCESS;
	cmr_s32 i, num;
	cmr_u32 tail_len;

	MSG_HEAD_T *msg_ret;

	if (NULL == tx_buf || NULL == src_ptr || NULL == data_ptr) {
		ISP_LOGE("fail to check param:tx_buf:%p, src_ptr:%p, data_ptr:%p", tx_buf, src_ptr, data_ptr);
		return ISP_PARAM_ERROR;
	}
	msg_ret = (MSG_HEAD_T *) (tx_buf + 1) ;

	num = src_size / valid_len;
	tail_len = src_size % valid_len;

	if (0 != num) {
		for (i = 0; i < num; i++) {
			memcpy(data_ptr, (cmr_u8 *) src_ptr + i * valid_len, valid_len);
			msg_ret->len = sizeof(MSG_HEAD_T) + sizeof(struct isp_data_header_normal) + valid_len;
			*(tx_buf + msg_ret->len + 1) = 0x7e;
			if ((i == (num - 1)) && (0 == tail_len))
				*data_status = 0x01;
			else
				*data_status = 0x00;

			ret = send(sockfd, tx_buf, msg_ret->len + 2, 0);
		}
	}
	if (0 != tail_len) {
		memcpy(data_ptr, (cmr_u8 *) src_ptr + num * valid_len, tail_len);
		msg_ret->len = sizeof(MSG_HEAD_T) + sizeof(struct isp_data_header_normal) + tail_len;
		*(tx_buf + msg_ret->len + 1) = 0x7e;
		*data_status = 0x01;
		ret = send(sockfd, tx_buf, msg_ret->len + 2, 0);
	}
	ISP_LOGV("denoise send over! ret = %d", ret);
	return ret;
}

cmr_s32 isp_denoise_read(cmr_u8 * tx_buf, cmr_u32 len, struct isp_data_header_read * data_head)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct isp_data_header_normal *data_head_ptr;
	cmr_u8 *data_ptr;
	cmr_u32 data_valid_len;
	cmr_u32 src_size = 0;
	cmr_s32 file_num = 0;
	cmr_u8 isp_mode = 0;
	cmr_u8 nr_mode = 0;
	cmr_u8 *nr_scene_and_level_map = NULL;
	cmr_u32 *temp_nr_map_addr = NULL;

	if (NULL == tx_buf) {
		ISP_LOGE("fail to check tx_buf:%p", tx_buf);
		return ISP_PARAM_ERROR;
	}

	data_head_ptr = (struct isp_data_header_normal *)(tx_buf + sizeof(MSG_HEAD_T) + 1);
	data_head_ptr->main_type = 0x01;	//denoise param
	data_ptr = ((cmr_u8 *) data_head_ptr) + sizeof(struct isp_data_header_normal);
	data_valid_len = len - 2 - sizeof(MSG_HEAD_T) - sizeof(struct isp_data_header_normal);
	file_num = data_head->sub_type;
	cmr_u32 offset_units = 0;
	void *nr_offset_addr = PNULL;
	struct sensor_nr_scene_map_param *multi_nr_scene_map_ptr = PNULL;
	struct sensor_nr_level_map_param *multi_nr_level_map_ptr = PNULL;
	struct sensor_nr_level_map_param *multi_nr_default_level_map_ptr = PNULL;

	isp_mode = data_head->isp_mode;
	nr_mode = data_head->nr_mode;
	if (nr_update_param.nr_scene_map_ptr != PNULL) {
		multi_nr_scene_map_ptr = (struct sensor_nr_scene_map_param *)nr_update_param.nr_scene_map_ptr;
	} else {
		return ISP_PARAM_ERROR;
	}
	if (nr_update_param.nr_level_number_map_ptr != PNULL) {
		multi_nr_level_map_ptr = (struct sensor_nr_level_map_param *)nr_update_param.nr_level_number_map_ptr;
	} else {
		return ISP_PARAM_ERROR;
	}
	if (nr_update_param.nr_default_level_map_ptr != PNULL) {
		multi_nr_default_level_map_ptr = (struct sensor_nr_level_map_param *)nr_update_param.nr_default_level_map_ptr;
	} else {
		return ISP_PARAM_ERROR;
	}

	data_head_ptr->sub_type = data_head->sub_type;
	if (data_head->sub_type == SCENEINFO) {
		data_head_ptr->sub_type = SCENEINFO;
		src_size = sizeof(struct sensor_nr_scene_map_param)
		    + sizeof(struct sensor_nr_level_map_param)
		    + sizeof(struct sensor_nr_level_map_param);

		if ((multi_nr_scene_map_ptr != NULL) && (multi_nr_level_map_ptr != NULL) && (multi_nr_default_level_map_ptr != NULL)) {
			nr_scene_and_level_map = (cmr_u8 *) ispParserAlloc(src_size);
			if (!nr_scene_and_level_map) {
				ISP_LOGE("fail to do ispParserAlloc!");
				return -1;
			}
			memcpy(nr_scene_and_level_map, multi_nr_scene_map_ptr, sizeof(struct sensor_nr_scene_map_param));
			temp_nr_map_addr = (cmr_u32 *) ((cmr_u8 *) nr_scene_and_level_map + sizeof(struct sensor_nr_scene_map_param));
			memcpy(temp_nr_map_addr, multi_nr_level_map_ptr, sizeof(struct sensor_nr_level_map_param));
			temp_nr_map_addr = (cmr_u32 *) ((cmr_u8 *) temp_nr_map_addr + sizeof(struct sensor_nr_level_map_param));
			memcpy(temp_nr_map_addr, multi_nr_default_level_map_ptr, sizeof(struct sensor_nr_level_map_param));
			nr_offset_addr = (cmr_u32 *) nr_scene_and_level_map;

		} else {
			ISP_LOGE("fail to check param");
		}
	} else {
			cmr_u32 nr_blk_id = get_nr_block_id_by_sub_type(data_head->sub_type);
			data_head_ptr->sub_type = data_head->sub_type;
			src_size = nr_blk_level_size[nr_blk_id] * multi_nr_level_map_ptr->nr_level_map[nr_blk_id];
			isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (cmr_u32 *) & multi_nr_scene_map_ptr->nr_scene_map[0], &offset_units);
			nr_offset_addr = (cmr_u8 *) get_nr_block_level_ptr(&nr_update_param, nr_blk_id) + offset_units * src_size;
	};

	ISP_LOGV("nr_offset_addr = %p, size_src = %d", nr_offset_addr, src_size);
	denoise_param_send(tx_buf, data_valid_len, (void *)nr_offset_addr, src_size, data_ptr, &data_head_ptr->packet_status);

	if (nr_scene_and_level_map) {
		ispParserFree(nr_scene_and_level_map);
	}
	return ret;
}

cmr_s32 isp_dump_reg(cmr_handle handler, cmr_u8 * tx_buf, cmr_u32 len)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct isp_info param_info;
	MSG_HEAD_T *msg_ret;
	struct isp_data_header_normal *data_head_ptr;
	cmr_u8 *data_ptr;
	cmr_u32 data_valid_len;
	cmr_u32 src_size;
	void *src_ptr;
	cmr_u32 num, i;
	cmr_u32 tail_len;

	if (NULL == tx_buf || NULL == handler) {
		ISP_LOGE("fail to check param");
		return ISP_PARAM_ERROR;
	}

	msg_ret = (MSG_HEAD_T *) (tx_buf + 1);
	data_head_ptr = (struct isp_data_header_normal *)(tx_buf + sizeof(MSG_HEAD_T) + 1);
	data_head_ptr->main_type = 0x00;	//dump isp reg
	data_ptr = ((cmr_u8 *) data_head_ptr) + sizeof(struct isp_data_header_normal);
	data_valid_len = len - 2 - sizeof(MSG_HEAD_T) - sizeof(struct isp_data_header_normal);

	ret = isp_ioctl(handler, ISP_CTRL_DUMP_REG, (void *)&param_info);

	src_ptr = param_info.addr;
	src_size = param_info.size;

	num = src_size / data_valid_len;
	tail_len = src_size % data_valid_len;

	for (i = 0; i < num; i++) {
		memcpy(data_ptr, (cmr_u8 *) src_ptr + i * data_valid_len, data_valid_len);
		msg_ret->len = sizeof(MSG_HEAD_T) + sizeof(struct isp_data_header_normal) + data_valid_len;
		*(tx_buf + (msg_ret->len) + 1) = 0x7e;
		if (0 == tail_len && i == (num - 1))
			data_head_ptr->packet_status = 0x01;	//last one
		else
			data_head_ptr->packet_status = 0x00;

		ret = send(sockfd, tx_buf, (msg_ret->len) + 2, 0);
	}
	if (0 != tail_len) {
		memcpy(data_ptr, (cmr_u8 *) src_ptr + num * data_valid_len, tail_len);
		msg_ret->len = sizeof(MSG_HEAD_T) + sizeof(struct isp_data_header_normal) + tail_len;
		*(tx_buf + (msg_ret->len) + 1) = 0x7e;
		data_head_ptr->packet_status = 0x01;	//last one
		ret = send(sockfd, tx_buf, (msg_ret->len) + 2, 0);
	}
	if (NULL != src_ptr) {
		free(src_ptr);
		src_ptr = NULL;
	}
	ISP_LOGV("dump over! ret=%d", ret);
	return ret;
}

static cmr_s32 isp_nr_reg_read(cmr_handle handler, cmr_u8 * tx_buf, cmr_u32 len, cmr_u8 * rx_buf)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct isp_data_header_read *data_head = (struct isp_data_header_read *)(rx_buf + sizeof(MSG_HEAD_T) + 1);
	if (0x01 == data_head->main_type) {
		ret = isp_denoise_read(tx_buf, len, data_head);
		ISP_LOGV("denoise_read over");
		return ret;
	}
	if (0x00 == data_head->main_type) {
		ret = isp_dump_reg(handler, tx_buf, len);
		ISP_LOGV("isp_dump_reg  over");
		return ret;
	}
	return ret;
}

static cmr_s32 isp_nr_write(cmr_handle handler, cmr_u8 * tx_buf, cmr_u8 * rx_buf)
{
	cmr_s32 ret = ISP_SUCCESS;
	MSG_HEAD_T *msg, *msg_ret;
	cmr_s32 rsp_len = 0;
	UNUSED(handler);

	msg = (MSG_HEAD_T *) (rx_buf + 1);
	msg_ret = (MSG_HEAD_T *) (tx_buf + 1);
	cmr_u32 data_len = msg->len - 8;
	struct isp_data_header_normal *data_head = (struct isp_data_header_normal *)(rx_buf + sizeof(MSG_HEAD_T) + 1);
	rsp_len = sizeof(MSG_HEAD_T) + 1;

	if (0x01 == data_head->main_type) {
		ret = isp_denoise_write((cmr_u8 *) data_head, &data_len);
		if (0x01 == data_head->packet_status) {
			if (ISP_SUCCESS == ret) {
				rsp_len += ispvideo_SetreTurnValue(tx_buf + rsp_len, ISP_SUCCESS);
				*(tx_buf + rsp_len) = 0x7e;
				msg_ret->len = rsp_len - 1;
				ret = send(sockfd, tx_buf, rsp_len + 1, 0);
			} else {
				rsp_len += ispvideo_SetreTurnValue(tx_buf + rsp_len, 0x01);
				*(tx_buf + rsp_len) = 0x7e;
				msg_ret->len = rsp_len - 1;
				ret = send(sockfd, tx_buf, rsp_len + 1, 0);
			}
		}
	}
	ISP_LOGV("End to write NR! ret = %d", ret);
	return ret;
}

cmr_u32 get_tune_packet_num(cmr_u32 packet_len)
{
	cmr_u32 packet_num = 0;
	cmr_u32 len_tmp = 0;
	len_tmp = 0;
	if (0 != ((packet_len - len_tmp) % (DATA_BUF_SIZE - PACKET_MSG_HEADER - DATA_CMD_LENGTH - len_tmp - 2))) {
		packet_num = (packet_len - len_tmp) / (DATA_BUF_SIZE - PACKET_MSG_HEADER - DATA_CMD_LENGTH - len_tmp - 2) + 1;
	} else {
		packet_num = (packet_len - len_tmp) / (DATA_BUF_SIZE - PACKET_MSG_HEADER - DATA_CMD_LENGTH - len_tmp - 2);
	}
	return packet_num;
}

cmr_u32 get_fix_packet_num(cmr_u32 packet_len)
{
	cmr_u32 packet_num = 0;
	cmr_u32 len_tmp = 0;
	if (0 != ((packet_len - len_tmp) % (DATA_BUF_SIZE - PACKET_MSG_HEADER - DATA_CMD_LENGTH - len_tmp - 2))) {
		packet_num = (packet_len - len_tmp) / (DATA_BUF_SIZE - PACKET_MSG_HEADER - DATA_CMD_LENGTH - len_tmp - 2) + 1;
	} else {
		packet_num = (packet_len - len_tmp) / (DATA_BUF_SIZE - PACKET_MSG_HEADER - DATA_CMD_LENGTH - len_tmp - 2);
	}
	return packet_num;
}

cmr_u32 get_note_packet_num(cmr_u32 packet_len)
{
	cmr_u32 packet_num = 0;
	if (0 != (packet_len % (DATA_BUF_SIZE - PACKET_MSG_HEADER - DATA_CMD_LENGTH - 2))) {
		packet_num = packet_len / (DATA_BUF_SIZE - PACKET_MSG_HEADER - DATA_CMD_LENGTH - 2) + 1;
	} else {
		packet_num = packet_len / (DATA_BUF_SIZE - PACKET_MSG_HEADER - DATA_CMD_LENGTH - 2);
	}
	return packet_num;
}

cmr_u32 get_libuse_info_packet_num(cmr_u32 packet_len)
{
	cmr_u32 packet_num = 0;
	if (0 != (packet_len % (DATA_BUF_SIZE - PACKET_MSG_HEADER - DATA_CMD_LENGTH - 2))) {
		packet_num = packet_len / (DATA_BUF_SIZE - PACKET_MSG_HEADER - DATA_CMD_LENGTH - 2) + 1;
	} else {
		packet_num = packet_len / (DATA_BUF_SIZE - PACKET_MSG_HEADER - DATA_CMD_LENGTH - 2);
	}
	return packet_num;
}

cmr_s32 send_isp_mode_id_param(struct isp_data_header_read * read_cmd, struct msg_head_tag * msg, cmr_u32 * data_addr, cmr_u32 data_len)
{
	cmr_s32 rtn = 0x00;
	cmr_s32 res;
	cmr_u32 len = 0;
	cmr_u32 rsp_len = 0;

	cmr_u32 len_msg = sizeof(struct msg_head_tag);
	cmr_u32 len_data_header = sizeof(struct isp_data_header_normal);

	struct isp_data_header_normal data_header;
	struct msg_head_tag msg_tag;
	memset(&data_header, 0, sizeof(struct isp_data_header_normal));
	memset(&msg_tag, 0, sizeof(struct msg_head_tag));

	data_header.isp_mode = read_cmd->isp_mode;
	data_header.main_type = read_cmd->main_type;
	data_header.sub_type = read_cmd->sub_type;
	data_header.data_total_len = data_len;
	msg_tag.seq_num = msg->seq_num;
	msg_tag.subtype = msg->subtype;
	msg_tag.type = msg->type;

	len = data_len + len_msg + len_data_header;
	msg_tag.len = len;
	data_header.packet_status = 0x01;

	memcpy(eng_rsp_diag + 1, (cmr_u8 *) & msg_tag, len_msg);
	rsp_len = len_msg + 1;
	memcpy(eng_rsp_diag + rsp_len, (cmr_u8 *) & data_header, len_data_header);
	rsp_len = rsp_len + len_data_header;

	memcpy(eng_rsp_diag + rsp_len, (cmr_u8 *) data_addr, data_len);
	rsp_len = len + 1;
	eng_rsp_diag[rsp_len] = 0x7e;
	res = send(sockfd, eng_rsp_diag, rsp_len + 1, 0);
	return rtn;
}

cmr_s32 send_tune_info_param(struct isp_data_header_read * read_cmd, struct msg_head_tag * msg, cmr_u32 * data_addr, cmr_u32 data_len)
{
	cmr_s32 rtn = 0x00;

	cmr_u32 packet_num = 0;
	cmr_u32 i;
	cmr_s32 res;
	cmr_u32 len = 0;

	cmr_u32 len_msg = sizeof(struct msg_head_tag);
	cmr_u32 len_data_header = sizeof(struct isp_data_header_normal);
	cmr_u8 *data_ptr = NULL;
	struct isp_data_header_normal data_header;
	struct msg_head_tag msg_tag;
	memset(&data_header, 0, sizeof(struct isp_data_header_normal));
	memset(&msg_tag, 0, sizeof(struct msg_head_tag));

	data_header.isp_mode = read_cmd->isp_mode;
	data_header.main_type = read_cmd->main_type;
	data_header.sub_type = read_cmd->sub_type;
	data_header.data_total_len = data_len;
	msg_tag.seq_num = msg->seq_num;
	msg_tag.subtype = msg->subtype;
	msg_tag.type = msg->type;

	packet_num = get_tune_packet_num(data_len);
	data_ptr = (cmr_u8 *) data_addr;

	for (i = 0; i < packet_num; i++) {
		cmr_u32 rsp_len = 0;
		if (i < (packet_num - 1)) {
			len = DATA_BUF_SIZE - 2;
			msg_tag.len = len;
			data_header.packet_status = 0x00;
		} else {
			len = data_len + packet_num * (len_msg + len_data_header + 2) - DATA_BUF_SIZE * i - 2;
			msg_tag.len = len;
			data_header.packet_status = 0x01;
		}
		memcpy(eng_rsp_diag + 1, (cmr_u8 *) & msg_tag, len_msg);
		rsp_len = len_msg + 1;
		memcpy(eng_rsp_diag + rsp_len, (cmr_u8 *) & data_header, len_data_header);
		rsp_len = rsp_len + len_data_header;

		memcpy(eng_rsp_diag + rsp_len, (cmr_u8 *) data_ptr, len - len_msg - len_data_header);
		data_ptr = (cmr_u8 *) data_ptr + len - len_msg - len_data_header;

		rsp_len = len + 1;
		eng_rsp_diag[rsp_len] = 0x7e;
		res = send(sockfd, eng_rsp_diag, rsp_len + 1, 0);
	}
	return rtn;
}

cmr_s32 send_fix_param(struct isp_data_header_read * read_cmd, struct msg_head_tag * msg, cmr_u32 * data_addr, cmr_u32 data_len)
{
	cmr_s32 rtn = 0x00;

	cmr_u32 packet_num = 0;
	cmr_u32 i;
	cmr_s32 res;
	cmr_u32 len = 0;
	cmr_u32 rsp_len = 0;
	cmr_u8 *data_ptr = NULL;

	cmr_u32 len_msg = sizeof(struct msg_head_tag);
	cmr_u32 len_data_header = sizeof(struct isp_data_header_normal);
	struct isp_data_header_normal data_header;
	struct msg_head_tag msg_tag;
	memset(&data_header, 0, sizeof(struct isp_data_header_normal));
	memset(&msg_tag, 0, sizeof(struct msg_head_tag));

	data_header.isp_mode = read_cmd->isp_mode;
	data_header.main_type = read_cmd->main_type;
	data_header.sub_type = read_cmd->sub_type;
	data_header.data_total_len = data_len;
	msg_tag.seq_num = msg->seq_num;
	msg_tag.subtype = msg->subtype;
	msg_tag.type = msg->type;

	packet_num = get_fix_packet_num(data_len);
	data_ptr = (cmr_u8 *) data_addr;

	for (i = 0; i < packet_num; i++) {
		if (i < (packet_num - 1)) {
			len = DATA_BUF_SIZE - 2;
			msg_tag.len = len;
			data_header.packet_status = 0x00;
		} else {
			if (0 != i)
				len = data_len + packet_num * (len_msg + len_data_header + 2) - DATA_BUF_SIZE * i - 2;
			else
				len = data_len + len_msg + len_data_header;
			msg_tag.len = len;
			data_header.packet_status = 0x01;
		}
		memcpy(eng_rsp_diag + 1, (cmr_u8 *) & msg_tag, len_msg);
		rsp_len = len_msg + 1;
		memcpy(eng_rsp_diag + rsp_len, (cmr_u8 *) & data_header, len_data_header);
		rsp_len = rsp_len + len_data_header;

		memcpy(eng_rsp_diag + rsp_len, (cmr_u8 *) data_ptr, len - len_msg - len_data_header);
		data_ptr = (cmr_u8 *) data_ptr + len - len_msg - len_data_header;

		rsp_len = len + 1;
		eng_rsp_diag[rsp_len] = 0x7e;
		res = send(sockfd, eng_rsp_diag, rsp_len + 1, 0);
	}
	return rtn;
}

cmr_s32 send_note_param(struct isp_data_header_read * read_cmd, struct msg_head_tag * msg, cmr_u32 * data_addr, cmr_u32 data_len)
{
	cmr_s32 rtn = 0x00;

	cmr_u32 packet_num = 0;
	cmr_u32 i;
	cmr_s32 res;
	cmr_u32 len = 0;

	cmr_u32 len_msg = sizeof(struct msg_head_tag);
	cmr_u32 len_data_header = sizeof(struct isp_data_header_normal);

	cmr_u8 *data_ptr = (cmr_u8 *) data_addr;
	struct isp_data_header_normal data_header;
	struct msg_head_tag msg_tag;
	memset(&data_header, 0, sizeof(struct isp_data_header_normal));
	memset(&msg_tag, 0, sizeof(struct msg_head_tag));

	data_header.isp_mode = read_cmd->isp_mode;
	data_header.main_type = read_cmd->main_type;
	data_header.sub_type = read_cmd->sub_type;
	msg_tag.seq_num = msg->seq_num;
	msg_tag.subtype = msg->subtype;
	msg_tag.type = msg->type;

	packet_num = get_note_packet_num(data_len);
	for (i = 0; i < packet_num; i++) {
		cmr_u32 rsp_len = 0;
		if (i < (packet_num - 1)) {
			len = DATA_BUF_SIZE - 2;
			msg_tag.len = len;
			data_header.packet_status = 0x00;
		} else {
			len = data_len + packet_num * (len_msg + len_data_header + 2) - DATA_BUF_SIZE * i - 2;
			msg_tag.len = len;
			data_header.packet_status = 0x01;
		}
		memcpy(eng_rsp_diag + 1, (cmr_u8 *) & msg_tag, len_msg);
		rsp_len = len_msg + 1;
		memcpy(eng_rsp_diag + rsp_len, (cmr_u8 *) & data_header, len_data_header);
		rsp_len = rsp_len + len_data_header;
		memcpy(eng_rsp_diag + rsp_len, (cmr_u8 *) data_ptr, len - len_msg - len_data_header);
		data_ptr = (cmr_u8 *) data_ptr + len - len_msg - len_data_header;
		rsp_len = len + 1;
		eng_rsp_diag[rsp_len] = 0x7e;
		res = send(sockfd, eng_rsp_diag, rsp_len + 1, 0);
	}
	return rtn;
}

cmr_s32 send_libuse_info_param(struct isp_data_header_read * read_cmd, struct msg_head_tag * msg, cmr_u32 * data_addr, cmr_u32 data_len)
{
	cmr_s32 rtn = 0x0;

	cmr_u32 packet_num = 0;
	cmr_u32 i;
	cmr_s32 res;
	cmr_u32 len = 0;

	cmr_u32 len_msg = sizeof(struct msg_head_tag);
	cmr_u32 len_data_header = sizeof(struct isp_data_header_normal);

	cmr_u8 *data_ptr = (cmr_u8 *) data_addr;
	struct isp_data_header_normal data_header;
	struct msg_head_tag msg_tag;
	memset(&data_header, 0, sizeof(struct isp_data_header_normal));
	memset(&msg_tag, 0, sizeof(struct msg_head_tag));

	data_header.isp_mode = read_cmd->isp_mode;
	data_header.main_type = read_cmd->main_type;
	data_header.sub_type = read_cmd->sub_type;
	msg_tag.seq_num = msg->seq_num;
	msg_tag.subtype = msg->subtype;
	msg_tag.type = msg->type;

	packet_num = get_libuse_info_packet_num(data_len);
	for (i = 0; i < packet_num; i++) {
		cmr_u32 rsp_len = 0;
		if (i < (packet_num - 1)) {
			len = DATA_BUF_SIZE - 2;
			msg_tag.len = len;
			data_header.packet_status = 0x00;
		} else {
			len = data_len + packet_num * (len_msg + len_data_header + 2) - DATA_BUF_SIZE * i - 2;
			msg_tag.len = len;
			data_header.packet_status = 0x01;
		}
		memcpy(eng_rsp_diag + 1, (cmr_u8 *) & msg_tag, len_msg);
		rsp_len = len_msg + 1;
		memcpy(eng_rsp_diag + rsp_len, (cmr_u8 *) & data_header, len_data_header);
		rsp_len = rsp_len + len_data_header;
		memcpy(eng_rsp_diag + rsp_len, (cmr_u8 *) data_ptr, len - len_msg - len_data_header);
		data_ptr = (cmr_u8 *) data_ptr + len - len_msg - len_data_header;
		rsp_len = len + 1;
		eng_rsp_diag[rsp_len] = 0x7e;
		res = send(sockfd, eng_rsp_diag, rsp_len + 1, 0);
	}
	return rtn;
}

cmr_s32 get_ae_table_param_length(struct sensor_raw_fix_info * sensor_raw_fix, cmr_u16 sub_type, cmr_u32 * data_len)
{
	cmr_s32 rtn = 0x00;

	cmr_u8 flicker = (sub_type >> 4) & 0x0f;
	cmr_u8 iso = sub_type & 0x0f;

	if (NULL != data_len) {
		*data_len = *data_len + sensor_raw_fix->ae.ae_tab[flicker][iso].index_len;
		*data_len = *data_len + sensor_raw_fix->ae.ae_tab[flicker][iso].exposure_len;
		*data_len = *data_len + sensor_raw_fix->ae.ae_tab[flicker][iso].dummy_len;
		*data_len = *data_len + sensor_raw_fix->ae.ae_tab[flicker][iso].again_len;
		*data_len = *data_len + sensor_raw_fix->ae.ae_tab[flicker][iso].dgain_len;
	}
	return rtn;
}

cmr_s32 get_ae_table_param(struct sensor_raw_fix_info * sensor_raw_fix, cmr_u16 sub_type, cmr_u32 * data_addr)
{
	cmr_s32 rtn = 0x00;

	cmr_u32 *tmp_ptr = NULL;
	cmr_u8 flicker = (sub_type >> 4) & 0x0f;
	cmr_u8 iso = sub_type & 0x0f;

	if (NULL != data_addr) {
		memcpy((void *)data_addr, (void *)sensor_raw_fix->ae.ae_tab[flicker][iso].index, sensor_raw_fix->ae.ae_tab[flicker][iso].index_len);
		tmp_ptr = (cmr_u32 *) ((cmr_u8 *) data_addr + sensor_raw_fix->ae.ae_tab[flicker][iso].index_len);
		memcpy((void *)tmp_ptr, (void *)sensor_raw_fix->ae.ae_tab[flicker][iso].exposure, sensor_raw_fix->ae.ae_tab[flicker][iso].exposure_len);
		tmp_ptr = (cmr_u32 *) ((cmr_u8 *) tmp_ptr + sensor_raw_fix->ae.ae_tab[flicker][iso].exposure_len);
		memcpy((void *)tmp_ptr, (void *)sensor_raw_fix->ae.ae_tab[flicker][iso].dummy, sensor_raw_fix->ae.ae_tab[flicker][iso].dummy_len);
		tmp_ptr = (cmr_u32 *) ((cmr_u8 *) tmp_ptr + sensor_raw_fix->ae.ae_tab[flicker][iso].dummy_len);
		memcpy((void *)tmp_ptr, (void *)sensor_raw_fix->ae.ae_tab[flicker][iso].again, sensor_raw_fix->ae.ae_tab[flicker][iso].again_len);
		tmp_ptr = (cmr_u32 *) ((cmr_u8 *) tmp_ptr + sensor_raw_fix->ae.ae_tab[flicker][iso].again_len);
		memcpy((void *)tmp_ptr, (void *)sensor_raw_fix->ae.ae_tab[flicker][iso].dgain, sensor_raw_fix->ae.ae_tab[flicker][iso].dgain_len);
		tmp_ptr = (cmr_u32 *) ((cmr_u8 *) tmp_ptr + sensor_raw_fix->ae.ae_tab[flicker][iso].dgain_len);
	}
	return rtn;
}

cmr_s32 get_ae_weight_param_length(struct sensor_raw_fix_info * sensor_raw_fix, cmr_u16 sub_type, cmr_u32 * data_len)
{
	cmr_s32 rtn = 0x00;
	cmr_u16 weight = sub_type;
	*data_len = *data_len + sensor_raw_fix->ae.weight_tab[weight].len;
	return rtn;
}

cmr_s32 get_ae_weight_param(struct sensor_raw_fix_info * sensor_raw_fix, cmr_u16 sub_type, cmr_u32 * data_addr)
{
	cmr_s32 rtn = 0x00;
	cmr_u16 weight = sub_type;
	if (NULL != data_addr)
		memcpy((void *)data_addr, (void *)sensor_raw_fix->ae.weight_tab[weight].weight_table, sensor_raw_fix->ae.weight_tab[weight].len);
	return rtn;
}

cmr_s32 get_ae_scene_param_length(struct sensor_raw_fix_info * sensor_raw_fix, cmr_u16 sub_type, cmr_u32 * data_len)
{
	cmr_s32 rtn = 0x00;
	cmr_u8 scene = (sub_type >> 4) & 0x0f;
	cmr_u8 flicker = sub_type & 0x0f;

	if (NULL != data_len) {
		*data_len = *data_len + sensor_raw_fix->ae.scene_tab[scene][flicker].scene_info_len;
		*data_len = *data_len + sensor_raw_fix->ae.scene_tab[scene][flicker].index_len;
		*data_len = *data_len + sensor_raw_fix->ae.scene_tab[scene][flicker].exposure_len;
		*data_len = *data_len + sensor_raw_fix->ae.scene_tab[scene][flicker].dummy_len;
		*data_len = *data_len + sensor_raw_fix->ae.scene_tab[scene][flicker].again_len;
		*data_len = *data_len + sensor_raw_fix->ae.scene_tab[scene][flicker].dgain_len;
	}
	return rtn;
}

cmr_s32 get_ae_scene_param(struct sensor_raw_fix_info * sensor_raw_fix, cmr_u16 sub_type, cmr_u32 * data_addr)
{
	cmr_s32 rtn = 0x00;

	cmr_u32 *tmp_ptr = NULL;
	cmr_u8 scene = (sub_type >> 4) & 0x0f;
	cmr_u8 flicker = sub_type & 0x0f;

	if (NULL != data_addr && flicker < 2) {
		memcpy(data_addr, sensor_raw_fix->ae.scene_tab[scene][flicker].scene_info, sensor_raw_fix->ae.scene_tab[scene][flicker].scene_info_len);
		tmp_ptr = (cmr_u32 *) ((cmr_u8 *) data_addr + sensor_raw_fix->ae.scene_tab[scene][flicker].scene_info_len);
		memcpy(tmp_ptr, sensor_raw_fix->ae.scene_tab[scene][flicker].index, sensor_raw_fix->ae.scene_tab[scene][flicker].index_len);
		tmp_ptr = (cmr_u32 *) ((cmr_u8 *) tmp_ptr + sensor_raw_fix->ae.scene_tab[scene][flicker].index_len);
		memcpy(tmp_ptr, sensor_raw_fix->ae.scene_tab[scene][flicker].exposure, sensor_raw_fix->ae.scene_tab[scene][flicker].exposure_len);
		tmp_ptr = (cmr_u32 *) ((cmr_u8 *) tmp_ptr + sensor_raw_fix->ae.scene_tab[scene][flicker].exposure_len);
		memcpy(tmp_ptr, sensor_raw_fix->ae.scene_tab[scene][flicker].dummy, sensor_raw_fix->ae.scene_tab[scene][flicker].dummy_len);
		tmp_ptr = (cmr_u32 *) ((cmr_u8 *) tmp_ptr + sensor_raw_fix->ae.scene_tab[scene][flicker].dummy_len);
		memcpy((void *)tmp_ptr, (void *)sensor_raw_fix->ae.scene_tab[scene][flicker].again, sensor_raw_fix->ae.scene_tab[scene][flicker].again_len);
		tmp_ptr = (cmr_u32 *) ((cmr_u8 *) tmp_ptr + sensor_raw_fix->ae.scene_tab[scene][flicker].again_len);
		memcpy((void *)tmp_ptr, (void *)sensor_raw_fix->ae.scene_tab[scene][flicker].dgain, sensor_raw_fix->ae.scene_tab[scene][flicker].dgain_len);
		tmp_ptr = (cmr_u32 *) ((cmr_u8 *) tmp_ptr + sensor_raw_fix->ae.scene_tab[scene][flicker].dgain_len);
	}
	return rtn;
}

cmr_s32 get_ae_auto_iso_param_length(struct sensor_raw_fix_info * sensor_raw_fix, cmr_u16 sub_type, cmr_u32 * data_len)
{
	cmr_s32 rtn = 0x00;
	cmr_u8 iso = sub_type;
	*data_len = *data_len + sensor_raw_fix->ae.auto_iso_tab[iso].len;
	return rtn;
}

cmr_s32 get_ae_auto_iso_param(struct sensor_raw_fix_info * sensor_raw_fix, cmr_u16 sub_type, cmr_u32 * data_addr)
{
	cmr_s32 rtn = 0x00;

	cmr_u8 iso = sub_type;
	if (NULL != data_addr)
		memcpy((void *)data_addr, (void *)sensor_raw_fix->ae.auto_iso_tab[iso].auto_iso_tab, sensor_raw_fix->ae.auto_iso_tab[iso].len);

	return rtn;
}

cmr_s32 get_lnc_param_length(struct sensor_raw_fix_info * sensor_raw_fix, cmr_u16 sub_type, cmr_u32 * data_len)
{
	cmr_s32 rtn = 0x00;
	cmr_u16 lnc_ct = sub_type;

	*data_len = *data_len + sensor_raw_fix->lnc.map[lnc_ct].map_info_len;
	*data_len = *data_len + sensor_raw_fix->lnc.map[lnc_ct].lnc_len;
	return rtn;
}

cmr_s32 get_lnc_param(struct sensor_raw_fix_info * sensor_raw_fix, cmr_u16 sub_type, cmr_u32 * data_addr)
{
	cmr_s32 rtn = 0x00;
	cmr_u32 *tmp_ptr = NULL;
	cmr_u16 lnc_ct = sub_type;

	if (NULL != data_addr) {
		memcpy((void *)data_addr, (void *)sensor_raw_fix->lnc.map[lnc_ct].map_info, sensor_raw_fix->lnc.map[lnc_ct].map_info_len);
		tmp_ptr = (cmr_u32 *) ((cmr_u8 *) data_addr + sensor_raw_fix->lnc.map[lnc_ct].map_info_len);
		memcpy((void *)tmp_ptr, (void *)sensor_raw_fix->lnc.map[lnc_ct].lnc_addr, sensor_raw_fix->lnc.map[lnc_ct].lnc_len);
	}
	return rtn;
}

cmr_s32 get_awb_param_length(struct sensor_raw_fix_info * sensor_raw_fix, cmr_u16 sub_type, cmr_u32 * data_len)
{
	cmr_s32 rtn = 0x00;
	switch (sub_type) {
	case AWB_WIN_MAP:
		*data_len = *data_len + sensor_raw_fix->awb.awb_map.len;
		break;
	case AWB_WEIGHT:
		*data_len = *data_len + sensor_raw_fix->awb.awb_weight.weight_len + sensor_raw_fix->awb.awb_weight.size_param_len;
		break;
	default:
		break;
	}

	return rtn;
}

cmr_s32 get_awb_param(struct sensor_raw_fix_info * sensor_raw_fix, cmr_u16 sub_type, cmr_u32 * data_addr)
{
	cmr_s32 rtn = 0x00;

	cmr_u32 *tmp_ptr = NULL;
	if (NULL != data_addr) {
		switch (sub_type) {
		case AWB_WIN_MAP:
			{
				memcpy((void *)data_addr, (void *)sensor_raw_fix->awb.awb_map.addr, sensor_raw_fix->awb.awb_map.len);
			}
			break;
		case AWB_WEIGHT:
			{
				memcpy((void *)data_addr, (void *)sensor_raw_fix->awb.awb_weight.addr, sensor_raw_fix->awb.awb_weight.weight_len);
				tmp_ptr = (cmr_u32 *) ((cmr_u8 *) data_addr + sensor_raw_fix->awb.awb_weight.weight_len);
				memcpy((void *)tmp_ptr, (void *)sensor_raw_fix->awb.awb_weight.size, sensor_raw_fix->awb.awb_weight.size_param_len);
			}
			break;
		default:
			break;
		}
	}
	return rtn;
}

cmr_s32 send_isp_param(struct isp_data_header_read * read_cmd, struct msg_head_tag * msg)
{
	cmr_s32 rtn = 0x00;

	cmr_u32 data_len = 0;
	cmr_u32 *data_addr = NULL;
	cmr_u8 mode_id = 0;

	struct sensor_raw_fix_info *sensor_raw_fix = NULL;
	struct isp_mode_param_info mode_param_info;
	struct sensor_raw_note_info sensor_note_param;
	memset(&mode_param_info, 0, sizeof(struct isp_mode_param_info));
	memset(&sensor_note_param, 0, sizeof(struct sensor_raw_note_info));

	SENSOR_EXP_INFO_T_PTR sensor_info_ptr = Sensor_GetInfo();
	struct sensor_raw_info *sensor_raw_info_ptr = (struct sensor_raw_info *)sensor_info_ptr->raw_info_ptr;

	if (0xff == read_cmd->isp_mode) {
		mode_id = 0;
	} else {
		mode_id = read_cmd->isp_mode;
	}
	if (NULL != sensor_raw_info_ptr->fix_ptr[mode_id]) {
		sensor_raw_fix = sensor_raw_info_ptr->fix_ptr[mode_id];
	}
	if (NULL != sensor_raw_info_ptr->mode_ptr[mode_id].addr) {
		mode_param_info = sensor_raw_info_ptr->mode_ptr[mode_id];
	}
	if (NULL != sensor_raw_info_ptr->note_ptr[mode_id].note) {
		sensor_note_param = sensor_raw_info_ptr->note_ptr[mode_id];
	}
	ISP_LOGV("read_cmd->main_type = %d", read_cmd->main_type);

	switch (read_cmd->main_type) {
	case MODE_ISP_ID:
		{
			cmr_u8 i;
			cmr_u8 isp_mode_num = 0;
			cmr_u8 data_mode_id[ISP_READ_MODE_ID_MAX] = { 0 };

			for (i = 0; i < ISP_READ_MODE_ID_MAX; i++) {
				if (NULL != sensor_raw_info_ptr->mode_ptr[i].addr) {
					data_mode_id[isp_mode_num] = i;
					isp_mode_num++;
				}
			}
			data_len = isp_mode_num;
			data_addr = (cmr_u32 *) ispParserAlloc(data_len);
			memset((cmr_u8 *) data_addr, 0x00, data_len);
			if (0 != data_len && NULL != data_addr) {
				memcpy((cmr_u8 *) data_addr, data_mode_id, isp_mode_num);
				rtn = send_isp_mode_id_param(read_cmd, msg, data_addr, data_len);
			}
		}
		break;
	case MODE_TUNE_INFO:
		{
			struct isp_mode_param *mode_param = (struct isp_mode_param *)mode_param_info.addr;
			data_len = mode_param_info.len;
			data_addr = (cmr_u32 *) ispParserAlloc(data_len);
			memset((cmr_u8 *) data_addr, 0x00, data_len);
			if (0 != data_len && NULL != data_addr) {
				data_addr[0] = mode_param->version_id;
				data_addr[1] = mode_id;
				data_addr[2] = mode_param->width;
				data_addr[3] = mode_param->height;
				memcpy((void *)((cmr_u8 *) data_addr), (void *)mode_param_info.addr, mode_param_info.len);
				rtn = send_tune_info_param(read_cmd, msg, data_addr, data_len);
				if (0x00 != rtn) {
					ISP_LOGE("fail to send tune_info");
				}
			}
		}
		break;
	case MODE_AE_TABLE:
		{
			if (NULL == sensor_raw_fix) {
				return rtn;
			}
			data_len = 0;
			rtn = get_ae_table_param_length(sensor_raw_fix, read_cmd->sub_type, &data_len);
			data_addr = (cmr_u32 *) ispParserAlloc(data_len);
			memset((cmr_u8 *) data_addr, 0x00, data_len);
			if (0 != data_len && NULL != data_addr) {
				rtn = get_ae_table_param(sensor_raw_fix, read_cmd->sub_type, data_addr);
				rtn = send_fix_param(read_cmd, msg, data_addr, data_len);
			}
		}
		break;
	case MODE_AE_WEIGHT_TABLE:
		{
			if (NULL == sensor_raw_fix) {
				return rtn;
			}
			data_len = 0;
			rtn = get_ae_weight_param_length(sensor_raw_fix, read_cmd->sub_type, &data_len);
			data_addr = (cmr_u32 *) ispParserAlloc(data_len);
			memset((cmr_u8 *) data_addr, 0x00, data_len);
			if (0 != data_len && NULL != data_addr) {
				rtn = get_ae_weight_param(sensor_raw_fix, read_cmd->sub_type, data_addr);
				rtn = send_fix_param(read_cmd, msg, data_addr, data_len);
			}
		}
		break;
	case MODE_AE_SCENE_TABLE:
		{
			if (NULL == sensor_raw_fix) {
				return rtn;
			}
			data_len = 0;
			rtn = get_ae_scene_param_length(sensor_raw_fix, read_cmd->sub_type, &data_len);
			data_addr = (cmr_u32 *) ispParserAlloc(data_len);
			memset((cmr_u8 *) data_addr, 0x00, data_len);
			if (0 != data_len && NULL != data_addr) {
				rtn = get_ae_scene_param(sensor_raw_fix, read_cmd->sub_type, data_addr);
				rtn = send_fix_param(read_cmd, msg, data_addr, data_len);
			}
		}
		break;
	case MODE_AE_AUTO_ISO_TABLE:
		{
			if (NULL == sensor_raw_fix) {
				return rtn;
			}
			data_len = 0;
			rtn = get_ae_auto_iso_param_length(sensor_raw_fix, read_cmd->sub_type, &data_len);
			data_addr = (cmr_u32 *) ispParserAlloc(data_len);
			memset((cmr_u8 *) data_addr, 0x00, data_len);
			if (0 != data_len && NULL != data_addr) {
				rtn = get_ae_auto_iso_param(sensor_raw_fix, read_cmd->sub_type, data_addr);
				rtn = send_fix_param(read_cmd, msg, data_addr, data_len);
			}
		}
		break;
	case MODE_LNC_DATA:
		{
			if (NULL == sensor_raw_fix) {
				return rtn;
			}
			data_len = 0;
			rtn = get_lnc_param_length(sensor_raw_fix, read_cmd->sub_type, &data_len);
			data_addr = (cmr_u32 *) ispParserAlloc(data_len);
			memset((cmr_u8 *) data_addr, 0x00, data_len);
			if (0 != data_len && NULL != data_addr) {
				rtn = get_lnc_param(sensor_raw_fix, read_cmd->sub_type, data_addr);
				rtn = send_fix_param(read_cmd, msg, data_addr, data_len);
			}
		}
		break;
	case MODE_NOTE_DATA:
		{
			data_len = sensor_note_param.node_len;
			data_addr = (cmr_u32 *) ispParserAlloc(data_len);
			memset((cmr_u8 *) data_addr, 0x00, data_len);
			if (0 != data_len && NULL != data_addr) {
				memcpy((cmr_u8 *) data_addr, sensor_note_param.note, data_len);
				rtn = send_note_param(read_cmd, msg, data_addr, data_len);
			}
		}
		break;
	case MODE_LIB_INFO_DATA:
		{
			data_len = sizeof(struct sensor_libuse_info);
			data_addr = (cmr_u32 *) ispParserAlloc(data_len);
			memset((cmr_u8 *) data_addr, 0x00, data_len);
			if (NULL != data_addr) {
				memcpy((cmr_u8 *) data_addr, sensor_raw_info_ptr->libuse_info, data_len);
				rtn = send_libuse_info_param(read_cmd, msg, data_addr, data_len);
			} else {
				ISP_LOGE("fail to get libuse!");
			}
		}
		break;
	default:
		break;
	}

	ispParserFree(data_addr);
	return rtn;
}

cmr_s32 down_ae_table_param(struct sensor_raw_fix_info * sensor_raw_fix, cmr_u16 sub_type, cmr_u8 * data_addr)
{
	cmr_s32 rtn = 0x00;
	cmr_u8 flicker = (sub_type >> 4) & 0x0f;
	cmr_u8 iso = sub_type & 0x0f;
	cmr_u32 offset_tmp = 0;

	if (NULL == sensor_raw_fix || NULL == data_addr) {
		ISP_LOGE("fail to check param");
		rtn = 0x01;
		return rtn;
	}
	memcpy((void *)sensor_raw_fix->ae.ae_tab[flicker][iso].index, (void *)(data_addr + offset_tmp), sensor_raw_fix->ae.ae_tab[flicker][iso].index_len);
	offset_tmp += sensor_raw_fix->ae.ae_tab[flicker][iso].index_len;
	memcpy((void *)sensor_raw_fix->ae.ae_tab[flicker][iso].exposure, (void *)(data_addr + offset_tmp), sensor_raw_fix->ae.ae_tab[flicker][iso].exposure_len);
	offset_tmp += sensor_raw_fix->ae.ae_tab[flicker][iso].exposure_len;
	memcpy((void *)sensor_raw_fix->ae.ae_tab[flicker][iso].dummy, (void *)(data_addr + offset_tmp), sensor_raw_fix->ae.ae_tab[flicker][iso].dummy_len);
	offset_tmp += sensor_raw_fix->ae.ae_tab[flicker][iso].dummy_len;
	memcpy((void *)sensor_raw_fix->ae.ae_tab[flicker][iso].again, (void *)(data_addr + offset_tmp), sensor_raw_fix->ae.ae_tab[flicker][iso].again_len);
	offset_tmp += sensor_raw_fix->ae.ae_tab[flicker][iso].again_len;
	memcpy((void *)sensor_raw_fix->ae.ae_tab[flicker][iso].dgain, (void *)(data_addr + offset_tmp), sensor_raw_fix->ae.ae_tab[flicker][iso].dgain_len);
	return rtn;
}

cmr_s32 down_ae_weight_param(struct sensor_raw_fix_info * sensor_raw_fix, cmr_u16 sub_type, cmr_u8 * data_addr)
{
	cmr_s32 rtn = 0x00;
	cmr_u8 weight = sub_type;
	cmr_u32 offset_tmp = 0;

	if (NULL == sensor_raw_fix || NULL == data_addr) {
		ISP_LOGE("fail to check param");
		rtn = 0x01;
		return rtn;
	}
	memcpy((void *)sensor_raw_fix->ae.weight_tab[weight].weight_table, (void *)(data_addr + offset_tmp), sensor_raw_fix->ae.weight_tab[weight].len);
	return rtn;
}

cmr_s32 down_ae_scene_param(struct sensor_raw_fix_info * sensor_raw_fix, cmr_u16 sub_type, cmr_u8 * data_addr)
{
	cmr_s32 rtn = 0x00;
	cmr_u8 scene = (sub_type >> 4) & 0x0f;
	cmr_u8 flicker = sub_type & 0x0f;
	cmr_u32 offset_tmp = 0;

	if (NULL == sensor_raw_fix || NULL == data_addr) {
		ISP_LOGE("fail to check param");
		rtn = 0x01;
		return rtn;
	}
	memcpy((void *)sensor_raw_fix->ae.scene_tab[scene][flicker].scene_info, (void *)(data_addr + offset_tmp), sensor_raw_fix->ae.scene_tab[scene][flicker].scene_info_len);
	offset_tmp += sensor_raw_fix->ae.scene_tab[scene][flicker].scene_info_len;
	memcpy((void *)sensor_raw_fix->ae.scene_tab[scene][flicker].index, (void *)(data_addr + offset_tmp), sensor_raw_fix->ae.scene_tab[scene][flicker].index_len);
	offset_tmp += sensor_raw_fix->ae.scene_tab[scene][flicker].index_len;
	memcpy((void *)sensor_raw_fix->ae.scene_tab[scene][flicker].exposure, (void *)(data_addr + offset_tmp), sensor_raw_fix->ae.scene_tab[scene][flicker].exposure_len);
	offset_tmp += sensor_raw_fix->ae.scene_tab[scene][flicker].exposure_len;
	memcpy((void *)sensor_raw_fix->ae.scene_tab[scene][flicker].dummy, (void *)(data_addr + offset_tmp), sensor_raw_fix->ae.scene_tab[scene][flicker].dummy_len);
	offset_tmp += sensor_raw_fix->ae.scene_tab[scene][flicker].dummy_len;
	memcpy((void *)sensor_raw_fix->ae.scene_tab[scene][flicker].again, (void *)(data_addr + offset_tmp), sensor_raw_fix->ae.scene_tab[scene][flicker].again_len);
	offset_tmp += sensor_raw_fix->ae.scene_tab[scene][flicker].again_len;
	memcpy((void *)sensor_raw_fix->ae.scene_tab[scene][flicker].dgain, (void *)(data_addr + offset_tmp), sensor_raw_fix->ae.scene_tab[scene][flicker].dgain_len);
	return rtn;
}

cmr_s32 down_ae_auto_iso_param(struct sensor_raw_fix_info * sensor_raw_fix, cmr_u16 sub_type, cmr_u8 * data_addr)
{
	cmr_s32 rtn = 0x00;
	cmr_u8 iso = sub_type;
	cmr_u32 offset_tmp = 0;

	if (NULL == sensor_raw_fix || NULL == data_addr) {
		ISP_LOGE("fail to check param");
		rtn = 0x01;
		return rtn;
	}
	memcpy((void *)sensor_raw_fix->ae.auto_iso_tab[iso].auto_iso_tab, (void *)(data_addr + offset_tmp), sensor_raw_fix->ae.auto_iso_tab[iso].len);
	return rtn;
}

cmr_s32 down_lnc_param(struct sensor_raw_fix_info * sensor_raw_fix, cmr_u16 sub_type, cmr_u8 * data_addr, cmr_u32 data_len)
{
	cmr_s32 rtn = 0x00;
	cmr_u32 offset_tmp = 0;
	cmr_u16 lnc_ct = sub_type;

	if (NULL == sensor_raw_fix || NULL == data_addr) {
		ISP_LOGE("fail to check param");
		rtn = 0x01;
		return rtn;
	}
	memcpy((void *)sensor_raw_fix->lnc.map[lnc_ct].map_info, (void *)(data_addr + offset_tmp), sensor_raw_fix->lnc.map[lnc_ct].map_info_len);
	offset_tmp += sensor_raw_fix->lnc.map[lnc_ct].map_info_len;
	sensor_raw_fix->lnc.map[lnc_ct].lnc_len = data_len - sensor_raw_fix->lnc.map[lnc_ct].map_info_len;
	memcpy((void *)sensor_raw_fix->lnc.map[lnc_ct].lnc_addr, (void *)(data_addr + offset_tmp), sensor_raw_fix->lnc.map[lnc_ct].lnc_len);
	offset_tmp += sensor_raw_fix->lnc.map[lnc_ct].lnc_len;
	return rtn;
}

cmr_s32 down_awb_param(struct sensor_raw_fix_info * sensor_raw_fix, cmr_u16 sub_type, cmr_u8 * data_addr, cmr_u32 data_len)
{
	cmr_s32 rtn = 0x00;
	UNUSED(sensor_raw_fix);
	UNUSED(sub_type);
	UNUSED(data_addr);
	UNUSED(data_len);
	return rtn;
}

cmr_s32 down_isp_param(cmr_handle isp_handler, struct isp_data_header_normal * write_cmd, struct msg_head_tag * msg, cmr_u8 * isp_data_ptr, cmr_s32 value)
{
	cmr_s32 rtn = 0x00;
	static cmr_u8 *data_addr = NULL;
	static cmr_u32 offset = 0;
	static cmr_u32 flag = 0;
	cmr_u32 data_len = 0;

	cmr_u8 mode_id = write_cmd->isp_mode;
	struct sensor_raw_fix_info *sensor_raw_fix = NULL;
	struct isp_mode_param_info mode_param_info;
	struct sensor_raw_note_info sensor_note_param;
	struct msg_head_tag *msg_ret = (struct msg_head_tag *)(eng_rsp_diag + 1);
	memset(&mode_param_info, 0, sizeof(struct isp_mode_param_info));
	memset(&sensor_note_param, 0, sizeof(struct sensor_raw_note_info));
	cmr_u32 len_msg = sizeof(struct msg_head_tag);
	cmr_u32 len_data_header = sizeof(struct isp_data_header_normal);

	SENSOR_EXP_INFO_T_PTR sensor_info_ptr = Sensor_GetInfo();
	struct sensor_raw_info *sensor_raw_info_ptr = (struct sensor_raw_info *)sensor_info_ptr->raw_info_ptr;
	if (MAX_MODE_NUM > mode_id) {
		if (NULL != sensor_raw_info_ptr->fix_ptr[mode_id]) {
			sensor_raw_fix = sensor_raw_info_ptr->fix_ptr[mode_id];
		} else {
			return ISP_ERROR;
		}
		if (NULL != sensor_raw_info_ptr->mode_ptr[mode_id].addr) {
			mode_param_info = sensor_raw_info_ptr->mode_ptr[mode_id];
		} else {
			return ISP_ERROR;
		}
		if (NULL != sensor_raw_info_ptr->note_ptr[mode_id].note) {
			sensor_note_param = sensor_raw_info_ptr->note_ptr[mode_id];
		} else {
			return ISP_ERROR;
		}
	}
	if (!isp_data_ptr) {
		ISP_LOGE("fail to check param");
		return ISP_ERROR;
	}
	ISP_LOGV("down_isp_param write_cmd->main_type %d\n", write_cmd->main_type);

	switch (write_cmd->main_type) {
	case MODE_TUNE_INFO:
		{
			ISP_LOGV("MODE_TUNE_INFO \n");
			if (0 == flag) {
				data_len = mode_param_info.len;
				ISP_LOGV("mode tune data len = %d", data_len);
				data_addr = (cmr_u8 *) ispParserAlloc(data_len);
				if (!data_addr)
					ISP_LOGE("fail to do malloc mem!");
				data_len = msg->len - len_msg - len_data_header;
				memcpy(data_addr + offset, isp_data_ptr, data_len);
				offset += data_len;
				flag++;
			} else {
				data_len = msg->len - len_msg - len_data_header;
				memcpy(data_addr + offset, isp_data_ptr, data_len);
				offset += data_len;
			}
			if (0x01 == write_cmd->packet_status) {
				offset = 0;
				flag = 0;
				if (MAX_MODE_NUM > mode_id){
					if (!sensor_raw_info_ptr->mode_ptr[mode_id].addr)
						ISP_LOGV(" sensor raw mode ptr addr is , mode_id=%d", mode_id);
					memcpy(sensor_raw_info_ptr->mode_ptr[mode_id].addr, data_addr, sensor_raw_info_ptr->mode_ptr[mode_id].len);
				}
				rtn = isp_ioctl(isp_handler, ISP_CTRL_IFX_PARAM_UPDATE | ISP_TOOL_CMD_ID, data_addr);
				if (NULL != data_addr) {
					free(data_addr);
					data_addr = NULL;
				}
				eng_rsp_diag[value] = 0x00;
				value = value + 0x04;
				msg_ret->len = value - 1;
				eng_rsp_diag[value] = 0x7e;
				rtn = send(sockfd, eng_rsp_diag, value + 1, 0);
			}
		}
		break;
	case MODE_AE_TABLE:
		{
			ISP_LOGV("MODE_AE_TABLE \n");
			if (0 == flag) {
				if (NULL == sensor_raw_fix) {
					ISP_LOGE("fail to check param!");
					rtn = 0x01;
					return rtn;
				}
				rtn = get_ae_table_param_length(sensor_raw_fix, write_cmd->sub_type, &data_len);
				data_addr = (cmr_u8 *) ispParserAlloc(data_len);
				if (NULL == data_addr) {
					ISP_LOGE("fail to malloc mem!");
					rtn = 0x01;
					return rtn;
				}
				flag++;
			}
			data_len = msg->len - len_msg - len_data_header;
			memcpy(data_addr + offset, isp_data_ptr, data_len);
			offset += data_len;
			if (0x01 == write_cmd->packet_status) {
				flag = 0;
				offset = 0;
				rtn = down_ae_table_param(sensor_raw_fix, write_cmd->sub_type, data_addr);
				rtn = isp_ioctl(isp_handler, ISP_CTRL_IFX_PARAM_UPDATE | ISP_TOOL_CMD_ID, data_addr);
				if (NULL != data_addr) {
					free(data_addr);
					data_addr = NULL;
				}

				eng_rsp_diag[value] = 0x00;
				value = value + 0x04;
				msg_ret->len = value - 1;
				eng_rsp_diag[value] = 0x7e;
				rtn = send(sockfd, eng_rsp_diag, value + 1, 0);
			}
		}
		break;
	case MODE_AE_WEIGHT_TABLE:
		{
			ISP_LOGV("MODE_AE_WEIGHT_TABLE \n");
			if (0 == flag) {
				if (NULL == sensor_raw_fix) {
					ISP_LOGE("fail to check param!");
					rtn = 0x01;
					return rtn;
				}
				rtn = get_ae_weight_param_length(sensor_raw_fix, write_cmd->sub_type, &data_len);
				data_addr = (cmr_u8 *) ispParserAlloc(data_len);
				if (NULL == data_addr) {
					ISP_LOGE("fail to malloc mem!");
					rtn = 0x01;
					return rtn;
				}
				flag++;
			}
			data_len = msg->len - len_msg - len_data_header;
			memcpy(data_addr + offset, isp_data_ptr, data_len);
			offset += data_len;

			if (0x01 == write_cmd->packet_status) {
				flag = 0;
				offset = 0;
				rtn = down_ae_weight_param(sensor_raw_fix, write_cmd->sub_type, data_addr);
				rtn = isp_ioctl(isp_handler, ISP_CTRL_IFX_PARAM_UPDATE | ISP_TOOL_CMD_ID, data_addr);
				if (NULL != data_addr) {
					free(data_addr);
					data_addr = NULL;
				}

				eng_rsp_diag[value] = 0x00;
				value = value + 0x04;
				msg_ret->len = value - 1;
				eng_rsp_diag[value] = 0x7e;
				rtn = send(sockfd, eng_rsp_diag, value + 1, 0);
			}
		}
		break;
	case MODE_AE_SCENE_TABLE:
		{
			if (0 == flag) {
				rtn = get_ae_scene_param_length(sensor_raw_fix, write_cmd->sub_type, &data_len);
				data_addr = (cmr_u8 *) ispParserAlloc(data_len);
				if (NULL == data_addr) {
					ISP_LOGE("fail to malloc mem!");
					rtn = 0x01;
					return rtn;
				}
				flag++;
			}
			data_len = msg->len - len_msg - len_data_header;
			memcpy(data_addr + offset, isp_data_ptr, data_len);
			offset += data_len;
			if (0x01 == write_cmd->packet_status) {
				flag = 0;
				offset = 0;
				rtn = down_ae_scene_param(sensor_raw_fix, write_cmd->sub_type, data_addr);
				rtn = isp_ioctl(isp_handler, ISP_CTRL_IFX_PARAM_UPDATE | ISP_TOOL_CMD_ID, data_addr);
				if (NULL != data_addr) {
					free(data_addr);
					data_addr = NULL;
				}

				eng_rsp_diag[value] = 0x00;
				value = value + 0x04;
				msg_ret->len = value - 1;
				eng_rsp_diag[value] = 0x7e;
				ISP_LOGV("MODE_AE_SCENE_TABLE \n");
				rtn = send(sockfd, eng_rsp_diag, value + 1, 0);
			}
		}
		break;
	case MODE_AE_AUTO_ISO_TABLE:
		{
			if (0 == flag) {
				rtn = get_ae_auto_iso_param_length(sensor_raw_fix, write_cmd->sub_type, &data_len);
				data_addr = (cmr_u8 *) ispParserAlloc(data_len);
				if (NULL == data_addr) {
					ISP_LOGE("fail to malloc mem!");
					rtn = 0x01;
					return rtn;
				}
				flag++;
			}
			data_len = msg->len - len_msg - len_data_header;
			memcpy(data_addr + offset, isp_data_ptr, data_len);
			offset += data_len;
			if (0x01 == write_cmd->packet_status) {
				flag = 0;
				offset = 0;
				rtn = down_ae_auto_iso_param(sensor_raw_fix, write_cmd->sub_type, data_addr);
				rtn = isp_ioctl(isp_handler, ISP_CTRL_IFX_PARAM_UPDATE | ISP_TOOL_CMD_ID, data_addr);
				if (NULL != data_addr) {
					free(data_addr);
					data_addr = NULL;
				}

				eng_rsp_diag[value] = 0x00;
				value = value + 0x04;
				msg_ret->len = value - 1;
				eng_rsp_diag[value] = 0x7e;
				ISP_LOGV("MODE_AE_AUTO_ISO_TABLE \n");
				rtn = send(sockfd, eng_rsp_diag, value + 1, 0);
			}
		}
		break;
	case MODE_LNC_DATA:
		{
			if (0 == flag) {
				data_addr = (cmr_u8 *) ispParserAlloc(write_cmd->data_total_len);
				if (NULL == data_addr) {
					ISP_LOGE("fail to malloc mem!");
					rtn = 0x01;
					return rtn;
				}
				flag++;
			}
			data_len = msg->len - len_msg - len_data_header;
			memcpy(data_addr + offset, isp_data_ptr, data_len);
			offset += data_len;
			if (0x01 == write_cmd->packet_status) {
				flag = 0;
				offset = 0;
				rtn = down_lnc_param(sensor_raw_fix, write_cmd->sub_type, data_addr, write_cmd->data_total_len);
				rtn = isp_ioctl(isp_handler, ISP_CTRL_IFX_PARAM_UPDATE | ISP_TOOL_CMD_ID, data_addr);

				eng_rsp_diag[value] = 0x00;
				value = value + 0x04;
				msg_ret->len = value - 1;
				eng_rsp_diag[value] = 0x7e;
				ISP_LOGV("MODE_LNC_DATA \n");
				rtn = send(sockfd, eng_rsp_diag, value + 1, 0);
			}
		}
		break;
	case MODE_AWB_DATA:
		{
			if (0 == flag) {
				data_addr = (cmr_u8 *) ispParserAlloc(write_cmd->data_total_len);
				if (NULL == data_addr) {
					ISP_LOGE("fail to malloc mem!");
					rtn = 0x01;
					return rtn;
				}
				flag++;
			}
			data_len = msg->len - len_msg - len_data_header;
			memcpy(data_addr + offset, isp_data_ptr, data_len);
			offset += data_len;
			if (0x01 == write_cmd->packet_status) {
				flag = 0;
				offset = 0;
				rtn = down_awb_param(sensor_raw_fix, write_cmd->sub_type, data_addr, write_cmd->data_total_len);
				rtn = isp_ioctl(isp_handler, ISP_CTRL_IFX_PARAM_UPDATE | ISP_TOOL_CMD_ID, data_addr);

				eng_rsp_diag[value] = 0x00;
				value = value + 0x04;
				msg_ret->len = value - 1;
				eng_rsp_diag[value] = 0x7e;
				ISP_LOGV("MODE_AWB_DATA \n");
				rtn = send(sockfd, eng_rsp_diag, value + 1, 0);
			}
			break;
		}
	case MODE_NOTE_DATA:
		{
			if (0 == flag) {
				data_len = sensor_note_param.node_len;
				data_addr = (cmr_u8 *) ispParserAlloc(data_len);
				if (NULL == data_addr) {
					ISP_LOGE("fail to malloc mem!");
					rtn = 0x01;
					return rtn;
				}
				flag++;
			}
			data_len = msg->len - len_msg - len_data_header;
			memcpy(data_addr + offset, isp_data_ptr, data_len);
			offset += data_len;
			if (0x01 == write_cmd->packet_status) {
				offset = 0;
				flag = 0;
				if (MAX_MODE_NUM > mode_id)
					memcpy(sensor_raw_info_ptr->note_ptr[mode_id].note, data_addr, sensor_raw_info_ptr->note_ptr[mode_id].node_len);
				if (NULL != data_addr) {
					free(data_addr);
					data_addr = NULL;
				}

				eng_rsp_diag[value] = 0x00;
				value = value + 0x04;
				msg_ret->len = value - 1;
				eng_rsp_diag[value] = 0x7e;
				ISP_LOGV("CMD_WRITE_ISP_PARAM3 \n");
				rtn = send(sockfd, eng_rsp_diag, value + 1, 0);
			}
			break;
		}
	case MODE_LIB_INFO_DATA:
		{
			if (0 == flag) {
				data_addr = (cmr_u8 *) ispParserAlloc(write_cmd->data_total_len);
				if (NULL == data_addr) {
					ISP_LOGE("fail to malloc mem!");
					rtn = 0x01;
					return rtn;
				}
				flag++;
			}
			data_len = msg->len - len_msg - len_data_header;
			memcpy(data_addr + offset, isp_data_ptr, data_len);
			offset += data_len;
			if (0x01 == write_cmd->packet_status) {
				flag = 0;
				offset = 0;
				memcpy(sensor_raw_info_ptr->libuse_info, data_addr, sizeof(struct sensor_libuse_info));

				eng_rsp_diag[value] = 0x00;
				value = value + 0x04;
				msg_ret->len = value - 1;
				eng_rsp_diag[value] = 0x7e;
				ISP_LOGV("MODE_LIB_INFO_DATA: \n");
				rtn = send(sockfd, eng_rsp_diag, value + 1, 0);
			}
			break;
		}
	default:
		break;
	}
	return rtn;
}

cmr_s32 check_cmd_valid(struct isp_check_cmd_valid * cmd, struct msg_head_tag * msg)
{
	cmr_s32 rtn = 0x00;
	cmr_u32 rsp_len = 0;

	cmr_u32 len_msg = sizeof(struct msg_head_tag);
	cmr_u32 len_data_header = sizeof(struct isp_data_header_normal);
	struct isp_data_header_normal data_header;
	struct msg_head_tag msg_tag;
	memset(&data_header, 0, sizeof(struct isp_data_header_normal));
	memset(&msg_tag, 0, sizeof(struct msg_head_tag));

	struct isp_check_cmd_valid *cmd_ptr = (struct isp_check_cmd_valid *)cmd;
	cmr_u32 unvalid_flag = 0;
	if (MODE_MAX >= cmd_ptr->main_type) {
		switch (cmd_ptr->main_type) {
		case MODE_NR_DATA:
			{
				if (FILE_NAME_MAX < cmd_ptr->sub_type) {
					unvalid_flag = 1;
				}
			}
			break;
		case MODE_AE_TABLE:
			{
				cmr_u8 flicker = (cmd_ptr->sub_type >> 4) & 0x0f;
				cmr_u8 iso = cmd_ptr->sub_type & 0x0f;
				if (ISP_ANTIFLICKER_MAX < flicker) {
					unvalid_flag = 1;
				} else {
					if (ISP_ISO_NUM_MAX < iso) {
						unvalid_flag = 1;
					}
				}
			}
			break;
		case MODE_AE_WEIGHT_TABLE:
			{
				if (ISP_AE_WEIGHT_TYPE_MAX < cmd_ptr->sub_type) {
					unvalid_flag = 1;
				}
			}
			break;
		case MODE_AE_SCENE_TABLE:
			{
				cmr_u8 scene = (cmd_ptr->sub_type >> 4) & 0x0f;
				cmr_u8 flicker = cmd_ptr->sub_type & 0x0f;
				if (ISP_ANTIFLICKER_MAX < flicker) {
					unvalid_flag = 1;
				} else {
					if (ISP_SCENE_NUM_MAX < scene) {
						unvalid_flag = 1;
					}
				}
			}
			break;
		case MODE_AE_AUTO_ISO_TABLE:
			{
				if (ISP_ANTIFLICKER_MAX < cmd_ptr->sub_type) {
					unvalid_flag = 1;
				}
			}
			break;
		case MODE_LNC_DATA:
			{
				if (ISP_LNC_TAB_MAX < cmd_ptr->sub_type) {
					unvalid_flag = 1;
				}
			}
			break;
		case MODE_AWB_DATA:
			{
				if (1 < cmd_ptr->sub_type) {
					unvalid_flag = 1;
				}
			}
			break;
		default:
			break;
		}
	} else {
		ISP_LOGE("fail to check flag, is out of range!");
		unvalid_flag = 1;;
	}
	if (0 != unvalid_flag) {
		ISP_LOGE("fail to check flag:sub type out of range!");
		rtn = 0x01;
		goto CHECK_CMD_UNVALID;
	} else {
		return rtn;
	}

CHECK_CMD_UNVALID:
	data_header.isp_mode = cmd_ptr->isp_mode;
	data_header.main_type = cmd_ptr->main_type;
	data_header.sub_type = cmd_ptr->sub_type;
	data_header.data_total_len = 0;

	msg_tag.seq_num = msg->seq_num;
	msg_tag.subtype = msg->subtype;
	msg_tag.type = msg->type;
	msg_tag.len = len_msg + len_data_header;
	data_header.packet_status = 0x02;

	memcpy(eng_rsp_diag + 1, (cmr_u8 *) & msg_tag, len_msg);
	rsp_len = len_msg + 1;
	memcpy(eng_rsp_diag + rsp_len, (cmr_u8 *) & data_header, len_data_header);
	rsp_len = rsp_len + len_data_header + 1;
	eng_rsp_diag[rsp_len] = 0x7e;
	rtn = send(sockfd, eng_rsp_diag, rsp_len + 1, 0);
	rtn = 0x01;
	return rtn;
}

static cmr_s32 handle_isp_data(cmr_u8 * buf, cmr_u32 len)
{
	cmr_s32 rlen = 0, rsp_len = 0;
	cmr_s32 ret = 1, res = 0;
	MSG_HEAD_T *msg, *msg_ret;

	ISP_DATA_HEADER_T isp_msg;
	cmr_s32 preview_tmpflag = 0;
	memset(&isp_msg, 0, sizeof(ISP_DATA_HEADER_T));

	struct camera_func *fun_ptr = ispvideo_GetCameraFunc();
	cmr_s32 is_stop_preview = 0;

	if (len < sizeof(MSG_HEAD_T) + 2) {
		ISP_LOGE("fail to check param\n");
		return -1;
	}

	msg = (MSG_HEAD_T *) (buf + 1);
	if (msg->type != 0xfe)
		return -1;

	rsp_len = sizeof(MSG_HEAD_T) + 1;
	memset(eng_rsp_diag, 0, sizeof(eng_rsp_diag));
	memcpy(eng_rsp_diag, buf, rsp_len);
	msg_ret = (MSG_HEAD_T *) (eng_rsp_diag + 1);

	if (CMD_GET_PREVIEW_PICTURE != msg->subtype) {
		msg_ret->seq_num = sequence_num++;
	}

	switch (msg->subtype) {
	case CMD_SFT_READ:
		{
			ISP_LOGV("get af info");
			ret = isp_sft_read(isp_handler, &eng_rsp_diag[rsp_len], &len);
			ISP_LOGV("ISP_SFT:CMD_SFT_READ rsp_len %d len %d\n", rsp_len, len);
			rsp_len += len;
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len - 1;
			res = send(sockfd, eng_rsp_diag, rsp_len + 1, 0);
			break;
		}
	case CMD_SFT_WRITE:
		{
			ISP_LOGV("set af info");
			len = msg_ret->len - 8;
			ret = isp_sft_write(isp_handler, buf + rsp_len, &len);
			ISP_LOGV("ISP_SFT:CMD_SFT_WRITE rsp_len %d len %d\n", rsp_len, len);
			rsp_len += ispvideo_SetreTurnValue((cmr_u8 *) & eng_rsp_diag[rsp_len], ISP_CMD_SUCCESS);
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len - 1;
			res = send(sockfd, eng_rsp_diag, rsp_len + 1, 0);
			break;
		}
	case CMD_SFT_TRIG:
		{
			ISP_LOGV("af trig");
			ret = isp_ioctl(isp_handler, ISP_CTRL_AF, NULL);
			if (!ret) {
				rsp_len += ispvideo_SetreTurnValue((cmr_u8 *) & eng_rsp_diag[rsp_len], ISP_CMD_SUCCESS);
			} else {
				rsp_len += ispvideo_SetreTurnValue((cmr_u8 *) & eng_rsp_diag[rsp_len], ISP_CMD_FAIL);
			}
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len - 1;
			res = send(sockfd, eng_rsp_diag, rsp_len + 1, 0);
			break;
		}
	case CMD_SFT_SET_POS:
		{
			ISP_LOGV("set pos");
			g_af_pos = *(cmr_u32 *) (buf + 9);
			ret = isp_ioctl(isp_handler, ISP_CTRL_SET_AF_POS, buf + 9);
			ret = isp_ioctl(isp_handler, ISP_CTRL_SFT_SET_PASS, NULL);
			usleep(1000 * 100);
			if (!ret) {
				rsp_len += ispvideo_SetreTurnValue((cmr_u8 *) & eng_rsp_diag[rsp_len], ISP_CMD_SUCCESS);
			} else {
				rsp_len += ispvideo_SetreTurnValue((cmr_u8 *) & eng_rsp_diag[rsp_len], ISP_CMD_FAIL);
			}
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len - 1;
			res = send(sockfd, eng_rsp_diag, rsp_len + 1, 0);
			break;
		}
	case CMD_SFT_GET_POS:
		{
			ISP_LOGV("get pos");
			cmr_u32 pos;
			ret = isp_ioctl(isp_handler, ISP_CTRL_GET_AF_POS, &pos);
			if (!ret) {
				rsp_len += ispvideo_SetreTurnValue((cmr_u8 *) & eng_rsp_diag[rsp_len], ISP_CMD_SUCCESS);
			} else {
				rsp_len += ispvideo_SetreTurnValue((cmr_u8 *) & eng_rsp_diag[rsp_len], ISP_CMD_FAIL);
			}
			*(cmr_u32 *) (eng_rsp_diag + rsp_len) = pos;
			rsp_len += sizeof(cmr_u32);
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len - 1;
			res = send(sockfd, eng_rsp_diag, rsp_len + 1, 0);
			break;
		}
	case CMD_SFT_OPEN_FILTER:
		{
			ISP_LOGV("open the filter");
			ret = isp_ioctl(isp_handler, ISP_CTRL_SFT_SET_PASS, NULL);
			usleep(1000 * 100);
			if (!ret) {
				rsp_len += ispvideo_SetreTurnValue((cmr_u8 *) & eng_rsp_diag[rsp_len], ISP_CMD_SUCCESS);
			} else {
				rsp_len += ispvideo_SetreTurnValue((cmr_u8 *) & eng_rsp_diag[rsp_len], ISP_CMD_FAIL);
			}
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len - 1;
			res = send(sockfd, eng_rsp_diag, rsp_len + 1, 0);
			break;
		}
	case CMD_SFT_GET_AF_VALUE:	/* value back to PC */
		{
			cmr_u32 statistic[50] = { 0 };
			ISP_LOGV("get af value 1");
			ret = isp_ioctl(isp_handler, ISP_CTRL_SFT_GET_AF_VALUE, statistic);
			ISP_LOGI("get af value 2 ret=%d,af_value=%d,af_value=%d", ret, statistic[0], statistic[25]);
			if (!ret) {
				rsp_len += ispvideo_SetreTurnValue((cmr_u8 *) & eng_rsp_diag[rsp_len], ISP_CMD_SUCCESS);
				memmove(eng_rsp_diag + rsp_len, statistic, sizeof(statistic));
				rsp_len += sizeof(statistic);
			} else {
				rsp_len += ispvideo_SetreTurnValue((cmr_u8 *) & eng_rsp_diag[rsp_len], ISP_CMD_FAIL);
			}
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len - 1;
			res = send(sockfd, eng_rsp_diag, rsp_len + 1, 0);
			break;
		}
	case CMD_SFT_TAKE_PICTURE:	/* pic back to PC */
		{
			ISP_LOGV("sft take picture");
			cmr_u8 *isp_ptr = buf + sizeof(MSG_HEAD_T) + 1;
			cmr_u32 width, height, interval;

			g_command = CMD_SFT_TAKE_PICTURE;
			if (NULL != fun_ptr->take_picture) {
				capture_img_end_flag = 0;
				capture_flag = 1;
				capture_format = *(cmr_u32 *) isp_ptr;
				width = (*(cmr_u32 *) (isp_ptr + 4)) >> 16;
				height = (*(cmr_u32 *) (isp_ptr + 4)) & 0x0000ffff;
				interval = *(cmr_u32 *) (isp_ptr + 8);
				usleep(1000 * interval);
				if (NULL != fun_ptr->set_capture_size) {
					fun_ptr->set_capture_size(width, height);
				}
				fun_ptr->take_picture(0, capture_format);
				sem_wait(&capture_sem_lock);
				if (NULL != fun_ptr->stop_preview) {
					fun_ptr->stop_preview(0, 0);
				}
				usleep(1000 * 10);

				if (NULL != fun_ptr->start_preview) {
					fun_ptr->start_preview(0, 0);
				}

				capture_flag = 0;
				g_command = CMD_TAKE_PICTURE;
			}
			break;
		}
	case CMD_SFT_TAKE_PICTURE_NEW:	/* save pic to file system */
		{
			ISP_LOGV("sft take picture new");
			cmr_u8 *isp_ptr = buf + sizeof(MSG_HEAD_T) + 1;
			cmr_u32 startpos, endpos, step, width, height, interval;
			cmr_u32 pos;
			g_command = CMD_SFT_TAKE_PICTURE_NEW;
			if (NULL != fun_ptr->take_picture) {
				ret = isp_ioctl(isp_handler, ISP_CTRL_STOP_3A, NULL);

				startpos = *(cmr_u32 *) isp_ptr;
				endpos = *(cmr_u32 *) (isp_ptr + 4);
				step = *(cmr_u32 *) (isp_ptr + 8);
				capture_format = *(cmr_u32 *) (isp_ptr + 12);
				width = (*(cmr_u32 *) (isp_ptr + 16)) >> 16;
				height = (*(cmr_u32 *) (isp_ptr + 16)) & 0x0000ffff;
				interval = *(cmr_u32 *) (isp_ptr + 20);
				for (pos = startpos; pos <= endpos; pos += step) {
					g_af_pos = pos;
					ret = isp_ioctl(isp_handler, ISP_CTRL_SET_AF_POS, &pos);
					if (ret)
						ISP_LOGE("fail to ISP_CTRL_SET_AF_POS \n");
					usleep(1000 * 10);
					capture_img_end_flag = 0;
					capture_flag = 1;
					usleep(1000 * interval);
					if (NULL != fun_ptr->set_capture_size) {
						fun_ptr->set_capture_size(width, height);
					}
					fun_ptr->take_picture(0, capture_format);
					sem_wait(&capture_sem_lock);
					if (NULL != fun_ptr->stop_preview) {
						fun_ptr->stop_preview(0, 0);
					}
					usleep(1000 * 10);

					if (NULL != fun_ptr->start_preview) {
						fun_ptr->start_preview(0, 0);
					}
					capture_flag = 0;

				}
				g_command = CMD_TAKE_PICTURE;
				ret = isp_ioctl(isp_handler, ISP_CTRL_START_3A, NULL);
			}
			break;
		}
	case CMD_ASIC_TAKE_PICTURE:	/* save pic to file system */
		{
			ISP_LOGV("asic take picture");
			cmr_u8 *isp_ptr = buf + sizeof(MSG_HEAD_T) + 1;
			cmr_u32 width, height;
			g_command = CMD_ASIC_TAKE_PICTURE;
			if (NULL != fun_ptr->take_picture) {
				capture_img_end_flag = 0;
				capture_flag = 1;
				capture_format = *(cmr_u32 *) isp_ptr;
				width = (*(cmr_u32 *) (isp_ptr + 4)) >> 16;
				height = (*(cmr_u32 *) (isp_ptr + 4)) & 0x0000ffff;
				if (NULL != fun_ptr->set_capture_size) {
					fun_ptr->set_capture_size(width, height);
				}
				fun_ptr->take_picture(0, capture_format);
				sem_wait(&capture_sem_lock);
				if (NULL != fun_ptr->stop_preview) {
					fun_ptr->stop_preview(0, 0);
				}
				usleep(1000 * 10);

				if (NULL != fun_ptr->start_preview) {
					fun_ptr->start_preview(0, 0);
				}

				capture_flag = 0;
				g_command = CMD_TAKE_PICTURE;

				rsp_len += ispvideo_SetreTurnValue((cmr_u8 *) & eng_rsp_diag[rsp_len], ISP_CMD_SUCCESS);
				eng_rsp_diag[rsp_len] = 0x7e;
				msg_ret->len = rsp_len - 1;
				res = send(sockfd, eng_rsp_diag, rsp_len + 1, 0);
			}
			break;
		}
	case CMD_ASIC_TAKE_PICTURE_NEW:	/* save pic to file system */
		{
			ISP_LOGV("asic take picture new");
			cmr_u8 *isp_ptr = buf + sizeof(MSG_HEAD_T) + 1;;
			cmr_u32 startpos, endpos, step, width, height;
			cmr_u32 pos;
			g_command = CMD_ASIC_TAKE_PICTURE_NEW;
			if (NULL != fun_ptr->take_picture) {
				startpos = *(cmr_u32 *) isp_ptr;
				endpos = *(cmr_u32 *) (isp_ptr + 4);
				step = *(cmr_u32 *) (isp_ptr + 8);
				capture_format = *(cmr_u32 *) (isp_ptr + 12);
				width = (*(cmr_u32 *) (isp_ptr + 16)) >> 16;
				height = (*(cmr_u32 *) (isp_ptr + 16)) & 0x0000ffff;

				for (pos = startpos; pos <= endpos; pos += step) {
					g_af_pos = pos;
					ret = isp_ioctl(isp_handler, ISP_CTRL_SET_AF_POS, &pos);
					usleep(1000 * 10);
					capture_img_end_flag = 0;
					capture_flag = 1;
					if (NULL != fun_ptr->set_capture_size) {
						fun_ptr->set_capture_size(width, height);
					}
					fun_ptr->take_picture(0, capture_format);
					sem_wait(&capture_sem_lock);
					if (NULL != fun_ptr->stop_preview) {
						fun_ptr->stop_preview(0, 0);
					}
					usleep(1000 * 10);

					if (NULL != fun_ptr->start_preview) {
						fun_ptr->start_preview(0, 0);
					}
					capture_flag = 0;
				}
				g_command = CMD_TAKE_PICTURE;
			}
			break;
		}
	case CMD_START_PREVIEW:
		{
			ISP_LOGV("CMD_START_PREVIEW \n");
			if (NULL != fun_ptr->start_preview) {
				ret = fun_ptr->start_preview(0, 0);
			}
			if (!ret) {
				rsp_len += ispvideo_SetreTurnValue((cmr_u8 *) & eng_rsp_diag[rsp_len], ISP_CMD_SUCCESS);
				preview_flag = 1;
			} else {
				rsp_len += ispvideo_SetreTurnValue((cmr_u8 *) & eng_rsp_diag[rsp_len], ISP_CMD_FAIL);
			}
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len - 1;
			res = send(sockfd, eng_rsp_diag, rsp_len + 1, 0);
			break;
		}
	case CMD_STOP_PREVIEW:
		{
			ISP_LOGV("CMD_STOP_PREVIEW \n");
			preview_flag = 0;
			rsp_len += ispvideo_SetreTurnValue((cmr_u8 *) & eng_rsp_diag[rsp_len], ISP_CMD_SUCCESS);
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len - 1;
			res = send(sockfd, eng_rsp_diag, rsp_len + 1, 0);
			break;
		}
	case CMD_GET_PREVIEW_PICTURE:
		{
			if (1 == preview_flag) {
				preview_img_end_flag = 0;
				sem_wait(&preview_sem_lock);
				preview_img_end_flag = 1;
			} else {
				ISP_LOGV("CMD_GET_PREVIEW_PICTURE \n");
				rsp_len += rlen;
				eng_rsp_diag[rsp_len] = 0x7e;
				msg_ret->len = rsp_len - 1;
				res = send(sockfd, eng_rsp_diag, rsp_len + 1, 0);
			}
			break;
		}
	case CMD_READ_ISP_PARAM:
		{
			ISP_LOGV("CMD_READ_ISP_PARAM \n");
			struct isp_parser_buf_in in_param = { 0x00, 0x00 };
			struct isp_parser_buf_rtn rtn_param = { 0x00, 0x00 };
			struct isp_parser_cmd_param rtn_cmd;
			cmr_u32 packet_num = 0;
			cmr_u32 send_number = 0;
			cmr_u8 *dig_ptr = buf;
			cmr_u32 i = 0;
			cmr_u8 *isp_ptr = buf + sizeof(MSG_HEAD_T) + 1;

			memset(&rtn_cmd, 0, sizeof(rtn_cmd));
			in_param.buf_len = ispvideo_GetIspParamLenFromSt(dig_ptr);
			in_param.buf_addr = (cmr_uint) ispParserAlloc(in_param.buf_len);

			if ((0x00 != in_param.buf_len)
			    && (0x00 != in_param.buf_addr)) {
				ret = ispvideo_GetIspParamFromSt(isp_ptr, (struct isp_parser_buf_rtn *)&in_param);
				if (ret) {
					ISP_LOGE("fail to get param \n");
				}
				ret = ispParser(isp_handler, ISP_PARSER_DOWN, (void *)in_param.buf_addr, (void *)&rtn_cmd);
				if (ret) {
					ISP_LOGE("fail to parser param \n");
				}
				ret = ispParserFree((void *)in_param.buf_addr);
				if (ret) {
					ISP_LOGE("fail to free\n");
				}

				if (ISP_UP_PARAM == rtn_cmd.cmd) {
					ret = ispParser(isp_handler, ISP_PARSER_UP_PARAM, (void *)in_param.buf_addr, (void *)&rtn_param);

					if (0x00 == ret) {
						packet_num = (rtn_param.buf_len + SEND_DATA_SIZE - 1) / SEND_DATA_SIZE;
						for (i = 0; i < packet_num; i++, send_number++) {
							if (i < (packet_num - 1))
								len = SEND_DATA_SIZE;
							else
								len = rtn_param.buf_len - SEND_DATA_SIZE * i;

							rsp_len = sizeof(MSG_HEAD_T) + 1;

							isp_msg.totalpacket = packet_num;
							isp_msg.packetsn = send_number;

							memcpy(eng_rsp_diag + rsp_len, (char *)&isp_msg, sizeof(ISP_DATA_HEADER_T));
							rsp_len += sizeof(ISP_DATA_HEADER_T);

							memcpy(eng_rsp_diag + rsp_len, (char *)rtn_param.buf_addr + i * SEND_DATA_SIZE, len);

							rsp_len += len;
							eng_rsp_diag[rsp_len] = 0x7e;
							msg_ret->len = rsp_len - 1;
							msg_ret->seq_num = sequence_num++;
							res = send(sockfd, eng_rsp_diag, rsp_len + 1, 0);

						}

					}
				}
			}

			break;
		}
	case CMD_WRITE_ISP_PARAM:
		{
			ISP_LOGV("CMD_WRITE_ISP_PARAM \n");
			struct isp_parser_buf_in in_param = { 0x00, 0x00 };
			static struct isp_parser_buf_in packet_param = { 0x00, 0x00 };
			cmr_u8 *dig_ptr = buf;
			cmr_u8 *isp_ptr = buf + sizeof(MSG_HEAD_T) + sizeof(ISP_DATA_HEADER_T) + 1;
			cmr_u32 *packet = (cmr_u32 *) (buf + sizeof(MSG_HEAD_T) + 1);
			cmr_u32 packet_num = 0;
			static cmr_u32 offset = 0;
			cmr_u32 packet_total = 0;
			packet_total = packet[0];
			packet_num = packet[1];
			if (0 == packet_num) {
				packet_param.buf_addr = (cmr_uint) ispParserAlloc(packet_total * SEND_DATA_SIZE);
				packet_param.buf_len = packet_total * SEND_DATA_SIZE;
			}

			in_param.buf_len = ispvideo_GetIspParamLenFromSt(dig_ptr) - sizeof(ISP_DATA_HEADER_T);
			in_param.buf_addr = (cmr_uint) ispParserAlloc(in_param.buf_len);

			if ((0x00 != in_param.buf_len)
			    && (0x00 != in_param.buf_addr)) {
				ret = ispvideo_GetIspParamFromSt(isp_ptr, (struct isp_parser_buf_rtn *)&in_param);
				if (ret) {
					ISP_LOGE("fail to get param \n");
				}
				memcpy((char *)packet_param.buf_addr + offset, (void *)in_param.buf_addr, in_param.buf_len);
				offset += in_param.buf_len;

				if (in_param.buf_addr) {
					ret = ispParserFree((void *)in_param.buf_addr);
					if (ret) {
						ISP_LOGE("fail to parse free\n");
					}
				}
			}

			if (packet_num == (packet_total - 1)) {
				offset = 0;
				ret = ispParser(isp_handler, ISP_PARSER_DOWN, (void *)packet_param.buf_addr, NULL);
				ret = ispParserFree((void *)packet_param.buf_addr);
				rsp_len += ispvideo_SetreTurnValue((cmr_u8 *) & eng_rsp_diag[rsp_len], ret);
				eng_rsp_diag[rsp_len] = 0x7e;
				msg_ret->len = rsp_len - 1;
				res = send(sockfd, eng_rsp_diag, rsp_len + 1, 0);
			}

			break;
		}
	case CMD_READ_ISP_PARAM_V1:
		{
			ISP_LOGV("CMD_READ_ISP_PARAM_V1 \n");
			struct isp_data_header_read *read_cmd = NULL;
			cmr_u32 data_len = 0;
			cmr_u8 *dig_ptr = buf;
			cmr_u8 *isp_ptr = buf + sizeof(MSG_HEAD_T) + 1;
			struct isp_check_cmd_valid cmd;
			memset(&cmd, 0, sizeof(struct isp_check_cmd_valid));

			data_len = ispvideo_GetIspParamLenFromSt(dig_ptr);
			ISP_LOGV("ISP_TOOL : data_len = %d", data_len);
			if (DATA_CMD_LENGTH == data_len) {
				ISP_LOGV("ISP_TOOL : data is read cmd");
				read_cmd = (struct isp_data_header_read *)isp_ptr;
			} else {
				ISP_LOGE("fail to check cmd");
				break;
			}
			cmd.isp_mode = read_cmd->isp_mode;
			cmd.main_type = read_cmd->main_type;
			cmd.sub_type = read_cmd->sub_type;
			ret = check_cmd_valid(&cmd, msg);
			if (0 == ret) {
				if (0x02 > read_cmd->main_type) {
					ret = isp_nr_reg_read(isp_handler, eng_rsp_diag, DATA_BUF_SIZE, buf);
				} else {
					ret = send_isp_param(read_cmd, msg);
				}
				if (0x00 != ret)
					ISP_LOGE("fail to read param");
			}
		}
		break;
	case CMD_WRITE_ISP_PARAM_V1:
		{
			ISP_LOGV("CMD_WRITE_ISP_PARAM_V1 \n");
			struct isp_data_header_normal *write_cmd = NULL;
			cmr_u8 *isp_ptr = buf + sizeof(MSG_HEAD_T) + 1;
			cmr_u8 *isp_data_ptr = isp_ptr + sizeof(struct isp_data_header_normal);
			struct isp_check_cmd_valid cmd;
			memset(&cmd, 0, sizeof(struct isp_check_cmd_valid));

			write_cmd = (struct isp_data_header_normal *)isp_ptr;
			cmd.isp_mode = write_cmd->isp_mode;
			cmd.main_type = write_cmd->main_type;
			cmd.sub_type = write_cmd->sub_type;
			ret = check_cmd_valid(&cmd, msg);
			if (0 == ret) {
				if (0x02 > write_cmd->main_type) {
					ret = isp_nr_write(isp_handler, eng_rsp_diag, buf);
				} else {
					ret = down_isp_param(isp_handler, write_cmd, msg, isp_data_ptr, rsp_len);
				}
			}
		}
		break;
	case CMD_UPLOAD_MAIN_INFO:
		{
			ISP_LOGV("CMD_UPLOAD_MAIN_INFO \n");
			/* TODO:read isp param operation
			 * rlen is the size of isp_param
			 * pass eng_rsp_diag+rsp_len
			 */

			struct isp_parser_buf_in in_param = { 0x00, 0x00 };
			struct isp_parser_buf_rtn rtn_param = { 0x00, 0x00 };
			struct isp_parser_cmd_param rtn_cmd;
			cmr_u8 *dig_ptr = buf;
			cmr_u8 *isp_ptr = buf + sizeof(MSG_HEAD_T) + 1;

			memset(&rtn_cmd, 0, sizeof(rtn_cmd));
			in_param.buf_len = ispvideo_GetIspParamLenFromSt(dig_ptr);
			in_param.buf_addr = (cmr_uint) ispParserAlloc(in_param.buf_len);

			if ((0x00 != in_param.buf_len)
			    && (0x00 != in_param.buf_addr)) {
				ret = ispvideo_GetIspParamFromSt(isp_ptr, (struct isp_parser_buf_rtn *)&in_param);
				if (ret) {
					ISP_LOGE("fail to get param\n");
				}
				ret = ispParser(isp_handler, ISP_PARSER_DOWN, (void *)in_param.buf_addr, (void *)&rtn_cmd);
				if (ret) {
					ISP_LOGE("fail to parser param\n");
				}
				ret = ispParserFree((void *)in_param.buf_addr);
				if (ret) {
					ISP_LOGE("fail to free\n");
				}

				if (ISP_MAIN_INFO == rtn_cmd.cmd) {
					ret = ispParser(isp_handler, ISP_PARSER_UP_MAIN_INFO, (void *)in_param.buf_addr, (void *)&rtn_param);
					if (0x00 == ret) {
						isp_ptr = eng_rsp_diag + sizeof(MSG_HEAD_T) + 1;
						rlen = ispvideo_SetIspParamToSt(isp_ptr, (struct isp_parser_buf_in *)&rtn_param);
						ret = ispParserFree((void *)rtn_param.buf_addr);
					}
				}
			}

			rsp_len += rlen;
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len - 1;
			res = send(sockfd, eng_rsp_diag, rsp_len + 1, 0);
			break;
		}
	case CMD_TAKE_PICTURE:
		{
			ISP_LOGV("CMD_TAKE_PICTURE\n");
			struct isp_parser_buf_in in_param = { 0x00, 0x00 };
			struct isp_parser_cmd_param rtn_cmd;
			cmr_u8 *dig_ptr = buf;
			cmr_u8 *isp_ptr = buf + sizeof(MSG_HEAD_T) + 1;

			memset(&rtn_cmd, 0, sizeof(rtn_cmd));
			in_param.buf_len = ispvideo_GetIspParamLenFromSt(dig_ptr);
			in_param.buf_addr = (cmr_uint) ispParserAlloc(in_param.buf_len);

			if ((0x00 != in_param.buf_len)
			    && (0x00 != in_param.buf_addr)) {
				ret = ispvideo_GetIspParamFromSt(isp_ptr, (struct isp_parser_buf_rtn *)&in_param);
				if (ret) {
					ISP_LOGE("fail to check param\n");
				}
				ret = ispParser(isp_handler, ISP_PARSER_DOWN, (void *)in_param.buf_addr, (void *)&rtn_cmd);
				if (ret) {
					ISP_LOGE("fail to parse param\n");
				}
				ret = ispParserFree((void *)in_param.buf_addr);
				if (ret) {
					ISP_LOGE("fail to free\n");
				}

				if ((ISP_CAPTURE == rtn_cmd.cmd)
				    && (NULL != fun_ptr->take_picture)) {

					capture_img_end_flag = 0;
					capture_flag = 1;

					capture_format = rtn_cmd.param[0];

					if (NULL != fun_ptr->set_capture_size) {
						fun_ptr->set_capture_size(rtn_cmd.param[1], rtn_cmd.param[2]);
					}

					fun_ptr->take_picture(0, capture_format);
					sem_wait(&capture_sem_lock);

					if (NULL != fun_ptr->stop_preview) {
						fun_ptr->stop_preview(0, 0);
					}

					usleep(1000 * 10);

					if (NULL != fun_ptr->start_preview) {
						fun_ptr->start_preview(0, 0);
					}

				}
			}

			capture_flag = 0;
			break;
		}
	case CMD_ISP_LEVEL:
		{
			ISP_LOGV("CMD_ISP_LEVEL \n");
			struct isp_parser_buf_in in_param = { 0x00, 0x00 };
			struct isp_parser_cmd_param rtn_cmd;
			cmr_u8 *dig_ptr = buf;
			cmr_u8 *isp_ptr = buf + sizeof(MSG_HEAD_T) + 1;

			memset(&rtn_cmd, 0, sizeof(rtn_cmd));
			in_param.buf_len = ispvideo_GetIspParamLenFromSt(dig_ptr);
			in_param.buf_addr = (cmr_uint) ispParserAlloc(in_param.buf_len);

			if ((0x00 != in_param.buf_len)
			    && (0x00 != in_param.buf_addr)) {
				ret = ispvideo_GetIspParamFromSt(isp_ptr, (struct isp_parser_buf_rtn *)&in_param);
				if (ret) {
					ISP_LOGE("fail to get param\n");
				}
				ret = ispParser(isp_handler, ISP_PARSER_DOWN, (void *)in_param.buf_addr, (void *)&rtn_cmd);
				if (ret) {
					ISP_LOGE("fail to parser param \n");
				}
				ispParserFree((void *)in_param.buf_addr);
			}

			rsp_len += ispvideo_SetreTurnValue((cmr_u8 *) & eng_rsp_diag[rsp_len], ret);
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len - 1;
			res = send(sockfd, eng_rsp_diag, rsp_len + 1, 0);
			break;
		}

	case CMD_READ_SENSOR_REG:
		{
			ISP_LOGV("CMD_READ_SENSOR_REG \n");
			struct isp_parser_buf_in in_param = { 0x00, 0x00 };
			struct isp_parser_cmd_param rtn_cmd;
			struct isp_parser_buf_rtn rtn_param = { 0x00, 0x00 };
			cmr_u8 *dig_ptr = buf;
			cmr_u8 *isp_ptr = buf + sizeof(MSG_HEAD_T) + 1;

			memset(&rtn_cmd, 0, sizeof(rtn_cmd));
			in_param.buf_len = ispvideo_GetIspParamLenFromSt(dig_ptr);
			in_param.buf_addr = (cmr_uint) ispParserAlloc(in_param.buf_len);

			if ((0x00 != in_param.buf_len)
			    && (0x00 != in_param.buf_addr)) {
				ret = ispvideo_GetIspParamFromSt(isp_ptr, (struct isp_parser_buf_rtn *)&in_param);
				if (ret) {
					ISP_LOGE("fail to get param\n");
				}
				ret = ispParser(isp_handler, ISP_PARSER_DOWN, (void *)in_param.buf_addr, (void *)&rtn_cmd);
				if (ret) {
					ISP_LOGE("fail to parser param\n");
				}
				ret = ispParserFree((void *)in_param.buf_addr);
				if (ret) {
					ISP_LOGE("fail to free\n");
				}

				if (ISP_READ_SENSOR_REG == rtn_cmd.cmd) {
					ret = ispParser(isp_handler, ISP_READ_SENSOR_REG, (void *)&rtn_cmd, (void *)&rtn_param);
					if (0x00 == ret) {
						isp_ptr = eng_rsp_diag + sizeof(MSG_HEAD_T) + 1;
						rlen = ispvideo_SetIspParamToSt(isp_ptr, (struct isp_parser_buf_in *)&rtn_param);
						ret = ispParserFree((void *)rtn_param.buf_addr);
					}
				}
			}

			rsp_len += rlen;
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len - 1;
			res = send(sockfd, eng_rsp_diag, rsp_len + 1, 0);
			break;
		}
	case CMD_WRITE_SENSOR_REG:
		{
			ISP_LOGV("CMD_WRITE_SENSOR_REG \n");
			struct isp_parser_buf_in in_param = { 0x00, 0x00 };
			struct isp_parser_cmd_param rtn_cmd;
			cmr_u8 *dig_ptr = buf;
			cmr_u8 *isp_ptr = buf + sizeof(MSG_HEAD_T) + 1;

			memset(&rtn_cmd, 0, sizeof(rtn_cmd));
			in_param.buf_len = ispvideo_GetIspParamLenFromSt(dig_ptr);
			in_param.buf_addr = (cmr_uint) ispParserAlloc(in_param.buf_len);

			if ((0x00 != in_param.buf_len)
			    && (0x00 != in_param.buf_addr)) {
				ret = ispvideo_GetIspParamFromSt(isp_ptr, (struct isp_parser_buf_rtn *)&in_param);
				if (ret) {
					ISP_LOGE("fail to get param\n");
				}
				ret = ispParser(isp_handler, ISP_PARSER_DOWN, (void *)in_param.buf_addr, (void *)&rtn_cmd);
				if (ret) {
					ISP_LOGE("fail to parser param\n");
				}
				ret = ispParserFree((void *)in_param.buf_addr);
				if (ret) {
					ISP_LOGE("fail to free\n");
				}
			}

			rsp_len += ispvideo_SetreTurnValue((cmr_u8 *) & eng_rsp_diag[rsp_len], ISP_CMD_SUCCESS);
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len - 1;
			res = send(sockfd, eng_rsp_diag, rsp_len + 1, 0);
			break;
		}
	case CMD_GET_INFO:
		{
			ISP_LOGV("CMD_GET_INFO \n");
			/* TODO:read isp param operation
			 * rlen is the size of isp_param
			 * pass eng_rsp_diag+rsp_len
			 */

			struct isp_parser_buf_in in_param = { 0x00, 0x00 };
			struct isp_parser_buf_rtn rtn_param = { 0x00, 0x00 };
			struct isp_parser_cmd_param rtn_cmd;
			cmr_u8 *dig_ptr = buf;
			cmr_u8 *isp_ptr = buf + sizeof(MSG_HEAD_T) + 1;
			cmr_u32 *ptr = NULL;

			memset(&rtn_cmd, 0, sizeof(rtn_cmd));
			in_param.buf_len = ispvideo_GetIspParamLenFromSt(dig_ptr);
			in_param.buf_addr = (cmr_uint) ispParserAlloc(in_param.buf_len);

			if ((0x00 != in_param.buf_len)
			    && (0x00 != in_param.buf_addr)) {
				ret = ispvideo_GetIspParamFromSt(isp_ptr, (struct isp_parser_buf_rtn *)&in_param);
				if (ret) {
					ISP_LOGE("fail to get param\n");
				}
				ret = ispParser(isp_handler, ISP_PARSER_DOWN, (void *)in_param.buf_addr, (void *)&rtn_cmd);
				if (ret) {
					ISP_LOGE("fail to parser param\n");
				}
				ret = ispParserFree((void *)in_param.buf_addr);
				if (ret) {
					ISP_LOGE("fail to free\n");
				}

				ISP_LOGV("CMD_GET_INFO rtn cmd:%d \n", rtn_cmd.cmd);

				if (ISP_INFO == rtn_cmd.cmd) {
					ret = ispParser(isp_handler, ISP_PARSER_UP_INFO, (void *)&rtn_cmd, (void *)&rtn_param);
					if (0x00 == ret) {
						isp_ptr = eng_rsp_diag + sizeof(MSG_HEAD_T) + 1;
						rlen = ispvideo_SetIspParamToSt(isp_ptr, (struct isp_parser_buf_in *)&rtn_param);
						ptr = (cmr_u32 *) rtn_param.buf_addr;
						ret = ispParserFree((void *)rtn_param.buf_addr);
					}
				}
			}

			rsp_len += rlen;
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len - 1;
			res = send(sockfd, eng_rsp_diag, rsp_len + 1, 0);
			break;
		}
	case CMD_OTP_WRITE:
		{
			ISP_LOGV("ISP_OTP:CMD_OTP_WRITE \n");
			preview_tmpflag = preview_flag;
			len -= rsp_len;
			if (is_stop_preview && preview_tmpflag) {
				if (NULL != fun_ptr->stop_preview) {
					fun_ptr->stop_preview(0, 0);
				}
			}
			if (!ret) {
				memset(&eng_rsp_diag[rsp_len], 0, len);
			} else {
				memset(&eng_rsp_diag[rsp_len], 0, len);
				eng_rsp_diag[rsp_len] = 0x01;
			}
			rsp_len += len;
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len - 1;

			if (is_stop_preview && preview_tmpflag) {
				if (NULL != fun_ptr->start_preview) {
					fun_ptr->start_preview(0, 0);
				}
				preview_flag = preview_tmpflag;
			}

			res = send(sockfd, eng_rsp_diag, rsp_len + 1, 0);
			ISP_LOGV("ISP_OTP:CMD_OTP_WRITE done\n");
			break;
		}
	case CMD_OTP_READ:
		{
			ISP_LOGV("ISP_OTP:CMD_OTP_READ \n");
			preview_tmpflag = preview_flag;
			len -= rsp_len;
			if (is_stop_preview && preview_tmpflag) {
				if (NULL != fun_ptr->stop_preview) {
					fun_ptr->stop_preview(0, 0);
				}
			}

			memcpy(eng_rsp_diag, buf, (msg_ret->len + 1));
			ISP_LOGV("ISP_OTP:CMD_OTP_READ rsp_len %d len %d\n", rsp_len, len);
			rsp_len += len;
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len - 1;

			if (is_stop_preview && preview_tmpflag) {
				if (NULL != fun_ptr->start_preview) {
					fun_ptr->start_preview(0, 0);
				}
				preview_flag = preview_tmpflag;
			}
			res = send(sockfd, eng_rsp_diag, rsp_len + 1, 0);
			ISP_LOGV("ISP_OTP:CMD_OTP_READ  done\n");
			break;
		}
	case CMD_DOWNLOAD_RAW_PIC:
		{
			cmr_u32 img_len, pack_sn, total_pack, img_width, img_height, img_headlen;
			cmr_u8 *img_addr;
			img_len = ispvideo_GetImgDataLen(buf);
			img_addr = ispvideo_GetImgDataInfo(buf, &pack_sn, &total_pack, &img_width, &img_height, &img_headlen);
			if (NULL != img_addr) {

				if (1 == pack_sn) {
					scene_param.width = img_width;
					scene_param.height = img_height;

					bzero(raw_filename, sizeof(raw_filename));
					sprintf(raw_filename + 1, "/data/misc/cameraserver/%dX%d_gain_%d_awbgain_r_%d_g_%d_b_%d_ct_%d_bv_%d.mipi_raw",
						scene_param.width, scene_param.height, scene_param.gain, scene_param.awb_gain_r,
						scene_param.awb_gain_g, scene_param.awb_gain_b, scene_param.smart_ct, scene_param.smart_bv);

					ISP_LOGV("simulation raw filename %s", raw_filename + 1);
					raw_filename[0] = 1;
					raw_fp = fopen(raw_filename + 1, "wb+");
				}
				if (raw_fp) {
					fwrite((void *)img_addr, 1, img_len, raw_fp);
				}

				if (pack_sn == total_pack) {
					if (raw_fp) {
						fclose(raw_fp);
						raw_fp = NULL;
					}
					tool_fmt_pattern = (img_headlen >> 0x10) & 0xFFFF;
					ISP_LOGV("image pattern %d", tool_fmt_pattern);
					rsp_len += ispvideo_SetreTurnValue((cmr_u8 *) & eng_rsp_diag[rsp_len], ISP_CMD_SUCCESS);
					eng_rsp_diag[rsp_len] = 0x7e;
					msg_ret->len = rsp_len - 1;
					res = send(sockfd, eng_rsp_diag, rsp_len + 1, 0);
				}
			}
			break;
		}
	case CMD_WRTIE_SCENE_PARAM:
		{
			struct isptool_scene_param scene_info;
			ret = ispvideo_GetSceneInfo(buf, &scene_info);

			if (0x00 == ret) {
				ret = isp_ioctl(isp_handler, ISP_CTRL_TOOL_SET_SCENE_PARAM, (void *)&scene_info);
				if (ret) {
					ISP_LOGE("fail to do isp ioctl ret %d", ret);
				}
				memcpy(&(scene_param.gain), &(scene_info.gain), sizeof(struct isptool_scene_param) - 8);
				ISP_LOGV("width/height %d/%d, gain 0x%x, awb r/g/b  0x%x, 0x%x, 0x%x, ct 0x%x, bv 0x%x",
					 scene_param.width, scene_param.height, scene_param.gain, scene_param.awb_gain_r,
					 scene_param.awb_gain_g, scene_param.awb_gain_b, scene_param.smart_ct, scene_param.smart_bv);

				rsp_len += ispvideo_SetreTurnValue((cmr_u8 *) & eng_rsp_diag[rsp_len], ISP_CMD_SUCCESS);
			} else {
				rsp_len += ispvideo_SetreTurnValue((cmr_u8 *) & eng_rsp_diag[rsp_len], ISP_CMD_FAIL);
			}

			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len - 1;
			res = send(sockfd, eng_rsp_diag, rsp_len + 1, 0);
			break;
		}
	case CMD_START_SIMULATION:
		{
			ISP_LOGV("simulation start");
			if ((NULL != fun_ptr->take_picture)) {
				capture_img_end_flag = 0;
				capture_flag = 1;

				capture_format = 0x10;
				if (NULL != fun_ptr->set_capture_size) {
					fun_ptr->set_capture_size(scene_param.width, scene_param.height);
				}

				fun_ptr->take_picture(1, capture_format);
				sem_wait(&capture_sem_lock);
			}
			capture_flag = 0;
			if (NULL != fun_ptr->start_preview) {
				fun_ptr->start_preview(0, 0);
			}
			break;
		}
	default:
		break;
	}
	return EXIT_SUCCESS;
}

void ispvideo_Scale(cmr_u32 format, cmr_u32 in_w, cmr_u32 in_h, char *in_imgptr, cmr_s32 in_imglen, cmr_u32 * out_w, cmr_u32 * out_h, char **out_imgptr, cmr_s32 * out_imglen)
{
	cmr_u32 x = 0x00;
	cmr_u32 y = 0x00;
	cmr_u32 src_img_w = in_w;
	cmr_u32 src_img_h = in_h;
	cmr_u32 img_w = in_w;
	cmr_u32 img_h = in_h;
	cmr_u8 *src_buf_ptr = (cmr_u8 *) in_imgptr;
	cmr_u8 *dst_buf_ptr = (cmr_u8 *) * out_imgptr;
	cmr_u32 shift_num = 0x01;

	if ((PREVIEW_MAX_WIDTH >= img_w)
	    && (PREVIEW_MAX_HEIGHT >= img_h)) {
		*out_w = img_w;
		*out_h = img_h;
		*out_imgptr = in_imgptr;
		*out_imglen = in_imglen;
		return;
	}

	for (x = 0x00;; x++) {
		if ((PREVIEW_MAX_WIDTH >= img_w)
		    && (PREVIEW_MAX_HEIGHT >= img_h))
			break;
		img_w >>= 0x01;
		img_h >>= 0x01;
		shift_num <<= 0x01;
	}

	*out_w = img_w;
	*out_h = img_h;

	if ((ISP_TOOL_YUV420_2FRAME == format)
	    || (ISP_TOOL_YVU420_2FRAME == format)) {
		*out_imglen = img_w * img_h + (img_w * img_h) / 2;
		shift_num -= 0x01;
		shift_num <<= 0x01;

		for (y = 0x00; y < img_h; y += 0x02) {	/*y */
			for (x = 0x00; x < img_w; x += 0x02) {
				*dst_buf_ptr++ = *src_buf_ptr++;
				*dst_buf_ptr++ = *src_buf_ptr++;
				src_buf_ptr += shift_num;
			}
			for (x = 0x00; x < img_w; x += 0x02) {
				*dst_buf_ptr++ = *src_buf_ptr++;
				*dst_buf_ptr++ = *src_buf_ptr++;
				src_buf_ptr += shift_num;
			}
			src_buf_ptr += (src_img_w * shift_num);
		}

		src_buf_ptr = (cmr_u8 *) in_imgptr;
		dst_buf_ptr = (cmr_u8 *) * out_imgptr;
		src_buf_ptr = src_buf_ptr + src_img_w * src_img_h;
		dst_buf_ptr = dst_buf_ptr + img_w * img_h;

		img_h >>= 0x01;
		for (y = 0x00; y < img_h; y++) {	/*uv */
			for (x = 0x00; x < img_w; x += 0x02) {
				*dst_buf_ptr++ = *src_buf_ptr++;
				*dst_buf_ptr++ = *src_buf_ptr++;
				src_buf_ptr += shift_num;
			}
			src_buf_ptr += (src_img_w * (shift_num / 2));
		}

	}

}

void send_img_data(cmr_u32 format, cmr_u32 width, cmr_u32 height, char *imgptr, cmr_s32 imagelen)
{
	cmr_s32 ret;

	if (0 == preview_img_end_flag) {
		pthread_mutex_lock(&ispstream_lock);

		char *img_ptr = (char *)preview_buf_ptr;
		cmr_s32 img_len;
		cmr_u32 img_w;
		cmr_u32 img_h;
		/* if preview size more than vga that is subsample to less than vga for preview frame ratio */
		ispvideo_Scale(format, width, height, imgptr, imagelen, &img_w, &img_h, &img_ptr, &img_len);

		ret = handle_img_data(format, img_w, img_h, img_ptr, img_len, 0, 0, 0, 0);

		sem_post(&preview_sem_lock);
		if (ret != 0) {
			ISP_LOGE("fail to handle data ret = %d.", ret);
		}

		pthread_mutex_unlock(&ispstream_lock);
	}
}

void send_capture_complete_msg()
{
	if ((capture_flag == 1) && (1 == capture_img_end_flag)) {
		sem_post(&capture_sem_lock);
		capture_flag = 0;
	}
}

void send_capture_data(cmr_u32 format, cmr_u32 width, cmr_u32 height, char *ch0_ptr, cmr_s32 ch0_len, char *ch1_ptr, cmr_s32 ch1_len, char *ch2_ptr, cmr_s32 ch2_len)
{
	cmr_s32 ret;

	if ((0 == capture_img_end_flag) && (format == (cmr_u32) capture_format)) {
		pthread_mutex_lock(&ispstream_lock);

		ISP_LOGV(" capture format: %d, width: %d, height: %d.\n", format, width, height);
		switch (g_command) {
		case CMD_ASIC_TAKE_PICTURE:
		case CMD_ASIC_TAKE_PICTURE_NEW:
			ret = handle_img_data_asic(format, width, height, ch0_ptr, ch0_len, ch1_ptr, ch1_len, ch2_ptr, ch2_len);
			break;
		default:
			ret = handle_img_data(format, width, height, ch0_ptr, ch0_len, ch1_ptr, ch1_len, ch2_ptr, ch2_len);
			break;
		}
		capture_img_end_flag = 1;
		if (ret != 0) {
			ISP_LOGE("fail to handle data ret = %d.", ret);
		}

		pthread_mutex_unlock(&ispstream_lock);
	}
}

cmr_s32 isp_RecDataCheck(cmr_u8 * rx_buf_ptr, cmr_s32 rx_bug_len, cmr_u8 * cmd_buf_ptr, cmr_s32 * cmd_len)
{
	cmr_s32 rtn;
	cmr_u8 packet_header = rx_buf_ptr[0];
	cmr_u16 packet_len = (rx_buf_ptr[6] << 0x08) | (rx_buf_ptr[5]);
	cmr_u8 packet_end = rx_buf_ptr[rx_bug_len - 1];
	cmr_s32 cmd_buf_offset = rx_packet_len;

	rtn = 0;
	*cmd_len = rx_bug_len;

	if ((0x7e == packet_header)
	    && (0x7e == packet_end) && (packet_len + 2 == rx_bug_len)) {
		/* one packet */

	} else {		/* mul packet */
		rx_packet_len += rx_bug_len;

		if ((0x7e == packet_end)
		    && (rx_packet_len == rx_packet_total_len)) {
			*cmd_len = rx_packet_len;
		} else {
			rtn = 1;
		}
	}

	memcpy((void *)&cmd_buf_ptr[cmd_buf_offset], rx_buf_ptr, rx_bug_len);

	if (0 == rtn) {
		rx_packet_len = 0x00;
		rx_packet_total_len = 0x00;
	}

	return rtn;
}

static void *isp_diag_handler(void *args)
{
	cmr_s32 from = *((cmr_s32 *) args);
	cmr_s32 cnt, res, cmd_len, rtn, last_len;
	static char *code = "diag channel exit";
	fd_set rfds;
	struct timeval tv;

	FD_ZERO(&rfds);
	FD_SET(from, &rfds);

	sockfd = from;
	rx_packet_len = 0x00;
	rx_packet_total_len = 0x00;
	bzero(nr_tool_flag, sizeof(nr_tool_flag));

	/* Read client request, send sequence number back */
	while (wire_connected) {

		/* Wait up to  two seconds. */
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		FD_SET(from, &rfds);
		res = select(from + 1, &rfds, NULL, NULL, &tv);
		if (res <= 0) {	/* timeout or other error */
			ISP_LOGE("fail to select res:%d\n", res);
			continue;
		}

		cnt = recv(from, diag_rx_buf, CMD_BUF_SIZE, MSG_DONTWAIT);
		if (cnt <= 0) {
			ISP_LOGE("fail to recv %s\n", strerror(errno));
			break;
		}

		do {
			if ((0x7e == diag_rx_buf[0])
			    && (0x00 == rx_packet_total_len)
			    && (0x00 == rx_packet_len)) {
				rx_packet_total_len = ((diag_rx_buf[6] << 0x08) | diag_rx_buf[5]) + 2;
			}

			if (cnt + rx_packet_len > rx_packet_total_len) {
				last_len = rx_packet_total_len - rx_packet_len;
				cnt = cnt - last_len;
				memcpy(temp_rx_buf, &diag_rx_buf[last_len], cnt);
				rtn = isp_RecDataCheck(diag_rx_buf, last_len, diag_cmd_buf, &cmd_len);
				if (0 == rtn) {
					handle_isp_data(diag_cmd_buf, cmd_len);
				} else {
					ISP_LOGV("mul packet and not end \n");
				}
				memcpy(diag_rx_buf, temp_rx_buf, cnt);
			} else {
				rtn = isp_RecDataCheck(diag_rx_buf, cnt, diag_cmd_buf, &cmd_len);
				if (0 == rtn) {
					handle_isp_data(diag_cmd_buf, cmd_len);
				} else {
					ISP_LOGV("mul packet and not end \n");
				}
				cnt = 0;
			}
		} while (cnt > 0);
	}

	bzero(raw_filename, sizeof(raw_filename));
	if (raw_fp) {
		fclose(raw_fp);
		raw_fp = NULL;
	}
	ISP_LOGV("exit %s\n", strerror(errno));
	return code;
}

static void *ispserver_thread(void *args)
{
	UNUSED(args);
	struct sockaddr claddr;
	cmr_s32 lfd, cfd, optval;
	struct sockaddr_in sock_addr;
	socklen_t addrlen;
#ifdef CLIENT_DEBUG
#define ADDRSTRLEN (128)
	char addrStr[ADDRSTRLEN];
	char host[50];
	char service[30];
#endif
	pthread_t tdiag;
	pthread_attr_t attr;

	ISP_LOGV("isp-video server version 1.0\n");

	memset(&sock_addr, 0, sizeof(struct sockaddr_in));
	sock_addr.sin_family = AF_INET;	/* Allows IPv4 */
	sock_addr.sin_addr.s_addr = INADDR_ANY;	/* Wildcard IP address; */
	sock_addr.sin_port = htons(PORT_NUM);

	lfd = socket(sock_addr.sin_family, SOCK_STREAM, 0);
	if (lfd == -1) {
		ISP_LOGE("fail to socket.\n");
		return NULL;
	}
	sock_fd = lfd;
	optval = 1;
	if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) != 0) {
		ISP_LOGE("fail to setsockope\n");
		return NULL;
	}

	if (bind(lfd, (struct sockaddr *)&sock_addr, sizeof(struct sockaddr_in)) != 0) {
		ISP_LOGE("fail to bind error %s\n", strerror(errno));
		return NULL;
	}

	if (listen(lfd, BACKLOG) == -1) {
		ISP_LOGE("fail to listen\n");
		return NULL;
	}

	sem_init(&preview_sem_lock, 0, 0);
	sem_init(&capture_sem_lock, 0, 0);
	pthread_mutex_init(&ispstream_lock, NULL);

	pthread_attr_init(&attr);
	for (;;) {		/* Handle clients iteratively */
		void *res;
		cmr_s32 ret;

		ISP_LOGV("log server waiting client dail in...\n");
		/* Accept a client connection, obtaining client's address */
		wire_connected = 0;
		addrlen = sizeof(struct sockaddr);
		cfd = accept(lfd, &claddr, &addrlen);
		if (cfd == -1) {
			ISP_LOGE("fail to accept %s\n", strerror(errno));
			break;
		}
		ISP_LOGV("log server connected with client\n");
		wire_connected = 1;
		sequence_num = 0;
		/* Ignore the SIGPIPE signal, so that we find out about broken
		 * connection errors via a failure from write().
		 */
		if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
			ISP_LOGE("fail to signal\n");
#ifdef CLIENT_DEBUG
		addrlen = sizeof(struct sockaddr);
		if (getnameinfo(&claddr, addrlen, host, 50, service, 30, NI_NUMERICHOST) == 0)
			snprintf(addrStr, ADDRSTRLEN, "(%s, %s)", host, service);
		else
			snprintf(addrStr, ADDRSTRLEN, "(?UNKNOWN?)");
		ISP_LOGV("Connection from %s\n", addrStr);
#endif

		/* create a thread for recv cmd */
		ret = pthread_create(&tdiag, &attr, isp_diag_handler, &cfd);
		if (ret != 0) {
			ISP_LOGV("diag thread create success\n");
			break;
		}
		pthread_setname_np(tdiag, "diag");

		pthread_join(tdiag, &res);
		ISP_LOGV("diag thread exit success %s\n", (char *)res);
		if (close(cfd) == -1)	/* Close connection */
			ISP_LOGE("fail to close file\n");
	}
	pthread_attr_destroy(&attr);
	if (close(lfd) == -1)	/* Close connection */
		ISP_LOGE("fail to close\n");

	sock_fd = 0x00;

	pthread_mutex_destroy(&ispstream_lock);

	return NULL;
}

cmr_s32 ispvideo_RegCameraFunc(cmr_u32 cmd, cmr_s32(*func) (cmr_u32, cmr_u32))
{
	struct camera_func *fun_ptr = ispvideo_GetCameraFunc();

	switch (cmd) {
	case REG_START_PREVIEW:
		{
			fun_ptr->start_preview = func;
			break;
		}
	case REG_STOP_PREVIEW:
		{
			fun_ptr->stop_preview = func;
			break;
		}
	case REG_TAKE_PICTURE:
		{
			fun_ptr->take_picture = func;
			break;
		}
	case REG_SET_PARAM:
		{
			fun_ptr->set_capture_size = func;
			break;
		}
	default:
		{
			break;
		}
	}

	return 0x00;
}

void stopispserver()
{
	ISP_LOGV("stopispserver\n");
	wire_connected = 0;

	if ((1 == preview_flag) && (0 == preview_img_end_flag)) {
		sem_post(&preview_sem_lock);
	}
#ifndef MINICAMERA
	if (0x00 != sock_fd) {
		shutdown(sock_fd, 0);
		shutdown(sock_fd, 1);
	}

	ispParserFree((void *)preview_buf_ptr);

	preview_flag = 0;
	capture_flag = 0;
	preview_img_end_flag = 1;
	capture_img_end_flag = 1;
	preview_buf_ptr = 0;
#endif
}

void startispserver(cmr_u32 cam_id)
{
	pthread_t tdiag;
	pthread_attr_t attr;
#ifdef MINICAMERA
	static cmr_s32 ret = -1;
#else
	cmr_s32 ret = -1;
#endif
	UNUSED(cam_id);

	ISP_LOGV("startispserver\n");

	preview_flag = 0;
	capture_flag = 0;
	preview_img_end_flag = 1;
	capture_img_end_flag = 1;

	if (ret != 0) {
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		ret = pthread_create(&tdiag, &attr, ispserver_thread, NULL);
		pthread_setname_np(tdiag, "ispserver");
		pthread_attr_destroy(&attr);
		if (ret < 0) {
			ISP_LOGE("fail to creat pthread\n");
			return;
		}
		preview_buf_ptr = (cmr_u8 *) ispParserAlloc(PREVIEW_MAX_WIDTH * PREVIEW_MAX_HEIGHT * 2);
	} else {
		ISP_LOGV("pthread already create now!\n");
	}
}

void setispserver(void *handle)
{
	isp_handler = handle;
	isp_ioctl(handle, ISP_CTRL_DENOISE_PARAM_READ, (void *)&nr_update_param);
	ISP_LOGV("setispserver %p\n", handle);
}
