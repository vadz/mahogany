# Makefile for extra/install directory

CWD = install
SUB_DIRS = scripts

include ../../makeopts

# do nothing for all
all:

#
# install the contents of all subdirs under this one
#
install:
	set -e; for i in $(SUB_DIRS); do $(INSTALL) -d $(DATADIR)/$$i ; done
	set -e; for i in $(SUB_DIRS); do  $(MAKE) -C $$i ; done
	$(INSTALL) -d $(DATADIR)/afm
	$(INSTALL_DATA) afm/*.afm $(DATADIR)/afm
	$(INSTALL_DATA) newmail.wav $(DATADIR)
	$(INSTALL_DATA) mailcap $(DATADIR)

clean:


include ../../makerules

.PHONY: all install clean
