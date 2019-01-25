//------------------------------------------------------------------------------
// enable or disable local debug
#define DBG_ENABLE_DBGMSG
#define DBG_ENABLE_WRNMSG
#define DBG_ENABLE_ERRMSG
#define DBG_ENABLE_INFMSG
#define DBG_ENABLE_FUNINF
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <memory.h>

#define DDR_SIZE "/proc/sprd_dmc/property"

#include "engopt.h"
#include "ddr.h"
#include <fcntl.h>


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int  getDDRsize( char *ddr_size)
{
	char num[5]={0};
	int fd=-1; 
	memset(num, 0, 5);

	fd= open(DDR_SIZE, O_RDONLY);
	if( fd < 0 ) {
       ENG_LOG("open '%s' error\n", DDR_SIZE);
       return -1;
	}
	read(fd, ddr_size, 5);
	
	ENG_LOG(" getDDRsize(  ddr_size=%s MB \n", ddr_size);
	close(fd);
	return 0;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------