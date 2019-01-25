#include <stdio.h>
//#include <sys/socket.h>
#include <sys/un.h>
#include <linux/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <cutils/log.h>
#include <cutils/list.h>
//#include <cutils/sockets.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdio.h>
#include <malloc.h>
#include <linux/signal.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "FLASHTEST"
#define TAG_NORMAL "FLASHTEST"

//#define FALSETEST_TEST_MODE

int filesize = 0;
int testtime = 0;
int currenttestprogress = 0;
bool isExit = false;
char* filePath = "/data/flashtest.txt";
char* logPath = "/data/flashtest.log";
int testResult = -1;//-1:unknowm;0:pass;other:fail
int readerror = 0;
FILE * fplog = NULL;

 void fillFixDataToBuff(char* buff,int len){
    char c = 0;
    for(int i = 0;i < len; i++ ){
        buff[i] = 48+c;
        c++;
        if(c > 9){
            c = 0;
        }
    }
}
bool isByteBuffEquals(char*buff1, char*buff2,int len) {
    for (int i = 0; i < len; i++) {
        if (buff1[i] != buff2[i]) {
            return false;
        }
    }
    return true;

}
#ifdef FALSETEST_TEST_MODE
void handler(int)

{
    if((!isExit) && (testResult== -1)){
        signal(SIGALRM, handler);
        alarm(10);
        currenttestprogress += 10;
        printf("test1 time =%d (seconds).\n",currenttestprogress);
		fflush(stdout);

        ALOGE("test time =%d (seconds).\n",currenttestprogress);
        char buff[256];
        memset(buff,0 ,256);
        sprintf(buff,"testTimes = %d(seconds) readWriteError=%d\n",currenttestprogress/10,readerror);
        if(fplog != NULL){
            fputs(buff,fplog);
        }
        
    }

    if(currenttestprogress >= testtime*10){
        isExit = true;
    }
    
}
#else
void handler(int)

{
    if((!isExit) && (testResult== -1) ){
        signal(SIGALRM, handler);
        alarm(60);
        currenttestprogress += 60;
        printf("test time =  %d (min) \n",currenttestprogress/60);
		fflush(stdout);
        ALOGE("test time = %d (min) \n",currenttestprogress/60);
        char buff[256];
        memset(buff,0 ,256);
        sprintf(buff,"testTimes = %d(min) readWriteError= %d\n",currenttestprogress/60,readerror);
        if(fplog != NULL){
            fputs(buff,fplog);
        }
        
    }

    if(currenttestprogress >= testtime*60*60){
        isExit = true;
    }
    
}
#endif
void* readWriteTest(void *){
    char* content = (char*)malloc(1025);
	memset(content,0 ,1025);
    fillFixDataToBuff(content,1024);
    
    char* s = (char*)malloc(1025);
    memset(s,0 ,1025);
    while(!isExit){
        if((access(filePath,0))!=-1){
            remove(filePath);
        }
        FILE * fp =fopen(filePath,"w+");
        if(fp != NULL){
            int filelen = 0;     
            while((filelen < (filesize*1024*1024)) && (!isExit)){
                fputs(content,fp);
                filelen += 1024;
            }
            fclose(fp);
            if(!isExit){
                FILE * fpr =fopen(filePath,"r");
                if(fpr != NULL){
                    int count = 0;
                    while((!isExit) && (count < filesize*1024)){
                        memset(s,0 ,1025);
                        fgets(s,1025,fpr);
                        if(!isByteBuffEquals(s,content,1024)){
							
                            readerror++;
                        }
                        count++;
                    }
                    fclose(fpr);
                }else{
                    testResult = 1;
                    readerror++; 
                }
            }
            
        }else{
            testResult = 1;
        }
        
    }
	/*if((access(filePath,0))!=-1){
        remove(filePath);
    }*/
    free(content);
    free(s);
    testResult = 0;
    return NULL;
}

void* handerTime(void *){
	handler(0);
	return NULL;
}

int main (int argc, char *argv[]){
	printf("Welcome to use FlashTest tool(v1.0.1). \n");
    if(argc == 3){ 
        filesize = atoi(argv[1]);
        testtime = atoi(argv[2]);
    }else{
        filesize = 1;
        testtime = 24;
    }
	currenttestprogress =0;
	//printf("argv::: testtime::%d\n",testtime);
    isExit = false;
    fplog =fopen(logPath,"w+");
    pthread_t wrthread;
	//pthread_t timerthread;
    int ret = pthread_create(&wrthread,NULL,readWriteTest,NULL);
	//int ret1 = pthread_create(&timerthread,NULL,handerTime,NULL);
    handler(0);
    void *retval;
	//void *retval1;
    pthread_join(wrthread,&retval);
	//pthread_join(timerthread,&retval1);
    char buff[256];
    memset(buff,0 ,256);
    if(testResult == 0){
        sprintf(buff,"testTimes = %d(min) readWriteError=%d\n testResult:pass",currenttestprogress/60,readerror);
        printf("testResult:pass");
        ALOGE("flashtest testResult:pass");
    }else{
        sprintf(buff,"testTimes = %d(min) readWriteError=%d\n testResult:fail",currenttestprogress/60,readerror);
        printf("testResult:fail");
        ALOGE("flashtest testResult:fail");
    }
    if(fplog != NULL){
        fputs(buff,fplog);
    }
    fclose(fplog);
    return 0;
}

