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

#include "sprd_power.h"
#include "common.h"
#include "config.h"
#include "devfreq.h"
#include "pm_qos.h"
#include "cpufreq.h"
#include "utils.h"

struct resources resources;

struct mode *default_mode = NULL;
struct mode *current_mode = NULL;
int power_mode = POWER_HINT_VENDOR_MODE_NORMAL;

struct func compare_funcs[] = {
    {.name = FUNC_NAME(common_comp_ascend_order), .f = {.comp = &common_comp_ascend_order}},
    {.name = FUNC_NAME(common_comp_descend_order), .f = {.comp = &common_comp_descend_order}},
    {.name = FUNC_NAME(common_subsys_comp), .f = {.comp = &common_subsys_comp}},
    {.name = "",},
};

struct func clear_funcs[] = {
    {.name = FUNC_NAME(common_clear), .f = {.clear = &common_clear}},
    {.name = FUNC_NAME(common_subsys_clear), .f = {.clear = &common_subsys_clear}},
    {.name = FUNC_NAME(devfreq_ddr_clear), .f = {.clear = &devfreq_ddr_clear}},
    {.name = FUNC_NAME(pm_qos_cpu_clear), .f = {.clear = &pm_qos_cpu_clear}},
    {.name = "",},
};

struct func set_funcs[] = {
    {.name = FUNC_NAME(common_set), .f = {.set = &common_set}},
    {.name = FUNC_NAME(common_subsys_set), .f = {.set = &common_subsys_set}},
    {.name = FUNC_NAME(devfreq_ddr_set), .f = {.set = &devfreq_ddr_set}},
    {.name = FUNC_NAME(pm_qos_cpu_set), .f = {.set = &pm_qos_cpu_set}},
    {.name = "",},
};

static void *find_function_by_name(const struct func *funcs, const char *name)
{
    if (CC_UNLIKELY(funcs == NULL || name == NULL))
        return NULL;

    if (DEBUG_V) ENTER("%s", name);
    for (int i = 0; strlen(funcs[i].name) > 0; i++) {
        if (strcmp(funcs[i].name, name) == 0)
            return (void*)(&(funcs[i].f));
    }

    ALOGD("Don't support func %s", name);
    return NULL;
}

/**
 * init_file - init a resource according to resource config file
 */
int init_file(struct file_node *file_node)
{
    int i = 0;
    struct path_file *path_file = NULL;
    struct file *file = NULL;
    union f *func = NULL;

    if (CC_UNLIKELY(file_node == NULL)) return 0;

    if (file_node->path[strlen(file_node->path) - 1] == '/') {
        file_node->path[strlen(file_node->path) - 1] = '\0';
    }

    for (; i < resources.count; i++) {
        if (strcmp(resources.path_files[i].path, file_node->path) == 0)
            break;
    }
    if (i < resources.count) {
        path_file = &(resources.path_files[i]);
    } else if (resources.count < NUM_PATH_MAX) {
        path_file = &(resources.path_files[resources.count++]);
        strncpy(path_file->path, file_node->path, LEN_PATH_MAX);
    } else {
        ALOGE("!!!path_files[] is full");
        return 0;
    }

    if (path_file->count >= NUM_FILE_MAX) {
        ALOGE("!!!files[] is full");
        return 0;
    }
    file = &(path_file->files[path_file->count++]);
    strncpy(file->name, file_node->file, LEN_FILE_MAX);

    if (file_node->clear == NULL) {
        file->clear = NULL;
    } else {
        func = (union f*)find_function_by_name(clear_funcs, file_node->clear);
        file->clear = func->clear;
    }

    func = (union f*)find_function_by_name(compare_funcs, file_node->comp);
    file->comp = func->comp;
    func = (union f*)find_function_by_name(set_funcs, file_node->set);
    file->set = func->set;

    if (file_node->def_value != NULL) {
        strncpy(file->value.def_value, file_node->def_value, LEN_VALUE_MAX);
    }

    if (file_node->no_has_def != NULL) {
        file->no_has_def = atoi(file_node->no_has_def);
    } else {
        file->no_has_def = 0;
    }

    return 1;
}

