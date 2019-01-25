/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _AE_MISC_H_
#define _AE_MISC_H_

#include "cmr_types.h"

#ifdef __cplusplus
extern "C" {
#endif

	struct ae_misc_init_in {
		cmr_u32 alg_id;
		cmr_u32 flash_version;
		cmr_u32 start_index;
		cmr_handle param_ptr;
		cmr_u32 size;
	};

	struct ae_misc_init_out {
		cmr_u32 start_index;
		char alg_id[32];
	};

	struct ae_misc_calc_in {
		cmr_handle sync_settings;
	};

	struct ae_misc_calc_out {
		cmr_handle ae_output;
	};

	cmr_handle ae_misc_init(struct ae_misc_init_in *in_param, struct ae_misc_init_out *out_param);
	cmr_s32 ae_misc_deinit(cmr_handle handle, cmr_handle in_param, cmr_handle out_param);
	cmr_s32 ae_misc_calculation(cmr_handle handle, struct ae_misc_calc_in *in_param, struct ae_misc_calc_out *out_param);

#ifdef __cplusplus
}
#endif
#endif
