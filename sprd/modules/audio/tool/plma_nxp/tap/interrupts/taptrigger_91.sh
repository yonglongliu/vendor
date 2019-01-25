#!/bin/bash

if [ "$(uname -m)" == "armv7l" ]; then
  # claim gpio
  if [ ! -f /sys/class/gpio/gpio60/value ] ;then
    echo 60 > /sys/class/gpio/export
  fi

  alias tapirq='cat /sys/class/gpio/gpio60/value' # show int_l pin =0
  alias cl='climax -dsysfs -l mono_tap.cnt'
else
  alias cl='climax -dhid -l mono_tap.cnt --slave=0x36'
  alias tapirq='cl --pin=GPIO_INT_L' # show int_l pin =0
  alias tone16k='AUDIODEV=plughw:CARD=Maximus,DEV=0 sox -r16000 -q -n -d synth .1 sine 200'
fi

alias tapread='cl --xmem=271'
alias taploop='while true;do tapread;sleep .4;done'
#interrupt
alias tapack='cl --xmem=290 -w0'
alias tapgetirq='tapirq;tapread;tapack;cl SPKS;cl SPKS;tapirq'

#tap_det_patterns = [ "OO", "OOO", "O-O-O", "O.O-O", "O-O.O", "O-OO-0", "OOO-0", "O-O-O0", "OO-O0" ]
cl PWDN=1
sleep 0.1

cl -r0x04 -w0x381B  # 16KHz fs, I2Sout
cl -r0x22 -w0x0027  # PGA gain 25dB
cl -r0x11 -w0x0010  # Enable speaker as a mic
cl -r0x70 -w0x0001  # Put the coolflux core under reset
cl -r0x09 -w0x8A4C  # Release power down and system reset, use bck as pll ref clk
cl -r0x0A -w0x3ED2  # Select the CF out 1 as the I2S data source. Same data L&R
# start pll
cl PWDN=0
sleep 0.1
# setup interrupt reg
cl -r0xf -w0xffff   # mask all ints, enable pin, polarity=high 
cl spkd=0 #umask SPK error flag
cl SPKS # clear sticky if set
# 
cl -p TapLiteIP48.patch #-b # load tap patch
cl -p tap_setup.patch #ACS=1, SBSL=1, RST=0
#
taploop

