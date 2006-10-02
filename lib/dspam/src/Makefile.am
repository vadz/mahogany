# $Id: Makefile.am,v 1.37 2006/08/28 19:13:04 jonz Exp $
#
# Makefile.am
# Jonathan A. Zdziarski <jonathan@nuclearelephant.com>
# Andrew W. Nosenko <awn@bcs.zp.ua>
#
# autoconf (GNU Autoconf) 2.59
# ltmain.sh (GNU libtool) 1.5.6
# automake (GNU automake) 1.9.2

includedir=@includedir@/dspam

SUBDIRS = . tools $(storage_drv_subdirs) 
DIST_SUBDIRS = . \
	tools.mysql_drv tools.pgsql_drv tools.sqlite_drv tools.hash_drv tools 

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = dspam.pc
etcdir = $(sysconfdir)
etc_SCRIPTS = dspam_conf
libversion = 7:0:0

# Defaults from dspam.conf

EXTRA_DIST = example.c dspam.pc.in dspam-uninstalled.pc.in dspam.conf.in 

CLEANFILES = dspam.conf

MAINTAINERCLEANFILES = Makefile.in aclocal.m4 auto-config.h.in \
	config.guess config.sub configure depcomp install-sh   \
	ltmain.sh missing mkinstalldirs

DEFS = @DEFS@ -DLOGDIR=\"$(logdir)\" \
       -DCONFIG_DEFAULT=\"$(sysconfdir)/dspam.conf\"\
       -D_THREAD_SAFE -D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS -D_GNU_SOURCE

lib_LTLIBRARIES = libdspam.la 

if DYNAMIC_DRIVER
if BUILD_HASH_DRV
lib_LTLIBRARIES += libhash_drv.la
endif
if BUILD_MYSQL_DRV
lib_LTLIBRARIES += libmysql_drv.la
endif
if BUILD_PGSQL_DRV
lib_LTLIBRARIES += libpgsql_drv.la
endif
if BUILD_SQLITE_DRV
lib_LTLIBRARIES += libsqlite_drv.la
endif
if BUILD_SQLITE3_DRV
lib_LTLIBRARIES += libsqlite3_drv.la
endif
endif

bin_PROGRAMS = dspam dspamc

# installed for libdspam
include_HEADERS = buffer.h config_shared.h decode.h error.h diction.h \
	libdspam.h libdspam_objects.h nodetree.h tokenizer.h ldap_client.h \
	storage_driver.h heap.h config.h

# libdspam.a contans objects common for dspam and tools/* binaries
libdspam_la_SOURCES = \
	config.h libdspam_objects.h \
	libdspam.c libdspam.h \
	heap.c heap.h \
	diction.c diction.h \
        base64.c base64.h \
	buffer.c buffer.h \
	util.c util.h \
	nodetree.c nodetree.h \
        error.c error.h \
	decode.c decode.h \
	pref.c pref.h \
	config_shared.c config_shared.h \
	bnr.c bnr.h \
	hash.c hash.h \
	list.c list.h \
	ldap_client.c ldap_client.h \
        tokenizer.c tokenizer.h \
	storage_driver.h

if BUILD_NCORE
libdspam_la_SOURCES += ncore_adp.c ncore_adp.h
endif

if DYNAMIC_DRIVER
libdspam_la_SOURCES += storage_driver.c
endif

EXTRA_libdspam_la_SOURCES = \
        mysql_drv.c mysql_drv.h \
        pgsql_drv.c pgsql_drv.h \
        sqlite_drv.c sqlite_drv.h \
        sqlite3_drv.c sqlite3_drv.h

#if STATIC_DRIVER
#if BUILD_HASH_DRV
#libdspam_la_SOURCES += libhash_drv.c libhash_drv.h
#endif
#if BUILD_MYSQL_DRV
#libdspam_la_SOURCES += libmysql_drv.c libmysql_drv.h
#endif
#if BUILD_PGSQL_DRV
#libdspam_la_SOURCES += libpgsql_drv.c libpgsql_drv.h
#endif
#if BUILD_SQLITE_DRV
#libdspam_la_SOURCES += libsqlite_drv.c libsqlite_drv.h
#endif
#if BUILD_SQLITE3_DRV
#libdspam_la_SOURCES += libsqlite3_drv.c libsqlite3_drv.h
#endif
#endif

libdspam_la_LIBADD = $(adapter_objects)
libdspam_la_LIBADD += -lm $(NETLIBS) $(NCORELIBS)
libdspam_la_DEPENDENCIES = $(adapter_objects)
if STATIC_DRIVER
libdspam_la_LIBADD += $(active_driver).lo
libdspam_la_DEPENDENCIES += $(active_driver).lo
endif

#   current:revision:age
libdspam_la_LDFLAGS = -rpath '$(libdir)' -version-info $(libversion)

if DYNAMIC_DRIVER

libmysql_drv_la_SOURCES = \
	mysql_drv.c mysql_drv.h 
libmysql_drv_la_LIBADD = $(libmysql_drv_libs)
libmysql_drv_la_CPPFLAGS = $(libmysql_drv_cppflags)
libmysql_drv_la_LDFLAGS = $(libmysql_drv_ldflags)
libmysql_drv_la_LDFLAGS += -rpath '$(libdir)' -version-info $(libversion) 
libmysql_drv_la_DEPENDENCIES =

libpgsql_drv_la_SOURCES = \
	pgsql_drv.c pgsql_drv.h 
