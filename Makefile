###############################################################################
# Project:     M
# File name:   Makefile
# Purpose:     the top level Makefile for Mahogany
# Author:      Mahogany Dev-Team
# Modified by:
# Created:     1998
# CVS-ID:      $Id$
# Copyright:   (c) 1997-2003 M-Team
# Licence:     M license
###############################################################################

include makeopts
include makeversion

M := mahogany-$(M_VERSION_MAJOR).$(M_VERSION_MINOR)

FILES := configure.in configure Makefile makeopts.in makerules
ifeq ($(USE_RESOURCES),yes)
SUB_DIRS := include lib res src
else
SUB_DIRS := include lib src
endif
ALL_DIRS := $(SUB_DIRS) doc

all semistatic quartstatic static:
	@set -e; for i in $(SUB_DIRS); do \
		if [ $$i = src ]; then \
			$(MAKE) -C src $@; \
		else \
			$(MAKE) -C $$i all; \
		fi \
	done

doc:
	set -e; for i in doc; do $(MAKE) -C $$i doc; done

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
	$(RM) config.* makeopts include/config.h
	find . -type l -name .src -exec rm -f {} \;
	find . -type l -name Makefile -exec rm -f {} \;
	$(RM) -f makeversion

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
	   $(DOCDIR) \
	   $(DOCDIR)/Tips \
	;do \
	if test ! -d $$i; then $(INSTALL) -d -m 755 $$i; \
	fi \
	done
	set -e; for i in $(SUB_DIRS) extra; do $(MAKE) -C $$i install; done
	set -e; for i in .src/doc/Tips/Tips*.txt; \
	do $(INSTALL_DATA) $$i $(DOCDIR)/Tips/$$i; \
	done

install_strip:
	$(MAKE) install INSTALL_OPTIONS="-s $(INSTALL_OPTIONS)"

install_bin_strip:
	$(MAKE) install_bin INSTALL_OPTIONS="-s $(INSTALL_OPTIONS)"

install_locale:
	$(MAKE) -C locale install

install_doc:
	$(MAKE) -C doc install
	set -e; for i in CHANGES COPYING CREDITS INSTALL README; \
	do $(INSTALL_DATA) .src/$$i $(DOCDIR); \
	done

install: install_bin install_locale install_doc
ifdef USE_MAC
	@echo "Mahogany.app has been built in $(top_builddir)/src,"
	@echo "now you can simply drag and drop it anywhere you want."
endif

locales:
	$(MAKE) -C locale all

mergecat:
	$(MAKE) -C locale mergecat

# create the file list for the RPM installation: used by redhat/M.spec
install_rpm:
	@echo "Creating the list of files for RPM"
	@echo "%doc $(DOCDIR)" > filelist
	@echo "%config $(DATADIR)/M.conf" >> filelist
	@echo "$(DATADIR)/scripts" >> filelist
	@echo "$(DATADIR)/newmail.wav" >> filelist
	@echo "$(DATADIR)/kylemail.wav" >> filelist
	@echo "$(DATADIR)/afm" >> filelist
	@echo "$(DATADIR)/icons" >> filelist
	@echo "$(DATADIR)/python" >> filelist
	@echo "$(DESTDIR)/share/locale" >> filelist
	@echo "$(BINDIR)/M" >> filelist
	@echo "$(BINDIR)/mahogany" >> filelist
	@echo "$(DESTDIR)/man/man1/mahogany.1" >> filelist
	@echo "$(DESTDIR)/man/man1/M.1" >> filelist
	@# the second subsitution takes care of RPM_BUILD_ROOT
	@$(PERL) -i -npe 's/^/%attr(-, root, root) /; s: /.*//: /:' filelist


.PHONY: all clean bak backup config program doc install install_doc \
        locales scandoc install_locale install_rpm classdoc
