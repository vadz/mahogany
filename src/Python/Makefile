# Makefile for src/classes directory
#
# $Id$
#
# $Log$
# Revision 1.4  1998/05/24 08:28:58  KB
# eventually fixed the type problem, now python works as expected
#
# Revision 1.3  1998/05/19 17:02:43  KB
# several small bugfixes
#
# Revision 1.2  1998/05/02 18:31:38  KB
# Python integration works.
#
#

CWD = Python

SWIGSRC = String.i MApplication.i Profile.i 
CXXSRC = InitPython.cc
SRC=$(CXXSRC) *.cpp

CXXTARGETS= $(CXXSRC:.cc=.o)
SWIGTARGETS = $(SWIGSRC:.i=.o)
SOTARGETS	=$(OTARGETS:.o=cmodule.so)

all:	$(CXXTARGETS) $(SWIGTARGETS)

.PHONY: mkswig all

include ../../makerules 
include ../../makeopts
clean:
	$(RM) *.o *.so *.pyc *.py *.doc

