# Makefile for src/classes directory
#
# $Id$
#

CWD = Python

SWIGSRC = MAppBase.i MailFolder.i String.i Profile.i Message.i
CXXSRC = InitPython.cpp PythonHelp.cpp
SRC=$(CXXSRC) *.cpp

CXXTARGETS= $(CXXSRC:.cpp=.o)
SWIGTARGETS = $(SWIGSRC:.i=.o) $(SWIGSRC:.i=.py)
SOTARGETS	=$(OTARGETS:.o=cmodule.so)

all:	$(CXXTARGETS) $(SWIGTARGETS)

.PHONY: mkswig all

include ../../makerules 
include ../../makeopts
clean:
	$(RM) *.o *.so *.pyc *.py *.doc
	ln -s INIT/Minit.py .

#%.py: %.i
#	$(SWIG) $(SWIGFLAGS) $<
