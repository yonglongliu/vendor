/********************************************************************************
 * al3awrapper_af.h
 *
 *  Created on: 2015/12/06
 *      Author: ZenoKuo
 *  Latest update: 2016/2/26
 *      Reviser: ZenoKuo
 *  Comments:
********************************************************************************/

#define _WRAPPER_AF_VER 0.8070

/*
 * API name: al3AWrapper_dispatch_af_stats
 * This API used for patching HW3A stats from ISP(Altek) for AF libs(Altek), after patching completed, AF ctrl should prepare patched
 * stats to AF libs
 * param isp_meta_data[In]: patched data after calling al3AWrapper_DispatchHW3AStats, used for patch AF stats for AF lib
 * param alaf_stats[Out]: result patched AF stats
 * return: error code
 */
uint32 al3awrapper_dispatchhw3a_afstats(void *isp_meta_data,void *alaf_stats);

//#define NEW_AF_ALGORITHM
#ifdef NEW_AF_ALGORITHM
/// ALTEK_MODIFIED >>>
uint32 al3awrapper_dispatchhw3a_yhist(void *isp_yhist,void *alaf_yhist);
/// ALTEK_MODIFIED <<<
#endif /* NEW_AF_ALGORITHM */

/*
 * API name: al3awrapperaf_translatefocusmodetoaptype
 * This API used for translating AF lib focus mode define to framework
 * param focus_mode[In] :   Focus mode define of AF lib (Altek)
 * return: Focus mode define for AP framework
 */
uint32 al3awrapperaf_translatefocusmodetoaptype(uint32 focus_mode);

/*
 * API name: al3awrapperaf_translatefocusmodetoaflibtype
 * This API used for translating framework focus mode to AF lib define
 * focus_mode[In] :   Focus mode define of AP framework define
 * return: Focus mode define for AF lib (Altek)
 */
uint32 al3awrapperaf_translatefocusmodetoaflibtype(uint32 focus_mode);

/*
 * API name: al3awrapperaf_translatecalibdattoaflibtype
 * This API used for translating EEPROM data to AF lib define
 * eeprom_addr[In] :   EEPROM data address
 * AF_Calib_Dat[Out] :   Altek data format
 * return: Error code
 */
uint32 al3awrapperaf_translatecalibdattoaflibtype(void *eeprom_addr,struct allib_af_input_init_info_t *af_Init_data);

/*
 * API name: al3awrapperaf_translateroitoaflibtype
 * This API used for translating ROI info to AF lib define
 * frame_id[In] :   Current frame id
 * roi_info[Out] :   Altek data format
 * return: Error code
 */
uint32 al3awrapperaf_translateroitoaflibtype(unsigned int frame_id,struct allib_af_input_roi_info_t *roi_info);

/*
 * API name: al3awrapperaf_translateaeinfotoaflibtype
 * This API used for translating AE info to AF lib define
 * ae_report_update_t[In] :   Iput AE data
 * struct allib_af_input_aec_info_t[Out] :   Altek data format
 * return: Error code
 */
uint32 al3awrapperaf_translateaeinfotoaflibtype(struct ae_report_update_t *ae_report,struct allib_af_input_aec_info_t *ae_info);

/*
 * API name: al3awrapperaf_updateispconfig_af
 * This API is used for query ISP config before calling al3awrapperaf_updateispconfig_af
 * param lib_config[in]: AF stats config info from AF lib
 * param isp_config[out]: input buffer, API would manage parameter and return via this pointer
 * return: error code
 */
uint32 al3awrapperaf_updateispconfig_af(struct allib_af_out_stats_config_t *lib_config,struct alhw3a_af_cfginfo_t *isp_config);

#ifdef NEW_AF_ALGORITHM
/// ALTEK_MODIFIED >>>
/*
 * API name: al3awrapperaf_updateispconfig_yhis
 * This API is used for translate ISP config
 * param lib_config[in]: AF stats config info from AF lib
 * param isp_config[out]: input buffer, API would manage parameter and return via this pointer
 * return: error code
 */
uint32 al3awrapperaf_updateispconfig_yhis(struct allib_af_out_stats_config_t *lib_config, struct alhw3a_yhist_cfginfo_t *isp_config);
/// ALTEK_MODIFIED <<<
#endif /* NEW_AF_ALGORITHM */

/*
 * API name: al3awrapperaf_GetDefaultCfg
 * This API is used for query default ISP config before calling al3awrapperaf_updateispconfig_af
 * param isp_config[out]: input buffer, API would manage parameter and return via this pointer
 * return: error code
 */
uint32 al3awrapperaf_getdefaultcfg(struct alhw3a_af_cfginfo_t *isp_config);

/*
 * API name: al3awrapper_updateafreport
 * This API is used for translate AF status  to generate AF report for AP
 * param lib_status[in]: af status from AF update
 * param report[out]: AF report for AP
 * return: error code
 */
uint32 al3awrapper_updateafreport(struct allib_af_out_status_t *lib_status,struct af_report_update_t *report);

/*
 * API name: al3awrapperaf_get_version
 * This API would return labeled version of wrapper
 * fWrapVersion[out], return current wapper version
 * return: error code
 */
uint32 al3awrapperaf_get_version(float *wrap_version);