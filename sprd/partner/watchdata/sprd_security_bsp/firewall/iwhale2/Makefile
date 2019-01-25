CC := ${CURDIR}/../../../../../../../prebuilts/gcc/linux-x86/x86/x86_64-linux-android-4.9/bin/x86_64-linux-android-gcc
SPRD_DIR := ${CURDIR}/../../../../../proprietories-source/sprdtrusty/external/lk/platform/sprd/firewall/iwhale2

SRC_DIR := ${SPRD_DIR}/src
INC_DIR := ${SPRD_DIR}/inc
LIB_OUT := ${CURDIR}

INCLUDE_PATH := $(INC_DIR)
CFILES := $(wildcard $(SRC_DIR)/*.c)

OBJS_ALL := $(CFILES:.c=.o)

OBJS := $(OBJS_ALL)

CFLAGS  := -Wall -nostdinc -std=gnu11 -O2 -m32 -fno-short-enums -fno-stack-protector

CFLAGS += $(INCLUDE_PATH:%=-I%)

TARGET := $(LIB_OUT)/sprd_firewall.a

all : $(TARGET)

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET) : $(OBJS)
	rm -f $@
	ar cr $@ $(OBJS)
	rm -f $(OBJS)

clean:
	rm -f $(OBJS) $(TARGET)
