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

all:	binaries
	@true

include ../../makerules $(wildcard *.d)

# MODOBJS defined by makerules
binaries: $(MODOBJS) $(SRC:.cpp=.o)
	@true

clean: stdclean

.PHONY: clean all
