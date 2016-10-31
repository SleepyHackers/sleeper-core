###############################################################################
# Makefile for SleeperMUD
# The original NakedMud Makefile was rather confusing and relied on a complicated
# system to compile the "mandatory modules". We've ditched NakedMud's module system
# for a dynamic linking one. See modules.c
###############################################################################

# compiler to use
CC = gcc

# the name of the binary file created that you will run to start your mud
BINARY  := sleeper

# the name of the SO created when compiling SleeperMUD as a module
MODULE := sleeper.so

# the name of the project - essentially, the executable sans capital letters
PROJECT := $(shell echo $(BINARY) | tr A-Z a-z)

# Module root directory
MODULE_DIR := modules

# Directories of modules using the 2.0 system
MODULES2 := dice party stats websockets webui_proto webui_mud

# Path to python config utility
PYTHON_CONFIG := python2.7-config

# Where .o files are built
OBJ_DIR := build

# Where dependency files are built
DEPENDS_DIR := depends

# Where .c files are
SRC_DIR := src

# Where .h files are
INCLUDE_DIR := include

# Threads used in compiler parallelization of loops
NUM_THREADS := 2

# Flags passed to C compiler

C_FLAGS := -Wall -g -ggdb -fPIC -I${INCLUDE_DIR} -O2 -pipe -march=native
C_FLAGS := ${C_FLAGS} ${shell ${PYTHON_CONFIG} --cflags} -Wno-strict-prototypes -Wno-unused-value -Wno-unused-but-set-variable 

# Libraries and link flags
LIBS := -lz -lpthread -lcrypt -lssl -lcrypto -lwebsockets -ldl -D THPOOL_DEBUG -pthread -Wl,--export-dynamic
LIBS += ${shell ${PYTHON_CONFIG} --ldflags}

# SleeperMUD core source files
# Extensions to the game should be done as modules, not here!
SRC     := gameloop.c mud.c utils.c interpret.c handler.c inform.c      \
	   action.c save.c socket.c io.c strings.c event.c races.c      \
	   log.c auxiliary.c world.c character.c room.c exit.c          \
	   extra_descs.c object.c body.c zone.c room_reset.c           \
           account.c list.c property_table.c hashtable.c map.c          \
           storage.c set.c buffer.c bitvector.c numbers.c               \
           prototype.c hooks.c parse.c near_map.c command.c             \
           filebuf.c modules.c alias.c dyn_vars.c editor.c notepad.c    \
           help.c hedit.c items.c portal.c iedit.c container.c          \
           furniture.c worn.c olc.c redit.c zedit.c esedit.c medit.c    \
           oedit.c bedit.c accedit.c pcedit.c protedit.c olc_extender.c \
	   scripts.c pychar.c pyobj.c pymud.c pymudsys.c pyhooks.c      \
           pyroom.c pyexit.c pyaccount.c pysocket.c script_editor.c     \
	   pyplugs.c pyevent.c pystorage.c pyauxiliary.c triggers.c     \
	   trigedit.c trighooks.c pyolc.c set_val.c socials.c socedit.c \
           time.c websocket.c lws-http.c thpool.c


#Object files
O_FILES := ${foreach src,${SRC},${OBJ_DIR}/${patsubst %.c,%.o,${src}}}

#Dependency files
DEPS := $(patsubst %.c,%.d, $(SRC))
DEPS := ${foreach dep,${DEPS},${DEPENDS_DIR}/${notdir ${dep}}}

################################################################################
# make commands
################################################################################

.PHONY: all modules as-module syms clean

all: ${DEPS} ${O_FILES}
	@echo Linking ${BINARY}
	@$(CC) -o $(BINARY) ${OBJ_DIR}/*.o $(LIBS)
	@echo -e "$(BINARY) successfully compiled."\
		 "To run your mud, use ./$(BINARY) [port] &\n"\

-include ${DEPS}

${DEPENDS_DIR}/%.d: ${SRC_DIR}/%.c
	@echo Building dependencies for $<
	@${CC} ${C_FLAGS} -MM -MF $@ -MT ${OBJ_DIR}/${patsubst %.c,%.o,${notdir $<}} -c $<

${OBJ_DIR}/%.o: ${SRC_DIR}/%.c
	@echo Compiling $<
	@${CC} ${C_FLAGS} -c $<
	@mv ${notdir $@} ${OBJ_DIR}

modules:
	@${foreach module,${MODULES2},/bin/bash -c "cd ${MODULE_DIR}/${module} && CC=\"${CC}\" CFLAGS=\"${CFLAGS}\" LIBS=\"${LIBS}\" make && cd ..";}

#For compiling as module
as-module: ${DEPS} ${O_FILES}
	@echo Linking ${MODULE}
	@${CC} ${LIBS} -shared -o ../lib/modules/${MODULE} ${O_FILES}
	@echo "SleeperMUD built as module"

syms:
	@objdump -e ../lib/modules/${MODULE} -TS
# * #

# clear all of the .o files and all of the save files that emacs makes. Also
# clears all of our Python files
clean:
	@rm -f $(BINARY)
	@rm -f ${MODULE}
	@rm -f ${OBJ_DIR}/*.o *.o ${DEPENDS_DIR}/*.d *~ */*~
	@rm -f ../lib/pymodules/*~ ../lib/pymodules/*.pyc
	@echo "$(PROJECT) source files cleaned"
