#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <dirent.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include "eng_modules.h"
#include "engopt.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define  LOG_TAG "ENGPC"
#include <cutils/log.h>

#define MAX_LEN NAME_MAX //50

typedef void (*REGISTER_FUNC)(struct eng_callback *register_callback);
typedef void (*REGISTER_EXT_FUNC)(struct eng_callback *reg, int *num);

static const char *eng_modules_path = "/system/lib/engpc";
static const char *eng_modules_path_new_version = "/vendor/lib/npidevice";

eng_modules* get_eng_modules(struct eng_callback p)
{
    ENG_LOG("%s",__FUNCTION__);
    eng_modules *modules = (eng_modules*)malloc(sizeof(eng_modules));
    if (modules == NULL)
    {
        ENG_LOG("%s malloc fail...",__FUNCTION__);
        return NULL;
    }
    memset(modules,0,sizeof(eng_modules));
    modules->callback.type = p.type;
    modules->callback.subtype = p.subtype;
    modules->callback.diag_ap_cmd = p.diag_ap_cmd;
    modules->callback.also_need_to_cp = p.also_need_to_cp;
    sprintf(modules->callback.at_cmd, "%s", p.at_cmd);
    modules->callback.eng_diag_func = p.eng_diag_func;
    modules->callback.eng_linuxcmd_func = p.eng_linuxcmd_func;

    return modules;
}

int readFileList(const char *basePath, char **f_name)
{
    DIR *dir;
    struct dirent *ptr;
    int num = 0;
    ENG_LOG("%s",__FUNCTION__);

    if ((dir = opendir(basePath)) == NULL)
    {
        ENG_LOG("Open %s error...%s",basePath,dlerror());
        return 0;
    }

    while ((ptr = readdir(dir)) != NULL)
    {
        if(ptr->d_type == 8){    ///file
            ENG_LOG("d_name:%s/%s\n",basePath,ptr->d_name);
            f_name[num] = ptr->d_name;
            num ++;
            ENG_LOG("d_name:%s\n",*f_name);
        }
    }
    closedir(dir);
    return num;
}

