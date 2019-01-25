PLMA_HOSTSW:=../..

CFLAGS=-g  -I$(PLMA_HOSTSW)/hal/inc
LDLIBS=-L$(PLMA_HOSTSW) -ltfa98xx

hal_test: hal_test.c hal_registers.c hal_registers.h

clean:
	rm -f *.o hal_test

h:	# show export command
	@echo export LD_LIBRARY_PATH=$(abspath $(PLMA_HOSTSW))

