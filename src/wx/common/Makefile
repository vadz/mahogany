# $Id$
# Makefile2 for src/wx directory and all its subdirectories
# Called by Makefile which handles the clean target in a non standard way.

# If you change the number of subdirectories, make sure you modify Makefile, too!

CWD = wx
SRC = pprocess.cpp phone.cpp vcard.cpp
HDR = $(SRC:.cpp=.h)
DEP = $(SRC:.cpp=.d)

TARGETS = $(SRC:.cpp=.o)

all: $(TARGETS)

clean: stdclean

.PHONY: clean all

include ../../../makeopts
include ../../../makerules
include .depend

