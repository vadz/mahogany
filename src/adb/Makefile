# Makefile for src/adb directory

include ../../makeopts

CPP :=	AdbFrame.cpp AdbManager.cpp AdbProvider.cpp AdbEntry.cpp \
	AdbDialogs.cpp ProvFC.cpp ProvBbdb.cpp ProvPalm.cpp \
	AdbModule.cpp AdbImport.cpp AdbExport.cpp Collect.cpp

MOD :=	ImportText.cpp ImportXFMail.cpp ImportPine.cpp \
	ImportMailrc.cpp ImportVCard.cpp \
	ExportPalm.cpp ExportText.cpp ExportVCard.cpp

# FIXME: should be a specific value rather than wildcard
ifneq "$(filter %, $(DEBUG))" ""
CPP +=	ProvDummy.cpp
endif

# SRC contains all the files that have translation strings
SRC :=	$(CPP) $(MOD)

# indirection to pick up definitions in makerules
all:	binaries

include ../../makerules $(wildcard *.d)

# MODOBJS and MODULES defined by makerules
binaries: $(CPP:.cpp=.o) $(MODOBJS) $(MODULES)
	@true

clean:	stdclean

.PHONY:	binaries clean
