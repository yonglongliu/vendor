#include "mv_interface.h"
#include "3dnr_module.h"
#include <time.h>
#include <errno.h>
#include <cutils/properties.h>
#include <semaphore.h>

#define SAVE_IMG
#define SAVE_IMG_PROPERTY     "property.3dnr.save"
#define IMG_SAVE_H 1088
#define DUMP_IMG_NUM 100
#define DUMP_IMG_ENABLE 0
static char debug[PROPERTY_VALUE_MAX] = { 0 };
static char timeout[PROPERTY_VALUE_MAX] = { 0 };
static char save_flag[PROPERTY_VALUE_MAX] = { 0 };

static c3dnr_info_t *p3dnr_info;
static sem_t blend_sem;
static cmr_s64 gTime, gTime1;

#if DUMP_IMG_ENABLE
char *g_buf_blend_pre = NULL;
char *g_buf_blend_post = NULL;
char *g_buf_blend_pre_pos = NULL;
char *g_buf_blend_post_pos = NULL;
cmr_s32 g_save_file = 0;
cmr_s32 g_cur_num = 0;
#endif

c3dnr_info_t *get_3dnr_moduleinfo()
{
	return p3dnr_info;
}

static cmr_s64 system_time()
{
	struct timespec t;
	t.tv_sec = t.tv_nsec = 0;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return (cmr_s64)(t.tv_sec) * 1000000000LL + (cmr_s64)t.tv_nsec;
}

static void startTime()
{
	gTime = system_time();
}

static void endTime(const char *str)
{
	cmr_s64 t = system_time() - gTime;
	double ds = ((double)t) / 1e9;
	BL_LOGI("Test: %s, %f s\n", str, ds);
}

static void startTime1()
{
	gTime1 = system_time();
}

static void endTime1(const char *str)
{
	cmr_s64 t = system_time() - gTime1;
	double ds = ((double)t) / 1e9;
	BL_LOGI("Test: %s, %f s\n", str, ds);
}

static void init_blend_sem()
{
	sem_init(&blend_sem, 0, 0);
}

static void wait_blend()
{
	sem_wait(&blend_sem);
}

static void post_blend()
{
	sem_post(&blend_sem);
}

cmr_s32 buffer_copy(c3dnr_buffer_t *buf_dst, c3dnr_buffer_t *buf_src)
{
	buf_dst->bufferY = buf_src->bufferY;
	buf_dst->bufferU = buf_src->bufferU;
	buf_dst->bufferV = buf_src->bufferV;
	buf_dst->fd = buf_src->fd;
	return 0;
}

cmr_s32 blend_trigger(c3dnr_buffer_t *pfirst_blendimg, c3dnr_buffer_t *psecond_blendimg, c3dnr_buffer_t *pblend_out,
		  cmr_s32 mv_y, cmr_s32 mv_x, cmr_s32 blending_no)
{
	c3dnr_io_info_t isp_3dnr;
	cmr_s32 ret = 0;
	isp_3dnr.image[0].bufferY = pfirst_blendimg->bufferY;
	isp_3dnr.image[0].fd = pfirst_blendimg->fd;
	isp_3dnr.image[1].bufferY = psecond_blendimg->bufferY;
	isp_3dnr.image[1].fd = psecond_blendimg->fd;
	isp_3dnr.image[2].bufferY = pblend_out->bufferY;
	isp_3dnr.image[2].fd = pblend_out->fd;
	isp_3dnr.width = p3dnr_info->orig_width;
	isp_3dnr.height = p3dnr_info->orig_height;
	isp_3dnr.mv_x = mv_x;
	isp_3dnr.mv_y = mv_y;
	isp_3dnr.blending_no = blending_no;
	if (NULL != p3dnr_info->isp_ioctrl)
		ret = p3dnr_info->isp_ioctrl(p3dnr_info->isp_handle, &isp_3dnr);
	else
		BL_LOGE("p3dnr_info->isp_ioctrl is NULL.\n");
	if (0 != ret) {
		BL_LOGE("fail to call isp_ioctl(). ret = %d.\n", ret);
	}
	BL_LOGI("done.\n");
	return ret;
}

static cmr_s32 save_yuv(char *filename, char *buffer, cmr_u32 width, cmr_u32 height, cmr_u32 save_height)
{
	FILE *fp;
	fp = fopen(filename, "wb");
	BL_LOGI("save_yuv: buf: 0x%p.\n", buffer);
	if (fp) {
		fwrite(buffer, 1, width * save_height, fp);
		fwrite(buffer + width * height, 1, width * save_height / 2, fp);
		fclose(fp);
		return 0;
	} else
		return -1;
}

static cmr_s32 check_is_cancelled()
{
	if (1 == p3dnr_info->is_cancel) {
		p3dnr_info->status = C3DNR_STATUS_IDLE;
		BL_LOGI("the status is %d.\n", p3dnr_info->status);
		return -1;
	}
	return 0;
}

