dnl $Id$
dnl
dnl    configure.ac
dnl    Jonathan A. Zdziarski <jonathan@nuclearelephant.com>
dnl
dnl    autoconf (GNU Autoconf) 2.59
dnl    ltmain.sh (GNU libtool) 1.5.6
dnl    automake (GNU automake) 1.9.2
dnl
AC_PREREQ(2.59)
AC_COPYRIGHT([Copyright (C) 2002-2006 Jonathan A. Zdziarski])
AC_INIT([dspam],[CVS],[dspam-dev@nuclearelephant.com])
CONFIGURE_ARGS=$@
AC_CONFIG_SRCDIR(./src)

AM_INIT_AUTOMAKE([1.7 foreign])

AC_CANONICAL_HOST

#---------------------------------------------------------
#	C language relative
AC_LANG(C)
AC_PROG_CC
#AC_PROG_CPP
AC_PROG_LIBTOOL
AC_PROG_MAKE_SET

AC_C_INLINE

AC_DEFINE_UNQUOTED(CONFIGURE_ARGS, ["$CONFIGURE_ARGS"],
    [Configure Arguments])

# Check for various flavors of varargs macros
AC_MSG_CHECKING([for ISO C99 varargs macros in C])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
        [[
            int a(int, int, int);
#           define call_a(...) a(1,__VA_ARGS__)
        ]],
        [[ call_a(2,3); ]])
    ],  dnl AC_LANG_PROGRAM() end
    [
        # action if-found
        AC_MSG_RESULT([yes])
        AC_DEFINE([HAVE_ISO_VARARGS], 1,
                  [Define if you have ISO C99 vararg macros])
    ],
    [
        # action if-not-found
        AC_MSG_RESULT([no])
    ])

#---------------------------------------------------------
#   Libc features
#
AC_CHECK_HEADERS(string.h)
AC_CHECK_HEADERS(math.h)
AC_CHECK_HEADERS(sys/time.h)
AC_HEADER_TIME
AC_CHECK_FUNCS(strlcpy)
AC_CHECK_FUNCS(strsep)
AC_CHECK_FUNCS(strtok_r)
AC_CHECK_FUNCS(getpwnam_r)
AC_CHECK_FUNCS(getpwuid_r)
AC_CHECK_FUNCS([memcpy memset])
AC_CHECK_FUNCS(localtime_r)
AC_CHECK_FUNCS(inet_ntoa_r)
AC_CHECK_FUNCS(strcasestr)

LIBS="-lm"

#   Daemon support
#
AC_ARG_ENABLE(daemon,
    [AS_HELP_STRING(--enable-daemon,
                        Enable support for DSPAM to run in --daemon mode
                    )])
AC_MSG_CHECKING([whether to enable daemon mode])
case x"$enable_daemon" in
    xyes)   # daemon output enabled explicity
            ;;
    xno)    # daemon output disabled explicity
            ;;
    x)      # daemon output disabled by default
            enable_daemon=no
            ;;
    *)      AC_MSG_ERROR([unexpected value $enable_daemon for --{enable,disable}-daemon configure option])
            ;;
esac
if test x"$enable_daemon" != xyes
then
    enable_daemon=no
else
    enable_daemon=yes    # overkill, but convenient
    AC_DEFINE(DAEMON, 1, [Defined if daemon support is enabled])
fi
AC_MSG_RESULT([$enable_daemon])

#
#   Determine how to compile with pthreads support
#
if test x"$enable_daemon" = xyes
then
  DS_PTHREADS(PT_CPPFLAGS, PT_LDFLAGS, PT_LIBS, [success=yes], [success=no])
  if test x"$success" != xyes
  then
    AC_MSG_FAILURE([unable to determine how to compile with pthreads])
  fi
fi

CPPFLAGS="$CPPFLAGS $PT_CPPFLAGS"
LDFLAGS="$LDFLAGS $PT_LDFLAGS"
LIBS="$LIBS $PT_LIBS"

