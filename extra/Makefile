# Makefile for extra directory
# $Id$
#


CWD = extra
SUB_DIRS = src/compface

include ../makeopts


all: 
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done
	set -e; $(MAKE) -C src/c-client -f Makefile.M
ifdef HAVE_LIBMAL
	set -e; $(MAKE) -C src/libmal
endif

doc:

clean:
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done
	set -e; $(MAKE) -C src/c-client -f Makefile.M $@
	$(RM) core
ifdef HAVE_LIBMAL
# make clean in libmal is incomplete:
	set -e; $(MAKE) -C src/libmal $@
	set -e; find src/libmal -name *.o -exec $(RM) {} \;
	set -e; find src/libmal -name *.lo -exec $(RM) {} \; 2>/dev/null || true
	set -e; find src/libmal -name *.a -exec $(RM) {} \; 2>/dev/null || true
	set -e; find src/libmal -name .libs -exec $(RM) -r {} \; 2>/dev/null || true
endif

dep depend: 
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done
	set -e; $(MAKE) -C src/c-client -f Makefile.M $@

install: 
	$(MAKE) -C install install

.PHONY: all dep clean doc static allclean install



