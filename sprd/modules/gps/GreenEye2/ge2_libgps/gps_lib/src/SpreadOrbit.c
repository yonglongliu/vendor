/*******************************************************************************
** File Name:       SoonerFix.c
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
** 2015/08/05               Mao Li                      Modified based on Zhicheng and Xingwei
** 2016/06/24               Mao Li                      Merge the lte.c and spreadorbit.c
*******************************************************************************/
#ifdef WIN32
#include <io.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include "CommonDefine.h"
#include "supportpkg.h"
#include "satmanage.h"
#include "SpreadOrbit.h"
#include "./core/definition.h"
#include "./core/global_data.h"
#else
#include <stdio.h>
#include <cutils/log.h>
#include <semaphore.h>
#include <math.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/time.h>
#include "SpreadOrbit.h"
#include "xeod_cg_api.h"
#include "gps_common.h"
#include "gps_cmd.h"
#endif
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define  LOG_TAG  "LIBGPS_LTE"

#define LAGRANGE_POINTS_NUM   (7)
#define MJD_GPST0 44244
#define TRUE 1
#define FALSE 0

static XeodFileHeader   g_tObsHeader;
static EphKepler        g_tSvEph[MAX_SV_PRN][MAX_OBS_PER_DAY*OBS_DAYS];
static EphCartesian*    g_tCartesianResult[MAX_SV_PRN];
//20160105 add by hongxin.liu for the union of cartesian and kepler
static tSVD_Kepler_Ephemeris*    g_tKeplerResult[MAX_SV_PRN];
//20160105
static unsigned int     g_bValidResult[MAX_SV_PRN];

#ifdef WIN32
ObsBlock         g_tObsBlock[MAX_SV_PRN];
EphBlock         g_tEphBlock[MAX_SV_PRN];
HANDLE g_hObsUpdateEvent =  NULL;
CRITICAL_SECTION  g_SFCriticalSection;
#else
static ObsBlock         g_tObsBlock[MAX_SV_PRN];
static EphBlock         g_tEphBlock[MAX_SV_PRN];
static sem_t g_hObsUpdateEvent;
static pthread_mutex_t g_SFCriticalSection = PTHREAD_MUTEX_INITIALIZER;
#endif
static char bThreadExitFlag = FALSE;
static char bThreadExited = FALSE;
unsigned int  g_SF_newSvMask = 0;
unsigned int  g_SF_saveEphMask = 0;

static double mjd_eph[MAX_SV_PRN],mjd_obs[MAX_SV_PRN];

#ifdef WIN32
static void E(char *fmt, ...)
{
    #define BUF_SIZE 256
	va_list body;
	char outStr[BUF_SIZE];

    va_start(body, fmt); // prepare the arguments
	vsnprintf(outStr, BUF_SIZE, fmt, body);
	va_end(body);            // clean the stack

	printf("%s", outStr);
}
static void D(char *fmt, ...)
{
    #define BUF_SIZE 256
	va_list body;
	char outStr[BUF_SIZE];

    va_start(body, fmt); // prepare the arguments
	vsnprintf(outStr, BUF_SIZE, fmt, body);
	va_end(body);            // clean the stack

	printf("%s", outStr);
}
#endif

static int SF_allocResultBuffer(void)
{
  int    i;
  unsigned int dwUnitSize;

  dwUnitSize = (86400 / PRED_CART_STEPSIZE * PRED_DAYS + 1)* sizeof(EphCartesian);
  g_tCartesianResult[0] = (EphCartesian*)malloc(dwUnitSize * MAX_SV_PRN);
  if (NULL == g_tCartesianResult[0])
    return FALSE;

  memset(g_tCartesianResult[0], 0, dwUnitSize * MAX_SV_PRN);
  for(i=1; i< MAX_SV_PRN; i++)
    g_tCartesianResult[i] = (EphCartesian*)((unsigned long)g_tCartesianResult[i-1] + dwUnitSize);

  return TRUE;
}
//20160105 add by hongxin.liu for the union of cartesian and kepler
static int SF_allocKeplerResultBuffer(void)
{
  int    i;
  unsigned int dwUnitSize;

  dwUnitSize = (86400 / PRED_KEPL_STEPSIZE * PRED_DAYS + 1)* sizeof(tSVD_Kepler_Ephemeris);
  g_tKeplerResult[0] = (tSVD_Kepler_Ephemeris*)malloc(dwUnitSize * MAX_SV_PRN);
  if (NULL == g_tKeplerResult[0])
    return FALSE;

  memset(g_tKeplerResult[0], 0, dwUnitSize * MAX_SV_PRN);
  for(i=1; i< MAX_SV_PRN; i++)
    g_tKeplerResult[i] = (tSVD_Kepler_Ephemeris*)((unsigned long)g_tKeplerResult[i-1] + dwUnitSize);

  return TRUE;
}
static int SF_init()
{
    BOOL bAllocResult, bAllocKepler;
    memset(&g_tObsHeader, 0, sizeof(XeodFileHeader));
	memset(&g_tObsBlock, 0, sizeof(ObsBlock) * MAX_SV_PRN);
	memset(&g_tSvEph, 0, sizeof(g_tSvEph));
	memset(&g_tEphBlock, 0, sizeof(EphBlock) * MAX_SV_PRN);
	memset(g_bValidResult, 0, sizeof(int) * MAX_SV_PRN);
    bAllocResult = SF_allocResultBuffer();
    bAllocKepler = SF_allocKeplerResultBuffer();
	return (bAllocResult & bAllocKepler);
}

static void SF_freeResultBuffer(void)
{
  free(g_tCartesianResult[0]);
}

static int  SF_loadObsFromFile(char* pDataPath)
{
  int  i;
  FILE *fpObs;
  int ret = 0;
  char pObsPath[MAX_PATH_SIZE];
  unsigned int temp_svId = 0;
  unsigned int temp_obsNum = 0;

  sprintf(pObsPath, "%s/%s",pDataPath, OBS_FILE_NAME);
  fpObs=fopen(pObsPath, "rb");
  if(fpObs == NULL)
  {
    E("Can't open OBS file [%s]!\n\n",pObsPath);
    return FALSE;
  }

  fseek(fpObs, 0, SEEK_SET);
  fread(&g_tObsHeader,sizeof(XeodFileHeader),1,fpObs);

  for(i = 0; i < MAX_SV_PRN; i++)
  {
	  mjd_obs[i] = g_tObsHeader.mjd0 + g_tObsHeader.dmjd_end[i];
  }

  for(i=0; i<MAX_SV_PRN; i++)
  {
	  if(!g_tObsHeader.eph_valid[i])
	  {
		  g_tObsBlock[i].sv_id = i + 1;
		  g_tObsBlock[i].num_of_obs = 0;
		  g_tObsBlock[i].eph_kep = g_tSvEph[i];
		  continue;
	  }
	  E("seek new svid %d and save obs. \n",i + 1);
	  fseek(fpObs, i*(sizeof(ObsBlock)-sizeof(EphKepler*)+sizeof(EphKepler)*MAX_OBS_PER_DAY*OBS_DAYS)+sizeof(XeodFileHeader), SEEK_SET);  //zhouxw      // LTE64BIT
	  fread(&temp_svId,sizeof(unsigned int),1,fpObs);
	  //E("temp svid is %d\n",temp_svId);
	  fread(&temp_obsNum,sizeof(unsigned int),1,fpObs);
	  g_tObsBlock[i].sv_id = temp_svId;
	  g_tObsBlock[i].num_of_obs = temp_obsNum;
	  g_tObsBlock[i].eph_kep = g_tSvEph[i];
	  E("num of obs = %d\n",temp_obsNum);
	  if( temp_obsNum == 0 )
	  {
		  g_tObsHeader.eph_valid[i] = 0;
		  continue;
	  }

	  SF_setNewSvMask(temp_svId);    //should not update all mask
	  fread(&g_tObsBlock[i].eph_kep[0],sizeof(EphKepler)*temp_obsNum, 1,fpObs);
  }


  fclose(fpObs);

  return TRUE;
}

