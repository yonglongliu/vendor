#TOOLPREFIX := ${CURDIR}/../tool-chain/gcc-arm-none-eabi-4_9-2015q2/bin/arm-none-eabi-
CC  := $(TOOLPREFIX)/${CURDIR}/../../../../../../../../prebuilts/gcc/linux-x86/x86/x86_64-linux-android-4.9/bin/x86_64-linux-android-gcc
CXX := $(TOOLPREFIX)g++
ASM := $(TOOLPREFIX)as
LD  := $(TOOLPREFIX)ld
AR  := $(TOOLPREFIX)ar
CPP := $(TOOLPREFIX)cpp
OBJCOPY := $(TOOLPREFIX)objcopy

SRC_PATH := ${CURDIR}/../../../../../../proprietories-source/sprdtrusty/external/lk/platform/sprd/crypto/sprd/r2p0/src
PAL_PATH := ${CURDIR}/../../../pal
OBJ_PATH := ${CURDIR}

INC_DIR := ${SRC_PATH}/../include ${SRC_PATH}/drivers/include

SRC_DIR := $(shell find ${SRC_PATH} -type d)
SRC_DIR += $(shell find ${PAL_PATH} -type d)
SRC_DIR := $(basename $(patsubst ./%,%,$(SRC_DIR)))

CFILES := $(foreach x, $(SRC_DIR), $(notdir $(wildcard $(addprefix $(x)/*,.c))))
OBJS := $(addprefix $(OBJ_PATH)/, $(addsuffix .o, $(basename $(CFILES))))

CFLAGS := -Wall -nostdinc -std=gnu11 -O2 -m32 -fno-short-enums -fno-stack-protector
CFLAGS += $(INC_DIR:%=-I%)

vpath %.c $(SRC_DIR)

TARGET := $(OBJ_PATH)/libsprdce.a

.PHONY: all

all : $(TARGET)

$(OBJ_PATH)/%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET) : $(OBJS)
	@echo '+++++++++++Make echo: build $(TARGET)+++++++++++'
	rm -f $@
	ar cr $@ $(OBJS)
	rm -f $(OBJS)

clean:
	rm -f $(OBJS) $(TARGET)
