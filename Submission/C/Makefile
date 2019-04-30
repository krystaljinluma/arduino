CC = clang
ARGS = -Wall -std=c99 -g -pthread

all: read_usb server main

read_usb: read_usb.c
	$(CC) -c $(ARGS) read_usb.c

server: server.c
	$(CC) -c $(ARGS) server.c
	
main: read_usb.o server.o main.c
	$(CC) -o main $(ARGS) read_usb.o server.o main.c

clean: 
	rm -rf read_usb server main *.o

clobber: clean
	rm -rf read_usb.o server.o main