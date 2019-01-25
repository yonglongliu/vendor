#ifndef ISP_ATM_H_
#define ISP_ATM_H_
#ifdef __cplusplus
extern "C"  {
#endif

struct atm_init_in {
    uint8_t  uOrigGamma[256];
};

struct atmctrl_cxt {
    void    *handle_algo;
    uint8_t  uOrigGamma[256];
    uint8_t  uATMGamma[256];
};


struct atm_init_param
{
    uint32_t u4Magic;
    uint8_t  uOrigGamma[256];
};

int isp_atm_init(struct atm_init_in *input_ptr, cmr_handle *handle_atm);
int isp_atm_deinit(cmr_handle *handle_atm);
void isp_atm2(cmr_handle *handle_atm, unsigned long long *pHist,
        int i4BV,
        unsigned int u4Bins,
        bool bHistB4Gamma,
        unsigned char *uBaseGamma,
        unsigned char *uModGamma,
        unsigned char *uOutputGamma,
        unsigned char* log_buffer, unsigned int* log_size);
int isp_atm(cmr_handle *handle_atm,
        struct atm_calc_param ATMInput,
        struct atm_calc_result *ATMOutput);
void isp_atm_smooth(unsigned int u4Weight, uint16_t u2Len,
        uint8_t *uOld, uint8_t *uNew, uint8_t *uOut);
int isp_hist(unsigned char *pbgr_2, bool bRGB, unsigned long long *hist, unsigned int width, unsigned int height);
int isp_histAEM(unsigned int* AEM_r, unsigned int* AEM_g, unsigned int* AEM_b, 
        unsigned long long *hist,
        bool bBinning,
        int shift,
        unsigned int width, unsigned int height,
        unsigned int blk_width, unsigned int blk_height);

#ifdef __cplusplus
}
#endif
#endif
