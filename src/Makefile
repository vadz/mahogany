# Makefile for src directory
# $Id$
#
# $Log$
# Revision 1.5  1998/05/02 18:31:37  KB
# Python integration works.
#
#

CWD = src
SUB_DIRS = util classes gui mail Python
MOBJS=util/*.o classes/*.o gui/*.o  mail/*.o # without Python

include ../makeopts

all: 
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done
	$(LINK) -o M util/*.o classes/*.o gui/*.o mail/*.o Python/*.o $(LIBS)
#	$(CXX) -shared $(MOBJS) -lswigpy $(NONPLIBS) -o libM.so
#	$(LINK) -o M -L. -lM $(SLIBS)

static:
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i all; done
	$(LINK) -o M.static $(LDFLAGS) util/*.o classes/*.o gui/*.o mail/*.o $(SLIBS) 

doc:
	$(MAKE) -C ../include $@

clean:
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done
	$(RM) core 

dep depend: 
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done


.PHONY: all dep clean doc static allclean



