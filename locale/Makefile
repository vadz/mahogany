# Makefile for locale directory
# $Id$

# Do a "make all" to update all .po files with latest strings from program
# sources. A simple "make" will only rebuild the .mo files, not the .po ones.

FILES_PO:=$(wildcard .src/*.po)
FILES_MO:=$(patsubst .src/%.po, %.mo, $(FILES_PO))

include ../makeopts

vpath %.po .src

%.mo: %.po
	$(MSGFMT) -v -o $@ $<

all:	$(FILES_MO)
	cd wxstd && $(MAKE)

clean:
	$(RM) $(FILES_MO) wxstd/*.mo

update: msgcat mergecat check
	@true

check:
	@for i in $(FILES_PO); do echo $$i:; msgfmt -v $$i ; done

install: all
	$(INSTALL) -d $(DATADIR)/locale
	set -e; for i in $(FILES_MO); \
	do $(INSTALL) -d $(DATADIR)/locale/`basename $$i .mo`/LC_MESSAGES; \
	$(INSTALL_DATA) $$i $(DATADIR)/locale/`basename $$i .mo`/LC_MESSAGES/M.mo; \
	done
	set -e; for i in wxstd/*.mo; \
	do $(INSTALL) -d $(DATADIR)/locale/`basename $$i .mo`/LC_MESSAGES; \
	$(INSTALL_DATA) $$i $(DATADIR)/locale/`basename $$i .mo`/LC_MESSAGES/wxstd.mo; \
	done

msgcat:
	$(MAKE) -C .. msgcat

mergecat:
	cp ../messages.po messages_po
#	cat messages_po extra_po | sed "1,$$ s/^\\#\\~ //g" > tmp && mv tmp messages_po
	cat extra_po >> messages_po
	$(RM) tmp.po ; for i in $(FILES_PO); do msgmerge $$i messages_po -o tmp.po && mv tmp.po $$i; done

.PHONY:	all clean doc install msgcat mergecat update check