__attribute__ ((visibility("default"))) cmr_s32 threednr_function_pre(c3dnr_buffer_t *small_image, c3dnr_buffer_t *orig_image)
{
	cmr_s32 width = p3dnr_info->small_width;
	cmr_s32 height = p3dnr_info->small_height;
	cmr_s8 mv_xy[2];

#ifdef SAVE_IMG
	char filename[128];
#endif
	{
		startTime1();
	}
#if DUMP_IMG_ENABLE
	{	//save the preview frame for algo team.
		property_get("save_3dnr_preview_data", save_flag, "no");
		if (!strcmp(save_flag, "no")) {
			p3dnr_info->status = C3DNR_STATUS_IDLE;
			BL_LOGI("3DNR function off. need to set yes to property save_3dnr_preview_data");
			return 0;
		}
	}
#endif
	p3dnr_info->status = C3DNR_STATUS_BUSY;
	{

		if ((width & 0x7) || (height & 0x1)) {	//judge whether the width and height meet the require of ISP
			BL_LOGE("the size of input image doesn't meet require:width=%d , height=%d\n", width, height);
			return -1;
		}
		if (width > p3dnr_info->small_width || height > p3dnr_info->small_height) {
			BL_LOGE
			    ("the width of input images is %d,the height of input images is %d,is lager than values of threednr_init function\n",
			     width, height);
			return -1;
		}

#if DUMP_IMG_ENABLE
		{	//save the preview frame for algo team.
			property_get("save_3dnr_preview_data", save_flag, "no");
			if (!strcmp(save_flag, "yes")) {
				if (g_cur_num < DUMP_IMG_NUM) {
					BL_LOGI("start to save the preview data before blending.");
					memcpy(g_buf_blend_pre_pos, orig_image->bufferY,
					       p3dnr_info->orig_width * p3dnr_info->orig_height * 3 / 2);
					g_buf_blend_pre_pos += p3dnr_info->orig_width * p3dnr_info->orig_height * 3 / 2;
					BL_LOGI("end to save the preview data before blending. num: %d.", g_cur_num);
				}
			}
		}
#endif

		threeDNR_1D_process(small_image, width, height, p3dnr_info->curr_frameno, MODE_PREVIEW);
		BL_LOGI("------------threeDNR_1D_process frameno:%d , width:%d , height:%d , smallimage:%p,%p,%p\n",
			p3dnr_info->curr_frameno, width, height, small_image->bufferY, small_image->bufferU,
			small_image->bufferV);

		if (1 == p3dnr_info->is_first_frame) {
			p3dnr_info->is_first_frame = 0;
			memcpy(p3dnr_info->out_img[0].bufferY, orig_image->bufferY,
			       p3dnr_info->orig_width * p3dnr_info->orig_height * 3 / 2);
		} else {
			//calculate mv
			if (1 == p3dnr_info->curr_frameno) {
				ProjectMatching_pre(p3dnr_info->xProj1D[0], p3dnr_info->yProj1D[0],
						p3dnr_info->xProj1D[1], p3dnr_info->yProj1D[1], width, height, mv_xy,
						2, p3dnr_info->extra);
			} else {
				ProjectMatching_pre(p3dnr_info->xProj1D[1], p3dnr_info->yProj1D[1],
						p3dnr_info->xProj1D[0], p3dnr_info->yProj1D[0], width, height, mv_xy,
						2, p3dnr_info->extra);
			}
			BL_LOGI("-----------ProjectMatching_pre framno:%d,%d , mv:%d,%d\n", 1 - p3dnr_info->curr_frameno,
				p3dnr_info->curr_frameno, mv_xy[0], mv_xy[1]);
			p3dnr_info->pfirst_blendimg = &p3dnr_info->out_img[1 - p3dnr_info->curr_frameno];
			p3dnr_info->psecond_blendimg = orig_image;
			p3dnr_info->pout_blendimg = &p3dnr_info->out_img[p3dnr_info->curr_frameno];

			//In preview mode, MV is calculated in original size with half pixel precision
			//so here MV need to be divided by 2
			//To avoid that -1 divided by 2 is always -1, the negative MV is first converted to positive before divided by 2
			/*if (mv_xy[0] < 0) {
				mv_xy[0] = -1 * ((-1 * mv_xy[0]) / 2);
			} else {
				mv_xy[0] = mv_xy[0] / 2;
			}
			if (mv_xy[1] < 0) {
				mv_xy[1] = -1 * ((-1 * mv_xy[1]) / 2);
			} else {
				mv_xy[1] = mv_xy[1] / 2;
			}*/
			blend_trigger(p3dnr_info->pfirst_blendimg, p3dnr_info->psecond_blendimg,
				      p3dnr_info->pout_blendimg, mv_xy[0], mv_xy[1], p3dnr_info->blend_num + 10);
			if (p3dnr_info->blend_num < 3) {
				p3dnr_info->blend_num++;
			}
			wait_blend();
			if (0 != check_is_cancelled()) {
				return 0;
			}
			BL_LOGI("1blend trigger framno:%d , firstbuff:%p , secondbuff:%p , outbuff:%p\n",
				p3dnr_info->curr_frameno, p3dnr_info->pfirst_blendimg->bufferY,
				p3dnr_info->psecond_blendimg->bufferY, p3dnr_info->pout_blendimg->bufferY);

#if DUMP_IMG_ENABLE
			{	//save the preview frame for algo team.
				property_get("save_3dnr_preview_data", save_flag, "no");
				if (!strcmp(save_flag, "yes")) {
					if (g_cur_num < DUMP_IMG_NUM) {
						cmr_s32 img_size = p3dnr_info->orig_width * p3dnr_info->orig_height * 3 / 2;

						/*BL_LOGI("start to save the preview data.");
						   memcpy(g_buf_blend_pre_pos, p3dnr_info->psecond_blendimg->bufferY, img_size);
						   g_buf_blend_pre_pos += img_size; */
						BL_LOGI("start to save the preview data after blending.");
						memcpy(g_buf_blend_post_pos, p3dnr_info->pout_blendimg->bufferY,
						       img_size);
						g_buf_blend_post_pos += img_size;
						BL_LOGI("end to save the preview data after blending. num: %d.",
							g_cur_num);
					} else if (0 == g_save_file) {
						FILE *fp = NULL;
						cmr_s32 size =
						    p3dnr_info->orig_width * p3dnr_info->orig_height * 3 / 2 *
						    DUMP_IMG_NUM;
						BL_LOGI("start to save the preview file before blending.");
						sprintf(filename, "/data/misc/cameraserver/%dx%d_img_preview_%d.yuv",
							p3dnr_info->orig_width, p3dnr_info->orig_height, DUMP_IMG_NUM);
						if (NULL != (fp = fopen(filename, "wb"))) {
							fwrite(g_buf_blend_pre, 1, size, fp);
							fclose(fp);
						} else {
							BL_LOGE("Fail to open file : %s.", filename);
						}
						BL_LOGI("start to save the preview file after blending.");
						sprintf(filename,
							"/data/misc/cameraserver/%dx%d_img_preview_after_blend_%d.yuv",
							p3dnr_info->orig_width, p3dnr_info->orig_height,
							DUMP_IMG_NUM - 1);
						if (NULL != (fp = fopen(filename, "wb"))) {
							fwrite(g_buf_blend_post, 1,
							       p3dnr_info->orig_width * p3dnr_info->orig_height * 3 /
							       2 * (DUMP_IMG_NUM - 1), fp);
							fclose(fp);
						} else {
							BL_LOGE("Fail to open file : %s.", filename);
						}
						g_save_file = 1;
						BL_LOGI("end to save all files.");
					}
				}
			}
#endif
			{	//save the preview frame for debug.
				static cmr_s32 num = 0;
				property_get("save_preview_data", save_flag, "no");
				if (!strcmp(save_flag, "yes")) {
					if (num < 10) {
						sprintf(filename, "/data/misc/cameraserver/%dx%d_ref_img_index_%d.yuv",
							p3dnr_info->orig_width, p3dnr_info->orig_height, num);
						save_yuv(filename, (char *)p3dnr_info->pfirst_blendimg->bufferY,
							 p3dnr_info->orig_width, p3dnr_info->orig_height,
							 p3dnr_info->orig_height);
						sprintf(filename, "/data/misc/cameraserver/%dx%d_cur_img_index_%d.yuv",
							p3dnr_info->orig_width, p3dnr_info->orig_height, num);
						save_yuv(filename, (char *)p3dnr_info->psecond_blendimg->bufferY,
							 p3dnr_info->orig_width, p3dnr_info->orig_height,
							 p3dnr_info->orig_height);
						sprintf(filename,
							"/data/misc/cameraserver/%dx%d_blend_img_index_%d.yuv",
							p3dnr_info->orig_width, p3dnr_info->orig_height, num);
						save_yuv(filename, (char *)p3dnr_info->pout_blendimg->bufferY,
							 p3dnr_info->orig_width, p3dnr_info->orig_height,
							 p3dnr_info->orig_height);
					}
					num++;
				}
			}
			BL_LOGI("size: %dx%d, dst: 0x%p, src: 0x%p", p3dnr_info->orig_width,
				  p3dnr_info->orig_height, orig_image->bufferY, p3dnr_info->pout_blendimg->bufferY);
			memcpy(orig_image->bufferY, p3dnr_info->pout_blendimg->bufferY,
			       p3dnr_info->orig_width * p3dnr_info->orig_height * 3 / 2);
			BL_LOGI("copy done.");
			{	//save the preview frame for debug.
				static cmr_s32 num0 = 0;
				property_get("save_preview_data", save_flag, "no");
				if (!strcmp(save_flag, "yes")) {
					if (num0 < 10) {
						sprintf(filename, "/data/misc/cameraserver/%dx%d_pre_img_index_%d.yuv",
							p3dnr_info->orig_width, p3dnr_info->orig_height, num0);
						save_yuv(filename, (char *)orig_image->bufferY, p3dnr_info->orig_width,
							 p3dnr_info->orig_height, p3dnr_info->orig_height);
					}
					num0++;
				}
			}
		}
		p3dnr_info->curr_frameno = 1 - p3dnr_info->curr_frameno;
	}
	if (!(strcmp(timeout, "yes"))) {
		endTime1("threednr_function_pre");
	}
	p3dnr_info->status = C3DNR_STATUS_IDLE;

#if DUMP_IMG_ENABLE
	{	//save the preview frame for algo team.
		property_get("save_3dnr_preview_data", save_flag, "no");
		if (!strcmp(save_flag, "yes")) {
			g_cur_num++;
		}
	}
#endif
	return 0;
}

