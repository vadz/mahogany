# Makefile for src/adb directory

CWD = adb
SRC = AdbFrame.cpp AdbManager.cpp AdbProvider.cpp AdbEntry.cpp \
      AdbDialogs.cpp \
      ProvFC.cpp ProvBbdb.cpp AdbImport.cpp \
      ImportText.cpp ImportXFMail.cpp ImportMailrc.cpp ImportPine.cpp

ifdef DEBUG
SRC += ProvDummy.cpp
endif

HDR = $(SRC:.cpp=.h)
DEP = $(SRC:.cpp=.d)

TARGETS = $(SRC:.cpp=.o)

all: $(TARGETS)

clean: stdclean

.PHONY: clean all

include ../../makeopts
include ../../makerules
include .depend
