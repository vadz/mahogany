# Makefile for src directory
# $Id$
#

CWD = src
UTILOBJ:=util/strutil.o util/sysutil.o util/miscutil.o util/upgrade.o
SUB_DIRS =  util classes gui mail adb wx # modules
MOBJS:=$(UTILOBJ) classes/*.o gui/*.o  mail/*.o adb/*.o \
      wx/generic/*.o wx/common/*.o

include ../makeopts

ifdef USE_LEAKTRACER
MOBJS := $(MOBJS) util/LeakTracer.o
endif

#
# List all directories which create modules (.so files) to be installed:
#
MODULES_DIRS = adb

ifdef USE_PYTHON
SUB_DIRS += Python
MOBJS += Python/*.o
endif

# LeakTracer must be the last object on the linker line!
ifeq ($(USE_MODULES), static)
MOBJS += modules/*.o
endif

# build the dynamically linked binary by default
all:	Mdll modules main

main:   Mdll

M:	subdirs
	$(LINK) -o M $(MOBJS) $(LDFLAGS) $(GUI_LIBS) $(LIBS)

#$(MOBJS): subdirs

#
#left over from my attempt to create a M.so shared lib
#

Mdll:	libM.so
	$(LINK) -o M -L. $(LDFLAGS) -lM 

libM.so: subdirs
	$(CXX) -shared $(MOBJS) $(LDFLAGS) $(NONPLIBS) $(LDFLAGS) $(GUI_LIBS) $(LIBS) -o libM.so

# build the binary which is statically linked to wxGTK, dynamically to
# everything else
quartstatic: modules_obj
	$(LINK) -o M $(MOBJS) $(LDFLAGS) $(GUI_LIBS) $(LIBS_STATIC)

# build the binary which is statically linked to wxGTK and GTK+, dynamically
# to everything else
semistatic: modules_obj
	$(LINK) -o M $(MOBJS) $(LDFLAGS) $(LIBS_STATIC) $(GUI_LIBS_STATIC)

# build a (completely) statically linked binary
static: modules_obj
	$(LINK) -static -o M.static $(LDFLAGS) $(MOBJS) $(LIBS) -lcrypt

subdirs:
	set -e; $(MAKE) -C ../include
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i all; done

modules:
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i modules; done
	set -e; $(MAKE) -C modules modules

modules_obj:
	set -e; $(MAKE) -C modules 

doc:
	$(MAKE) -C ../include $@

clean:
	set -e; for i in $(SUB_DIRS) modules; do $(MAKE) -C $$i $@; done
	$(RM) core libM.so M M.static

dep depend:
	set -e; for i in $(SUB_DIRS) modules; do $(MAKE) -C $$i $@; done

install: install-modules
	strip M
	$(INSTALL_PROGRAM) M $(DATADIR)/$(CANONICAL_HOST)/bin
	if test -f libM.so ; then $(INSTALL_PROGRAM) libM.so $(DATADIR)/$(CANONICAL_HOST)/lib; fi
	$(RM) -f $(BINDIR)/M $(DATADIR)/bin/M
	$(INSTALL) -d $(BINDIR)
	ln -f -s ../share/Mahogany/$(CANONICAL_HOST)/bin/M $(BINDIR)
	$(RM) $(BINDIR)/mahogany
	ln -f -s ../share/Mahogany/$(CANONICAL_HOST)/bin/M $(BINDIR)/mahogany
	$(INSTALL) -d $(DATADIR)/icons
	for i in icons/*.xpm ; do $(INSTALL_DATA) $$i $(DATADIR)/icons; done
ifdef USE_PYTHON
	$(MAKE) -C Python $@
endif

#
# This must call modules-install for all modules directories:
#
install-modules:
ifeq ($(USE_MODULES),dynamic)
	for i in $(MODULES_DIRS); do $(MAKE) -C $$i $@ ; done
	set -e; $(MAKE) -C modules install
endif

msgcat:
	set -e; for i in $(SUB_DIRS) modules ; do $(MAKE) -C $$i $@; done
#	$(MAKE) -C $(BUILDDIR)/locale mergecat

.PHONY: all dep clean doc semistatic static allclean install msgcat subdirs install-modules modules main modules_obj



