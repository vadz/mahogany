# Makefile for M root directory
# $Id$
#


CWD = 
SUB_DIRS = include extra src
FILES = makeopts makerules Makefile makeopts.in configure.in configure
EXTRA = extra

# FIXME: VZ where can I take the version from?
M = mahogany-0.50

include makeopts

all:
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i all; done

doc:
	set -e; for i in include extra doc ; do $(MAKE) -C $$i doc; done

clean:  
	set -e; for i in $(SUB_DIRS) doc; do $(MAKE) -C $$i $@; done

allclean: clean
	find . -name .depend -exec rm {} \;
	find . -name \*.bak -exec rm {} \;
	find . -name \*~ -exec rm {} \; 
	find . -name .\\\#* -exec rm {} \; || true
	find . -name .libs -exec rm -rf {} \; || true
	$(RM) config.status *cache* makeopts *.po config.log src/M
	find . -name .xvpics -exec rm -r -f {} \; 
	$(RM) -r debian/tmp

dep depend: 
	@echo "The first time \"make depend\" will print lots of warnings"
	@echo "about missing header files - these can be safely ignored."
	@echo "They should disappear if you run it again."
	@echo "----------------------------------------------------------"
	set -e; $(MAKE) -C include # generate some headers first
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i $@; done

config.status: 
	@echo "You should really run ./configure with appropriate arguments."
	@echo "Type ./configure --help to find out more about it."
	@echo "Running ./configure with default setup now..."
	./configure

config: configure makeopts.in
	./configure || $(RM) config.status

configure: configure.in aclocal.m4
	autoconf && $(RM) config.status

makeopts: makeopts.in config.status
	./config.status

include/config.h: include/config.h.in configure
	./configure

bak backup:
	tar cvf M-`date +"%y-%m-%d"`.tar $(SUB_DIRS) $(FILES) $(EXTRA)
	gzip -9 M-`date +"%y-%m-%d"`.tar
#
# probably the most complicated target:
#
install:
	@echo "Installing M in " $(BINDIR)
	@echo "        data in " $(DATADIR)
	@echo "        docs in " $(DOCDIR)
	$(INSTALL) -d $(DATADIR) $(BINDIR) $(DOCDIR)
	$(INSTALL) -d $(DATADIR)/$(CANONICAL_HOST)/bin
	$(INSTALL) -d $(DATADIR)/$(CANONICAL_HOST)/lib
	$(INSTALL) -d $(BINDIR)
	$(INSTALL) -d $(DATADIR)/bin
	$(INSTALL) -d $(DATADIR)/lib
	$(INSTALL) -d $(DATADIR)/doc
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i install; done

install_doc:
	$(MAKE) -C doc install
	$(INSTALL_DATA) TODO README $(DOCDIR)

# create the file list for the RPM installation
install_rpm:
	@echo "Creating the list of files for the RPM"
	@echo "%doc $(DOCDIR)" > filelist
	@echo "%config $(DATADIR)/M.conf" >> filelist
	@echo "$(DATADIR)/scripts" >> filelist
	@echo "$(DATADIR)/newmail.wav" >> filelist
	@echo "$(DATADIR)/kylemail.wav" >> filelist
	@echo "$(DATADIR)/afm" >> filelist
	@echo "$(DATADIR)/bin" >> filelist
	@echo "$(DATADIR)/$(CANONICAL_HOST)" >> filelist
	@echo "$(DATADIR)/icons" >> filelist
	@echo "$(DATADIR)/lib" >> filelist
	@echo "$(DATADIR)/locale" >> filelist
	@echo "$(BINDIR)/M" >> filelist
	@echo "$(BINDIR)/mahogany" >> filelist
	@# the second subsitution takes care of RPM_BUILD_ROOT
	@perl -i -npe 's/^/%attr(-, root, root) /; s: /.*//: /:' filelist

install_all: install install_doc install_locale

install_locale:
	$(MAKE) -C locale install

# prepare the scene for building the RPM
rpm_prep:
	@export RPM_ROOT=`rpm --showrc | grep topdir | cut -d: -f 2 | sed 's/^ //'`; \
	echo "Preparing to build the RPM under $$RPM_ROOT..."; \
	cd ..; \
	mv M $(M); \
	tar cvzf $$RPM_ROOT/SOURCES/$(M).tar.gz \
		--exclude="*.o" --exclude="M" --exclude="CVS" \
		--exclude=".cvsignore" --exclude="*~" --exclude="*.swp" \
		--exclude="Python" --exclude="*.mo" --exclude="*.a" \
		$(M)/README $(M)/TODO $(M)/INSTALL $(M)/Makefile \
		$(M)/aclocal.m4 $(M)/config.status $(M)/configure.* \
		$(M)/doc $(M)/extra/Makefile $(M)/include $(M)/locale \
		$(M)/makeopts.in $(M)/makerules $(M)/src $(M)/extra/install \
		$(M)/extra/scripts $(M)/extra/src; \
	mv $(M) M; \
	cd M; \
	cp redhat/mahogany.gif $$RPM_ROOT/SOURCES; \
	cp redhat/M.spec $$RPM_ROOT/SPECS

rpm:
	cd $(RPM_ROOT)/SPECS
	rpm -bb M.spec

msgcat:
	$(MAKE) -C src msgcat
	$(MAKE) -C include msgcat

locales:
	$(MAKE) -C locale all

.PHONY: all dep clean bak backup config program doc install install_doc \
        install_all msgcat locales scandoc install_locale

