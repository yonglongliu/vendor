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

#define LOG_TAG "isp_para_tune_com"
#include "isp_param_tune_com.h"
#include "isp_param_size.h"
#include "isp_app.h"
#include "isp_video.h"

extern void *sensor_get_dev_cxt(void);
static cmr_s32 _ispParamVerify(void *in_param_ptr)
{
	cmr_s32 rtn = 0x00;
	cmr_u32 *param_ptr = (cmr_u32 *) in_param_ptr;
	cmr_u32 verify_id = param_ptr[0];
	cmr_u32 packet_size = param_ptr[2];
	cmr_u32 end_id = param_ptr[(packet_size / 4) - 1];

	if (ISP_PACKET_VERIFY_ID != verify_id) {
		rtn = 0x01;
	}

	if (ISP_PACKET_END_ID != end_id) {
		rtn = 0x01;
	}

	return rtn;
}

static cmr_s32 _ispParserDownParam(cmr_handle isp_handler, void *in_param_ptr)
{
	cmr_u32 rtn = 0x00;
	cmr_u32 *param_ptr = (cmr_u32 *) ((cmr_u8 *) in_param_ptr + 0x0c);	// packet data version_id||module_id||packet_len||module_info||data
	cmr_u32 version_id = param_ptr[0];
	cmr_u32 module_id = param_ptr[1];
	cmr_u32 packet_len = param_ptr[2];
	void *data_addr = (void *)&param_ptr[4];
	cmr_u32 data_len = packet_len - 0x10;
	cmr_u32 mode_offset = 0;
	SENSOR_EXP_INFO_T *sensor_info_ptr = Sensor_GetInfo();
	ISP_LOGV("ver %d, module id %d", version_id, module_id);

	while (mode_offset < data_len) {
		struct isp_mode_param *mode_param_ptr = (struct isp_mode_param *)((char *)data_addr + mode_offset);
		ISP_LOGV(" _ispParserDownParam mode_param_ptr->mode_id =%d", mode_param_ptr->mode_id);

		if (0 != mode_param_ptr->size) {
			memcpy(sensor_info_ptr->raw_info_ptr->mode_ptr[mode_param_ptr->mode_id].addr, (char *)data_addr + mode_offset, mode_param_ptr->size);
			sensor_info_ptr->raw_info_ptr->mode_ptr[mode_param_ptr->mode_id].len = mode_param_ptr->size;
			mode_offset += mode_param_ptr->size;
		}
	}

	rtn = isp_ioctl(isp_handler, ISP_CTRL_PARAM_UPDATE | ISP_TOOL_CMD_ID, data_addr);

	return rtn;
}

static cmr_s32 _ispParserDownLevel(cmr_handle isp_handler, void *in_param_ptr)
{
	cmr_s32 rtn = 0x00;
	cmr_u32 *param_ptr = (cmr_u32 *) ((cmr_u8 *) in_param_ptr + 0x0c);	// packet data
	cmr_u32 module_id = param_ptr[0];
	enum isp_ctrl_cmd cmd = ISP_CTRL_MAX;
	void *ioctl_param_ptr = NULL;
	struct isp_af_win af_param;

	cmd = module_id;

	if (ISP_CTRL_AF == cmd) {
		SENSOR_EXP_INFO_T_PTR sensor_info_ptr = Sensor_GetInfo();
		struct isp_af_win *in_af_ptr = (struct isp_af_win *)&param_ptr[3];
		cmr_u16 img_width = (param_ptr[2] >> 0x10) & 0xffff;
		cmr_u16 img_height = param_ptr[2] & 0xffff;
		cmr_u16 prv_width = sensor_info_ptr->sensor_mode_info[1].width;
		cmr_u16 prv_height = sensor_info_ptr->sensor_mode_info[1].height;
		cmr_u32 i = 0x00;

		ISP_LOGV("zone prv_width=%d prv_height=%d", prv_width, prv_height);

		af_param.valid_win = in_af_ptr->valid_win;
		af_param.mode = in_af_ptr->mode;
		for (i = 0x00; i < af_param.valid_win; i++) {
			af_param.win[i].start_x = (in_af_ptr->win[i].start_x * prv_width) / img_width;
			af_param.win[i].start_y = (in_af_ptr->win[i].start_y * prv_height) / img_height;
			af_param.win[i].end_x = (in_af_ptr->win[i].end_x * prv_width) / img_width;
			af_param.win[i].end_y = (in_af_ptr->win[i].end_y * prv_height) / img_height;
		}
		ioctl_param_ptr = (void *)&af_param;
	} else {
		ioctl_param_ptr = (void *)&param_ptr[2];
	}

	cmd |= ISP_TOOL_CMD_ID;

	rtn = isp_ioctl(isp_handler, cmd, ioctl_param_ptr);

	return rtn;
}

