CC=/home/zamnedix/local/1/bin/gcc
CFLAGS=-O2 -pipe -march=native

all: 
	${CC} ${CFLAGS} -c -fPIC cdecode.c cencode.c
	${CC} ${CFLAGS} -shared *.o -o libb64.so
	rm *.o
