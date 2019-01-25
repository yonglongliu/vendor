# validate lookup result
ifeq ($(KDIR),)
$(warning No KDIR found for platform $(TARGET_PLATFORM))
endif

ifeq ($(BUILD),debug)
ccflags-y += -DDEBUG
endif

obj-m += sprdbt_tty.o
sprdbt_tty-objs := tty.o \
                   rfkill.o \
                   lpm.o \
                   alignment/sitm.o

modules:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) M=$(CURDIR) $@

clean:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) M=$(CURDIR) $@


clean:
	rm -f *.o  .*.o.cmd modules.order Module.symvers *.ko *.mod.c .*.ko.cmd
	rm -rf .tmp_versions
	rm -f alignment/*.o alignment/.*.o.cmd


kernelrelease:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) $@
