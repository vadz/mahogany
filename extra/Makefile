# Makefile for extra directory
# $Id$
#
# $Log$
# Revision 1.2  1998/07/05 12:20:03  KB
# wxMessageView works and handles mime (segfault on deletion)
# wsIconManager loads files
# install target
#
# Revision 1.1  1998/05/18 18:00:52  KB
# new makefile for extra sources
#
#

CWD = extra
SUB_DIRS = src/c-client src/compface

include ../makeopts


all: 
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done

doc:

clean:
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done
	$(RM) core src/OSTYPE

dep depend: 
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done

install: all
	$(MAKE) -C install install

.PHONY: all dep clean doc static allclean install



