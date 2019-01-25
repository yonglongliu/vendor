/*-------------------------------------------------------------------*/
/*  Copyright(C) 2017 by Spreadtrum                                  */
/*  All Rights Reserved.                                             */
/*-------------------------------------------------------------------*/
/* 
    Face beautify library API
*/

#ifndef __SPRD_FACEBEAUTY_API_H__
#define __SPRD_FACEBEAUTY_API_H__

#if (defined( WIN32 ) || defined( WIN64 )) && (defined FBAPI_EXPORTS)
#define FB_EXPORTS __declspec(dllexport)
#else
#define FB_EXPORTS
#endif

#ifndef FBAPI
#define FBAPI(rettype) extern FB_EXPORTS rettype
#endif

/* The error codes */
#define FB_OK                   0     /* Ok!                                      */
#define FB_ERROR_INTERNAL       -1    /* Error: Unknown internal error            */
#define FB_ERROR_NOMEMORY       -2    /* Error: Memory allocation error           */
#define FB_ERROR_INVALIDARG     -3    /* Error: Invalid argument                  */

/* The work modes */
#define FB_WORKMODE_STILL       0x00  /* Still mode: for capture                  */
#define FB_WORKMODE_MOVIE       0x01  /* Movie mode: for preview and video        */

/* Skin color defintions */
#define FB_SKINCOLOR_WHITE      0     /* White color                              */
#define FB_SKINCOLOR_ROSY       1     /* Rosy color                               */
#define FB_SKINCOLOR_WHEAT      2     /* The healthy wheat color                  */

#define FB_LIPCOLOR_CRIMSON     0     /* Crimson color                            */
#define FB_LIPCOLOR_PINK        1     /* Pink color                               */
#define FB_LIPCOLOR_FUCHSIA     2     /* Fuchsia colr                             */

/* Face beautify options */
typedef struct {
    unsigned char removeBlemishFlag; /* Flag for removing blemish; 0 --> OFF; 1 --> ON    */
    unsigned char skinSmoothLevel;   /* Smooth skin level. Value range [0, 10]            */
    unsigned char skinColorType;     /* The target skin color: white, rosy, or wheat      */
    unsigned char skinColorLevel;    /* The level to tune skin color. Value range [0, 10] */
    unsigned char skinBrightLevel;   /* Skin brightness level. Value range [0, 10]        */
    unsigned char lipColorType;      /* The target lip color: crimson, pink or fuchsia    */ 
    unsigned char lipColorLevel;     /* Red lips level. Value range [0, 10]               */
    unsigned char slimFaceLevel;     /* Slim face level. Value range [0, 10]              */
    unsigned char largeEyeLevel;     /* Enlarge eye level. Value range [0, 10]            */
    unsigned char debugMode;         /* Debug mode. 0 --> OFF; 1 --> ON                   */
} FB_BEAUTY_OPTION;


typedef enum {
    YUV420_FORMAT_CBCR = 0,          /* NV12 format; pixel order:  CbCrCbCr     */
    YUV420_FORMAT_CRCB = 1           /* NV21 format; pixel order:  CrCbCrCb     */
} FB_YUV420_FORMAT;

/* YUV420SP image structure */
typedef struct {
    int width;                       /* Image width                              */
    int height;                      /* Image height                             */
    FB_YUV420_FORMAT format;         /* Image format                             */
    unsigned char *yData;            /* Y data pointer                           */
    unsigned char *uvData;           /* UV data pointer                          */
} FB_IMAGE_YUV420SP;


/* The face information structure */
typedef struct
{
    int x, y, width, height;        /* Face rectangle                            */
    int yawAngle;                   /* Out-of-plane rotation angle (Yaw);In [-90, +90] degrees;   */
    int rollAngle;                  /* In-plane rotation angle (Roll);   In (-180, +180] degrees; */
} FB_FACEINFO;


/* The face beauty handle */
typedef void * FB_BEAUTY_HANDLE;

#ifdef  __cplusplus
extern "C" {
#endif

/* Get the software version */
FBAPI(const char *) FB_GetVersion();

/* Initialize the FB_BEAUTY_OPTION structure by default values */
FBAPI(void) FB_InitBeautyOption(FB_BEAUTY_OPTION *option);

/*
\brief Create a Face Beauty handle
\param hFB          Pointer to the created Face Beauty handle
\param workMode     Work mode: FB_WORKMODE_STILL or FB_WORKMODE_MOVIE
\param threadNum    Number of thread to use. Value range [1, 4]
\return Returns FB_OK if successful. Returns negative numbers otherwise.
*/
FBAPI(int) FB_CreateBeautyHandle(FB_BEAUTY_HANDLE *hFB, int workMode, int threadNum);

/* Release the Face Beauty handle */
FBAPI(void) FB_DeleteBeautyHandle(FB_BEAUTY_HANDLE *hFB);

/* 
\brief Do face beautification on the YUV420SP image
\param hFB          The Face Beauty handle
\param imageYUV     Pointer to the YUV420SP image. The image is processed in-place
\param option       The face beautification options
\param faceInfo     The head of the face array. 
\param faceCount    The face count in the face array.
\return Returns FB_OK if successful. Returns negative numbers otherwise.
*/
FBAPI(int) FB_FaceBeauty_YUV420SP(FB_BEAUTY_HANDLE hFB, 
                                  FB_IMAGE_YUV420SP *imageYUV,
                                  const FB_BEAUTY_OPTION *option,
                                  const FB_FACEINFO *faceInfo,
                                  int faceCount);



#ifdef  __cplusplus
}
#endif

#endif /* __SPRD_FACEBEAUTY_API_H__ */
