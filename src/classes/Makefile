# Makefile for src/classes directory

CWD = classes
SRC = MObject.cpp MApplication.cpp PathFinder.cpp Profile.cpp MimeList.cpp MimeTypes.cpp \
XFace.cpp kbList.cpp # Script.cpp
HDR = $(SRC:.cpp=.h)
DEP = $(SRC:.cpp=.d)

TARGETS = $(SRC:.cpp=.o)

all: $(TARGETS)

CommonBase.o:

include ../../makeopts
include ../../makerules
include .depend