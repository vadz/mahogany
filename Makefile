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
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done

config.status: configure
	@ ./config.status --recheck || \
	echo "\n"; \
	echo "********************************************************" ; \
	echo "* You should run configure with appropriate arguments  *" ; \
	echo "* Refer to doc/Information.txt or use configure --help *" ; \
	echo "* If running 'make config' does not work, try running  *" ; \
	echo "* 'configure' manually. See config.log for output from *" ; \
	echo "* configure, which might help you find the problem.    *" ; \
	echo "********************************************************" ; \
	echo "\n"; \
	exit 1;

config: configure makeopts.in
	./configure || $(RM) config.status

configure: configure.in aclocal.m4
	autoconf && $(RM) config.status

makeopts: makeopts.in config.status
	CONFIG_FILES=$@ CONFIG_HEADERS=./config.status

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
	@echo "        docs in " $(PREFIX)/doc/M
	$(INSTALL) -d $(DATADIR) $(BINDIR) $(DOCDIR)
	$(INSTALL) -d $(DATADIR)/$(CANONICAL_HOST)/bin
	$(INSTALL) -d $(DATADIR)/$(CANONICAL_HOST)/lib
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i install; done
	$(INSTALL_DATA) `find doc -not -type d` $(DOCDIR)

.PHONY: all dep clean bak backup config program doc

