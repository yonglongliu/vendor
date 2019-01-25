#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/***************************************************************************************************
****************************************************************************************************
**  this function generates a string infomation called 'Secure element unique ID'
**  for NXP NFC device. this ID is required by Factory testing
**
**  PARAM:  
**  	cplc [out]	-	buffer that CPLC data will saved to. 
**		len [in]	-	length of cplc buff. 
**						WARNING: the length of this buffer MUST be initialized more then 104 (bytes)
**						when buffer is malloc 
**
**  RETURN:
**			0		-	success
**			-1		-	internal ERROR
**			-2		-	not find raw test_result file (/data/nfc/test_result)
**			-3		-	CPLC data not found
**			-4		-	IC Perso.Equipment ID not match
**			-5		-	command result not pass
**
**  DESCRIPTION:
**  	this function will parse the specific section called CPLC in '/data/nfc/test_result', which
**		is generated after MMI NFC test.  finally get CPLC data 104 characters in all
**
****************************************************************************************************
****************************************************************************************************/
typedef struct _cplc
{
	char frameInfo[16];
	char IcFabricator[4];
	char IcType[4];
	char OSId[4];
	char OSReleaseDate[4];
	char OSReleaseLev[4];
	char ICFabricationDate[4];
	char ICSn[8];
	char ICBatchId[4];
	char ICModuleFabricator[4];
	char ICModulePackageDate[4];
	char ICCManufacturer[4];
	char ICEmbeddingDate[4];
	char ICPrePersonalizer[4];
	char ICPrePersoEquipmentDate[4];
	char ICPrePersoEquipmentId[8];
	char ICPersonalizer[4];
	char ICPersonalizerDate[4];
	char ICPersoEquipmentID[8];
	char cmdResult[4];
} _CPLC;

int getNfcCPLC(char* cplc, int len)
{
	const char *sNfcTestResultFilePath = "/data/nfc/test_result";
#ifdef SAMSUNG_NFC
        const char *cplc_key_str = "01001D72509F7F2A";
#elif defined NXP_NFC
        const char *cplc_key_str = "03003199509F7F2A";
#else
        const char *cplc_key_str = "";
#endif
	//const char *sICPersoEquipmentID = "80464158";
	const char *sICPersoEquipmentID = "0054514D";
	const char *sCmdResult = "9000";
	
	const int cplc_len = 104;
	char CPLC[104] = {0};
	char line[1024] = {'\0'};

	_CPLC* pCPLC = 0;
	char *temp = 0;
	char *pos = 0;
	char charactor = 0;
	int find = 0;
	int fail = 0;
    int i = 0;

	if(cplc == 0 || len < 104)
	{
		printf("internal ERROR: has not malloc a buff\n");
		return -1;
	}

	memset(cplc, 0, len);
	
	FILE *pFile = fopen(sNfcTestResultFilePath, "r");
	if(!pFile)
	{
		perror("open /data/nfc/test_result ");
		return -2;
	}

	while(fgets(line, sizeof(line), pFile) != 0)
	{
		pos = strstr(line, cplc_key_str);
		if(pos)
		{
			i = line + strlen(line) - 1 - pos;
			//printf("i = %d\n",i);
			if(i != cplc_len)
			{
				printf("internal ERROR: wrong CPLC len %d\n",i);
				fail = -1;
				goto FAIL;
			}
			
			memcpy(CPLC, pos, i);
			
			find = 1;
			break;
		}

        memset(line, 0, sizeof(line));
	}

	if(find != 1)
	{
		printf("CPLC info not found, fail\n");
		fail = -3;
		goto FAIL;
	}
	
	pCPLC = (_CPLC*)CPLC;
	
	/*if(strncmp(pCPLC->ICPersoEquipmentID, sICPersoEquipmentID, strlen(sICPersoEquipmentID)))
	{
		printf("ICPersoEquipmentID not matched, fail\n");
		fail = -4;
		goto FAIL;
	}*/
	
	if(strncmp(pCPLC->cmdResult, sCmdResult, strlen(sCmdResult)))
	{
		printf("cmdResult not matched, fail\n");
		fail = -5;
		goto FAIL;
	}

	memcpy(cplc, CPLC, cplc_len);
	
	FAIL:
		fclose(pFile);
		pFile = 0;
		return fail;				
}


/***************************************************************************************************
****************************************************************************************************
**  this function save NFC CPLC data to ProdNV
**  for NXP NFC device. required by Factory testing
**
**  PARAM:  
**  	cplc [in]	-	CPLC data
**
**  RETURN:
**			0		-	success
**			-6		-	input cplc data ERROR
**			-7		-	input cplc data length error
**			-8		-	can not open ProdNV file
**			-9		-	write data to Prodnv fail
****************************************************************************************************
****************************************************************************************************/
int save_cplc_to_prodnv(char* cplc)
{
	const char* sNfcCplcFilePath = "/productinfo/nfccplc.txt";
	const int cplc_len = 104;
	int fail = 0;
	
	if(!cplc)
	{
		printf("cplc data not ready\n");
		return -6;
	}
	
	if(strlen(cplc) != cplc_len)
	{
		printf("cplc data length is invalid\n");
		return -7;
	}
	
	FILE *pFile = fopen(sNfcCplcFilePath, "w");
	if(!pFile)
	{
		perror("open /productinfo/nfccplc.txt ");
		return -8;
	}
	
	if(fputs(cplc, pFile) == EOF)
	{
		perror("write cplc to prodnv fail. ");
		fail = -9;
		goto FAIL;
	}

        if(fflush(pFile) == EOF)
        {
		perror("write(SYNC) cplc to prodnv fail. ");
		fail = -9;
		goto FAIL;
        }

	printf("save cplc data done\n");
	
	FAIL:
		fclose(pFile);
		pFile = 0;
		return fail;
}


int main(void)
{
    char *cplc = malloc(105);
    memset(cplc, 0, 105);
    
    int ret = getNfcCPLC(cplc, 105);
    printf("ret = %d , cplc: %s\n", ret, cplc);
    
    if(!ret)
    {
		if((ret = save_cplc_to_prodnv(cplc)) < 0)
		{
			printf("save cplc fail...rc=%d\n",ret);
		}
	}

    return ret;
}
