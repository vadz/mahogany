# Makefile for doc directory
# $Id$
#

CWD = doc
FILES_TXT=*.txt
FILES_TEX=*.tex
FILES_PS=$(FILES_TEX:.tex=.ps)

include ../makeopts

all: 

install: ps README.html
	$(INSTALL) -d $(DOCDIR)/txt $(DOCDIR)/ps $(DOCDIR)/html  wxhelp.map

ps: $(FILES_PS)

clean:


.PHONY: all install ps clean

include ../makerules



