# Makefile for doc directory
# $Id$
#

CWD = doc
FILES_TXT=*.txt
FILES_TEX=*.tex
FILES_PS=$(FILES_TEX:.tex=.ps)

include ../makeopts

all: 

install: ps
	$(INSTALL) -d $(DOCDIR)/txt $(DOCDIR)/ps $(DOCDIR)/html 

ps: $(FILES_PS)


.PHONY: all install ps

include ../makerules



