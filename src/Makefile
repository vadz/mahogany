# Makefile for src directory
# $Id$
#

CWD = src
SUB_DIRS = util classes gui mail adb wx modules
MOBJS=util/*.o classes/*.o gui/*.o  mail/*.o adb/*.o \
      wx/generic/*.o wx/common/*.o

include ../makeopts

ifdef USE_PYTHON
SUB_DIRS += Python
MOBJS += Python/*.o
endif

# build the dynamically linked binary by default
all: subdirs
	$(LINK) -o M $(MOBJS) $(LDFLAGS) $(GUI_LIBS) $(LIBS)

M:	all

#
#left over from my attempt to create a M.so shared lib
#

Mx:	libM.so
	$(LINK) -o M -L. $(LDFLAGS) -lM $(LDFLAGS) $(GUI_LIBS) $(LIBS)

libM.so:
	$(CXX) -shared $(MOBJS) $(LDFLAGS) $(NONPLIBS) -o libM.so

# build the binary which is statically linked to wxGTK, dynamically to
# everything else
quartstatic: subdirs
	$(LINK) -o M $(MOBJS) $(LDFLAGS) $(GUI_LIBS) $(LIBS_STATIC)

# build the binary which is statically linked to wxGTK and GTK+, dynamically
# to everything else
semistatic: subdirs
	$(LINK) -o M $(MOBJS) $(LDFLAGS) $(LIBS_STATIC) $(GUI_LIBS_STATIC)

# build a (completely) statically linked binary
static: subdirs
	$(LINK) -static -o M.static $(LDFLAGS) $(MOBJS) $(LIBS) -lcrypt

subdirs:
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i all; done

doc:
	$(MAKE) -C ../include $@

clean:
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done
	$(RM) core libM.so

dep depend:
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done

install: M
	strip M
	$(INSTALL_PROGRAM) M $(DATADIR)/$(CANONICAL_HOST)/bin
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

msgcat:
	set -e; for i in $(SUB_DIRS) ; do $(MAKE) -C $$i $@; done
#	$(MAKE) -C $(BUILDDIR)/locale mergecat

.PHONY: all dep clean doc semistatic static allclean install msgcat subdirs



