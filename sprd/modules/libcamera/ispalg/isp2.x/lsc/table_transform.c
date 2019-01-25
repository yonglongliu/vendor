#include <math.h>
#include <stdio.h>
//#include <windows.h>
#include "lsc_adv.h"

////////////////////////////// sub functions //////////////////////////////

static void save_tab_to_channel(unsigned int gain_width, unsigned int gain_height, unsigned int gain_pattern, unsigned short *ch_r, unsigned short *ch_gr, unsigned short *ch_gb, unsigned short *ch_b,
								unsigned short *rlt_tab)
{
	unsigned int i = 0;

	for (i = 0; i < gain_width * gain_height; i++) {

		switch (gain_pattern) {
		case 0:				//Gr
			ch_gr[i] = rlt_tab[4 * i + 0];
			ch_r[i] = rlt_tab[4 * i + 1];
			ch_b[i] = rlt_tab[4 * i + 2];
			ch_gb[i] = rlt_tab[4 * i + 3];
			break;
		case 1:				//R
			ch_r[i] = rlt_tab[4 * i + 0];
			ch_gr[i] = rlt_tab[4 * i + 1];
			ch_gb[i] = rlt_tab[4 * i + 2];
			ch_b[i] = rlt_tab[4 * i + 3];
			break;
		case 2:				//B
			ch_b[i] = rlt_tab[4 * i + 0];
			ch_gb[i] = rlt_tab[4 * i + 1];
			ch_gr[i] = rlt_tab[4 * i + 2];
			ch_r[i] = rlt_tab[4 * i + 3];
			break;
		case 3:				//Gb
			ch_gb[i] = rlt_tab[4 * i + 0];
			ch_b[i] = rlt_tab[4 * i + 1];
			ch_r[i] = rlt_tab[4 * i + 2];
			ch_gr[i] = rlt_tab[4 * i + 3];
			break;
		default:
			break;
		}
	}
}

float calc_slope(float a1, float a2, float a3)
{
	// (-1,a1)  ,  (0,a2)  ,  (1,a3)
	// let y = ax^2 + bx + c
	// y'= 2ax + b
	// y'(0) = b
	// a = (a1 - 2a2 + a3)/2
	// b = (a3 - a1)/2
	// c = a2
	UNUSED(a2);

	float slope = (a3 - a1) / 2;

	return slope;
}

static void matrix_multiply_R(float *C_mx, float *A_mx, int *B_mx)
{
	int j, i;
	for (j = 0; j < 4; j++) {
		for (i = 0; i < 4; i++) {
			C_mx[j * 4 + i] = A_mx[j * 4 + 0] * B_mx[0 * 4 + i] + A_mx[j * 4 + 1] * B_mx[1 * 4 + i] + A_mx[j * 4 + 2] * B_mx[2 * 4 + i] + A_mx[j * 4 + 3] * B_mx[3 * 4 + i];
		}
	}
}

static void matrix_multiply_L(float *C_mx, int *A_mx, float *B_mx)
{
	int j, i;
	for (j = 0; j < 4; j++) {
		for (i = 0; i < 4; i++) {
			C_mx[j * 4 + i] = A_mx[j * 4 + 0] * B_mx[0 * 4 + i] + A_mx[j * 4 + 1] * B_mx[1 * 4 + i] + A_mx[j * 4 + 2] * B_mx[2 * 4 + i] + A_mx[j * 4 + 3] * B_mx[3 * 4 + i];
		}
	}
}