// Read all config file
int config_read()
{
    memset(&resources, 0, sizeof(resources));

    ALOGD("size:%u + %u", sizeof(power), sizeof(resources));
    if (read_resource_config() == 0) {
        ALOGE("!!!Parse resource file failed");
        return 0;
    }

    if (read_scene_id_define_file() == 0) {
        ALOGE("!!!Parse scene id define file failed");
        return 0;
    }

    if (read_scene_config() == 0) {
        ALOGE("!!!Parse scene config file failed");
        return 0;
    }

    for (int i = 0; i < power.count; i++) {
        if (strncmp(power.modes[i].name, "normal", strlen("normal")) ==  0) {
            default_mode = &(power.modes[i]);
            current_mode = default_mode;
            power_mode = POWER_HINT_VENDOR_MODE_NORMAL;
            break;
        }
    }

    if (current_mode == NULL) {
        ALOGE("!!!Don't define normal mode");
        return 0;
    }

    return 1;
}

// Handle request timeout
static void *signal_handler(void *args)
{
    struct sprd_power_module *pm = (struct sprd_power_module *)args;
    sigset_t sigset, old_sigset;
    struct file *file = NULL;
    bool found = false;
    siginfo_t info;
    int i = 0;

    if (CC_UNLIKELY(pm == NULL)) return NULL;

    prctl(PR_SET_NAME, "power_hint");
    setpriority(PRIO_PROCESS, 0, getpriority(PRIO_PROCESS, 0) + 4);

    sigemptyset(&sigset);
	sigaddset(&sigset, SIGALRM);
    if (pthread_sigmask(SIG_BLOCK, &sigset, &old_sigset) != 0) {
        ALOGE("%s: set signal mask fail:(%s)", __func__, strerror(errno));
        return NULL;
    }

    for (int i = 0; i < resources.count; i++) {
        for (int j = 0; j < resources.path_files[i].count; j++) {
            file = &(resources.path_files[i].files[j]);
            sprd_timer_create(SIGALRM, &(file->timer_id), gettid());
        }
    }

    while (1) {
        if(sigwaitinfo(&sigset, &info) > 0) {
            ALOGD_IF(DEBUG_V, "%s: catch signo:%d, value=0x%lx", __func__, info.si_signo, *((long *)info.si_value.sival_ptr));

            found = false;
            for (int i = 0; i < resources.count; i++) {
                for (int j = 0; j < resources.path_files[i].count; j++) {
                    file = &(resources.path_files[i].files[j]);
                    if (info.si_value.sival_ptr == &(file->timer_id)) {
                        pthread_mutex_lock(&pm->lock);
                        ALOGD("##Timing deboost");
                        memset(file->value.target_value, 0, LEN_VALUE_MAX);
                        file->set(0, 0, resources.path_files[i].path, file);
                        pthread_mutex_unlock(&pm->lock);
                        found = true;
                        break;
                    }
                }

                if (found) break;
            }
        }
    }

    return NULL;
}

/**
 * start_thread_for_timing_request - create thread hanling the timeout signal
 */
void start_thread_for_timing_request(void *args)
{
    pthread_t tid;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&tid, &attr, &signal_handler, args) != 0) {
        pthread_attr_destroy(&attr);
        ALOGE("%s: Thread create fail!!!!!!!!", __func__);
        return;
    }
    pthread_attr_destroy(&attr);
}

// Get default value of files included by subsystem
static int get_default_value_for_subsys()
{
    char buf[128] = {'\0'};
    struct file *file = NULL;
    bool log = false;
    struct subsys *subsys = NULL;
    struct subsys_inode *inode = NULL;

    for (int i = 0; i < resources.subsys_count; i++) {
        subsys = &(resources.subsystems[i]);
        for (int j = 0; j < subsys->inode_count; j++) {
            inode = &(subsys->inodes[j]);
            if (inode->no_has_def == 0) {
                if (strlen(inode->value.def_value) == 0) {
                    snprintf(buf, sizeof(buf), "%s/%s", inode->path, inode->file);
                    if ((access(buf, F_OK) != 0) || get_string_default_value(buf, inode->value.def_value, LEN_VALUE_MAX) == 0) {
                        ALOGD("!!!Get %s default value failed", buf);
                        return 0;
                    } else {
                        log = true;
                    }
                }
                strcpy(inode->value.target_value, inode->value.def_value);
            } else {
                memset(inode->value.target_value, 0, sizeof(LEN_VALUE_MAX));
            }
        }
    }

    if (log) {
        // Only print one time
        ALOGD(">>>>>>>>>>>>>>>>>>>>");
        for (int i = 0; i < resources.subsys_count; i++) {
            subsys = &(resources.subsystems[i]);
            ALOGD("Subsys %s inode default value:", subsys->name);
            for (int j = 0; j < subsys->inode_count; j++) {
                inode = &(subsys->inodes[j]);
                ALOGD("%s/%s: %s", inode->path, inode->file, inode->value.def_value);
            }
        }
        ALOGD("<<<<<<<<<<<<<<<<<<<<");
    }

    return 1;
}

