# Makefile for src/modules directory
# $Id$
#
# To create a new module for Mahogany, simply start from Mdummy.cpp
# and modify it. Save your Module's source in this directory and run
# make. The Makefile will find it automatically, there is no need to
# edit this file.
# (KB)
#
# Don't forget to add a .mmd (Mahogany Module Definition) file, which
# must contain the following lines:
# Mahogany-Module-Definition
# Name: Modulename
# Version: any kind of version id
# Author: who wrote it
# <empty line>
# Any descriptive text...
#
#######################################################################

include ../../makeopts

# I hate doing this, but for now it's necessary as the pi-sock headers
# aren't fully C++ compliant on RH6.1
CXXFLAGS:=$(CXXFLAGS_RELAXED)

MOD :=	Mdummy.cpp Filters.cpp Calendar.cpp \
	PineImport.cpp XFMailImport.cpp

ifdef PISOCK_LIB
MOD += PalmOS.cpp
ifdef LIBMAL_SRC
CPPFLAGS := $(CPPFLAGS) -DMALSYNC -DHAVE_LIBMAL -I$(LIBMAL_SRC)/malsync_src/mal/client/common -I$(LIBMAL_SRC)/malsync_src/mal/common -I$(LIBMAL_SRC) 
endif
endif

ifdef EXPERIMENTAL
MOD +=	MCrypt.cpp MExtProcCrypt.cpp MPGPCrypt.cpp MPassphrase.cpp
vpath %.cpp .src/crypt
endif

all:	binaries
	@true

include ../../makerules $(wildcard *.d)

# MODOBJS and MODULES defined by makerules
binaries: $(MODOBJS $(MODULES)
	@true

ifdef PISOCK_LIB
PalmOS.so: PalmOS.cpp
	$(CXX) -shared -fPIC $(CXXFLAGS) -o $@ $< -L$(BUILDDIR)/src $(PISOCK_LIB) 
endif

clean: 
	$(RM) *.o *.so *.lo

install: $(MODULES)
ifeq ($(USE_MODULES),dynamic)
	$(INSTALL) -d $(MODULEDIR)
	for i in $(MODULES); do $(INSTALL_PROGRAM) $$i $(MODULEDIR); done
else
	@true
endif

.PHONY: clean