//======================== load eph from file ==============================//

static int  SF_loadEphFromFile(char* pDataPath)
{
  int  i;
  XeodFileHeader tEphHead;
  unsigned int EphSize1;
  FILE *fpEph = NULL;

  fpEph = fopen(pDataPath,"rb");
  if(fpEph==NULL)
  {
    E("Can't open EPH file [%s]!\n\n",pDataPath);
    return FALSE;
  }

  memset(&tEphHead, 0, sizeof(XeodFileHeader));

  fseek(fpEph, 0, SEEK_SET);
  fread(&tEphHead,sizeof(XeodFileHeader),1,fpEph);
  memcpy(g_bValidResult,&tEphHead.eph_valid[0], sizeof(unsigned int) * MAX_SV_PRN);
  for(i = 0; i < MAX_SV_PRN; i++)
  {
	  mjd_eph[i] = tEphHead.mjd0 + tEphHead.dmjd_start[i];
  }

#if SPREAD_ORBIT_EXTEND_KEPLER
  EphSize1= sizeof(tSVD_Kepler_Ephemeris) * (86400/PRED_KEPL_STEPSIZE*PRED_DAYS+1);
#else
  EphSize1= sizeof(EphCartesian) * ((86400/PRED_CART_STEPSIZE)*PRED_DAYS+1);
#endif

  for(i = 0; i < MAX_SV_PRN; i++)
  {
    fseek(fpEph, sizeof(XeodFileHeader) + i * (sizeof(EphBlock)- 2*sizeof(char *) +EphSize1), SEEK_SET);
#if SPREAD_ORBIT_EXTEND_KEPLER
    fread(&g_tEphBlock[i], sizeof(EphBlock) - 2*sizeof(char *), 1, fpEph);
    fread(g_tKeplerResult[i], EphSize1, 1, fpEph);//g_tKeplEphBlock[i].eph_kepl
    E("===>>>>>sv=%d,obsnum=%d,weekno=%d,toe=%d\n",g_tEphBlock[i].sv_id,g_tEphBlock[i].obs_num,
		g_tKeplerResult[i]->weekNo,g_tKeplerResult[i]->toe);
#else
    fread(&g_tEphBlock[i], sizeof(EphBlock)-2*sizeof(char *), 1, fpEph);
    fread(g_tCartesianResult[i], EphSize1, 1, fpEph);
	E("===>>>>>sv=%d,obsnum=%d,mjd0=%d,delta_mjd=%f\n",g_tEphBlock[i].sv_id,g_tEphBlock[i].obs_num,
		g_tCartesianResult[i]->mjd0,g_tCartesianResult[i]->delta_mjd);
#endif
  }
	//E("===>>>>>sv1,orbit para %f,%f,%f,%f,%f\n",g_tKeplerResult[0][5].Cus,g_tKeplerResult[0][5].Crc,
	//g_tKeplerResult[0][5].Crs,g_tKeplerResult[0][5].Cic,g_tKeplerResult[0][5].Cis);
  fclose(fpEph);
  return TRUE;
}
static int  SF_loadKepEphFromFile(char* pDataPath)
{
  int  i;
  XeodFileHeader tEphHead;
  unsigned int EphSize1;
  FILE *fpEph = NULL;

  fpEph = fopen(pDataPath,"rb");
  if(fpEph==NULL)
  {
    E("Can't open EPH file [%s]!\n\n",pDataPath);
    return FALSE;
  }

  memset(&tEphHead, 0, sizeof(XeodFileHeader));

  fseek(fpEph, 0, SEEK_SET);
  fread(&tEphHead,sizeof(XeodFileHeader),1,fpEph);
  memcpy(g_bValidResult,&tEphHead.eph_valid[0], sizeof(unsigned int) * MAX_SV_PRN);
  for(i = 0; i < MAX_SV_PRN; i++)
  {
	  /* prn=8 ofthen dmjd_start=0, for the g_bValidResult is 0 */
	  mjd_eph[i] = tEphHead.mjd0 + tEphHead.dmjd_start[i];
  }

  EphSize1= sizeof(EphCartesian) * ((86400/PRED_CART_STEPSIZE)*PRED_DAYS+1);

  for(i = 0; i < MAX_SV_PRN; i++)
  {
    fseek(fpEph, sizeof(XeodFileHeader) + i * (sizeof(EphBlock)-4+EphSize1), SEEK_SET);

    fread(&g_tEphBlock[i], sizeof(EphBlock)-4, 1, fpEph);
    fread(g_tCartesianResult[i], EphSize1, 1, fpEph);
	E("===>>>>>mjd0=%d,delta_mjd=%f\n",g_tCartesianResult[i]->mjd0,g_tCartesianResult[i]->delta_mjd);
  }

  fclose(fpEph);
  return TRUE;
}
static unsigned int SF_getFileCheckSum(FILE * pfile)
{
  unsigned int checksum = 0;
  long datasize = ftell(pfile) - sizeof(XeodFileHeader) - sizeof(XeodFileTail);

  fflush(pfile);
  fseek(pfile, 0, SEEK_END);

  if(datasize >= 0)
  {
	  int totalcount=datasize/4;
	  const int max=1024/4;
	  unsigned long pbuffer[1024];

	  int readcount;
	  int leftcount=totalcount;
	  int i;

	  fseek(pfile, sizeof(XeodFileHeader), SEEK_SET);

	  do
	  {
		  readcount=leftcount;
		  if(readcount>max)
			  readcount=max;
		  if(fread(pbuffer, readcount*sizeof(unsigned long), 1, pfile) == 1)
		  {
			  leftcount-=readcount;
			  for(i = 0; i < readcount; i ++)
				  checksum += pbuffer[i];
		  }
		  else
			  break;
	  }
	  while(leftcount>0);
  }

  return checksum;
}

