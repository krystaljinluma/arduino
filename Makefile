all: read_usb.o server

read_usb.o: read_usb.c
	clang -c read_usb.c
server: read_usb.o server.c
	clang server.c read_usb.o -o server

clean:
	rm -rf *.o
clobber: clean
	rm -rf read_usb.o server