/*
*    Copyright (C) 2013 SAMSUNG S.LSI
*
*   Licensed under the Apache License, Version 2.0 (the "License");
*   you may not use this file except in compliance with the License.
*   You may obtain a copy of the License at:
*
*   http://www.apache.org/licenses/LICENSE-2.0
 *
*   Unless required by applicable law or agreed to in writing, software
*   distributed under the License is distributed on an "AS IS" BASIS,
*   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*   See the License for the specific language governing permissions and
*   limitations under the License.
*
*   Author: Woonki Lee <woonki84.lee@samsung.com>
 *
*/
#ifndef __NFC_SEC_HALUTIL__
#define __NFC_SEC_HALUTIL__

#include "osi.h"

#ifndef __bool_true_false_are_defined
#define __bool_true_false_are_defined
typedef enum {false, true} bool;
#endif

#define HAL_UTIL_GET_INT_16 0x0001

#ifdef FWU_APK
#define EFS_SIGN_LENGTH 256
#endif

bool get_config_int(char *field, int *data);
int get_config_byteArry(char *field, uint8_t *byteArry, size_t arrySize);
int get_config_string(char *field, char *strBuffer, size_t bufferSize);
int get_config_count(char *field);
uint8_t get_config_propnci_get_oid(int n);
int get_hw_rev();
#ifdef FWU_APK
bool file_signature_check(char *file_path);
#endif

#ifdef NFC_HAL_NCI_TRACE
#define util_nci_analyzer(x)    sec_nci_analyzer(x)
#else
#define util_nci_analyzer(x)
#endif

#endif //__NFC_SEC_HALUTIL__
