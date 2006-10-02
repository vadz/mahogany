# $Id: pgsql_drv.m4,v 1.3 2006/05/13 13:28:30 jonz Exp $
# Autuconf macroses for checking for PostgreSQL
# Rustam Aliyev <rustam@azernews.com>
# Jonathan Zdziarski <jonathan@nuclearelephant.com>
#
#   Public available macro:
#       DS_PGSQL([pgsql_cppflags_out],
#                [pgsql_libs_out],
#                [pgsql_ldflags_out]
#                [additional-action-if-found],
#                [additional-action-if-not-found]
#                )
#
#   Another macros are considered as private for implementation and
#   sould not be used in the configure.ac.  At this time these are:
#       DS_PGSQL_HEADERS()
#       DS_PGSQL_LIBS()
#

#   DS_PGSQL_HEADERS([pgsql_cppflags_out],
#                  [additional-action-if-found],
#                  [additional-action-if-not-found]
#                 )
AC_DEFUN([DS_PGSQL_HEADERS],
[
pgsql_headers_save_CPPFLAGS="$CPPFLAGS"
pgsql_headers_CPPFLAGS=''
pgsql_headers_success=yes

#
#   virtual users
#
AC_ARG_ENABLE(virtual-users,
    [AS_HELP_STRING([--enable-virtual-users],
                    [Cause pgsql_drv to generate virtual uids for each user])])
AC_MSG_CHECKING([whether to enable virtual users])
case x"$enable_virtual_users" in
    xyes)   # enabled explicity
            ;;
    xno)    # disabled explicity
            ;;
    x)      # disabled by default
            enable_virtual_users=no
            ;;
    *)      AC_MSG_ERROR([unexpected value $enable_virtual_users for --{enable,disable}-virtual-users configure option])
            ;;
esac
if test x"$enable_virtual_users" != xyes
then
    enable_virtual_users=no
else
    enable_virtual_users=yes    # overkill, but convenient
    AC_DEFINE(VIRTUAL_USERS, 1, [Defined if homedir dotfiles is enabled])
fi
AC_MSG_RESULT([$enable_virtual_users])


AC_ARG_WITH(pgsql-includes,
    [AS_HELP_STRING([--with-pgsql-includes=DIR],
                    [Where to find PostgreSQL headers])])
AC_MSG_CHECKING([where to find PostgreSQL headers])
if test x"$with_pgsql_includes" = x
then
    AC_MSG_RESULT([compiler default paths])
else
    AC_MSG_RESULT([$with_pgsql_includes])
    if test -d "$with_pgsql_includes"
    then
        :
    else
        AC_MSG_ERROR([required include path for libpq headers $with_pgsql_includes is not a directory])
    fi
    pgsql_headers_CPPFLAGS="-I$with_pgsql_includes"
    CPPFLAGS="$pgsql_headers_CPPFLAGS $CPPFLAGS"
fi
AC_CHECK_HEADER([libpq-fe.h],
                [],
                [ pgsql_headers_success=no ])
if test x"$pgsql_headers_success" = xyes
then
    AC_PREPROC_IFELSE([AC_LANG_SOURCE([[
    #include <libpq-fe.h>
    #ifdef LIBPQ_FE_H
    /* Success */
    #else
    #error Unsupported version of PgSQL 
    #endif
            ]])],
            [],
            [
                AC_MSG_FAILURE([Unsupported version of PostgreSQL (no LIBPQ_FE_H defined)])
                pgsql_headers_success=no
            ])
fi

CPPFLAGS="$pgsql_headers_save_CPPFLAGS"
if test x"$pgsql_headers_success" = xyes
then
    ifelse([$1], [], [:], [$1="$pgsql_headers_CPPFLAGS"])
    ifelse([$2], [], [:], [$2])
else
    ifelse([$3], [], [:], [$3])
fi
])

