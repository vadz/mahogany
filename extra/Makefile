# Makefile for extra directory
# $Id$
#


CWD = extra
SUB_DIRS = src/compface

include ../makeopts


all: 
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done
	set -e; $(MAKE) -C src/c-client -f Makefile.M
doc:

clean:
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done
	set -e; $(MAKE) -C src/c-client -f Makefile.M $@
	$(RM) core

dep depend: 
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done
	set -e; $(MAKE) -C src/c-client -f Makefile.M $@

install: 
	$(MAKE) -C install install

.PHONY: all dep clean doc static allclean install