__attribute__ ((visibility("default"))) cmr_s32 threednr_function(c3dnr_buffer_t *small_image, c3dnr_buffer_t *orig_image)
{
	cmr_s32 width = p3dnr_info->small_width;
	cmr_s32 height = p3dnr_info->small_height;
	cmr_s8 mv_xy[2];

#ifdef SAVE_IMG
	char filename[128];
#endif
	{
		startTime1();
	}

	if (C3DNR_STATUS_DEINITED == p3dnr_info->status) {
		BL_LOGE("the status is deinited. cann't call the function.\n");
		return -1;
	}
	p3dnr_info->status = C3DNR_STATUS_BUSY;

#ifdef SAVE_IMG
	{
		property_get(SAVE_IMG_PROPERTY, save_flag, "no");
		if (!strcmp(save_flag, "yes")) {
			sprintf(filename, "/data/misc/cameraserver/%dx%d_big_img_index_%d.yuv", p3dnr_info->orig_width,
				IMG_SAVE_H /*p3dnr_info->orig_height */ , p3dnr_info->curr_frameno);
			save_yuv(filename, (char *)orig_image->bufferY, p3dnr_info->orig_width,
				 p3dnr_info->orig_height, IMG_SAVE_H);
			sprintf(filename, "/data/misc/cameraserver/%dx%d_small_img_index_%d.yuv",
				p3dnr_info->small_width, IMG_SAVE_H /*p3dnr_info->orig_height */ ,
				p3dnr_info->curr_frameno);
			save_yuv(filename, (char *)small_image->bufferY, p3dnr_info->small_width,
				 p3dnr_info->small_height, IMG_SAVE_H);
		}
	}
#endif
	{

		//p3dnr_info->frame_num = 0;                    //32bit unsigned frame number
		if ((width & 0x7) || (height & 0x1)) {		//judge whether the width and height meet the require of ISP
			BL_LOGE("the size of input image doesn't meet require:width=%d , height=%d\n", width, height);
			return -1;
		}
		if (width > p3dnr_info->small_width || height > p3dnr_info->small_height) {
			BL_LOGE
			    ("the width of input images is %d,the height of input images is %d,is lager than values of threednr_init function\n",
			     width, height);
			return -1;
		}

		threeDNR_1D_process(small_image, width, height, p3dnr_info->curr_frameno, MODE_CAPTURE);
		BL_LOGI("------------threeDNR_1D_process frameno:%d , width:%d , height:%d , smallimage:%x,%x,%x\n",
			p3dnr_info->curr_frameno, width, height, small_image->bufferY[0], small_image->bufferY[1],
			small_image->bufferY[2]);
		buffer_copy(&p3dnr_info->porigimg[p3dnr_info->curr_frameno], orig_image);
		if (p3dnr_info->curr_frameno > 0) {
			//calculate mv
			ProjectMatching(p3dnr_info->xProj1D[p3dnr_info->curr_frameno - 1],
					p3dnr_info->yProj1D[p3dnr_info->curr_frameno - 1],
					p3dnr_info->xProj1D[p3dnr_info->curr_frameno],
					p3dnr_info->yProj1D[p3dnr_info->curr_frameno], width, height, mv_xy, 2,
					p3dnr_info->extra);
			BL_LOGI("-----------ProjectMatching framno:%d,%d , mv:%d,%d\n", p3dnr_info->curr_frameno - 1,
				p3dnr_info->curr_frameno, mv_xy[0], mv_xy[1]);
			if ((p3dnr_info->curr_frameno > 1)) {
				//wait the former blending ok
				BL_LOGI("------------------start-wait blend framno:%d\n", p3dnr_info->curr_frameno);
				wait_blend();
				BL_LOGI("------------------done-wait blend framno:%d\n", p3dnr_info->curr_frameno);
				if (0 != check_is_cancelled()) {
					BL_LOGI("the 3dnr is canceled, return.");
					return 0;
				}
#ifdef SAVE_IMG
				{	//save blend image.
					property_get(SAVE_IMG_PROPERTY, save_flag, "no");
					if (!strcmp(save_flag, "yes")) {
						c3dnr_buffer_t *buf = NULL;
						BL_LOGI("get blend img. cur_frm: %d.", p3dnr_info->curr_frameno);
						if (((p3dnr_info->curr_frameno - 1) == 1)) {
							buf = &p3dnr_info->pdst_img;
						} else {
							buf = &p3dnr_info->porigimg[p3dnr_info->curr_frameno - 2];
						}
						sprintf(filename,
							"/data/misc/cameraserver/%dx%d_blend_img_index_%d.yuv",
							p3dnr_info->orig_width,
							IMG_SAVE_H /*p3dnr_info->orig_height */ ,
							p3dnr_info->curr_frameno - 1);
						save_yuv(filename, (char *)buf->bufferY, p3dnr_info->orig_width,
							 p3dnr_info->orig_height, IMG_SAVE_H);
					}
				}
#endif
				BL_LOGI("ok to get blend img. cur_frm: %d.", p3dnr_info->curr_frameno);
			}
			//call blending trigger function , curr_frameno&curr_frameno-1
			if (p3dnr_info->curr_frameno == 1) {
				p3dnr_info->pfirst_blendimg = &p3dnr_info->porigimg[0];
				p3dnr_info->pout_blendimg = &p3dnr_info->pdst_img;
			}
			p3dnr_info->psecond_blendimg = &p3dnr_info->porigimg[p3dnr_info->curr_frameno];
			blend_trigger(p3dnr_info->pfirst_blendimg, p3dnr_info->psecond_blendimg,
				      p3dnr_info->pout_blendimg, mv_xy[0], mv_xy[1], p3dnr_info->curr_frameno - 1);
			BL_LOGI("1blend trigger framno:%d , firstbuff:%p , secondbuff:%p , outbuff:%p\n",
				p3dnr_info->curr_frameno, p3dnr_info->pfirst_blendimg->bufferY,
				p3dnr_info->psecond_blendimg->bufferY, p3dnr_info->pout_blendimg->bufferY);

#ifdef SAVE_IMG
			{	//save blend image.
				property_get(SAVE_IMG_PROPERTY, save_flag, "no");
				if (!strcmp(save_flag, "yes")) {
					sprintf(filename, "/data/misc/cameraserver/%dx%d_ref_img_index_%d.yuv",
						p3dnr_info->orig_width, IMG_SAVE_H /*p3dnr_info->orig_height */ ,
						p3dnr_info->curr_frameno - 1);
					save_yuv(filename, (char *)p3dnr_info->pfirst_blendimg->bufferY,
						 p3dnr_info->orig_width, p3dnr_info->orig_height, IMG_SAVE_H);
					sprintf(filename, "/data/misc/cameraserver/%dx%d_cur_img_index_%d.yuv",
						p3dnr_info->orig_width, IMG_SAVE_H /*p3dnr_info->orig_height */ ,
						p3dnr_info->curr_frameno - 1);
					save_yuv(filename, (char *)p3dnr_info->psecond_blendimg->bufferY,
						 p3dnr_info->orig_width, p3dnr_info->orig_height, IMG_SAVE_H);
				}
			}
#endif
			p3dnr_info->pfirst_blendimg = p3dnr_info->pout_blendimg;
			p3dnr_info->pout_blendimg = &p3dnr_info->porigimg[p3dnr_info->curr_frameno];
			if (p3dnr_info->curr_frameno == (p3dnr_info->imageNum - 2)) {
				p3dnr_info->pout_blendimg = &p3dnr_info->porigimg[0];
			}
			//if the last one wait blending ok
			if (p3dnr_info->curr_frameno == (p3dnr_info->imageNum - 1)) {

				//wait the last one blending ok
				wait_blend();
				if (0 != check_is_cancelled()) {
					BL_LOGI("the 3dnr is canceled, return.");
					return 0;
				}
				if (2 == p3dnr_info->imageNum) {
					memcpy((char *)p3dnr_info->porigimg[0].bufferY,
					       (char *)p3dnr_info->pdst_img.bufferY,
					       p3dnr_info->orig_width * p3dnr_info->orig_height * 3 / 2);
				}
				BL_LOGI("libsprd3dnr.so all done.");

#ifdef SAVE_IMG
				property_get(SAVE_IMG_PROPERTY, save_flag, "no");
				if (!strcmp(save_flag, "yes")) {
					sprintf(filename, "/data/misc/cameraserver/%dx%d_blend_img_index_%d.yuv",
						p3dnr_info->orig_width, IMG_SAVE_H /*p3dnr_info->orig_height */ ,
						p3dnr_info->curr_frameno);
					save_yuv(filename, (char *)p3dnr_info->porigimg[0].bufferY,
						 p3dnr_info->orig_width, p3dnr_info->orig_height, IMG_SAVE_H);
					BL_LOGI("save_yuv last buf: 0x%p.", p3dnr_info->porigimg[0].bufferY);
				}
#endif
				p3dnr_info->status = C3DNR_STATUS_IDLE;
			}
		} else {
			p3dnr_info->status = C3DNR_STATUS_IDLE;
		}

		//record current orig image
		p3dnr_info->curr_frameno++;
	}
	if (!(strcmp(timeout, "yes"))) {
		endTime1("threednr_function");
	}
	return 0;
}

