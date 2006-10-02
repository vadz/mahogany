# $Id: mysql_drv.m4,v 1.2 2006/05/13 13:28:30 jonz Exp $
# Autoconf macros for checking for MySQL
# Jonathan Zdziarski <jonathan@nuclearelephant.com>
#
#   Public available macro:
#       DS_MYSQL([mysql_cppflags_out],
#                [mysql_libs_out],
#                [mysql_ldflags_out]
#                [additional-action-if-found],
#                [additional-action-if-not-found]
#                )
#
#   Another macros are considered as private for implementation and
#   sould not be used in the configure.ac.  At this time these are:
#       DS_MYSQL_HEADERS()
#       DS_MYSQL_LIBS()
#

#   DS_MYSQL_HEADERS([mysql_cppflags_out],
#                  [additional-action-if-found],
#                  [additional-action-if-not-found]
#                 )
AC_DEFUN([DS_MYSQL_HEADERS],
[
mysql_headers_save_CPPFLAGS="$CPPFLAGS"
mysql_headers_CPPFLAGS=''
mysql_headers_success=yes

#
#   virtual users
#
AC_ARG_ENABLE(virtual-users,
    [AS_HELP_STRING([--enable-virtual-users],
                    [Cause mysql_drv to generate virtual uids for each user])])
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


AC_ARG_WITH(mysql-includes,
    [AS_HELP_STRING([--with-mysql-includes=DIR],
                    [Where to find Mysql headers])])
AC_MSG_CHECKING([where to find MySQL headers])
if test x"$with_mysql_includes" = x
then
    AC_MSG_RESULT([compiler default paths])
else
    AC_MSG_RESULT([$with_mysql_includes])
    if test -d "$with_mysql_includes"
    then
        :
    else
        AC_MSG_ERROR([required include path for mysql headers $with_mysql_includes is not a directory])
    fi
    mysql_headers_CPPFLAGS="-I$with_mysql_includes"
    CPPFLAGS="$mysql_headers_CPPFLAGS $CPPFLAGS"
fi
AC_CHECK_HEADER([mysql.h],
                [],
                [ mysql_headers_success=no ])
if test x"$mysql_headers_success" = xyes
then
    AC_PREPROC_IFELSE([AC_LANG_SOURCE([[
    #include <mysql.h>
    #ifdef PROTOCOL_VERSION
    /* Success */
    #else
    #error Unsupported version of MySQL 
    #endif
            ]])],
            [],
            [
                AC_MSG_FAILURE([Unsupported version of MySQL (no PROTOCOL_VERSION defined)])
                mysql_headers_success=no
            ])
fi

CPPFLAGS="$mysql_headers_save_CPPFLAGS"
if test x"$mysql_headers_success" = xyes
then
    ifelse([$1], [], [:], [$1="$mysql_headers_CPPFLAGS"])
    ifelse([$2], [], [:], [$2])
else
    ifelse([$3], [], [:], [$3])
fi
])

#
#   DS_MYSQL_LIBS([mysql_ldflags_out], [mysql_libs_out],
#                  [additional-action-if-found],
#                  [additional-action-if-not-found]
#                 )
AC_DEFUN([DS_MYSQL_LIBS],
[
mysql_libs_save_LDFLAGS="$LDFLAGS"
mysql_libs_save_LIBS="$LIBS"
mysql_libs_LDFLAGS=''
mysql_libs_LIBS=''
mysql_libs_success=no
mysql_libs_netlibs=''

success=yes
                                                                                
AC_SEARCH_LIBS([gzopen], [z], [success=yes], [success=no])
if test x"$success" = xno
then
  AC_MSG_ERROR([zlib required for mysql_drv])
fi

AC_ARG_WITH(mysql-libraries,
    [AS_HELP_STRING([--with-mysql-libraries=DIR],
                    [Where to find MySQL])])
AC_MSG_CHECKING([where to find MySQL libraries])
if test x"$with_mysql_libraries" = x
then
    AC_MSG_RESULT([compiler default paths])
else
    AC_MSG_RESULT([$with_mysql_libraries])
    if test -d "$with_mysql_libraries"
    then
        :
    else
        AC_MSG_ERROR([required path for mysql libraries ($with_mysql_libraries) is not a directory])
    fi
    mysql_libs_LDFLAGS="-L$with_mysql_libraries"
fi

DS_NETLIBS([mysql_libs_netlibs],
           [mysql_libs_success=yes],
           [mysql_libs_success=no])
if test x"$mysql_libs_success" = xyes
then
    AC_MSG_CHECKING([for mysql_init in -lmysqlclient])
    mysql_libs_LIBS="-lmysqlclient $mysql_libs_netlibs -lm -lz"
    LIBS="$mysql_libs_LIBS $mysql_libs_save_LIBS"
    LDFLAGS="$mysql_libs_LDFLAGS $mysql_libs_save_LDFLAGS"

    AC_LINK_IFELSE([AC_LANG_PROGRAM([[
            #include <stdlib.h>
            #include <mysql.h>
        ]],
        [[
            MYSQL *mysql = mysql_init(NULL);
        ]])],
        [ mysql_libs_success=yes ],
        [ mysql_libs_success=no ]
        )
    AC_MSG_RESULT([$mysql_libs_success])
fi
LIBS="$mysql_libs_save_LIBS"
LDFLAGS="$mysql_libs_save_LDFLAGS"
if test x"$mysql_libs_success" = xyes
then
    ifelse([$1], [], [:], [$1="$mysql_libs_LDFLAGS"])
    ifelse([$2], [], [:], [$2="$mysql_libs_LIBS"])
    ifelse([$3], [], [:], [$3])
else
    ifelse([$4], [], [:], [$4])
fi
])

#
#   DS_MYSQL([db_cppflags_out], [db_ldflags_out], [db_libs_out],
#                  [additional-action-if-found],
#                  [additional-action-if-not-found]
#                 )
AC_DEFUN([DS_MYSQL],
[
mysql_save_CPPFLAGS="$CPPFLAGS"
mysql_save_LDFLAGS="$LDFLAGS"
mysql_save_LIBS="$LIBS"

mysql_CPPFLAGS=''
mysql_LIBS=''

mysql_success=yes

DS_MYSQL_HEADERS([mysql_CPPFLAGS], [], [mysql_success=no])

if test x"$mysql_success" = xyes
then
    CPPFLAGS="$mysql_CPPFLAGS $CPPFLAGS"
    DS_MYSQL_LIBS([mysql_LDFLAGS], [mysql_LIBS], [], [mysql_success=no])
fi

CPPFLAGS="$mysql_save_CPPFLAGS"
LDFLAGS="$mysql_save_LDFLAGS"
LIBS="$mysql_save_LIBS"

if test x"$mysql_success" = xyes
then
    ifelse([$1], [], [:], [$1="$mysql_CPPFLAGS"])
    ifelse([$2], [], [:], [$2="$mysql_LDFLAGS"])
    ifelse([$3], [], [:], [$3="$mysql_LIBS"])
    ifelse([$4], [], [:], [$4])
else
    ifelse([$5], [], [:], [$5])
fi
])
