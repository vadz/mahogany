# Makefile for doc directory
# $Id$
#

CWD = doc
FILES_TXT=*.txt
#FILES_TEX=$(wildcard *.tex)
FILES_TEX=Manual.htex 
FILES_PS=$(FILES_TEX:.htex=.ps) classes.ps
SUB_DIRS=$(FILES_TEX:.htex=)

include ../makeopts

all: 
	$(MAKE) -k ps
	$(MAKE) -k makemap

doc:	all

subdirs: 
	set -e; for i in $(SUB_DIRS); do if [ ! -d $$i ] ; then mkdir $$i ; fi ; done
	set -e; for i in $(SUB_DIRS); do cp docxx.sty $$i ; done
	set -e; for i in $(SUB_DIRS); do $(MAKE) DIR=$$i -f ../Makefile.sub -C $$i setup ; done
	set -e; for i in $(SUB_DIRS); do ( cd $$i && $(MAKE) -f ../Makefile.sub all DIR=$$i ) ; done

install2: 
	$(INSTALL) -d $(DOCDIR) $(DOCDIR)/Postscript $(DOCDIR)/classes $(DOCDIR)/classes/images
	$(INSTALL_DATA) $(FILES_PS) $(DOCDIR)/Postscript
	set -e; for i in $(SUB_DIRS); do $(MAKE) DIR=$$i -f ../Makefile.sub -C $$i install ; done
	$(INSTALL_DATA) wxhelp.map $(DOCDIR)
	$(INSTALL_DATA) classes/*.html $(DOCDIR)/classes
	if [ "$(SCANDOCHTML)" = "$(DOCHTML)" ] ; then $(INSTALL_DATA) $(BUILDDIR)/extra/scandoc/images/* $(DOCDIR)/classes/images ; fi
	$(INSTALL_DATA) classes/images/*.gif $(DOCDIR)/classes/images || echo no gifs to install
	$(INSTALL_DATA) classes/images/*.jpg $(DOCDIR)/classes/images || echo no jpegs to install
	$(INSTALL) -d $(DESTDIR)/doc
#	ln -f -s ../share/Mahogany/doc $(DESTDIR)/doc/Mahogany

ps: $(FILES_PS) 

install:
	if [ -f Mdoc.tar.gz ] ; then gzip -dc < Mdoc.tar.gz | (cd $(DOCDIR) && tar xvf - ); else $(MAKE) install2 ; fi

classes.tex: ../include/*.h ../include/adb/*.h ../include/gui/*.h
	$(MAKE) -C ../include doc

makemap: subdirs
	cp wxhelp.map0 wxhelp.tmp
	../extra/scripts/html2wxhelp >>wxhelp.tmp
	set -e; for i in $(SUB_DIRS); do cat $$i/wxhelp.map | \
	sed "1,$$ s/^\([ 	-0-9]*\)/\1$$i\//g" >>wxhelp.tmp; done
	cat wxhelp.tmp | sort -n > wxhelp.map

clean:
	$(RM) *.log *.aux *.dvi *.sgml wxhelp.tab wxhelp.tmp *.ps *.toc
	set -e; for i in $(SUB_DIRS); do $(RM) -r $$i; done
	$(RM) -r classes classes.tex wxhelp.map

.PHONY: all install ps clean subdirs makemap install2

include ../makerules



