# Makefile for src directory
# $Id$
#

CWD = src
SUB_DIRS = util classes gui mail
MOBJS=util/*.o classes/*.o gui/*.o  mail/*.o 

include ../makeopts

ifdef USE_PYTHON
SUB_DIRS += Python
MOBJS += Python/*.o
endif

all: 
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done
	$(LINK) -o M $(MOBJS) $(LIBS)
#	$(CXX) -shared $(MOBJS) -lswigpy $(NONPLIBS) -o libM.so
#	$(LINK) -o M -L. -lM $(SLIBS)

static:
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i all; done
	$(LINK) -o M.static $(LDFLAGS) $(MOBJS) $(SLIBS) 

doc:
	$(MAKE) -C ../include $@

clean:
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done
	$(RM) core 

dep depend: 
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done

install: all
	strip M
	$(INSTALL_PROGRAM) M $(BINDIR)
	$(INSTALL_DATA) -d $(DATADIR)/icons
	$(INSTALL_DATA) icons/* $(DATADIR)/icons

.PHONY: all dep clean doc static allclean install



