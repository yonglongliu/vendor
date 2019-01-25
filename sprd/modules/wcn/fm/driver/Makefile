# validate lookup result
ifeq ($(KDIR),)
$(error No KDIR found for platform $(TARGET_PLATFORM))
endif

ifeq ($(BUILD),debug)
ccflags-y += -DDEBUG
endif

obj-m += sprd_fm.o
sprd_fm-objs := fmdrv_main.o \
                fmdrv_ops.o \
                fmdrv_rds_parser.o

modules:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) M=$(CURDIR) $@

clean:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) M=$(CURDIR) $@

kernelrelease:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) $@
