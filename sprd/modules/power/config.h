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

#ifndef INCLUDE_SPRD_POWER_CONFIG_H
#define INCLUDE_SPRD_POWER_CONFIG_H

#include <linux/time.h>

#include "common.h"

// Define the path of config file
#define PATH_SCENE_CONFIG                 "/vendor/etc/power_scene_config.xml"
#define PATH_SCENE_ID_DEFINE              "/vendor/etc/power_scene_id_define.txt"
#define PATH_RESOURCE_FILE_INFO           "/vendor/etc/power_resource_file_info.xml"

#define MODE_PATH                         "/power/mode"
#define RESOURCE_FILE_PATH                "/resources/file"
#define RESOURCE_SUBSYS_PATH              "/resources/subsys"
#define NUM_RESOURCE_MAX                  10

#define LEN_MODE_NAME_MAX                 30
#define NUM_MODE_MAX                      8
#define LEN_SCENE_NAME_MAX                30
#define NUM_SCENE_MAX                     50

/**
 * struct scene - record the configuration of a scene
 * @name: the scene name
 * @count: the number of element in sets array
 * @sets: the configuration of the scene
 */
struct scene {
    char name[LEN_SCENE_NAME_MAX];
    int count;
    struct set sets[NUM_FILE_MAX];
};

/**
 * struct mode - record all configuration of a mode
 * @name: the mode name
 * @count: the number of scene
 * @scenes: all scene configuration of the mode
 */
struct mode {
    char name[LEN_MODE_NAME_MAX];
    int count;
    struct scene scenes[NUM_SCENE_MAX];
};

/**
 * struct power - record all info for all modes
 * @count: the number of mode
 * @modes: the configuration of all supported modes
 */
struct power {
    int count;
    struct mode modes[NUM_MODE_MAX];
};

extern struct power power;

int read_scene_config(void);

// Store id info from power_scene_id_define.txt
struct scene_id {
   unsigned int id;
   unsigned int subtype;
   char scene_name[LEN_SCENE_NAME_MAX];
};

int read_scene_id_define_file();
char *scene_id_to_string(int scene_id, int subtype);

int read_resource_config(void);
#endif
