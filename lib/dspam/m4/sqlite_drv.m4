# $Id: sqlite_drv.m4,v 1.2 2006/05/13 13:28:30 jonz Exp $
# Autuconf macros for checking for SQLite
# Jonathan Zdziarski <jonathan@nuclearelephant.com>
#
#   Public available macro:
#       DS_SQLITE([sqlite_cppflags_out],
#                      [sqlite_ldflags_out],
#                      [sqlite_libs_out],
#                      [sqlite_version_major_out],
#                      [sqlite_version_minor_out],
#                      [sqlite_version_patchlevel_out],
#                      [additional-action-if-found],
#                      [additional-action-if-not-found]
#                      )
#
#   Another macros are considered as private for implementation and
#   sould not be used in the configure.ac.  At this time these are:
#       DS_SQLITE_HEADERS()
#       DS_SQLITE_LIBS()
#

#   DS_SQLITE_HEADERS([sqlite_cppflags_out],
#                  [sqlite_version_major_out],
#                  [sqlite_version_minor_out],
#                  [sqlite_version_patchelevel_out],
#                  [additional-action-if-found],
#                  [additional-action-if-not-found]
#                 )
AC_DEFUN([DS_SQLITE_HEADERS],
[
AC_REQUIRE([AC_PROG_AWK])

ds_sqlite_headers_save_CPPFLAGS="$CPPFLAGS"
ds_sqlite_headers_CPPFLAGS=''
ds_sqlite_headers_success=yes
ds_sqlite_headers_version_major=''
ds_sqlite_headers_version_minor=''
ds_sqlite_headers_version_patchlevel=''

# unistd.h and errno.h are needed for header version check below.
AC_CHECK_HEADERS([unistd.h errno.h])

AC_ARG_WITH(sqlite-includes,
            [AS_HELP_STRING([--with-sqlite-includes=DIR],
                            [Where to find SQLite headers])])
if test x"$with_sqlite_includes" != x
then
    if test -d "$with_sqlite_includes"
    then
        :
    else
        AC_MSG_ERROR([required include path for sqlite headers ($with_sqlite_includes) is not a directory])
    fi
    ds_sqlite_headers_CPPFLAGS="-I$with_sqlite_includes"
    CPPFLAGS="$ds_sqlite_headers_CPPFLAGS $CPPFLAGS"
fi
AC_CHECK_HEADER([sqlite.h],
                [],
                [ ds_sqlite_headers_success=no ])
if test x"$ds_sqlite_headers_success" = xyes
then
    # Determine SQLite hearder version
    AC_LANG_PUSH(C)
    AC_MSG_CHECKING([SQLite header version])
    AC_RUN_IFELSE([AC_LANG_SOURCE([[
        #include <sqlite.h>
        #include <stdio.h>
        #ifdef HAVE_UNISTD_H
        #   include <unistd.h>
        #endif
        #ifdef HAVE_ERRNO_H
        #   include <errno.h>
        #endif

        #define OUTFILE "conftest.libsqlitever"

        int main(void)
        {
            FILE* fd;
            int rc;

            rc = unlink(OUTFILE);   /* avoid symlink attack */
            if (rc < 0 && errno != ENOENT)
            {
                fprintf(stderr, "error unlinking '%s'", OUTFILE);
                exit(1);
            }

            fd = fopen(OUTFILE, "w");
            if (!fd)
            {
                /* Don't try to investigate errno for portability reasons */
                fprintf(stderr, "error opening '%s' for writing", OUTFILE);
                exit(1);
            }

            rc = fprintf(fd, "%s", SQLITE_VERSION);
            if (rc < 0)
            {
                fprintf(stderr, "error writing to the '%s'", OUTFILE);
                exit(1);
            }
            exit(0);
        }
        ]])],
        [
            dnl In following AWK calls `$[1]' is used instead of `$1'
            dnl for preventing substitution by macro arguments.  Don't
            dnl worry, in final `configure' these `$[1]' will appears as
            dnl required: `$1'.

            ds_sqlite_headers_verstr=`cat conftest.libsqlitever`
            ds_sqlite_headers_version_major=`cat conftest.libsqlitever | $AWK -F. '{print $[1]}'`
            ds_sqlite_headers_version_minor=`cat conftest.libsqlitever | $AWK -F. '{print $[2]}'`
            ds_sqlite_headers_version_patchlevel=`cat conftest.libsqlitever | $AWK -F. '{print $[3]}'`

            AC_MSG_RESULT([$ds_sqlite_headers_version_major.$ds_sqlite_headers_version_minor.$ds_sqlite_headers_version_patchlevel])
        ],
        [
            AC_MSG_RESULT([failure (unsupported version?)])
            ds_sqlite_headers_success=no
        ],
        [   # cross-compilation
            AC_MSG_ERROR([cross-compilation is unsupported, sorry])
            ds_sqlite_headers_success=no
        ])
    AC_LANG_POP(C)
fi
CPPFLAGS="$ds_sqlite_headers_save_CPPFLAGS"
if test x"$ds_sqlite_headers_success" = xyes
then
    ifelse([$1], [], [:], [$1="$ds_sqlite_headers_CPPFLAGS"])
    ifelse([$2], [], [:], [$2="$ds_sqlite_headers_version_major"])
    ifelse([$3], [], [:], [$3="$ds_sqlite_headers_version_minor"])
    ifelse([$4], [], [:], [$4="$ds_sqlite_headers_version_patchlevel"])
    ifelse([$5], [], [:], [$5])
    :
else
    ifelse([$6], [], [:], [$6])
    :
fi
])

#
#   DS_SQLITE_LIBS([sqlite_ldflags_out],
#                  [sqlite_libs_out],
#                  [sqlite_version_major_in],
#                  [sqlite_version_minor_in],
#                  [sqlite_version_patchelevel_in],     # unused
#                  [additional-action-if-found],
#                  [additional-action-if-not-found]
#                 )
AC_DEFUN([DS_SQLITE_LIBS],
[
ds_sqlite_libs_save_LIBS="$LIBS"
ds_sqlite_libs_save_LDFLAGS="$LDFLAGS"
ds_sqlite_libs_LIBS=''
ds_sqlite_libs_LDFLAGS=''
ds_sqlite_libs_success=no

ds_sqlite_libs_ver_major="${$3}"
ds_sqlite_libs_ver_minor="${$4}"

if test x"$ds_sqlite_libs_ver_major" = x
then
    AC_MSG_ERROR([[DS@&t@_SQLITE_LIBS: non-optional argument _ds_sqlite_libs_version_major_in is omited]]);
fi
if test x"$ds_sqlite_libs_ver_minor" = x
then
    AC_MSG_ERROR([[DS@&t@_SQLITE_LIBS: non-optional argument _ds_sqlite_libs_version_minor_in is omited]]);
fi

AC_ARG_WITH(sqlite-libraries,
            [AS_HELP_STRING([--with-sqlite-libraries=DIR],
                            [Where to find SQLite libraries])])
if test x"$with_sqlite_libraries" != x
then
    if test -d "$with_sqlite_libraries"
    then
        :
    else
        AC_MSG_ERROR([required path for sqlite libraries ($with_sqlite_libraries) is not a directory])
    fi
    ds_sqlite_libs_LDFLAGS="-L$with_sqlite_libraries"
fi

AC_MSG_CHECKING([how to link SQLite libraries])
for ds_sqlite_libs_tmp_sqlite in \
    "-lsqlite"
do
    LDFLAGS="$ds_sqlite_libs_LDFLAGS $ds_sqlite_libs_save_LDFLAGS"
    for ds_sqlite_libs_tmp_libpth in '' 
    do
        ds_sqlite_libs_LIBS="$ds_sqlite_libs_tmp_sqlite $ds_sqlite_libs_tmp_libpth"
        LIBS="$ds_sqlite_libs_LIBS $ds_sqlite_libs_save_LIBS"
        AC_LINK_IFELSE([AC_LANG_PROGRAM([[
            #include <sqlite.h>
            ]],
            [[
              const char *v = sqlite_version;
            ]])],
            [ ds_sqlite_libs_success=yes ],
            [ ds_sqlite_libs_success=no ]
            )

        if test x"$ds_sqlite_libs_success" != xyes
        then
            continue
        fi

        DS_LIBTOOL_RUN_IFELSE([AC_LANG_PROGRAM([[
            #include <stdio.h>
            #include <sqlite.h>
            #include <stdlib.h>
            #include <string.h>
            ]],
            [[
                const char *x = sqlite_version;
                char *y;
                char header_version[16];
                int major, minor, patchlevel;
                int hmajor, hminor, hpatchlevel;
                int is_match;
                                                                                
                strcpy(header_version, x);
                y = strtok(header_version, ".");
                major = atoi(y);
                y = strtok(NULL, ".");
                minor = atoi(y);
                y = strtok(NULL, ".");
                patchlevel = atoi(y);
                                                                                
                strcpy(header_version, SQLITE_VERSION);
                y = strtok(header_version, ".");
                hmajor = atoi(y);
                y = strtok(NULL, ".");
                hminor = atoi(y);
                y = strtok(NULL, ".");
                hpatchlevel = atoi(y);
                                                                                
                fprintf(stderr, "sqlite version from header: %d.%d.%d\n",
                        hmajor, hminor, hpatchlevel
                        );
                                                                                
                fprintf(stderr, "sqlite version from library: %d.%d.%d\n",
                        major,
                        minor,
                        patchlevel
                        );
                if (major == hmajor
                    && minor == hminor
                    && patchlevel == hpatchlevel
                    )
                {
                    is_match = 1;
                }
                else
                {
                    is_match = 0;
                }
                return is_match ? 0 : 1;
            ]])],
            [ ds_sqlite_libs_success=yes],
            [ ds_sqlite_libs_success=no ],
            [ ds_sqlite_libs_success=yes]  # Assume success for cross-compiling
            )

        if test x"$ds_sqlite_libs_success" = xyes
        then
            break 2
        fi
    done
done

if test x"$ds_sqlite_libs_success" = xyes
then
    AC_MSG_RESULT([$ds_sqlite_libs_LIBS])
else
    AC_MSG_RESULT([failure])
fi

LIBS="$ds_sqlite_libs_save_LIBS"
LDFLAGS="$ds_sqlite_libs_save_LDFLAGS"
if test x"$ds_sqlite_libs_success" = xyes
then
    ifelse([$1], [], [:], [$1="$ds_sqlite_libs_LDFLAGS"])
    ifelse([$2], [], [:], [$2="$ds_sqlite_libs_LIBS"])
    ifelse([$6], [], [:], [$6]);
    :
else
    ifelse([$7], [], [:], [$7])
    :
fi
])

#
#   DS_SQLITE([sqlite_cppflags_out],
#                  [sqlite_ldflags_out],
#                  [sqlite_libs_out],
#                  [sqlite_version_major_out],
#                  [sqlite_version_minor_out],
#                  [sqlite_version_patchlevel_out],
#                  [additional-action-if-found],
#                  [additional-action-if-not-found]
#                 )
AC_DEFUN([DS_SQLITE],
[
ds_sqlite_save_CPPFLAGS="$CPPFLAGS"
ds_sqlite_save_LIBS="$LIBS"
ds_sqlite_save_LDFLAGS="$LDFLAGS"

ds_sqlite_CPPFLAGS=''
ds_sqlite_LIBS=''
ds_sqlite_LDFLAGS=''

ds_sqlite_success=yes
ds_sqlite_version_major=''
ds_sqlite_version_minor=''
ds_sqlite_version_patchlevel=''

DS_SQLITE_HEADERS([ds_sqlite_CPPFLAGS],
    [ds_sqlite_version_major],
    [ds_sqlite_version_minor],
    [ds_sqlite_version_patchlevel],
    [],
    [ds_sqlite_success=no])

if test x"$ds_sqlite_success" = xyes
then
    CPPFLAGS="$ds_sqlite_CPPFLAGS $CPPFLAGS"
    DS_SQLITE_LIBS([ds_sqlite_LDFLAGS],
        [ds_sqlite_LIBS],
        [ds_sqlite_version_major],
        [ds_sqlite_version_minor],
        [ds_sqlite_version_patchlevel],
        [],
        [ds_sqlite_success=no])
fi

CPPFLAGS="$ds_sqlite_save_CPPFLAGS"
LIBS="$ds_sqlite_save_LIBS"
LDFLAGS="$ds_sqlite_save_LDFLAGS"

if test x"$ds_sqlite_success" = xyes
then
    ifelse([$1], [], [:], [$1="$ds_sqlite_CPPFLAGS"])
    ifelse([$2], [], [:], [$2="$ds_sqlite_LDFLAGS"])
    ifelse([$3], [], [:], [$3="$ds_sqlite_LIBS"])

    ifelse([$4], [], [:], [$4="$ds_sqlite_version_major"])
    ifelse([$5], [], [:], [$5="$ds_sqlite_version_minor"])
    ifelse([$6], [], [:], [$6="$ds_sqlite_version_patchlevel"])

    ifelse([$7], [], [:], [$7])
    :
else
    ifelse([$8], [], [:], [$8])
    :
fi
])
