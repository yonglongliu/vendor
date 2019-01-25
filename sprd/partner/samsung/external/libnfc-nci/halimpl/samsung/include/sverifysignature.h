#ifndef S_VERIFYSIGNATURE_H
#define S_VERIFYSIGNATURE_H

#define MC_SIGNATURE_SIZE 256
#define SVERIFY_RET_SUCCESS 0

#define SVERIFY_ERROR_BASE_FUNCTION 100

#ifdef __cplusplus
extern "C" {
#endif
int SVerifyFWSignature(const char *pCertPath, const char* sImgPath);
int SVerifyFWSignaturePImg(const char* sImgPath, char* pOutPlainImg, unsigned long* pOutPlainImgSize);
#ifdef __cplusplus
} 
#endif

#endif