static int get_or_set_default_value_to_target()
{
    char buf[128] = {'\0'};
    struct file *file = NULL;
    bool log = false;

    for (int i = 0; i < resources.count; i++) {
        for (int j = 0; j < resources.path_files[i].count; j++) {
            file = &(resources.path_files[i].files[j]);
            snprintf(buf, sizeof(buf), "%s/%s", resources.path_files[i].path, file->name);
            if (file->no_has_def == 0) {
                if (strlen(file->value.def_value) == 0) {
                    if (strncmp(resources.path_files[i].path, "subsys", 6) != 0) {
                        if((access(buf, F_OK) != 0) || (get_string_default_value(buf, file->value.def_value, LEN_VALUE_MAX) == 0)) {
                            ALOGE("!!!Get %s default value failed", buf);
                            return 0;
                        } else {
                            log = true;
                        }
                    } else {
                        ALOGE("!!!Must specific the default value for subsys %s file node", file->name);
                    }
                }
                strcpy(file->value.target_value, file->value.def_value);
            } else {
                memset(file->value.target_value, 0, sizeof(LEN_VALUE_MAX));
            }
        }
    }

    if (log) {
        // Only print one time
        ALOGD(">>>>>>>>>>>>>>>>>>>>");
        ALOGD("The resource default value:");
        for (int i = 0; i < resources.count; i++) {
            ALOGD("%s:", resources.path_files[i].path);
            for (int j = 0; j < resources.path_files[i].count; j++) {
                file = &(resources.path_files[i].files[j]);
                ALOGD("%s: %s", file->name, file->value.def_value);
            }
        }
        ALOGD("<<<<<<<<<<<<<<<<<<<<");
    }

    return 1;
}

static int _boost(const struct scene *scene, int enable, int data)
{
    struct path_file *path_file = NULL;
    struct file *file = NULL;
    struct set *set = NULL;
    bool found = false;
#ifdef BOOST_SPECIFICED
    struct boost_entry boost_entrys[NUM_RESOURCE_MAX];
    int count = 0;

    memset(boost_entrys, 0, sizeof(boost_entrys));
#endif

    if (get_or_set_default_value_to_target() == 0)
        return 0;

    if (get_default_value_for_subsys() == 0)
        return 0;

    for (int i = 0; i < scene->count; i++) {
        found = false;
        set = &(scene->sets[i]);
        for (int j = 0; j < resources.count; j++) {
            if (strcmp(set->path, resources.path_files[j].path) == 0) {
                path_file = &(resources.path_files[j]);
                for (int k = 0; k < path_file->count; k++) {
                    if (strcmp(set->file, path_file->files[k].name) == 0) {
                        file = &(path_file->files[k]);
                        strncpy(file->value.target_value, set->value, LEN_VALUE_MAX);
                        found = true;
#ifdef BOOST_SPECIFICED
                        boost_entrys[count].path = path_file->path;
                        boost_entrys[count++].file = file;
#endif
                        break;
                    }
                }
            }
            if (found)
                break;
        }

        if (!found) {
            ALOGE("!!!Undefined resource %s/%s", set->path, set->file);
            return 0;
        }
    }

    // Maybe the scene don't hava set node
    if (!found) return 0;

#ifdef BOOST_SPECIFICED
    for (int i = 0; i < count; i++) {
        file = boost_entrys[i].file;
        file->set(enable, data, boost_entrys[i].path, file);
    }
#else
    for (int i = 0; i < resources.count; i++) {
        for (int j = 0; j < resources.path_files[i].count; j++) {
            file = &(resources.path_files[i].files[j]);
            if (file->set != NULL && (strlen(file->value.target_value) != 0))
                file->set(enable, data, resources.path_files[i].path, file);
        }
    }
#endif

    return 1;
}

