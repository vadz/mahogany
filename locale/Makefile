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

all:
	$(MAKE) mo-files wxstd-mo-files

mo-files: $(FILES_MO)

wxstd-mo-files:
	(cd wxstd && $(MAKE) )

clean:
	$(RM) $(FILES_MO) wxstd/*.mo

dep depend: 


update:
	$(MAKE) msgcat
	$(MAKE) mergecat
	$(MAKE) check

check:
	@for i in $(FILES_PO); do echo $$i:; msgfmt -v $$i ; done

install: 
	$(MAKE) all
	$(INSTALL) -d $(DATADIR)/locale
	for i in $(FILES_MO); do $(INSTALL) -d $(DATADIR)/locale/`basename $$i .mo`/LC_MESSAGES; $(INSTALL_DATA) $$i $(DATADIR)/locale/`basename $$i .mo`/LC_MESSAGES/M.mo; done
	for i in wxstd/*.mo; do $(INSTALL) -d $(DATADIR)/locale/`basename $$i .mo`/LC_MESSAGES; $(INSTALL_DATA) $$i $(DATADIR)/locale/`basename $$i .mo`/LC_MESSAGES/wxstd.mo; done

msgcat:
	$(MAKE) -C .. msgcat

mergecat:
	cp ../messages.po messages_po
#	cat messages_po extra_po | sed "1,$$ s/^\\#\\~ //g" > tmp && mv tmp messages_po
	cat extra_po >> messages_po
	$(RM) tmp.po ; for i in $(FILES_PO); do msgmerge $$i messages_po -o tmp.po && mv tmp.po $$i; done

.PHONY: all dep clean doc semistatic static allclean install msgcat mergecat mo-files  wxstd-mo-files update check

%.mo: %.po
	$(MSGFMT) -v -o $@ $<