float table_bicubic_interpolation(unsigned short *src_tab, unsigned int src_gain_width, unsigned int src_gain_height, int TL_i, int TL_j, float dx, float dy)
{
	UNUSED(src_gain_height);

	float f00 = (float)src_tab[(TL_j + 1) * src_gain_width + (TL_i + 0)];
	float f10 = (float)src_tab[(TL_j + 1) * src_gain_width + (TL_i + 1)];
	float f01 = (float)src_tab[(TL_j + 0) * src_gain_width + (TL_i + 0)];
	float f11 = (float)src_tab[(TL_j + 0) * src_gain_width + (TL_i + 1)];
	float f00_L = (float)src_tab[(TL_j + 1) * src_gain_width + (TL_i - 1)];
	float f00_D = (float)src_tab[(TL_j + 2) * src_gain_width + (TL_i + 0)];
	float f00_LD = (float)src_tab[(TL_j + 2) * src_gain_width + (TL_i - 1)];
	float f10_R = (float)src_tab[(TL_j + 1) * src_gain_width + (TL_i + 2)];
	float f10_D = (float)src_tab[(TL_j + 2) * src_gain_width + (TL_i + 1)];
	float f10_RD = (float)src_tab[(TL_j + 2) * src_gain_width + (TL_i + 2)];
	float f01_L = (float)src_tab[(TL_j + 0) * src_gain_width + (TL_i - 1)];
	float f01_U = (float)src_tab[(TL_j - 1) * src_gain_width + (TL_i + 0)];
	float f01_LU = (float)src_tab[(TL_j - 1) * src_gain_width + (TL_i - 1)];
	float f11_R = (float)src_tab[(TL_j + 0) * src_gain_width + (TL_i + 2)];
	float f11_U = (float)src_tab[(TL_j - 1) * src_gain_width + (TL_i + 1)];
	float f11_RU = (float)src_tab[(TL_j - 1) * src_gain_width + (TL_i + 2)];

	float fx00 = calc_slope(f00_L, f00, f10);
	float fy00 = calc_slope(f00_D, f00, f01);
	float fx10 = calc_slope(f00, f10, f10_R);
	float fy10 = calc_slope(f10_D, f10, f11);
	float fx01 = calc_slope(f01_L, f01, f11);
	float fy01 = calc_slope(f00, f01, f01_U);
	float fx11 = calc_slope(f01, f11, f11_R);
	float fy11 = calc_slope(f10, f11, f11_U);
	float fx00_D = calc_slope(f00_LD, f00_D, f10_D);
	float fx10_D = calc_slope(f00_D, f10_D, f10_RD);
	float fx01_U = calc_slope(f01_LU, f01_U, f11_U);
	float fx11_U = calc_slope(f01_U, f11_U, f11_RU);

	float fxy00 = calc_slope(fx00_D, fx00, fx01);
	float fxy10 = calc_slope(fx10_D, fx10, fx11);
	float fxy01 = calc_slope(fx00, fx01, fx01_U);
	float fxy11 = calc_slope(fx10, fx11, fx11_U);

	int coef_mx_L[16] = { 1, 0, 0, 0, 0, 0, 1, 0, -3, 3, -2, -1, 2, -2, 1, 1 };
	int coef_mx_R[16] = { 1, 0, -3, 2, 0, 0, 3, -2, 0, 1, -2, 1, 0, 0, -1, 1 };
	float f_mx[16] = { f00, f01, fy00, fy01, f10, f11, fy10, fy11, fx00, fx01, fxy00, fxy01, fx10, fx11, fxy10, fxy11 };
	float tmp_mx[16];
	float a_mx[16];
	float x_row[4] = { 1, dx, dx * dx, dx * dx * dx };
	float y_col[4] = { 1, dy, dy * dy, dy * dy * dy };
	float tmp_col[4];
	float answer;
	int j;
	matrix_multiply_R(tmp_mx, f_mx, coef_mx_R);
	matrix_multiply_L(a_mx, coef_mx_L, tmp_mx);

	for (j = 0; j < 4; j++) {
		tmp_col[j] = a_mx[j * 4 + 0] * y_col[0] + a_mx[j * 4 + 1] * y_col[1] + a_mx[j * 4 + 2] * y_col[2] + a_mx[j * 4 + 3] * y_col[3];
	}

	answer = x_row[0] * tmp_col[0] + x_row[1] * tmp_col[1] + x_row[2] * tmp_col[2] + x_row[3] * tmp_col[3];

	return answer;
}

