# validate lookup result
ifeq ($(KDIR),)
$(error No KDIR found for platform $(TARGET_PLATFORM))
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

kernelrelease:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) $@
