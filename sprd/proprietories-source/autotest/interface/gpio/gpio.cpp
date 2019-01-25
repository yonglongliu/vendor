#include <cutils/log.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <cutils/misc.h>
#include <string.h>
#include <cstdlib>

#include "autotest.h"
//------------------------------------------------------------------------------
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


int Getgpio_value(char *pin_addr,int gpio_num)
{
    int ret = 0;

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

/*
GPIO PC-->engpc:	7E 4C 07 00 00 0D 00 38 07 01 xx 00 xx xx 7E	GPO output "0"	" GPIO输出0或1 红色字段xx代表gpio号码 ,蓝色字段xx xx 代表软件偏移地址"
	 engpc-->pc:    7E 00 00 00 00 08 00 38 00 7E
	 PC-->engpc:    7E 4C 07 00 00 0D 00 38 07 01 xx 01 xx xx 7E	GPO output "1"
	 engpc-->pc:    7E 00 00 00 00 08 00 38 00 7E
	 PC-->engpc:    7E 4C 07 00 00 0D 00 38 07 00 xx yy xx xx 7E	GPI in	"红色字段xx代表gpio号码 ,蓝色字段xx xx 代表软件偏移地址，命令返回GPI读取到的状态         yy 0：输入下拉 1：输入上拉"
	 engpc-->pc:    7E 00 00 00 00 09 00 38 00  xx 7E 		xx代表读到的GPIO值，为01或00。
**/
int testGpio(char *buf, int len, char *rsp, int rsplen)
{
    int ret = 0;  //用于表示具体测试函数返回值，
	/****************************************************************
    ret <0,表示函数返回失败。
	ret =0,表示测试OK，无需返回数据。
	ret >0,表示测试OK，获取ret个数据，此时，ret的大小表示data中保存的数据的个数。
	***********************************************************************/

	char data[100] = {0 }; //用于保存获取到的数据。

	FUN_ENTER ; //打印函数提名字：testGpio ++

	//打印PC下发的数据。
	LOGD("pc->engpc:%d number--> %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x",len,buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7],buf[8],buf[9],buf[10],buf[11],buf[12],buf[13],buf[14]); //78 xx xx xx xx _ _38 _ _ 打印返回的10个数据	

	//如下为功能代码
    cam_sd_tcard_addvoltage();
    char pin_addr[15] = {0};
    int  gpio_num = buf[10];
    int  value = buf[11];
    const char *pinconfig = "pin_config";
    sprintf(pin_addr, "0x%x", (buf[12]<<8) | buf[13]);
    LOGD("Setgpio  value  pin_addr = %s!!!gpio_num= %d, value = %d\n  ", pin_addr,gpio_num, value);
	switch(buf[9])
	{
	case 1:
       ret = Setgpio_value(pin_addr, gpio_num, value);
	   break;
	case 0:
	   ret = Getgpio_value(pin_addr,gpio_num);
	  break;
    default:
        break;
	}
    LOGD("ret = %d!!!\n  ", ret);

/*------------------------------后续代码为通用代码，所有模块可以直接复制，------------------------------------------*/

	//填充协议头7E xx xx xx xx 08 00 38
	MSG_HEAD_T *msg_head_ptr;
	memcpy(rsp, buf, 1 + sizeof(MSG_HEAD_T)-1);  //复制7E xx xx xx xx 08 00 38至rsp,，减1是因为返回值中不包含MSG_HEAD_T的subtype（38后面的数据即为subtype）
	msg_head_ptr = (MSG_HEAD_T *)(rsp + 1);

	//修改返回数据的长度，如上复制的数据长度等于PC发送给engpc的数据长度，现在需要改成engpc返回给pc的数据长度。
	msg_head_ptr->len = 8; //返回的最基本数据格式：7E xx xx xx xx 08 00 38 xx 7E，去掉头和尾的7E，共8个数据。如果需要返回数据,则直接在38 XX后面补充


	//填充成功或失败标志位rsp[1 + sizeof(MSG_HEAD_T)]，如果ret>0,则继续填充返回的数据。
	LOGD("msg_head_ptr,ret=%d",ret);

	if(ret<0)
	{
	rsp[sizeof(MSG_HEAD_T)] = 1;    //38 01 表示测试失败。
	}else if (ret==0){
	rsp[sizeof(MSG_HEAD_T)] = 0;    //38 00 表示测试ok
	}else if(ret >0)
	{
	rsp[sizeof(MSG_HEAD_T)] = 0;    //38 00 表示测试ok,ret >0,表示需要返回 ret个字节数据。
    memcpy(rsp + 1 + sizeof(MSG_HEAD_T), data, ret);	  //将获取到的ret个数据，复制到38 00 后面
	msg_head_ptr->len+=ret;   //返回长度：基本数据长度8+获取到的ret个字节数据长度。
	}
	LOGD("rsp[1 + sizeof(MSG_HEAD_T):%d]:%d",sizeof(MSG_HEAD_T),rsp[sizeof(MSG_HEAD_T)]);
	//填充协议尾部的7E
    rsp[msg_head_ptr->len + 2 - 1]=0x7E;  //加上数据尾标志
    LOGD("dylib test :return len:%d",msg_head_ptr->len + 2);
	LOGD("engpc->pc:%x %x %x %x %x %x %x %x %x %x",rsp[0],rsp[1],rsp[2],rsp[3],rsp[4],rsp[5],rsp[6],rsp[7],rsp[8],rsp[9]); //78 xx xx xx xx _ _38 _ _ 打印返回的10个数据
	FUN_EXIT ; //打印函数体名字
   return msg_head_ptr->len + 2;
/*------------------------------如上虚线之间代码为通用代码，直接赋值即可---------*/

}

extern "C" void register_this_module_ext(struct eng_callback *reg, int *num)
{
int moudles_num = 0;   //用于表示注册的节点的数目。
LOGD("register_this_module :dllibtest");

    //1st command
   reg->type = 0x38;  //38对应BBAT测试
   reg->subtype = 0x07;   //0x07对应BBAT中的GPIO测试。
   reg->eng_diag_func = testGpio; //testGpio对应BBAT中GPIO测试功能函数，见上。
   moudles_num++;

   /*************
   //2st command
   reg->type = 0x39;  //39对应nativemmi测试
   reg->subtype = 0x07;   //0x07对应nativemmi中的GPIO测试。
   reg->eng_diag_func = testnativeGpio;
   moudles_num++;
   **********************/

   *num = moudles_num;
LOGD("register_this_module_ext: %d - %d",*num, moudles_num);
}



