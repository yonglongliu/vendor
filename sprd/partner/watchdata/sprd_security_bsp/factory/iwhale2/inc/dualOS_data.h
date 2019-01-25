#ifndef __DUALOS_DATA_H
#define __DUALOS_DATA_H

#define DualOS_CERT_NUM_MAX  10

/* version counters value */
typedef enum {

	/* trusted version */
	DualOS_DX_SW_VERSION_COUNTER1 = 1,
	/* non trusted version */
	DualOS_DX_SW_VERSION_COUNTER2,

	DualOS_DX_SW_VERSION_MAX      = 0x7FFFFFFF

}DualOS_DxSbSwVersionId_t;

/* sw version */
typedef struct {
	DualOS_DxSbSwVersionId_t  id;
	uint32_t swVersion;
}DualOS_DxSbSwVersion_t;

typedef struct{
	DualOS_DxSbSwVersion_t swVersionInfo[DualOS_CERT_NUM_MAX];
	uint32_t num;
}DualOS_SwVersionRecordInfo_t;

typedef struct{
    uint32_t g_DX_Hbk[8];
    uint32_t g_pubKeyHash[8];
    uint32_t iNodeCounter;
    DualOS_SwVersionRecordInfo_t swVersionRecordInfo;
}DualOSData;

extern DualOSData *pDualDateVal;

#endif