#
#   Network related libraries on Solaris is another game...
#   Needed for dspam_ngstat and mysql driver.
#
AC_CHECK_HEADERS([sys/types.h sys/socket.h netinet/in.h arpa/inet.h])

DS_NETLIBS(NETLIBS, [success=yes], [success=no])
if test x"$success" != xyes
then
    AC_MSG_FAILURE([required libraries are not found or not usable])
fi
AC_SUBST(NETLIBS)

#
#   Dynamic linker libraries (usually -ldl)
#
DS_DLLIBS(DLLIBS, [success=yes], [success=no])
if test x"$success" != xyes
then
    AC_MSG_FAILURE([required libraries are not found or not usable])
fi
LIBS="$LIBS $DLLIBS"

#
#   Support for NodalCore(tm) C-Series Hardware Extensions
#
DS_NCORE()
DS_LDAP()

AM_CONDITIONAL([BUILD_NCORE], [ test x$build_ncore = xyes ])

#---------------------------------------------------------
#   DSPAM features
#

#
#   DSPAM_HOME -- directory where pre-user dictionaries, signatures,
#              etc. should be stored.
#
#   This variable is AC_SUBST'ed, not AC_DEFINE'd because used not only
#   for C-code, but for Makefile rules also.
#
### dspam_home='$(prefix)/var/dspam'
dspam_home='$(prefix)/var/dspam'
AC_ARG_WITH(dspam_home,
    [AS_HELP_STRING(--with-dspam-home=DIR,
                        [Specify directory where per-user dictionaries,
                        signatures, etc. should be stored.]
                        @<:@default: /var/dspam@:>@
                    )],
            [ dspam_home="$withval" ])
AC_SUBST(dspam_home)
AC_MSG_CHECKING([for dspam_home])
AC_MSG_RESULT([$dspam_home])

if test -d /var/dspam 
then
  AC_MSG_WARN([the default location for dspam_home has changed. please be sure to update your configuration or filesystem to reflect this.])
fi

# DSPAM_HOME ownership
AC_ARG_WITH(dspam_home_mode,
    [AS_HELP_STRING(--with-dspam-home-mode=MODE,
                        Set access mode for DSPAM_HOME[,] default 770
                    )],
    [ dspam_home_mode="$withval" ],
    [ dspam_home_mode='770' ]
    )
AC_ARG_WITH(dspam_home_owner,
    [AS_HELP_STRING(--with-dspam-home-owner=OWNER,
                        Set owner for DSPAM_HOME[,] default root
                    )],
    [ dspam_home_owner="$withval" ],
    [ dspam_home_owner='root' ]
    )
AC_ARG_WITH(dspam_home_group,
    [AS_HELP_STRING(--with-dspam-home-group=GROUP,
                        Set group for DSPAM_HOME[,] default mail
                    )],
    [ dspam_home_group="$withval" ],
    [ dspam_home_group='mail' ]
    )
AC_SUBST(dspam_home_mode)
AC_SUBST(dspam_home_owner)
AC_SUBST(dspam_home_group)

AC_ARG_WITH(logfile,
    [AS_HELP_STRING(--with-logfile=LOGFILE,
                        Set flat logfile pathname for logging
                    )],
    [ AC_DEFINE_UNQUOTED(LOGFILE, ["$withval"], [Logfile]) ],
    [ logfile='' ]
    )

# dspam binary ownership
case x"$host" in
    *-freebsd*)     default_dspam_mode=4510;;
    *)              default_dspam_mode=2510;;
esac

case "$host" in
    *-linux*)       LDFLAGS="$LDFLAGS -rdynamic";;
    *-freebsd*)     LDFLAGS="$LDFLAGS -rdynamic";;
esac

