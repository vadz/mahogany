# Makefile for src directory
# $Id$

# Our master target.  Put it here so all the intermediate targets generated
# while reading the Names.mk files won't be considered the master one.
all:	binary

# Get all the names that need to be generated.  The Names.mk file in each
# subdirectory fills in these parameters.
#
# Objects we will build to be included in the M binary
MOBJS	:=
# Modules, compiled separately or bound directly
MSOS	:=
# Source files to be scanned for translation strings
MSGSRC	:=
# Generated objects to be removed by clean
CLEAN	:=
#
include ../makeopts $(wildcard .src/*/Names.mk) $(wildcard $(patsubst .src/%/Names.mk,%/*.d,$(wildcard .src/*/Names.mk)))

# modules: MOBJS get linked in statically, MSOS installed as shared libs
ifeq ($(USE_MODULES),static)
# static, link modules directly
MOBJS	+= $(MSOS:.so=.o)
MSOS	:= 
else
ifeq ($(USE_MODULES),dynamic)
# dynamic, nothing to do
else
# none, don't compile modules
MSOS	:=
endif
endif

# LeakTracer must be the last object on the linker line!
ifdef USE_LEAKTRACER
MOBJS += util/LeakTracer.o
endif

include ../makerules

# under Mac OS we need to build a bundle and not just a single binary
ifdef USE_MAC
M_APP := M_bundle
else
M_APP := M
endif

# for cygwin/mingw we need to add resources to the main binary
# and use -mwindows in order to make GUI binary
ifdef USE_RESOURCES
M_RES := ../res/M.res -mwindows
else
M_RES :=
endif

# for cygwin with -mno-cygwin (mingw) we need to add additional libs
ifdef USE_MINGW
M_MINGW_LIBS := -lwsock32 -lwinmm
else
M_MINGW_LIBS :=
endif

# OK, we've got all the names and the rules to generate them; we can
# now actually specify the dependencies.
binary:	$(M_APP) $(MSOS)
	@true

M:	$(MOBJS)
	$(LINK) -o M $(MOBJS) $(LDFLAGS) $(GUI_LIBS) $(LIBS) $(M_MINGW_LIBS) $(M_RES)
ifdef USE_MAC
	$(REZFLAGS) $@
	$(SETFILE) -a C $@
endif

ifdef USE_MAC

include ../makeversion

BUNDLE := M.app/Contents

# Target to build a Mac OS X application bundle
M_bundle: $(BUNDLE)/MacOS/M \
        $(BUNDLE)/Info.plist \
        $(BUNDLE)/PkgInfo \
        $(BUNDLE)/Resources/M.rsrc \
        $(BUNDLE)/Resources/M.icns

$(BUNDLE)/Info.plist: $(SOURCEDIR)/macosx/Info.plist.in
	@$(INSTALL) -d `dirname $@`
	@sed "s/VERSION/$(M_VERSION_MAJOR).$(M_VERSION_MINOR).$(M_VERSION_RELEASE)/" $< > $@

$(BUNDLE)/PkgInfo:
	@$(INSTALL) -d `dirname $@`
	@echo -n "APPL????" > $@

$(BUNDLE)/MacOS/M: M
	@$(INSTALL) -d `dirname $@`
	@mv $< $@

$(BUNDLE)/Resources/M.rsrc: $(SOURCEDIR)/macosx/M.rsrc
	@$(INSTALL) -d `dirname $@`
	@cp $< $@

$(BUNDLE)/Resources/M.icns: $(SOURCEDIR)/macosx/M.icns
	@$(INSTALL) -d `dirname $@`
	@cp $< $@

endif # USE_MAC

Mdll:	libM.so
	$(LINK) -o M $(LDFLAGS) libM.so

libM.so: $(MOBJS)
	$(CXX) -o libM.so -shared -Xlinker -z -Xlinker nodefs $(MOBJS) $(LDFLAGS) $(NONPLIBS) $(LDFLAGS) $(GUI_LIBS) $(LIBS)

# build the binary which is statically linked to wxGTK, dynamically to
# everything else
quartstatic: $(MOBJS) $(MSOS) $(M_BUNDLE)
	$(LINK) -o M $(MOBJS) $(LDFLAGS) $(GUI_LIBS_QUARTSTATIC) $(LIBS)

# build the binary which is statically linked to wxGTK and GTK+, dynamically
# to everything else
semistatic: $(MOBJS) $(MSOS) $(M_BUNDLE)
	$(LINK) -o M $(MOBJS) $(LDFLAGS) $(GUI_LIBS_SEMISTATIC) $(LIBS)

# build a (completely) statically linked binary
static: $(MOBJS) $(M_BUNDLE)
	$(LINK) -o M $(MOBJS) $(LDFLAGS) -static $(GUI_LIBS_STATIC) $(LIBS)

doc:
	@true

clean:
	$(RM) core libM.so M $(CLEAN)
	$(RM) $(MOBJS) $(MSOS)
	$(RM) $(MOBJS:.o=.d) $(MSOS:.so=.d)

install:
	$(INSTALL_PROGRAM) $(INSTALL_OPTIONS) M $(BINDIR)
	$(RM) $(BINDIR)/mahogany
	ln -s M $(BINDIR)/mahogany
	if test -f libM.so; \
	then $(INSTALL_PROGRAM) libM.so $(LIBDIR); \
	fi
ifeq ($(USE_MODULES),dynamic)
	$(INSTALL) -d $(MODULEDIR)
	set -e $(MSOS); while test $$# -ge 1; \
	do $(INSTALL_PROGRAM) $$1 $(MODULEDIR); shift; done
endif
	$(INSTALL) -d $(DATADIR)/icons
	for i in .src/icons/*.xpm; \
	do $(INSTALL_DATA) $$i $(DATADIR)/icons; \
	done
ifdef USE_PYTHON
	$(INSTALL) -d $(DATADIR)/python
	for i in Python/*.py; \
		do $(INSTALL_DATA) $$i $(DATADIR)/python; \
	done
	$(INSTALL) -d $(DATADIR)/scripts
	for i in Python/.src/Scripts/*.py; \
		do $(INSTALL_DATA) $$i $(DATADIR)/scripts; \
	done
endif

# invoked from locale to build message catalog
msgcat:
ifeq "$(TRANSLATIONS)" "yes"
	cd .src; exec $(XGETTEXT) -C -k_ -o $(BUILDDIR)/messages.po $(MSGSRC)
	$(MAKE) -C ../include msgcat
else
	false translation tools are not installed
endif

.PHONY:	all clean doc quartstatic semistatic static install msgcat
