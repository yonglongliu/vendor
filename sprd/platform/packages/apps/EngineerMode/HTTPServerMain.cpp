/* HTTPServerMain.cpp */

#include<iostream>
#include<string>

#include<stdlib.h>
#include <dlfcn.h>
#ifdef BUILD_ENG
  #include "sprd_atci.h"
#endif

#include"HTTPServer.h"
#include <pthread.h>
#include <cutils/properties.h>

using namespace std;
int maintest(int argc, char* argv[]);
typedef int (*FACTORY_RESET_FUNC)();
void open_modem_log(bool is_on);
void trigger_reboot_test();
void loadLibrecover();

#define MAX_AT_RESPONSE_LEN 1024

int main(int argc, char* argv[])
{
	int port;
	HTTPServer* httpServer;
    char buf[PROPERTY_VALUE_MAX]={0};

	MYLOG("------main---------");

#ifdef OPEN_REBOOT_TEST
	trigger_reboot_test();
#else
	MYLOG("------reboot test closed---------");
#endif

//open modem log in user debug version
        property_get("persist.sys.isfirstboot", buf, "true");
        if (!strncmp("true", buf, 4)) {
	     MYLOG("is first boot");
#ifdef OPEN_MODEM_LOG
	     open_modem_log(true);
#else
            open_modem_log(false);
#endif
            property_set("persist.sys.isfirstboot","false");
         }

//#define fortest
#ifdef fortest
    return maintest(argc, argv);
#endif
	if(argc == 2){
		port = atoi(argv[1]);
		httpServer = new HTTPServer(port);
	}else{
		httpServer = new HTTPServer();
	}

	if(httpServer->run()){
		cerr<<"Error starting HTTPServer"<<endl;
	}

	free(httpServer);

	return 0;
}

#include "ATProcesser.h"
int maintest(int argc, char* argv[])
{
    string content;
    content=ATProcesser("").readfile(argv[1]);
    cout << content;
    return 0;
};

void open_modem_log(bool is_on){

     const char* atresptr;
      char atrsp[MAX_AT_RESPONSE_LEN]={0};

     if(is_on){
    	atresptr= sendCmd(0, "at+armlog=1\r");
     }else{
		atresptr= sendCmd(0, "at+armlog=0\r");
        }

        snprintf(atrsp, MAX_AT_RESPONSE_LEN, "%s", atresptr);
        MYLOG("atrsp= %s", atrsp);

     if(is_on){
            property_set("persist.sys.cp2log","1");
          atresptr= sendCmd(0, "AT+SPCAPLOG=1\r");
          atresptr= sendCmd(0, "AT+SPDSPOP=2\r");
 	}else{
            property_set("persist.sys.cp2log","0");
	   atresptr= sendCmd(0, "AT+SPCAPLOG=0\r");
          atresptr= sendCmd(0, "AT+SPDSPOP=0\r");
 	}
}

