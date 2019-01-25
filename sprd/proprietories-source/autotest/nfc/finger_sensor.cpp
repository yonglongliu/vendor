#include "type.h"
#include "finger_sensor.h"
#include <hardware/hardware.h> 
#include <hardware/fingerprint.h> 

#include <dlfcn.h>
int finger_sensor_test_sharkl2()
{
    ALOGE("start finger_sensor_test_sharkl2");
    int err;
	static const uint16_t kVersion = HARDWARE_MODULE_API_VERSION(2, 0);
	fingerprint_module_t const* mModule;
	fingerprint_device_t* mDevice;
	
    const hw_module_t *hw_module = NULL;
    if (0 != (err = hw_get_module(FINGERPRINT_HARDWARE_MODULE_ID, &hw_module))) {
        ALOGE("Can't open fingerprint HW Module, error: %d", err);
        return 1;
    }
    if (NULL == hw_module) {
        ALOGE("No valid fingerprint module");
        return 1;
    }

    mModule = reinterpret_cast<const fingerprint_module_t*>(hw_module);

    if (mModule->common.methods->open == NULL) {
        ALOGE("No valid open method");
        return 1;
    }

    hw_device_t *device = NULL;

    if (0 != (err = mModule->common.methods->open(hw_module, NULL, &device))) { //open success :0
        ALOGE("Can't open fingerprint methods, error: %d", err);
        return 1;
    }

    if (kVersion != device->version) {
        ALOGE("Wrong fp version. Expected %d, got %d", kVersion, device->version);
         return 1; // 
    }

    mDevice = reinterpret_cast<fingerprint_device_t*>(device);



    if (0 != (err = mDevice->common.close(reinterpret_cast<hw_device_t*>(mDevice)))) {//closehal
        ALOGE("Can't close fingerprint module, error: %d", err);
        return 1;
    }
  ALOGE("end finger_sensor_test_sharkl2");
	return 0;
}
#define LIB_FINGERSOR_PATH "/system/lib/libfactorylib.so"
int finger_sensor_test_whale2()
{    int test_flag=1;  
      void *sprd_handle_fingersor_dl;
         typedef int (*spi_test_dl)();
            sprd_handle_fingersor_dl = dlopen(LIB_FINGERSOR_PATH, RTLD_NOW);
            if(sprd_handle_fingersor_dl == NULL){
                INFMSG("fingersor lib dlopen failed!\n");
                test_flag = 0x01;
            }else{
                spi_test_dl spi_test = (spi_test_dl)dlsym(sprd_handle_fingersor_dl, "spi_test");
                if(spi_test != NULL && spi_test() >=0) {
                    INFMSG("test fingerprint sensor test sucess !!!\n");
                    test_flag = 0x00;  //PASS
                } else {
                    INFMSG("test fingerprint sensor test fail !!!\n");
                    test_flag = 0x01;
                }
                dlclose(sprd_handle_fingersor_dl);
            }
			
			return test_flag;
}
