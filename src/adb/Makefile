# Makefile for src/adb directory

SRC = AdbFrame.cpp AdbManager.cpp AdbProvider.cpp AdbEntry.cpp \
      AdbDialogs.cpp ProvFC.cpp ProvBbdb.cpp ProvPalm.cpp \
      AdbModule.cpp AdbImport.cpp AdbExport.cpp

MOD = ImportText.cpp ImportXFMail.cpp ImportPine.cpp \
      ImportMailrc.cpp ImportVCard.cpp \
      ExportPalm.cpp ExportText.cpp ExportVCard.cpp

ifdef DEBUG
SRC += ProvDummy.cpp
endif

include ../../makeopts

ifeq ($(USE_MODULES),static)
TARGETS = $(MOD:.cpp=.o) $(SRC:.cpp=.o)
else
ifeq ($(USE_MODULES),dynamic)
TARGETS = $(MOD:.cpp=.so) $(SRC:.cpp=.o)
else
TARGETS = $(SRC:.cpp=.o)
endif
endif

all: $(TARGETS)
	@true

include ../../makerules $(wildcard *.d)

clean: stdclean

.PHONY: clean all
