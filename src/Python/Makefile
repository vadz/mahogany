# Makefile for src/classes directory
#
# $Id$
#
# $Log$
# Revision 1.6  1998/05/24 14:47:33  KB
# lots of progress on Python, but cannot call functions yet
# kbList fixes again?
#
# Revision 1.5  1998/05/24 07:54:14  KB
# works?
#
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

SWIGSRC = MApplication.i MailFolder.i String.i Profile.i 
CXXSRC = InitPython.cc PythonHelp.cc
SRC=$(CXXSRC) *.cpp

CXXTARGETS= $(CXXSRC:.cc=.o)
SWIGTARGETS = $(SWIGSRC:.i=.o) $(SWIGSRC:.i=.py)
SOTARGETS	=$(OTARGETS:.o=cmodule.so)

all:	$(CXXTARGETS) $(SWIGTARGETS)

.PHONY: mkswig all

include ../../makerules 
include ../../makeopts
clean:
	$(RM) *.o *.so *.pyc *.py *.doc
	ln -s INIT/Minit.py .