static void free_set_null(void *handle)
{
	if (handle != NULL) {
		free(handle);
		handle = NULL;
	}
}

////////////////////////////// lsc_table_transform main //////////////////////////////

int lsc_table_transform(struct lsc_table_transf_info *src, struct lsc_table_transf_info *dst, enum lsc_transform_action action, void *action_info)
{
	int rtn = 0;

	ISP_LOGV("src_info[%d,%d,%d,%d,%d]", src->img_width, src->img_height, src->grid, src->gain_width, src->gain_height);
	ISP_LOGV("src->pm_tab0[%d,%d,%d,%d]", src->pm_tab0[0], src->pm_tab0[1], src->pm_tab0[2], src->pm_tab0[3]);
	ISP_LOGV("src->tab[%d,%d,%d,%d]", src->tab[0], src->tab[1], src->tab[2], src->tab[3]);
	ISP_LOGV("dst_info[%d,%d,%d,%d,%d]", dst->img_width, dst->img_height, dst->grid, dst->gain_width, dst->gain_height);
	ISP_LOGV("dst->pm_tab0[%d,%d,%d,%d]", dst->pm_tab0[0], dst->pm_tab0[1], dst->pm_tab0[2], dst->pm_tab0[3]);

	unsigned int gain_pattern_tmp = 3;
	unsigned short *src_r = (unsigned short *)malloc(src->gain_width * src->gain_height * sizeof(unsigned short));
	unsigned short *src_gr = (unsigned short *)malloc(src->gain_width * src->gain_height * sizeof(unsigned short));
	unsigned short *src_gb = (unsigned short *)malloc(src->gain_width * src->gain_height * sizeof(unsigned short));
	unsigned short *src_b = (unsigned short *)malloc(src->gain_width * src->gain_height * sizeof(unsigned short));
	unsigned short *src0_r = (unsigned short *)malloc(src->gain_width * src->gain_height * sizeof(unsigned short));
	unsigned short *src0_gr = (unsigned short *)malloc(src->gain_width * src->gain_height * sizeof(unsigned short));
	unsigned short *src0_gb = (unsigned short *)malloc(src->gain_width * src->gain_height * sizeof(unsigned short));
	unsigned short *src0_b = (unsigned short *)malloc(src->gain_width * src->gain_height * sizeof(unsigned short));
	unsigned short *dst0_r = (unsigned short *)malloc(dst->gain_width * dst->gain_height * sizeof(unsigned short));
	unsigned short *dst0_gr = (unsigned short *)malloc(dst->gain_width * dst->gain_height * sizeof(unsigned short));
	unsigned short *dst0_gb = (unsigned short *)malloc(dst->gain_width * dst->gain_height * sizeof(unsigned short));
	unsigned short *dst0_b = (unsigned short *)malloc(dst->gain_width * dst->gain_height * sizeof(unsigned short));
	float *dif0_r = (float *)malloc(dst->gain_width * dst->gain_height * sizeof(float));
	float *dif0_gr = (float *)malloc(dst->gain_width * dst->gain_height * sizeof(float));
	float *dif0_gb = (float *)malloc(dst->gain_width * dst->gain_height * sizeof(float));
	float *dif0_b = (float *)malloc(dst->gain_width * dst->gain_height * sizeof(float));

	save_tab_to_channel(src->gain_width, src->gain_height, gain_pattern_tmp, src_r, src_gr, src_gb, src_b, src->tab);
	save_tab_to_channel(src->gain_width, src->gain_height, gain_pattern_tmp, src0_r, src0_gr, src0_gb, src0_b, src->pm_tab0);
	save_tab_to_channel(dst->gain_width, dst->gain_height, gain_pattern_tmp, dst0_r, dst0_gr, dst0_gb, dst0_b, dst->pm_tab0);

	// for binning
	struct binning_info *binning = NULL;

	// for crop
	struct crop_info *crop = NULL;
	int crop_x = 0;				// dst table coord-x on channel plane
	int crop_y = 0;				// dst table coord-y on channel plane
	int start_x = 0;			// crop->start_x/2;
	int start_y = 0;			// crop->start_y/2;
	int TL_i = 0;				// src table top left index-i
	int TL_j = 0;				// src table top left index-j
	float dx = 0.0;				// distence to left     , where total length normalize to 1
	float dy = 0.0;				// distence to bottem   , where total length normalize to 1
	unsigned int j, i;
	switch (action) {

	case LSC_BINNING:
		binning = (struct binning_info *)action_info;
		ISP_LOGV("binning_info->ratio=%f, src->grid=%d, dst->grid=%d", binning->ratio, src->grid, dst->grid);

		if (src->gain_width != dst->gain_width || src->gain_height != dst->gain_height || src->grid != (unsigned int)(dst->grid / binning->ratio)) {
			memcpy(dst->tab, dst->pm_tab0, dst->gain_width * dst->gain_height * 4 * sizeof(unsigned short));
			ISP_LOGE("do LSC_BINNING error, output default dnp table, src->grid=%d, (unsigned int)(dst->grid/binning->ratio)=%d", src->grid, (unsigned int)(dst->grid / binning->ratio));
			goto exit;
		}

		for (j = 0; j < dst->gain_height; j++) {
			for (i = 0; i < dst->gain_width; i++) {
				dif0_gb[j * dst->gain_width + i] = (float)dst0_gb[j * dst->gain_width + i] / (float)src0_gb[j * src->gain_width + i];
				dif0_b[j * dst->gain_width + i] = (float)dst0_b[j * dst->gain_width + i] / (float)src0_b[j * src->gain_width + i];
				dif0_r[j * dst->gain_width + i] = (float)dst0_r[j * dst->gain_width + i] / (float)src0_r[j * src->gain_width + i];
				dif0_gr[j * dst->gain_width + i] = (float)dst0_gr[j * dst->gain_width + i] / (float)src0_gr[j * src->gain_width + i];

				dst->tab[(j * dst->gain_width + i) * 4 + 0] = (unsigned short)(src->tab[(j * dst->gain_width + i) * 4 + 0] * dif0_gb[j * dst->gain_width + i]);
				dst->tab[(j * dst->gain_width + i) * 4 + 1] = (unsigned short)(src->tab[(j * dst->gain_width + i) * 4 + 1] * dif0_b[j * dst->gain_width + i]);
				dst->tab[(j * dst->gain_width + i) * 4 + 2] = (unsigned short)(src->tab[(j * dst->gain_width + i) * 4 + 2] * dif0_r[j * dst->gain_width + i]);
				dst->tab[(j * dst->gain_width + i) * 4 + 3] = (unsigned short)(src->tab[(j * dst->gain_width + i) * 4 + 3] * dif0_gr[j * dst->gain_width + i]);
			}
		}
		break;

	case LSC_CROP:
		crop = (struct crop_info *)action_info;
		ISP_LOGV("crop_info[%d,%d,%d,%d]", crop->start_x, crop->start_y, crop->width, crop->height);

		if (crop->start_x > src->img_width
			|| crop->start_y > src->img_height
			|| crop->start_x + crop->width > src->img_width
			|| crop->start_y + crop->height > src->img_height
			|| crop->start_x < dst->grid * 2
			|| crop->start_y < dst->grid * 2
			|| crop->start_x + (dst->gain_width - 2) * dst->grid * 2 > (src->gain_width - 3) * src->grid * 2
			|| crop->start_y + (dst->gain_height - 2) * dst->grid * 2 > (src->gain_height - 3) * src->grid * 2) {
			memcpy(dst->tab, dst->pm_tab0, dst->gain_width * dst->gain_height * 4 * sizeof(unsigned short));
			ISP_LOGE("do LSC_CROP error, output default dnp table");
			goto exit;
		}

		start_x = crop->start_x / 2;
		start_y = crop->start_y / 2;
		for (j = 0; j < dst->gain_height; j++) {
			for (i = 0; i < dst->gain_width; i++) {
				crop_x = start_x + (i - 1) * dst->grid;
				crop_y = start_y + (j - 1) * dst->grid;

				TL_i = (int)(crop_x / src->grid) + 1;
				TL_j = (int)(crop_y / src->grid) + 1;
				dx = (float)(crop_x - (TL_i - 1) * src->grid) / src->grid;
				dy = (float)(TL_j * src->grid - crop_y) / src->grid;

				dst->tab[(j * dst->gain_width + i) * 4 + 0] = (unsigned short)table_bicubic_interpolation(src_gb, src->gain_width, src->gain_height, TL_i, TL_j, dx, dy);
				dst->tab[(j * dst->gain_width + i) * 4 + 1] = (unsigned short)table_bicubic_interpolation(src_b, src->gain_width, src->gain_height, TL_i, TL_j, dx, dy);
				dst->tab[(j * dst->gain_width + i) * 4 + 2] = (unsigned short)table_bicubic_interpolation(src_r, src->gain_width, src->gain_height, TL_i, TL_j, dx, dy);
				dst->tab[(j * dst->gain_width + i) * 4 + 3] = (unsigned short)table_bicubic_interpolation(src_gr, src->gain_width, src->gain_height, TL_i, TL_j, dx, dy);

				dif0_gb[j * dst->gain_width + i] = table_bicubic_interpolation(src0_gb, src->gain_width, src->gain_height, TL_i, TL_j, dx, dy);
				dif0_b[j * dst->gain_width + i] = table_bicubic_interpolation(src0_b, src->gain_width, src->gain_height, TL_i, TL_j, dx, dy);
				dif0_r[j * dst->gain_width + i] = table_bicubic_interpolation(src0_r, src->gain_width, src->gain_height, TL_i, TL_j, dx, dy);
				dif0_gr[j * dst->gain_width + i] = table_bicubic_interpolation(src0_gr, src->gain_width, src->gain_height, TL_i, TL_j, dx, dy);

				dif0_gb[j * dst->gain_width + i] = (float)dst0_gb[j * dst->gain_width + i] / dif0_gb[j * dst->gain_width + i];
				dif0_b[j * dst->gain_width + i] = (float)dst0_b[j * dst->gain_width + i] / dif0_b[j * dst->gain_width + i];
				dif0_r[j * dst->gain_width + i] = (float)dst0_r[j * dst->gain_width + i] / dif0_r[j * dst->gain_width + i];
				dif0_gr[j * dst->gain_width + i] = (float)dst0_gr[j * dst->gain_width + i] / dif0_gr[j * dst->gain_width + i];

				dst->tab[(j * dst->gain_width + i) * 4 + 0] = (unsigned short)(dst->tab[(j * dst->gain_width + i) * 4 + 0] * dif0_gb[j * dst->gain_width + i]);
				dst->tab[(j * dst->gain_width + i) * 4 + 1] = (unsigned short)(dst->tab[(j * dst->gain_width + i) * 4 + 1] * dif0_b[j * dst->gain_width + i]);
				dst->tab[(j * dst->gain_width + i) * 4 + 2] = (unsigned short)(dst->tab[(j * dst->gain_width + i) * 4 + 2] * dif0_r[j * dst->gain_width + i]);
				dst->tab[(j * dst->gain_width + i) * 4 + 3] = (unsigned short)(dst->tab[(j * dst->gain_width + i) * 4 + 3] * dif0_gr[j * dst->gain_width + i]);
			}
		}
		break;
	}
  exit:
	free_set_null(src_r);
	free_set_null(src_gr);
	free_set_null(src_gb);
	free_set_null(src_b);
	free_set_null(src0_r);
	free_set_null(src0_gr);
	free_set_null(src0_gb);
	free_set_null(src0_b);
	free_set_null(dst0_r);
	free_set_null(dst0_gr);
	free_set_null(dst0_gb);
	free_set_null(dst0_b);
	free_set_null(dif0_r);
	free_set_null(dif0_gr);
	free_set_null(dif0_gb);
	free_set_null(dif0_b);

	return rtn;
}
