# Makefile for local directory
# $Id$
#

CWD = locale
SUB_DIRS = 
FILES_MO=$(wildcard *.mo)
FILES_PO=$($FILES_MO:.mo=.po)

include ../makeopts

all: 	$(FILES_PO)

clean:
	$(RM) $(FILES_PO)

dep depend: 


install: all
	$(INSTALL) -d $(DATADIR)/locale
	for i in $(FILES_PO); do $(INSTALL) -d $(DATADIR)/locale/`basename $$i .po`/LC_MESSAGES; $(INSTALL_DATA) $$i $(DATADIR)/locale/`basename $$i .po`/LC_MESSAGES/M.po; done


.PHONY: all dep clean doc semistatic static allclean install msgcat

%.po: %.mo
	$(MSGFMT) -o $@ $<



