
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <memory.h>
#include "common.h"
#include "protocol.h"
#include "tee_client_api.h"

int main( int arg, char *argv[])
{
	TEEC_Result result;
	TEEC_Context context = {0};
	TEEC_Session session = {0};
	TEEC_Operation operation = {0};
	TEEC_SharedMemory inputMem01 = {0};
	TEEC_SharedMemory inputMem02 = {0};
	TEEC_UUID uuid = TA_UUID;
	int len01 = 1024;
	int len02 = 16*1024;

	memset( &context, 0, sizeof( TEEC_Context));
	result = TEEC_InitializeContext((const char *) NULL, &context);
	if( TEEC_SUCCESS != result)
	{
    	LOGE( "E> init context failed, result=0x%08X.\n", result);
	    return result;
	}
	LOGI( "I> initialize context success.\n");

	memset( &session, 0, sizeof(TEEC_Session));
	memset( &operation, 0, sizeof( TEEC_Operation));
	operation.started       = 0;
	operation.paramTypes    = TEEC_NONE;

	result = TEEC_OpenSession( &context, &session, &uuid,
			TEEC_LOGIN_PUBLIC, NULL, NULL, NULL);
	if( TEEC_SUCCESS != result)
	{
		LOGE( "E> open session failed, result=0x%08X.\n", result);
		TEEC_FinalizeContext(&context);
		return result;
	}
	LOGI( "I> open session success.\n");
	result = TEEC_InvokeCommand( &session, CMD_TEST_INTEL_SPI,
			NULL, NULL);
	if( TEEC_SUCCESS != result)
	{
		LOGE( "E> invoke command failed, result=0x%08X.", result);
	}
	else
	{
		LOGE( "E> invoke command success.\n");
	}

	TEEC_CloseSession( &session);
	TEEC_FinalizeContext(&context);

	return;
}

#ifdef __cplusplus
}
#endif


