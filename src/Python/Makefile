# Makefile for src/Python directory
#
# $Id$
#
# For all targets apart from clean, it will call the
# Makefile2 which includes the usual standard rules.
# This is needed to avoid warnings.

all:
	@$(MAKE) -f Makefile2 $@

clean:
	$(RM) *.o *.so *.pyc *.py *.html *.doc junk $(CPPTARGETS)
	ln -s INIT/Minit.py .

%.cpp: %.i
	$(SWIG) $(SWIGFLAGS) -o $(ODIR)/$@ $<

%: force
	@$(MAKE) -f Makefile2 $@
force: ;

.PHONY: clean

include ../../makeopts
