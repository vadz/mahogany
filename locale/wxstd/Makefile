# Makefile for locale/wxstd directory
# $Id$
#

FILES_PO:=$(wildcard *.po)
FILES_MO:=$(FILES_PO:.po=.mo)

include ../../makeopts

wxstd-mo-files: $(FILES_MO)

clean:
	$(RM) $(FILES_MO)

dep depend: 

all:
	$(MAKE) wxstd-mo-files


.PHONY: all dep clean doc semistatic static allclean install msgcat mergecat mo-files  wxstd-mo-files

%.mo: %.po
	$(MSGFMT) -o $@ $<