static int SF_initEphFile(char* pDataPath)
{
  int  i;
  char pTmpPath[MAX_PATH_SIZE],strTail[32];
  XeodFileHeader tEphHead;
  XeodFileTail filetail;
  unsigned int EphSize1;
  FILE *fpEph;

#if SPREAD_ORBIT_EXTEND_KEPLER
  sprintf(pTmpPath, "%s/%s", pDataPath, KEPL_EPH_FILE_NAME);
  memcpy(strTail,"kep.eph\0\0",8);
#else
  sprintf(pTmpPath, "%s/%s", pDataPath, EPH_FILE_NAME);
  memcpy(strTail,"ext.eph\0\0",8);
#endif

  fpEph = fopen(pTmpPath,"wb");
  if(fpEph==NULL)
  {
    E("Can't open EPH file [%s]!\n\n",pTmpPath);
    return FALSE;
  }

  memset(&tEphHead, 0, sizeof(XeodFileHeader));

  fseek(fpEph, 0, SEEK_SET);
  fwrite(&tEphHead,sizeof(XeodFileHeader),1,fpEph);
#if SPREAD_ORBIT_EXTEND_KEPLER
  EphSize1= sizeof(tSVD_Kepler_Ephemeris) * (86400/PRED_KEPL_STEPSIZE*PRED_DAYS+1);
#else
  EphSize1= sizeof(EphCartesian) * ((86400/PRED_CART_STEPSIZE)*PRED_DAYS+1);
#endif

  for(i=0; i<MAX_SV_PRN; i++)
  {
    //. must write all before, otherwise parse error if no value
    fseek(fpEph, sizeof(XeodFileHeader) + i * (sizeof(EphBlock)- 2*sizeof(char *)+EphSize1), SEEK_SET);
    fwrite(&g_tEphBlock[i], sizeof(EphBlock)- 2*sizeof(char *), 1, fpEph);
#if SPREAD_ORBIT_EXTEND_KEPLER
    fwrite(g_tKeplerResult[i], EphSize1, 1, fpEph);
#else
    fwrite(g_tCartesianResult[i], EphSize1, 1, fpEph);
#endif
  }

  memset(&filetail, 0, sizeof(XeodFileTail));
  memcpy(filetail.label, strTail, 8);
  filetail.file_version = XEOD_FILE_VERSION;
  filetail.check_sum = 0;
  fwrite(&filetail,sizeof(XeodFileTail),1,fpEph);

  fclose(fpEph);

  return TRUE;
}
//20160106 add by hongxin.liu
static int  SaveKeplerEph(char* pDataPath)
{
  int  i;
  char pTmpPath[MAX_PATH_SIZE];
  XeodFileHeader tEphHead;
  XeodFileTail filetail;
  unsigned int EphSize1;
  FILE *fpEph;

  sprintf(pTmpPath, "%s/%s", pDataPath, KEPL_EPH_FILE_NAME);
  fpEph = fopen(pTmpPath,"wb");
  if(fpEph==NULL)
  {
    printf("Can't open EPH file [%s]!\n\n",pTmpPath);
    return FALSE;
  }

  memset(&tEphHead, 0, sizeof(XeodFileHeader));
  memcpy(&tEphHead.eph_valid[0], g_bValidResult, sizeof(unsigned int) * MAX_SV_PRN);

  for(i=0; i<MAX_SV_PRN; i++)
  {
    if(!g_tObsHeader.eph_valid[i] || !g_bValidResult[i])
      continue;

    tEphHead.mjd0 = g_tEphBlock[i].mjd0;//g_tCartesianResult[i][0].mjd0;
    tEphHead.dmjd_start[i] = g_tEphBlock[i].dmjd0;//g_tCartesianResult[i][0].delta_mjd;
    tEphHead.dmjd_end[i] = g_tEphBlock[i].dmjd0 + 6;//g_tCartesianResult[i][86400/PRED_CART_STEPSIZE*PRED_DAYS].delta_mjd +
  }

  tEphHead.leap_sec = g_tObsHeader.leap_sec;

  fseek(fpEph, 0, SEEK_SET);
  fwrite(&tEphHead,sizeof(XeodFileHeader),1,fpEph);

  EphSize1= sizeof(tSVD_Kepler_Ephemeris) * (86400/PRED_KEPL_STEPSIZE*PRED_DAYS+1);

  for(i=0; i<MAX_SV_PRN; i++)
  {
    if( (g_SF_saveEphMask & ((unsigned int)1<<i)) == 0 )
        continue;
    fseek(fpEph, sizeof(XeodFileHeader) + i * (sizeof(EphBlock)-4+EphSize1), SEEK_SET);

    fwrite(&g_tEphBlock[i], sizeof(EphBlock)-4, 1, fpEph);
    fwrite(g_tEphBlock[i].eph_kepl, EphSize1, 1, fpEph);
  }

  fclose(fpEph);

  return TRUE;
}

static int  SF_saveEph(char* pDataPath)
{
  int  i,k=0;
  char pTmpPath[MAX_PATH_SIZE];
  XeodFileHeader tEphHead;
  XeodFileTail filetail;
  unsigned int EphSize1;
  FILE *fpEph;

#if SPREAD_ORBIT_EXTEND_KEPLER
  sprintf(pTmpPath, "%s/%s", pDataPath, KEPL_EPH_FILE_NAME);
#else
  sprintf(pTmpPath, "%s/%s", pDataPath, EPH_FILE_NAME);
#endif
  fpEph = fopen(pTmpPath,"rb+");
  if(fpEph==NULL)
  {
    E("Can't open EPH file [%s]!\n\n",pTmpPath);
    return FALSE;
  }

  memset(&tEphHead, 0, sizeof(XeodFileHeader));
  memcpy(&tEphHead.eph_valid[0], g_bValidResult, sizeof(unsigned int) * MAX_SV_PRN);

  for(i=0; i<MAX_SV_PRN; i++)
  {
    if(!g_tObsHeader.eph_valid[i] || !g_bValidResult[i])
      continue;
#if SPREAD_ORBIT_EXTEND_KEPLER
    if(tEphHead.mjd0==0)
    {
        tEphHead.mjd0 = g_tEphBlock[i].mjd0;
        tEphHead.dmjd_start[i] = g_tEphBlock[i].dmjd0;
        tEphHead.dmjd_end[i] = g_tEphBlock[i].dmjd0 + 6;
    }
    else if(tEphHead.mjd0 > g_tEphBlock[i].mjd0)
    {
        tEphHead.dmjd_start[i] = g_tEphBlock[i].dmjd0;
        tEphHead.dmjd_end[i] = tEphHead.dmjd_start[i]+6;
        for(k=0; k<MAX_SV_PRN; k++)
        {
            if(k!=i)
            {
                tEphHead.dmjd_start[k] += (tEphHead.mjd0 - g_tEphBlock[i].mjd0);
                tEphHead.dmjd_end[k] =  tEphHead.dmjd_start[k]+6;
            }
        }
        tEphHead.mjd0 = g_tEphBlock[i].mjd0;
    }
    else
    {
        tEphHead.dmjd_start[i] = g_tEphBlock[i].dmjd0 + (g_tEphBlock[i].mjd0 - tEphHead.mjd0);
        tEphHead.dmjd_end[i] = tEphHead.dmjd_start[i] + 6;
    }
#else
    if(tEphHead.mjd0==0)
    {
        tEphHead.mjd0 = g_tCartesianResult[i][0].mjd0;
        tEphHead.dmjd_start[i] = g_tCartesianResult[i][0].delta_mjd;
        tEphHead.dmjd_end[i] = g_tCartesianResult[i][86400/PRED_CART_STEPSIZE*PRED_DAYS].delta_mjd +
                               g_tCartesianResult[i][86400/PRED_CART_STEPSIZE*PRED_DAYS].mjd0 - tEphHead.mjd0;
    }
    else if(tEphHead.mjd0 > g_tCartesianResult[i][0].mjd0)
    {
        tEphHead.dmjd_start[i] = g_tCartesianResult[i][0].delta_mjd;
        tEphHead.dmjd_end[i] = g_tCartesianResult[i][86400/PRED_CART_STEPSIZE*PRED_DAYS].delta_mjd +
                               g_tCartesianResult[i][86400/PRED_CART_STEPSIZE*PRED_DAYS].mjd0 - g_tCartesianResult[i][0].mjd0;
        for(k=0; k<MAX_SV_PRN; k++)
        {
            if(k!=i)
            {
                tEphHead.dmjd_start[k] += (tEphHead.mjd0 - g_tCartesianResult[i][0].mjd0);
                tEphHead.dmjd_end[k] += (tEphHead.mjd0 - g_tCartesianResult[i][0].mjd0);
            }
        }
        tEphHead.mjd0 = g_tCartesianResult[i][0].mjd0;
    }
    else
    {
        tEphHead.dmjd_start[i] = g_tCartesianResult[i][0].delta_mjd + (g_tCartesianResult[i][0].mjd0 - tEphHead.mjd0);
        tEphHead.dmjd_end[i] = g_tCartesianResult[i][86400/PRED_CART_STEPSIZE*PRED_DAYS].delta_mjd +
                               g_tCartesianResult[i][86400/PRED_CART_STEPSIZE*PRED_DAYS].mjd0 - tEphHead.mjd0;
    }

#endif
  }

  tEphHead.leap_sec = g_tObsHeader.leap_sec;

  fseek(fpEph, 0, SEEK_SET);
  fwrite(&tEphHead,sizeof(XeodFileHeader),1,fpEph);
#if SPREAD_ORBIT_EXTEND_KEPLER
  EphSize1= sizeof(tSVD_Kepler_Ephemeris) * (86400/PRED_KEPL_STEPSIZE*PRED_DAYS+1);
#else
  EphSize1= sizeof(EphCartesian) * ((86400/PRED_CART_STEPSIZE)*PRED_DAYS+1);
#endif

  for(i=0; i<MAX_SV_PRN; i++)
  {
      //. have been write before
    if( (g_SF_saveEphMask & ((unsigned int)1<<i)) == 0 )
        continue;
    fseek(fpEph, sizeof(XeodFileHeader) + i * (sizeof(EphBlock)- 2*sizeof(char *) +EphSize1), SEEK_SET);

    fwrite(&g_tEphBlock[i], sizeof(EphBlock)- 2*sizeof(char *), 1, fpEph);
    E("SF_saveEph: svid=%d\n",g_tEphBlock[i].sv_id);
#if SPREAD_ORBIT_EXTEND_KEPLER
    fwrite(g_tEphBlock[i].eph_kepl, EphSize1, 1, fpEph);
#else
    fwrite(g_tCartesianResult[i], EphSize1, 1, fpEph);
#endif
    SF_clearSaveEphMask(i+1);
  }

  fclose(fpEph);

  return TRUE;
}
static int  SF_saveObs(char* pDataPath)
{
  int  i;
  char pTmpPath[MAX_PATH_SIZE];
  XeodFileHeader tEphHead;
  XeodFileTail filetail;
  unsigned int EphSize1;
  FILE *fpObs;


  sprintf(pTmpPath, "%s/%s", pDataPath, OBS_FILE_NAME);
  E("SF_saveObs enter,path is %s",pTmpPath);
  if(access(pTmpPath,0) == -1)
  {
    fpObs = fopen(pTmpPath,"wb+");
	if(fpObs != NULL) fclose(fpObs);
  }
  fpObs = fopen(pTmpPath,"wb");
  if(fpObs==NULL)
  {
    E("Can't open EPH file [%s]!\n\n",pTmpPath);
    return FALSE;
  }

  fseek(fpObs, 0, SEEK_SET);
  fwrite(&g_tObsHeader,sizeof(XeodFileHeader),1,fpObs);

  for(i = 0; i < MAX_SV_PRN; i ++)
  {
	  if(!g_tObsHeader.eph_valid[i])
	  {
		  fseek(fpObs, i*(sizeof(ObsBlock)-sizeof(EphKepler*)+sizeof(EphKepler)*MAX_OBS_PER_DAY*OBS_DAYS)+sizeof(XeodFileHeader), SEEK_SET);  //zhouxw    // LTE64BIT
		  fwrite(&g_tObsBlock[i],sizeof(ObsBlock)-sizeof(EphKepler*),1,fpObs);    // LTE64BIT
		  fwrite(g_tObsBlock[i].eph_kep,sizeof(EphKepler)*MAX_OBS_PER_DAY*OBS_DAYS,1,fpObs);
		  continue;
	  }
      fseek(fpObs, i*(sizeof(ObsBlock)-sizeof(EphKepler*)+sizeof(EphKepler)*MAX_OBS_PER_DAY*OBS_DAYS)+sizeof(XeodFileHeader), SEEK_SET);  //zhouxw      // LTE64BIT
	  fwrite(&g_tObsBlock[i],sizeof(ObsBlock)-sizeof(EphKepler*),1,fpObs);   // LTE64BIT
	  fwrite(g_tObsBlock[i].eph_kep,sizeof(EphKepler)*g_tObsBlock[i].num_of_obs,1,fpObs);
  }

  memset(&filetail, 0, sizeof(XeodFileTail));
  memcpy(filetail.label, "raw.obs\0\0", 8);
  filetail.file_version = XEOD_FILE_VERSION;
  filetail.check_sum = 0;
  fwrite(&filetail,sizeof(XeodFileTail),1,fpObs);
  //fflush(fpObs);
  E("save file success");
  /* update checksum */
  //filetail.check_sum = SF_getFileCheckSum(fpObs);
  //fwrite(&filetail,sizeof(XeodFileTail),1,fpObs);

  fclose(fpObs);

  return TRUE;
}


