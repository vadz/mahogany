# Makefile for M root directory
# $Id$
#
# $Log$
# Revision 1.5  1998/05/18 17:48:10  KB
# more list<>->kbList changes, fixes for wxXt, improved makefiles
#
# Revision 1.4  1998/05/02 18:29:41  KB
# After many problems, Python integration is eventually taking off -
# works.
#
#

CWD = 
SUB_DIRS = extra src include
FILES = makeopts makerules Makefile makeopts.in configure.in configure
EXTRA = extra

include makeopts

all:
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i all; done

clean:
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done

allclean:
	find . -name .depend -exec rm {} \;

dep depend: 
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done

config: configure
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

.PHONY: all dep clean bak backup config program 

