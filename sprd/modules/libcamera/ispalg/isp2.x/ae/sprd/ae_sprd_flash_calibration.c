#include "ae_sprd_flash_calibration.h"
#include "cmr_log.h"
#include <cutils/properties.h>
/*
 * for flash calibration
*/
#define CALI_VERSION 4
#define CALI_VERSION_SUB 20171004
struct flash_led_brightness {
	uint16 levelNumber_pf1;
	uint16 levelNumber_pf2;
	uint16 levelNumber_mf1;
	uint16 levelNumber_mf2;
	uint16 ledBrightness_pf1[32];
	uint16 ledBrightness_pf2[32];
	uint16 ledBrightness_mf1[32];
	uint16 ledBrightness_mf2[32];
	uint16 index_pf1[32];
	uint16 index_pf2[32];
	uint16 index_mf1[32];
	uint16 index_mf2[32];

};

struct flash_g_frames {
	uint16 levelNumber_pf1;
	uint16 levelNumber_pf2;
	uint16 levelNumber_mf1;
	uint16 levelNumber_mf2;
	float shutter_off;
	float shutter_pf1[32];
	float shutter_pf2[32];
	float shutter_mf1[32];
	float shutter_mf2[32];
	float gain_off;
	uint32 gain_pf1[32];
	uint32 gain_pf2[32];
	uint32 gain_mf1[32];
	uint32 gain_mf2[32];
	float g_off[15];
	float g_pf1[32][15];
	float g_pf2[32][15];
	float g_mf1[32][15];
	float g_mf2[32][15];
	uint16 index_pf1[32];
	uint16 index_pf2[32];
	uint16 index_mf1[32];
	uint16 index_mf2[32];
};
struct flash_calibration_data {
	uint32 version;
	uint32 version_sub;
	char name[32];
	int32 error;
	uint8 preflashLevelNum1;
	uint8 preflashLevelNum2;
	uint8 flashLevelNum1;
	uint8 flashLevelNum2;
	bool flashMask[1024];
	uint16 brightnessTable[1024];
	uint16 rTable[1024];		//g: 1024
	uint16 bTable[1024];
	uint16 preflashBrightness[1024];
	uint16 preflashCt[1024];
	int16 driverIndexP1[32];
	int16 driverIndexP2[32];
	int16 driverIndexM1[32];
	int16 driverIndexM2[32];

	uint16 maP1[32];
	uint16 maP2[32];
	uint16 maM1[32];
	uint16 maM2[32];
	uint16 numP1_hwSample;
	uint16 numP2_hwSample;
	uint16 numM1_hwSample;
	uint16 numM2_hwSample;
	uint16 mAMaxP1;
	uint16 mAMaxP2;
	uint16 mAMaxP12;
	uint16 mAMaxM1;
	uint16 mAMaxM2;
	uint16 mAMaxM12;

	float rgRatPf;
	uint16 rPf1[32];
	uint16 gPf1[32];
	uint16 rPf2[32];
	uint16 gPf2[32];
	float rgTab[20];
	float ctTab[20];

};

enum FlashCaliError {
	FlashCali_too_close = -10000,
	FlashCali_too_far,
};

enum FlashCaliState {
	FlashCali_start,
	FlashCali_ae,
	FlashCali_cali,
	FlashCali_end,
	FlashCali_none,
};

struct FCData {
	int numP1_hw;
	int numP2_hw;
	int numM1_hw;
	int numM2_hw;

	int numP1_hwSample;
	int indP1_hwSample[256];
	float maP1_hwSample[256];
	int numP2_hwSample;
	int indP2_hwSample[256];
	float maP2_hwSample[256];
	int numM1_hwSample;
	int indM1_hwSample[256];
	float maM1_hwSample[256];
	int numM2_hwSample;
	int indM2_hwSample[256];
	float maM2_hwSample[256];

	float mAMaxP1;
	float mAMaxP2;
	float mAMaxP12;
	float mAMaxM1;
	float mAMaxM2;
	float mAMaxM12;

	int numP1_alg;
	int numP2_alg;
	int numM1_alg;
	int numM2_alg;
	int indHwP1_alg[32];
	int indHwP2_alg[32];
	int indHwM1_alg[32];
	int indHwM2_alg[32];
	float maHwP1_alg[32];
	float maHwP2_alg[32];
	float maHwM1_alg[32];
	float maHwM2_alg[32];

	int mask[1024];

	float rBuf[1024];			//for sta
	float gBuf[1024];			//for sta
	float bBuf[1024];			//for sta
	float expTime;
	int gain;
	float expTimeBase;
	int gainBase;

	int testIndAll;
	int testInd;
	int isMainTab[200];
	int ind1Tab[200];
	int ind2Tab[200];
	int testMinFrm[200];
	int expReset[200];

	float expTab[200];
	int gainTab[200];

	float rData[200];
	float gData[200];
	float bData[200];

	int stateAeFrameCntSt;
	int stateCaliFrameCntSt;
	int stateCaliFrameCntStSub;

	float rFrame[200][15];
	float gFrame[200][15];
	float bFrame[200][15];

	struct flash_calibration_data out;

};

//============================================================
//input and output
//name: property name
//n: contains n int in property values string
//data: property values string to int data
//return: number of valid data, max: 10
static int get_prop_multi(const char *name, int n, int *data)
{
#define MAX_PROP_DATA_NUM 10
	int i;
	char str[300] = "";
	char strTemp[300] = "";
	for (i = 0; i < n; i++)
		data[i] = 0;

	property_get(name, str, "");

	int ret = -1;
	if (strlen(str) == 0)
		return 0;

#define FIND_NUM 1
#define FIND_ZERO 0
	int state = FIND_NUM;
	int charCnt = 0;
	int dataCnt = 0;
	ret = 0;
	int len;
	len = strlen(str);
	for (i = 0; i < len; i++) {
		if (state == FIND_NUM) {
			if (str[i] == ' ' || str[i] == '\t') {

			} else {
				strTemp[charCnt] = str[i];
				charCnt++;
				state = FIND_ZERO;
			}
		} else {
			if (str[i] == ' ' || str[i] == '\t') {
				state = FIND_NUM;
				if (dataCnt < MAX_PROP_DATA_NUM) {
					strTemp[charCnt] = 0;
					data[dataCnt] = atoi(strTemp);
					dataCnt++;
					charCnt = 0;
				}
			} else {
				strTemp[charCnt] = str[i];
				charCnt++;
			}
		}
	}
	if (charCnt != 0) {
		if (dataCnt < MAX_PROP_DATA_NUM) {
			strTemp[charCnt] = 0;
			data[dataCnt] = atoi(strTemp);
			dataCnt++;
		}
	}
	ret = dataCnt;
	return ret;
}

static int _aem_stat_preprocess2(cmr_u32 * src_aem_stat, float *dst_r, float *dst_g, float *dst_b, struct ae_size aem_blk_size, struct ae_size aem_blk_num, cmr_u8 aem_shift)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_u64 bayer_pixels = aem_blk_size.w * aem_blk_size.h / 4;
	cmr_u32 stat_blocks = aem_blk_num.w * aem_blk_num.h;
	cmr_u32 *src_r_stat = (cmr_u32 *) src_aem_stat;
	cmr_u32 *src_g_stat = (cmr_u32 *) src_aem_stat + stat_blocks;
	cmr_u32 *src_b_stat = (cmr_u32 *) src_aem_stat + 2 * stat_blocks;
	//cmr_u16 *dst_r = (float*)dst_aem_stat;
	//cmr_u16 *dst_g = (float*)dst_aem_stat + stat_blocks;
	//cmr_u16 *dst_b = (float*)dst_aem_stat + stat_blocks * 2;
	double max_value = 1023;
	double sum = 0;
	double avg = 0;
	cmr_u32 i = 0;
	double r = 0, g = 0, b = 0;

	double mul_shift = 1;
	for (i = 0; i < aem_shift; i++)
		mul_shift = mul_shift * 2;

	if (bayer_pixels < 1)
		return AE_ERROR;

	for (i = 0; i < stat_blocks; i++) {
/*for r channel */
		sum = *src_r_stat++;
		sum = sum * mul_shift;
		avg = sum / bayer_pixels;
		r = avg > max_value ? max_value : avg;

/*for g channel */
		sum = *src_g_stat++;
		sum = sum * mul_shift;
		avg = sum / bayer_pixels;
		g = avg > max_value ? max_value : avg;

/*for b channel */
		sum = *src_b_stat++;
		sum = sum * mul_shift;
		avg = sum / bayer_pixels;
		b = avg > max_value ? max_value : avg;

		dst_r[i] = r;
		dst_g[i] = g;
		dst_b[i] = b;
	}

	return rtn;
}

