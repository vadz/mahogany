# Makefile for src directory
# $Id$

CWD = src
SUB_DIRS = util classes gui mail

include ../makeopts

all: 
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done
	$(LINK) -o M util/*.o classes/*.o gui/*.o mail/*.o $(LIBS)

static:
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i all; done
	$(LINK) -o M.static $(LDFLAGS) util/*.o classes/*.o gui/*.o mail/*.o $(SLIBS) 

doc:
	$(MAKE) -C ../include $@

clean:
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done
	$(RM) core set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done

dep depend: 
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done


.PHONY: all dep clean doc static