AC_ARG_WITH(dspam_mode,
    [AS_HELP_STRING(--with-dspam-mode=MODE,
                        Set access mode for dspam binary[,] default 2510
                    )],
    [ dspam_mode="$withval" ],
    [ dspam_mode="$default_dspam_mode" ]
    )
AC_ARG_WITH(dspam_owner,
    [AS_HELP_STRING(--with-dspam-owner=OWNER,
                        Set owner for dspam binary[,] default root
                    )],
    [ dspam_owner="$withval" ],
    [ dspam_owner='root' ]
    )
AC_ARG_WITH(dspam_group,
    [AS_HELP_STRING(--with-dspam-group=GROUP,
                        Set group for dspam binary[,] default mail
                    )],
    [ dspam_group="$withval" ],
    [ dspam_group='mail' ]
    )
AC_SUBST(dspam_mode)
AC_SUBST(dspam_owner)
AC_SUBST(dspam_group)

#
#   Local delivery Agent
#
AC_MSG_CHECKING([which delivery agent to activate in dspam.conf])
default_delivery_agent="/usr/bin/procmail"
case "$host" in
    *-linux*)       default_delivery_agent="/usr/bin/procmail";;
    *-freebsd*)     default_delivery_agent="/usr/libexec/mail.local";;
    *-solaris2*)    default_delivery_agent="/usr/bin/mail";;
    *-cygwin*)      default_delivery_agent="/usr/bin/procmail";;
esac
AC_ARG_WITH(delivery_agent,
    [AS_HELP_STRING(--with-delivery-agent=BINARY @<:@ARGS@:>@,
                        Override the default delivery agent.  This sets only
                        the default[,] which may be changed in dspam.conf.
                    )],
    [ delivery_agent="$withval" ],
    [ delivery_agent="$default_delivery_agent" ]
    )
AC_MSG_RESULT([$delivery_agent])
AC_SUBST(delivery_agent)

#
#   Trusted User Security
#
AC_ARG_ENABLE(trusted_user_security,
    [AS_HELP_STRING(--disable-trusted-user-security,Disable trusted user security (enabled by default))])
AC_MSG_CHECKING([whether to enable trusted user security])
case x"$enable_trusted_user_security" in
    xyes)   # trusted user security enabled explicity
            ;;
    xno)    # trusted user security disabled explicity
            enable_trusted_user_security=no
            ;;
    x)      # trusted user security enabled by default
            enable_trusted_user_security=yes
            ;;
    *)      AC_MSG_ERROR([unexpected value $enable_trusted_user_security for --{enable,disable}-trusted-user-security configure option])
            ;;
esac
if test x"$enable_trusted_user_security" != xyes
then
    enable_trusted_user_security=no
else
    enable_trusted_user_security=yes    # overkill, but convenient
    AC_DEFINE(TRUSTED_USER_SECURITY, 1,
              [Defined if trusted user security are enabled])
fi
AC_MSG_RESULT([$enable_trusted_user_security])

#           
#   Clam/AV Support
#
AC_ARG_ENABLE(clamav,
    [AS_HELP_STRING(--enable-clamav,
                        Enable Clam/AV support for DSPAM.
                    )])
AC_MSG_CHECKING([whether to enable clamav])
case x"$enable_clamav" in
    xyes)   # clamav output enabled explicity
            ;;
    xno)    # clamav output disabled explicity
            ;;
    x)      # clamav output disabled by default
            enable_clamav=no
            ;;   
    *)      AC_MSG_ERROR([unexpected value $enable_clamav for --{enable,disable}-clamav configure option])
            ;;
esac
if test x"$enable_clamav" != xyes
then
    enable_clamav=no
else
    enable_clamav=yes    # overkill, but convenient
    AC_DEFINE(CLAMAV, 1, [Defined if clamav is enabled])
fi
AC_MSG_RESULT([$enable_clamav])

#
#   Debug support
#
AC_ARG_ENABLE(debug,
    [AS_HELP_STRING(--enable-debug,
                        Enable debugging support for DSPAM.
                        Don't enable this unless something needs testing!
                    )])