void threeDNR_1D_process(c3dnr_buffer_t *curr_image, cmr_s32 width, cmr_s32 height, cmr_u32 fram_no, c3dnr_mode_e mode)
{
    BL_LOGI("3dnr mode: %d.", mode);
	if (T3DNR_CAPTURE_MODE_CAPTURE_STAGE != p3dnr_info->operation_mode) {
		if (T3DNR_FULLSIZE_MODE_FUSE_STAGE != p3dnr_info->operation_mode) {
			if (!(strcmp(timeout, "yes"))) {
				startTime();
			}
			IntegralProjection1D_process(curr_image->bufferY, width, height,
						     p3dnr_info->xProj1D[fram_no], p3dnr_info->yProj1D[fram_no],
						     p3dnr_info->extra, mode);
			if (!(strcmp(timeout, "yes"))) {
				endTime("IntegralProjection1D_process");
			}
		}
	}
}

cmr_s32 get_weight(char *weight, cmr_u32 imageNum)
{
	char *ptr = NULL;
	cmr_u32 i;
	char *str = weight;
	cmr_u8 value;
	for (i = 0; i < imageNum - 1; i++) {
		ptr = strstr(str, "_");
		if (NULL != ptr) {
			value = atoi(++ptr);
			if (0 != value)
				p3dnr_info->y_src_weight[i] = value;
			else
				return -1;
		} else {
			BL_LOGE("can not find the weightY of %dth frame\n", i);
			return -1;
		}
		str = ptr;
		ptr = strstr(str, "_");
		if (NULL != ptr) {
			value = atoi(++ptr);
			if (0 != value)
				p3dnr_info->u_src_weight[i] = value;
			else
				return -1;
		} else {
			BL_LOGE("can not find the weightU of %dth frame\n", i);
			return -1;
		}
		str = ptr;
		ptr = strstr(str, "_");
		if (NULL != ptr) {
			value = atoi(++ptr);
			if (0 != value)
				p3dnr_info->v_src_weight[i] = value;
			else
				return -1;
		} else {
			BL_LOGE("can not find the weightV of %dth frame\n", i);
			return -1;
		}
		str = ptr;
		if (!strcmp(debug, "yes"))
			BL_LOGI("the %dth frame value:%d,%d,%d\n", i, p3dnr_info->y_src_weight[i],
				p3dnr_info->u_src_weight[i], p3dnr_info->v_src_weight[i]);
	}
	return 0;
}