void SF_setExitFlag(int flag)
{
	bThreadExitFlag = flag;
#ifdef WIN32
	SetEvent(g_hObsUpdateEvent);
#else
	sem_post(&g_hObsUpdateEvent);
#endif
}

int SF_isThreadExited()
{
	return bThreadExited;
}

static int GetJulianDate(int WN, double sow, double *delta_mjd)
{
  int dow = ((int)sow) / 86400;
  int mjd= MJD_GPST0 + WN*7 + dow;

  if(delta_mjd!=NULL)
    *delta_mjd=(sow - dow * 86400) / 86400;

  return mjd;
}

static double ecc_anomaly(double Mk, double e)
{
  short i;
  double Ek, new_Ek, Ek_diff;

  Ek = Mk;
  i = 0;
  do
  {
    new_Ek = Mk + (e * sin(Ek));
    Ek_diff = fabs(new_Ek - Ek);
    Ek = new_Ek;
  }
  while ((Ek_diff > 1.0e-8) && (++i < 10));

  return Ek;
}

static double diffTime(double tbig,int wbig, double tsmall,int wsmall)
{
  return tbig-tsmall+(wbig-wsmall)*604800.0;
}

void SatPosSpeedECEF(EphKepler *ep, double time, int weekno,ECEFPOSITION *pos, ECEFPOSITION *speed, double *dTr)
{
  double  tk, a, b, sinEk, arg_y, arg_x;
  double  Mk, Ek, vk, phik, sin_phik2, cos_phik2;
  double  del_uk, del_rk, del_ik, uk, rk, ik;
  double  xkp, ykp, ok;
  double  sinok, cosok, cosik;
  double	n, r1me2, OMEGA_n, ODOT_n, Axis;
  double	Ek1, vk1, rk1, sinok1, cosok1, xkp1, ykp1;
  double	sinuk, cosuk;

  if (!ep->available==TRUE)
  {
    pos->available = FALSE;
    return;
  }
  Axis = ep->sqrtA*ep->sqrtA;
  n = WGS_SQRT_GM / (ep->sqrtA*Axis)+ep->deltaN;
  r1me2 = sqrt(1.0-ep->ecc*ep->ecc);
  OMEGA_n = ep->OMEGA0 - WGS84_OMEGDOTE*ep->toe;
  ODOT_n = ep->OMEGADOT - WGS84_OMEGDOTE;
  tk = diffTime(time,weekno, ep->toe,ep->week_no);

  Mk = ep->M0 + (n * tk);

  Ek = ecc_anomaly(Mk, ep->ecc);
  a = cos(Ek);
  b = 1.0 - (ep->ecc * a);
  sinEk = sin(Ek);
  arg_y = r1me2 * sinEk;
  arg_x = a - ep->ecc;
  vk = atan2(arg_y, arg_x);
  phik = vk + ep->w;
  sin_phik2 = sin(phik + phik);
  cos_phik2 = cos(phik + phik);
  del_uk = (ep->Cus * sin_phik2) + (ep->Cuc * cos_phik2);
  del_rk = (ep->Crc * cos_phik2) + (ep->Crs * sin_phik2);
  del_ik = (ep->Cic * cos_phik2) + (ep->Cis * sin_phik2);
  uk = phik + del_uk;
  rk = (Axis * b) + del_rk;
  ik = ep->i0 + del_ik + (ep->IDOT * tk);
  sinuk = sin(uk);
  cosuk = cos(uk);
  xkp = rk * cosuk;
  ykp = rk * sinuk;
  ok = OMEGA_n + (ODOT_n * tk);
  sinok = sin(ok);
  cosok = cos(ok);
  cosik = cos(ik);

  pos->x = (xkp * cosok) - (ykp * cosik * sinok);
  pos->y = (xkp * sinok) + (ykp * cosik * cosok);
  pos->z = ykp * sin(ik);
  pos->available = TRUE;

  if (speed != NULL)
  {
    Ek1 = n / b;
    vk1 = r1me2 * Ek1 / b;
    rk1 = Axis * ep->ecc * sinEk * Ek1;
    xkp1 = rk1 * cosuk - rk * sinuk * vk1;
    ykp1 = rk1 * sinuk + rk * cosuk * vk1;
    sinok1 = cosok * ODOT_n;
    cosok1 = -sinok * ODOT_n;
    speed->x = xkp * cosok1 + xkp1 * cosok - cosik * (ykp * sinok1 + ykp1 * sinok);
    speed->y = xkp * sinok1 + xkp1 * sinok + cosik * (ykp * cosok1 + ykp1 * cosok);
    speed->z = ykp1 * sin(ik);
    speed->available = TRUE;
  }

  if(dTr != NULL)
  {
    *dTr = WGS_F_GTR * ep->ecc * ep->sqrtA * sin(Ek);
  }
}

