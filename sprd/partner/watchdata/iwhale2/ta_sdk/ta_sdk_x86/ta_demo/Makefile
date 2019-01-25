include config.mk
TARGETS := ${ta_uuid}.elf
ta_name= \"$(ta_uuid)\"
#CCACHE = /usr/bin/ccache
#TOOLPREFIX := ${CURDIR}/../../toolChains/gcc-arm-none-eabi-4_9-2015q2/bin/arm-none-eabi-

SOURCE_DIR := ${CURDIR}
OUT_DIR := ${CURDIR}/../bin
INCLUDE_DIRS := ${SOURCE_DIR}/include
COMMON_DIR := ${SOURCE_DIR}/../common

# Source files required to build the target
CFILES   := $(patsubst $(SOURCE_DIR)/%,%,$(CSRC))

NK_CFLAGS += -O2

# Libraries required to build the target
LIBS := muslc panda wdossys wdos datastruct platsupport wscos gpteeapi taentry gcc

# Custom linker script
NK_LDFLAGS += -T $(COMMON_DIR)/linker.lds

ifeq ($V, 1)
	Q =
else
ifeq ($V, 2)
	Q =
else
ifeq ($V, 3)
	Q =
else
	Q = @
endif
endif
endif

CC  := $(CCACHE) $(TOOLPREFIX)gcc
CXX := $(CCACHE) $(TOOLPREFIX)g++
ASM := $(TOOLPREFIX)as
LD  := $(TOOLPREFIX)ld
AR  := $(TOOLPREFIX)ar
CPP := $(TOOLPREFIX)cpp
OBJCOPY := $(TOOLPREFIX)objcopy

# Default path configuration (useful for local development)
WDCORE_LIBDIR     ?= $(COMMON_DIR)/lib
WDCORE_INCLUDEDIR ?= $(COMMON_DIR)/include
WDISOC_INCLUDEDIR ?= $(COMMON_DIR)/include/isoc
WDCORE_BINDIR     ?= $(COMMON_DIR)/bin

# Compile configuration
INCLUDE_PATH := $(WDCORE_INCLUDEDIR) $(INCLUDE_DIRS) $(WDISOC_INCLUDEDIR)
WARNINGS     := all

ASFLAGS := -Wa

CFLAGS := -Wall -nostdinc -std=gnu11 -O2 -m32  -fno-short-enums -fno-stack-protector -DTA_NAME=$(ta_name)

ASFLAGS += $(INCLUDE_PATH:%=-I%)
CFLAGS += $(INCLUDE_PATH:%=-I%)
CPPFLAGS += $(INCLUDE_PATH:%=-I%)

STARTGROUP := -Wl,--start-group
ENDGROUP := -Wl,--end-group

LDFLAGS += $(NK_LDFLAGS) \
		   $(WDCORE_LIBDIR:%=-L%) \
		   $(STARTGROUP) \
		   $(LIBS:%=-l%) \
		   $(LIBGCC) \
		   $(ENDGROUP) \
		   $(CFLAGS) \
		   -static -nostdlib

ARCHIVES += $(LIBS:%=lib%.a)

ENTRY_POINT ?= _start

# Force start symbol to be linked in if need be - the user may already have it in a
# .o file, otherwise this will pull it from a library such as
# libsel4platsupport.
LDFLAGS += -u ${ENTRY_POINT}

# Set the entry point
LDFLAGS += -e ${ENTRY_POINT}

ASFLAGS += $(NK_ASFLAGS)

# Object files
OBJFILES = $(ASMFILES:%.S=%.o) $(CFILES:%.c=%.o) $(CXXFILES:%.cxx=%.o) $(OFILES)

# Where to look for header dependencies
vpath %.h $(INCLUDE_PATH)

# Where to look for library dependencies
vpath %.a $(WDCORE_LIBDIR)

# Where to find the sources
vpath %.c $(COMMON_DIR)/src
vpath %.cxx $(COMMON_DIR)/src
vpath %.S $(COMMON_DIR)/src

vpath %.c $(SOURCE_DIR)
vpath %.cxx $(SOURCE_DIR)
vpath %.S $(SOURCE_DIR)

# Default is to build/install all targets
default: $(TARGETS)
	mv $(TARGETS) $(ta_uuid).bin
	${COMMON_DIR}/../tools/secta $(ta_uuid).bin	
	mv $(ta_uuid).secta $(OUT_DIR)
	rm $(ta_uuid).bin
	
#
# For each ".bin" or ".a" target, we also setup a rule to copy it into a final
# binaries output directory. For binary files we want to remove the .bin suffix
#
#default: $(patsubst %.bin,$(WDCORE_BINDIR)/%,$(filter %.bin,$(TARGETS)))

%.o: %.c $(HFILES)
	@echo " [CC] $@"
	$(Q)$(CC) -x c $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# Note: Create a separate rule ARCHIVES to clearly indicate they
# the ARCHIVES must be built before doing any other steps. Previously
# when ARCHIVES was a prerequisite on the line below make thought
# it was local to this directory and would make the archives using
# src/main.obj.
%.elf: $(ARCHIVES)

# Note: below the CC line does not have ARCHIVES because
# LDFLAGS already includes the "-l" version of ARCHIVES
%.elf: $(OBJFILES)
	@echo " [LINK] $@"
	$(Q)$(CC) $(OBJFILES) $(LDFLAGS) -o $@
clean:
	@echo " [CLEAN] $@"
	$(Q)rm -rf ${TARGETS} ${OBJFILES}