AC_MSG_CHECKING([whether to enable debug output])
case x"$enable_debug" in
    xyes)   # debug output enabled explicity
            ;;
    xno)    # debug output disabled explicity
            ;;
    x)      # debug output disabled by default
            enable_debug=no
            ;;
    *)      AC_MSG_ERROR([unexpected value $enable_debug for --{enable,disable}-debug configure option])
            ;;
esac
if test x"$enable_debug" != xyes
then
    enable_debug=no
else
    enable_debug=yes    # overkill, but convenient
    AC_DEFINE(DEBUG, 1, [Defined if debug output is enabled])
fi
AC_MSG_RESULT([$enable_debug])

#
#   Enable syslog
#
AC_ARG_ENABLE(syslog,
    [AS_HELP_STRING(--enable-syslog,
                        Enable (or disable) syslog support
                    )])
AC_MSG_CHECKING([whether to enable syslog output])
case x"$enable_syslog" in
    xyes)   # enabled explicity
            ;;
    xno)    # disabled explicity
            ;;
    x)      # enabled by default
            enable_syslog=yes
            ;;
    *)      AC_MSG_ERROR([unexpected value $enable_syslog for --{enable,disable}-syslog configure option])
            ;;
esac
if test x"$enable_syslog" != xyes
then
    enable_syslog=no
else
    enable_syslog=yes    # overkill, but convenient
    AC_DEFINE(USE_SYSLOG, 1, [Defined if syslog is enabled])
fi
AC_MSG_RESULT([$enable_syslog])

#
#   BNR Debug
#
AC_ARG_ENABLE(bnr-debug,
    [AS_HELP_STRING(--enable-bnr-debug,
                        Activates debugging output for Bayesian Noise Reduction
                    )])
AC_MSG_CHECKING([whether to enable debug output for BNR])
case x"$enable_bnr_debug" in
    xyes)   # enabled explicity
            ;;
    xno)    # disabled explicity
            ;;
    x)      # disabled by default
            enable_bnr_debug=no
            ;;
    *)      AC_MSG_ERROR([unexpected value $enable_bnr_debug for --{enable,disable}-bnr-debug configure option])
            ;;
esac
if test x"$enable_bnr_debug" != xyes
then
    enable_bnr_debug=no
else
    enable_bnr_debug=yes    # overkill, but convenient
    AC_DEFINE(LIBBNR_DEBUG, 1, [Defined if BNR Debug is enabled])
fi
AC_MSG_RESULT([$enable_bnr_debug])


#
#   Homedir support
#
AC_ARG_ENABLE(homedir,
    [AS_HELP_STRING(--enable-homedir,
                        Enable home directory filesystem storage.
                    )])
AC_MSG_CHECKING([whether to enable home directory support])
case x"$enable_homedir" in
    xyes)   # homedir enabled explicity
            ;;
    xno)    # homedir disabled explicity
            ;;
    x)      # homedir disabled by default
            enable_homedir=no
            ;;
    *)      AC_MSG_ERROR([unexpected value $enable_homedir for --{enable,disable}-homedir configure option])
            ;;
esac
if test x"$enable_homedir" != xyes
then
    enable_homedir=no
else
    enable_homedir=yes    # overkill, but convenient
    AC_DEFINE(HOMEDIR, 1, [Defined if home directory support is enabled])
fi
AC_MSG_RESULT([$enable_homedir])

#
#   MySQL4 Initialization
#
AC_ARG_ENABLE(mysql4-initialization,
    [AS_HELP_STRING(--disable-mysql4-initialization,
                        Performs Initialization and Cleanup of MySQL client
                        libraries v4.1+.
                    )])
