#include "tee_internal_api.h"
#include "wdta_debug_protocol.h"

TEE_Result TA_EXPORT TA_CreateEntryPoint(void)
{
	LOGI("I-wdta> TA_CreateEntryPoint.");

	return TEE_SUCCESS;
}
void TA_EXPORT TA_DestroyEntryPoint(void)
{
	LOGI("I-wdta> TA_DestroyEntryPoint.");

	return;
}

TEE_Result TA_EXPORT TA_OpenSessionEntryPoint( uint32_t nParamTypes,
		TEE_Param pParams[4], void ** ppSessionContext)
{

	LOGI("I-wdta> TA_OpenSessionEntryPoint.");

	return TEE_SUCCESS;
}

void TA_EXPORT TA_CloseSessionEntryPoint( void * ppSessionContext)
{

	LOGI("I-wdta> TA_CloseSessionEntryPoint.");

	return;
}
TEE_Result TA_EXPORT TA_InvokeCommandEntryPoint( void *pSessionContext, 
		uint32_t nCommandID, uint32_t nParamTypes, TEE_Param pParam[4])
{
	TEE_Result result = TEE_SUCCESS;
	
	LOGI("I-wdta> TA_InvokeCommandEntryPoint.");
	LOGI("I-wdta> InvokeCommand, cmd=0x%X.", nCommandID);


	return result;
}
