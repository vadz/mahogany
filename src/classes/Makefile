# Makefile for src/classes directory
# $Id$

CWD = classes
SRC = MObject.cpp MApplication.cpp PathFinder.cpp Profile.cpp \
      XFace.cpp kbList.cpp MFolder.cpp MEvent.cpp
HDR = $(SRC:.cpp=.h)
DEP = $(SRC:.cpp=.d)

TARGETS = $(SRC:.cpp=.o)

all: $(TARGETS)

CommonBase.o:

clean: stdclean

.PHONY: clean

include ../../makeopts
include ../../makerules
include .depend

