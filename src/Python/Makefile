# Makefile for src/Python directory
# $Id$

include ../../makeopts

CPPFLAGS := $(CPPFLAGS) -DSWIG_GLOBAL
CXXFLAGS := $(CXXFLAGS_RELAXED)

SWIGSRC	:= MAppBase.i MailFolder.i MString.i MProfile.i Message.i\
	MObject.i #SendMessageCC.i
SRC	:= InitPython.cpp PythonHelp.cpp

PYTARGETS  := $(SWIGSRC:.i=.py)
CPPTARGETS := $(SWIGSRC:.i=.cpp) swiglib.cpp

OBJECTS    := $(CPPTARGETS:.cpp=.o) $(SRC:.cpp=.o)

all:	$(PYTARGETS) $(OBJECTS)
	@true

include ../../makerules $(wildcard *.d)

ifdef SWIG
swiglib.cpp: swiglib.i
	$(SWIG) $(SWIGFLAGS_LIBSWIG) -o $@ $<
endif

Minit.py: .src/INIT/Minit.py
	ln -s $< $@

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
	$(RM) *.pyc *.html *.doc junk Minit.py
	$(RM) $(CPPTARGETS)
	$(RM) $(PYTARGETS)

.PHONY: mkswig all install clean
