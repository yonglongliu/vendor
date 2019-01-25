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

#include "isp_type.h"
#include "isp_adpt.h"


extern struct adpt_register_type ae_altek_adpt_info;
static struct adpt_register_type *ae_adpt_reg_table[] = {
	&ae_altek_adpt_info,
};

static cmr_int adpt_get_ae_ops(struct isp_lib_config *lib_info,
			       struct adpt_ops_type **ops)
{
	cmr_int ret = -1;
	cmr_u8 i = 0;
	struct adpt_register_type **table = ae_adpt_reg_table;

	for (i = 0; i < ARRAY_SIZE(ae_adpt_reg_table); i++) {
		if (lib_info->product_id == table[i]->lib_info->product_id) {
			if (lib_info->version_id == table[i]->lib_info->version_id) {
				*ops = table[i]->ops;
				ret = 0;
				break;
			}
		}
	}

	return ret;
}

extern struct adpt_register_type af_altek_adpt_info;
static struct adpt_register_type *af_adpt_reg_table[] = {
	&af_altek_adpt_info,
};

static cmr_int adpt_get_af_ops(struct isp_lib_config *lib_info,
			       struct adpt_ops_type **ops)
{
	cmr_int ret = -1;
	cmr_u8 i = 0;
	struct adpt_register_type **table = af_adpt_reg_table;

	for (i = 0; i < ARRAY_SIZE(af_adpt_reg_table); i++) {
		if (lib_info->product_id == table[i]->lib_info->product_id) {
			if (lib_info->version_id == table[i]->lib_info->version_id) {
				*ops = table[i]->ops;
				ret = 0;
				break;
			}
		}
	}

	return ret;
}

extern struct adpt_register_type awb_altek_adpt_info;
static struct adpt_register_type *awb_adpt_reg_table[] = {
	&awb_altek_adpt_info,
};

static cmr_int adpt_get_awb_ops(struct isp_lib_config *lib_info,
				struct adpt_ops_type **ops)
{
	cmr_int ret = -1;
	cmr_u8 i = 0;
	struct adpt_register_type **table = awb_adpt_reg_table;

	for (i = 0; i < ARRAY_SIZE(awb_adpt_reg_table); i++) {
		if (lib_info->product_id == table[i]->lib_info->product_id) {
			if (lib_info->version_id == table[i]->lib_info->version_id) {
				*ops = table[i]->ops;
				ret = 0;
				break;
			}
		}
	}

	return ret;
}

struct adpt_register_type *lsc_adpt_reg_table[2]; /* TBD */
static cmr_int adpt_get_lsc_ops(struct isp_lib_config *lib_info,
				struct adpt_ops_type **ops)
{
	cmr_int ret = -1;
	cmr_u8 i = 0;
	struct adpt_register_type **table = lsc_adpt_reg_table;

	for (i = 0; i < ARRAY_SIZE(lsc_adpt_reg_table); i++) {
		if (lib_info->product_id == table[i]->lib_info->product_id) {
			if (lib_info->version_id == table[i]->lib_info->version_id) {
				*ops = table[i]->ops;
				ret = 0;
				break;
			}
		}
	}

	return ret;
}


extern struct adpt_register_type afl_altek_adpt_info;
static struct adpt_register_type *afl_adpt_reg_table[] = {
	&afl_altek_adpt_info,
};
static cmr_int adpt_get_afl_ops(struct isp_lib_config *lib_info,
			       struct adpt_ops_type **ops)
{
	cmr_int ret = -1;
	cmr_u8 i = 0;
	struct adpt_register_type **table = afl_adpt_reg_table;

	for (i = 0; i < ARRAY_SIZE(afl_adpt_reg_table); i++) {
		if (lib_info->product_id == table[i]->lib_info->product_id) {
			if (lib_info->version_id == table[i]->lib_info->version_id) {
				*ops = table[i]->ops;
				ret = 0;
				break;
			}
		}
	}

	return ret;
}

extern struct adpt_register_type pdaf_altek_adpt_info;
static struct adpt_register_type *pdaf_adpt_reg_table[] = {
	&pdaf_altek_adpt_info,
};

static cmr_int adpt_get_pdaf_ops(struct isp_lib_config *lib_info,
			       struct adpt_ops_type **ops)
{
	cmr_int ret = -1;
	cmr_u8 i = 0;
	struct adpt_register_type **table = pdaf_adpt_reg_table;

	for (i = 0; i < ARRAY_SIZE(pdaf_adpt_reg_table); i++) {
		if (lib_info->product_id == table[i]->lib_info->product_id) {
			if (lib_info->version_id == table[i]->lib_info->version_id) {
				*ops = table[i]->ops;
				ret = 0;
				break;
			}
		}
	}

	return ret;
}

static cmr_int (*modules_ops[]) (struct isp_lib_config *,
				 struct adpt_ops_type **) = {
	[ADPT_LIB_AE] = adpt_get_ae_ops,
	[ADPT_LIB_AF] = adpt_get_af_ops,
	[ADPT_LIB_AWB] = adpt_get_awb_ops,
	[ADPT_LIB_LSC] = adpt_get_lsc_ops,
	[ADPT_LIB_AFL] = adpt_get_afl_ops,
	[ADPT_LIB_PDAF] = adpt_get_pdaf_ops,
};

cmr_int adpt_get_ops(cmr_int lib_type, struct isp_lib_config *lib_info,
		     struct adpt_ops_type **ops)
{
	cmr_int ret = -1;

	if (ADPT_LIB_MAX > lib_type)
		ret = modules_ops[lib_type](lib_info, ops);

	return ret;
}