int SF_getEphPoints(int svid, short week, double tow, EphCartesian pPredictEph[], tSVD_Kepler_Ephemeris *pPredKep)
{
	int i;
	int Mjd = 0;
	double dMjd = 0;
	int svIndex = svid - 1;
	int ephIndex = 0;
	int ret = 0;
	double keplerEpgage = 0;
	int kepIndex = 0;

	if(svid < 1 || svid > 32 || week == 0)
		return ret;

	if(g_bValidResult[svIndex] == FALSE)
		return ret;

	/* Get Julian Date */
	Mjd = GetJulianDate(week, tow, &dMjd);
    D("svid=%d,Mjd=%d,week=%d,tow=%f,dMjd=%f",svid,Mjd,week,tow,dMjd);
    if(g_tObsBlock[svid-1].num_of_obs < 1 || g_tObsBlock[svid-1].eph_kep == NULL)
    {
        E("svid %d have no save obs or eph_kep is null,must return",svid);
        return ret;
    }
    keplerEpgage = (week - g_tObsBlock[svid-1].eph_kep[g_tObsBlock[svid-1].num_of_obs-1].week_no)*604800+ (tow - g_tObsBlock[svid-1].eph_kep[g_tObsBlock[svid-1].num_of_obs-1].toe);
	/* the latest points, get it from kep eph directly */
	if((keplerEpgage < 7200.0) && (keplerEpgage > -7200.0))
	{
		return 2;
	}

	if((g_SF_newSvMask & (1<<svIndex)) != 0)
	{
		E("===>>>lte is working, no respond on svid %d", svIndex+1);
		return ret;
	}
#if SPREAD_ORBIT_EXTEND_KEPLER
    //what is the first kepler?
    kepIndex = ((week - g_tKeplerResult[svIndex][0].weekNo)*604800 + (tow - g_tKeplerResult[svIndex][0].toe) + PRED_KEPL_STEPSIZE/2) / PRED_KEPL_STEPSIZE;
    if(kepIndex > (DAY2SEC/PRED_KEPL_STEPSIZE)*PRED_DAYS )
    {
        return ret;
    }
    else if(kepIndex >= 0)
    {
        //SF_ExtKep2EphKep(&g_tKeplerResult[svIndex][kepIndex],pPredKep);
        memcpy(pPredKep,&g_tKeplerResult[svIndex][kepIndex], sizeof(tSVD_Kepler_Ephemeris));
        //E("ext kepler %d:WN-%d,toe-%d",kepIndex,pPredKep->weekNo,pPredKep->toe);
        //E("orbit para getpoints %f %f %f %f %f\r\n",pPredKep->Cus,pPredKep->Crc,pPredKep->Crs,pPredKep->Cic,pPredKep->Cis);
        if(pPredKep->status == 0)
		{
			ret = 3;
		}
		else
		{
			ret = 0;
		}

    }
#else
	/* fetch the point close the input time */
	ephIndex = ((Mjd + dMjd - g_tCartesianResult[svIndex][0].mjd0 - g_tCartesianResult[svIndex][0].delta_mjd) * DAY2SEC + PRED_CART_STEPSIZE/2 ) / PRED_CART_STEPSIZE;
    D("ephIndex=%d,mjd0=%d,delta_mjd=%f",ephIndex,g_tCartesianResult[svIndex][0].mjd0,g_tCartesianResult[svIndex][0].delta_mjd);
	ephIndex -= LAGRANGE_POINTS_NUM/2;
	if( ephIndex > (PRED_DAYS * DAY2SEC/PRED_CART_STEPSIZE) )
	{
		return ret;
	}
	else if(ephIndex >= 0)
	{
        ret = 1;
		for(i = 0; i < LAGRANGE_POINTS_NUM; i++)
		{
			memcpy(&pPredictEph[i], &g_tCartesianResult[svIndex][ephIndex + i], sizeof(EphCartesian));
            E("point %d:mjd0-%d,delta_mjd-%f,quality-%d",i,pPredictEph[i].mjd0,pPredictEph[i].delta_mjd,pPredictEph[i].quality);
            if(pPredictEph[i].quality == 0)
            {
                ret = 0;
                break;
            }
		}
	}
#endif

  return ret;
}

