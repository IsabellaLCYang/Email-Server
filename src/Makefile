CC=gcc
#
# If you want to enable the "standard" server behaviour of forking a process
# to handle each incoming socket connection, then define the symbol DOFORK
# using the following line. 
# CFLAGS=-g -Wall -std=gnu11 -DDOFORK
CFLAGS=-g -Wall -std=gnu11

all: mysmtpd mypopd 

mysmtpd: mysmtpd.o netbuffer.o mailuser.o server.o
	gcc $(CLFAGS) mysmtpd.o netbuffer.o mailuser.o server.o   -o mysmtpd

mypopd: mypopd.o netbuffer.o mailuser.o server.o
	gcc $(CFLAGS) mypopd.o netbuffer.o mailuser.o server.o   -o mypopd

mysmtpd.o: mysmtpd.c netbuffer.h mailuser.h server.h
mypopd.o: mypopd.c netbuffer.h mailuser.h server.h
netbuffer.o: netbuffer.c netbuffer.h
mailuser.o: mailuser.c mailuser.h
server.o: server.c server.h

clean:
	-rm -rf mysmtpd mypopd mysmtpd.o mypopd.o netbuffer.o mailuser.o server.o
tidy: clean
	-rm -rf *~ 
