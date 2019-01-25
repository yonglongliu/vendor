#! /usr/bin/env python

from pytfa import *
import os

dev=NXP_I2C()
print "Version:",dev.Version()[:-1] #strip \n
print "BufferSize:",dev.BufferSize()
dev.Trace()
tracefile="testi2c.txt"
try:
    os.remove(tracefile)
except OSError:
    pass
#dev.Trace_file(tracefile)
dev.WriteRead(0x34, '0x71 0x12 0x34')
print dev.WriteRead(0x34, '0x71',2)
print dev.WriteRead('0x68 0x71 0x34 0x21',2)
print dev.WriteRead(0x34, '0x71',2)
dev.WriteRead('I2C w [  4]: 0x68 0x71 0xaa 0x55')
dev.WriteRead('I2C W [  2]: 0x68 0x71')
print "read",dev.WriteRead('I2C R [  3]: 0x69 0xaa 0x55')
print dev.WriteRead(0x34, '0x71',2)
dev.WriteRead(0x34, 0x71, 0x5678)
print dev.WriteRead(0x34, 0x71,None,2)
#print "revid",dev.WriteRead(0x34,)
#pins
for i in range(5):
    print "pin",i,';',dev.GetPin(i)
dev.SetPin(1,1)
dev.SetPin(2,1)
dev.SetPin(3,1)
for i in range(5):
    print "pin",i,';',dev.GetPin(i)
dev.Close()
f=open(tracefile,'r')
print f.read()
f.close()