static cmr_s32 _ispParserUpMainInfo(void *rtn_param_ptr)
{
	cmr_u32 rtn = 0x00;
	SENSOR_EXP_INFO_T_PTR sensor_info_ptr = Sensor_GetInfo();
	struct isp_parser_buf_rtn *rtn_ptr = (struct isp_parser_buf_rtn *)rtn_param_ptr;
	cmr_u32 i = 0x00;
	cmr_u32 *data_addr = NULL;
	cmr_u32 data_len = 0x10;
	struct isp_main_info *param_ptr = NULL;
	struct sensor_version_info *temp_param_version = NULL;

	data_len = sizeof(struct isp_main_info);
	data_addr = ispParserAlloc(data_len);
	if (!data_addr) {
		ISP_LOGE("fail to do ispParserAlloc");
		return -1;
	}

	temp_param_version = (struct sensor_version_info *)ispParserAlloc(ISP_PARASER_VERSION_INFO_SIZE);
	if (!temp_param_version) {
		ISP_LOGE("fail to check temp_param_version");
		ispParserFree(data_addr);
		return -1;
	}

	param_ptr = (struct isp_main_info *)data_addr;

	memset((void *)data_addr, 0x00, sizeof(struct isp_main_info));

	rtn_ptr->buf_addr = (cmr_uint) data_addr;
	rtn_ptr->buf_len = sizeof(struct isp_main_info);

	if ((NULL != sensor_info_ptr)) {
		/*get sensor name */
		if (NULL != sensor_info_ptr->raw_info_ptr) {
			param_ptr->version_id = sensor_info_ptr->raw_info_ptr->version_info->version_id;
			strcpy((char *)&param_ptr->sensor_id, (char *)&sensor_info_ptr->raw_info_ptr->version_info->sensor_ver_name.sensor_name);
		} else {
			param_ptr->version_id = TOOL_DEFAULT_VER;
			memset((char *)&param_ptr->sensor_id, 0, sizeof(param_ptr->sensor_id));
			ispParserFree(temp_param_version);
			return -1;
		}

		/* get version info: version_info[0] is tune version; version_info[1] is code version */
		memcpy(param_ptr->version_info[0], (char *)sensor_info_ptr->raw_info_ptr->version_info, ISP_PARASER_VERSION_INFO_SIZE);
		temp_param_version->ae_struct_version = AE_VERSION;
		temp_param_version->awb_struct_version = AWB_VERSION;
		temp_param_version->lnc_struct_version = LNC_VERSION;
		temp_param_version->nr_struct_version = NR_VERSION;
		memcpy(param_ptr->version_info[1], (char *)temp_param_version, ISP_PARASER_VERSION_INFO_SIZE);

		/*set preview param */
		param_ptr->preview_size = (sensor_info_ptr->sensor_mode_info[1].width << 0x10) & 0xffff0000;
		param_ptr->preview_size |= sensor_info_ptr->sensor_mode_info[1].height & 0xffff;

		for (i = SENSOR_MODE_PREVIEW_ONE; i < SENSOR_MODE_MAX; i++) {
			if ((0x00 != sensor_info_ptr->sensor_mode_info[i].width)
			    && (0x00 != sensor_info_ptr->sensor_mode_info[i].height)) {
				param_ptr->capture_size[param_ptr->capture_num] = (sensor_info_ptr->sensor_mode_info[i].width << 0x10) & 0xffff0000;
				param_ptr->capture_size[param_ptr->capture_num] |= sensor_info_ptr->sensor_mode_info[i].height & 0xffff;
				param_ptr->capture_num++;
			} else {
				break;
			}
		}
		/* new format hsb no equal to zero, */
		param_ptr->capture_num |= 0x00010000;

		param_ptr->preview_format = ISP_VIDEO_YVU420_2FRAME;
		param_ptr->capture_format = ISP_VIDEO_YVU420_2FRAME | ISP_VIDEO_JPG;

		if (NULL != sensor_info_ptr->raw_info_ptr) {
			if (0x00 == sensor_info_ptr->sensor_interface.is_loose) {
				param_ptr->capture_format |= ISP_VIDEO_MIPI_RAW10;
			} else {
				param_ptr->capture_format |= ISP_VIDEO_NORMAL_RAW10;
			}
		} else {
			strcat((char *)&param_ptr->sensor_id, "--YUV");
		}
	} else {
		strcpy((char *)&param_ptr->sensor_id, "no sensor identified");
		param_ptr->version_id = TOOL_DEFAULT_VER;
	}

	ispParserFree(temp_param_version);

	return rtn;
}

