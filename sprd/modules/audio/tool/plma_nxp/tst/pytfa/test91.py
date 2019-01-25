#! /usr/bin/env python
from pytfa import *
from time import *

def init():
	tfa=Tfa("hid",slave=0x36,revid=0x92)
	#tfa=Tfa("dummy91",slave=0x36,revid=0x92)
	tfa.container("/home/wim/configs/91/mono.cnt")
	#tfa=Tfa("165.114.233.9@5555")
	tfa.show()
	#tfa.verbose()
	tfa.trace(0)
	tfa.probe(tfa.slave)
	#tfa.start()
	tfa.open()
	print hex(tfa.rd(3))
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

def	dumpbf(name):
	global tfa
	print name,":",tfa.rdbf(name)
	
def start():
	global tfa	
	print "reset"
	tfa.reset()
	sleep(.1)
	tfa.start()
	dumpbf("SWPROFIL")
	dumpbf("SWVSTEP")

def checkbf(name,exp):
	global tfa	
	val = tfa.rdbf(name)
	if val!=exp:
		print name,"error, exp",exp, "actual",val
def prof(prof,v=0):
	print "prof",prof,"v",v
	#sleep(.1)
	tfa.start(prof,v)
	checkbf("SWPROFIL",prof+1)
	checkbf("SWVSTEP",v+1)
		
if __name__ == '__main__':
	tfa=init()
	#diagall()
	#diag(13)
	#tfa.trace()
	tfa.reset()
	tfa.start()
	tfa.verbose()
	start()
	checkbf("SAAMEN",0)
	checkbf("SWPROFIL",1)
	prof(0,0)
	checkbf("SAAMEN",0)
	checkbf("SWPROFIL",1)
	prof(1,0)
	checkbf("SAAMEN",1)
	checkbf("SWPROFIL",2)
	prof(0,0)
	checkbf("SAAMEN",0)
	checkbf("SWPROFIL",1)
	
	
