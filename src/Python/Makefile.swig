###############################################################################
# Project:     Mahogany
# File name:   src/Python/Makefile
# Purpose:     helper Makefile for generating *.*-swig files from *.i ones
# Author:      Vadim Zeitlin
# Modified by:
# Created:     1998
# CVS-ID:      $Id$
# Copyright:   (c) 1998-2004 M-Team
# Licence:     M license
###############################################################################

# this makefile should be used from the source (and not build) directory, it
# is used to update the precompiled *-swig files which are put in the cvs and
# are used during the build if swig itself is not available

SWIG=swig
SWIGFLAGS := -c++ -python -shadow

IFACES_DIR := ../../include/interface
IFACES := $(filter-out swigcmn.i,$(notdir $(wildcard $(IFACES_DIR)/*.i)))

vpath %.i $(IFACES_DIR)

%.cpp %.py: %.i
	$(SWIG) -I../../include -I$(IFACES_DIR) $(SWIGFLAGS) -o $*.cpp $<

%.cpp-swig: %.cpp
	sed -e 's/"Python.h"/"MPython.h"/' $< > $@

%.py-swig: %.py
	mv -f $< $@

all: swig

swig: $(IFACES:.i=.cpp-swig) $(IFACES:.i=.py-swig)

swigclean:
	rm -f *.*-swig

.INTERMEDIATE: $(IFACES:.i=.py)

.PHONY: all swig swigclean
