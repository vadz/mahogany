# Makefile for extra/install/scripts directory

CWD = scripts
FILES = py/*.py

include ../../../makeopts

#
# install the contents of all subdirs under this one
#
install:
	$(INSTALL_PROGRAM) Mnetscape $(BINDIR)
	set -e; for i in $(FILES); do  $(INSTALL_DATA) $$i $(DATADIR)/scripts; done

include ../../../makerules

.PHONY: install
