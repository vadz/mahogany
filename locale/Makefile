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

install: all
	$(INSTALL) -d $(LOCALEDIR)
	set -e; for i in $(FILES_MO); \
	do $(INSTALL) -d $(LOCALEDIR)/`basename $$i .mo`/LC_MESSAGES; \
	$(INSTALL_DATA) $$i $(LOCALEDIR)/`basename $$i .mo`/LC_MESSAGES/M.mo; \
	done

check:
	@for i in $(FILES_PO); \
	do echo $$i:; $(MSGFMT) -v -o /dev/null $$i; \
	done

# print out some information for the translator's web page
# should run this only after `make mergecat' has merged new strings
percentage:
	@for i in $(FILES_PO); \
	do	x=`$(MSGFMT) -v -o /dev/null "$$i" 2>&1 | sed -e 's/[,\.]//g' \
			-e 's,\([0-9]*\) translated messages,TR=\1,' \
			-e 's,\([0-9]*\) fuzzy translations,FZ=\1,' \
			-e 's,\([0-9]*\) untranslated messages,UT=\1,'`; \
		TR=0 FZ=0 UT=0; \
		eval $$x; \
		echo "$$i" `expr 100 "*" $$TR / "(" $$TR + $$FZ + $$UT ")"` \
			percent of `expr $$TR + $$FZ + $$UT` strings; \
		sed -n	-e '/Project-Id-Version/p' -e '/Revision-Date/p' \
			-e '/Last-Translator/p' -e '/^$$/{p;q;}' "$$i"; \
	done

else		# we don't have MSGFMT; don't try to compile translations

all install check:
	@true
endif		# MSGFMT

clean:
	$(RM) $(FILES_MO)

mergecat:
ifeq "$(TRANSLATIONS)" "yes"
# we have the programs necessary to update existing translations
# with any new strings used by the program
	$(MAKE) -C ../src msgcat 
	cat ../messages.po > messages_po
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
