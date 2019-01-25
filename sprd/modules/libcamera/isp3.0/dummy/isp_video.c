#include <sys/types.h>
#include "isp_type.h"

void *ispvideo_GetIspHandle(void)
{
	return NULL;
}

void send_img_data(uint32_t format, uint32_t width, uint32_t height,
		   char *imgptr, int imagelen)
{
	UNUSED(format);
	UNUSED(width);
	UNUSED(height);
	UNUSED(imgptr);
	UNUSED(imagelen);
}

void send_capture_complete_msg()
{
}

void send_capture_data(uint32_t format, uint32_t width, uint32_t height,
		       char *ch0_ptr, int ch0_len, char *ch1_ptr, int ch1_len,
		       char *ch2_ptr, int ch2_len)
{
UNUSED(format);
UNUSED(width);
UNUSED(height);
UNUSED(ch0_ptr);
UNUSED(ch0_len);
UNUSED(ch1_ptr);
UNUSED(ch1_len);
UNUSED(ch2_ptr);
UNUSED(ch2_len);
}

int ispvideo_RegCameraFunc(uint32_t cmd, int (*func) (uint32_t, uint32_t))
{
UNUSED(cmd);
UNUSED(func);
	return 0;
}

void stopispserver()
{
}

void startispserver()
{
}

void setispserver(void *handle)
{
UNUSED(handle);
}
