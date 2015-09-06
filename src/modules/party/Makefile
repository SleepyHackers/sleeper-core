SRC   := party.c follow.c
OUT   := party.so
ROOT  := ../../..

# * End config * #

OBJS := ${patsubst %.c,%.o,${filter %.c, ${SRC}}}
OUT := ${ROOT}/lib/modules/${OUT}

%.o: %.c
	gcc -I../.. -c -fpic $<
all: ${OBJS}
	gcc -shared -o ${OUT} *.o
syms:
	@objdump -e ${OUT} -TS

