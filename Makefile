# Makefile for M root directory
# $Id$

FILES := configure.in configure Makefile makeopts.in makerules
SUB_DIRS := include extra res src
ALL_DIRS := $(SUB_DIRS) doc

include makeopts

all semistatic quartstatic static:
	@set -e; for i in $(SUB_DIRS); do \
		if [ $$i = src ]; then \
			$(MAKE) -C src $@; \
		else \
			$(MAKE) -C $$i all; \
		fi \
	done

makeversion: .src/makeversion.in
	sh -e .src/makeversion.in

include makeversion

M := mahogany-$(M_VERSION_MAJOR).$(M_VERSION_MINOR)

doc:
	set -e; for i in extra doc; do $(MAKE) -C $$i doc; done

classdoc:
	set -e; for i in doc; do $(MAKE) -C $$i doc; done

clean:
	set -e; for i in $(ALL_DIRS); do $(MAKE) -C $$i $@; done

allclean: clean
	find . -name \*.bak -exec rm -f {} \;
	find . -name \*~ -exec rm -f {} \;
	find . -name .\\\#* -exec rm -f {} \;
	find . -name \*.mo -exec rm -f {} \;
	find . -name .libs -exec rm -rf {} \;
	find . -name .xvpics -exec rm -r -f {} \;
	$(RM) -r debian/tmp *.po config.log src/M config.status
	$(RM) config.* makeopts makeversion include/config.h
	find . -type l -name .src -exec rm -f {} \;
	find . -type l -name Makefile -exec rm -f {} \;

realclean: allclean
clobber: realclean

depend:
	@echo "-------------------------------------------------------------"
	@echo "-------------------------------------------------------------"
	@echo "\"make depend\" is obsolete; there's no need to run it any more"
	@echo "-------------------------------------------------------------"
	@echo "-------------------------------------------------------------"

bak backup:
	tar cvf $(FILES) $(ALL_DIRS) | gzip -z9 > M-`date +"%y-%m-%d"`.tar.gz


# probably the most complicated target:
#
install_bin:
	@echo "Installing M in " $(BINDIR)
	@echo "        data in " $(DATADIR)
	@echo "        docs in " $(DOCDIR)
	set -e; for i in \
	   $(BINDIR) \
	   $(DATADIR) \
	   $(DATADIR)/$(CANONICAL_HOST) \
	   $(DATADIR)/$(CANONICAL_HOST)/bin \
	   $(DATADIR)/$(CANONICAL_HOST)/lib \
	   $(DOCDIR) \
	   $(DOCDIR)/Tips \
	;do \
	if test ! -d $$i; then $(INSTALL) -d -m 755 $$i; \
	fi \
	done
	set -e; for i in $(SUB_DIRS); do $(MAKE) -C $$i install; done
	set -e; for i in .src/doc/Tips/Tips*.txt; \
	do $(INSTALL_DATA) $$i $(DOCDIR)/Tips; \
	done

install_strip:
	$(MAKE) install INSTALL_OPTIONS="-s $(INSTALL_OPTIONS)"

install_bin_strip:
	$(MAKE) install_bin INSTALL_OPTIONS="-s $(INSTALL_OPTIONS)"

install_locale:
	$(MAKE) -C locale install

install_doc:
	$(MAKE) -C doc install
	set -e; for i in TODO README; \
	do $(INSTALL_DATA) .src/$$i $(DOCDIR); \
	done

install: install_bin install_locale install_doc

locales:
	$(MAKE) -C locale all

mergecat:
	$(MAKE) -C locale mergecat

#########################################################################
# The targets below don't really belong here.  They should either be in
# a separate makefile or the logic should be moved to the spec file.
#########################################################################

# create the file list for the RPM installation
install_rpm:
	@echo "Creating the list of files for RPM"
	@echo "%doc $(DOCDIR)" > filelist
	@echo "%config $(DATADIR)/M.conf" >> filelist
	@echo "$(DATADIR)/scripts" >> filelist
	@echo "$(DATADIR)/newmail.wav" >> filelist
#	@echo "$(DATADIR)/kylemail.wav" >> filelist
	@echo "$(DATADIR)/afm" >> filelist
	@echo "$(DATADIR)/$(CANONICAL_HOST)" >> filelist
	@echo "$(DATADIR)/icons" >> filelist
	@echo "$(DATADIR)/locale" >> filelist
	@echo "$(BINDIR)/M" >> filelist
	@echo "$(BINDIR)/mahogany" >> filelist
	@echo "$(DESTDIR)/man/man1/mahogany.1" >> filelist
	@echo "$(DESTDIR)/man/man1/M.1" >> filelist
	@# the second subsitution takes care of RPM_BUILD_ROOT
	@$(PERL) -i -npe 's/^/%attr(-, root, root) /; s: /.*//: /:' filelist

# prepare the scene for building the RPM version
rpm_prep:
	if test -z "$$RPM_ROOT"; then \
		export RPM_TOP_DIR=`rpm --showrc | grep topdir | cut -d: -f 2 | sed 's/^ //'`; \
	else \
		export RPM_TOP_DIR=$(RPM_ROOT); \
	fi; \
	if test -z "$$RPM_TOP_DIR"; then \
   	echo "You must set RPM_ROOT first."; \
      exit 1; \
   fi; \
	echo "*** Preparing to build RPM in $$RPM_TOP_DIR..."; \
	cd $(SOURCEDIR)/..; \
	mv M $(M); \
	tar cvzf $$RPM_TOP_DIR/SOURCES/$(M).tar.gz \
		--exclude="*.o" --exclude="M" --exclude="CVS" \
		--exclude=".cvsignore" --exclude="*~" --exclude="*.swp" \
		--exclude="*.mo" --exclude="*.a" \
		--exclude=".sniffdir" --exclude=".depend" \
		$(M)/README $(M)/TODO $(M)/INSTALL $(M)/Makefile \
		$(M)/aclocal.m4 $(M)/configure.* \
		$(M)/doc $(M)/extra/Makefile $(M)/include $(M)/locale \
		$(M)/makeopts.in $(M)/makerules $(M)/src $(M)/extra/install \
		$(M)/extra/scripts $(M)/makeversion.in $(M)/extra/src; \
	mv $(M) M; \
	cd M; \
	cp redhat/mahogany.gif $$RPM_ROOT/SOURCES; \
	cp redhat/M.spec $$RPM_ROOT/SPECS

# build the source and binary RPMs
rpm: rpm_prep
	@if test -z "$$RPM_ROOT"; then \
		export RPM_TOP_DIR=`rpm --showrc | grep topdir | cut -d: -f 2 | sed 's/^ //'`; \
		export RPM_BUILD_ROOT=`rpm --showrc | grep buildroot | cut -d: -f 2 | sed 's/^ //'`; \
	else \
		export RPM_TOP_DIR=$(RPM_ROOT); \
		export RPM_BUILD_ROOT=$(RPM_ROOT)/ROOT; \
	fi; \
	echo "*** Building RPM under $$RPM_BUILD_ROOT..."; \
	cd $$RPM_TOP_DIR/SPECS && rpm --buildroot $$RPM_BUILD_ROOT -bb M.spec ;\
	cd $$RPM_TOP_DIR/SPECS && rpm --buildroot $$RPM_BUILD_ROOT -bs M.spec

.PHONY: all clean bak backup config program doc install install_doc \
        install_all locales scandoc install_locale rpm_prep rpm classdoc
