# Makefile for src/adb directory

CWD = adb
SRC = AdbFrame.cpp AdbManager.cpp AdbProvider.cpp AdbEntry.cpp \
      AdbDialogs.cpp \
      ProvFC.cpp ProvBbdb.cpp ProvPalm.cpp \
      AdbModule.cpp AdbImport.cpp AdbExport.cpp

#MOD = ImportText.cpp ImportXFMail.cpp ImportPine.cpp ImportMailrc.cpp
MOD = ImportXFMail.cpp ImportPine.cpp ImportMailrc.cpp ImportVCard.cpp \
      ExportPalm.cpp ExportText.cpp


ifdef DEBUG
SRC += ProvDummy.cpp
endif

HDR = $(SRC:.cpp=.h)
DEP = $(SRC:.cpp=.d)

TARGETS = $(SRC:.cpp=.o) $(MODOBJS)

all: $(TARGETS)

clean: stdclean

.PHONY: clean all

include ../../makeopts
include ../../makerules
include .depend
