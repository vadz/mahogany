# $Id: Makefile.am,v 1.9 2006/05/13 13:28:30 jonz Exp $
#
# Makefile.am
# Jonathan A. Zdziarski <jonathan@nuclearelephant.com>
#
# autoconf (GNU Autoconf) 2.59
# ltmain.sh (GNU libtool) 1.5.6
# automake (GNU automake) 1.9.2

includedir=@includedir@/dspam

SUBDIRS = . webui m4 man src 
DIST_SUBDIRS = . webui m4 txt man src doc scripts

EXTRA_DIST = autogen.sh RELEASE.NOTES CHANGELOG LICENSE \
  README UPGRADING

MAINTAINERCLEANFILES = Makefile.in aclocal.m4 auto-config.h.in \
	config.guess config.sub configure depcomp install-sh   \
	ltmain.sh missing mkinstalldirs

DEFS = @DEFS@ 

ACLOCAL_AMFLAGS = -I m4

