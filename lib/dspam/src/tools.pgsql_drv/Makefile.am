# $Id: Makefile.am,v 1.5 2005/09/24 17:53:51 jonz Exp $
# tools.pgsql_drv/Makefile.am
#

EXTRA_DIST = pgsql_objects.sql virtual_users.sql purge.sql 

AM_CPPFLAGS = \
  -DLOGDIR=\"$(logdir)\" -DCONFIG_DEFAULT=\"$(sysconfdir)/dspam.conf\" \
  -D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS -D_GNU_SOURCE -I$(top_srcdir)

CFLAGS = @CFLAGS@ -I$(top_srcdir)/src

bin_PROGRAMS = dspam_pg2int8

DEFS = @DEFS@ -DLOGDIR=\"$(logdir)\" \
       -DCONFIG_DEFAULT=\"$(sysconfdir)/dspam.conf\"\
       -D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS -D_GNU_SOURCE

dspam_pg2int8_SOURCES = dspam_pg2int8.c ../read_config.c ../config_api.c
dspam_pg2int8_LDFLAGS = -static
if DYNAMIC_DRIVER
if BUILD_PGSQL_DRV 
dspam_pg2int8_CPPFLAGS = $(libpgsql_drv_cppflags) 
dspam_pg2int8_LDFLAGS += $(libpgsql_drv_libs) $(libpgsql_drv_ldflags)
endif
endif

LDADD = ../libdspam.la