static cmr_s32 _ispParserUpParam(void *rtn_param_ptr)
{
	cmr_s32 rtn = 0x00;
	cmr_s32 i = 0;
	SENSOR_EXP_INFO_T_PTR sensor_info_ptr = Sensor_GetInfo();
	cmr_u32 version_id = sensor_info_ptr->raw_info_ptr->version_info->version_id;
	struct sensor_raw_info *sensor_raw_info_ptr = (struct sensor_raw_info *)sensor_info_ptr->raw_info_ptr;
	struct isp_parser_buf_rtn *rtn_ptr = (struct isp_parser_buf_rtn *)rtn_param_ptr;
	cmr_u32 *data_addr = NULL;
	cmr_u32 data_len = 0;
	cmr_u32 data_offset = 0;

	for (i = 0; i < MAX_MODE_NUM; i++) {
		data_len += sensor_raw_info_ptr->mode_ptr[i].len;
	}

	rtn_ptr->buf_len = data_len + 0x10;
	data_addr = ispParserAlloc(rtn_ptr->buf_len);
	rtn_ptr->buf_addr = (cmr_uint) data_addr;

	if (NULL != data_addr) {
		// packet cfg version_id||module_id||packet_len||module_info||data
		data_addr[0] = version_id;
		data_addr[1] = ISP_PACKET_ALL;
		data_addr[2] = data_len;
		data_addr[3] = 0x00;

		for (i = 0; i < MAX_MODE_NUM; i++) {
			if (NULL != sensor_raw_info_ptr->mode_ptr[i].addr) {
				memcpy((char *)&data_addr[4] + data_offset, (char *)sensor_raw_info_ptr->mode_ptr[i].addr, sensor_raw_info_ptr->mode_ptr[i].len);
				data_offset += sensor_raw_info_ptr->mode_ptr[i].len;
				ISP_LOGV("ISP_TOOL:_sensor_raw_info_ptr->mode_ptr[i].len: %d   i=  %d\n", sensor_raw_info_ptr->mode_ptr[i].len, i);

			}
		}
		ISP_LOGV("ISP_TOOL:_ispParserUpParam data_offset: %d\n", data_offset);

	} else {
		rtn_ptr->buf_addr = (cmr_uint) NULL;
		rtn_ptr->buf_len = 0x00;
	}

	return rtn;
}

