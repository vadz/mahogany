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

# Need SRC for translation processing
SRC :=	$(patsubst .src/%,%,$(wildcard .src/*.cpp))
MOD :=	$(filter-out PalmOS.cpp, $(SRC))

ifdef PISOCK_LIB
MOD +=	PalmOS.cpp
ifdef LIBMAL_SRC
PalmOS.o PalmOS.so: CPPFLAGS += -DMALSYNC -DHAVE_LIBMAL -I$(LIBMAL_SRC) 
endif
PalmOS.so: SO_LIBS += $(PISOCK_LIB)
endif

# indirection to pick up definitions in makerules
all:	binaries

include ../../makerules $(wildcard *.d)

# MODOBJS and MODULES defined by makerules
binaries: $(MODOBJS) $(MODULES)
	@true

clean:	stdclean
	rm -f *.exe

# strangely, we need optimization on in order to compile successfully
%.exe: %.cpp
	$(CXX) -o $@ $(CPPFLAGS) -DTEST $(CXXFLAGS) -g3 -O1 $< $(LDFLAGS) $(GUI_LIBS) $(LIBS)

%.test: %.exe
	./$< < .src/$*.tests

tests: Filters.test

.PHONY:	binaries clean tests
