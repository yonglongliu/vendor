/************************************************************************
 *
 *  Module:       TfascApi_defs.h
 *  Description:  tfa side channel driver API defs
 *
 *  Runtime Env.: Kernel 
 *  Author(s):    Udo Eberhardt, Frank Hoffmann
 *  Company:      Thesycon GmbH, Ilmenau
 ************************************************************************/

#ifndef __TfascApi_defs_h__
#define __TfascApi_defs_h__

typedef unsigned __int64 TfaDeviceId;


typedef struct tagTfaDeviceInfo
{
  unsigned int numTfaChips;

} TfaDeviceInfo;


typedef struct tagTfaChipInfo
{
	unsigned int busNumber;
  unsigned char slaveAddress;
} TfaChipInfo;



#endif // __TfascApi_defs_h__

/******************************** EOF ***********************************/
