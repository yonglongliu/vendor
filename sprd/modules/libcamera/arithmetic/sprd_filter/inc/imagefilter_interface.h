#ifndef __IMAGEFILTER_INTERFACE_H__
#define __IMAGEFILTER_INTERFACE_H__

#ifdef WIN32
#define JNIEXPORT
#else
#include <jni.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void *IFENGINE;
#define IFLUT64_SIZE 512

typedef enum { // optional params
    NoneFilter = 0,
    // ColorMapFilter = 1,			// param1:IFMap256
    // PixelFilter = 2,				// param1:IFSize
    // InvertColorFilter = 3,
    MonoFilter = 4,
    MonoChromeFilter = 5,
    // ColorMatrixFilter = 6,		// param1:IFMatrix3x3
    // GrayFilter = 7,
    NostalgiaFilter = 8,
    // LookupFilter = 9,				// param1:IFLUT64
    Strip2Filter = 10,
    Strip3Filter,
    BerylFilter,
    BrannanFilter,
    CrispWarmFilter,
    CrispWinterFilter,
    EarlybirdFilter,
    FallColorsFilter,
    Filmstock50Filter,
    GothamFilter,
    HefeFilter,
    HorrorBlueFilter,
    InkwellFilter,
    LomofiFilter,
    LordKelvinFilter,
    NashvilleFilter,
    SoftWarmingFilter,
    SutroFilter,
    TealOrangePlusContrastFilter,
    TensionGreenFilter,
    ToasterFilter,
    WaldenFilter,
    XProIIFilter,
} IFFilterType;

typedef enum {
    NV21,
    RGB888,
} IFImageType;

typedef struct {
    IFImageType imageType;
    int imageWidth;
    int imageHeight;
    char *paramPath;
} IFInitParam;

typedef struct {
    void *param1;
    void *param2;
} IFFilterParam;

typedef struct { unsigned char val[3][3]; } IFMatrix3x3;

typedef struct {
    unsigned char R[256];
    unsigned char G[256];
    unsigned char B[256];
} IFMap256;

typedef struct {
    int R;
    int G;
    int B;
} IFVec3;

typedef struct {
    int x;
    int y;
} IFSize;

typedef struct {
    // B-Quad G-Row R-Col
    unsigned char val[IFLUT64_SIZE][IFLUT64_SIZE][3]; // 512 x 512 RGB texture
} IFLUT64;

typedef struct {
    void *c1;
    void *c2;
    void *c3;
} IFImageData;

JNIEXPORT IFENGINE ImageFilterCreateEngine(IFInitParam *initParam);
JNIEXPORT int ImageFilterRun(IFENGINE engine, IFImageData *input,
                             IFImageData *output, IFFilterType filterType,
                             IFFilterParam *filterParam);
JNIEXPORT int ImageFilterDestroyEngine(IFENGINE engine);

#ifdef __cplusplus
}
#endif

#endif
