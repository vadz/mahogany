# Makefile for src directory
# $Id$
#

CWD = src
SUB_DIRS = util classes gui mail adb wx
MOBJS=util/*.o classes/*.o gui/*.o  mail/*.o adb/*.o \
      wx/generic/*.o #wx/common/*.o

include ../makeopts

ifdef USE_PYTHON
SUB_DIRS += Python
MOBJS += Python/*.o
endif

all: 
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i all; done
	$(LINK) -o M $(MOBJS) $(LDFLAGS) $(LIBS)
#
#left over from my attempt to create a M.so shared lib
#
#	$(CXX) -shared $(MOBJS) $(LDFLAGS) -lswigpy $(NONPLIBS) -o libM.so
#	$(LINK) -o M -L. $(LDFLAGS) -lM $(SLIBS)

#semistatic:
#	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i all; done
#	$(LINK) -o M.semistatic $(LDFLAGS) $(MOBJS) $(SLIBS) 

static:
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i all; done
	$(LINK) -static -o M.static $(LDFLAGS) $(MOBJS) $(LIBS) -lcrypt

doc:
	$(MAKE) -C ../include $@

clean:
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done
	$(RM) core 

dep depend: 
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done

install: M
	strip M
	$(INSTALL_PROGRAM) M $(DATADIR)/$(CANONICAL_HOST)/bin
	$(RM) -f $(BINDIR)/M $(DATADIR)/bin/M
	ln -s ../share/M/$(CANONICAL_HOST)/bin/M $(BINDIR)
	$(INSTALL) -d $(DATADIR)/icons
	for i in icons/*.xpm ; do $(INSTALL_DATA) $$i $(DATADIR)/icons; done
	$(MAKE) -C Python $@

msgcat:
	set -e; for i in $(SUB_DIRS) ; do $(MAKE) -C $$i $@; done
#	$(MAKE) -C $(BUILDDIR)/locale mergecat

.PHONY: all dep clean doc semistatic static allclean install msgcat



