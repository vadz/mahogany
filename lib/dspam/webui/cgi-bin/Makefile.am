# $Id: Makefile.am,v 1.2 2005/07/02 04:15:55 jonz Exp $
#

bin_SCRIPTS = configure_pl
CLEANFILES = configure.pl

EXTRA_DIST =            \
	dspam.cgi       \
	graph.cgi       \
	default.prefs   \
	rgb.txt         \
	admin.cgi       \
	admingraph.cgi  \
	admins          \
	configure.pl.in

SUBDIRS = templates

sed_substitute_variables = \
	dspam_transformed=`echo dspam | sed '$(transform)'`; \
	sed -e 's,@dspam_home\@,$(dspam_home),g' \
	-e 's,@bindir\@,$(bindir),g'

configure_pl: configure.pl.in Makefile
	${sed_substitute_variables} < $(srcdir)/configure.pl.in > configure.pl

