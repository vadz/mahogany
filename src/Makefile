# Makefile for src directory
# $Id$
#

CWD = src
SUB_DIRS = util classes gui mail adb
MOBJS=util/*.o classes/*.o gui/*.o  mail/*.o adb/*.o

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

install: all
	strip M
	$(INSTALL_PROGRAM) M $(DATADIR)/$(CANONICAL_HOST)/bin
	ln -fs $(DATADIR)/$(CANONICAL_HOST)/bin/M $(BINDIR)
	ln -fs $(DATADIR)/$(CANONICAL_HOST)/bin/M $(DATADIR)/bin
	$(INSTALL) -d $(DATADIR)/icons
	$(INSTALL_DATA) `find icons/* -prune -not -type d` $(DATADIR)/icons

.PHONY: all dep clean doc semistatic static allclean install



