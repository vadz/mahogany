# Makefile for src/classes directory
# $Id$

include ../../makeopts

CWD = classes
SRC = MObject.cpp MApplication.cpp PathFinder.cpp Profile.cpp \
      XFace.cpp kbList.cpp MFolder.cpp MEvent.cpp MailCollector.cpp \
      MModule.cpp

HDR = $(SRC:.cpp=.h)
DEP = $(SRC:.cpp=.d)

TARGETS = $(SRC:.cpp=.o)

ifdef EXPERIMENTAL
TARGETS += Filters.o
endif

all: $(TARGETS)

CommonBase.o:

clean: stdclean

.PHONY: clean

include ../../makerules
include .depend

