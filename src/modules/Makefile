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

TARGETS = $(SRC:.cpp=.so)

all: $(TARGETS)

clean: 
	$(RM) *.o *.so *.lo

install:
	$(INSTALL) -d $(MODULEDIR)
	$(INSTALL_PROGRAM) $(TARGETS) $(MODULEDIR)

.PHONY: clean

include ../../makerules
include .depend

