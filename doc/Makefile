# Makefile for doc directory
# $Id$

FILES_TEX = HackersGuide.htex Manual.htex
FILES_PDF = $(FILES_TEX:.htex=.pdf) #classes.ps
SUB_DIRS  = $(FILES_TEX:.htex=)

include ../makeopts

all:	test
	if [ ! -f Mdocs.tar.gz ]; then $(MAKE) makemap; fi

include ../makerules

vpath %.htex .src

doc:	all

docxx.sty: .src/docxx.sty
	ln -sf .src/docxx.sty docxx.sty
html.sty: .src/html.sty
	ln -sf .src/html.sty html.sty

subdirs: docxx.sty html.sty 
	set -e; for i in $(FILES_PDF); do $(MAKE) $$i; done
	set -e; for i in $(SUB_DIRS); \
	do if [ ! -d $$i ]; then mkdir $$i; fi; \
	   cp -f docxx.sty html.sty $$i; \
	   $(MAKE) DIR=$$i -f ../.src/Makefile.sub -C $$i setup all; \
	done
	
install2:
	$(INSTALL) -d $(DOCDIR)/Pdf
	$(INSTALL_DATA) $(FILES_PDF) $(DOCDIR)/Pdf
	set -e; for i in $(SUB_DIRS); \
	do $(MAKE) DIR=$$i -f ../.src/Makefile.sub -C $$i install; \
	done
	$(INSTALL_DATA) wxhelp.map $(DOCDIR)
#	ln -sf ../share/Mahogany/doc $(DESTDIR)/doc/Mahogany

install_classdoc:
	$(INSTALL) -d $(DOCDIR) $(DOCDIR)/Postscript $(DOCDIR)/classes $(DOCDIR)/classes/images
	$(INSTALL_DATA) classes/* $(DOCDIR)/classes

install: all
	if [ -f Mdocs.tar.gz ]; \
	then gzip -dc < Mdocs.tar.gz | (cd $(DOCDIR) && tar xvf -); \
	else $(MAKE) install2; \
	fi

classdoc:
	$(MAKE) -C ../include doc
	cp ../src/icons/Micon.jpg ../extra/scandoc/images/*.gif ../extra/scandoc/images/*.jpg classes

makemap: .src/wxhelp.map0 subdirs
	@$(RM) -f wxhelp.map
	#../extra/scripts/html2wxhelp >>wxhelp.map
	set -e; for i in $(SUB_DIRS); \
	do sed "s/^\([ 	-0-9]*\)/\1$$i\//g" $$i/wxhelp.map; \
	done >>wxhelp.map
	sort -n -o wxhelp.map .src/wxhelp.map0 wxhelp.map

clean:
	$(RM) *.log *.aux *.dvi *.sgml wxhelp.tab *.ps *.toc *.pdf
	$(RM) -r $(SUB_DIRS) classes classes.tex wxhelp.map
	$(RM) tmp.*

test:
	@if test -f Manual.tex; then \
	echo "Found a Manual.tex file. There should not be one."; \
	echo "The Manual should be exported to a file Manual.htex instead."; \
	echo "Usually all you need to do now is:"; \
	echo "mv Manual.tex Manual.htex"; \
	echo "Then run make again.";\
	exit 1; \
	fi

.PHONY: all install ps clean subdirs makemap install2 test classdoc \
	install_classdoc all2 pdf