/**
 * boost - boost by hint id and subtype
 * @hint_id: power hint id
 * @subtype: id subtype
 * @enable: if 0 disable boost, else enable
 * @data: include data from framework
 * return: 1 if successs, else 0
 *
 * calling some set functions according to scene config file
 */
int boost(int scene_id, int subtype, int enable, int data)
{
    char *scene_name = NULL;
    struct scene *scene = NULL;
    int index = -1;

    if (CC_UNLIKELY(current_mode == NULL)) {
        ALOGE("mode is null");
        return 0;
    }

    scene_name = scene_id_to_string(scene_id, subtype);
    if (scene_name == NULL) {
        ALOGE("scene_id_to_string(%d, %d) fail", scene_id, subtype);
        return 0;
    }

    for (int i = 0; i < current_mode->count; i++) {
       if (strcmp(scene_name, current_mode->scenes[i].name) == 0) {
           scene = &(current_mode->scenes[i]);
           break;
       }
    }
    if (scene == NULL) {
        ALOGI("Don't support %s in %s mode", scene_name, current_mode->name);
        return 0;
    }

    ALOGD_IF(DEBUG, "###%s %s scene bgn###", enable?"Enter": "Exit", scene_name);
    _boost(scene, enable, data);
    ALOGD_IF(DEBUG, "###%s %s scene end###", enable?"Enter": "Exit", scene_name);
    return 1;
}

void clear_requests_for_all_file()
{
    struct file *file = NULL;

    for (int i = 0; i < resources.count; i++) {
        for (int j = 0; j < resources.path_files[i].count; j++) {
            file = &(resources.path_files[i].files[j]);
            if (file->clear != NULL)
                file->clear(resources.path_files[i].path, file);
        }
    }
}

int update_mode(int mode, int enable)
{
    char *mode_name = NULL;

    mode_name = scene_id_to_string(mode, 0);
    if (mode_name == NULL) {
        ALOGE("scene_id_to_string(0x%08x, 0) fail", mode);
        return 0;
    }

    if (enable) {
        for (int i = 0; i < power.count; i++) {
            if (strcmp(power.modes[i].name, mode_name) == 0) {
                current_mode = &(power.modes[i]);
                power_mode = mode;
                break;
            }
        }
    } else {
        current_mode = default_mode;
        power_mode = POWER_HINT_VENDOR_MODE_NORMAL;
    }

    clear_requests_for_all_file();

    return 1;
}

// The value bigger, the priority higher
int common_comp_ascend_order(const void *a, const void *b)
{
    struct req_item  *aa = (struct req_item *)a;
    struct req_item  *bb = (struct req_item *)b;

    return (strtol(aa->value, NULL, 10) - strtol(bb->value, NULL, 10));
}

// The value bigger, the priority lower
int common_comp_descend_order(const void *a, const void *b)
{
    struct req_item  *aa = (struct req_item *)a;
    struct req_item  *bb = (struct req_item *)b;

    return (strtol(bb->value, NULL, 10) - strtol(aa->value, NULL, 10));
}

/**
 * common_clear - clear all request for the file and recovery default value
 */
int common_clear(const char *path, struct file *file)
{
    if (path == NULL || file == NULL) return 0;

    ENTER();
    sprd_timer_settime(file->timer_id, 0);
    memset(&(file->stat), 0, sizeof(struct request_stat));

    if (strlen(file->value.def_value) > 0) {
        char buf[128] = {'\0'};

        snprintf(buf, sizeof(buf), "%s/%s", path, file->name);
        if (access(buf, F_OK) == 0) {
            sprd_write(buf, file->value.def_value);
            ALOGD_IF(DEBUG, "Set %s: %s", buf, file->value.def_value);
        }
    }

    return 1;
}

