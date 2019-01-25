#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <dirent.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include "autotest_modules.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define  LOG_TAG "autotest"
#include <cutils/log.h>

#define	MAX_LEN 50

typedef void (*REGISTER_FUNC)(struct autotest_callback *register_callback);

autotest_modules* get_autotest_modules(struct autotest_callback p)
{
    ALOGD("%s",__FUNCTION__);
    autotest_modules *modules = (autotest_modules*)malloc(sizeof(autotest_modules));
    if (modules == NULL)
    {
        ALOGE("%s malloc fail...",__FUNCTION__);
        return NULL;
    }
    memset(modules,0,sizeof(autotest_modules));
    modules->callback.diag_ap_cmd = p.diag_ap_cmd;
    modules->callback.autotest_func = p.autotest_func;

    return modules;
}

int readFileList(const char *basePath, char **f_name)
{
    DIR *dir;
    struct dirent *ptr;
    int num = 0;
    ALOGD("%s",__FUNCTION__);

    if ((dir = opendir(basePath)) == NULL)
    {
        ALOGE("Open dir error...");
        return -1;
    }

    while ((ptr = readdir(dir)) != NULL)
    {
        if(ptr->d_type == 8){    ///file
            ALOGD("d_name:%s/%s\n",basePath,ptr->d_name);
            f_name[num] = ptr->d_name;
            num ++;
            ALOGD("d_name:%s\n",*f_name);
        }
    }
    closedir(dir);
    return num;
}


int autotest_modules_load(struct list_head *head )
{
    REGISTER_FUNC autotest_register_func = NULL;
    struct autotest_callback register_callback;
    char path[MAX_LEN]=" ";
#ifdef SHARKLJ1_BBAT_BIT32
	static const char *autotest_modules_path = "/system/lib/autotest";
#else 
 	static const char *autotest_modules_path = "/system/lib64/autotest";
#endif
    void *handler[MAX_LEN];
    char *f_name[MAX_LEN];
    char **p;
    int i = 0;
    p = f_name;
    autotest_modules *modules;

    ALOGD("%s",__FUNCTION__);

    INIT_LIST_HEAD(head);
    int num = readFileList(autotest_modules_path,p);
    ALOGD("file num: %d\n",num);

    for (i = 0 ; i < num; i++) {
        snprintf(path, sizeof(path), "%s/%s",
                        autotest_modules_path, f_name[i]);
        ALOGD("find lib path: %s",path);

        if (access(path, R_OK) == 0){
            handler[i] = dlopen(path,RTLD_LAZY);
            if (handler[i] == NULL){
                ALOGE("%s dlopen fail! %s \n",path,dlerror());
            }else{
                autotest_register_func = (REGISTER_FUNC)dlsym(handler[i], "register_this_module");
                if(!autotest_register_func){
                    dlclose(handler[i]);
                    ALOGE("%s dlsym fail! %s\n",path,dlerror());
                    continue;
                }
                autotest_register_func(&register_callback);

                modules = get_autotest_modules(register_callback);
                if (modules == NULL){
                    continue;
                }
                list_add_tail(&modules->node, head);
            }
        }
    }
    return 0;
}

