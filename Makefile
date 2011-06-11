#Cbatticon makefile
DESTDIR = /usr/bin
cbatticon: 
	gcc cbatticon.c -o cbatticon `pkg-config --cflags --libs gtk+-2.0 libnotify` -lacpi
	
all: cbatticon	
	
clean:
	rm cbatticon

install: cbatticon
	mv cbatticon ${DESTDIR}
	
