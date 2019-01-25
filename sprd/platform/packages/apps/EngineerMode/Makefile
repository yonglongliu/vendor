CC=gcc
CPP=g++
CPPFLAGS=-Wall -g
LD=g++
LDFLAGS=

COMMON_OBJS=HTTPRequest.o HTTPResponse.o
SVR_OBJS=HTTPServer.o ATProcesser.o
OBJS=$(COMMON_OBJS) $(SVR_OBJS)

all: HTTPServerMain

objs: $(OBJS)

HTTPServerMain: $(COMMON_OBJS) $(SVR_OBJS)


clean:
	rm -f *.o HTTPServerMain