#! /usr/bin/env python
from pytfa import *
from time import *

def init(slave=0x36):
	tfa=Tfa("hid",slave,revid=0x92)
	tfa.container("/home/wim/configs/91/stereo.cnt")
	#tfa=Tfa("165.114.233.9@5555")
	tfa.show()
	#tfa.verbose()
	tfa.trace(0)
	tfa.probe(tfa.slave)
	#tfa.start()
	tfa.open()
	id =tfa.rd(3)
	if  id== 0x92:
		print "it's a tfa9892"
	else:
		print "not a tfa9892 0x%x",id 
	return tfa

def diagall():
	global tfa
	for test in range(1,12):
		err=tfa.diag(test)
		if err!=0:
			print "error:",err," in test",test,":",tfa.errorstring
			break
	print "------skipped test 12, beacuse of (often) strange speaker readbacks"
def diag(test):
	global tfa
	err=tfa.diag(test)
	if err!=0:
		print "error:",err," in test",test,":",tfa.errorstring

def unlock():
	global tfa
	#tfa.wrbf("PWDN",0) not enough for mtp?
	#sleep(.1)
	#tfa.start()
	tfa.wr(0x40, 0x5a6b) #UnlockUnhide
	tfa.wr(0xb, 0x5a)	 #Unlock key2
	xor=tfa.rd(0x8b)^0x5A
	tfa.wr(0x60, xor)	 #Unlock key1
def dumpMTP():
	global tfa
	for i in range(0x80,0x8f+1):
		print "MTP 0x%02x:0x%04x"%(i, tfa.rd(i))
def setSAAM():
	global tfa
	"""set hw features mtp 0x85 bit14 and 15 0xc000 """
	unlock()
	tfa.wr(0x85,0x8c00)
	tfa.wrbf("CIMTP",1)
	sleep(.1)
	print hex(tfa.rd(0x85))
				 	
if __name__ == '__main__':
	tfa=init()
# 	diagall()
# 	diag(3)
	setSAAM()
	dumpMTP()
	tfa=init(0x37)
# 	diagall()
# 	diag(3)
	setSAAM()
	dumpMTP()
	
