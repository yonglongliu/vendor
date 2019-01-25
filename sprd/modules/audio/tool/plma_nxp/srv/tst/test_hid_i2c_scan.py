#!/usr/bin/env python

import ctypes, os, sys

if __name__ == '__main__':

    # load the shared libraries
    if os.name == 'nt':
        LDPATH=".\\"
        HALDLL=LDPATH + 'Tfa98xx_hal.dll'
        TFADLL=LDPATH + 'Tfa98xx_2.dll'
        SRVDLL=LDPATH + 'Tfa98xx_srv.dll'
    else:
        LDPATH="./"
        TFADLL = LDPATH + 'libtfa98xx.so'
        HALDLL = TFADLL
        SRVDLL = TFADLL
    
    hal = ctypes.CDLL(HALDLL)
    tfa = ctypes.CDLL(TFADLL)
    srv = ctypes.CDLL(SRVDLL)

    # lookup table for mapping register 3 values to device names
    tfa_device_names = { 
        0x12:   "TFA9887/87B/95",
        0x91:   "TFA9890B",
        0x92:   "TFA9891",
        0xb97:  "TFA9897B",
        0x80:   "TFA9890",
        0x81:   "TFA9890",
        0xa88:  "TFA9888N1A",
        0x1b88: "TFA9888N1B",
        0x2c88: "TFA9888N2C"}

    # get number of hid devices 
    nr_hid_devices = hal.lxScriboNrHidDevices()
    print "\nnumber of hid devices is %d" % nr_hid_devices

    # for all hid devices scan for devices on the i2c bus
    for i in range(0, nr_hid_devices):
        target = "hid"+ str(i)

        # register the target
        print "\nTarget:", target
        err = hal.NXP_I2C_Open(target)
        if err < 0:     # check this for device connection
            print "Target Registration failed"
            sys.exit(-1)
        else:
            print "Target Registration OK"

        # scan for i2c devices
        nr_i2c_devices = srv.tfa_diag_scan_i2c_devices()
        if nr_i2c_devices <= 0:
            print "no i2c devices found"
            continue
            
        print "\nnumber of i2c devices is %d" % nr_i2c_devices
        
        # Check contents of register 3 of all i2c devices found
        for j in range(0, nr_i2c_devices):
            i2c_addr = srv.tfa_diag_get_i2c_address(j);

            # open device
            handle = ctypes.c_int()
            pHandle = ctypes.byref(handle)
            err = tfa.tfa_probe((i2c_addr*2), pHandle)
            err = tfa.tfa98xx_open(handle)

            # read register
            data = ctypes.c_ushort()
            pData = ctypes.byref(data)         
            err = tfa.tfa98xx_read_register16(handle, 3, pData)
            if err > 0:
                print "Error reading r3: %s" % (tfa.Tfa98xx_GetErrorString(err))
            else:
                if data.value in tfa_device_names:
                    tfa_device_name = tfa_device_names[data.value]
                else:
                    tfa_device_name = "unknown"
                print "0x%x: %s (r3=0x%x)" %  (i2c_addr, tfa_device_name, data.value)

            # close device
            tfa.tfa98xx_close(handle)
        
        # Unregister hid device    
        hal.NXP_I2C_Close()

    # unload shared libaries
    del srv
    del tfa
    del hal