static cmr_s32 _ispParserUpdata(void *in_param_ptr, void *rtn_param_ptr)
{
	cmr_s32 rtn = 0x00;
	struct isp_parser_up_data *in_ptr = (struct isp_parser_up_data *)in_param_ptr;
	struct isp_parser_buf_rtn *rtn_ptr = (struct isp_parser_buf_rtn *)rtn_param_ptr;
	cmr_u32 *data_addr = NULL;
	cmr_u32 data_len = 0x10;

	data_len += in_ptr->buf_len;
	data_addr = ispParserAlloc(data_len);

	if (NULL != data_addr) {
		data_addr[0] = data_len;
		data_addr[1] = in_ptr->format;
		data_addr[2] = in_ptr->pattern;
		data_addr[3] = in_ptr->width;
		data_addr[4] = in_ptr->height;

		memcpy((void *)&data_addr[5], (void *)in_ptr->buf_addr, in_ptr->buf_len);

		rtn_ptr->buf_addr = (cmr_uint) data_addr;
		rtn_ptr->buf_len = data_len;
	}

	return rtn;
}

static cmr_s32 _ispParserCapturesize(void *in_param_ptr, void *rtn_param_ptr)
{
	cmr_s32 rtn = 0x00;
	struct isp_parser_cmd_param *in_ptr = (struct isp_parser_cmd_param *)in_param_ptr;
	struct isp_parser_buf_rtn *rtn_ptr = (struct isp_parser_buf_rtn *)rtn_param_ptr;
	cmr_u32 *data_addr = NULL;
	cmr_u32 data_len = 0x0c;

	data_addr = ispParserAlloc(data_len);

	if (NULL != data_addr) {
		data_addr[0] = in_ptr->cmd;
		data_addr[1] = data_len;
		data_addr[2] = in_ptr->param[0];

		rtn_ptr->buf_addr = (cmr_uint) data_addr;
		rtn_ptr->buf_len = data_len;
	}

	return rtn;
}

static cmr_s32 _ispParserReadSensorReg(void *in_param_ptr, void *rtn_param_ptr)
{
	cmr_s32 rtn = 0x00;
	struct isp_parser_cmd_param *in_ptr = (struct isp_parser_cmd_param *)in_param_ptr;
	struct isp_parser_buf_rtn *rtn_ptr = (struct isp_parser_buf_rtn *)rtn_param_ptr;
	cmr_u32 *data_addr = NULL;
	cmr_u32 data_len = 0x0c;
	cmr_u32 reg_num = in_ptr->param[0];
	cmr_u16 reg_addr = (cmr_u16) in_ptr->param[1];
	cmr_u16 reg_data = 0x00;
	cmr_u32 i = 0x00;
	struct sensor_drv_context *current_sensor_cxt = (struct sensor_drv_context *)sensor_get_dev_cxt();

	rtn_ptr->buf_addr = (cmr_uint) NULL;
	rtn_ptr->buf_len = 0x00;

	data_len += reg_num * 0x08;
	data_addr = ispParserAlloc(data_len);

	if (NULL != data_addr) {
		data_addr[0] = in_ptr->cmd;
		data_addr[1] = data_len;
		data_addr[2] = reg_num;

		for (i = 0x00; i < reg_num; i++) {
			reg_data = hw_Sensor_ReadReg(current_sensor_cxt->sensor_hw_handler, reg_addr);
			data_addr[3 + i * 2] = (cmr_u32) reg_addr;
			data_addr[4 + i * 2] = (cmr_u32) reg_data;
			reg_addr++;
		}

		rtn_ptr->buf_addr = (cmr_uint) data_addr;
		rtn_ptr->buf_len = data_len;
	}

	return rtn;
}

