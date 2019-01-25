from pytfa import *

dev=NXP_I2C('hid')

dev.regRead8(0x6,3)
dev.regRead16(0x34,3)
dev.regRead16(0x35,3)
print hex(dev.regRead16(0x36,3))
print hex(dev.regRead16(0x37,3))

