# Makefile for src/wx directory
# $Id$

SUBDIRS=generic common vcard

all clean msgcat:
	set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i $@; done

.PHONY: clean all msgcat