int common_set(int enable, int duration, const char *path, struct file *file)
{
    char buf[128] = {'\0'};
    struct timespec now;
    struct req_item *req_item = NULL;
    long long time_value;

    if (CC_UNLIKELY(path == NULL || file == NULL))
        return 0;

    ENTER("enable:%d, duration: %d, %s/%s: %s", enable, duration, path, file->name
        , file->value.target_value);

    memset(&now, 0, sizeof(now));
    snprintf(buf, sizeof(buf), "%s/%s", path, file->name);

    sort_request_for_file(enable, duration, file);
    if (DEBUG_V) {
        char time[20] = {'\0'};
        ALOGD(">>>>>>>>>>>>>>>>>>>>");
        ALOGD("%s:", buf);
        for (int i = 0; i < file->stat.count; i++) {
            sprd_strftime(time, 20, file->stat.items[i].duration_end_time);
            ALOGD("  value:%s, times:%d, end_time: %s", file->stat.items[i].value
                , file->stat.items[i].times, time);
        }
        ALOGD("<<<<<<<<<<<<<<<<<<<<");
    }

    if (file->stat.count <= 0) {
        common_clear(path, file);
        return 1;
    }

    // Set timer if the highest priority request has duration time
    clock_gettime(CLOCK_MONOTONIC, &now);
    req_item = &(file->stat.items[file->stat.count - 1]);
    time_value = calc_timespan_ms(now, req_item->duration_end_time);
    if (time_value > 0) {
        sprd_timer_settime(file->timer_id, time_value);
    }

    // If the highest priority request is the same with
    // current, don't need send the request to driver
    if(strlen(file->stat.current.value) != 0
        && file->comp((void *)req_item, (void *)(&(file->stat.current))) == 0)
        return 1;

    // Update current request
    memcpy(&(file->stat.current), req_item, sizeof(struct req_item));

    if (access(buf, F_OK) == 0) {
        ALOGD("Set %s: %s", buf, req_item->value);
        sprd_write(buf, req_item->value);
    }

    return 1;
}

static void *find_subsys_by_name(char *name)
{
    if (name == NULL) return NULL;

    for (int i = 0; i < resources.subsys_count; i++) {
        if (strcmp(name, resources.subsystems[i].name) == 0)
            return  &(resources.subsystems[i]);
    }

    return NULL;
}

static int add_priority_to_target_value(struct file *file)
{
    struct subsys *subsys = NULL;

    if (file == NULL) return 0;

    subsys = find_subsys_by_name(file->name);
    if (subsys == NULL) {
        ALOGE("Don't support subsys %s", file->name);
        return 0;
    }

    for (int i = 0; i < subsys->config_count; i++) {
        if (strcmp(file->value.target_value, subsys->configs[i].name) == 0) {
            snprintf(file->value.target_value + strlen(file->value.target_value), LEN_VALUE_MAX
                ,":%d", subsys->configs[i].priority);
            return 1;
        }
    }

    return 0;
}

static int common_subsys_set_current_config(struct file *file)
{
    struct subsys *subsys = NULL;
    struct config *config = NULL;
    struct subsys_inode *inode = NULL;
    char *conf_name = NULL;
    int conf_name_len = 0;
    char *ptr = NULL;
    char buf[128] = {'\0'};

    ENTER();
    if (CC_UNLIKELY(file == NULL)) return 0;

    subsys = find_subsys_by_name(file->name);
    if (subsys == NULL) {
        ALOGE("Don't support subsys %s", file->name);
        return 0;
    }

    conf_name = file->stat.current.value;
    ptr = strchr(file->stat.current.value, ':');
    conf_name_len = ptr - conf_name;

    for (int i = 0; i < subsys->config_count; i++) {
        if (strncmp(conf_name, subsys->configs[i].name, conf_name_len) == 0) {
            config = &(subsys->configs[i]);
            break;
        }
    }

    if (config == NULL) {
        ALOGE("Don't find config: %s in %s subsys", conf_name, subsys->name);
        return 0;
    }

    for (int i = 0; i < config->count; i++) {
        for (int j = 0; j < subsys->inode_count; j++) {
            if (strcmp(config->sets[i].path, subsys->inodes[j].path) == 0
                && strcmp(config->sets[i].file, subsys->inodes[j].file) == 0 ) {
                strcpy(subsys->inodes[j].value.target_value, config->sets[i].value);
                break;
            }
        }
    }

    for (int i = 0; i < subsys->inode_count; i++) {
        inode = &(subsys->inodes[i]);
        if (strlen(inode->value.target_value) != 0) {
            snprintf(buf, sizeof(buf), "%s/%s", inode->path, inode->file);
            if (access(buf, F_OK) == 0) {
                sprd_write(buf, inode->value.target_value);
                ALOGD_IF(DEBUG, "Set %s: %s", buf, inode->value.target_value);
            }
        }
    }

    return 1;
}