AC_MSG_CHECKING([whether to enable MySQL client initialization])
case x"$enable_mysql4_initialization" in
    xyes)   # enabled explicity
            enable_mysql4_initialization=yes
            ;;
    xno)    # disabled explicity
            enable_mysql4_initialization=no
            ;;
    x)      # disabled by default
            enable_mysql4_initialization=yes
            ;;
    *)      AC_MSG_ERROR([unexpected value $enable_mysql4_initialization for --{enable,disable}-mysql4-initialization configure option])
            ;;
esac
if test x"$enable_mysql4_initialization" != xyes
then
    enable_mysql4_initialization=no
else
    enable_mysql4_initialization=yes    # overkill, but convenient
    AC_DEFINE(MYSQL4_INITIALIZATION, 1, [Defined if mysql4-initialization is enabled])
fi
AC_MSG_RESULT([$enable_mysql4_initialization])

#
#   Preferences Extension
#   For storage drivers with preferences extension, the driver's extension is
#   used instead of the built-in file-based function
#
AC_ARG_ENABLE(preferences-extension,
    [AS_HELP_STRING(--enable-preferences-extension,
                        Enable if storage driver supports preferences extension
                    )])
AC_MSG_CHECKING([whether to enable preferences-extension])
case x"$enable_preferences_extension" in
    xyes)   # enabled explicity
            ;;
    xno)    # disabled explicity
            ;;
    x)      # disabled by default
            enable_preferences_extension=no
            ;;
    *)      AC_MSG_ERROR([unexpected value $enable_preferences_extension for --{enable,disable}-preferences-extension configure option])
            ;;
esac
if test x"$enable_preferences_extension" != xyes
then
    enable_preferences_extension=no
else
    enable_preferences_extension=yes    # overkill, but convenient
    AC_DEFINE(PREFERENCES_EXTENSION, 1, [Defined if storage driver supports preferences extension])
fi
AC_MSG_RESULT([$enable_preferences_extension])

#
#   Verbose debug output
#
AC_ARG_ENABLE(verbose-debug,
    [AS_HELP_STRING(--enable-verbose-debug,
                        Cause DSPAM produce verbose debug output and
                        write them into LOGDIR/dspam.debug file.
                        Implies '--enable-debug'.
                        Never enable this for production builds!
                    )])
AC_MSG_CHECKING([whether to enable verbose debug output])
case x"$enable_verbose_debug" in
    xyes)   # debug output enabled explicity
            ;;
    xno)    # debug output disabled explicity
            ;;
    x)      # debug output disabled by default
            enable_verbose_debug=no
            ;;
    *)      AC_MSG_ERROR([unexpected value $enable_verbose_debug for --{enable,disable}-verbose-debug configure option])
            ;;
esac
if test x"$enable_verbose_debug" != xyes
then
    enable_verbose_debug=no
else
    enable_verbose_debug=yes    # overkill, but convenient
    AC_DEFINE(DEBUG, 1, [Defined if debug output is enabled])
    AC_DEFINE(VERBOSE, 1, [Defined if verbose debug output is enabled])
fi
AC_MSG_RESULT([$enable_verbose_debug])

#
#   Long Usernames
#
AC_ARG_ENABLE(long-usernames,
    [AS_HELP_STRING(--enable-long-usernames,
                        Cause DSPAM to support long usernames.  The default
                        is to use the same length on usernames as the operating
                        system.
                    )])
AC_MSG_CHECKING([whether to enable long usernames])
case x"$enable_long_usernames" in
    xyes)   # debug output enabled explicity
            ;;
    xno)    # debug output disabled explicity
            ;;
    x)      # debug output disabled by default
            enable_long_usernames=no
            ;;
    *)      AC_MSG_ERROR([unexpected value $enable_long_usernames for --{enable,disable}-long-usernames configure option])
            ;;
esac
if test x"$enable_long_usernames" != xyes
then
    enable_long_usernames=no
else
    enable_long_usernames=yes    # overkill, but convenient
    AC_DEFINE(LONG_USERNAMES, 1, [Defined if long usernames is enabled])
