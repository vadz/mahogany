# $Id#
# tools.hash_drv/Makefile.am
#

EXTRA_DIST = 

AM_CPPFLAGS = \
  -DLOGDIR=\"$(logdir)\" -DCONFIG_DEFAULT=\"$(sysconfdir)/dspam.conf\" \
  -D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS -D_GNU_SOURCE -I$(top_srcdir)

CFLAGS = @CFLAGS@ -I$(top_srcdir)/src

bin_PROGRAMS = cssstat csscompress cssclean cssconvert

DEFS = @DEFS@ -DLOGDIR=\"$(logdir)\" \
       -DCONFIG_DEFAULT=\"$(sysconfdir)/dspam.conf\"\
       -D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS -D_GNU_SOURCE

cssstat_SOURCES =  cssstat.c ../read_config.c ../util.c ../diction.c \
                   ../nodetree.c ../hash_drv.c ../error.c ../config_shared.c \
                   ../pref.c
cssstat_LDFLAGS = -static
if DYNAMIC_DRIVER
if BUILD_HASH_DRV
cssstat_CPPFLAGS = $(libhash_drv_cppflags)
cssstat_LDFLAGS += $(libhash_drv_libs) $(libhash_drv_ldflags)
endif
endif

cssclean_SOURCES = cssclean.c ../read_config.c ../util.c ../diction.c \
                   ../nodetree.c ../hash_drv.c ../error.c ../config_shared.c \
                   ../pref.c
cssclean_LDFLAGS = -static
if DYNAMIC_DRIVER
if BUILD_HASH_DRV
cssclean_CPPFLAGS = $(libhash_drv_cppflags)
cssclean_LDFLAGS += $(libhash_drv_libs) $(libhash_drv_ldflags)
endif
endif

cssconvert_SOURCES = cssconvert.c ../read_config.c ../util.c ../diction.c \
                   ../nodetree.c ../hash_drv.c ../error.c ../config_shared.c \
                   ../pref.c
cssconvert_LDFLAGS = -static
if DYNAMIC_DRIVER
if BUILD_HASH_DRV
cssconvert_CPPFLAGS = $(libhash_drv_cppflags)
cssconvert_LDFLAGS += $(libhash_drv_libs) $(libhash_drv_ldflags)
endif
endif

csscompress_SOURCES = csscompress.c ../read_config.c ../util.c ../diction.c \
                   ../nodetree.c ../hash_drv.c ../error.c ../config_shared.c \
                   ../pref.c
csscompress_LDFLAGS = -static
if DYNAMIC_DRIVER
if BUILD_HASH_DRV
csscompress_CPPFLAGS = $(libhash_drv_cppflags)
csscompress_LDFLAGS += $(libhash_drv_libs) $(libhash_drv_ldflags)
endif
endif