int common_subsys_set(int enable, int duration, const char *path, struct file *file)
{
    struct timespec now;
    struct req_item *req_item = NULL;
    long long time_value;

    if (CC_UNLIKELY(path == NULL || file == NULL))
        return 0;

    if (strlen(file->value.target_value) != 0 && add_priority_to_target_value(file) == 0) {
        ALOGE("Add priority for %s failed", file->value.target_value);
        return 0;
    }

    ENTER("enable:%d, duration: %d, %s:%s: %s", enable, duration, path, file->name
        , file->value.target_value);
    memset(&now, 0, sizeof(now));

    sort_request_for_file(enable, duration, file);
    if (DEBUG_V) {
        char time[20] = {'\0'};
        ALOGD(">>>>>>>>>>>>>>>>>>>>");
        ALOGD("%s:", file->name);
        for (int i = 0; i < file->stat.count; i++) {
            sprd_strftime(time, sizeof(time), file->stat.items[i].duration_end_time);
            ALOGD("  value:%s, times:%d, end_time: %s", file->stat.items[i].value
                , file->stat.items[i].times, time);
        }
        ALOGD("<<<<<<<<<<<<<<<<<<<<");
    }

    if (file->stat.count <= 0) {
        common_subsys_clear(path, file);
        return 1;
    }

    // Set timer if the highest priority request has duration time
    clock_gettime(CLOCK_MONOTONIC, &now);
    req_item = &(file->stat.items[file->stat.count - 1]);
    time_value = calc_timespan_ms(now, req_item->duration_end_time);
    if (time_value > 0) {
        sprd_timer_settime(file->timer_id, time_value);
    }

    // If the highest priority request is the same with
    // current, don't need send the request to driver
    if(strlen(file->stat.current.value) != 0
        && file->comp((void *)req_item, (void *)(&(file->stat.current))) == 0)
        return 1;

    // Update current request
    memcpy(&(file->stat.current), req_item, sizeof(struct req_item));
    ALOGD_IF(DEBUG_V, "current value: %s", file->stat.current.value);
    common_subsys_set_current_config(file);

    return 1;
}

int common_subsys_clear(const char *path, struct file *file)
{
    struct subsys *subsys = NULL;
    struct subsys_inode *inode = NULL;
    char buf[128] = {'\0'};

    if (CC_UNLIKELY(path == NULL || file == NULL))
        return 0;

    ENTER("%s", file->name);
    subsys = find_subsys_by_name(file->name);
    if (subsys == NULL) {
        ALOGE("Don't support subsys %s", file->name);
        return 0;
    }

    sprd_timer_settime(file->timer_id, 0);
    memset(&(file->stat), 0, sizeof(struct request_stat));

    for (int i = 0; i < subsys->inode_count; i++) {
        inode = &(subsys->inodes[i]);
        if (inode->no_has_def == 0) {
            snprintf(buf, sizeof(buf), "%s/%s", inode->path, inode->file);
            if (access(buf, F_OK) == 0) {
                sprd_write(buf, inode->value.def_value);
                ALOGD_IF(DEBUG, "Set %s: %s", buf, inode->value.def_value);
            }
        }
    }

    return 1;
}

// used by qsort()
int common_subsys_comp(const void *a, const void *b)
{
    struct req_item  *aa = (struct req_item *)a;
    struct req_item  *bb = (struct req_item *)b;
    int level_a = 0;
    int level_b = 0;
    // value: conf_num:level
    char *aaa = strchr(aa->value, ':');
    char *bbb = strchr(bb->value, ':');

    if (aaa == NULL || bbb == NULL) {
        ALOGE("Request item don't include priority");
        return 0;
    }
    level_a = atoi(aaa + 1);
    level_b = atoi(bbb + 1);

    return level_a - level_b;
}