static int getCenterMean(cmr_u32 * src_aem_stat,
						 float *dst_r, float *dst_g, float *dst_b, struct ae_size aem_blk_size, struct ae_size aem_blk_num, cmr_u8 aem_shift, float *rmean, float *gmean, float *bmean)
{
	int ret;
	ret = _aem_stat_preprocess2(src_aem_stat, dst_r, dst_g, dst_b, aem_blk_size, aem_blk_num, aem_shift);
	int i;
	int j;
	float rsum = 0;
	float gsum = 0;
	float bsum = 0;
	for (i = 13; i < 19; i++)
		for (j = 13; j < 19; j++) {
			int ind;
			ind = j * 32 + i;
			rsum += dst_r[ind];
			gsum += dst_g[ind];
			bsum += dst_b[ind];
		}
	*rmean = rsum / 36;
	*gmean = gsum / 36;
	*bmean = bsum / 36;
	return ret;
}

static void control_led(struct ae_ctrl_cxt *cxt, int onoff, int isMainflash, int led1, int led2)	//0~31
{
	UNUSED(cxt);
	UNUSED(onoff);
	UNUSED(isMainflash);
	UNUSED(led1);
	UNUSED(led2);

	ISP_LOGE("control_led %d %d %d %d", onoff, isMainflash, led1, led2);
	int type;
	struct ae_flash_cfg cfg;
	struct ae_flash_element element;
	if (isMainflash == 0)
		type = AE_FLASH_TYPE_PREFLASH;
	else
		type = AE_FLASH_TYPE_MAIN;

	if (onoff == 0) {
		cfg.led_idx = 0;
		cfg.type = type;
		cfg.led0_enable = 0;
		cfg.led1_enable = 0;
		cxt->isp_ops.flash_ctrl(cxt->isp_ops.isp_handler, &cfg, NULL);
	} else {
		int led1_driver_ind = led1;
		int led2_driver_ind = led2;
		if (led1_driver_ind < 0)
			led1_driver_ind = 0;
		if (led2_driver_ind < 0)
			led2_driver_ind = 0;

		cfg.led_idx = 1;
		cfg.type = type;
		element.index = led1_driver_ind;
		cxt->isp_ops.flash_set_charge(cxt->isp_ops.isp_handler, &cfg, &element);

		cfg.led_idx = 2;
		cfg.type = type;
		element.index = led2_driver_ind;
		cxt->isp_ops.flash_set_charge(cxt->isp_ops.isp_handler, &cfg, &element);

		if (led1 == -1)
			cfg.led0_enable = 0;
		else
			cfg.led0_enable = 1;

		if (led2 == -1)
			cfg.led1_enable = 0;
		else
			cfg.led1_enable = 1;

		cfg.type = type;
		cxt->isp_ops.flash_ctrl(cxt->isp_ops.isp_handler, &cfg, NULL);
	}

}

static cmr_s32 ae_round(float a)
{
	if (a > 0)
		return (cmr_s32) (a + 0.5);
	else
		return (cmr_s32) (a - 0.5);
}

void calRgbFrameData(int isMainFlash, float *rRaw, float *gRaw, float *bRaw, float *r, float *g, float *b)
{
	float rs = 0;
	float gs = 0;
	float bs = 0;
	int i;
	if (isMainFlash == 0) {
		for (i = 5; i < 15; i++) {
			rs += rRaw[i];
			gs += gRaw[i];
			bs += bRaw[i];
		}
		rs /= 10;
		gs /= 10;
		bs /= 10;
	} else {
		rs = (rRaw[5] + rRaw[6]) / 2;
		gs = (gRaw[5] + gRaw[6]) / 2;
		bs = (bRaw[5] + bRaw[6]) / 2;
	}
	*r = rs;
	*g = gs;
	*b = bs;

}

static double interp(double x1, double y1, double x2, double y2, double x)
{
	double y;
	if (x1 == x2)
		return (y2 + y1) / 2;
	y = y1 + (y2 - y1) / (x2 - x1) * (x - x1);
	return y;
}

//rgTab increasing
float interpCt(float *rgTab, float *ctTab, int numTab, float rg)
{
	int i;
	if (rg < rgTab[0]) {
		return ctTab[0];
	} else if (rg > rgTab[numTab - 1]) {
		return ctTab[numTab - 1];
	}

	for (i = 1; i < numTab; i++) {
		if (rgTab[i] > rg) {
			return interp(rgTab[i - 1], ctTab[i - 1], rgTab[i], ctTab[i], rg);
		}
	}

	return 10000.0;
}

void ShellSortFloatIncEx(float *data, int *data_dep, int len)
{
	int i, j;
	double tmp;
	int tmp2;
	int gap = len / 2;
	for (; gap > 0; gap = gap / 2) {
		for (i = gap; i < len; i++) {
			tmp = data[i];
			tmp2 = data_dep[i];
			for (j = i; j >= gap && tmp < data[j - gap]; j -= gap) {
				data[j] = data[j - gap];
				data_dep[j] = data_dep[j - gap];
			}
			data[j] = tmp;
			data_dep[j] = tmp2;
		}
	}
};

void removeIntArrayElementByInd(int n, int *arr, int ind)
{
	int i;
	for (i = ind; i < n - 1; i++) {
		arr[i] = arr[i + 1];
	}
}

void removeFloatArrayElementByInd(int n, float *arr, int ind)
{
	int i;
	for (i = ind; i < n - 1; i++) {
		arr[i] = arr[i + 1];
	}
}

void reduceFlashIndexTab(int n, int *indTab, float *maTab, float maMax, int nMax, int *outIndTab, float *outMaTab, int *outIndTabLen)
{
	nMax -= 1;
	int tempIndTab[256];
	float tempmADifTab[256];
	float tempmATab[256];
	int i;
	int nCur = 0;
	memset((void *)&tempIndTab[0], 0, sizeof(tempIndTab));
	for (i = 0; i < n; i++) {
		if (maTab[i] <= maMax) {
			tempIndTab[i] = indTab[i];
			tempmATab[i] = maTab[i];
			nCur = i + 1;
		}
	}
	while (nCur > nMax) {
		int indSort[256] = { 0 };
		for (i = 0; i < nCur - 2; i++) {
			int ind_1;
			int ind_p1;
			ind_1 = tempIndTab[i];
			ind_p1 = tempIndTab[i + 2];
			tempmADifTab[i] = -maTab[ind_1] + maTab[ind_p1];
			indSort[i] = i + 1;
		}
		ShellSortFloatIncEx(tempmADifTab, indSort, nCur - 2);
		removeIntArrayElementByInd(nCur, tempIndTab, indSort[0]);
		removeFloatArrayElementByInd(nCur, tempmATab, indSort[0]);
		nCur -= 1;
	}
	outIndTab[0] = -1;
	outMaTab[0] = 0;
	for (i = 0; i < nCur; i++) {
		outIndTab[i + 1] = tempIndTab[i];
		outMaTab[i + 1] = tempmATab[i];
	}
	*outIndTabLen = nCur + 1;
}

void readDebugBin2(const char *f, struct FCData *d)
{
	FILE *fp;
	fp = fopen(f, "rb");
	fread(d, 1, sizeof(struct FCData), fp);
	fclose(fp);
}

