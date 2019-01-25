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

#ifndef INCLUDE_SPRD_POWER_COMMON_H
#define INCLUDE_SPRD_POWER_COMMON_H

#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/time.h>
#include <stdbool.h>
#include <cutils/properties.h>
#include <cutils/compiler.h>
#include <math.h>
#include <sys/prctl.h>
#include <sys/resource.h>

#define LOG_TAG "PowerHAL"
#include <utils/Log.h>

#define LEN_PATH_MAX                      60
#define LEN_FILE_MAX                      30
#define LEN_VALUE_MAX                     60

#define NUM_FILE_MAX                      20
#define NUM_PATH_MAX                      10
#define NUM_SUBSYS_CONFIG_MAX             10
#define NUM_SUBSYS_MAX                    10
#define LEN_SUBSYS_NAME_MAX               20
#define LEN_CONFIG_NAME_MAX               20

#define NUM_REQUST_FOR_FILE_MAX           20

extern int power_mode;
extern struct mode *current;

struct file;

typedef int (*comp_func_ptr_t)(const void *, const void *);
typedef int (*clear_func_ptr_t)(const char *path, struct file *file);
typedef int (*set_func_ptr_t)(int enable, int duration,const char *path, struct file *file);

/**
 * struct req_item - record a request info
 * @value: the value to set
 * @times: the times of request the value
 * @duration_end_time: the duration time of one request
 */
struct req_item {
    char value[LEN_VALUE_MAX];
    int times;
    struct timespec duration_end_time;
};

/**
 * struct request_stat - record all requests for a file
 * @count: the number of request item for current file
 * @current: the value currently in force
 * @items: record every request item
 */
struct request_stat {
    int count;
    struct req_item current;
    struct req_item items[NUM_REQUST_FOR_FILE_MAX];
};

/**
 * struct file - the all info for a file
 * @name: the file name
 * @value: the def_value or target value
 * @no_has_defalut: if the file has default value, 0 if hava, default 0
 * @timer_id: the timer id
 * @comp: the comppare function used by request sort
 * @clear: clear all requests for current file
 * @set: called when boost or deboost
 * @stat: record all resources for the file
 */
struct file {
    char name[LEN_FILE_MAX];
    struct {
        char def_value[LEN_VALUE_MAX];
        char target_value[LEN_VALUE_MAX];
    } value;
    int no_has_def;
    timer_t timer_id;
    comp_func_ptr_t comp;
    clear_func_ptr_t clear;
    set_func_ptr_t set;
    struct request_stat stat;
};

/**
 * struct path_file - record all files under a path
 * @path: the directory
 * @count: the number of element in files array
 * @files: record all file info under the directory
 */
struct path_file {
    char path[LEN_PATH_MAX];
    int count;
    struct file files[NUM_FILE_MAX];
};

// Some data type for subsys node
/**
 * struct subsys_inode - description a inode of a subsystem
 * @path: the directory
 * @file: file name
 * @value: the default value of file
 */
struct subsys_inode {
    char path[LEN_PATH_MAX];
    char file[LEN_FILE_MAX];
    int no_has_def;
    struct {
        char def_value[LEN_VALUE_MAX];
        char target_value[LEN_VALUE_MAX];
    } value;
};

/**
 * struct set - record the configuration for a file
 * @path: the path of file
 * @file: the file name
 * @value: the value to set
 */
struct set {
    char path[LEN_PATH_MAX];
    char file[LEN_FILE_MAX];
    char value[LEN_VALUE_MAX];
};

/**
 * struct config - a configuration of subsystem
 * @name: config name
 * @priority: priority of the configuration
 * @count: the number of element in sets array
 * @sets: detail configuration info
 */
struct config {
    char name[LEN_CONFIG_NAME_MAX];
    int priority;
    int count;
    struct set sets[NUM_FILE_MAX];
};

/**
 * struct subsys - descript a subsystem
 * @name: the subsystem name
 * @inode_count: the number of element in inodes array
 * @inodes: files that the subsystem include
 * @config_count: the number of element in configs array
 * @configs: all configurations that the subsystem supported
 */
struct subsys {
    char name[LEN_SUBSYS_NAME_MAX];
    int inode_count;
    struct subsys_inode inodes[NUM_FILE_MAX];
    int config_count;
    struct config configs[NUM_SUBSYS_CONFIG_MAX];
};

/**
 * struct resources - record all resource info
 * @count: the number of the resource directory
 * @path_files: record all supported resource files
 */
struct resources {
    int count;
    struct path_file path_files[NUM_PATH_MAX];
    int subsys_count;
    struct subsys subsystems[NUM_SUBSYS_MAX];
};

extern struct resources resources;

// Store resource to boost
struct boost_entry {
    char *path;
    struct file *file;
};
//###############################################
#define LEN_FUNCTION_NAME_MAX               30
#define FUNC_NAME(name)                     #name

union f {
    comp_func_ptr_t comp;
    clear_func_ptr_t clear;
    set_func_ptr_t set;
};

/**
 * struct func - function name mapping
 */
struct func {
    char name[LEN_FUNCTION_NAME_MAX];
    union f f;
};

int common_comp_ascend_order(const void *a, const void *b);
int common_comp_descend_order(const void *a, const void *b);
int common_clear(const char *path, struct file *file);
int common_set(int enable, int duration, const char *path, struct file *file);
int common_subsys_set(int enable, int duration, const char *path, struct file *file);
int common_subsys_clear(const char *path, struct file *file);
int common_subsys_comp(const void *a, const void *b);

int config_read();

struct file_node {
    char *path;
    char *file;
    char *comp;
    char *clear;
    char *set;
    char *def_value;
    char *no_has_def;
};
int init_file(struct file_node *file_node);
void start_thread_for_timing_request(void *args);

int boost(int scene_id, int subtype, int enable, int data);
int update_mode(int mode, int enable);
void sort_request_for_file(int enable, int duration, struct file *file);
void clear_requests_for_all_file();
#endif
