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
#ifndef _AF_ADPT_H_
#define _AF_ADPT_H_

struct af_adpt_init_in {
	cmr_handle caller_handle;
	struct af_ctrl_init_in *ctrl_in;
	struct af_ctrl_cb_ops_type cb_ctrl_ops;
};

#endif