double SF_lagrangeInterpolation(double MjdTime, double TimeSerial[], double PosSerial[])
{
	int i, k;
	double curPos = 0;
	double coef = 1.0;

	for(i = 1; i <= LAGRANGE_POINTS_NUM; i ++)
	{
		coef = 1;

		for(k = 1; k <= LAGRANGE_POINTS_NUM; k ++)
		{
			if(k != i)
				coef = coef * (MjdTime - TimeSerial[k-1])/(TimeSerial[i-1] - TimeSerial[k-1]);
		}

		curPos += coef * PosSerial[i-1];
	}

	return curPos;
}
//struct transformation for code size
void EphKepler2CPGPSEph(EphKepler *ApEphKep, PGPS_EPHEMERIS CpGPSEph)
{
    CpGPSEph->svid = ApEphKep->svid;
    CpGPSEph->week = ApEphKep->week_no;
    CpGPSEph->ura = ApEphKep->acc;
    CpGPSEph->iode2 = ApEphKep->iode2;
    CpGPSEph->iode3 = ApEphKep->iode3;
    CpGPSEph->M0 = ApEphKep->M0;
    CpGPSEph->delta_n = ApEphKep->deltaN;
    CpGPSEph->ecc = ApEphKep->ecc;
    CpGPSEph->Ek = ApEphKep->Ek;
    CpGPSEph->sqrtA = ApEphKep->sqrtA;
    CpGPSEph->omega0 = ApEphKep->OMEGA0;
    CpGPSEph->i0 = ApEphKep->i0;
    CpGPSEph->w = ApEphKep->w;
    CpGPSEph->omega_dot = ApEphKep->OMEGADOT;
    CpGPSEph->idot = ApEphKep->IDOT;
    CpGPSEph->cuc = ApEphKep->Cuc;
    CpGPSEph->cus = ApEphKep->Cus;
    CpGPSEph->crc = ApEphKep->Crc;
    CpGPSEph->crs = ApEphKep->Crs;
    CpGPSEph->cic = ApEphKep->Cic;
    CpGPSEph->cis = ApEphKep->Cis;
    CpGPSEph->tgd = ApEphKep->group_delay;
    CpGPSEph->af0 = ApEphKep->af0;
    CpGPSEph->af1 = ApEphKep->af1;
    CpGPSEph->af2 = ApEphKep->af2;
    //CpGPSEph->aodc = ApEphKep->aodc;
    CpGPSEph->health = ApEphKep->health;
    CpGPSEph->toc = ApEphKep->toc;
    CpGPSEph->toe = ApEphKep->toe;
    //CpGPSEph->available = ApEphKep->available;
    //CpGPSEph->repeat = ApEphKep->repeat;
    //CpGPSEph->update = ApEphKep->update;
    //CpGPSEph->fit_interval = ApEphKep->fit_interval;

	CpGPSEph->axis = CpGPSEph->sqrtA * CpGPSEph->sqrtA;
	CpGPSEph->n = WGS_SQRT_GM / (CpGPSEph->axis * CpGPSEph->sqrtA) + CpGPSEph->delta_n;
	CpGPSEph->root_ecc = sqrt(1.0 - CpGPSEph->ecc*CpGPSEph->ecc);
	CpGPSEph->omega_t = CpGPSEph->omega0 - WGS84_OMEGDOTE * CpGPSEph->toe;
	CpGPSEph->omega_delta = CpGPSEph->omega_dot - WGS84_OMEGDOTE;
	CpGPSEph->Ek = 0;
}
void tSVD_Kepler_Ephemeris2CPGPSEph(tSVD_Kepler_Ephemeris *AptSVDKepEph, PGPS_EPHEMERIS CpGPSEph)
{
    CpGPSEph->af0 = AptSVDKepEph->af0 ;
    CpGPSEph->af1 = AptSVDKepEph->af1 ;
    CpGPSEph->af2 = AptSVDKepEph->af2 ;
    CpGPSEph->axis = AptSVDKepEph->sqrta * AptSVDKepEph->sqrta;
    CpGPSEph->cic = AptSVDKepEph->Cic ;
    CpGPSEph->cis = AptSVDKepEph->Cis ;
    CpGPSEph->crc = AptSVDKepEph->Crc ;
    CpGPSEph->crs = AptSVDKepEph->Crs ;
    CpGPSEph->cuc = AptSVDKepEph->Cuc ;
    CpGPSEph->cus = AptSVDKepEph->Cus ;
    CpGPSEph->delta_n = AptSVDKepEph->deltan ;
    CpGPSEph->ecc = AptSVDKepEph->e ;
    CpGPSEph->Ek = 0;
    CpGPSEph->flag = 0;
    CpGPSEph->health = AptSVDKepEph->status;
    CpGPSEph->i0 = AptSVDKepEph->i0 ;
    CpGPSEph->idot = AptSVDKepEph->idot ;
    CpGPSEph->iodc = AptSVDKepEph->iodc ;
    CpGPSEph->iode2 = 0;
    CpGPSEph->iode3 = 0;
    CpGPSEph->M0 = AptSVDKepEph->m0 ;
    CpGPSEph->n = WGS_SQRT_GM / (CpGPSEph->axis * AptSVDKepEph->sqrta) + AptSVDKepEph->deltan;
    CpGPSEph->omega0 = AptSVDKepEph->omega0 ;
    CpGPSEph->omega_delta = AptSVDKepEph->omegaDot - WGS84_OMEGDOTE;
    CpGPSEph->omega_dot = AptSVDKepEph->omegaDot ;
    CpGPSEph->omega_t = CpGPSEph->omega0 - WGS84_OMEGDOTE * AptSVDKepEph->toe;
    CpGPSEph->root_ecc = sqrt(1.0 - CpGPSEph->ecc*CpGPSEph->ecc);
    CpGPSEph->sqrtA = AptSVDKepEph->sqrta ;
    CpGPSEph->svid = AptSVDKepEph->svid ;
    CpGPSEph->tgd = AptSVDKepEph->tgd ;
    CpGPSEph->tgd2 = 0;
    CpGPSEph->toc = AptSVDKepEph->toc ;
    CpGPSEph->toe = AptSVDKepEph->toe ;
    CpGPSEph->ura = 0;
    CpGPSEph->w = AptSVDKepEph->omega ;
    CpGPSEph->week = AptSVDKepEph->weekNo;
}
void CPGPSEph2EphKepler(PGPS_EPHEMERIS CpGPSEph, EphKepler *ApEphKep)
{
    ApEphKep->svid = CpGPSEph->svid;
    ApEphKep->week_no = CpGPSEph->week;
    ApEphKep->acc = CpGPSEph->ura;
    ApEphKep->iode2=CpGPSEph->iode2;
    ApEphKep->iode3 = CpGPSEph->iode3;
    ApEphKep->M0 = CpGPSEph->M0;
    ApEphKep->deltaN = CpGPSEph->delta_n;
    ApEphKep->ecc = CpGPSEph->ecc;
    ApEphKep->Ek = CpGPSEph->Ek;
    ApEphKep->sqrtA = CpGPSEph->sqrtA;
    ApEphKep->OMEGA0 = CpGPSEph->omega0;
    ApEphKep->i0 = CpGPSEph->i0;
    ApEphKep->w = CpGPSEph->w;
    ApEphKep->OMEGADOT = CpGPSEph->omega_dot;
    ApEphKep->IDOT = CpGPSEph->idot;
    ApEphKep->Cuc = CpGPSEph->cuc;
    ApEphKep->Cus = CpGPSEph->cus;
    ApEphKep->Crc = CpGPSEph->crc;
    ApEphKep->Crs = CpGPSEph->crs;
    ApEphKep->Cic = CpGPSEph->cic;
    ApEphKep->Cis = CpGPSEph->cis;
    ApEphKep->group_delay = CpGPSEph->tgd;
    ApEphKep->af0 = CpGPSEph->af0;
    ApEphKep->af1 = CpGPSEph->af1;
    ApEphKep->af2 = CpGPSEph->af2;
    //CpGPSEph->aodc = ApEphKep->aodc;
    ApEphKep->health = CpGPSEph->health;
    ApEphKep->toc = CpGPSEph->toc;
    ApEphKep->toe = CpGPSEph->toe;
    ApEphKep->available = TRUE;
    //CpGPSEph->repeat = ApEphKep->repeat;
    ApEphKep->update = TRUE;
    //CpGPSEph->fit_interval = ApEphKep->fit_interval;

}

//PSATELLITE_INFO pSatInfo mod to char *buf;
int SF_getSvPredictPosVel( int svid,  int week,  int tow, char *output)
{
	int mjd;
	double dmjd;
	int i;
	int ret = 0;
	double state[8][LAGRANGE_POINTS_NUM]; /* time, x, y, z, vx,vy,vz, clk */
	EphCartesian prdEph[LAGRANGE_POINTS_NUM];
	tSVD_Kepler_Ephemeris prdKep;
    GPS_EPHEMERIS GpsEph;
	ECEFPOSITION pos, vel;

    memset(&GpsEph,0,sizeof(GPS_EPHEMERIS));

	/* get the most close prd points */
	ret = SF_getEphPoints(svid, week, (double)tow, prdEph, &prdKep);
    E("SF_getEphPoints ret is %d",ret);
	/* get cur pos/vel based on lagrange Interpolation */
	if(ret == 1)
	{
		memcpy(output,&g_tEphBlock[svid - 1],sizeof(EphBlock) - sizeof(EphCartesian*));     // LTE64BIT
		memcpy(output+sizeof(EphBlock)-sizeof(EphCartesian*),prdEph,sizeof(prdEph));  // LTE64BIT
		ret = sizeof(EphBlock) + sizeof(prdEph) - sizeof(EphCartesian*);   // LTE64BIT

	}
	/* the latest points, get it from kep eph directly */
	else if (ret == 2)
	{
        EphKepler2CPGPSEph(&g_tObsBlock[svid-1].eph_kep[g_tObsBlock[svid-1].num_of_obs-1],&GpsEph);
		memcpy(output,&GpsEph,sizeof(GPS_EPHEMERIS));
		ret = sizeof(GPS_EPHEMERIS);
	}
    else if (ret == 3)
    {
        memcpy(output, &g_tEphBlock[svid - 1], sizeof(EphBlock) - 2 *sizeof(char *));
        E("SF_getSvPredictPosVel: svid=%d\n",g_tEphBlock[svid - 1].sv_id);
        //E("orbit para output %f %f %f %f %f\n",prdKep.Cus,prdKep.Crc,prdKep.Crs,prdKep.Cic,prdKep.Cis);
        tSVD_Kepler_Ephemeris2CPGPSEph(&prdKep,&GpsEph);
        memcpy(output+sizeof(EphBlock) - 2 *sizeof(char *), &GpsEph, sizeof(GpsEph));
        ret = sizeof(EphBlock) + sizeof(GpsEph) - 2 *sizeof(char *);
    }
	return ret;
}


void SF_setNewSvMask(int svid)
{
	g_SF_newSvMask |= (unsigned int)1 << (svid -1);
}

void SF_clearNewSvMask(int svid)
{
	g_SF_newSvMask &= ~((unsigned int)1 << (svid -1));
}

void SF_setSaveEphMask(int svid)
{
      g_SF_saveEphMask  |= (unsigned int)1 << (svid -1);
}

void SF_clearSaveEphMask(int svid)
{
	  g_SF_saveEphMask &= ~((unsigned int)1 << (svid -1));
}

