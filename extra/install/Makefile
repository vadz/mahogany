# Makefile for extra/install directory

CWD = install

include ../../makeopts

all:

install:
	$(INSTALL) -d $(DATADIR)/scripts
	$(INSTALL) -d $(DATADIR)/bin
        $(INSTALL) -d $(DATADIR)/$(CANONICAL_HOST)/bin
        $(INSTALL) -d $(DATADIR)/$(CANONICAL_HOST)/lib
	$(INSTALL) -d $(DATADIR)/doc/classes
	$(INSTALL) -d $(DATADIR)/doc/python
	ln -fs $(DATADIR)/doc $(PREFIX)/doc/M
	$(INSTALL) `find scripts/* -not -type d` $(DATADIR)/bin
	$(INSTALL) `find scripts/py/* -not -type d | fgrep -v config.` $(DATADIR)/scripts
	$(INSTALL) $(BUILDDIR)/doc/classes/* $(DATADIR)/doc/classes
	$(INSTALL) $(BUILDDIR)/doc/*.lyx $(DATADIR)/doc/
	$(INSTALL) $(BUILDDIR)/doc/*.tex $(DATADIR)/doc/
	$(INSTALL) $(BUILDDIR)/doc/*.ps $(DATADIR)/doc/
	$(INSTALL) $(BUILDDIR)/src/Python/*.doc $(DATADIR)/doc/python
	$(INSTALL) $(LOCAL_LIBS) $(DATADIR)/$(CANONICAL_HOST)/lib

include ../../makerules

.PHONY: all install
