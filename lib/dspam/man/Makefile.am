# $Id: Makefile.am,v 1.3 2006/05/14 15:37:30 jonz Exp $
#
# Makefile.am
# Jonathan A. Zdziarski <jonathan@nuclearelephant.com>
# Andrew W. Nosenko <awn@bcs.zp.ua>
#
# autoconf (GNU Autoconf) 2.59
# ltmain.sh (GNU libtool) 1.5.6
# automake (GNU automake) 1.9.2

includedir=@includedir@/dspam

SUBDIRS = .
DIST_SUBDIRS = .

man_MANS = dspam.1 dspam_stats.1 dspam_train.1 dspam_clean.1 dspam_dump.1 \
  dspam_merge.1 libdspam.3

EXTRA_DIST = dspam.1 dspam_stats.1 dspam_train.1 dspam_clean.1 dspam_dump.1 \
  dspam_merge.1 libdspam.3

MAINTAINERCLEANFILES = Makefile.in 

DEFS = @DEFS@ 

# Failing of chgrp is expected and normal case when someone do
#   make install DESTDIR=...
# from user other than root (from user not included in the `mail' group
# is exactly), and only after this and after checking what was installed
# changes ownership.
#
# Caveat: creating the $(dspam_home) in the install-exec-hook is not the
# right way, but for some (unknown to me) reason `installdirs-local'
# target have no chances to be executed under automake-1.7.3 (at least).
#
# Solaris' ln seems to have a problem with -sf and existing symlinks.
install-data-hook:
	rm -f $(DESTDIR)$(mandir)/man3/dspam_init.3
	rm -f $(DESTDIR)$(mandir)/man3/dspam_create.3
	rm -f $(DESTDIR)$(mandir)/man3/dspam_addattribute.3
	rm -f $(DESTDIR)$(mandir)/man3/dspam_attach.3
	rm -f $(DESTDIR)$(mandir)/man3/dspam_process.3
	rm -f $(DESTDIR)$(mandir)/man3/dspam_getsource.3
	rm -f $(DESTDIR)$(mandir)/man3/dspam_destroy.3
	rm -f $(DESTDIR)$(mandir)/man3/dspam_detach.3
	ln -s $(mandir)/man3/libdspam.3 $(DESTDIR)$(mandir)/man3/dspam_init.3
	ln -s $(mandir)/man3/libdspam.3 $(DESTDIR)$(mandir)/man3/dspam_create.3
	ln -s $(mandir)/man3/libdspam.3 $(DESTDIR)$(mandir)/man3/dspam_addattribute.3
	ln -s $(mandir)/man3/libdspam.3 $(DESTDIR)$(mandir)/man3/dspam_attach.3
	ln -s $(mandir)/man3/libdspam.3 $(DESTDIR)$(mandir)/man3/dspam_process.3
	ln -s $(mandir)/man3/libdspam.3 $(DESTDIR)$(mandir)/man3/dspam_getsource.3
	ln -s $(mandir)/man3/libdspam.3 $(DESTDIR)$(mandir)/man3/dspam_destroy.3
	ln -s $(mandir)/man3/libdspam.3 $(DESTDIR)$(mandir)/man3/dspam_detach.3

uninstall-hook:
	rm -f $(DESTDIR)$(mandir)/man3/dspam_init.3
	rm -f $(DESTDIR)$(mandir)/man3/dspam_create.3
	rm -f $(DESTDIR)$(mandir)/man3/dspam_addattribute.3
	rm -f $(DESTDIR)$(mandir)/man3/dspam_attach.3
	rm -f $(DESTDIR)$(mandir)/man3/dspam_process.3
	rm -f $(DESTDIR)$(mandir)/man3/dspam_getsource.3
	rm -f $(DESTDIR)$(mandir)/man3/dspam_destroy.3
	rm -f $(DESTDIR)$(mandir)/man3/dspam_detach.3

ACLOCAL_AMFLAGS = -I m4

