# Makefile for src/modules directory
# $Id$
#
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
#
#######################################################################

include ../../makeopts

CWD = modules
SRC = Mdummy.cpp Filters.cpp Calendar.cpp

ifdef PISOCK_LIB
SRC := $(SRC) PalmOS.cpp
# for MAL:
CPPFLAGS := $(CPPFLAGS) -I$(BUILDDIR)/extra/src/libmal/malsync_src/mal/client/common -I$(BUILDDIR)/extra/src/libmal/malsync_src/mal/common
PISOCK_LIB := $(PISOCK_LIB) -L $(BUILDDIR)/extra/src/libmal -lmal
endif

HDR = $(SRC:.cpp=.h)
DEP = $(SRC:.cpp=.d)

ifeq ($(USE_MODULES),static)
TARGETS = $(SRC:.cpp=.o)
else
ifeq ($(USE_MODULES),dynamic)
TARGETS = $(SRC:.cpp=.so) #$(SRC:.cpp=.mmd)
else
TARGETS=
endif
endif


# I hate doing this, but for now it's necessary as the pi-sock headers
# aren't fully C++ compliant on RH6.1
CXXFLAGS:=$(CXXFLAGS_RELAXED)

INTERFACE = ../../include/MInterface.h ../../include/MInterface.cpp

all: $(TARGETS)

modules: all

$(TARGETS): $(INTERFACE)

$(INTERFACE): ../../include/MInterface.mid
	$(MAKE) -C ../../include MInterface.h

ifdef PISOCK_LIB
PalmOS.so: PalmOS.cpp
	$(CXX) -shared -fPIC $(CXXFLAGS) -o $@ $< -L$(BUILDDIR)/src $(PISOCK_LIB) 

endif

clean: 
	$(RM) *.o *.so *.lo

install: $(TARGETS)
ifeq ($(USE_MODULES),dynamic)
	$(INSTALL) -d $(MODULEDIR)
	for i in $(TARGETS); do $(INSTALL_PROGRAM) $$i $(MODULEDIR); done
endif

install-modules: install

.PHONY: clean

include ../../makerules
include .depend

