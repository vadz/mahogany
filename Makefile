# Makefile for M root directory
# $Id$

CWD = 
SUB_DIRS = src include
FILES = makeopts makerules Makefile makeopts.in configure.in configure
EXTRA = extra

include makeopts

all: 
	echo $(BUILDDIR)
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done

clean:
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done

dep depend: 
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done

bak backup:
	tar cvf M-`date +"%y-%m-%d"`.tar $(SUB_DIRS) $(FILES) $(EXTRA)
	gzip -9 M-`date +"%y-%m-%d"`.tar

.PHONY: all dep clean bak backup