void readFCConfig(char *f, struct FCData *d, char *fout)
{
	int i;
	FILE *fp;
	fp = fopen(f, "rt");
	if (fp) {
		fscanf(fp, "%d", &d->numP1_hw);
		fscanf(fp, "%d", &d->numP2_hw);
		fscanf(fp, "%d", &d->numM1_hw);
		fscanf(fp, "%d", &d->numM2_hw);

		fscanf(fp, "%d", &d->numP1_hwSample);
		for (i = 0; i < d->numP1_hwSample; i++)
			fscanf(fp, "%d", &d->indP1_hwSample[i]);
		for (i = 0; i < d->numP1_hwSample; i++)
			fscanf(fp, "%f", &d->maP1_hwSample[i]);

		fscanf(fp, "%d", &d->numP2_hwSample);
		for (i = 0; i < d->numP2_hwSample; i++)
			fscanf(fp, "%d", &d->indP2_hwSample[i]);
		for (i = 0; i < d->numP2_hwSample; i++)
			fscanf(fp, "%f", &d->maP2_hwSample[i]);

		fscanf(fp, "%d", &d->numM1_hwSample);
		for (i = 0; i < d->numM1_hwSample; i++)
			fscanf(fp, "%d", &d->indM1_hwSample[i]);
		for (i = 0; i < d->numM1_hwSample; i++)
			fscanf(fp, "%f", &d->maM1_hwSample[i]);

		fscanf(fp, "%d", &d->numM2_hwSample);
		for (i = 0; i < d->numM2_hwSample; i++)
			fscanf(fp, "%d", &d->indM2_hwSample[i]);
		for (i = 0; i < d->numM2_hwSample; i++)
			fscanf(fp, "%f", &d->maM2_hwSample[i]);

		fscanf(fp, "%f", &d->mAMaxP1);
		fscanf(fp, "%f", &d->mAMaxP2);
		fscanf(fp, "%f", &d->mAMaxP12);
		fscanf(fp, "%f", &d->mAMaxM1);
		fscanf(fp, "%f", &d->mAMaxM2);
		fscanf(fp, "%f", &d->mAMaxM12);
		fclose(fp);
	}

	if (fout != 0) {
		fp = fopen(fout, "wt");
		fprintf(fp, "%d\n", d->numP1_hw);
		fprintf(fp, "%d\n", d->numP2_hw);
		fprintf(fp, "%d\n", d->numM1_hw);
		fprintf(fp, "%d\n", d->numM2_hw);

		fprintf(fp, "%d\n", d->numP1_hwSample);
		for (i = 0; i < d->numP1_hwSample; i++)
			fprintf(fp, "%d\t", d->indP1_hwSample[i]);
		fprintf(fp, "\n");
		for (i = 0; i < d->numP1_hwSample; i++)
			fprintf(fp, "%f\t", d->maP1_hwSample[i]);
		fprintf(fp, "\n");

		fprintf(fp, "%d\n", d->numP2_hwSample);
		for (i = 0; i < d->numP2_hwSample; i++)
			fprintf(fp, "%d\t", d->indP2_hwSample[i]);
		fprintf(fp, "\n");
		for (i = 0; i < d->numP2_hwSample; i++)
			fprintf(fp, "%f\t", d->maP2_hwSample[i]);
		fprintf(fp, "\n");

		fprintf(fp, "%d\n", d->numM1_hwSample);
		for (i = 0; i < d->numM1_hwSample; i++)
			fprintf(fp, "%d\t", d->indM1_hwSample[i]);
		fprintf(fp, "\n");
		for (i = 0; i < d->numM1_hwSample; i++)
			fprintf(fp, "%f\t", d->maM1_hwSample[i]);
		fprintf(fp, "\n");

		fprintf(fp, "%d\n", d->numM2_hwSample);
		for (i = 0; i < d->numM2_hwSample; i++)
			fprintf(fp, "%d\t", d->indM2_hwSample[i]);
		fprintf(fp, "\n");
		for (i = 0; i < d->numM2_hwSample; i++)
			fprintf(fp, "%f\t", d->maM2_hwSample[i]);
		fprintf(fp, "\n");

		fprintf(fp, "%f\n", d->mAMaxP1);
		fprintf(fp, "%f\n", d->mAMaxP2);
		fprintf(fp, "%f\n", d->mAMaxP12);
		fprintf(fp, "%f\n", d->mAMaxM1);
		fprintf(fp, "%f\n", d->mAMaxM2);
		fprintf(fp, "%f\n", d->mAMaxM12);
		fclose(fp);
	}

}