static cmr_s32 _ispParserWriteSensorReg(void *in_param_ptr)
{
	cmr_s32 rtn = 0x00;
	struct isp_parser_cmd_param *in_ptr = (struct isp_parser_cmd_param *)in_param_ptr;
	cmr_u32 reg_num = in_ptr->param[1];
	cmr_u16 reg_addr = 0x00;
	cmr_u16 reg_data = 0x00;
	cmr_u32 i = 0x00;
	struct sensor_drv_context *current_sensor_cxt = (struct sensor_drv_context *)sensor_get_dev_cxt();

	for (i = 0x00; i < reg_num; i++) {
		reg_addr = (cmr_u16) in_ptr->param[2 + i * 2];
		reg_data = (cmr_u16) in_ptr->param[3 + i * 2];
		hw_Sensor_WriteReg(current_sensor_cxt->sensor_hw_handler, reg_addr, reg_data);
	}

	return rtn;
}

static cmr_s32 _ispParserGetInfo(cmr_handle isp_handler, void *in_param_ptr, void *rtn_param_ptr)
{
	cmr_s32 rtn = 0x00;
	cmr_u32 *param_ptr = (cmr_u32 *) in_param_ptr;
	struct isp_parser_buf_rtn *rtn_ptr = (struct isp_parser_buf_rtn *)rtn_param_ptr;
	enum isp_ctrl_cmd cmd = param_ptr[1];
	void *ioctl_param_ptr = NULL;
	cmr_u32 *data_addr = NULL;
	cmr_u32 data_len = 0x14 + param_ptr[2];

	rtn_ptr->buf_addr = 0;
	rtn_ptr->buf_len = 0x00;

	ISP_LOGV("ISP_TOOL:_ispParserGetInfo %d\n", (cmr_u32) cmd);

	data_addr = ispParserAlloc(data_len);

	if (NULL != data_addr) {
		data_addr[0] = data_len;
		data_addr[1] = 0x14;
		data_addr[2] = cmd;
		data_addr[3] = 0x00;
		data_addr[4] = 0x00;
		ioctl_param_ptr = (void *)&data_addr[5];
		memcpy((void *)&data_addr[5], (void *)&param_ptr[3], param_ptr[2]);
	}

	cmd |= ISP_TOOL_CMD_ID;

	rtn = isp_ioctl(isp_handler, cmd, ioctl_param_ptr);

	rtn_ptr->buf_addr = (cmr_uint) data_addr;
	rtn_ptr->buf_len = data_len;

	return rtn;
}

static cmr_s32 _ispParserDownCmd(void *in_param_ptr, void *rtn_param_ptr)
{
	cmr_s32 rtn = 0x00;
	cmr_u32 *param_ptr = (cmr_u32 *) in_param_ptr + 0x03;
	struct isp_parser_cmd_param *rtn_ptr = (struct isp_parser_cmd_param *)rtn_param_ptr;
	cmr_u32 cmd = param_ptr[0];
	cmr_u32 i = 0x00;
	struct isp_size_info *size_info_ptr = ISP_ParamGetSizeInfo();

	rtn_ptr->cmd = cmd;

	ISP_LOGV("ISP_TOOL:_ispParserDownCmd type: 0x%x, 0x%x, 0x%x\n", param_ptr[0], param_ptr[1], param_ptr[2]);

	switch (cmd) {
	case ISP_CAPTURE:
		{
			rtn_ptr->param[0] = param_ptr[2];	/*format */

			if (0x00 != (param_ptr[3] & 0xffff0000)) {
				rtn_ptr->param[1] = (param_ptr[3] >> 0x10) & 0xffff;	/*width */
				rtn_ptr->param[2] = param_ptr[3] & 0xffff;	/*height */
			} else {
				for (i = 0x00; ISP_SIZE_END != size_info_ptr[i].size_id; i++) {
					if (size_info_ptr[i].size_id == param_ptr[3]) {
						rtn_ptr->param[1] = size_info_ptr[i].width;	//width
						rtn_ptr->param[2] = size_info_ptr[i].height;	//height
						break;
					}
				}
			}
			break;
		}
	case ISP_READ_SENSOR_REG:
		{
			rtn_ptr->param[0] = param_ptr[2];	//addr num
			rtn_ptr->param[1] = param_ptr[3];	//start addr
			break;
		}
	case ISP_WRITE_SENSOR_REG:
		{
			_ispParserWriteSensorReg((void *)param_ptr);
			break;
		}
	case ISP_PREVIEW:
	case ISP_STOP_PREVIEW:
	case ISP_UP_PREVIEW_DATA:
	case ISP_UP_PARAM:
	case ISP_TAKE_PICTURE_SIZE:
	case ISP_MAIN_INFO:
		{
			break;
		}
	case ISP_INFO:
		{
			rtn_ptr->param[0] = param_ptr[2];	//thrd cmd
			memcpy((void *)&rtn_ptr->param[1], (void *)&param_ptr[3], param_ptr[3] + 0x04);
			ISP_LOGV("ISP_TOOL:_ispParserDownCmd thrd cmd: 0x%x\n", rtn_ptr->param[0]);
			break;
		}
	default:
		{
			break;
		}
	}

	return rtn;
}

