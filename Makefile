# Makefile for M root directory
# $Id$
#


CWD = 
SUB_DIRS = extra src include
FILES = makeopts makerules Makefile makeopts.in configure.in configure
EXTRA = extra

include makeopts

all:
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i all; done

doc:
	set -e; for i in include extra ; do $(MAKE) -C $$i doc; done

clean:
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done

allclean:
	find . -name .depend -exec rm {} \;

dep depend: 
	@echo "The first time \"make depend\" will print lots of warnings"
	@echo "about missing header files - these can be safely ignored."
	@echo "They should disappear if you run it again."
	@echo "----------------------------------------------------------"
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done

config.status: 
	@echo "You should really run ./configure with appropriate arguments."
	@echo "Type ./configure --help to find out more about it."
	@echo "Running ./configure with default setup now..."
	./configure

config: configure makeopts.in
	./configure || $(RM) config.status

configure: configure.in aclocal.m4
	autoconf && $(RM) config.status

makeopts: makeopts.in config.status
	./config.status

include/config.h: include/config.h.in configure
	./configure

bak backup:
	tar cvf M-`date +"%y-%m-%d"`.tar $(SUB_DIRS) $(FILES) $(EXTRA)
	gzip -9 M-`date +"%y-%m-%d"`.tar
#
# probably the most complicated target:
#
install:
	@echo "Installing M in " $(BINDIR)
	@echo "        data in " $(DATADIR)
	@echo "        docs in " $(DOCDIR)
	$(INSTALL) -d $(DATADIR) $(BINDIR) $(DOCDIR)
	$(INSTALL) -d $(DATADIR)/$(CANONICAL_HOST)/bin
	$(INSTALL) -d $(DATADIR)/$(CANONICAL_HOST)/lib
	$(INSTALL) -d $(BINDIR)
	$(INSTALL) -d $(DATADIR)/bin
	$(INSTALL) -d $(DATADIR)/lib
	$(INSTALL) -d $(DATADIR)/doc
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i install; done

install_doc:
	$(MAKE) -C doc install
	$(INSTALL_DATA) TODO README $(DOCDIR)

install_all: install install_doc install_locale

install_locale:
	$(MAKE) -C locale install

msgcat:
	$(MAKE) -C src msgcat
	$(MAKE) -C include msgcat

locales:
	$(MAKE) -C locale all

.PHONY: all dep clean bak backup config program doc install install_doc \
        install_all msgcat locales scandoc install_locale

