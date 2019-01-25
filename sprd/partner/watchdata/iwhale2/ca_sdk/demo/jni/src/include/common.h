#ifndef 	__WDCA_DEBUG_COMMON_H__
#define 	__WDCA_DEBUG_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <tee_client_api.h>

#define LOGI printf
#define LOGW printf
#define LOGD printf
#define LOGE printf

typedef TEEC_Result (*WDCA_Debug_TestFunc_ptr)(void);
				      
typedef struct _WDCA_Debug_TestIdentifier
{
	int valid;
	char id[64];
	char name[128];
	char file[64];
	WDCA_Debug_TestFunc_ptr func;
}WDCA_Debug_TestIdentifier_t;


#ifdef __cplusplus
}
#endif

#endif