static cmr_s32 _ispParserDownHandle(cmr_handle isp_handler, void *in_param_ptr, void *rtn_param_ptr)
{
	cmr_s32 rtn = 0x00;
	cmr_u32 *param_ptr = (cmr_u32 *) in_param_ptr;	//packet data
	cmr_u32 type = param_ptr[1];

	rtn = _ispParamVerify(in_param_ptr);

	ISP_LOGV("ISP_TOOL:_ispParserDownHandle param: 0x%x, 0x%x, 0x%x\n", param_ptr[0], param_ptr[1], param_ptr[2]);

	ISP_LOGV("ISP_TOOL:_ispParserDownHandle type: 0x%x\n", type);

	switch (type) {
	case ISP_TYPE_CMD:
		{
			rtn = _ispParserDownCmd(in_param_ptr, rtn_param_ptr);
			break;
		}
	case ISP_TYPE_PARAM:
		{
			rtn = _ispParserDownParam(isp_handler, in_param_ptr);
			break;
		}
	case ISP_TYPE_LEVEL:
		{
			rtn = _ispParserDownLevel(isp_handler, in_param_ptr);
			break;
		}
	default:
		{
			break;
		}
	}

	return rtn;
}

static cmr_s32 _ispParserUpHandle(cmr_handle isp_handler, cmr_u32 cmd, void *in_param_ptr, void *rtn_param_ptr)
{
	cmr_s32 rtn = 0x00;
	struct isp_parser_buf_rtn *rtn_ptr = (struct isp_parser_buf_rtn *)rtn_param_ptr;
	cmr_u32 *data_addr = NULL;
	cmr_u32 data_len = 0x10;

	ISP_LOGV("ISP_TOOL:_ispParserUpHandle %d\n", cmd);

	switch (cmd) {
	case ISP_PARSER_UP_MAIN_INFO:
		{
			rtn = _ispParserUpMainInfo(rtn_param_ptr);
			break;
		}
	case ISP_PARSER_UP_PARAM:
		{
			rtn = _ispParserUpParam(rtn_param_ptr);
			break;
		}
	case ISP_PARSER_UP_PRV_DATA:
	case ISP_PARSER_UP_CAP_DATA:
		{
			rtn = _ispParserUpdata(in_param_ptr, rtn_param_ptr);
			break;
		}
	case ISP_PARSER_UP_CAP_SIZE:
		{
			rtn = _ispParserCapturesize(in_param_ptr, rtn_param_ptr);
			break;
		}
	case ISP_PARSER_UP_SENSOR_REG:
		{
			rtn = _ispParserReadSensorReg(in_param_ptr, rtn_param_ptr);
			break;
		}
	case ISP_PARSER_UP_INFO:
		{
			ISP_LOGV("ISP_TOOL:ISP_PARSER_UP_INFO %d\n", cmd);

			rtn = _ispParserGetInfo(isp_handler, in_param_ptr, rtn_param_ptr);
			break;
		}
	default:
		{
			break;
		}
	}

	if (0x00 == rtn) {
		data_len += rtn_ptr->buf_len;
		data_addr = ispParserAlloc(data_len);

		if (NULL != data_addr) {
			data_addr[0] = ISP_PACKET_VERIFY_ID;

			switch (cmd) {
			case ISP_PARSER_UP_PARAM:
				{
					data_addr[1] = ISP_TYPE_PARAM;
					break;
				}
			case ISP_PARSER_UP_PRV_DATA:
				{
					data_addr[1] = ISP_TYPE_PRV_DATA;
					break;
				}
			case ISP_PARSER_UP_CAP_DATA:
				{
					data_addr[1] = ISP_TYPE_CAP_DATA;
					break;
				}
			case ISP_PARSER_UP_CAP_SIZE:
				{
					data_addr[1] = ISP_TYPE_CMD;
					break;
				}
			case ISP_PARSER_UP_MAIN_INFO:
				{
					data_addr[1] = ISP_TYPE_MAIN_INFO;
					break;
				}
			case ISP_PARSER_UP_SENSOR_REG:
				{
					data_addr[1] = ISP_TYPE_SENSOR_REG;
					break;
				}
			case ISP_PARSER_UP_INFO:
				{
					data_addr[1] = ISP_TYPE_INFO;
					break;
				}
			default:
				{
					break;
				}
			}

			data_addr[2] = data_len;

			memcpy((void *)&data_addr[3], (void *)rtn_ptr->buf_addr, rtn_ptr->buf_len);

			data_addr[(data_len >> 0x02) - 0x01] = ISP_PACKET_END_ID;

			ispParserFree((void *)rtn_ptr->buf_addr);

			rtn_ptr->buf_addr = (cmr_uint) data_addr;
			rtn_ptr->buf_len = data_len;
		}
	}

	return rtn;
}

