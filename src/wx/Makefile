# Makefile for src/wx directory
#
# $Id$
#
# For all targets apart from clean, it will call the
# Makefile2 which includes the usual standard rules.
# This is needed to avoid warnings.

all:
	@$(MAKE) -f Makefile2 $@

clean:
	$(RM) common/*.o generic/*.o

%: force
	@$(MAKE) -f Makefile2 $@
force: ;

,PHONY: clean
