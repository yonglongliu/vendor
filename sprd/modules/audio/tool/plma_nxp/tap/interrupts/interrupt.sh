 cl --start #start clock
 cl --pin=GPIO_INT_L # show int_l pin =1
 cl -r0xf -w0xffff   # mask all ints, enable pin, polarity=high 
 cl --pin=GPIO_INT_L # show int_l pin =0
 cl spkd=0 #umask SPK error flag
 cl -r0xf -b|grep 0x0f
 #INTP:1 INT:1 WDD:1 SPKD:0 DCCD:1 CLKD:1 OCDD:1 UVDD:1 OVDD:1 OTDD:1 VDDD:1
 cl --pin=GPIO_INT_L
 cl --iomem=0x8100 -w4 # write 1 to cf_flag_cf_speakererror
 cl --pin=GPIO_INT_L # show int_l pin =0
 cl spks # read to clear
 cl --pin=GPIO_INT_L # show int_l pin =0
 
