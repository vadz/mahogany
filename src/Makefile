# Makefile for src directory
# $Id$

include ../makeopts

# use sub-make for M so wildcard dependencies are expanded correctly
all:	subdirs
	@$(MAKE) --no-print-directory M

# List all directories which create modules (.so files) to be installed:
#
MOD_DIRS:= modules adb

SUB_DIRS:= $(MOD_DIRS) util classes gui mail wx
UTILOBJ := util/strutil.o util/sysutil.o util/miscutil.o util/upgrade.o \
           util/twofish2.o
MOBJS   = $(UTILOBJ) classes/*.o gui/*.o mail/*.o adb/*.o \
	wx/generic/*.o wx/common/*.o wx/vcard/*.o

ifdef USE_PYTHON
SUB_DIRS += Python
MOBJS += Python/*.o
endif

ifeq ($(USE_MODULES), static)
MOBJS += modules/*.o
endif

# LeakTracer must be the last object on the linker line!
ifdef USE_LEAKTRACER
MOBJS += util/LeakTracer.o
endif

M:	$(MOBJS)
	$(LINK) -o M $(MOBJS) $(LDFLAGS) $(GUI_LIBS) $(LIBS)

Mdll:	libM.so
	$(LINK) -o M -L. $(LDFLAGS) -lM 

libM.so: subdirs
	$(CXX) -shared -Xlinker -z -Xlinker nodefs $(MOBJS) $(LDFLAGS) $(NONPLIBS) $(LDFLAGS) $(GUI_LIBS) $(LIBS) -o libM.so

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
	$(LINK) -static -o M.static $(LDFLAGS) $(MOBJS) $(LIBS)

subdirs:
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i all; done

doc:
	@true

clean:
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done
	$(RM) core libM.so M M.static

install:
	$(INSTALL_PROGRAM) -s -m 755 M $(DATADIR)/$(CANONICAL_HOST)/bin
	if test -f libM.so; \
	then $(INSTALL_PROGRAM) libM.so $(DATADIR)/$(CANONICAL_HOST)/lib; \
	fi
	$(RM) -f $(BINDIR)/M $(DATADIR)/bin/M
	ln -f -s ../share/Mahogany/$(CANONICAL_HOST)/bin/M $(BINDIR)/M
	$(RM) $(BINDIR)/mahogany
	ln -f -s ../share/Mahogany/$(CANONICAL_HOST)/bin/M $(BINDIR)/mahogany
ifeq ($(USE_MODULES),dynamic)
	set -e; for i in $(MOD_DIRS); do $(MAKE) -C $$i install-modules; done
endif
	$(INSTALL) -d $(DATADIR)/icons
	for i in .src/icons/*.xpm; \
	do $(INSTALL_DATA) $$i $(DATADIR)/icons; \
	done
ifdef USE_PYTHON
	$(MAKE) -C Python $@
endif

msgcat:
	echo > $(BUILDDIR)/messages.po
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done
#	$(MAKE) -C $(BUILDDIR)/locale mergecat

.PHONY:	all clean doc semistatic static allclean install msgcat subdirs
