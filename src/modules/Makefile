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
##
##  DO NOT EDIT THIS MAKEFILE - JUST PLACE YOUR SOURCE IN THE DIRECTORY
##
#######################################################################

include ../../makeopts

CWD = modules
SRC = $(wildcard *.cpp)

HDR = $(SRC:.cpp=.h)
DEP = $(SRC:.cpp=.d)

TARGETS = $(SRC:.cpp=.so) $(SRC:.cpp=.mmd)

INTERFACE = ../../include/MInterface.h ../../include/MInterface.cpp

all: $(TARGETS)

$(TARGETS): $(INTERFACE)

$(INTERFACE): ../../include/MInterface.mid
	$(MAKE) -C ../../include MInterface.h

clean: 
	$(RM) *.o *.so *.lo

install:
	$(INSTALL) -d $(MODULEDIR)
	$(INSTALL_PROGRAM) $(TARGETS) $(MODULEDIR)

.PHONY: clean

include ../../makerules
include .depend

