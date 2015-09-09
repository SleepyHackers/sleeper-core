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

# Directory tree DB location
DB_FILE := sleeper.db

# the name of the project - essentially, the executable sans capital letters
PROJECT := $(shell echo $(BINARY) | tr A-Z a-z)

# Directories of modules using the 2.0 system
MODULES2 := dice party stats

# Top level directory of python
PYTHON_TOP := /usr/include

# Python version
PYTHON_VERSION := python3.4

# Where .o files are built
OBJ_DIR := build

# Where dependency files are built
DEPENDS_DIR := depends

# Where .c files are
SRC_DIR := src

# Where .h files are
INCLUDE_DIR := include

# Flags passed to C compiler
C_FLAGS := -Wall -g -ggdb -I${PYTHON_TOP}/${PYTHON_VERSION} -I${INCLUDE_DIR}

# Libraries and link flags
LIBS    := -lz -lpthread -lcrypt -lm -ldl -lutil -L/usr/lib/${PYTHON_VERSION}/config -l${PYTHON_VERSION} -Wl,--export-dynamic

# SleeperMUD core source files
# Extensions to the game should be done as modules, not here!
SRC     := gameloop.c mud.c utils.c interpret.c handler.c inform.c      \
	   action.c save.c socket.c io.c strings.c event.c races.c      \
	   log.c auxiliary.c world.c character.c room.c exit.c          \
	   extra_descs.c object.c body.c zone.c room_reset.c            \
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
           time.c

#Object files
O_FILES := ${foreach src,${SRC},${OBJ_DIR}/${patsubst %.c,%.o,${notdir ${src}}}}

#Dependency files
DEPS := $(patsubst %.c,%.d, $(SRC))
DEPS := ${foreach dep,${DEPS},${DEPENDS_DIR}/${notdir ${dep}}}

-include ${DEPS}

#Calculate path of each source file
#SRC := ${foreach src,${SRC},${SRC_DIR}/${src}}

################################################################################
# make commands
################################################################################
${OBJ_DIR}/%.o:  ${SRC_DIR}/%.c
	${CC} ${C_FLAGS} -c $<
#	${eval SRCFILE=${ls */${patsubst %.o,%.c,${notdir $@}}}}
#	${info SRCFILE ${SRCFILE}}
#	${eval SRCFILE=${locate -d ${DB_FILE} "${patsubst %.o,%.c,${notdir $@}}"}}
#ifeq (${SRCFILE},)
#	@${info "Couldn't find source file for $@"}
#	@exit 0
#endif
#	mv ${patsubst %.c,%.o,$<} ${OBJ_DIR}

${DEPENDS_DIR}/%.d: ${SRC_DIR}/%.c
	${CC} ${C_FLAGS} -MM -MF $@ -MT ${OBJ_DIR}/${patsubst %.c,%.o,${notdir $<}} -c $<

all: ${DEPS} ${O_FILES}
	$(CC) -o $(BINARY) ${OBJ_DIR}/*.o $(LIBS)
	@echo -e "$(BINARY) successfully compiled."\
		 "To run your mud, use ./$(BINARY) [port] &\n"\

modules:
	@${foreach module,${MODULES2},/bin/bash -c "cd ${module} && make && cd ..";}

#For compiling as module
as-module:
	${CC} ${C_FLAGS} -fPIC -c ${SRC} -o depends
	mv *.o ${OBJ_DIR}
	${CC} ${LIBS} -shared -o ../lib/modules/${MODULE} ${O_FILES}
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

# calculate all of our dependencies. It's messy, but it works
#%.d: %.c
#	@echo "Building dependencies for $(patsubst %.d,%.c, $@)"
#	@case $(shell dirname $@) in					 \
#	"" | ".") 							 \
#	  gcc $(C_FLAGS) -MM $(patsubst %.d,%.c, $@) | 			 \
#	    sed -e 's@^\(.*\)\.o:@\1.d \1.o:@' > $@			 \
#	;; 								 \
#	*) 								 \
#	  gcc $(C_FLAGS) -MM $(patsubst %.d,%.c, $@) | 			 \
#	    sed -e "s@^\(.*\)\.o:@$(shell dirname $@)/\1.d $(shell dirname $@)/\1.o:@" > $
#					 			 \
#      ;;                                                               \
#	esac