fi
AC_MSG_RESULT([$enable_long_usernames])

#
#   Large scale implementation
#
AC_ARG_ENABLE(large-scale,
    [AS_HELP_STRING(--enable-large-scale,Manage file structure for a large scale implementation)])
AC_MSG_CHECKING([whether to enable large scale implementation])
case x"$enable_large_scale" in
    xyes)   # large-scale enabled explicitly
            ;;
    xno)    # large-scale disabled explicitly
            ;;
    x)      # large-scale disabled by default
            enable_large_scale=no
            ;;
    *)      AC_MSG_ERROR([unexpected value $enable_large_scale for --{enable,disable}-large-scale configure option])
            ;;
esac
if test x"$enable_large_scale" != xyes
then
    enable_large_scale=no
else
    enable_large_scale=yes    # overkill, but convenient
    AC_DEFINE(LARGESCALE, 1, [Defined if large_scale is enabled])
fi
AC_MSG_RESULT([$enable_large_scale])

#
#   Domain Scale Filesystem
#
AC_ARG_ENABLE(domain-scale,
    [AS_HELP_STRING(--enable-domain-scale,Manage file structure to support a domain implementation)])
AC_MSG_CHECKING([whether to enable domain structure implementation])
case x"$enable_domain_scale" in
    xyes)   # domain-scale enabled explicitly
            ;;
    xno)    # domain-scale disabled explicitly
            ;;
    x)      # domain-scale disabled by default
            enable_domain_scale=no
            ;;
    *)      AC_MSG_ERROR([unexpected value $enable_domain_scale for --{enable,disable}-domain-scale configure option])
            ;;
esac
if test x"$enable_domain_scale" != xyes
then
    enable_domain_scale=no
else
    enable_domain_scale=yes    # overkill, but convenient
    AC_DEFINE(DOMAINSCALE, 1, [Defined if domain_scale is enabled])
fi
AC_MSG_RESULT([$enable_domain_scale])

if test x"$enable_domain_scale$enable_large_scale" = xyesyes
then
  AC_MSG_ERROR([you may not have domain-scale and large-scale implemented simultaneously])
fi

#
#   Log Directory
#

logdir="$dspam_home/log"
AC_ARG_WITH(logdir,
    [AS_HELP_STRING(--with-logdir=DIR,
                        Specify directory where logfiles are to be written.
                        @<:@default: $dspam_home/log@:>@
                    )],
            [ logdir="$withval" ])
AC_SUBST(logdir)
AC_MSG_CHECKING([for logdir])
AC_MSG_RESULT([$logdir])

#---------------------------------------------------------
#   storage driver selection
storagedrv=hash_drv
storage_drv_subdirs=""
active_driver=""
static_driver=yes
AC_ARG_WITH(storage-driver,
   [AS_HELP_STRING(--with-storage-driver=LIST,Specify storage drivers to enable. @<:@default: hash_drv@:>@)],
    [ storagedrv="$withval" ])

AC_MSG_CHECKING([which storage drivers to build])
AC_MSG_RESULT([$storagedrv])

#---------------------------------------------------------
#   Libraries for storage drivers
#

ac_save_IFS=$IFS; IFS=,
for drv in $storagedrv; do
  IFS=$ac_save_IFS

  if test x"$active_driver" = x
  then
    active_driver=$drv
    AC_SUBST(active_driver)
  else
    static_driver=no
  fi

if test x"$drv" = xhash_drv
then
  hash_drv=yes
  storage_drv_subdirs="$storage_drv_subdirs tools.hash_drv"
fi