libpgsql_drv_la_LIBADD = $(libpgsql_drv_libs)
libpgsql_drv_la_CPPFLAGS = $(libpgsql_drv_cppflags)
libpgsql_drv_la_LDFLAGS = $(libpgsql_drv_ldflags)
libpgsql_drv_la_LDFLAGS += -rpath '$(libdir)' -version-info $(libversion)
libpgsql_drv_la_DEPENDENCIES =

libsqlite_drv_la_SOURCES = \
	sqlite_drv.c sqlite_drv.h 
libsqlite_drv_la_LIBADD = $(libsqlite_drv_libs)
libsqlite_drv_la_CPPFLAGS = $(libsqlite_drv_cppflags)
libsqlite_drv_la_LDFLAGS = $(libsqlite_drv_ldflags)
libsqlite_drv_la_LDFLAGS += -rpath '$(libdir)' -version-info $(libversion)
libsqlite_drv_la_DEPENDENCIES =

libsqlite3_drv_la_SOURCES = \
	sqlite3_drv.c sqlite3_drv.h 
libsqlite3_drv_la_LIBADD = $(libsqlite3_drv_libs)
libsqlite3_drv_la_CPPFLAGS = $(libsqlite3_drv_cppflags)
libsqlite3_drv_la_LDFLAGS = $(libsqlite3_drv_ldflags)
libsqlite3_drv_la_LDFLAGS += -rpath '$(libdir)' -version-info $(libversion)
libsqlite3_drv_la_DEPENDENCIES =

libhash_drv_la_SOURCES = \
	hash_drv.c hash_drv.h
libhash_drv_la_LDFLAGS = -rpath '$(libdir)' 
libhash_drv_la_LDFLAGS += -version-info $(libversion)

endif

dspam_SOURCES = \
        config.h libdspam.h language.h \
        dspam.c dspam.h \
        buffer.h \
	storage_driver.h \
	read_config.c read_config.h \
	config_api.c config_api.h \
	daemon.c daemon.h \
	client.c client.h \
	agent_shared.c agent_shared.h

dspam_LDADD = libdspam.la
dspam_LDFLAGS = -static

dspamc_SOURCES = \
	config.h libdspam.h language.h \
	dspamc.c dspamc.h \
	buffer.h error.h \
	read_config.c read_config.h \
	client.c client.h \
	agent_shared.c agent_shared.h 

dspamc_LDADD = -lm buffer.o config_shared.o error.o pref.o nodetree.o util.o $(NETLIBS)
dspamc_LDFLAGS = -static

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
install-exec-hook:
	-if test ! -d $(DESTDIR)$(sysconfdir); then \
		mkdir -p $(DESTDIR)$(sysconfdir); \
		chmod 755 $(DESTDIR)$(sysconfdir); \
	fi
	-if test ! -f $(DESTDIR)$(sysconfdir)/dspam.conf; then \
		cp dspam.conf $(DESTDIR)$(sysconfdir)/dspam.conf; \
		chmod 640 $(DESTDIR)$(sysconfdir)/dspam.conf; \
		chown "$(dspam_owner)" $(DESTDIR)$(sysconfdir)/dspam.conf; \
		chgrp "$(dspam_group)" $(DESTDIR)$(sysconfdir)/dspam.conf; \
	fi
	-if test x"$(dspam_owner)" != xnone; then \
                chown "$(dspam_owner)" $(DESTDIR)$(bindir)/dspam; \
        fi
	-if test x"$(dspam_group)" != xnone; then \
                chgrp "$(dspam_group)" $(DESTDIR)$(bindir)/dspam; \
        fi
	-if test x"$(dspam_mode)" != xnone; then \
                chmod "$(dspam_mode)" $(DESTDIR)$(bindir)/dspam; \
	fi
	-if test ! -d $(DESTDIR)$(dspam_home); then \
                $(mkinstalldirs) $(DESTDIR)$(dspam_home); \
		if test x"$(dspam_home_owner)" != xnone; then \
	        	chown "$(dspam_home_owner)" $(DESTDIR)$(dspam_home); \
	        fi; \
		if test x"$(dspam_home_group)" != xnone; then \
	                chgrp "$(dspam_home_group)" $(DESTDIR)$(dspam_home); \
	        fi; \
		if test x"$(dspam_home_mode)" != xnone; then \
	                chmod "$(dspam_home_mode)" $(DESTDIR)$(dspam_home); \
	        fi; \
	fi
	-if test ! -d $(DESTDIR)$(logdir); then \
		$(mkinstalldirs) $(DESTDIR)$(logdir); \
		if test x"$(dspam_home_owner)" != xnone; then \
			chown "$(dspam_home_owner)" $(DESTDIR)$(logdir); \
		fi; \
		if test x"$(dspam_home_group)" != xnone; then \
			chgrp "$(dspam_home_group)" $(DESTDIR)$(logdir); \
		fi; \
		if test x"$(dspam_home_mode)" != xnone; then \
			chmod "$(dspam_home_mode)" $(DESTDIR)$(logdir); \
		fi; \
	fi

ACLOCAL_AMFLAGS = -I m4

sed_substitute_variables = \
        dspam_transformed=`echo dspam | sed '$(transform)'`; \
        sed -e 's,@dspam_home\@,$(dspam_home),g' \
            -e 's,@delivery_agent\@,$(delivery_agent),g' \
            -e 's,@storage_driver\@,lib$(active_driver).so,g' \
            -e 's,@libdir\@,$(libdir),g'

dspam_conf: dspam.conf.in Makefile
	${sed_substitute_variables} < $(srcdir)/dspam.conf.in > dspam.conf