cmr_u32 ispParserGetSizeID(cmr_u32 width, cmr_u32 height)
{
	cmr_u32 i = 0x00;
	cmr_u32 size_id = 0x00;
	struct isp_size_info *size_info_ptr = ISP_ParamGetSizeInfo();

	for (i = 0x00; ISP_SIZE_END != size_info_ptr[i].size_id; i++) {
		if ((size_info_ptr[i].width == width)
		    && (size_info_ptr[i].height == height)) {
			size_id = size_info_ptr[i].size_id;
			break;
		}
	}

	return size_id;
}

cmr_s32 ispParser(cmr_handle isp_handler, cmr_u32 cmd, void *in_param_ptr, void *rtn_param_ptr)
{
	cmr_s32 rtn = 0x00;

	ISP_LOGV("ISP_TOOL:cmd = %d\n", cmd);

	switch (cmd) {
	case ISP_PARSER_UP_MAIN_INFO:
	case ISP_PARSER_UP_PARAM:
	case ISP_PARSER_UP_PRV_DATA:
	case ISP_PARSER_UP_CAP_DATA:
	case ISP_PARSER_UP_CAP_SIZE:
	case ISP_PARSER_UP_SENSOR_REG:
	case ISP_PARSER_UP_INFO:
		{
			rtn = _ispParserUpHandle(isp_handler, cmd, in_param_ptr, rtn_param_ptr);
			break;
		}
	case ISP_PARSER_DOWN:
		{
			rtn = _ispParserDownHandle(isp_handler, in_param_ptr, rtn_param_ptr);
			break;
		}

	default:
		{
			break;
		}
	}

	return rtn;
}

cmr_u32 *ispParserAlloc(cmr_u32 size)
{
	cmr_u32 *addr = 0x00;

	if (0x00 != size) {
		addr = (cmr_u32 *) malloc(size);
	}

	return addr;
}

cmr_s32 ispParserFree(void *addr)
{
	cmr_s32 rtn = 0x00;
	void *temp_addr = addr;

	if (NULL != temp_addr) {
		free(temp_addr);
	}

	return rtn;
}
