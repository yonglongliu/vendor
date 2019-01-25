#ifndef _ISP_ADPT_H_
#define _ISP_ADPT_H_

#include "cmr_types.h"

enum ae_product_lib_id {
	ADPT_SPRD_AE_LIB,
	ADPT_AL_AE_LIB,
	ADPT_MAX_AE_LIB,
};

enum awb_product_lib_id {
	ADPT_SPRD_AWB_LIB,
	ADPT_AL_AWb_LIB,
	ADPT_MAX_AWB_LIB,
};

enum af_product_lib_id {
	ADPT_SPRD_AF_LIB,
	ADPT_AL_AF_LIB,
	ADPT_SFT_AF_LIB,
	ADPT_MAX_AF_LIB,
};

enum lsc_product_lib_id {
	ADPT_SPRD_LSC_LIB,
	ADPT_MAX_LSC_LIB,
};

enum pdaf_product_lib_id {
	ADPT_SPRD_PDAF_LIB,
	ADPT_MAX_PDAF_LIB,
};

enum adpt_lib_type {
	ADPT_LIB_AE,
	ADPT_LIB_AF,
	ADPT_LIB_AWB,
	ADPT_LIB_LSC,
	ADPT_LIB_AFL,
	ADPT_LIB_PDAF,
	ADPT_LIB_MAX,
};

struct adpt_ops_type {
	void *(*adpt_init) (void *in, void *out);
	 cmr_s32(*adpt_deinit) (void *handle, void *in, void *out);
	 cmr_s32(*adpt_process) (void *handle, void *in, void *out);
	 cmr_s32(*adpt_ioctrl) (void *handle, cmr_s32 cmd, void *in, void *out);
};

struct adpt_register_type {
	cmr_s32 lib_type;
	struct third_lib_info *lib_info;
	struct adpt_ops_type *ops;
};

cmr_s32 adpt_get_ops(cmr_s32 lib_type, struct third_lib_info *lib_info, struct adpt_ops_type **ops);
#endif
