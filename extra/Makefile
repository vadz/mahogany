# Makefile for extra directory
# $Id$
#


CWD = extra
SUB_DIRS = src/c-client src/compface

include ../makeopts


all: 
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done

doc:

clean:
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done
	$(RM) core

dep depend: 
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done

install: 
	$(MAKE) -C install install

.PHONY: all dep clean doc static allclean install



