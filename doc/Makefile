# Makefile for doc directory
# $Id$
#

CWD = doc
FILES_TXT=*.txt
#FILES_TEX=$(wildcard *.tex)
FILES_TEX=HackersGuide.htex Manual.htex 
FILES_PDF=$(FILES_TEX:.htex=.pdf) #classes.ps
SUB_DIRS=$(FILES_TEX:.htex=)

include ../makeopts

all: test
	if [ ! -f Mdocs.tar.gz ] ; then $(MAKE) all2 ; fi

all2:
	$(MAKE) -k pdf
	$(MAKE) -k makemap

doc:	all

subdirs: 
	set -e; for i in $(SUB_DIRS); do if [ ! -d $$i ] ; then mkdir $$i ; fi ; done
	set -e; for i in $(SUB_DIRS); do cp docxx.sty html.sty $$i ; done
	set -e; for i in $(SUB_DIRS); do $(MAKE) DIR=$$i -f ../Makefile.sub -C $$i setup ; done
	set -e; for i in $(SUB_DIRS); do ( cd $$i && $(MAKE) -f ../Makefile.sub all DIR=$$i ) ; done

install2: 
	$(INSTALL) -d $(DOCDIR)
	$(INSTALL) -d $(DOCDIR)/Pdf
	$(INSTALL_DATA) $(FILES_PDF) $(DOCDIR)/Pdf
	$(INSTALL) -d $(DOCDIR)/Tips
	$(INSTALL) Tips/Tips*.txt  $(DOCDIR)/Tips
	set -e; for i in $(SUB_DIRS); do $(MAKE) DIR=$$i -f ../Makefile.sub -C $$i install ; done
	$(INSTALL_DATA) wxhelp.map $(DOCDIR)
#	$(INSTALL) -d $(DESTDIR)/doc
#	ln -f -s ../share/Mahogany/doc $(DESTDIR)/doc/Mahogany

install_classdoc:
	$(INSTALL) -d $(DOCDIR) $(DOCDIR)/Postscript $(DOCDIR)/classes $(DOCDIR)/classes/images
	$(INSTALL_DATA) classes/* $(DOCDIR)/classes

pdf:
	for i in $(FILES_PDF); do $(MAKE) -k $$i; done   # one after another

install: all
	if [ -f Mdocs.tar.gz ] ; then gzip -dc < Mdocs.tar.gz | (cd $(DOCDIR) && tar xvf - ); else $(MAKE) install2 ; fi

classdoc: 
	$(MAKE) -C ../include doc
	cp ../src/icons/Micon.jpg ../extra/scandoc/images/*.gif ../extra/scandoc/images/*.jpg classes

makemap: subdirs
	cp wxhelp.map0 wxhelp.tmp
	#../extra/scripts/html2wxhelp >>wxhelp.tmp
	set -e; for i in $(SUB_DIRS); do cat $$i/wxhelp.map | \
	sed "1,$$ s/^\([ 	-0-9]*\)/\1$$i\//g" >>wxhelp.tmp; done
	cat wxhelp.tmp | sort -n > wxhelp.map

clean:
	$(RM) *.log *.aux *.dvi *.sgml wxhelp.tab wxhelp.tmp *.ps *.toc *.pdf
	set -e; for i in $(SUB_DIRS); do $(RM) -r $$i; done
	$(RM) -r classes classes.tex wxhelp.map

test:
	@if test -f Manual.tex ; then \
	echo "Found a Manual.tex file. There should not be one."; \
	echo "The Manual should be exported to a file Manual.htex instead."; \
	echo "Usually all you need to do now is:"; \
	echo "mv Manual.tex Manual.htex"; \
	echo "Then run make again." ;\
	exit 1; \
	fi 

.PHONY: all install ps clean subdirs makemap install2 test classdoc install_classdoc all2 pdf

include ../makerules