cmr_s32 initModule(cmr_s32 small_width, cmr_s32 small_height, cmr_s32 orig_width, cmr_s32 orig_height, cmr_u32 imageNum)
{
	cmr_s32 ret = 0;
	cmr_u32 i = 0;
	char weight[PROPERTY_VALUE_MAX] = { 0 };
	char weight_default[PROPERTY_VALUE_MAX] =
	    "_128_128_128_128_128_128_128_128_128_128_128_128_128_128_128_128_128_128";
	property_get("debug", debug, "no");
	BL_LOGI("debug=%s\n", debug);
	init_blend_sem();
	cmr_s32 *mem0 =
	    new cmr_s32[(small_width * small_height) * (imageNum) + THREAD_NUM_MAX * small_width + 2 * (imageNum - 1)];
	if (NULL == mem0) {
		return -1;
	}
	p3dnr_info->xProj1D = new cmr_s32 *[imageNum];
	if (NULL == p3dnr_info->xProj1D) {
		delete [] mem0;
		return -1;
	}
	p3dnr_info->yProj1D = new cmr_s32 *[imageNum];
	if (NULL == p3dnr_info->yProj1D) {
		delete [] mem0;
		delete [] p3dnr_info->xProj1D;
		return -1;
	}
	for (i = 0; i < imageNum; i++) {
		p3dnr_info->xProj1D[i] = mem0 + i * small_height;
		p3dnr_info->yProj1D[i] = mem0 + imageNum * small_height + i * small_width;
	}
	p3dnr_info->extra = mem0 + (small_height + small_width) * (imageNum);
	p3dnr_info->MV_x = p3dnr_info->extra + THREAD_NUM_MAX * small_width;
	p3dnr_info->MV_y = p3dnr_info->MV_x + imageNum - 1;
	cmr_u8 *mem1 = new cmr_u8[3 * imageNum];
	if (NULL == mem1) {
		delete [] mem0;
		delete [] p3dnr_info->xProj1D;
		delete [] p3dnr_info->yProj1D;
		return -1;
	}
	p3dnr_info->y_src_weight = mem1;
	p3dnr_info->u_src_weight = p3dnr_info->y_src_weight + imageNum;
	p3dnr_info->v_src_weight = p3dnr_info->u_src_weight + imageNum;
	p3dnr_info->intermedbuf.bufferY = NULL;
	p3dnr_info->intermedbuf.bufferU = NULL;
	p3dnr_info->intermedbuf.bufferV = NULL;
	if (imageNum > 2) {
		p3dnr_info->intermedbuf.bufferY = (cmr_u8 *)malloc(orig_width * orig_height * 3 / 2);
		if (NULL == p3dnr_info->intermedbuf.bufferY) {
			delete [] mem0;
			delete [] mem1;
			delete [] p3dnr_info->xProj1D;
			delete [] p3dnr_info->yProj1D;
			return -1;
		}
		{
			p3dnr_info->intermedbuf.bufferU = p3dnr_info->intermedbuf.bufferY + orig_width * orig_height;
			p3dnr_info->intermedbuf.bufferV =
			    p3dnr_info->intermedbuf.bufferU + orig_width * orig_height / 4;
		}
	}
	p3dnr_info->porigimg = (c3dnr_buffer_t *)malloc(sizeof(c3dnr_buffer_t) * p3dnr_info->imageNum);
	if (p3dnr_info->porigimg == NULL) {
		delete [] mem0;
		delete [] mem1;
		delete [] p3dnr_info->xProj1D;
		delete [] p3dnr_info->yProj1D;
		BL_LOGI("porigimg alloc error");
		free(p3dnr_info->intermedbuf.bufferY);
		return -1;
	}
	property_get("3dnr_params", weight, "no");
	if (!strcmp(weight, "no")) {
		BL_LOGI("user doesn't input weight table,please use default weight table\n");
		ret = get_weight(weight_default, imageNum);
		if (ret < 0)
			BL_LOGE("get default weight table fails!\n");
	} else {
		ret = get_weight(weight, imageNum);
		if (ret < 0) {
			BL_LOGI
			    ("the input weight table can't meet the demand of 3DNR,please use default weight table\n");
			ret = get_weight(weight_default, imageNum);
			if (ret < 0)
				BL_LOGE("get default weight table fails!\n");
		}
	}
	BL_LOGI("-------------outimage is:%p\n", p3dnr_info->pout_blendimg->bufferY);
	return 0;
}

