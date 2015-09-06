SRC   := stats.c
OUT   := stats.so
BUILD :=  ../../lib/modules

# * End config * #

OBJS := ${patsubst %.c,%.o,${filter %.c, ${SRC}}}

%.o: %.c
	gcc -I.. -c -fpic $<
all: ${OBJS}
	gcc -shared -o ${BUILD}/${OUT} *.o
syms:
	@objdump -e ${OUT} -TS
