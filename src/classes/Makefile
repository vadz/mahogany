# Makefile for src/classes directory

CWD = classes
SRC = CommonBase.cpp MObject.cpp MApplication.cpp appconf.cpp PathFinder.cpp Profile.cpp MimeList.cpp MimeTypes.cpp \
Adb.cpp XFace.cpp kbList.cpp # Script.cpp
HDR = $(SRC:.cpp=.h)
DEP = $(SRC:.cpp=.d)

TARGETS = $(SRC:.cpp=.o)

all: $(TARGETS)

CommonBase.o:

include ../../makeopts
include ../../makerules
