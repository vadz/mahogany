# Makefile for locale directory
# $Id$
#

#
# Do a "make all" to update all .po files with latest strings from program
# sources. A simple "make" will only rebuild the .mo files, not the .po ones.
#

CWD = locale
SUB_DIRS = 
FILES_PO:=$(wildcard *.po)
FILES_MO:=$(FILES_PO:.po=.mo)

include ../makeopts

mo-files: $(FILES_MO)

clean:
	$(RM) $(FILES_MO)

dep depend: 

all:
	$(MAKE) -C .. msgcat
	$(MAKE) mergecat
	$(MAKE) mo-files


install: mo-files
	$(INSTALL) -d $(DATADIR)/locale
	for i in $(FILES_MO); do $(INSTALL) -d $(DATADIR)/locale/`basename $$i .mo`/LC_MESSAGES; $(INSTALL_DATA) $$i $(DATADIR)/locale/`basename $$i .mo`/LC_MESSAGES/M.mo; done

mergecat:
	cp ../messages.po messages.po
	cat extra_po >>messages.po
	for i in $(FILES_PO); do msgmerge $$i messages.po -o tmp.po && mv tmp.po $$i; done

.PHONY: all dep clean doc semistatic static allclean install msgcat mergecat mo-files

%.mo: %.po
	$(MSGFMT) -o $@ $<



