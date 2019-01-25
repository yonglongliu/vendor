CC=gcc -g
CFLAGS = -g 
# uncomment this for SunOS
# LIBS = -lsocket -lnsl

all: server client

server:  udp_sockets.c 
	$(CC) -o server  udp_sockets.c  $(LIBS) -DSERVER
client:  udp_sockets.c 
	$(CC) -o client  udp_sockets.c  $(LIBS) -USERVER



clean:
	rm -f server  server.o client client.o
