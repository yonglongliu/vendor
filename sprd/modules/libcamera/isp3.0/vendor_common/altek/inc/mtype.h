/******************************************************************************
 * File name: mtype.h
 *
 * Created on: 2014/9/9
 *     Author: HubertHuang
 ******************************************************************************/

#ifndef MTYPE_H_
#define MTYPE_H_

/* Global constant */
#ifndef TRUE
#define TRUE                    1
#endif

#ifndef FALSE
#define FALSE                   0
#endif

#ifndef NULL
#define NULL                    ((void *)0)
#endif

#ifndef UNUSED
#define UNUSED(x) (void)x
#endif

/* Global data type */

typedef float                   FLOAT32;
typedef double                  FLOAT64;

typedef char                    CHAR;

/* Notes for SINT8:
 * 1. The INT8 has bug. In fact it is "unsigned char". Use SINT8 to indicate "signed char".
 */
typedef signed char             SINT8;

/* Notes for INT8
 * 1. For the new developer, it is recommended to use SINT8 instead of INT8.
 * 2. If you are not the ARC developer, SINT8 is equal to INT8.
 * 3. Once if it is necessary to transfer source code to ARC OS, "INT8" should be all replaced with "SINT8".
 */
#ifdef _ARC
typedef char                    INT8;
#else
typedef signed char             INT8;
#endif
typedef unsigned char           UINT8;
typedef short                   INT16;
typedef unsigned short          UINT16;
typedef int                     INT32;
typedef unsigned int            UINT32;

#ifdef WIN32
typedef __int64                 INT64;
typedef unsigned __int64        UINT64;
#else
typedef long long               INT64;
typedef unsigned long long      UINT64;
#endif

typedef UINT8                   BOOL;
typedef UINT32                  ERRCODE;
typedef void                    *PVOID;
typedef char                    *PSTR;

typedef PVOID                   HWND;
typedef UINT16                  WPARAM;
typedef UINT32                  LPARAM;
typedef UINT32                  HANDLE;

typedef void (*CALLBACK)();

/* For Android platform */
typedef unsigned char           uint8;
typedef unsigned short          uint16;
typedef unsigned int            uint32;
typedef unsigned long long      uint64;
#ifdef _ARC
typedef char                    int8;
#else
typedef signed char             int8;
#endif
typedef short                   int16;
typedef int                     int32;
typedef long long               int64;

typedef signed char             sint8;

#endif /* MTYPE_H_ */
