# Makefile for src/classes directory

CWD = classes
SRC = CommonBase.cc MApplication.cc appconf.cc PathFinder.cc Profile.cc MimeList.cc MimeTypes.cc \
Adb.cc XFace.cc
HDR = $(SRC:.cc=.h)
DEP = $(SRC:.cc=.d)

TARGETS = $(SRC:.cc=.o)

all: $(TARGETS)

CommonBase.o:

include ../../makeopts
include ../../makerules
