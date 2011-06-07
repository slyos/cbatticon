#Cbatticon makefile

cbatticon: 
	gcc cbatticon.c -o cbatticon `pkg-config --cflags --libs gtk+-2.0 libnotify` -lacpi
	
all: cbatticon	
	
clean:
	rm cbatticon

install: cbatticon
	mv cbatticon /usr/bin
	echo "installed to /usr/bin"
