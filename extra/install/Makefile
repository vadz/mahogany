# Makefile for extra/install directory
# install the contents of all subdirs under this one

SUB_DIRS = scripts

include ../../makeopts

install:
	set -e; for i in $(SUB_DIRS); \
	do $(INSTALL) -d $(DATADIR)/$$i; \
	   $(MAKE) -C $$i; \
	done
	$(INSTALL) -d $(DATADIR)/afm
	set -e; for i in .src/afm/*.afm; \
	do $(INSTALL_DATA) $$i $(DATADIR)/afm; \
	done
	$(INSTALL_DATA) .src/newmail.wav $(DATADIR)
	$(INSTALL_DATA) .src/kylemail.wav $(DATADIR)
#	$(INSTALL_DATA) .src/mailcap $(DATADIR)
	$(INSTALL_DATA) .src/M.conf $(DATADIR)
	if test ! -d $(DESTDIR)/man/man1; then $(INSTALL) -d $(DESTDIR)/man/man1; fi
	$(INSTALL_DATA) .src/mahogany.1 $(DESTDIR)/man/man1
	$(INSTALL_DATA) .src/mahogany.1 $(DESTDIR)/man/man1/M.1

.PHONY: install
