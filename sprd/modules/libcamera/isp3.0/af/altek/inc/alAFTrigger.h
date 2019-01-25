#ifndef _ALAFTRIGGER_H_
#define _ALAFTRIGGER_H_

#ifdef alAFTrigger_EXPORT
#define alAFTrigger __declspec(dllexport)
#else
#define alAFTrigger
#endif

#include "alAFTrigger_Errcode.h"

typedef struct {

	int dcurrentVCM;

	void *tPDInfo;

	float fTriggerSensitivity;

} TriggerInReg;

enum HAFState { Waiting, Focusing };

#ifdef __cplusplus
extern "C" {
#endif

alAFTrigger alAFTrigger_ERR_CODE alAFTrigger_Initial();
alAFTrigger alAFTrigger_ERR_CODE alAFTrigger_SetHAFState(unsigned char a_ucHAFState);
alAFTrigger alAFTrigger_ERR_CODE alAFTrigger_GetHAFState(unsigned char *a_pucHAFState);
alAFTrigger alAFTrigger_ERR_CODE alAFTrigger_Calculate(float *a_pfOutProbTrigger, TriggerInReg *a_tInTriggerReg);
alAFTrigger alAFTrigger_ERR_CODE alAFTrigger_Close();
alAFTrigger alAFTrigger_ERR_CODE alAFTrigger_GetVersionInfo(void *a_pOutBuf, int a_dInBufMaxSize);

#ifdef __cplusplus
}
#endif

#endif