static void flashCalibration(struct ae_ctrl_cxt *cxt)
{
	struct ae_alg_calc_param *cur_status = &cxt->cur_status;
	static int frameCount = 0;
	static int caliState = FlashCali_start;
	static struct FCData *caliData = 0;
	int propValue[10];
	int propRet;
	propRet = get_prop_multi("persist.sys.isp.ae.flash_cali", 1, propValue);
	if (propRet <= 0)
		return;

	if (propValue[0] == 0) {
		control_led(cxt, 0, 0, 0, 0);
		caliState = FlashCali_start;
	}
	if (propValue[0] == 1) {

		ISP_LOGD("qqfc state=%d", caliState);
		if (caliState == FlashCali_start) {
			ISP_LOGD("qqfc FlashCali_start");
			caliData = (struct FCData *)malloc(sizeof(struct FCData));
			caliData->out.error = 0;

#ifdef WIN32
			readFCConfig("fc_config.txt", caliData, "fc_config_check.txt");
#else
			readFCConfig("/data/misc/cameraserver/fc_config.txt", caliData, "/data/misc/cameraserver/fc_config_check.txt");
#endif
			frameCount = 0;
			cxt->cur_status.settings.lock_ae = AE_STATE_LOCKED;	//lock ae

			reduceFlashIndexTab(caliData->numP1_hwSample, caliData->indP1_hwSample, caliData->maP1_hwSample, caliData->mAMaxP1, 32, caliData->indHwP1_alg, caliData->maHwP1_alg, &caliData->numP1_alg);
			reduceFlashIndexTab(caliData->numP2_hwSample, caliData->indP2_hwSample, caliData->maP2_hwSample, caliData->mAMaxP2, 32, caliData->indHwP2_alg, caliData->maHwP2_alg, &caliData->numP2_alg);
			reduceFlashIndexTab(caliData->numM1_hwSample, caliData->indM1_hwSample, caliData->maM1_hwSample, caliData->mAMaxM1, 32, caliData->indHwM1_alg, caliData->maHwM1_alg, &caliData->numM1_alg);
			reduceFlashIndexTab(caliData->numM2_hwSample, caliData->indM2_hwSample, caliData->maM2_hwSample, caliData->mAMaxM2, 32, caliData->indHwM2_alg, caliData->maHwM2_alg, &caliData->numM2_alg);

			//gen test
			int i;
			int id = 0;
			//preflash1
			caliData->expReset[id] = 1;
			caliData->ind1Tab[id] = 0;
			caliData->ind2Tab[id] = 0;
			caliData->testMinFrm[id] = 0;
			caliData->isMainTab[id] = 0;
			id++;
			for (i = 1; i < caliData->numP1_alg; i++) {
				caliData->expReset[id] = 0;
				caliData->ind1Tab[id] = i;
				caliData->ind2Tab[id] = 0;
				caliData->testMinFrm[id] = 0;
				caliData->isMainTab[id] = 0;
				id++;
			}
			//preflash2
			caliData->expReset[id] = 1;
			caliData->ind1Tab[id] = 0;
			caliData->ind2Tab[id] = 0;
			caliData->testMinFrm[id] = 0;
			caliData->isMainTab[id] = 0;
			id++;
			for (i = 1; i < caliData->numP2_alg; i++) {
				caliData->expReset[id] = 0;
				caliData->ind1Tab[id] = 0;
				caliData->ind2Tab[id] = i;
				caliData->testMinFrm[id] = 0;
				caliData->isMainTab[id] = 0;
				id++;
			}
			//m1
			caliData->expReset[id] = 1;
			caliData->ind1Tab[id] = 0;
			caliData->ind2Tab[id] = 0;
			caliData->testMinFrm[id] = 0;
			caliData->isMainTab[id] = 0;
			id++;
			for (i = 1; i < caliData->numM1_alg; i++) {
				caliData->expReset[id] = 0;
				caliData->ind1Tab[id] = i;
				caliData->ind2Tab[id] = 0;
				caliData->testMinFrm[id] = 120;
				caliData->isMainTab[id] = 1;
				id++;
			}
			//m2
			caliData->expReset[id] = 1;
			caliData->ind1Tab[id] = 0;
			caliData->ind2Tab[id] = 0;
			caliData->testMinFrm[id] = 0;
			caliData->isMainTab[id] = 0;
			id++;
			for (i = 1; i < caliData->numM2_alg; i++) {
				caliData->expReset[id] = 0;
				caliData->ind1Tab[id] = 0;
				caliData->ind2Tab[id] = i;
				caliData->testMinFrm[id] = 120;
				caliData->isMainTab[id] = 1;
				id++;
			}
			caliData->testIndAll = id;

			caliData->expTimeBase = 0.05 * AEC_LINETIME_PRECESION;
			caliData->expTime = 0.05 * AEC_LINETIME_PRECESION;
			caliData->gainBase = 8 * 128;
			caliData->gain = 8 * 128;

			//caliData->stateCaliFrameCntSt = frameCount + 1;
			//caliState = FlashCali_cali;

			caliData->stateAeFrameCntSt = 1;
			caliState = FlashCali_ae;

		} else if (caliState == FlashCali_ae) {
			int frm = frameCount - caliData->stateAeFrameCntSt;
			if ((frm % 3 == 0) && (frm != 0)) {
				float rmean;
				float gmean;
				float bmean;
				struct ae_alg_calc_param *current_status = &cxt->sync_cur_status;
				getCenterMean((cmr_u32 *) & cxt->sync_aem[0],
							  caliData->rBuf, caliData->gBuf, caliData->bBuf, cxt->monitor_cfg.blk_size, cxt->monitor_cfg.blk_num, current_status->monitor_shift, &rmean, &gmean, &bmean);
				ISP_LOGD("qqfc AE frmCnt=%d sh,gain=%d %d, gmean=%f", (int)frameCount, (int)caliData->expTime, (int)caliData->gain, gmean);
				if ((gmean > 200 && gmean < 400) || (caliData->expTime == 0.05 * AEC_LINETIME_PRECESION && caliData->gain == 8 * 128)) {
					caliData->stateCaliFrameCntSt = frameCount + 1;
					caliData->expTimeBase = caliData->expTime;
					caliData->gainBase = caliData->gain;
					caliState = FlashCali_cali;
				} else {
					if (gmean < 10) {
						caliData->expTime *= 25;
						caliData->gain = caliData->gain;
					} else {
						caliData->expTime *= 300 / gmean;
						caliData->gain = caliData->gain;
					}
					if (caliData->expTime > 0.05 * AEC_LINETIME_PRECESION) {
						float ratio = caliData->expTime / (0.05 * AEC_LINETIME_PRECESION);
						caliData->expTime = 0.05 * AEC_LINETIME_PRECESION;
						caliData->gain *= ratio;
					} else if (caliData->expTime < 0.001 * AEC_LINETIME_PRECESION) {
						float ratio = caliData->expTime / (0.001 * AEC_LINETIME_PRECESION);
						caliData->expTime = 0.001 * AEC_LINETIME_PRECESION;
						caliData->gain *= ratio;
					}
					if (caliData->gain < 128)
						caliData->gain = 128;
					else if (caliData->gain > 8 * 128)
						caliData->gain = 8 * 128;
				}
			}

		} else if (caliState == FlashCali_cali) {
			int frmCntState;
			int frmCnt;
			frmCntState = frameCount - caliData->stateCaliFrameCntSt;
			if (frmCntState == 0) {
				caliData->testInd = 0;
				caliData->stateCaliFrameCntStSub = caliData->stateCaliFrameCntSt;
			}

			frmCnt = frameCount - caliData->stateCaliFrameCntStSub;
			ISP_LOGD("qqfc caliData->testInd=%d", caliData->testInd);
			if (frmCnt == 0) {
				int id;
				id = caliData->testInd;
				int led1;
				int led2;
				int led1_hw;
				int led2_hw;
				led1 = caliData->ind1Tab[id];
				led2 = caliData->ind2Tab[id];
				if (caliData->isMainTab[id] == 0) {
					led1_hw = caliData->indHwP1_alg[led1];
					led2_hw = caliData->indHwP2_alg[led2];
				} else {
					led1_hw = caliData->indHwM1_alg[led1];
					led2_hw = caliData->indHwM1_alg[led2];
				}
				if (caliData->expReset[id] == 1) {
					caliData->expTime = caliData->expTimeBase;
					caliData->gain = caliData->gainBase;
				}
				caliData->expTab[id] = caliData->expTime;
				caliData->gainTab[id] = caliData->gain;
				if (led1 == 0 && led2 == 0)
					control_led(cxt, 0, 0, 0, 0);
				else
					control_led(cxt, 1, caliData->isMainTab[id], led1_hw, led2_hw);
			} else if (frmCnt == 3) {
				float rmean;
				float gmean;
				float bmean;
				struct ae_alg_calc_param *current_status = &cxt->sync_cur_status;
				getCenterMean((cmr_u32 *) & cxt->sync_aem[0],
							  caliData->rBuf, caliData->gBuf, caliData->bBuf, cxt->monitor_cfg.blk_size, cxt->monitor_cfg.blk_num, current_status->monitor_shift, &rmean, &gmean, &bmean);
				if (gmean > 600) {

					double rat = 2;
					rat = gmean / 300.0;
					double rat1;
					double rat2;
					double rat3;

					if (caliData->expTime > 0.03 * AEC_LINETIME_PRECESION) {
						rat1 = rat;
						int expTest;
						expTest = caliData->expTime / rat1;
						if (expTest < 0.03 * AEC_LINETIME_PRECESION)
							expTest = 0.03 * AEC_LINETIME_PRECESION;
						rat1 = (double)caliData->expTime / expTest;
						caliData->expTime = expTest;
					} else
						rat1 = 1;

					if (caliData->gain > 128) {
						rat2 = rat / rat1;
						int gainTest = caliData->gain;
						gainTest /= rat2;
						if (gainTest < 128)
							gainTest = 128;
						rat2 = (double)caliData->gain / gainTest;
						caliData->gain = gainTest;
					} else {
						rat2 = 1;
					}
					{
						rat3 = rat / rat1 / rat2;
						int expTest;
						expTest = caliData->expTime / rat3;
						if (expTest < 0.001 * AEC_LINETIME_PRECESION)
							expTest = 0.001 * AEC_LINETIME_PRECESION;
						rat3 = (double)caliData->expTime / expTest;
						caliData->expTime = expTest;

						if (expTest == 0.001 * AEC_LINETIME_PRECESION && caliData->gain == 128) {
							char err[] = "error: the exposure is min\n";
#ifdef WIN32
							printf(err);
#endif
							ISP_LOGE("%s", err);
							caliData->out.error = FlashCali_too_close;

						} else {
							caliData->stateCaliFrameCntStSub = frameCount + 1;
						}
					}
				} else {
					caliData->rFrame[caliData->testInd][frmCnt] = rmean;
					caliData->gFrame[caliData->testInd][frmCnt] = gmean;
					caliData->bFrame[caliData->testInd][frmCnt] = bmean;
				}
			}
			if (frmCnt < 15) {
				float rmean;
				float gmean;
				float bmean;
				struct ae_alg_calc_param *current_status = &cxt->sync_cur_status;
				getCenterMean((cmr_u32 *) & cxt->sync_aem[0],
							  caliData->rBuf, caliData->gBuf, caliData->bBuf, cxt->monitor_cfg.blk_size, cxt->monitor_cfg.blk_num, current_status->monitor_shift, &rmean, &gmean, &bmean);
				caliData->rFrame[caliData->testInd][frmCnt] = rmean;
				caliData->gFrame[caliData->testInd][frmCnt] = gmean;
				caliData->bFrame[caliData->testInd][frmCnt] = bmean;

			} else if (frmCnt > caliData->testMinFrm[caliData->testInd]) {
				control_led(cxt, 0, 0, 0, 0);
				float r;
				float g;
				float b;
				calRgbFrameData(caliData->isMainTab[caliData->testInd], caliData->rFrame[caliData->testInd], caliData->gFrame[caliData->testInd], caliData->bFrame[caliData->testInd], &r, &g, &b);
				caliData->rData[caliData->testInd] = r;
				caliData->gData[caliData->testInd] = g;
				caliData->bData[caliData->testInd] = b;
				caliData->testInd++;
				caliData->stateCaliFrameCntStSub = frameCount + 1;
				if (caliData->testInd == caliData->testIndAll)
					caliState = FlashCali_end;
			}

		} else if (caliState == FlashCali_end) {
			//readDebugBin2("d:\\temp\\fc_debug.bin", caliData);
			int i;
			int j;
			float rTab1[32];
			float gTab1[32];
			float bTab1[32];
			float rTab2[32];
			float gTab2[32];
			float bTab2[32];
			float rTab1Main[32];
			float gTab1Main[32];
			float bTab1Main[32];
			float rTab2Main[32];
			float gTab2Main[32];
			float bTab2Main[32];

			double rbase;
			double gbase;
			double bbase;
			rbase = caliData->rData[0];
			gbase = caliData->gData[0];
			bbase = caliData->bData[0];
			for (i = 0; i < 32; i++) {
				gTab1[i] = -1;
				gTab2[i] = -1;
				gTab1Main[i] = -1;
				gTab2Main[i] = -1;
			}
			rTab1[0] = 0;
			gTab1[0] = 0;
			bTab1[0] = 0;
			rTab2[0] = 0;
			gTab2[0] = 0;
			bTab2[0] = 0;
			rTab1Main[0] = 0;
			gTab1Main[0] = 0;
			bTab1Main[0] = 0;
			rTab2Main[0] = 0;
			gTab2Main[0] = 0;
			bTab2Main[0] = 0;
			double maxV = 0;
			for (i = 0; i < caliData->testIndAll; i++) {
				double rat;
				rat = (double)caliData->expTimeBase * caliData->gainBase / caliData->gainTab[i] / caliData->expTab[i];

				int id1;
				int id2;
				id1 = caliData->ind1Tab[i];
				id2 = caliData->ind2Tab[i];

				float r;
				float g;
				float b;

				r = caliData->rData[i] * rat - rbase;
				g = caliData->gData[i] * rat - gbase;
				b = caliData->bData[i] * rat - bbase;
				if (r < 0 || g < 0 || b < 0) {
					r = 0;
					g = 0;
					b = 0;
				}

				if (maxV < g)
					maxV = g;
				if (caliData->ind1Tab[i] != 0 && caliData->isMainTab[i]) {
					rTab1Main[id1] = r;
					gTab1Main[id1] = g;
					bTab1Main[id1] = b;
				} else if (caliData->ind1Tab[i] != 0 && caliData->isMainTab[i] == 0) {
					rTab1[id1] = r;
					gTab1[id1] = g;
					bTab1[id1] = b;
				} else if (caliData->ind2Tab[i] != 0 && caliData->isMainTab[i]) {
					rTab2Main[id2] = r;
					gTab2Main[id2] = g;
					bTab2Main[id2] = b;
				} else if (caliData->ind2Tab[i] != 0 && caliData->isMainTab[i] == 0) {
					rTab2[id2] = r;
					gTab2[id2] = g;
					bTab2[id2] = b;
				}
			}
			double sc = 0;
			if (maxV > 0.0) {
				sc = 65536.0 / maxV / 2;
			} else {
				ISP_LOGW("maxV is invalidated, %f", maxV);
			}
			for (i = 0; i < 32; i++) {
				rTab1[i] *= sc;
				gTab1[i] *= sc;
				bTab1[i] *= sc;
				rTab2[i] *= sc;
				gTab2[i] *= sc;
				bTab2[i] *= sc;
				rTab1Main[i] *= sc;
				gTab1Main[i] *= sc;
				bTab1Main[i] *= sc;
				rTab2Main[i] *= sc;
				gTab2Main[i] *= sc;
				bTab2Main[i] *= sc;
			}

			caliData->out.version = CALI_VERSION;
			caliData->out.version_sub = CALI_VERSION_SUB;
			strcpy(caliData->out.name, "flash_calibration_data");

			for (i = 0; i < 32; i++) {
				caliData->out.driverIndexP1[i] = -1;
				caliData->out.driverIndexP2[i] = -1;
				caliData->out.driverIndexM1[i] = -1;
				caliData->out.driverIndexM2[i] = -1;
			}
			for (i = 0; i < caliData->numP1_alg; i++)
				caliData->out.driverIndexP1[i] = caliData->indHwP1_alg[i];
			for (i = 0; i < caliData->numP2_alg; i++)
				caliData->out.driverIndexP2[i] = caliData->indHwP2_alg[i];
			for (i = 0; i < caliData->numM1_alg; i++)
				caliData->out.driverIndexM1[i] = caliData->indHwM1_alg[i];
			for (i = 0; i < caliData->numM2_alg; i++)
				caliData->out.driverIndexM2[i] = caliData->indHwM2_alg[i];
			for (i = 0; i < caliData->numP1_alg; i++)
				caliData->out.maP1[i] = ae_round(caliData->maHwP1_alg[i]);
			for (i = 0; i < caliData->numP2_alg; i++)
				caliData->out.maP2[i] = ae_round(caliData->maHwP2_alg[i]);
			for (i = 0; i < caliData->numM1_alg; i++)
				caliData->out.maM1[i] = ae_round(caliData->maHwM1_alg[i]);
			for (i = 0; i < caliData->numM2_alg; i++)
				caliData->out.maM2[i] = ae_round(caliData->maHwM2_alg[i]);

			caliData->out.numP1_hwSample = caliData->numP1_hwSample;
			caliData->out.numP2_hwSample = caliData->numP2_hwSample;
			caliData->out.numM1_hwSample = caliData->numM1_hwSample;
			caliData->out.numM2_hwSample = caliData->numM2_hwSample;
			caliData->out.mAMaxP1 = ae_round(caliData->mAMaxP1);
			caliData->out.mAMaxP2 = ae_round(caliData->mAMaxP2);
			caliData->out.mAMaxP12 = ae_round(caliData->mAMaxP12);
			caliData->out.mAMaxM1 = ae_round(caliData->mAMaxM1);
			caliData->out.mAMaxM2 = ae_round(caliData->mAMaxM2);
			caliData->out.mAMaxM12 = ae_round(caliData->mAMaxM12);

			//--------------------------
			//--------------------------
			//@@

			float rgtab[20];
			float cttab[20];
			for (i = 0; i < 20; i++) {
				rgtab[i] = cxt->ctTabRg[i];
				cttab[i] = cxt->ctTab[i];
			}

			//--------------------------
			//--------------------------
			//Preflash ct
			//preflash brightness
			for (i = 0; i < 1024; i++) {
				caliData->out.preflashCt[i] = 0;
				caliData->out.preflashBrightness[i] = 0;
			}

			float maxRg = 0;
			for (i = 0; i < caliData->numP1_alg; i++) {
				if (maxRg < gTab1[i])
					maxRg = gTab1[i];
				if (maxRg < rTab1[i])
					maxRg = rTab1[i];
			}
			for (i = 0; i < caliData->numP2_alg; i++) {
				if (maxRg < gTab2[i])
					maxRg = gTab2[i];
				if (maxRg < rTab2[i])
					maxRg = rTab2[i];
			}
			float ratMul;
			ratMul = maxRg / 65535.0;

			caliData->out.rgRatPf = ratMul;

			for (i = 0; i < caliData->numP1_alg; i++) {
				caliData->out.rPf1[i] = ae_round(rTab1[i] * 65535.0 / maxRg);
				caliData->out.gPf1[i] = ae_round(gTab1[i] * 65535.0 / maxRg);
			}
			for (i = 0; i < caliData->numP2_alg; i++) {
				caliData->out.rPf2[i] = ae_round(rTab2[i] * 65535.0 / maxRg);
				caliData->out.gPf2[i] = ae_round(gTab2[i] * 65535.0 / maxRg);
			}
			for (i = 0; i < 20; i++) {
				caliData->out.rgTab[i] = rgtab[i];
				caliData->out.ctTab[i] = cttab[i];

			}

#if 0
			for (j = 0; j < caliData->numP2_alg; j++)
				for (i = 0; i < caliData->numP1_alg; i++) {
					int ind;
					ind = j * 32 + i;
					float ma1;
					float ma2;
					ma1 = caliData->maHwP1_alg[i];
					ma2 = caliData->maHwP1_alg[j];
					if (ma1 + ma2 <= caliData->mAMaxP12) {
						double rg;
						caliData->out.preflashCt[ind] = 0;
						if (gTab1[i] + gTab2[j] != 0) {
							rg = (rTab1[i] + rTab2[j]) / (gTab1[i] + gTab2[j]);
							caliData->out.preflashCt[ind] = interpCt(rgtab, cttab, 20, rg);
						}
						caliData->out.preflashBrightness[ind] = ae_round(gTab1[i] + gTab2[j]);
					} else {
						caliData->out.preflashCt[ind] = 0;
						caliData->out.preflashBrightness[ind] = 0;
					}
				}
#else
			for (j = 0; j < caliData->numP2_alg; j++)
				for (i = 0; i < caliData->numP1_alg; i++) {
					int ind;
					ind = j * 32 + i;
					float ma1;
					float ma2;
					ma1 = ae_round(caliData->maHwP1_alg[i]);
					ma2 = ae_round(caliData->maHwP1_alg[j]);
					if (ma1 + ma2 <= ae_round(caliData->mAMaxP12)) {
						double rg;
						caliData->out.preflashCt[ind] = 0;

						int rPf1;
						int gPf1;
						int rPf2;
						int gPf2;
						rPf1 = caliData->out.rPf1[i];
						gPf1 = caliData->out.gPf1[i];
						rPf2 = caliData->out.rPf2[j];
						gPf2 = caliData->out.gPf2[j];
						if (gPf1 + gPf2 != 0) {
							rg = (rPf1 + rPf2) / (double)(gPf1 + gPf2);
							caliData->out.preflashCt[ind] = interpCt(rgtab, cttab, 20, rg);
						}
						caliData->out.preflashBrightness[ind] = ae_round((gPf1 + gPf2) * caliData->out.rgRatPf);
					} else {
						caliData->out.preflashCt[ind] = 0;
						caliData->out.preflashBrightness[ind] = 0;
					}
				}
#endif
			for (i = 0; i < 1024; i++) {
				caliData->out.brightnessTable[i] = 0;
				caliData->out.rTable[i] = 0;
				caliData->out.bTable[i] = 0;
			}
			//mainflash brightness
			//mainflash r/b table
			for (i = 0; i < caliData->numM1_alg; i++)
				for (j = 0; j < caliData->numM2_alg; j++) {
					int ind;
					ind = j * 32 + i;
					caliData->out.brightnessTable[ind] = 0;
					if (gTab1Main[i] != -1 && gTab2Main[j] != -1) {
						caliData->out.brightnessTable[ind] = ae_round(gTab1Main[i] + gTab2Main[j]);
						double r;
						double g;
						double b;
						r = rTab1Main[i] + rTab2Main[j];
						g = gTab1Main[i] + gTab2Main[j];
						b = bTab1Main[i] + bTab2Main[j];
						if (r > 0 && g > 0 && b > 0) {
							caliData->out.rTable[ind] = ae_round(1024 * g / r);
							caliData->out.bTable[ind] = ae_round(1024 * g / b);
						} else {
							caliData->out.brightnessTable[ind] = 0;
							caliData->out.rTable[ind] = 0;
							caliData->out.bTable[ind] = 0;
						}
					} else {
						caliData->out.brightnessTable[ind] = 0;
						caliData->out.rTable[ind] = 0;
						caliData->out.bTable[ind] = 0;
					}
				}
			//flash mask
			for (i = 0; i < 1024; i++) {
				caliData->out.flashMask[i] = 0;
			}
			for (j = 0; j < caliData->numM2_alg; j++)
				for (i = 0; i < caliData->numM1_alg; i++) {
					int ind;
					ind = j * 32 + i;
					float ma1;
					float ma2;
					ma1 = ae_round(caliData->maHwM1_alg[i]);
					ma2 = ae_round(caliData->maHwM2_alg[j]);
					if (ma1 + ma2 < ae_round(caliData->mAMaxM12))
						caliData->out.flashMask[ind] = 1;
					else
						caliData->out.flashMask[ind] = 0;
				}
			caliData->out.flashMask[0] = 0;

			{
				FILE *fp;
#ifdef WIN32
				fp = fopen("d:\\temp\\flash_led_brightness.bin", "wb");
#else
				fp = fopen("/data/misc/cameraserver/flash_led_brightness.bin", "wb");
#endif
				struct flash_led_brightness led_bri;
				memset(&led_bri, 0, sizeof(struct flash_led_brightness));
				led_bri.levelNumber_pf1 = caliData->numP1_alg - 1;
				led_bri.levelNumber_pf2 = caliData->numP2_alg - 1;
				led_bri.levelNumber_mf1 = caliData->numM1_alg - 1;
				led_bri.levelNumber_mf2 = caliData->numM2_alg - 1;
				for (i = 0; i < led_bri.levelNumber_pf1; i++)
					led_bri.ledBrightness_pf1[i] = caliData->out.preflashBrightness[i + 1];
				for (i = 0; i < led_bri.levelNumber_pf2; i++)
					led_bri.ledBrightness_pf2[i] = caliData->out.preflashBrightness[(i + 1) * 32];
				for (i = 0; i < led_bri.levelNumber_mf1; i++)
					led_bri.ledBrightness_mf1[i] = caliData->out.brightnessTable[i + 1];
				for (i = 0; i < led_bri.levelNumber_mf2; i++)
					led_bri.ledBrightness_mf2[i] = caliData->out.brightnessTable[(i + 1) * 32];

				for (i = 0; i < led_bri.levelNumber_pf1; i++)
					led_bri.index_pf1[i] = caliData->indHwP1_alg[i + 1];
				for (i = 0; i < led_bri.levelNumber_pf2; i++)
					led_bri.index_pf2[i] = caliData->indHwP2_alg[i + 1];
				for (i = 0; i < led_bri.levelNumber_mf1; i++)
					led_bri.index_mf1[i] = caliData->indHwM1_alg[i + 1];
				for (i = 0; i < led_bri.levelNumber_mf2; i++)
					led_bri.index_mf2[i] = caliData->indHwM2_alg[i + 1];
				fwrite(&led_bri, 1, sizeof(struct flash_led_brightness), fp);
				fclose(fp);

				struct flash_g_frames gf;
				memset(&gf, 0, sizeof(struct flash_g_frames));
				gf.levelNumber_pf1 = led_bri.levelNumber_pf1;
				gf.levelNumber_pf2 = led_bri.levelNumber_pf2;
				gf.levelNumber_mf1 = led_bri.levelNumber_mf1;
				gf.levelNumber_mf2 = led_bri.levelNumber_mf2;

				for (i = 0; i < 32; i++) {
					gf.index_pf1[i] = led_bri.index_pf1[i];
					gf.index_pf2[i] = led_bri.index_pf2[i];
					gf.index_mf1[i] = led_bri.index_mf1[i];
					gf.index_mf2[i] = led_bri.index_mf2[i];
				}
				for (j = 0; j < 15; j++)
					gf.g_off[j] = caliData->gFrame[0][j];
				gf.gain_off = caliData->gainTab[0];
				gf.shutter_off = caliData->expTab[0];
				for (i = 0; i < caliData->testIndAll; i++) {
					int ind;
					if (caliData->ind1Tab[i] != 0 && caliData->isMainTab[i]) {
						ind = caliData->ind1Tab[i] - 1;
						for (j = 0; j < 15; j++)
							gf.g_mf1[ind][j] = caliData->gFrame[i][j];
						gf.gain_mf1[ind] = caliData->gainTab[i];
						gf.shutter_mf1[ind] = caliData->expTab[i];
					} else if (caliData->ind1Tab[i] != 0 && caliData->isMainTab[i] == 0) {
						ind = caliData->ind1Tab[i] - 1;
						for (j = 0; j < 15; j++)
							gf.g_pf1[ind][j] = caliData->gFrame[i][j];
						gf.gain_pf1[ind] = caliData->gainTab[i];
						gf.shutter_pf1[ind] = caliData->expTab[i];
					} else if (caliData->ind2Tab[i] != 0 && caliData->isMainTab[i]) {
						ind = caliData->ind2Tab[i] - 1;
						for (j = 0; j < 15; j++)
							gf.g_mf2[ind][j] = caliData->gFrame[i][j];
						gf.gain_mf2[ind] = caliData->gainTab[i];
						gf.shutter_mf2[ind] = caliData->expTab[i];
					} else if (caliData->ind2Tab[i] != 0 && caliData->isMainTab[i] == 0) {
						ind = caliData->ind2Tab[i] - 1;
						for (j = 0; j < 15; j++)
							gf.g_pf2[ind][j] = caliData->gFrame[i][j];
						gf.gain_pf2[ind] = caliData->gainTab[i];
						gf.shutter_pf2[ind] = caliData->expTab[i];
					}
				}
#ifdef WIN32
				fp = fopen("d:\\temp\\flash_g_frames.bin", "wb");
#else
				fp = fopen("/data/misc/cameraserver/flash_g_frames.bin", "wb");
#endif
				fwrite(&gf, 1, sizeof(struct flash_g_frames), fp);
				fclose(fp);

#ifdef WIN32
				fp = fopen("d:\\temp\\flash_g_frames.txt", "wt");

				fprintf(fp, "levelNumber_pf1: %d\n", (int)gf.levelNumber_pf1);
				fprintf(fp, "levelNumber_pf2: %d\n", (int)gf.levelNumber_pf2);
				fprintf(fp, "levelNumber_mf1: %d\n", (int)gf.levelNumber_mf1);
				fprintf(fp, "levelNumber_mf2: %d\n", (int)gf.levelNumber_mf2);

				fprintf(fp, "index_pf1:\n");
				for (i = 0; i < 32; i++)
					fprintf(fp, "%d\t", (int)gf.index_pf1[i]);
				fprintf(fp, "\n");

				fprintf(fp, "index_pf2:\n");
				for (i = 0; i < 32; i++)
					fprintf(fp, "%d\t", (int)gf.index_pf2[i]);
				fprintf(fp, "\n");

				fprintf(fp, "index_mf1:\n");
				for (i = 0; i < 32; i++)
					fprintf(fp, "%d\t", (int)gf.index_mf1[i]);
				fprintf(fp, "\n");

				fprintf(fp, "index_mf2:\n");
				for (i = 0; i < 32; i++)
					fprintf(fp, "%d\t", (int)gf.index_mf2[i]);
				fprintf(fp, "\n");

				fprintf(fp, "shutter_off: %lf\n", (double)gf.shutter_off);
				fprintf(fp, "shutter_pf1:\n");
				for (i = 0; i < 32; i++)
					fprintf(fp, "%lf\t", (double)gf.shutter_pf1[i]);
				fprintf(fp, "\n");

				fprintf(fp, "shutter_pf2:\n");
				for (i = 0; i < 32; i++)
					fprintf(fp, "%lf\t", (double)gf.shutter_pf2[i]);
				fprintf(fp, "\n");

				fprintf(fp, "shutter_mf1:\n");
				for (i = 0; i < 32; i++)
					fprintf(fp, "%lf\t", (double)gf.shutter_mf1[i]);
				fprintf(fp, "\n");

				fprintf(fp, "shutter_mf2:\n");
				for (i = 0; i < 32; i++)
					fprintf(fp, "%lf\t", (double)gf.shutter_mf2[i]);
				fprintf(fp, "\n");

				fprintf(fp, "gain_off: %d\n", (int)gf.gain_off);
				fprintf(fp, "gain_pf1:\n");
				for (i = 0; i < 32; i++)
					fprintf(fp, "%d\t", (int)gf.gain_pf1[i]);
				fprintf(fp, "\n");

				fprintf(fp, "gain_pf2:\n");
				for (i = 0; i < 32; i++)
					fprintf(fp, "%d\t", (int)gf.gain_pf2[i]);
				fprintf(fp, "\n");

				fprintf(fp, "gain_mf1:\n");
				for (i = 0; i < 32; i++)
					fprintf(fp, "%d\t", (int)gf.gain_mf1[i]);
				fprintf(fp, "\n");

				fprintf(fp, "gain_mf2:\n");
				for (i = 0; i < 32; i++)
					fprintf(fp, "%d\t", (int)gf.gain_mf2[i]);
				fprintf(fp, "\n");

				fprintf(fp, "g_off:\n");
				for (j = 0; j < 15; j++)
					fprintf(fp, "%lf\t", (double)gf.g_off[j]);
				fprintf(fp, "\n");

				fprintf(fp, "g_pf1:\n");
				for (i = 0; i < 32; i++) {
					for (j = 0; j < 15; j++)
						fprintf(fp, "%lf\t", (double)gf.g_pf1[i][j]);
					fprintf(fp, "\n");
				}

				fprintf(fp, "g_pf2:\n");
				for (i = 0; i < 32; i++) {
					for (j = 0; j < 15; j++)
						fprintf(fp, "%lf\t", (double)gf.g_pf2[i][j]);
					fprintf(fp, "\n");
				}

				fprintf(fp, "g_mf1:\n");
				for (i = 0; i < 32; i++) {
					for (j = 0; j < 15; j++)
						fprintf(fp, "%lf\t", (double)gf.g_mf1[i][j]);
					fprintf(fp, "\n");
				}

				fprintf(fp, "g_mf2:\n");
				for (i = 0; i < 32; i++) {
					for (j = 0; j < 15; j++)
						fprintf(fp, "%lf\t", (double)gf.g_mf2[i][j]);
					fprintf(fp, "\n");
				}
				fclose(fp);
#endif
			}

			caliData->numM1_alg = 32;
			caliData->numM2_alg = 32;
			caliData->numP1_alg = 32;
			caliData->numP2_alg = 32;
			caliData->out.preflashLevelNum1 = caliData->numP1_alg;
			caliData->out.preflashLevelNum2 = caliData->numP2_alg;
			caliData->out.flashLevelNum1 = caliData->numM1_alg;
			caliData->out.flashLevelNum2 = caliData->numM2_alg;

			FILE *fp;

			int propValue[10];
			int propRet;
			propRet = get_prop_multi("persist.sys.isp.ae.fc_debug", 1, propValue);

			int debug1En = 0;
			int debug2En = 0;
			propRet = get_prop_multi("persist.sys.isp.ae.fc_debug", 1, propValue);
			if (propRet >= 1)
				debug1En = propValue[0];
			propRet = get_prop_multi("persist.sys.isp.ae.fc_debug2", 1, propValue);
			if (propRet >= 1)
				debug2En = propValue[0];

			if (debug1En == 1) {
#ifdef WIN32
				fp = fopen("d:\\temp\\fc_frame_rgb.txt", "wt");
#else
				fp = fopen("/data/misc/cameraserver/fc_frame_rgb.txt", "wt");
#endif
				for (i = 0; i < caliData->testIndAll; i++) {
					if (caliData->isMainTab[i] == 0)
						fprintf(fp, "pf ");
					else
						fprintf(fp, "mf ");

					int led1;
					int led2;
					led1 = caliData->ind1Tab[i];
					led2 = caliData->ind2Tab[i];
					int led1_hw;
					int led2_hw;
					if (caliData->isMainTab[i] == 0) {
						led1_hw = caliData->indHwP1_alg[led1];
						led2_hw = caliData->indHwP2_alg[led2];
					} else {
						led1_hw = caliData->indHwM1_alg[led1];
						led2_hw = caliData->indHwM1_alg[led2];
					}
					fprintf(fp, "ind1,ind2: %d\t%d\n",
							//(int)caliData->ind1Tab[i],
							//(int)caliData->ind2Tab[i]);
							(int)led1_hw, (int)led2_hw);

					fprintf(fp, "expBase,gainBase,exp,gain: %d\t%d\t%d\t%d\n", (int)caliData->expTimeBase, (int)caliData->gainBase, (int)caliData->expTab[i], (int)caliData->gainTab[i]);
					int j;
					for (j = 0; j < 15; j++)
						fprintf(fp, "%lf\t%lf\t%lf\t\n", (double)caliData->rFrame[i][j], (double)caliData->gFrame[i][j], (double)caliData->bFrame[i][j]);
					fprintf(fp, "============\n\n");

				}
				fclose(fp);
			}
			if (debug2En == 1) {
#ifdef WIN32
				fp = fopen("d:\\temp\\fc_debug.txt", "wt");
#else
				fp = fopen("/data/misc/cameraserver/fc_debug.txt", "wt");
#endif
				fprintf(fp, "\ndriver_ind\n");
				for (i = 0; i < caliData->numP1_alg; i++)
					fprintf(fp, "%d\t", caliData->out.driverIndexP1[i]);
				fprintf(fp, "\n");

				for (i = 0; i < caliData->numP2_alg; i++)
					fprintf(fp, "%d\t", caliData->out.driverIndexP2[i]);
				fprintf(fp, "\n");

				for (i = 0; i < caliData->numM1_alg; i++)
					fprintf(fp, "%d\t", caliData->out.driverIndexM1[i]);
				fprintf(fp, "\n");

				for (i = 0; i < caliData->numM2_alg; i++)
					fprintf(fp, "%d\t", caliData->out.driverIndexM2[i]);
				fprintf(fp, "\n");

				fprintf(fp, "\nmask\n");
				for (j = 0; j < caliData->numM2_alg; j++) {
					for (i = 0; i < caliData->numM1_alg; i++) {
						int ind;
						ind = j * caliData->numM1_alg + i;
						fprintf(fp, "%d\t", (int)caliData->out.flashMask[ind]);
					}
					fprintf(fp, "\n");
				}
				fprintf(fp, "\nbrightness\n");
				for (j = 0; j < caliData->numM2_alg; j++) {
					for (i = 0; i < caliData->numM1_alg; i++) {
						int ind;
						ind = j * caliData->numM1_alg + i;
						fprintf(fp, "%d\t", (int)caliData->out.brightnessTable[ind]);
					}
					fprintf(fp, "\n");
				}
				fprintf(fp, "\nr tab\n");
				for (j = 0; j < caliData->numM2_alg; j++) {
					for (i = 0; i < caliData->numM1_alg; i++) {
						int ind;
						ind = j * caliData->numM1_alg + i;
						fprintf(fp, "%d\t", (int)caliData->out.rTable[ind]);
					}
					fprintf(fp, "\n");
				}
				fprintf(fp, "\nb tab\n");
				for (j = 0; j < caliData->numM2_alg; j++) {
					for (i = 0; i < caliData->numM1_alg; i++) {
						int ind;
						ind = j * caliData->numM1_alg + i;
						fprintf(fp, "%d\t", (int)caliData->out.bTable[ind]);
					}
					fprintf(fp, "\n");
				}
				fprintf(fp, "\npre bright\n");
				for (j = 0; j < caliData->numP2_alg; j++) {
					for (i = 0; i < caliData->numP1_alg; i++) {
						int ind;
						ind = j * caliData->numP1_alg + i;
						fprintf(fp, "%d\t", (int)caliData->out.preflashBrightness[ind]);
					}
					fprintf(fp, "\n");
				}
				fprintf(fp, "\npre ct\n");
				for (j = 0; j < caliData->numP2_alg; j++) {
					for (i = 0; i < caliData->numP1_alg; i++) {
						int ind;
						ind = j * caliData->numP1_alg + i;
						fprintf(fp, "%d\t", (int)caliData->out.preflashCt[ind]);
					}
					fprintf(fp, "\n");
				}
				fclose(fp);
			}

			if (debug1En == 1) {
#ifdef WIN32
				fp = fopen("d:\\temp\\fc_debug.bin", "wb");
#else
				fp = fopen("/data/misc/cameraserver/fc_debug.bin", "wb");
#endif
				fwrite(caliData, 1, sizeof(struct FCData), fp);
				fclose(fp);

			}

			if (caliData->out.error != 0) {
#ifdef WIN32
				fp = fopen("d:\\temp\\fc_error.txt", "wt");
#else
				fp = fopen("/data/misc/cameraserver/fc_error.txt", "wt");
#endif
				if (caliData->out.error == FlashCali_too_close) {
					fprintf(fp, "error: chart to camera is to close!\n");
				}

				fclose(fp);
			} else {
#ifdef WIN32
				fp = fopen("d:\\temp\\flashcalibration.bin", "wb");
#else
				fp = fopen("/data/misc/cameraserver/flashcalibration.bin", "wb");
#endif
				fwrite(&caliData->out, 1, sizeof(struct flash_calibration_data), fp);
				fclose(fp);
			}

			free(caliData);
			caliState = FlashCali_none;

		} else {
			//none
#ifdef WIN32
			printf("state=none\n");
#endif
			ISP_LOGD("flash calibration done!!\n");
		}

		if (caliState < FlashCali_end) {
			ISP_LOGD("qqfc exp=%d %d", (int)caliData->expTime, (int)caliData->gain);
			int lineTime = cxt->cur_status.line_time;
			cur_status->settings.manual_mode = 0;
			cur_status->settings.table_idx = 0;
			cur_status->settings.exp_line = caliData->expTime / lineTime;
			cur_status->settings.gain = caliData->gain;
		}
	}
	frameCount++;
}

