#!/bin/bash

if [ "$(uname -m)" == "armv7l" ]; then
    # claim gpio
    if [ ! -f /sys/class/gpio/gpio60/value ] ;then
        echo 60 > /sys/class/gpio/export
    fi

    alias cl='climax -dsysfs -l mono_tap.cnt'
    alias tapirq='echo -n "GPIO_INT_L=" ; cat /sys/class/gpio/gpio60/value' # show int_l pin =0
else
    alias cl='climax -dhid -l mono_tap.cnt --slave=0x36'
    alias tapirq='cl --pin=GPIO_INT_L' # show int_l pin =0
fi

alias tapread='cl --xmem=271'
alias taploop='while true;do tapread;sleep .4;done'

#interrupt
alias tapack='cl --xmem=290 -w0'
alias tapgetirq='tapirq;tapread;tapack;cl SPKS;cl SPKS;tapirq'

#tap_det_patterns = [ "OO", "OOO", "O-O-O", "O.O-O", "O-O.O", "O-OO-0", "OOO-0", "O-O-O0", "OO-O0" ]

cl --start -Pswval
cl --start -Ptaptrigger

#taploop
echo "Check interrupt with tapgetirq or poll xmem by using taploop"