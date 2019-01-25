#ifndef __EMMC__H__
#define __EMMC__H__

//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
//add the emmc test interface
//-----------------------------------------------------------------------------
#define TEXT_EMMC_FAIL          "No EMMC on the Phone"
#define TEXT_EMMC_CAPACITY              "The Capacity of EMMC:"
#define TEXT_EMMC_OK            "EMMC on the Phone"
#define MENU_TEST_EMMC          "> Test EMMC"
#define TEXT_EMMC_STATE                         "EMMC status:"
int get_emmc_size(char *req, char *rsp);
int get_emmcsize(void);

//-----------------------------------------------------------------------------
//--};
#ifdef __cplusplus
}
#endif // __cplusplus
//-----------------------------------------------------------------------------

#endif //add the emmc h_