static void remove_invalid_item(struct file *file)
{
    int j = 0;
    struct req_item tmp[NUM_REQUST_FOR_FILE_MAX];

    if (DEBUG_V) ENTER();

    for (int i = 0; i <= file->stat.count; i++) {
        if (file->stat.items[i].times > 0) {
            memcpy(&tmp[j++], &(file->stat.items[i]), sizeof(struct req_item));
        }
    }
    file->stat.count = j;
    memset(file->stat.items, 0, sizeof(file->stat.items));

    if (file->stat.count <= 0) return;
    memcpy(file->stat.items, tmp, j*sizeof(struct req_item));
}

static void remove_eplased_item(struct file *file)
{
    struct timespec now;
    long long diff_ms = -1;
    struct request_stat *stat = NULL;

    if (DEBUG_V) ENTER();

    clock_gettime(CLOCK_MONOTONIC, &now);
    for (int i = 0; i <= file->stat.count; i++) {
        stat = &(file->stat);
        if (stat->items[i].duration_end_time.tv_sec > 0 || stat->items[i].duration_end_time.tv_nsec > 0) {
            diff_ms = calc_timespan_ms(now, stat->items[i].duration_end_time);
            if (diff_ms <= 0) {
                stat->items[i].times--;
                memset(&(stat->items[i].duration_end_time), 0, sizeof(struct timespec));
            }
        }
    }

    remove_invalid_item(file);
}

static int find_item_by_value(const struct file *file)
{
    struct req_item tmp;

    if (file == NULL) return -1;

    if (DEBUG_V) ENTER();
    memset(&tmp, 0, sizeof(tmp));
    strcpy(tmp.value, file->value.target_value);

    for (int i = 0; i < file->stat.count; i++) {
        if (file->comp((void *)&tmp, (void *)(&(file->stat.items[i]))) == 0) {
            return i;
        }
    }

    return -1;
}

static void request_item_sort(struct file *file)
{
    if (DEBUG_V) ENTER("The number of request is %d", file->stat.count);
    if (file->stat.count > 1)
        qsort(file->stat.items, file->stat.count, sizeof(struct req_item), file->comp);
}

void sort_request_for_file(int enable, int duration, struct file *file)
{
    int index = -1;
    struct timespec now;
    static struct timespec duration_end_time;
    long long diff_ms;

    if (DEBUG_V) ENTER();
    if (CC_UNLIKELY(enable == 1 && strlen(file->value.target_value) == 0)) {
        ALOGE("The target value is incorrect");
        return;
    }

    if (enable == 0 && strlen(file->value.target_value) == 0) {
        remove_eplased_item(file);
        return;
    }

    index = find_item_by_value(file);
    if (enable) {
        if (index == -1) {
            if (file->stat.count >= NUM_REQUST_FOR_FILE_MAX) {
                ALOGE("!!!Request items array is full");
                return;
            }

            strcpy(file->stat.items[file->stat.count].value, file->value.target_value);
            file->stat.items[file->stat.count].times = 1;
            if (duration > 0) {
                clock_gettime(CLOCK_MONOTONIC, &now);
                file->stat.items[file->stat.count].duration_end_time.tv_sec = now.tv_sec + duration/SEC_TO_MS;
                file->stat.items[file->stat.count].duration_end_time.tv_nsec = now.tv_nsec + (duration%SEC_TO_MS)*MS_TO_NS;
            }
            file->stat.count++;

            request_item_sort(file);
        } else {
            if (duration == 0) {
                file->stat.items[index].times++;
            } else {
                clock_gettime(CLOCK_MONOTONIC, &now);
                diff_ms = calc_timespan_ms(now, file->stat.items[index].duration_end_time);
                if (diff_ms < duration) {
                    if (file->stat.items[index].duration_end_time.tv_sec == 0
                        && file->stat.items[index].duration_end_time.tv_nsec == 0) {
                        file->stat.items[index].times++;
                    }

                    file->stat.items[index].duration_end_time.tv_sec = now.tv_sec + duration/SEC_TO_MS;
                    file->stat.items[index].duration_end_time.tv_nsec = now.tv_nsec + (duration%SEC_TO_MS)*MS_TO_NS;
                }
            }
        }
    } else {
        if (DEBUG_V) ALOGD("%s: index=%d", __func__, index);
        if (index < 0) return;

        file->stat.items[index].times--;
        if (file->stat.items[index].times == 0)
            remove_invalid_item(file);
    }

    if (DEBUG_V) ALOGD("Exit %s", __func__);
}
