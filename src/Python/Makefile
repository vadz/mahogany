# Makefile for src/classes directory
#
# $Id$
#

CWD = Python

SWIGSRC = MAppBase.i MailFolder.i String.i Profile.i Message.i MObject.i
CPPSRC = InitPython.cpp PythonHelp.cpp
SRC=$(CXXSRC) *.cpp

CPPTARGETS = $(SWIGSRC:.i=.cpp) 
OBJECTS    = $(CPPTARGETS:.cpp=.o) $(CPPSRC:.cpp=.o)


all:	$(OBJECTS)

.PHONY: mkswig all

include ../../makerules 
include ../../makeopts
include .depend

install:
	$(INSTALL) -d $(DATADIR)/scripts
	$(INSTALL_DATA) *.py $(DATADIR)/scripts
	$(INSTALL_DATA) Scripts/*.py $(DATADIR)/scripts
	$(INSTALL) -d $(DOCDIR)/Python
	$(INSTALL_DATA) *.doc $(DOCDIR)/Python

clean:
	$(RM) *.o *.so *.pyc *.py *.doc $(CPPTARGETS)
	ln -s INIT/Minit.py .

#%.py: %.i
#	$(SWIG) $(SWIGFLAGS) $<
