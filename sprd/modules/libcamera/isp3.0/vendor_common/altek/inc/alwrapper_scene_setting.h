/******************************************************************************
*  File name: alwrapper_scene_setting.h
*  Create Date:
*
*  Comment:
*  Scene setting structure define
 ******************************************************************************/

#ifndef _AL_3AWRAPPER_SCENE_H_
#define _AL_3AWRAPPER_SCENE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define _ALWRAPPER_SCENE_SETTING_VERSION 0.1010

enum scene_setting_list_t {
	Auto = 0,
	Night,
	Sports,
	Portrait,
	Landscape,

	Self_Portrait,
	Group_Portrait,
	Sky_Portrait,
	Backlight_Portrait,
	Night_Portrait,
	White,
	Nature_Green,
	Blue_Sky,
	Sunset,
	Backlight,
	Night_Landscape,
	Action,
	Candle,
	Text,
	Flower,
	Close_Up,

	Scene_Reserved_1,
	Scene_Reserved_2,
	Scene_Reserved_3,
	Scene_Reserved_4,
	Scene_Reserved_5,
	Scene_Reserved_6,
	Scene_Reserved_7,
	Scene_Reserved_8,

	Scene_Max,
};

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct alwrapper_scene_setting_header {
	uint32 ud_structure_version;
	float f_tuning_version;
	uint32 ud_structure_size;
	uint32 ud_scene_offset;
	uint32 ud_scene_number;
	uint32 ud_scene_size;
	uint32 ud_reserved[4];
};
#pragma pack(pop)  /* restore old alignment setting from stack */


#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct alwrapper_scene_setting {
	int32 d_ae_p_curve;
	int32 d_ae_ev_comp;
	int32 d_awb_mode;
	int32 d_af_foucs;
	int32 d_irp_sharpness;
	int32 d_irp_contrast;
	int32 d_irp_saturation;
	int32 d_reserved[9];

	// customer reserved
	int32 d_customer_1;
	int32 d_customer_2;
	int32 d_customer_3;
	int32 d_customer_4;
	int32 d_customer_5;
	int32 d_customer_6;
	int32 d_customer_7;
	int32 d_customer_8;
};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct scene_setting_info {
	uint32 uw_mode;
	void* puc_addr;
};
#pragma pack(pop)  /* restore old alignment setting from stack */


/*
 * API name: al3awrapper_scene_get_setting
 * This API is used to obtain scene setting info from scene setting file
 * param scene[in]: scene mode
 * param setting_file[in]: file address pointer to scene setting file w/ IDX
 * param scene_info[out]: output scene setting info, including scene mode and addr
 * return: error code
 */
uint32 al3awrapper_scene_get_setting(enum scene_setting_list_t scene, void *setting_file, struct scene_setting_info *scene_info);


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* _AL_3AWRAPPER_SCENE_H_ */


