/*
 * Copyright (C) 2012 The Android Open Source Project
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
#ifndef _AE_DEBUG_INFO_PARSER_H_
#define _AE_DEBUG_INFO_PARSER_H_

#include "cmr_types.h"

#ifdef __cplusplus
extern "C" {
#endif

	struct ae_debug_info_packet_in {
		char id[32];
		cmr_handle aem_stats;
		cmr_handle alg_status;
		cmr_handle alg_results;
		cmr_handle packet_buf;
	};

	struct ae_debug_info_packet_out {
		cmr_u32 size;
	};

	struct ae_debug_info_unpacket_in {
		char alg_id[32];
		cmr_handle packet_buf;
		cmr_u32 packet_len;
		cmr_handle unpacket_buf;
		cmr_u32 unpacket_len;
	};

	struct ae_debug_info_unpacket_out {
		cmr_u32 reserved;
	};

	cmr_s32 ae_debug_info_packet(cmr_handle input, cmr_handle output);
	cmr_s32 ae_debug_info_unpacket(cmr_handle input, cmr_handle output);
	cmr_handle ae_debug_info_get_lib_version(void);

#ifdef __cplusplus
}
#endif
#endif
