# Makefile for doc directory
# $Id$

FILES_TEX = HackersGuide.htex Manual.htex
SUB_DIRS  = $(FILES_TEX:.htex=)
SUBDIRS   = $(FILES_TEX:.htex=.dir)
SUB_MAPS  = $(FILES_TEX:.htex=/wxhelp.map)

include ../makeopts
include ../makeversion

# Mdocs.tar.gz file may be put into this directory if you don't want to rebuild
# the docs but want to use the prebuilt version
MDOCS_TGZ := Mdocs-$(M_VERSION_MAJOR).$(M_VERSION_MINOR).tar.gz

all:	test
	if [ ! -f $(MDOCS_TGZ) ]; then $(MAKE) makemap; fi

test:
	@if test -f Manual.tex; then \
	echo "Found a Manual.tex file. There should not be one."; \
	echo "The Manual should be exported to a file Manual.htex instead."; \
	echo "Usually all you need to do now is:"; \
	echo "mv Manual.tex Manual.htex"; \
	echo "Then run make again.";\
	exit 1; \
	fi

include ../makerules

doc:	all

%.dir:
	if [ ! -d $* ]; then mkdir $*; fi
	$(MAKE) DIR=$* -f ../.src/Makefile.sub -C $* all

ifdef HAVE_DOCTOOLS
makemap: .src/wxhelp.map0 $(SUBDIRS)
	set -e; for i in $(SUB_DIRS); \
	do sed "s,^\([ 	0-9-]*\),\1$$i/,g" $$i/wxhelp.map; \
	done | sort -n -o wxhelp.map .src/wxhelp.map0 -
else
makemap:
	@echo "  # One of the tools required to build documentation is missing."
	@echo "  # Please either install the required tool(s) and rerun configure"
	@echo "  # or download a pre-built $(MDOCS_TGZ) and place it in the"
	@echo "  # doc directory before running \"make install\"."
	@false
endif

install: test
	if [ -f $(MDOCS_TGZ) ]; \
	then gzip -dc < $(MDOCS_TGZ) | (cd $(DOCDIR) && tar xvf -); \
	else $(MAKE) install2; \
	fi

install2: makemap
	@$(RM) -r $(DOCDIR)/Pdf
	$(INSTALL) -d $(DOCDIR)/Pdf
	set -e; for i in $(SUB_DIRS); \
	do $(MAKE) DIR=$$i -f ../.src/Makefile.sub -C $$i install; \
	done
	$(INSTALL_DATA) wxhelp.map $(DOCDIR)

clean:
	$(RM) *.log *.aux *.dvi *.sgml wxhelp.tab *.ps *.toc *.pdf tmp.*
	$(RM) -r $(SUB_DIRS) classes classes.tex wxhelp.map

install_classdoc: classdoc
	$(INSTALL) -d $(DOCDIR) $(DOCDIR)/ClassDoc
	for i in classes/*; do $(INSTALL_DATA) $$i $(DOCDIR)/ClassDoc; done

classdoc:
	$(MAKE) -C ../include doc
	cp ../src/icons/Micon.jpg ../extra/scandoc/images/*.gif ../extra/scandoc/images/*.jpg classes

mdocs_targz: install
	(cd $(DOCDIR) && tar cvf - *) | gzip -c9 > $(MDOCS_TGZ)

.PHONY: all doc install clean makemap install2 test classdoc install_classdoc mdocs_targz
