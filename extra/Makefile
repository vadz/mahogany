# Makefile for extra directory
# $Id$

include ../makeopts

SUB_DIRS := src/compface
ifdef HAVE_LIBMAL
SUB_DIRS += src/libmal
endif

all: 
ifeq ($(USE_MINGW),yes)
	$(MAKE) -j1 -C src/c-client -f .src/makefile.nt.gcc
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i; done
else
	$(MAKE) -j1 -C src/c-client -f .src/Makefile.M
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i; done
endif

doc: ; @true

clean:
	$(MAKE) -C src/c-client -f .src/Makefile.M $@
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done
	$(RM) core

install: 
	$(MAKE) -C install install

.PHONY: all dep clean doc static allclean install
