# Makefile for extra/install directory

CWD = install

include ../../makeopts

all:

install:
	$(INSTALL) -d $(DATADIR)/scripts
	$(INSTALL) -d $(DATADIR)/bin
	$(INSTALL) -d $(DATADIR)/doc/classes
	ln -fs $(DATADIR)/doc $(PREFIX)/doc/M
	$(INSTALL) `find scripts/* -not -type d` $(DATADIR)/bin
	$(INSTALL) `find scripts/py/* -not -type d` $(DATADIR)/scripts
	$(INSTALL) $(BUILDDIR)/doc/classes/* $(DATADIR)/doc/classes
	$(INSTALL) $(BUILDDIR)/doc/*.lyx $(DATADIR)/doc/
	$(INSTALL) $(BUILDDIR)/doc/*.tex $(DATADIR)/doc/
	$(INSTALL) $(BUILDDIR)/doc/*.ps $(DATADIR)/doc/

include ../../makerules

.PHONY: all install