static void _set_led2(struct ae_ctrl_cxt *cxt)
{
	struct ae_alg_calc_param *cur_status = &cxt->cur_status;
	int propValue[10];
	int ret;
	static int led_onOff = 0;
	static int led_isMain = 0;
	static int led_led1 = 0;
	static int led_led2 = 0;

	ret = get_prop_multi("persist.sys.isp.ae.fc_led", 4, propValue);
	if (ret > 0) {
		if (led_onOff == propValue[0] && led_isMain == propValue[1] && led_led1 == propValue[2] && led_led2 == propValue[3]) {
		} else {
			led_onOff = propValue[0];
			led_isMain = propValue[1];
			led_led1 = propValue[2];
			led_led2 = propValue[3];
			control_led(cxt, led_onOff, led_isMain, led_led1, led_led2);
			ISP_LOGD("qqfc led control %d %d %d %d", led_onOff, led_isMain, led_led1, led_led2);
		}
	}

	static int lock_lock = 0;
	static int exp_exp_line = 0;
	static int exp_exp_time = 0;
	static int exp_dummy = 0;
	static int exp_isp_gain = 0;
	static int exp_sensor_gain = 0;
	ret = get_prop_multi("persist.sys.isp.ae.fc_exp", 5, propValue);
	if (ret > 0) {
		if (exp_exp_line == propValue[0] && exp_exp_time == propValue[1] && exp_dummy == propValue[2] && exp_isp_gain == propValue[3] && exp_sensor_gain == propValue[4]) {
		} else {
			exp_exp_line = propValue[0];
			exp_exp_time = propValue[1];
			exp_dummy = propValue[2];
			exp_isp_gain = propValue[3];
			exp_sensor_gain = propValue[4];
			cur_status->settings.lock_ae = lock_lock;
			cur_status->settings.manual_mode = 0;
			cur_status->settings.table_idx = 0;
			cur_status->settings.exp_line = exp_exp_line;
			cur_status->settings.gain = exp_isp_gain * exp_sensor_gain / 4096;
			ISP_LOGD("qqfc set exp %d %d %d %d %d", exp_exp_line, exp_exp_time, exp_dummy, exp_isp_gain, exp_sensor_gain);
		}
	}

	ret = get_prop_multi("persist.sys.isp.ae.fc_lock", 1, propValue);
	if (ret > 0) {
		if (lock_lock == propValue[0]) {
		} else {
			lock_lock = propValue[0];
			if (lock_lock == 1) {
				//_set_pause(cxt);
				cxt->cur_status.settings.lock_ae = AE_STATE_LOCKED;
				ISP_LOGD("qqfc lock");
			}

		}
	}

	static int lock_unlock = 0;
	ret = get_prop_multi("persist.sys.isp.ae.fc_unlock", 1, propValue);
	if (ret > 0) {
		if (lock_unlock == propValue[0]) {
		} else {
			lock_unlock = propValue[0];
			if (lock_unlock == 1) {
				// _set_restore_cnt(cxt);
				cxt->cur_status.settings.lock_ae = AE_STATE_NORMAL;
				ISP_LOGD("qqfc unlock");
			}
		}
	}

	static int exp2 = 0;
	ret = get_prop_multi("persist.sys.isp.ae.fc_exp2", 1, propValue);
	if (ret > 0) {
		if (exp2 == propValue[0]) {
		} else {
			exp2 = propValue[0];
			if (exp2 == 1) {
				cur_status->settings.manual_mode = 0;
				cur_status->settings.table_idx = 0;
				cur_status->settings.exp_line = 800;
				cur_status->settings.gain = 256;
			}
		}
	}

	static int exp1 = 0;
	ret = get_prop_multi("persist.sys.isp.ae.fc_exp1", 1, propValue);
	if (ret > 0) {
		if (exp1 == propValue[0]) {
		} else {
			exp1 = propValue[0];
			if (exp1 == 1) {
				cur_status->settings.manual_mode = 0;
				cur_status->settings.table_idx = 0;
				cur_status->settings.exp_line = 100;
				cur_status->settings.gain = 256;
				ISP_LOGD("qqfc exp1 set");
			}
		}
	}

}

void flash_calibration_script(cmr_handle ae_cxt)
{
	char str[PROPERTY_VALUE_MAX];
	struct ae_ctrl_cxt *cxt = (struct ae_ctrl_cxt *)ae_cxt;
	memset((void *)&str[0], 0, sizeof(str));
	property_get("persist.sys.isp.ae.fc_stript", str, "");
	if (!strcmp(str, "on")) {
		flashCalibration(cxt);
		_set_led2(cxt);
	}
}