void SF_updateObsFileContents(GPS_EPHEMERIS *eph_CP)
{
	unsigned int i = 0, j = 0, obsNum = 0, shiftNum = 0;
	int mjd0, mjd,mjd_new;
	double dmjd,dmjd_new;
	double deltaTime0 = 0, deltaTime1 = 0, deltaTime, min0 = 0, max0 = 0,deltaMjd;
    EphKepler ephKep_AP;
    EphKepler *eph = &ephKep_AP;

    memset(eph, 0, sizeof(EphKepler));
    CPGPSEph2EphKepler(eph_CP,eph);

	obsNum = g_tObsBlock[eph->svid-1].num_of_obs;

    if(eph == NULL)
    {
        E("updateobsfilecontents eph is null");
        return;
    }
	D("try updateobs 00,obsnum=%d,weekno=%d,svid=%d,toe=%d,health=%d",obsNum,eph->week_no,eph->svid,eph->toe,eph->health);
	/* only save interval eph */
	if((eph->week_no <= 1024) || (eph->svid > 32) || ((eph->toe % 7200) != 0 ) || (eph->health != 0))
		return;

	if((obsNum > 0) && (obsNum <= MAX_OBS_NUM))
	{
		if(eph->week_no == g_tObsBlock[eph->svid-1].eph_kep[obsNum-1].week_no)
		{
			if(eph->toe <= g_tObsBlock[eph->svid-1].eph_kep[obsNum-1].toe)
				return;
		}
		/* less than the exist obs is forbidden. */
		if(eph->week_no < g_tObsBlock[eph->svid-1].eph_kep[obsNum-1].week_no)
			return;
		D("succeed updateobs 00,obsnum=%d,weekno=%d,svid=%d,toe=%d,health=%d",obsNum,eph->week_no,eph->svid,eph->toe,eph->health);

		/* if num of obs is overflow, then move the oldest out, hardly exist */
		if( obsNum == MAX_OBS_NUM )
		{
		    E("obsnum is over max obs:%d",obsNum);
			shiftNum = g_tObsBlock[eph->svid-1].num_of_obs - MAX_OBS_NUM + 1;  //int to unsigned

			for(i = shiftNum; i < g_tObsBlock[eph->svid-1].num_of_obs; i++)
			{
				memmove(&g_tObsBlock[eph->svid-1].eph_kep[i-shiftNum],&g_tObsBlock[eph->svid-1].eph_kep[i], sizeof(EphKepler));
			}
			g_tObsBlock[eph->svid-1].num_of_obs = MAX_OBS_NUM - 1;
		}

		deltaTime0 = (eph->week_no - g_tObsBlock[eph->svid-1].eph_kep[0].week_no) * WEEK2SEC + eph->toe - g_tObsBlock[eph->svid-1].eph_kep[0].toe;
		deltaTime1 = (eph->week_no - g_tObsBlock[eph->svid-1].eph_kep[obsNum-1].week_no) * WEEK2SEC + eph->toe - g_tObsBlock[eph->svid-1].eph_kep[obsNum-1].toe;

		/* time interval too large to the last obs or no obs available, invalidate all */
		if( fabs(deltaTime1) >= (OBS_DAYS * DAY2SEC) )
		{
			g_tObsBlock[eph->svid-1].num_of_obs = 0;
			memset(&g_tObsBlock[eph->svid-1].eph_kep[obsNum-1],0, sizeof(EphKepler));
		}
		else if( fabs(deltaTime0) >=  (OBS_DAYS * DAY2SEC) )
		{
			for(i=0; i<g_tObsBlock[eph->svid-1].num_of_obs; i++)
			{
				deltaTime =  (eph->week_no - g_tObsBlock[eph->svid-1].eph_kep[i].week_no) * WEEK2SEC + eph->toe - g_tObsBlock[eph->svid-1].eph_kep[i].toe;
				if(deltaTime < (OBS_DAYS * DAY2SEC) )
					break;
			}
			for(j=i; j<g_tObsBlock[eph->svid-1].num_of_obs; j++)
				memmove(&g_tObsBlock[eph->svid-1].eph_kep[j-i],&g_tObsBlock[eph->svid-1].eph_kep[j], sizeof(EphKepler));
			g_tObsBlock[eph->svid-1].num_of_obs = g_tObsBlock[eph->svid-1].num_of_obs - i;
		}
	}
	/* add the new obs into the obsBlock */
	g_tObsBlock[eph->svid-1].num_of_obs ++;
	obsNum = g_tObsBlock[eph->svid-1].num_of_obs;
	if(obsNum > MAX_OBS_NUM)
	{
	    E("obsNum is %d,too large,so maybe make system crash",obsNum);
	    return;
	}
	memcpy(&g_tObsBlock[eph->svid-1].eph_kep[obsNum - 1],eph, sizeof(EphKepler));
	g_tObsHeader.eph_valid[eph->svid-1] = 1;
	g_tObsBlock[eph->svid-1].sv_id = eph->svid;

	/* update obs header and tail */
	/* should cal the mjd of first obs TODO !!! */
	/* the mjd0 is minimize of the whole */
	mjd0 = g_tObsHeader.mjd0;
	mjd_new = GetJulianDate((int)eph->week_no,(double)eph->toe, &dmjd_new);

	if(mjd0 == 0 || mjd_new < mjd0 )
	{
		g_tObsHeader.mjd0 = mjd_new;
		/* update dmjd_start and dmjd_end for each satellite */
		for(i = 0; i < MAX_SV_PRN; i ++)
		{
			if(g_tObsHeader.eph_valid[i] == 0)
				continue;

			for(j = 0; j < g_tObsBlock[i].num_of_obs; j ++)
			{

				mjd = GetJulianDate(g_tObsBlock[i].eph_kep[j].week_no,(double)g_tObsBlock[i].eph_kep[j].toe, &dmjd);

				deltaMjd = mjd - g_tObsHeader.mjd0 + dmjd;
				if(j == 0)
				{
					min0 = max0 = deltaMjd;
				}
				else
				{
					if(min0 > deltaMjd)
						min0 = deltaMjd;
					if(max0 < deltaMjd)
						max0 = deltaMjd;
				}
			}
			g_tObsHeader.dmjd_start[i] = min0;
			g_tObsHeader.dmjd_end[i] = max0;
		}
	}
	else
	{
		/* only update the new eph Header */
		for(j=0; j<g_tObsBlock[eph->svid-1].num_of_obs; j++)
		{
			mjd = GetJulianDate(g_tObsBlock[eph->svid-1].eph_kep[j].week_no,(double)g_tObsBlock[eph->svid-1].eph_kep[j].toe, &dmjd);
			deltaMjd = mjd - g_tObsHeader.mjd0 + dmjd;
			if(j == 0)
			{
				min0 = max0 = deltaMjd;
			}
			else
			{
				if(min0 > deltaMjd)
					min0 = deltaMjd;
				if(max0 < deltaMjd)
					max0 = deltaMjd;
			}
		}
		g_tObsHeader.dmjd_start[eph->svid-1] = min0;
		g_tObsHeader.dmjd_end[eph->svid-1] = max0;
	}
	SF_setNewSvMask(eph->svid);
	E("we will trigger gen lte,svid is %d",eph->svid);
#ifdef WIN32
	SetEvent(g_hObsUpdateEvent);
	E("sv %d get new obs, toe %ld\r\n ",eph->svid, eph->toe);
	Sleep(2000);   /*0.1 sec neccessary? */
	D("will saveobs now");
	if(!SF_saveObs("./FlashBackup"))  // 4ms need
	{
		E("Save OBS file failed when update!\n");
	}
#else
	sem_post(&g_hObsUpdateEvent);
	E("sv %d get new obs, toe %d\r\n ",eph->svid, eph->toe);

	D("will saveobs now");
	if(!SF_saveObs("/data/gnss/lte")) //4ms need
	{
		E("Save OBS file failed when update!\n");
	}
#endif

}

