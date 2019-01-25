

ccflags-y+=-Werror
ccflags-y+=-I$(M)/include/arm_common
ccflags-y+=-I$(M)/include/wd_tee
ccflags-y+=-I$(M)/include
ccflags-y+=-I$(M)/core

obj-m := wdtee_armtz.o

wdtee_armtz-objs:=   \
		armtz/tee_tz_drv.o \
		armtz/tee_mem.o \
		armtz/handle.o \
		armtz/wdtee_log.o \
		core/tee_core.o  \
		core/tee_context.o  \
		core/tee_session.o  \
		core/tee_shm.o  \
		core/tee_supp_com.o  \
		core/tee_sysfs.o \
		core/tee_debugfs.o \
		core/tee_kernel_api.o \
		core/tee_mutex_wait.o \
		core/tee_wait_queue.o 


