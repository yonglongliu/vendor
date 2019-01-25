#ifndef ATM_H_
#define ATM_H_

#ifdef WIN32
#include "sci_types.h"
#else
#include <linux/types.h>
#include <sys/types.h>
#include <android/log.h>
#include <sys/system_properties.h>
#endif
#include "CurveUtility.h"
/*------------------------------------------------------------------------------*
 *				Compiler Flag					*
 *-------------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif

typedef enum {
    ATM_RET_OK,
    ATM_RET_NO_PIXEL,
    ATM_RET_HIST_INVALID,
    ATM_RET_NULLPTR,
} ATM_RET;

typedef enum {
    ATM_MODE_NORMAL = 0,
    ATM_MODE_GLOBAL_ONLY,
    ATM_MODE_LOCAL_ONLY
} ATM_MODE;

struct ATMCalcParam
{
    int magic;
    int version;
    int date;//hardcode for algo matching
    int time;//hardcode for algo matching
    int check;

    // control
    ATM_MODE eMode;  // algo strategy

    int i4BV;
    int i4LowPT;
    int i4LowPcentThd;
    int i4LowRightThd;
    int i4LowLeftThd;
    int i4HighPT;
    int i4HighPcentThd;
    int i4HighRightThd;
    int i4HighLeftThd;
    GEN_LUT strBVLut;
};

#define MAX_SYNC_SENSORS (4)

struct _atm_init_param
{
    uint32_t u4Magic;
    uint8_t  uOrigGamma[256];
};

struct atm_calc_param
{
    struct ATMCalcParam stAlgoParams;    // control parameters

    int date;				//ex: 20160331, hardcode for algo matching
    int time;				//ex: 221059, , hardcode for algo matching

    unsigned long long *pHist;
    uint32_t u4Bins;
    uint8_t *uBaseGamma;
    uint8_t *uModGamma;
    bool bHistB4Gamma;
};

struct atm_calc_result
{
    ATM_RET eStatus;  // should be 0

    uint8_t *uGamma;
    int32_t i4RespCurve[256];
    uint8_t uLowPT;
    uint8_t uHighPT;
    uint8_t uFinalLowBin;
    uint8_t uFinalHighBin;
//    uint8_t *log_buffer;
//    uint32_t log_size;
};

typedef enum
{
    ATM_IOCTRL_SET_MODE = 1,
    ATM_IOCTRL_SET_STATE = 2,
    ATM_IOCTRL_CMD_MAX,
} ATM_IOCTL;


/*------------------------------------------------------------------------------*
 *				Function Prototype					*
 *-------------------------------------------------------------------------------*/

void *atm_init(struct _atm_init_param *init_param);
int atm_calc(void* atm_handle, struct atm_calc_param* calc_param, struct atm_calc_result* calc_result);
int atm_ioctrl(void *atm_handle, int cmd, void *param);
int atm_deinit(void *atm_handle);


/*------------------------------------------------------------------------------*
 *				Compiler Flag					*
 *-------------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*------------------------------------------------------------------------------*/
#endif
// End
