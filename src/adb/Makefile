# Makefile for src/adb directory

CWD = adb
SRC = AdbFrame.cpp AdbManager.cpp AdbProvider.cpp AdbEntry.cpp \
      AdbDialogs.cpp \
      ProvFC.cpp ProvBbdb.cpp ProvPalm.cpp \
      AdbImport.cpp AdbExport.cpp \
      ImportText.cpp ImportXFMail.cpp ImportMailrc.cpp ImportPine.cpp \
      ExportPalm.cpp ExportText.cpp 

MOD = ModulesSample.cpp

ifdef DEBUG
SRC += ProvDummy.cpp
endif

HDR = $(SRC:.cpp=.h)
DEP = $(SRC:.cpp=.d)

TARGETS = $(SRC:.cpp=.o) $(MODOBJS)

all: $(TARGETS) modules

clean: stdclean

.PHONY: clean all

include ../../makeopts
include ../../makerules
include .depend
