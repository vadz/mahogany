# Makefile for extra/install directory

SUB_DIRS = scripts

include ../../makeopts

# install the contents of all subdirs under this one
#
install:
	set -e; for i in $(SUB_DIRS); \
	  do $(INSTALL) -d $(DATADIR)/$$i ; \
	  $(MAKE) -C $$i ; \
	done
	$(INSTALL) -d $(DATADIR)/afm
	$(INSTALL_DATA) .src/afm/*.afm $(DATADIR)/afm
	$(INSTALL_DATA) .src/newmail.wav .src/kylemail.wav $(DATADIR)
#	$(INSTALL_DATA) .src/mailcap $(DATADIR)
	$(INSTALL_DATA) .src/M.conf $(DATADIR)

.PHONY: all install clean
