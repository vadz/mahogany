# Makefile for locale directory
# $Id$

# Do a "make mergecat" to update all .po files with latest strings from
# program sources.  A simple "make" will only rebuild the .mo files, not
# the .po ones.

FILES_PO := $(patsubst .src/%,%,$(wildcard .src/*.po))
FILES_MO := $(FILES_PO:.po=.mo)

include ../makeopts

ifdef MSGFMT	# we have MSGFMT; we can compile translations to binary
vpath %.po .src
%.mo: %.po
	$(MSGFMT) -v -o $@ $<

all:	$(FILES_MO)
	cd wxstd && $(MAKE)

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

check:
	@for i in $(FILES_PO); \
	do echo $$i:; $(MSGFMT) -v -o /dev/null $$i; \
	done

else		# we don't have MSGFMT; don't try to compile translations

all install check:
	@true
endif		# MSGFMT

clean:
	$(RM) $(FILES_MO) wxstd/*.mo

mergecat: .src/extra_po
ifeq "$(TRANSLATIONS)" "yes"
# we have the programs necessary to update existing translations
# with any new strings used by the program
	$(MAKE) -C ../src msgcat 
	cat ../messages.po .src/extra_po > messages_po
	@$(RM) tmp.po
	@echo "Merging message strings into translation files:"
	@for i in $(FILES_PO); \
	do echo -n "$$i: "; \
	   $(MSGMERGE) .src/$$i messages_po -o tmp.po && mv tmp.po $$i; \
	done
else
	@echo "you need xgettext and msgmerge to update the translations"
	@false
endif

.PHONY:	all clean doc install mergecat update check