#
#   DS_PGSQL_LIBS([pgsql_ldflags_out], [pgsql_libs_out],
#                  [additional-action-if-found],
#                  [additional-action-if-not-found]
#                 )
AC_DEFUN([DS_PGSQL_LIBS],
[
pgsql_libs_save_LDFLAGS="$LDFLAGS"
pgsql_libs_save_LIBS="$LIBS"
pgsql_libs_LDFLAGS=''
pgsql_libs_LIBS=''
pgsql_libs_success=no
pgsql_freemem_success=no
pgsql_libs_netlibs=''

AC_ARG_WITH(pgsql-libraries,
    [AS_HELP_STRING([--with-pgsql-libraries=DIR],
                    [Where to find PostgreSQL libraries])])
AC_MSG_CHECKING([where to find PostgreSQL libraries])
if test x"$with_pgsql_libraries" = x
then
    AC_MSG_RESULT([compiler default paths])
else
    AC_MSG_RESULT([$with_pgsql_libraries])
    if test -d "$with_pgsql_libraries"
    then
        :
    else
        AC_MSG_ERROR([required path for libpq libraries ($with_pgsql_libraries) is not a directory])
    fi
    pgsql_libs_LDFLAGS="-L$with_pgsql_libraries"
fi

DS_NETLIBS([pgsql_libs_netlibs],
           [pgsql_libs_success=yes],
           [pgsql_libs_success=no])
if test x"$pgsql_libs_success" = xyes
then
    AC_MSG_CHECKING([for PQconnectdb in libpq])
    pgsql_libs_LIBS="-lpq $pgsql_libs_netlibs"
    LIBS="$pgsql_libs_LIBS $pgsql_libs_save_LIBS"
    LDFLAGS="$pgsql_libs_LDFLAGS $pgsql_libs_save_LDFLAGS"
    
    AC_LINK_IFELSE([AC_LANG_PROGRAM([[
            #include <stdlib.h>
            #include <libpq-fe.h>
        ]],
        [[
            PGconn *pgsql = PQconnectdb(NULL);
        ]])],
        [ pgsql_libs_success=yes ],
        [ pgsql_libs_success=no ]
        )
    AC_MSG_RESULT([$pgsql_libs_success])

fi

if test x"$pgsql_libs_success" = xyes
then
    AC_MSG_CHECKING([if this version of Postgres supports PQfreemem])
    AC_LINK_IFELSE([AC_LANG_PROGRAM([[
            #include <stdlib.h>
            #include <libpq-fe.h>
        ]],
        [[
            PQfreemem(NULL);
        ]])],
        [ pgsql_freemem_success=yes ],
        [ pgsql_freemem_success=no ]
        )
    AC_MSG_RESULT([$pgsql_freemem_success])
fi

if test x"$pgsql_freemem_success" = xyes
then
  AC_DEFINE([HAVE_PQFREEMEM], 1, [Defined if PQfreemem is supported])
fi

LIBS="$pgsql_libs_save_LIBS"
LDFLAGS="$pgsql_libs_save_LDFLAGS"
if test x"$pgsql_libs_success" = xyes
then
    ifelse([$1], [], [:], [$1="$pgsql_libs_LDFLAGS"])
    ifelse([$2], [], [:], [$2="$pgsql_libs_LIBS"])
    ifelse([$3], [], [:], [$3])
else
    ifelse([$4], [], [:], [$4])
fi
])

#
#   DS_PGSQL([db_cppflags_out], [db_ldflags_out], [db_libs_out],
#                  [additional-action-if-found],
#                  [additional-action-if-not-found]
#                 )
AC_DEFUN([DS_PGSQL],
[
pgsql_save_CPPFLAGS="$CPPFLAGS"
pgsql_save_LDFLAGS="$LDFLAGS"
pgsql_save_LIBS="$LIBS"

pgsql_CPPFLAGS=''
pgsql_LIBS=''

pgsql_success=yes

DS_PGSQL_HEADERS([pgsql_CPPFLAGS], [], [pgsql_success=no])

if test x"$pgsql_success" = xyes
then
    CPPFLAGS="$pgsql_CPPFLAGS $CPPFLAGS"
    DS_PGSQL_LIBS([pgsql_LDFLAGS], [pgsql_LIBS], [], [pgsql_success=no])
fi

CPPFLAGS="$pgsql_save_CPPFLAGS"
LDFLAGS="$pgsql_save_LDFLAGS"
LIBS="$pgsql_save_LIBS"

if test x"$pgsql_success" = xyes
then
    ifelse([$1], [], [:], [$1="$pgsql_CPPFLAGS"])
    ifelse([$2], [], [:], [$2="$pgsql_LDFLAGS"])
    ifelse([$3], [], [:], [$3="$pgsql_LIBS"])
    ifelse([$4], [], [:], [$4])
else
    ifelse([$5], [], [:], [$5])
fi
])
