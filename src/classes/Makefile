# Makefile for src/classes directory

CWD = classes
SRC = MObject.cpp MApplication.cpp PathFinder.cpp Profile.cpp \
      XFace.cpp kbList.cpp upgrade.cpp MFolder.cpp
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