void Config3DNR(c3dnr_param_info_t *param)
{
	p3dnr_info->fusion_mode = 1;					//1bit unsigned {0, 1} imge fusion mode:0: adaptive weight 1: fixed weight
	p3dnr_info->MV_mode = 0;					//1bit unsigned {0, 1} MV_mode:0 MV calculated by integral projection 1: MV set by users
	p3dnr_info->filter_switch = 1;					//1bit unsigned{0, 1} whether use 3x3 filter to smooth current pixel for calculate pixel difference for Y component 0: not filter 1: filter
	p3dnr_info->orig_width = param->orig_width;
	p3dnr_info->orig_height = param->orig_height;
	p3dnr_info->small_width = param->small_width;
	p3dnr_info->small_height = param->small_height;
	p3dnr_info->imageNum = param->total_frame_num;
	p3dnr_info->curr_frameno = 0;
	p3dnr_info->pdst_img.bufferY = param->poutimg->bufferY;
	p3dnr_info->pdst_img.bufferU = param->poutimg->bufferU;
	p3dnr_info->pdst_img.bufferV = param->poutimg->bufferV;
	p3dnr_info->pdst_img.fd = param->poutimg->fd;
	p3dnr_info->is_first_frame = 1;
	p3dnr_info->blend_num = 0;
	buffer_copy(&p3dnr_info->out_img[0], &param->out_img[0]);
	buffer_copy(&p3dnr_info->out_img[1], &param->out_img[1]);
	if (p3dnr_info->imageNum % 2 == 0) {
		p3dnr_info->pout_blendimg = &p3dnr_info->pdst_img;
		BL_LOGI("-------------outblending is:%p\n", p3dnr_info->pout_blendimg->bufferY);
	} else {
		p3dnr_info->pout_blendimg = &p3dnr_info->intermedbuf;
		BL_LOGI("-------------outblending is:%p\n", p3dnr_info->pout_blendimg->bufferY);
	}
	p3dnr_info->isp_handle = param->isp_handle;
	p3dnr_info->isp_ioctrl = param->isp_ioctrl;
}

