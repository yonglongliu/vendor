/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SYSTEM_KEYMASTER1_ATTESTATION_TAGS_H_
#define SYSTEM_KEYMASTER1_ATTESTATION_TAGS_H_
#if (TRUSTY_KEYMASTER_VERSION == 1)
#include <keymaster/keymaster_tags.h>
namespace keymaster {

#define KM_TAG_ATTESTATION_APPLICATION_ID ((keymaster_tag_t)(KM_BYTES | 709))
#define KM_TAG_ATTESTATION_ID_BRAND ((keymaster_tag_t)(KM_BYTES | 710))
#define KM_TAG_ATTESTATION_ID_DEVICE ((keymaster_tag_t)(KM_BYTES | 711))
#define KM_TAG_ATTESTATION_ID_PRODUCT ((keymaster_tag_t)(KM_BYTES | 712))
#define KM_TAG_ATTESTATION_ID_SERIAL ((keymaster_tag_t)(KM_BYTES | 713))
#define KM_TAG_ATTESTATION_ID_IMEI ((keymaster_tag_t)(KM_BYTES | 714))
#define KM_TAG_ATTESTATION_ID_MEID ((keymaster_tag_t)(KM_BYTES | 715))
#define KM_TAG_ATTESTATION_ID_MANUFACTURER ((keymaster_tag_t)(KM_BYTES | 716))
#define KM_TAG_ATTESTATION_ID_MODEL ((keymaster_tag_t)(KM_BYTES | 717))

DECLARE_KEYMASTER_TAG(KM_BYTES, TAG_ATTESTATION_APPLICATION_ID);
DECLARE_KEYMASTER_TAG(KM_BOOL, TAG_RESET_SINCE_ID_ROTATION);
DECLARE_KEYMASTER_TAG(KM_BYTES, TAG_ATTESTATION_ID_BRAND);
DECLARE_KEYMASTER_TAG(KM_BYTES, TAG_ATTESTATION_ID_DEVICE);
DECLARE_KEYMASTER_TAG(KM_BYTES, TAG_ATTESTATION_ID_PRODUCT);
DECLARE_KEYMASTER_TAG(KM_BYTES, TAG_ATTESTATION_ID_SERIAL);
DECLARE_KEYMASTER_TAG(KM_BYTES, TAG_ATTESTATION_ID_IMEI);
DECLARE_KEYMASTER_TAG(KM_BYTES, TAG_ATTESTATION_ID_MEID);
DECLARE_KEYMASTER_TAG(KM_BYTES, TAG_ATTESTATION_ID_MANUFACTURER);
DECLARE_KEYMASTER_TAG(KM_BYTES, TAG_ATTESTATION_ID_MODEL);

#define KM_ERROR_ATTESTATION_APPLICATION_ID_MISSING ((keymaster_error_t)-65)
#define KM_ERROR_CANNOT_ATTEST_IDS ((keymaster_error_t)-66)
}
#endif
#endif