if test x"$drv" = xmysql_drv
then
    DS_MYSQL(MYSQL_CPPFLAGS, MYSQL_LDFLAGS, MYSQL_LIBS,
             [success=yes], [success=no])
    if test x"$success" != xyes
    then
        AC_MSG_ERROR([Required version of libmysqlclient not found])
    fi
    libmysql_drv_cppflags="$MYSQL_CPPFLAGS $CPPFLAGS"
    libmysql_drv_libs="$MYSQL_LIBS $LIBS" 
    libmysql_drv_ldflags="$MYSQL_LDFLAGS $LDFLAGS"

    AC_SUBST(libmysql_drv_cppflags)
    AC_SUBST(libmysql_drv_libs)
    AC_SUBST(libmysql_drv_ldflags)
    mysql_drv=yes
    storage_drv_subdirs="$storage_drv_subdirs tools.mysql_drv"
fi

if test x"$drv" = xpgsql_drv
then
    DS_PGSQL(PGSQL_CPPFLAGS, PGSQL_LDFLAGS, PGSQL_LIBS,
             [success=yes], [success=no])
    if test x"$success" != xyes
    then
        AC_MSG_ERROR([Required version of libpq not found])
    fi
    libpgsql_drv_cppflags="$PGSQL_CPPFLAGS $CPPFLAGS"
    libpgsql_drv_libs="$PGSQL_LIBS $LIBS"
    libpgsql_drv_ldflags="$PGSQL_LDFLAGS $LDFLAGS"
    AC_SUBST(libpgsql_drv_cppflags)
    AC_SUBST(libpgsql_drv_libs)
    AC_SUBST(libpgsql_drv_ldflags)
    pgsql_drv=yes
    storage_drv_subdirs="$storage_drv_subdirs tools.pgsql_drv"
fi
                                                                                
if test x"$drv" = xsqlite_drv
then
    DS_SQLITE(SQLITE_CPPFLAGS, SQLITE_LDFLAGS, SQLITE_LIBS,
              [sqlite_version_major], [], [],
              [success=yes], [success=no])
    if test x"$success" != xyes
    then
        AC_MSG_ERROR([Required version of libsqlite not found])
    fi
    if test x"$sqlite_version_major" != x2
    then
        AC_MSG_ERROR([Version mismatch: sqlite_drv requires libsqlite version 2])
    fi

    libsqlite_drv_cppflags="$SQLITE_CPPFLAGS $CPPFLAGS"
    libsqlite_drv_libs="$SQLITE_LIBS $LIBS"
    libsqlite_drv_ldflags="$SQLITE_LDFLAGS $LDFLAGS"
    AC_SUBST(libsqlite_drv_cppflags)
    AC_SUBST(libsqlite_drv_libs)
    AC_SUBST(libsqlite_drv_ldflags)
    sqlite_drv=yes
    storage_drv_subdirs="$storage_drv_subdirs tools.sqlite_drv"
fi

if test x"$drv" = xsqlite3_drv
then
    DS_SQLITE3(SQLITE3_CPPFLAGS, SQLITE3_LDFLAGS, SQLITE3_LIBS,
              [sqlite_version_major], [], [],
              [success=yes], [success=no])
    if test x"$success" != xyes
    then
        AC_MSG_ERROR([Required version of libsqlite not found])
    fi
    if test x"$sqlite_version_major" != x3
    then
        AC_MSG_ERROR([Version mismatch: sqlite3_drv requires libsqlite version 3])
    fi

    libsqlite3_drv_cppflags="$SQLITE3_CPPFLAGS $CPPFLAGS"
    libsqlite3_drv_libs="$SQLITE3_LIBS $LIBS"
    libsqlite3_drv_ldflags="$SQLITE3_LDFLAGS $LDFLAGS"
    AC_SUBST(libsqlite3_drv_cppflags)
    AC_SUBST(libsqlite3_drv_libs)
    AC_SUBST(libsqlite3_drv_ldflags)
    sqlite3_drv=yes
    storage_drv_subdirs="$storage_drv_subdirs tools.sqlite_drv"
fi
done

