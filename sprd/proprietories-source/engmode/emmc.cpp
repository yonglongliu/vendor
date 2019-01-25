#include "emmc.h"
//#include "type.h"
#include <dlfcn.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <utils/Log.h>
#include <math.h> 
//------------------------------------------------------------------------------
// enable or disable local debug
#define DBG_ENABLE_DBGMSG
#define DBG_ENABLE_WRNMSG
#define DBG_ENABLE_ERRMSG
#define DBG_ENABLE_INFMSG
#define DBG_ENABLE_FUNINF
//#include "debug.h"
#include "engopt.h"


#define ENG_STREND "\r\n"
//------------------------------------------------------------------------------


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--} // namespace check emmc size
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

int get_emmc_size(char *req, char *rsp)
{
	int fd;
	int ret = 1; //fail
	int cur_row = 2;
	char  buffer[64]={0},temp[64]={0};
	int read_len = 0;
	float size = 0;
	char *endptr;


	if (0 == access("/sys/block/mmcblk0",F_OK)){
		sprintf(temp,"%s%s",TEXT_EMMC_STATE,TEXT_EMMC_OK);
		//INFMSG("access_ok %s%s",TEXT_EMMC_STATE,TEXT_EMMC_OK);
		ENG_LOG("access_ok\n");


		fd = open("/sys/block/mmcblk0/size",O_RDONLY);
		if(fd < 0){
			ENG_LOG("emmc card no exist\n");
			sprintf(rsp, "%s%s", "0", ENG_STREND);
		}
		read_len = read(fd,buffer,sizeof(buffer));
		if(read_len <= 0){
			ENG_LOG("emmc card no read\n");
			sprintf(rsp, "%s%s", "0", ENG_STREND);
		}

		size = strtoul(buffer,&endptr,0);
		close(fd);
		ENG_LOG("sys/block/mmcblk0/size value = %f, read_len = %d ", size, read_len);
		sprintf(temp, "%s %4.2f GB", TEXT_EMMC_CAPACITY,(size/2/1024/1024));
		ENG_LOG("%s %4.2f GB", TEXT_EMMC_CAPACITY,(size/2/1024/1024));
		size=ceil(size/2/1024/1024);
		sprintf(rsp, "%d%s%s", (int)size,"G",ENG_STREND);

	}else{
		sprintf(temp,"%s%s",TEXT_EMMC_STATE,TEXT_EMMC_FAIL);

		ENG_LOG("SD card no exist\n");
		sprintf(rsp, "%s%s", "0", ENG_STREND);
	}
	return 0;
}


int get_emmcsize(void)
{
	int fd;
	int ret = 1; //fail
	int cur_row = 2;
	char  buffer[64]={0},temp[64]={0};
	int read_len = 0;
	float size = 0;
	char *endptr;
	if (0 == access("/sys/block/mmcblk0",F_OK)){
		fd = open("/sys/block/mmcblk0/size",O_RDONLY);
		if(fd < 0){
			ENG_LOG("emmc card no exist\n");
			return -1;
		}
		read_len = read(fd,buffer,sizeof(buffer));
		if(read_len <= 0){
			ENG_LOG("emmc card no read\n");
			return -1;
		}
		size = strtoul(buffer,&endptr,0);
		close(fd);
		ENG_LOG("sys/block/mmcblk0/size value = %f, read_len = %d ", size, read_len);
		ENG_LOG("%s %4.2f GB", TEXT_EMMC_CAPACITY,(size/2/1024/1024));
		size=ceil(size/2/1024/1024);
		return (int)size;
	}else{
		sprintf(temp,"%s%s",TEXT_EMMC_STATE,TEXT_EMMC_FAIL);
		return -1;
	}

}

