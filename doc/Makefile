# Makefile for doc directory
# $Id$
#

CWD = doc
FILES_TXT=*.txt
FILES_TEX=$(wildcard *.tex)
FILES_PS=$(FILES_TEX:.tex=.ps)
SUB_DIRS=$(FILES_TEX:.tex=)

include ../makeopts

all: subdirs ps makemap

subdirs:
	set -e; for i in $(SUB_DIRS); do if [ ! -d $$i ] ; then mkdir $$i ; fi ; done
	set -e; for i in $(SUB_DIRS); do $(MAKE) DIR=$$i -f ../Makefile.sub -C $$i all ; done

install: all
	$(INSTALL) -d $(DOCDIR) $(DOCDIR)/Postscript
	set -e; for i in $(SUB_DIRS); do $(MAKE) DIR=$$i -f ../Makefile.sub -C $$i install ; done
	$(INSTALL_DATA) wxhelp.map $(DOCDIR)

ps: $(FILES_PS) 

makemap: subdirs
	set -e; for i in $(SUB_DIRS); do $(MAKE) DIR=$$i -f ../Makefile.sub -C $$i wxhelp.map ; done
	cp wxhelp.map0 wxhelp.tmp
	../extra/scripts/html2wxhelp >>wxhelp.tmp
	set -e; for i in $(SUB_DIRS); do cat $$i/wxhelp.map | sed "1,$$ s/;/;$$i: /g" | \
	sed "1,$$ s/^\([ 	-0-9]*\)/\1$$i\//g" >>wxhelp.tmp; done
	cat wxhelp.tmp | sort -n > wxhelp.map

clean:
	$(RM) *.log *.aux *.dvi *.sgml wxhelp.tab wxhelp.tmp *.ps *.toc
	set -e; for i in $(SUB_DIRS); do $(RM) -r $$i; done


.PHONY: all install ps clean subdirs makemap

include ../makerules



