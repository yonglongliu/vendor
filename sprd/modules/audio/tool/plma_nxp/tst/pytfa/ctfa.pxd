#tfa.pxd
cpdef enum tfa_error:
        tfa_error_ok, #/**< no error */
        tfa_error_device, #/**< no response from device */
        tfa_error_bad_param, #/**< parameter no accepted */
        tfa_error_noclock, #/**< required clock not present */
        tfa_error_timeout, #/**< a timeout occurred */
        tfa_error_dsp, #/**< a DSP error was returned */
        tfa_error_container, #/**< no or wrong container file */
        tfa_error_max #/**< impossible value, max enum */
 
cpdef enum Tfa98xx_Error :
    Tfa98xx_Error_Ok = 0,
    Tfa98xx_Error_Device,        #/*Currently only used to keep in sync with tfa_error */
    Tfa98xx_Error_Bad_Parameter,
    Tfa98xx_Error_Fail,             #/*generic failure, avoid mislead message */
    Tfa98xx_Error_NoClock,          #/*no clock detected */
    Tfa98xx_Error_StateTimedOut,
    Tfa98xx_Error_DSP_not_running,    #/*communication with the DSP failed */
    Tfa98xx_Error_AmpOn,            #/*amp is still running */
    Tfa98xx_Error_NotOpen,            #/*the given handle is not open */
    Tfa98xx_Error_InUse,    #/*too many handles */
    #/*the expected response did not occur within the expected time */
    Tfa98xx_Error_RpcBase = 100,
    Tfa98xx_Error_RpcBusy = 101,
    Tfa98xx_Error_RpcModId = 102,
    Tfa98xx_Error_RpcParamId = 103,
    Tfa98xx_Error_RpcInfoId = 104,
    Tfa98xx_Error_RpcNotAllowedSpeaker = 105,
 
    Tfa98xx_Error_Not_Implemented,
    Tfa98xx_Error_Not_Supported,
    Tfa98xx_Error_I2C_Fatal,    #/*Fatal I2C error occurred */
    #/*Nonfatal I2C error, and retry count reached */
    Tfa98xx_Error_I2C_NonFatal,
    Tfa98xx_Error_Other = 1000
ctypedef int Tfa98xx_handle_t
ctypedef int NXP_I2C_Error_t
ctypedef unsigned short uint16_t
ctypedef unsigned char uint8_t
ctypedef uint8_t nxpTfaContainer_t
# cpdef enum tfa_error:
#          tfa_error_ok=1 #/**< no error */   
              
cdef extern from "app/climax/inc/climax.h":
    int climain(int argc, char *argv[])
                  
cdef extern from "tfa/inc/tfa.h":
    tfa_error tfa_probe(unsigned char slave_address, int *pDevice)
    tfa_error tfa_start(int next_profile, int *vstep)
    
cdef extern from "tfa/inc/tfa_service.h":
    Tfa98xx_Error tfa98xx_open(Tfa98xx_handle_t handle)
    Tfa98xx_Error tfa98xx_close(Tfa98xx_handle_t handle)
    unsigned short tfaContBfEnum(const char *name, int family, unsigned short rev)
    char *tfaContBfName(uint16_t num, unsigned short rev)
    #tfaContGetSlave(int devn, uint8_t *slave)
    void tfa_cnt_verbose(int level)
    int tfa_set_bf(Tfa98xx_handle_t dev_idx, const uint16_t bf, const uint16_t value)
    int tfa_get_bf(Tfa98xx_handle_t dev_idx, const uint16_t bf)
    Tfa98xx_Error tfa98xx_read_register16(Tfa98xx_handle_t handle,
                           unsigned char subaddress,
                           unsigned short *pValue)
    Tfa98xx_Error tfa98xx_write_register16(Tfa98xx_handle_t handle,
                        unsigned char subaddress,
                        unsigned short value)
    Tfa98xx_Error tfa98xx_dsp_read_mem(Tfa98xx_handle_t handle,
                       unsigned int start_offset,
                       int num_words, int *pValues)
    Tfa98xx_Error tfa98xx_dsp_write_mem_word(Tfa98xx_handle_t handle,
                        unsigned short address, 
                                        int value, int memtype)
    Tfa98xx_Error tfa98xx_read_data(Tfa98xx_handle_t handle,
                     unsigned char subaddress,
                     int num_bytes, unsigned char data[])
    Tfa98xx_Error tfa98xx_write_data(Tfa98xx_handle_t handle,
                      unsigned char subaddress,
                      int num_bytes,
                      const unsigned char data[])
    Tfa98xx_Error tfa98xx_write_raw(Tfa98xx_handle_t handle,
                          int num_bytes,
                          const unsigned char data[])
    
cdef extern from "hal/inc/NXP_I2C.h":
    void NXP_I2C_Trace(int on)
    void NXP_I2C_Trace_file(char *filename)
    void NXP_I2C_rev(int *major, int *minor, int *revision)
    NXP_I2C_Error_t NXP_I2C_Version(char *data)
    int NXP_I2C_GetPin(int pin)
    int NXP_I2C_Open(char *targetname)
    int NXP_I2C_Close()
    NXP_I2C_Error_t NXP_I2C_SetPin(int pin, int value)
    int NXP_I2C_BufferSize()
    NXP_I2C_Error_t NXP_I2C_WriteRead(  unsigned char sla,
                int num_write_bytes,
                const unsigned char write_data[],
                int num_read_bytes,
                unsigned char read_buffer[] )


    
# cdef extern from "hal/inc/lxScribo.h":
#   #  ctypedef struct Tfa:
#    #     pass
#     int lxScriboRegister(char *dev)
    
cdef extern from "srv/inc/tfaContainerWrite.h":
    int tfa98xx_cnt_loadfile(char *filename, int cnt_verbose)
    void tfa_probe_all(nxpTfaContainer_t* p_cnt)
    nxpTfaContainer_t * tfa98xx_get_cnt()
    
    