/*******************************************************************************
** File Name:       SoonerFix.h
** Author:          Zhicheng Li	
** Date:            2015-02-12
** Copyright:       2014 Spreatrum, Incoporated. All Rights Reserved.
** Description:     This file define the  function, parameters used for  
**                  SoonerFix
********************************************************************************
********************************************************************************
**                              Edit History
** -----------------------------------------------------------------------------
** DATE                         NAME                      DESCRIPTION
** Year/Month/Data        ***                               Create.
*******************************************************************************/
#ifndef __SPREAD_ORBIT_H
#define __SPREAD_ORBIT_H

/*******************************************************************************
INCLUDED FILES
*******************************************************************************/
#ifdef WIN32
#include <Windows.h>
//#include "basictype.h"
#include "./core/xeod_cg_api.h"
extern ObsBlock g_tObsBlock[MAX_SV_PRN];
extern EphBlock g_tEphBlock[MAX_SV_PRN];
extern HANDLE g_hObsUpdateEvent;
extern CRITICAL_SECTION g_SFCriticalSection;
#else
#include "xeod_cg_api.h"
#define SPREAD_ORBIT_EXTEND_KEPLER 1
#endif
/*******************************************************************************
CONSTANTS AND MACROS DEFINE
*******************************************************************************/
#ifndef MAX_PATH_SIZE
#define MAX_PATH_SIZE   256
#endif

#define OBS_FILE_NAME   ("raw.obs")
#define EPH_FILE_NAME   ("ext.eph")

//20160106 add by hongxin.liu
#define KEPL_EPH_FILE_NAME   ("kep.eph")
//20160106

/*******************************************************************************
EXTERN DECLARATION of FUNCTIONS 
*******************************************************************************/
#ifdef WIN32
DWORD WINAPI SoonerFix_PredictThread(LPVOID lpParam);
#endif
//common
void SF_setExitFlag(int flag); 
int SF_isThreadExited();
void SF_setNewSvMask(int svid);
void SF_clearNewSvMask(int svid);
//common
void SF_setSaveEphMask(int svid);
void SF_clearSaveEphMask(int svid);
int SF_getSvPredictPosVel( int svid,  int week,  int tow, char *output);

#endif //__SPREAD_ORBIT_H
