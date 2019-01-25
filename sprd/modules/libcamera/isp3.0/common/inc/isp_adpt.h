#ifndef _ISP_ADPT_H_
#define _ISP_ADPT_H_
#include "isp_common_types.h"

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
	cmr_int (*adpt_init) (void *in, void *out, cmr_handle *handle);
	cmr_int (*adpt_deinit) (cmr_handle handle);
	cmr_int (*adpt_process) (cmr_handle handle, void *in, void *out);
	cmr_int (*adpt_ioctrl) (cmr_handle handle, cmr_int cmd, void *in, void *out);
};

struct adpt_register_type {
	cmr_int lib_type;
	struct isp_lib_config *lib_info;
	struct adpt_ops_type *ops;
};

cmr_int adpt_get_ops(cmr_int lib_type, struct isp_lib_config *lib_info,
		     struct adpt_ops_type **ops);
#endif