void SF_updateObsTask(char *buf,int lenth)
{
	GPS_EPHEMERIS sfEph;
	memset(&sfEph, 0, sizeof(GPS_EPHEMERIS));
	D("sf update obstask enter,lenth =%d,ephkepler=%d",lenth,(int)sizeof(GPS_EPHEMERIS));
	memcpy(&sfEph,buf,lenth);
	if(sfEph.svid > 32 || sfEph.svid < 1)
	{
	    E("===>>>error:lte updateobstask svid(%d) is wrong!!",sfEph.svid);
	    return;
	}
	D("receive obs data,svid=%d,weekno=%d,toe=%d",sfEph.svid,sfEph.week,sfEph.toe);
	SF_updateObsFileContents(&sfEph);
}
#ifdef WIN32
DWORD WINAPI SoonerFix_PredictThread(LPVOID lpParam)
#else
void SoonerFix_PredictThread(void *arg)
#endif
{
	int   i, first_update = 0;
    int  nRetVal;
	int result=TRUE;
	char pDataPath[MAX_PATH_SIZE];
	char pEphPath[MAX_PATH_SIZE];
#ifndef WIN32
    pid_t ttid = 0;
    int curpriority = 0;
#endif
	GpsState*  s = _gps_state;

	bThreadExited = FALSE;
#ifndef WIN32
	sem_init(&g_hObsUpdateEvent,0,0);
	D("sooner fix is enter,arg:%p\n",arg);
	ttid = gettid();
	//processId = pthread_self();
	D("processid of lte predict ttid=%d",ttid);
	curpriority = getpriority(PRIO_PROCESS,ttid);
	D("lte priority is %d",curpriority);
	sprintf(pDataPath, "%s","/data/gnss/lte");
    if(setpriority(PRIO_PROCESS,ttid,16) == 0)
    {
        D("lte process setpriority success");
    }
#else
	sprintf(pDataPath, "%s", "./FlashBackup");
#endif

	/* initialization */
	if (!SF_init())
	{
		E("No enough memory!\n");
		return;
	}

	if (!SF_loadObsFromFile(pDataPath))
	{
		E("SoonerFix_PredictThread: no obs file available\n");
		memset(&g_tObsHeader, 0, sizeof(XeodFileHeader));
		for(i=0; i<MAX_SV_PRN; i++)
		{
			g_tObsBlock[i].sv_id = 0;
			g_tObsBlock[i].num_of_obs  = 0;
			g_tObsBlock[i].eph_kep = g_tSvEph[i];
		}
	}
    /* load eph files */
#if SPREAD_ORBIT_EXTEND_KEPLER
    sprintf(pEphPath, "%s/%s",pDataPath, KEPL_EPH_FILE_NAME);
#else
    sprintf(pEphPath, "%s/%s",pDataPath, EPH_FILE_NAME);
#endif
	if(access(pEphPath,0) != -1)
	{
		SF_loadEphFromFile(pEphPath);
		//g_SF_newSvMask = 0;    //now should update by eph
	}
    else
        SF_initEphFile(pDataPath);

    for(i = 0; i < MAX_SV_PRN; i++)
	{
		//if(i+1 == 8)  continue;
		E("svid %d:mjd_obs %f,mjd_eph %f\n",i,mjd_obs[i],mjd_eph[i]);
		if((mjd_obs[i] > mjd_eph[i] + 0.08333) && (g_SF_newSvMask & ((unsigned int)1<<i)))          // 2 hours
		{
			E("svid=%d should update.\n",i+1);
			first_update = 1;
			g_SF_newSvMask = g_SF_newSvMask | (1<<i);   //update
			continue;      //break to continue
		}
	}

	s->lte_running = 1;
	/* prediction */
	while(!bThreadExitFlag)             //for test
	{
        if(first_update == 0)
        {
#ifdef WIN32
			WaitForSingleObject(g_hObsUpdateEvent, INFINITE);   //this is a signal,for test
#else
			sem_wait(&g_hObsUpdateEvent);   //this is a signal,for test
#endif
        }

        first_update = 0;
		E("after sem wait,g_SF_newSvMask:%d",g_SF_newSvMask);
		if(g_SF_newSvMask == 0)
			continue;
    E("XEOD_Init before enter,pDataPath:%p",pDataPath);
		nRetVal = XEOD_Init(&g_tObsHeader, NULL, pDataPath);
		if (nRetVal != XEOD_CG_SUCCESS)
		{
			//SF_freeResultBuffer();
			E("XEOD_CG Init Failed! Error Code: %d\n", nRetVal);
			continue;
			//return FALSE;
		}
		else
		{
			E("XEOD_CG Init Done!\n");
		}

		for(i=0; i<MAX_SV_PRN; i++)
		{
			if( (g_SF_newSvMask & ((unsigned int)1<<i)) == 0 ) /* obs is not update */
				continue;

			if(bThreadExitFlag)
				break;

			g_tEphBlock[i].eph_cart = g_tCartesianResult[i];
#if SPREAD_ORBIT_EXTEND_KEPLER
            g_tEphBlock[i].eph_kepl = g_tKeplerResult[i];
#endif
			if(!g_tObsHeader.eph_valid[i])
				continue;

			E("Processing SV: %2d...",i+1);

			/* lock when update */
#ifdef WIN32
			EnterCriticalSection(&g_SFCriticalSection);
#else
			pthread_mutex_lock(&g_SFCriticalSection);
#endif
			nRetVal=XEOD_GeneratePredEph(&g_tEphBlock[i], &g_tObsBlock[i], INTEG_STEPSIZE, 0);
#if SPREAD_ORBIT_EXTEND_KEPLER
            //20151222 add by hongxin.liu->for the output of orbit parameters from the requirement of firmware team
	        nRetVal=XEOD_GetKeplerEEBlock(&g_tEphBlock[i]);//, &g_tEEBinBlock[2520*i], &g_tWNBlock[42*i]
#endif
			if ( nRetVal != XEOD_CG_SUCCESS)
			{
				E("GeneratePredEph Failed! Error Code: %d\n",nRetVal);
			}
			else
			{
				E("GeneratePredEph Done!\n");
				g_bValidResult[i] = TRUE;
                SF_setSaveEphMask(i+1);
			}
			/* unlock  after predition */
#ifdef WIN32
			LeaveCriticalSection(&g_SFCriticalSection);
#else
			pthread_mutex_unlock(&g_SFCriticalSection);
#endif
			SF_clearNewSvMask(i+1);
		}
        /* no need to save all */
        if(!SF_saveEph(pDataPath))
		{
			E("Save EPH file failed!\n");
			result=FALSE;
		}
		else
		{
			E("save EPH file success");
		}
	}

	/* update eph file and deinit */
	if(!SF_saveEph(pDataPath))
	{
		E("Save EPH file failed!\n");
		result=FALSE;
	}
//for test
#if 1
	/* Save Obs */
	if(!SF_saveObs(pDataPath))
	{
		E("Save EPH file failed!\n");
		result=FALSE;
	}
#endif

	nRetVal=XEOD_Deinit();
	if ( nRetVal != XEOD_CG_SUCCESS)
	{
		result=FALSE;
		E("XEOD_CG Deinit Failed! Error Code: %d\n",nRetVal);
	}
	else
	{
		E("XEOD_CG Deinit Done!\n\n");
	}

	SF_freeResultBuffer();

	bThreadExited = TRUE;
	return;
}

#ifndef WIN32
int parse_lte_request(char *buf,unsigned short lenth)
{
	int mask = 0,week = 0,tow = 0;
    GpsState*  s = _gps_state;

    memcpy(&mask,buf,sizeof(int));
    memcpy(&week,buf + sizeof(int),sizeof(int));
    memcpy(&tow,buf + 2*sizeof(int),sizeof(int));
	D("receive lenth:%ud sav mask is 0x%x,week:%d,tow:%d",lenth,mask,week,tow);
    agpsdata->mask = mask;
    agpsdata->weekno = week;
    agpsdata->tow =  tow;
    send_agps_cmd(AGPS_LTE_CMD);
	return 0;
}
#endif
