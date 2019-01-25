#ifndef _ARCSOFT_BEAUTYSHOT_WRAPPER_H_
#define _ARCSOFT_BEAUTYSHOT_WRAPPER_H_

#include "merror.h"
#include "ammem.h"
#include "asvloffscreen.h"
#include "abstypes.h"

#ifdef __cplusplus
extern "C" {
#endif

MInt32 arcsoft_bsv_create(MHandle *cxt);
MInt32 arcsoft_bsv_destroy(MHandle cxt);
const MPBASE_Version* arcsoft_bsv_get_version(MHandle cxt);
MRESULT arcsoft_bsv_init(MHandle cxt);
MRESULT arcsoft_bsv_fini(MHandle cxt);
MVoid arcsoft_bsv_set_feature_level(MHandle cxt, MInt32 feature_key, MInt32 level);
MInt32 arcsoft_bsv_get_feature_level(MHandle cxt, MInt32 feature_key);
MRESULT arcsoft_bsv_process(MHandle cxt, LPASVLOFFSCREEN src, LPASVLOFFSCREEN dst, ABS_TFaces *faces_in, ABS_TFaces *faces_out);

MInt32 arcsoft_bsi_create(MHandle *cxt);
MInt32 arcsoft_bsi_destroy(MHandle cxt);
const MPBASE_Version* arcsoft_bsv_get_version(MHandle cxt);
MRESULT arcsoft_bsi_init(MHandle cxt);
MRESULT arcsoft_bsi_fini(MHandle cxt);
MVoid arcsoft_bsi_set_feature_level(MHandle cxt, MInt32 feature_key, MInt32 level);
MInt32 arcsoft_bsi_get_feature_level(MHandle cxt, MInt32 feature_key);
MRESULT arcsoft_bsi_process(MHandle cxt, LPASVLOFFSCREEN src, LPASVLOFFSCREEN dst, ABS_TFaces *faces_in, ABS_TFaces *faces_out);

#ifdef __cplusplus
}
#endif

#endif

