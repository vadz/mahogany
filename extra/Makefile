# Makefile for extra directory
# $Id$

include ../makeopts

SUB_DIRS := src/compface
ifdef HAVE_LIBMAL
SUB_DIRS += src/libmal
endif

all: 
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i; done
	$(MAKE) -C src/c-client -f .src/Makefile.M

doc: ; @true

clean:
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done
	$(MAKE) -C src/c-client -f .src/Makefile.M $@
	$(RM) core
ifdef HAVE_LIBMAL
# make clean in libmal is incomplete:
	${RM} -r x `find src/libmal -depth -name '*.o' -o -name '*.lo' -o -name '*.a' -o -name '.libs' -print`
endif

install: 
	$(MAKE) -C install install

.PHONY: all dep clean doc static allclean install