int eng_modules_search(const char * eng_path, struct list_head *head){
    REGISTER_FUNC eng_register_func = NULL;
    REGISTER_EXT_FUNC eng_register_ext_func = NULL;
    struct eng_callback register_callback;
    struct eng_callback register_arr[32];
    struct eng_callback *register_arr_ptr = register_arr;
    int register_num = 0;
    int i = 0;
    char path[MAX_LEN]=" ";
    char lnk_path[MAX_LEN]=" ";
    int readsize = 0;

    eng_modules *modules;

    //get so name fail:empty
    DIR *dir;
    struct dirent *ptr;
    void *handler = NULL;

    ENG_LOG("%s",__FUNCTION__);

    if (NULL == eng_path) {
        ENG_LOG("%s eng_path = NULL", __FUNCTION__);
        return 0;
    }

    if ((dir = opendir(eng_path)) == NULL)
    {
        ENG_LOG("Open %s error...%s",eng_path,dlerror());
        return 0;
    }

    while ((ptr = readdir(dir)) != NULL)
    {
      if (ptr->d_type == 8 || ptr->d_type == 10) { /// file  , 10 == DT_LNK
        ENG_LOG("d_name:%s/%s\n", eng_path, ptr->d_name);
        snprintf(path, sizeof(path), "%s/%s", eng_path, ptr->d_name);
        ENG_LOG("find lib path: %s", path);

        if (ptr->d_type == 10) //DT_LNK
        {
            memset(lnk_path,0,sizeof(lnk_path));
            readsize = readlink(path, lnk_path, sizeof(lnk_path));
            ENG_LOG("%s readsize:%d lnk_path:%s \n", path ,readsize, lnk_path);

            if(readsize == -1) {
              ENG_LOG("ERROR! Fail to readlink!\n");
              continue;
            }

            memset(path, 0, sizeof(path));
            strncpy(path, lnk_path, strlen(lnk_path));
        }

        if (access(path, R_OK) == 0) {
          handler = NULL;
          handler = dlopen(path, RTLD_LAZY);
          if (handler == NULL) {
            ENG_LOG("%s dlopen fail! %s \n", path, dlerror());
          } else {
            eng_register_func = (REGISTER_FUNC)dlsym(handler, "register_this_module");
            if (eng_register_func != NULL) {
              memset(&register_callback, 0, sizeof(struct eng_callback));
              eng_register_func(&register_callback);
              ENG_LOG("%d:type:%d subtype:%d data_cmd:%d at_cmd:%s", i,
                        register_callback.type, register_callback.subtype,
                        register_callback.diag_ap_cmd, register_callback.at_cmd);
              modules = get_eng_modules(register_callback);
              if (modules == NULL) {
                ENG_LOG("%s modules == NULL\n", __FUNCTION__);
                continue;
              }
              list_add_tail(&modules->node, head);
            }

            eng_register_ext_func = (REGISTER_EXT_FUNC)dlsym(handler, "register_this_module_ext");
            if (eng_register_ext_func != NULL) {
              memset(register_arr, 0, sizeof(register_arr));
              eng_register_ext_func(register_arr_ptr, &register_num);
              ENG_LOG("register_num:%d",register_num);

              for (i = 0; i < register_num; i++) {
                ENG_LOG("%d:type:%d subtype:%d data_cmd:%d at_cmd:%s", i,
                        register_arr[i].type, register_arr[i].subtype,
                        register_arr[i].diag_ap_cmd, register_arr[i].at_cmd);
                modules = get_eng_modules(register_arr[i]);
                if (modules == NULL) {
                  ENG_LOG("%s modules == NULL\n", __FUNCTION__);
                  continue;
                }
                list_add_tail(&modules->node, head);
              }
            }
            if (eng_register_func == NULL && eng_register_ext_func == NULL) {
              dlclose(handler);
              ENG_LOG("%s dlsym fail! %s\n", path, dlerror());
              continue;
            }
          }
        } else {
            ENG_LOG("%s is not allow to read!\n", path);
        }
      }
    }
    closedir(dir);

    return 0;
}

int eng_modules_load(struct list_head *head )
{
#if 0
    REGISTER_FUNC eng_register_func = NULL;
    struct eng_callback register_callback;
    char path[MAX_LEN]=" ";

    void *handler[MAX_LEN];
    char *f_name[MAX_LEN];
    char **p;
    int i = 0;
    p = f_name;
    eng_modules *modules;
#endif

    ENG_LOG("%s",__FUNCTION__);

    INIT_LIST_HEAD(head);
    eng_modules_search(eng_modules_path, head);
    eng_modules_search(eng_modules_path_new_version, head);
#if 0
    int num = readFileList(eng_modules_path,p);
    ENG_LOG("file num: %d\n",num);

    for (i = 0 ; i < num; i++) {
        snprintf(path, sizeof(path), "%s/%s",
                        eng_modules_path, f_name[i]);
        ENG_LOG("find lib path: %s",path);

        if (access(path, R_OK) == 0){
            handler[i] = dlopen(path,RTLD_LAZY);
            if (handler[i] == NULL){
                ENG_LOG("%s dlopen fail! %s \n",path,dlerror());
            }else{
                eng_register_func = (REGISTER_FUNC)dlsym(handler[i], "register_this_module");
                if(!eng_register_func){
                    dlclose(handler[i]);
                    ENG_LOG("%s dlsym fail! %s\n",path,dlerror());
                    continue;
                }
                eng_register_func(&register_callback);

                modules = get_eng_modules(register_callback);
                if (modules == NULL){
                    continue;
                }
                list_add_tail(&modules->node, head);
            }
        }
    }
#endif
    return 0;
}

