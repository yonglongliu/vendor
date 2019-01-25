#include <cutils/log.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <cutils/misc.h>
#include <string.h>
#include <cstdlib>

#include "diag.h"
#include "autotest_modules.h"
#include "../debug.h"
#include "gpio_addr_index.h"

#define DEV_PIN_CONFIG_PATH1     "/sys/kernel/debug/pinctrl/e42a0000.pinctrl/pins_debug"
#define DEV_PIN_CONFIG_PATH2    "/sys/kernel/debug/pinctrl/402a0000.pinctrl/pins_debug"
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//====================================================================
//add the setgpio function set enable or disable gpio
//step one : pin offset exchange the gpio function
//step two : check the gpio struct path
//step three : exchange the gpio function to export mode
//step four: operation the gpio function disable or enable
//step five : get the reslut the operation of the gpio
//====================================================================
int Setgpio_value(char *pin_addr,int gpio_num,int value)
{
    char cmd[255] = {0};
    int ret = 2;
    char gpio_path[255] = {0};
    char export_cmd[255] = {0};
    char out_cmd[255] = {0};
    char setval_cmd[255] = {0};
    int fd= -1;
    char *resp = 0;

    LOGD("Setgpio_value---- the pin_addr = %s, gpio_num = %d, value = %d\n ", pin_addr, gpio_num, value );
    if(0 == access(DEV_PIN_CONFIG_PATH1, F_OK)){
	sprintf(cmd, "echo %s > %s", pin_addr,DEV_PIN_CONFIG_PATH1);
    }else if(0 == access(DEV_PIN_CONFIG_PATH2, F_OK)){
	sprintf(cmd, "echo %s > %s", pin_addr,DEV_PIN_CONFIG_PATH2);
	}else{
	LOGD("cann't find the  e42a0000 and  402a0000 path\n");
	return -1 ;
	}
    system(cmd);
    LOGD("the Setgpio_value = %s\n", cmd);
    usleep(500);
    //add the gpio_num list path
    if(0 != access("/sys/class/gpio/export", F_OK)){
        LOGD("the path can`t find %s\n", gpio_path);
        return  -1;
    }

    sprintf(export_cmd, "echo %d > /sys/class/gpio/export", gpio_num);
    //modify the gpio function set export function
    system(export_cmd);
    usleep(200);

    //moidify the gpionum list set out direction function
    char gpio_out[255] = {0};
    sprintf(gpio_out, "/sys/class/gpio/gpio%d/direction", gpio_num);
    if(0 != access(gpio_out, F_OK)){
        LOGD("the path can`t find %s\n", gpio_out);
        return  -1;
    }
    sprintf(out_cmd, "echo out > %s", gpio_out);
    system(out_cmd);
    usleep(100);

    LOGD(" Setgpio_value = %s\n", out_cmd);
    //moidify the gpionum list set out direction function
    char gpio_setval_path[255] = {0};
    sprintf(gpio_setval_path, "/sys/class/gpio/gpio%d/value", gpio_num);
    if(0 != access(gpio_setval_path, F_OK)){
        LOGD("the path can`t find %s\n", gpio_setval_path);
        return  -1;
    }
    sprintf(setval_cmd, "echo %d > %s", value, gpio_setval_path);
    system(setval_cmd);
    usleep(100);
    char gpio_enable_status[3] = {0};
    if (0 == access(gpio_setval_path , F_OK)){
        fd = open(gpio_setval_path, O_RDONLY);
        if(fd < 0){
            LOGD("the %s--- open fail file %s\n", __func__, gpio_setval_path);
            return ret;
        }else{
            memset(gpio_enable_status, 0, sizeof(gpio_enable_status));
            fd = read(fd , gpio_enable_status, 3);
            if(fd < 0){
                LOGD("the %s--- read fail file %s\n", __func__, gpio_setval_path);
                return ret;
            }
            gpio_enable_status[2] = '\0';
            //DBGMSG("the  gpio_enable_status value---%s\n", gpio_enable_status);
            ret = atoi(gpio_enable_status);
            LOGD("the gpio_enable_status=value %d \n",ret);
        }
    }else{
        LOGD("the %s---filepath can`t find  %s\n", __func__, gpio_setval_path);
    }
    usleep(200);
    return 0;//ret;
}