AM_CONDITIONAL([BUILD_DB4_DRV], [ test x"$db4_drv" = xyes ])
AM_CONDITIONAL([BUILD_DB3_DRV], [ test x"$db3_drv" = xyes ])
AM_CONDITIONAL([BUILD_HASH_DRV], [ test x"$hash_drv" = xyes ])
AM_CONDITIONAL([BUILD_SQLITE_DRV], [ test x"$sqlite_drv" = xyes ])
AM_CONDITIONAL([BUILD_SQLITE3_DRV], [ test x"$sqlite3_drv" = xyes ])
AM_CONDITIONAL([BUILD_MYSQL_DRV], [ test x"$mysql_drv" = xyes ])
AM_CONDITIONAL([BUILD_PGSQL_DRV], [ test x"$pgsql_drv" = xyes ])
AM_CONDITIONAL([STATIC_DRIVER], [ test x"$static_driver" = xyes ])
AM_CONDITIONAL([DYNAMIC_DRIVER], [ test x"$static_driver" = xno ])

AC_MSG_CHECKING([whether we are building a single, static storage driver])

if test x"$static_driver" = xyes
then
AC_DEFINE([STATIC_DRIVER], 1,
          [Define if using a single, statically linked storage driver])
AC_MSG_RESULT([yes])
CPPFLAGS="$CPPFLAGS $libmysql_drv_cppflags $libpgsql_drv_cppflags $libsqlite_drv_cppflags $libsqlite3_drv_cppflags $libhash_drv_cppflags"
LIBS="$LIBS $libmysql_drv_libs $libpgsql_drv_libs $libsqlite_drv_libs $libsqlite3_drv_libs $libhash_drv_libs"
LDFLAGS="$LDFLAGS $libmysql_drv_ldflags $libpgsql_drv_ldflags $libsqlite_drv_ldflags $libsqlite3_drv_ldflags $libhash_drv_ldflags"
else
AC_MSG_RESULT([no])
fi
 
AC_SUBST(storage_drv_subdirs)

if test x"$storage_drv_subdirs" = x
then
    AC_MSG_ERROR([no valid storage drivers specified])
fi

if test x"$enable_daemon" = xyes && test x"$mysql_drv" != xyes && test x"$pgsql_drv" != xyes && test x"$hash_drv" != xyes
then
  AC_MSG_ERROR([daemon mode requires one of the following storage drivers: mysql_drv pgsql_drv])
fi


#---------------------------------------------------------
#   Libraries (other)
#

#---------------------------------------------------------
#   Documentation generation tools
#
### AC_CHECK_PROG([HAVE_DOXYGEN], [doxygen], [yes], [no])
### AC_CHECK_PROG([HAVE_DOT], [dot], [yes], [no])
### AM_CONDITIONAL([HAVE_DOXYGEN], [ test x"$HAVE_DOXYGEN" = xyes ])

#----------------------------------------------------------
# final cut
#
GCC_BUILD_OPTIONS

AC_CONFIG_HEADERS(src/auto-config.h)
AH_TOP([
#ifndef __auto_config_h
#define __auto_config_h
])
AH_BOTTOM([
#endif /* !__auto_config_h */
])

AC_CONFIG_FILES([Makefile src/tools/Makefile webui/Makefile m4/Makefile])
AC_CONFIG_FILES([src/tools.mysql_drv/Makefile])
AC_CONFIG_FILES([txt/Makefile src/tools.pgsql_drv/Makefile ])
AC_CONFIG_FILES([src/Makefile webui/cgi-bin/templates/Makefile])
AC_CONFIG_FILES([src/tools.sqlite_drv/Makefile man/Makefile])
AC_CONFIG_FILES([src/dspam.pc src/dspam-uninstalled.pc])
AC_CONFIG_FILES([doc/Makefile src/tools.hash_drv/Makefile scripts/Makefile])
AC_CONFIG_FILES([webui/cgi-bin/Makefile webui/htdocs/Makefile])
AC_OUTPUT