__attribute__ ((visibility("default"))) cmr_s32 threednr_init(c3dnr_param_info_t *param)
{
	cmr_s32 ret = 0;
	if ((param->small_width & 0x7) || (param->small_height & 0x1)) {	//judge whether the width and height meet the require of ISP
		BL_LOGE("the size of input image doesn't meet require:width=%d , height=%d\n", param->small_width,
			param->small_height);
		return -1;
	}
	if (param->total_frame_num < 2) {	//judge whether the number of frames is more than 2
		BL_LOGE("the number of input images is %d,can not meet the demand of 3DNR\n", param->total_frame_num);
		return -1;
	}
	ret = DNR_init_threadPool();
	if (ret < 0) {
		BL_LOGE("DNR_init_threadPool fail\n");
		return -1;
	}
	p3dnr_info = new c3dnr_info_t;
	if (NULL == p3dnr_info) {
		BL_LOGE("class malloc fail");
		return -1;
	}
	if (NULL != param->isp_handle) {
		p3dnr_info->isp_handle = param->isp_handle;
		BL_LOGI("isp_handle is not NULL.");
	} else {
		BL_LOGE("isp_handle is NULL.");
		return -1;
	}
	Config3DNR(param);
	ret =
	    initModule(param->small_width, param->small_height, param->orig_width, param->orig_height,
		       param->total_frame_num);
	if (ret < 0) {
		BL_LOGE("initModule malloc fail");
		return -1;
	}
	p3dnr_info->operation_mode = T3DNR_CAPTURE_MODE_PREVIEW_STAGE;

	//create blend monitor thread
	p3dnr_info->status = C3DNR_STATUS_INITED;
	p3dnr_info->is_cancel = 0;

#if DUMP_IMG_ENABLE
	cmr_s32 size = DUMP_IMG_NUM * param->orig_width * param->orig_height * 3 / 2;
	if (NULL == (g_buf_blend_pre = (char *)malloc(size))) {
		BL_LOGE("Fail to malloc memory for g_buf_blend_pre.");
		return -1;
	}
	memset(g_buf_blend_pre, 0, size);
	g_buf_blend_pre_pos = g_buf_blend_pre;
	if (NULL == (g_buf_blend_post = (char *)malloc(size))) {
		BL_LOGE("Fail to malloc memory for g_buf_blend_pre.");
		return -1;
	}
	memset(g_buf_blend_post, 0, size);
	g_buf_blend_post_pos = g_buf_blend_post;
	g_save_file = 0;
	g_cur_num = 0;
#endif
	return ret;
}