int Getgpio_value(char *pin_addr,int gpio_num,int value[0])
{
    int ret = -1;
	int fd = -1;
	char cmd[255] = {0};
	char export_cmd[255] = {0};
    char out_cmd[255] = {0};
	char setval_cmd[255] = {0};
    char gpio_setval_path[255] = {0};
	char gpio_enable_status[3] = {0};
	LOGD(" gpio_num= %d\n", gpio_num);
	LOGD("Getgpio_value---- the pin_addr = %s, gpio_num = %d,\n ", pin_addr, gpio_num);
    if(0 == access(DEV_PIN_CONFIG_PATH1, F_OK)){
	sprintf(cmd, "echo %s > %s", pin_addr,DEV_PIN_CONFIG_PATH1);
    }else if(0 == access(DEV_PIN_CONFIG_PATH2, F_OK)){
	sprintf(cmd, "echo %s > %s", pin_addr,DEV_PIN_CONFIG_PATH2);
	}else{
	LOGD("cann't find the  e42a0000 and  402a0000 path\n");
	return -1 ;
	}
    system(cmd);
    LOGD("the Getgpio_value = %s\n", cmd);
    usleep(500);
    //add the gpio_num list path
    if(0 != access("/sys/class/gpio/export", F_OK)){
        LOGD("the path can`t find %s\n", "/sys/class/gpio/export");
        return  -1;
    }
    sprintf(export_cmd, "echo %d > /sys/class/gpio/export", gpio_num);
    //modify the gpio function set export function
    system(export_cmd);
    usleep(200);

    //moidify the gpionum list set out direction function
    char gpio_out[255] = {0};
    sprintf(gpio_out, "/sys/class/gpio/gpio%d/direction", gpio_num);
    if(0 != access(gpio_out, F_OK)){
        LOGD("the path can`t find %s\n", gpio_out);
        return  -1;
    }
    sprintf(out_cmd, "echo in > %s", gpio_out);
	LOGD(" Getgpio_value = %s\n", out_cmd);
    system(out_cmd);
    usleep(100);

    sprintf(gpio_setval_path, "/sys/class/gpio/gpio%d/value", gpio_num);
    if (0 == access(gpio_setval_path , F_OK)){
        fd = open(gpio_setval_path, O_RDONLY);
        if(fd < 0){
            LOGD("the %s--- open fail file %s\n", __func__, gpio_setval_path);
            return ret;
        }else{
            memset(gpio_enable_status, 0, sizeof(gpio_enable_status));
            fd = read(fd , gpio_enable_status, 3);
            if(fd < 0){
                LOGD("the %s--- read fail file %s\n", __func__, gpio_setval_path);
                return ret;
            }
            gpio_enable_status[2] = '\0';
            //DBGMSG("the  gpio_enable_status value---%s\n", gpio_enable_status);
            value[0] = atoi(gpio_enable_status);
			ret = 0;
            LOGD("atoi(gpio_enable_status)=%d,value[0]=%d\n",atoi(gpio_enable_status),value[0]);
        }
    }else{
        LOGD("the %s---filepath can`t find  %s\n", __func__, gpio_setval_path);
    }
    return ret;
}
//----------------------------------------------------------------------------
void  cam_sd_tcard_addvoltage(void)
{
    system("echo 1 > /sys/kernel/debug/sprd-regulator/vddcamio/enable");
    usleep(50);
    system("echo 1 > /sys/kernel/debug/sprd-regulator/vddsdcore/enable");
    usleep(50);
    system("echo 1 > /sys/kernel/debug/sprd-regulator/vddsdio/enable");
    usleep(50);
    system("echo 1 > /sys/kernel/debug/sprd-regulator/vddsd/enable");
    usleep(50);
    system("echo 1 > /sys/kernel/debug/sprd-regulator/vddsim0/enable");
    usleep(50);
    system("echo 1 > /sys/kernel/debug/sprd-regulator/vddsim1/enable");
    usleep(50);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

static int testGpio(const uchar * data, int data_len, uchar * rsp, int rsp_size)
{
    int ret = 0;
	int ret1 = 0;
	int gpiovalue[0];
    LOGD("data[0] = %d\n", *data);
    //as not the gpio drv
    cam_sd_tcard_addvoltage();
	int gpio_num=data[1];
    char pin_addr[15] ={0} ;
    int  value = data[2];
    const char *pinconfig = "pin_config";
    sprintf(pin_addr, "0x%x", gpionumber[gpio_num].bias_address);
    LOGD("pin_addr = %s!!!gpio_num= %d, value = %d\n  ", pin_addr,gpio_num, value);
    if(1 == data[0])
        ret = Setgpio_value(pin_addr, gpio_num, value);
    else if(0 == data[0])
	{
		ret1 = Getgpio_value(pin_addr,gpio_num,gpiovalue);
		if(ret1 < 0)
		ret = -1;
		else{
		ret = 1;
		rsp[0] = gpiovalue[0];
		}
	LOGD("Getgpio_value gpiovalue[0]=%d\n  ",gpiovalue[0]);
	}
	LOGD("ret=%d\n  ",ret);
    return ret;
}

extern "C"
void register_this_module(struct autotest_callback * reg)
{
   LOGD("file:%s, func:%s\n", __FILE__, __func__);
   reg->diag_ap_cmd = DIAG_CMD_GPIO;
   reg->autotest_func = testGpio;
}
