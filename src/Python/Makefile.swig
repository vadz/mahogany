# this is a temp helper makefile used to regenerate all *.*-swig files from
# the *.i SWIG interface definitions - of course, it should be done in the
# main makefile, really, but I'm afraid to touch to this stuff for now and
# will change it completely later (VZ)

SWIG=swig
SWIGFLAGS := -c++ -python -shadow

IFACES := $(wildcard *.i)
IFACES := $(filter-out SendMessageCC.i, $(IFACES))
OBJS := $(IFACES:.i=.o)

%.cpp %.py: %.i
	$(SWIG) -I$(dir $<) $(SWIGFLAGS) -o $@ $<

%.cpp-swig: %.cpp %.i
	sed 's/"Python.h"/"MPython.h"/' $(basename $@).cpp > $@

%.py-swig: %.py %.i
	cp $(basename $@).py $@

all: swig

swig: $(IFACES:.i=.cpp-swig) $(IFACES:.i=.py-swig)

swigclean:
	rm -f *.*-swig

.INTERMEDIATE: $(IFACES:.i=.py)

.PHONY: all swig swigclean
