
#include <stdio.h>

#include "hal_utils.h"
#include "dbgprint.h"

void hexdump(char *str, const unsigned char *data, int num_write_bytes)
{
	FILE *filetype = stdout;
	hexdump_to_file(filetype, str, data, num_write_bytes);
}

void hexdump_to_file(FILE *filetype, char *str, const unsigned char *data, int num_write_bytes)
{
	int i;

	if(str == NULL)
		str="";

	PRINT_FILE(filetype, "%s [%d]:", str, num_write_bytes);
	for(i=0;i<num_write_bytes;i++)
	{
		PRINT_FILE(filetype, "0x%02x ", data[i]);
	}
	PRINT_FILE(filetype, "\n");
	fflush(filetype);
}

int hal_add(int var1)
{
	return var1+3;
}