# Makefile for doc directory
# $Id$

FILES_TEX = HackersGuide.htex Manual.htex
FILES_PDF = $(FILES_TEX:.htex=.pdf) #classes.ps
SUB_DIRS  = $(FILES_TEX:.htex=)

include ../makeopts

all:	test
	if [ ! -f Mdocs.tar.gz ]; then $(MAKE) newdoc; fi

include ../makerules

vpath %.htex .src

doc:	all

install: 
	if [ -f Mdocs.tar.gz ]; \
	then gzip -dc < Mdocs.tar.gz | (cd $(DOCDIR) && tar xvf -); \
	else $(MAKE) newinstall; \
	fi


# The "newdoc" target copies each .htex file into a subdirectory and
# builds the online docs in there. Only after that has happened, it
# removes the help-IDs from the file and builds the PDF version.
newdoc: doctoolstest
	$(MAKE) clean
	echo > .src/wxhelp.map
	set -e; for i in $(SUB_DIRS); do \
	mkdir $$i && ( cd $$i &&  \
        cp ../$$i.htex . ; latex $$i.htex ; \
	$(LATEX2HTML)  -local_icons -debug $$i.htex ; \
	( cd $$i && ../../../extra/scripts/html2wxhelp | \
	sed "s/^\([ 	-0-9]*\)/\1$$i\//g" >> ../../wxhelp.map; ); \
	cat $$i.htex | ../../extra/scripts/striphelpids > $$i.tex ) ; \
	$(MAKE) $$i/$$i.pdf ; \
	done
	sort -n -o wxhelp.map .src/wxhelp.map0 wxhelp.map

#Copies the files to their locations.
newinstall:
#	Check if we have the "Manual" directory, if yes, assume
#	that the docs have been built.
	@if [ ! -d Manual ] ; then $(MAKE) newdoc ; fi
	$(RM) -r $(DOCDIR)/Pdf
	$(INSTALL) -d $(DOCDIR)/Pdf
	$(INSTALL_DATA) wxhelp.map $(DOCDIR)
	set -e; for i in $(SUB_DIRS); do \
	( cd $$i && \
	  $(RM) -r $(DOCDIR)/$$i ; \
	  $(INSTALL_DATA) $$i.pdf $(DOCDIR)/Pdf/ ;\
	  $(INSTALL) -d $(DOCDIR)/$$i ; \
	  for f in `find $$i -type f` ; do $(INSTALL_DATA) $$f $(DOCDIR)/$$i ; done ;\
	) done

install_classdoc: classdoc
	$(INSTALL) -d $(DOCDIR) $(DOCDIR)/ClassDoc 
	for i in classes/* ; do $(INSTALL_DATA) $$i $(DOCDIR)/ClassDoc ; done

classdoc:
	$(MAKE) -C ../include doc
	cp ../src/icons/Micon.jpg ../extra/scandoc/images/*.gif ../extra/scandoc/images/*.jpg classes


doctoolstest:
	@echo "  # Checking for tools required to build documentation."
	@echo "  # If this fails, please either install the required tools"
	@echo "  # or download a pre-built Mdocs.tar.gz and place it in this"
	@echo "  # directory before running \"make install\"."
	@if [ -z "$(LATEX)" ] ; then echo "latex not found" ; exit 1 ; fi
	@if [ -z "$(DVIPS)" ] ; then echo "dvips not found" ; exit 1 ;fi 
	@if [ -z "$(PSTOPDF)" ] ; then echo "ps2pdf not found" ; exit 1 ;fi
	@if [ -z "$(LATEX2HTML)" ] ; then echo "latex2html not found" ; exit 1 ;fi
	@echo "All tools found."

.PHONY: all install ps clean subdirs makemap install2 test classdoc \
	install_classdoc all2 pdf newinstall newdoc doctoolstest





#
# all the following is obsolete and should be removed:
#



docxx.sty: .src/docxx.sty
	ln -sf .src/docxx.sty docxx.sty
html.sty: .src/html.sty
	ln -sf .src/html.sty html.sty

subdirs: docxx.sty html.sty 
	set -e; for i in $(FILES_PDF); do $(MAKE) $$i; done
	set -e; for i in $(SUB_DIRS); \
	do if [ ! -d $$i ]; then mkdir $$i; fi; \
	   cp -f docxx.sty html.sty $$i; \
	   $(MAKE) DIR=$$i -f ../.src/Makefile.sub -C $$i all; \
	done

install2:
	$(INSTALL) -d $(DOCDIR)/Pdf
	$(INSTALL_DATA) $(FILES_PDF) $(DOCDIR)/Pdf
	set -e; for i in $(SUB_DIRS); \
	do $(MAKE) DIR=$$i -f ../.src/Makefile.sub -C $$i install; \
	done
	$(INSTALL_DATA) wxhelp.map $(DOCDIR)
#	ln -sf ../share/Mahogany/doc $(DESTDIR)/doc/Mahogany




makemap: .src/wxhelp.map0 subdirs
	@$(RM) -f wxhelp.map
#../extra/scripts/html2wxhelp >>wxhelp.map
	set -e; for i in $(SUB_DIRS); \
	do sed "s/^\([ 	-0-9]*\)/\1$$i\//g" $$i/wxhelp.map; \
	done >>wxhelp.map
	sort -n -o wxhelp.map .src/wxhelp.map0 wxhelp.map

clean:
	$(RM) *.log *.aux *.dvi *.sgml wxhelp.tab *.ps *.toc *.pdf tmp.*
	$(RM) -r $(SUB_DIRS) classes classes.tex wxhelp.map

test:
	@if test -f Manual.tex; then \
	echo "Found a Manual.tex file. There should not be one."; \
	echo "The Manual should be exported to a file Manual.htex instead."; \
	echo "Usually all you need to do now is:"; \
	echo "mv Manual.tex Manual.htex"; \
	echo "Then run make again.";\
	exit 1; \
	fi

