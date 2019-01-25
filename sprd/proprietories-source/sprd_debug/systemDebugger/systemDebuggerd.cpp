#define LOG_TAG "systemDebuggerd"

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sprd_debugger.h>
#include <cutils/properties.h>
#include <cutils/sockets.h>
#include <cutils/log.h>
#include <fcntl.h>
#include <android-base/file.h>

#define SPRD_CONFIG_PATH      "data/sprd_debug/.sprd_debugger.conf"
#define SPRD_CONFIG_ORIG      "/system/etc/.sprd_debugger.conf"
#define SPRD_SYSDUMP_CONFIG   "/proc/sprd_sysdump"

char configBuffer[25];
char* sysdumpconfig = NULL;

int copyConfFile(){
    int ch;
    FILE *orig, *conf;
    if((orig=fopen(SPRD_CONFIG_ORIG,"r"))==NULL){
        ALOGE("Copy_file: Unable to open %s : %s\n", SPRD_CONFIG_ORIG, strerror(errno));
        return -1;
    }
    if((conf=fopen(SPRD_CONFIG_PATH,"w"))==NULL){
        ALOGE("Copy_file: Unable to open %s : %s\n", SPRD_CONFIG_PATH, strerror(errno));
        return -1;
    }
    int i=0;
    ch = fgetc(orig);
    while(ch!=EOF){
       if (ch =='\n')
          configBuffer[i] = '\0';
       else
          configBuffer[i]=ch;
       ALOGV("configBuffer[%d]= %c", i,configBuffer[i]);
       fputc(ch,conf);
       ch=fgetc(orig);
       i++;
    }
    sysdumpconfig = configBuffer;
    fclose(orig);
    fclose(conf);
    ALOGD("sysdumpconfig return %s", sysdumpconfig);
    if(sysdumpconfig == NULL) {
       ALOGE("sysdumpconfig  get fail");
       return -1;
    }
    return 1;
}

int writeConfig(const char* content) {
     ALOGV("write event");
     FILE *fp = fopen(SPRD_CONFIG_PATH,"w");
     if(fp==NULL) {
        ALOGE("write_file: Unable to open %s : %s\n", SPRD_CONFIG_PATH, strerror(errno));
        return -1;
     }
     int result = fprintf(fp,"%s" ,content);
     fclose(fp);
     return result;
}


//write flag to /proc/sprd_sysdump
int writeFile(const char* path, const char* content){
    int fd = TEMP_FAILURE_RETRY(open(path, O_WRONLY|O_CREAT|O_NOFOLLOW|O_CLOEXEC, 0600));
    if (fd == -1) {
        ALOGE("write_file: Unable to open %s : %s\n",path, strerror(errno));
        return -1;
    }
    int result = android::base::WriteStringToFd(content, fd) ? 0 : -1;
    if (result == -1) {
        ALOGE("write_file: Unable to write to '%s': %s\n", path, strerror(errno));
        return -1;
    }
    close(fd);
    return result;
}

static int handle_request(int fd) {
   ALOGV("HANDLE REQUEST");
   sprd_debugger_action_t action;
   int status = 0 ;
   status = TEMP_FAILURE_RETRY(read(fd, &action, sizeof(action)));
  if (status < 0) {
    ALOGE("read failure? %d", action);
    return -1;
  }
  if (status != sizeof(action)) {
    ALOGE("invalid request of size %d ", status);
    return -1;
}
   sprd_debugger_action_t event;
   event = action ;
   ALOGD("EVENT = %d , ACTION = %d ,status =%d", event, action, status);
   if (event == DEBUGGER_ACTION_SYSDUMP_ON) {
       if(writeFile(SPRD_SYSDUMP_CONFIG, "on") == 0) {
           writeConfig("on");
           property_set("persist.sys.sysdump", "on");
       }
   } else if (event == DEBUGGER_ACTION_SYSDUMP_OFF){
       if(writeFile(SPRD_SYSDUMP_CONFIG, "off") == 0) {
           writeConfig("off");
           property_set("persist.sys.sysdump", "off");
       }
   }
   write(fd, "success", sizeof("success"));
   close(fd);
   return status;
}
#define SOCKET_NAME "SystemDebugger"
static int do_server(){
  FILE* fp;
  int result;

  if ((fp = fopen(SPRD_CONFIG_PATH, "r"))== NULL) {
     result = copyConfFile();
     if(result==-1) {
        ALOGE(" Unable to open %s : %s\n", SPRD_CONFIG_PATH, strerror(errno));
        return -1;
     }
     ALOGD("Initial config %s ", sysdumpconfig);
  } else {
    sysdumpconfig = fgets(configBuffer, sizeof(configBuffer), fp);
    fclose(fp);
  }

  ALOGD("sysdumpconfig = %s",sysdumpconfig);
  if(sysdumpconfig == NULL){
    if ((fp = fopen(SPRD_SYSDUMP_CONFIG, "r"))){
        sysdumpconfig = fgets(configBuffer, sizeof(configBuffer), fp);
        ALOGD("gets config from %s = %s", SPRD_SYSDUMP_CONFIG,sysdumpconfig);
        if(strstr(sysdumpconfig,"1")) {
           writeConfig("on");
           strcpy(sysdumpconfig, "on");
        } else {
           writeConfig("off");
           strcpy(sysdumpconfig, "off");
        }
        ALOGD("set sysdumpconfig = %s",sysdumpconfig);
    } else {
        ALOGE(" Unable to open %s : %s\n", SPRD_SYSDUMP_CONFIG, strerror(errno));
        return -1;
    }
  }
  if(strstr(sysdumpconfig,"on")) {
    if(writeFile(SPRD_SYSDUMP_CONFIG, "on") == -1) {
       property_set("persist.sys.sysdump", "-1");
    } else {
       property_set("persist.sys.sysdump", "on");
       ALOGD("Initial sysdump on");
    }
  } else if(strstr(sysdumpconfig,"off")){
    if(writeFile(SPRD_SYSDUMP_CONFIG, "off") == -1) {
       property_set("persist.sys.sysdump", "-1");
    } else {
       property_set("persist.sys.sysdump", "off");
       ALOGD("Initial sysdump off");
    }
  }

  //init socket server
  int s = socket_local_server(SOCKET_NAME, ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
  if (s < 0) {
    ALOGE("get socket %s fd= %d failed!", SOCKET_NAME,s);
    return -1;
  }
  fcntl(s, F_SETFD, FD_CLOEXEC);
  for (;;) {
    sockaddr addr;
    socklen_t alen = sizeof(addr);
    ALOGD("waiting for connection\n");
    int fd = accept(s, &addr, &alen);
    if (fd < 0) {
      ALOGD("accept failed: %s\n", strerror(errno));
      continue;
    }
    fcntl(fd, F_SETFD, FD_CLOEXEC);
    handle_request(fd);
  }
  return 0;
}

int main(int argc, char** argv) {
  ALOGD("start SystemDebuggerd %d", argc);
  if(argc == 1) {
    return do_server(); //socket server
  }
  return 1;
}
