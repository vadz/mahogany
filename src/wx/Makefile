# Makefile for src/wx directory
#
# $Id$
#
# For all targets apart from clean, it will call the
# Makefile2 which includes the usual standard rules.
# This is needed to avoid warnings.

SUBDIRS=generic

all:
	for i in $(SUBDIRS); do $(MAKE) -C $$i $@ ; done

depend:
	for i in $(SUBDIRS); do $(MAKE) -C $$i $@ ; done

clean:
	$(RM) common/*.o generic/*.o

msgcat:
	for i in $(SUBDIRS); do $(MAKE) -C $$i $@ ; done

dep: depend

depend:
	for i in $(SUBDIRS); do $(MAKE) -C $$i $@ ; done

%: force
	$(MAKE) -f Makefile2 $@ 
force: ;

.PHONY: clean all
