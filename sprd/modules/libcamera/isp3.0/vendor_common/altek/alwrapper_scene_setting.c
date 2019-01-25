/******************************************************************************
 * alwrapper_scene_setting.c
 *
 *  Created on: 2017/01/16
 *      Author: HanTseng
 *  Latest update:
 *      Reviser:
 *  Comments:
 *       This c file is mainly used for AP framework to:
 *       1. dispatch scene setting
 ******************************************************************************/

#include <string.h>
#include "mtype.h"

#include "alwrapper_scene_setting.h"
#include "alwrapper_scene_errcode.h"


/*
 * API name: al3awrapper_scene_get_setting
 * This API is used to obtain scene setting info from scene setting file
 * param scene[in]: scene mode
 * param setting_file[in]: file address pointer to scene setting file w/ IDX
 * param scene_info[out]: output scene setting info, including scene mode and addr
 * return: error code
 */
uint32 al3awrapper_scene_get_setting(enum scene_setting_list_t scene, void *setting_file, struct scene_setting_info *scene_info)
{
	uint32 ret = ERR_WRP_SCENE_SUCCESS;
	short sceneIdx = (short)scene;
	char *addr;
	struct alwrapper_scene_setting_header *header;
	struct alwrapper_scene_setting *setting;

	if (setting_file == NULL || scene_info == NULL)
		return ERR_WRP_SCENE_INVALID_ADDR;

	header = (struct alwrapper_scene_setting_header*)setting_file;
	if (header->ud_structure_size != sizeof(struct alwrapper_scene_setting_header) ||
	    header->ud_scene_number + 1 != Scene_Max)
		return ERR_WRP_SCENE_INVALID_FORMAT;

	addr = (char*)setting_file;
	scene_info->uw_mode = (uint32)scene;
	if (Auto == scene)
		scene_info->puc_addr = NULL;
	else
		scene_info->puc_addr = (void*)(addr + header->ud_scene_offset + header->ud_scene_size * (sceneIdx - 1));
	return ret;
}


