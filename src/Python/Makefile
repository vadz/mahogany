# Makefile for src/Python directory
# $Id$

include ../../makeopts

CPPFLAGS:=$(CPPFLAGS) -DSWIG_GLOBAL
CXXFLAGS:=$(CXXFLAGS_RELAXED)

SWIGSRC = MAppBase.i MailFolder.i MString.i MProfile.i Message.i\
	MObject.i #SendMessageCC.i
CPPSRC = InitPython.cpp PythonHelp.cpp

CPPTARGETS := $(SWIGSRC:.i=.cpp) swiglib.cpp
PYTARGETS  := $(SWIGSRC:.i=.py)

OBJECTS    := $(CPPTARGETS:.cpp=.o) $(CPPSRC:.cpp=.o)

all:	$(OBJECTS) $(PYTARGETS)
	@true

include ../../makerules $(wildcard *.d)

ifdef USE_SWIG
swiglib.cpp: swiglib.i
	$(SWIG) $(SWIGFLAGS_LIBSWIG) -o $@ $<
endif

install:
	$(INSTALL) -d $(DATADIR)/scripts
	for i in *.py .src/Scripts/*.py; \
	do $(INSTALL_DATA) $$i $(DATADIR)/scripts; \
	done
	$(INSTALL) -d $(DOCDIR)/Python
	touch dummy.html
	for i in *.html; \
	do $(INSTALL_DATA) $$i $(DOCDIR)/Python; \
	done
	$(RM) $(DATADIR)/scripts/dummy.py $(DOCDIR)/Python/dummy.html

clean:	stdclean
	$(RM) *.pyc *.html *.doc junk $(CPPTARGETS) Minit.py
	#??? ln -s INIT/Minit.py .

.PHONY: mkswig all install clean