void trigger_reboot_test(){
    char reboot[PROPERTY_VALUE_MAX]={0};
    char reboot_count[PROPERTY_VALUE_MAX]={0};
    char reboot_delay[PROPERTY_VALUE_MAX]={0};
    property_get("persist.sys.reboot_test", reboot, "false");
    property_get("persist.sys.reboot_test.count", reboot_count, "0");
    property_get("persist.sys.reboot_test.delay", reboot_delay, "0");
    MYLOG("------trigger_reboot_test-----------reboot");
    char reset_contents[PROPERTY_VALUE_MAX]={0};
    string catCmd = "cat /productinfo/factory_reset.txt";
    string tmp_contents = ATProcesser("").shell(catCmd,"r");
	char delim[] = "-";
    int reset_count = 0;
    int reset_property = 0;
    int reset_delay = 0;
    if (!tmp_contents.empty()) {
       strcpy(reset_contents,tmp_contents.c_str());
       char* reset_data = strtok(reset_contents,delim);
       if (reset_data != NULL && strlen(reset_data) > 0) {
          reset_count = atoi(reset_data);
          int tmp_count = 0;
		  MYLOG("-----reset_count=%d",reset_count);
          while (reset_data != NULL && reset_count) {
            reset_data = strtok(NULL,delim);
            tmp_count++;
			if(!reset_data || tmp_count > 2){
				break;
			}
            if(tmp_count == 1){
				MYLOG("-----reset_data=%s",reset_data);
               reset_property = atoi(reset_data);
            } else {
               MYLOG("------reset_data=%s",reset_data);
               reset_delay = atoi(reset_data);
            }
          }
       }
    }
    MYLOG("------trigger_reboot_test-----------reboot=%s",reboot);
    int trigger = 1;
    int wait_count = 0;
    if (!strncmp("true", reboot, 4) || reset_count > 0) {
        while (trigger) {
            string content = ATProcesser("").shell("ps | grep b2g | wc -l", "r");
            MYLOG("------trigger_reboot_test---------b2g num=%s",content.c_str());
            if (!content.empty()) {
                MYLOG("------trigger_reboot_test-----------reboot_count=%s",reboot_count);
                int b2g_num = atoi(content.c_str());
                int reboot_num = atoi(reboot_count);
                if (b2g_num >= 5) {
                    trigger = 0;
                    if(reboot_num > 1){
                    reboot_num--;
                    sprintf(reboot_count, "%d", reboot_num);
                    property_set("persist.sys.reboot_test.count", reboot_count);
                    if (reboot_num <= 1) {
                        MYLOG("------trigger_reboot_test-----------reboot test end");
                        property_set("persist.sys.reboot_test", "false");
                        property_set("persist.sys.reboot_test.count", "0");
                        property_set("persist.sys.reboot_test.delay", "0");
                    }
                    sleep(atoi(reboot_delay));
                    ATProcesser("").shell("reboot", "r");
                    MYLOG("------trigger_reboot_test-----------reboot device");
                    } else if ( reset_count > 0){
                      reset_count--;
                      MYLOG("------trigger_reset_test-----------reset_count=%d",reset_count);
                      char cmd[PROPERTY_VALUE_MAX] = {0};
                      sprintf(cmd, "echo %d-%d-%d \> /productinfo/factory_reset.txt", reset_count,reset_property,reset_delay);
                      MYLOG("------trigger_reset_test-----------cmd=%s",cmd);
                      ATProcesser("").shell(cmd, "r");
                      MYLOG("-----trigger_reset_test-----------reset_count =%d  reset_property=%d",reset_count,reset_property);
                      if(reset_count > 0 && reset_property > 0 ){
                          sleep(reset_delay);
                          loadLibrecover();
                      }else{
                          char rcmd[PROPERTY_VALUE_MAX] = {0};
                          sprintf(rcmd, "echo %d-%d-%d \> /productinfo/factory_reset.txt", 0,0,0);
                          ATProcesser("").shell(rcmd, "r");
                      }
                    }
                    break;
                }
            }
            sleep(2);
            wait_count++;
            if (wait_count > 40) {
                trigger = 0;
                break;
            }
            MYLOG("------trigger_reboot_test-----------wait_count=%d", wait_count);
        }
    }else {
        MYLOG("------trigger_reboot_test----------reboot disabled");
    }
}

//func loadLibrecover
void loadLibrecover()
{
    MYLOG(" load librecovery.so!");
   void *handle;
   FACTORY_RESET_FUNC factoryReset = NULL;
   handle = dlopen("librecovery.so", RTLD_LAZY);
   if(!handle)
   {
      MYLOG("load librecovery.so failed!");
      return;
   }

   factoryReset = (FACTORY_RESET_FUNC)dlsym(handle, "factoryReset");

    MYLOG("load factoryReset");
   int resetOk = factoryReset();
   if(resetOk == 0){
       MYLOG("factoryReset reboot");
   ATProcesser("").shell("reboot", "r");
   }
   //close so
   dlclose(handle);
}
