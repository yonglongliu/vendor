#define LOG_TAG "SPRD_DEBUGGER"

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sprd_debugger.h>
#include <cutils/sockets.h>
#include <cutils/log.h>

#define SOCKET_NAME "SystemDebugger"

int send_request(sprd_debugger_action_t action){
  //sprd_debugger_action_t action = event;
  int s = socket_local_client(SOCKET_NAME, ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
  if( s < 0 ){
  ALOGE("client connection faild %d " , s);
  return -1;
  }
  int result = 0;
  int msg_len = sizeof(action);
  if (TEMP_FAILURE_RETRY(write(s, &action, msg_len)) != (ssize_t) msg_len) {
    ALOGE("ERROR TO WRITE");
    result = -1;
  } else {
    char ack;
    if (TEMP_FAILURE_RETRY(read(s, &ack, 1)) != 1) {
      ALOGE("ERROR TO READ");
      result = -1;
    }
  }
  if (result == -1){
    close(s);
    return -1;
  }
  char buffer[1024];
  ssize_t n;
  while (n = (TEMP_FAILURE_RETRY(read(s, buffer, sizeof(buffer)))) > 0) {
    ALOGV("while %s", buffer);
    if (strstr(buffer,"success")) {
      break;
    }
  }
  close(s);
  return result;
}

int main(int argc, char** argv) {
  ALOGD("start sprd_debugger %d", argc);
  int result = 0;
  for (int i = 1; i < argc; i++) {
    ALOGV("argv[%d] is %s",i,argv[i]);
    if (!strcmp(argv[i], "-s")) {
    } else if (!strcmp(argv[i], "on")) {
      result = send_request(DEBUGGER_ACTION_SYSDUMP_ON);
      ALOGD("Request sysdump on[%d] is %s",i,argv[i]);
    } else if (!strcmp(argv[i], "off")){
      result = send_request(DEBUGGER_ACTION_SYSDUMP_OFF);
      ALOGD("Request sysdump off[%d] is %s",i,argv[i]);
    }
  }
  return result;
}