__attribute__ ((visibility("default"))) cmr_s32 threednr_deinit()
{
	cmr_s32 ret = 0, num = 0, is_timeout = 0;
	while (1) {
		if (C3DNR_STATUS_BUSY == p3dnr_info->status) {
			usleep(1000 * 10);
			BL_LOGI("wait 10ms: the 3dnr is busy.");
			num++;
			if (num > 50) {
				post_blend();
				BL_LOGI("wait too long, post blend.");
				is_timeout = 1;
				break;
			}
		} else {
			BL_LOGI("the 3dnr isn't busy, can deinit it.");
			break;
		}
	}
	p3dnr_info->status = C3DNR_STATUS_DEINITED;
	if (p3dnr_info->porigimg) {
		free(p3dnr_info->porigimg);
	}
	if (NULL != p3dnr_info->xProj1D[0])
		delete [] p3dnr_info->xProj1D[0];
	if (NULL != p3dnr_info->xProj1D)
		delete[]p3dnr_info->xProj1D;
	if (NULL != p3dnr_info->yProj1D)
		delete[]p3dnr_info->yProj1D;
	else {
		BL_LOGI("xProjCurr is NULL");
	}
	if (NULL != p3dnr_info->y_src_weight)
		delete[]p3dnr_info->y_src_weight;
	else {
		BL_LOGI("y_src_weight is NULL");
	}
	if (p3dnr_info->intermedbuf.bufferY != NULL)
		free(p3dnr_info->intermedbuf.bufferY);
	delete p3dnr_info;
	ret = DNR_destroy_threadPool();
	if (ret < 0) {
		BL_LOGE("DNR_destroy_threadPool fail\n");
		goto exit;
	}
#if DUMP_IMG_ENABLE
	if (NULL != g_buf_blend_pre) {
		free(g_buf_blend_pre);
		g_buf_blend_pre = NULL;
	}
	if (NULL != g_buf_blend_post) {
		free(g_buf_blend_post);
		g_buf_blend_post = NULL;
	}
#endif

exit:
	if (is_timeout) {
		ret = -ETIMEDOUT;
		return ret;
	}

	return ret;
}

__attribute__ ((visibility("default"))) cmr_s32 threednr_callback()
{
	post_blend();
	BL_LOGI("finish the last blend.");
	return 0;
}

__attribute__ ((visibility("default"))) cmr_s32 threednr_cancel()
{
	p3dnr_info->is_cancel = 1;
	BL_LOGI("cancel the 3dnr process.");
	return 0;
}
