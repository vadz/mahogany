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

config: configure makeopts.in
	./configure

configure:	configure.in
	autoconf

makeopts: makeopts.in configure
	./configure

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
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i install; done
	$(INSTALL_DATA) `find doc -not -type d` $(DOCDIR)

.PHONY: all dep clean bak backup config program doc

