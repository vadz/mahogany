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


ifdef USE_PYTHON
SUB_DIRS += Python
MOBJS += Python/*.o
endif

# LeakTracer must be the last object on the linker line!
ifeq ($(USE_MODULES), static)
MOBJS += modules/*.o
endif

# build the dynamically linked binary by default
all:	M

M:	subdirs
	$(LINK) -o M $(MOBJS) $(LDFLAGS) $(GUI_LIBS) $(LIBS)

#$(MOBJS): subdirs

#
#left over from my attempt to create a M.so shared lib
#

Mx:	libM.so
	$(LINK) -o M -L. $(LDFLAGS) -lM $(LDFLAGS) $(GUI_LIBS) $(LIBS)

libM.so:
	$(CXX) -shared $(MOBJS) $(LDFLAGS) $(NONPLIBS) -o libM.so

# build the binary which is statically linked to wxGTK, dynamically to
# everything else
quartstatic: $(MOBJS)
	$(LINK) -o M $(MOBJS) $(LDFLAGS) $(GUI_LIBS) $(LIBS_STATIC)

# build the binary which is statically linked to wxGTK and GTK+, dynamically
# to everything else
semistatic: $(MOBJS)
	$(LINK) -o M $(MOBJS) $(LDFLAGS) $(LIBS_STATIC) $(GUI_LIBS_STATIC)

# build a (completely) statically linked binary
static: $(MOBJS)
	$(LINK) -static -o M.static $(LDFLAGS) $(MOBJS) $(LIBS) -lcrypt

subdirs:
	set -e; $(MAKE) -C include
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i all; done
	set -e; $(MAKE) -i -C modules all

doc:
	$(MAKE) -C ../include $@

clean:
	set -e; for i in $(SUB_DIRS) modules; do $(MAKE) -C $$i $@; done
	$(RM) core libM.so M M.static

dep depend:
	set -e; for i in $(SUB_DIRS) modules; do $(MAKE) -C $$i $@; done

install: 
	strip M
	$(INSTALL_PROGRAM) M $(DATADIR)/$(CANONICAL_HOST)/bin
	$(RM) -f $(BINDIR)/M $(DATADIR)/bin/M
	$(INSTALL) -d $(BINDIR)
	ln -f -s ../share/Mahogany/$(CANONICAL_HOST)/bin/M $(BINDIR)
	$(RM) $(BINDIR)/mahogany
	ln -f -s ../share/Mahogany/$(CANONICAL_HOST)/bin/M $(BINDIR)/mahogany
	$(MAKE) -C modules $@
	$(INSTALL) -d $(DATADIR)/icons
	for i in icons/*.xpm ; do $(INSTALL_DATA) $$i $(DATADIR)/icons; done
	$(INSTALL) -d $(DATADIR)/icons/GNOME
	for i in icons/GNOME/*.xpm ; do $(INSTALL_DATA) $$i $(DATADIR)/icons/GNOME; done
	$(INSTALL) -d $(DATADIR)/icons/small
	for i in icons/small/*.xpm ; do $(INSTALL_DATA) $$i $(DATADIR)/icons/small; done
ifdef USE_PYTHON
	$(MAKE) -C Python $@
endif

msgcat:
	set -e; for i in $(SUB_DIRS) modules ; do $(MAKE) -C $$i $@; done
#	$(MAKE) -C $(BUILDDIR)/locale mergecat

.PHONY: all dep clean doc semistatic static allclean install msgcat subdirs



