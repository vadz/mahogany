dnl $Id$
dnl local macro definitions for M's configure.in

dnl package,message,variable,default,helpmessage
AC_DEFUN([M_OVERRIDES],
[AC_MSG_CHECKING(for $2)
define([M_FUNC], regexp([$5], [--[^-]*-], \&))dnl
define([M_VAR],
	ifelse(	M_FUNC,--with-,    withval,
 		M_FUNC,--without-, withval,
 		M_FUNC,--enable-,  enableval,
 		M_FUNC,--disable-, enableval,
 		withval))dnl
define([M_FUNC],
	ifelse(M_VAR, enableval, [defn([AC_ARG_ENABLE])],
 		[defn([AC_ARG_WITH])]))dnl
M_FUNC($1,[  $5],
[case "$]M_VAR[" in
  yes) m_cv_$3=1 ;;
  no)  m_cv_$3=0 ;;
  *)   m_cv_$3="$]M_VAR[" ;;
  esac],
AC_CACHE_VAL(m_cv_$3,m_cv_$3='$4')[dnl])dnl
undefine([M_FUNC])undefine([M_VAR])dnl
$3="$m_cv_$3"
case "$m_cv_$3" in
''|0)  AC_MSG_RESULT(no) ;;
1)  AC_MSG_RESULT(yes) ;;
*)  AC_MSG_RESULT($m_cv_$3) ;;
esac])

dnl M_CHECK_MYLIB(LIBRARY, FUNCTION, LIBPATHLIST [, ACTION-IF-FOUND 
dnl            [, ACTION-IF-NOT-FOUND [, OTHER-LIBRARIES [, EXTRA-MSG]]]])

AC_DEFUN([M_CHECK_MYLIB],
[AC_MSG_CHECKING([for $2 in -l$1$7])
dnl Use a cache variable name containing both the library and function name,
dnl because the test really is for library $1 defining function $2, not
dnl just for library $1.  Separate tests with the same $1 and different $2s
dnl may have different results.
m_lib_var=`echo $1['_']$2 | sed 'y%./+-%__p_%'`
AC_CACHE_VAL(m_cv_lib_$m_lib_var,
[m_save_LIBS="$LIBS"
for j in "." $3 ; do
LIBS="-L$j -l$1 $6 $LIBS"
AC_TRY_LINK(dnl
ifelse([$2], [main], , dnl Avoid conflicting decl of main.
[/* Override any gcc2 internal prototype to avoid an error.  */
#ifdef __cplusplus
extern "C"
#endif
/* We use char because int might match the return type of a gcc2
    builtin and then its argument prototype would still apply.  */
char $2();
]),
	    [$2()],
            [eval "m_cv_lib_$m_lib_var=\"libpath_$m_lib_var=-L$j\"" 
             eval eval "`echo '$m_cv_lib_'$m_lib_var`"
             LIBS="$m_save_LIBS"
             break
            ],
	     eval "m_cv_lib_$m_lib_var=no")
LIBS="$m_save_LIBS"
done
])dnl
if eval "test \"`echo '$m_cv_lib_'$m_lib_var`\" != no"; then
  if test x$j = "x."; then
    unset j
  fi
  AC_MSG_RESULT(yes (in ${j-standard location}))
  eval eval "`echo '$m_cv_lib_'$m_lib_var`"
  ifelse([$4], ,
         [ changequote(, )dnl
	   m_tr_lib=HAVE_LIB`echo $1 | sed -e 's/[^a-zA-Z0-9_]/_/g' \
	     -e 'y/abcdefghijklmnopqrstuvwxyz/ABCDEFGHIJKLMNOPQRSTUVWXYZ/'`
	   changequote([, ])dnl
	   AC_DEFINE_UNQUOTED($m_tr_lib)
	   eval eval "`echo '$m_cv_lib_'$m_lib_var`"
	   eval "LIBS=\"`echo '$libpath_'$m_lib_var` -l$1 $LIBS\""
         ], [$4])
else
  AC_MSG_RESULT(no)
  ifelse([$5], , , [$5])dnl
fi
])


dnl M_CHECK_MYHEADER(HEADER-FILE, LIBPATHLIST, [ACTION-IF-FOUND
dnl 	[, ACTION-IF-NOT-FOUND [, EXTRA-MSG [, EXTRA-SUFFIX]]]])
dnl
dnl Explanation of not obvious arguments:
dnl	EXTRA-MSG is an additional message printed after standardd one
dnl		  ("checking for header")
dnl	EXTRA-SUFFIX is the extra suffix to add to the cache variable, useful
dnl		  when you need to check for the same header in different
dnl		  paths (example: Python.h)
AC_DEFUN([M_CHECK_MYHEADER],
  [ m_safe=`echo "$1$6" | sed 'y%./+-%__p_%'`
    AC_MSG_CHECKING([for $1$5])
    AC_CACHE_VAL(m_cv_header_$m_safe,
      [ m_save_CPPFLAGS="$CPPFLAGS"
        for j in "." $2 ; do
           CPPFLAGS="-I$j $CPPFLAGS"
           AC_TRY_CPP(dnl
             [#include <$1>],
             [
               if test "$j" = "."; then
                  j="/usr/include"
               fi
               eval "m_cv_header_$m_safe=$j"
               CPPFLAGS="$m_save_CPPFLAGS"
               break
             ],
             [ eval "m_cv_header_$m_safe=no" ]
           )
           CPPFLAGS="$m_save_CPPFLAGS"
        done
      ]
    )
    if eval "test \"`echo '$m_cv_header_'$m_safe`\" != no"; then
       dnl FIXME surely there must be a simpler way? (VZ)
       dir=`eval echo $\`eval echo m_cv_header_$m_safe\``
       if test x$dir != x"/usr/include"; then
         dirshow=$dir
         eval "CPPFLAGS=\"-I$dir $CPPFLAGS\""
       fi
       AC_MSG_RESULT(yes (in ${dirshow-standard location}))
       ifelse([$3], , :, [$3])
    else
       AC_MSG_RESULT(no)
       ifelse([$4], , , [$4])dnl
    fi
  ]
)

dnl M_CHECK_MYHEADER_VER(HEADER-FILE, VERSION, LIBPATHLIST, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
AC_DEFUN([M_CHECK_MYHEADER_VER],
  [ m_safe=`echo "$1_$2" | sed 'y%./+-%__p_%'`
    AC_MSG_CHECKING([for $1 ($2)])
    AC_CACHE_VAL(m_cv_header_$m_safe,
      [ m_save_CPPFLAGS="$CPPFLAGS"
        for j in "." $3 ; do
           CPPFLAGS="-I$j $CPPFLAGS"
           AC_TRY_CPP(dnl
             [#include <$1>],
             [
               if test "$j" = "."; then
                  j="/usr/include"
               fi
               eval "m_cv_header_$m_safe=$j"
               CPPFLAGS="$m_save_CPPFLAGS"
               break
             ],
             [ eval "m_cv_header_$m_safe=no" ]
           )
           CPPFLAGS="$m_save_CPPFLAGS"
        done
      ]
    )
    if eval "test \"`echo '$m_cv_header_'$m_safe`\" != no"; then
       dnl FIXME surely there must be a simpler way? (VZ)
       dir=`eval echo $\`eval echo m_cv_header_$m_safe\``
       AC_MSG_RESULT(yes (in $dir))
       eval "CPPFLAGS=\"-I$dir $CPPFLAGS\""
       ifelse([$4], , :, [$4])
    else
       AC_MSG_RESULT(no)
       ifelse([$5], , , [$5])dnl
    fi
  ]
)


## libtool.m4 - Configure libtool for the target system. -*-Shell-script-*-
## Copyright (C) 1996-1998 Free Software Foundation, Inc.
## Gordon Matzigkeit <gord@gnu.ai.mit.edu>, 1996
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
##
## As a special exception to the GNU General Public License, if you
## distribute this file as part of a program that contains a
## configuration script generated by Autoconf, you may include it under
## the same distribution terms that you use for the rest of that program.

# serial 24 AM_PROG_LIBTOOL
AC_DEFUN([AM_PROG_LIBTOOL],
[AC_REQUIRE([AM_ENABLE_SHARED])dnl
AC_REQUIRE([AM_ENABLE_STATIC])dnl
AC_REQUIRE([AC_CANONICAL_HOST])dnl
AC_REQUIRE([AC_PROG_RANLIB])dnl
AC_REQUIRE([AC_PROG_CC])dnl
AC_REQUIRE([AM_PROG_LD])dnl
AC_REQUIRE([AM_PROG_NM])dnl
AC_REQUIRE([AC_PROG_LN_S])dnl
dnl
# Always use our own libtool.
LIBTOOL='$(SHELL) $(top_builddir)/libtool'
AC_SUBST(LIBTOOL)dnl

# Check for any special flags to pass to ltconfig.
libtool_flags=
test "$enable_shared" = no && libtool_flags="$libtool_flags --disable-shared"
test "$enable_static" = no && libtool_flags="$libtool_flags --disable-static"
test "$silent" = yes && libtool_flags="$libtool_flags --silent"
test "$ac_cv_prog_gcc" = yes && libtool_flags="$libtool_flags --with-gcc"
test "$ac_cv_prog_gnu_ld" = yes && libtool_flags="$libtool_flags --with-gnu-ld"

# Some flags need to be propagated to the compiler or linker for good
# libtool support.
case "$host" in
*-*-irix6*)
  # Find out which ABI we are using.
  echo '[#]line __oline__ "configure"' > conftest.$ac_ext
  if AC_TRY_EVAL(ac_compile); then
    case "`/usr/bin/file conftest.o`" in
    *32-bit*)
      LD="${LD-ld} -32"
      ;;
    *N32*)
      LD="${LD-ld} -n32"
      ;;
    *64-bit*)
      LD="${LD-ld} -64"
      ;;
    esac
  fi
  rm -rf conftest*
  ;;

*-*-sco3.2v5*)
  # On SCO OpenServer 5, we need -belf to get full-featured binaries.
  CFLAGS="$CFLAGS -belf"
  ;;
esac

# Actually configure libtool.  ac_aux_dir is where install-sh is found.
CC="$CC" CFLAGS="$CFLAGS" CPPFLAGS="$CPPFLAGS" \
LD="$LD" NM="$NM" RANLIB="$RANLIB" LN_S="$LN_S" \
${CONFIG_SHELL-/bin/sh} $ac_aux_dir/ltconfig \
$libtool_flags --no-verify $ac_aux_dir/ltmain.sh $host \
|| AC_MSG_ERROR([libtool configure failed])
])

# AM_ENABLE_SHARED - implement the --enable-shared flag
# Usage: AM_ENABLE_SHARED[(DEFAULT)]
#   Where DEFAULT is either `yes' or `no'.  If omitted, it defaults to
#   `yes'.
AC_DEFUN([AM_ENABLE_SHARED],
[define([AM_ENABLE_SHARED_DEFAULT], ifelse($1, no, no, yes))dnl
AC_ARG_ENABLE(shared,
changequote(<<, >>)dnl
<<  --enable-shared         build shared libraries [default=>>AM_ENABLE_SHARED_DEFAULT]
changequote([, ])dnl
[  --enable-shared=PKGS    only build shared libraries if the current package
                          appears as an element in the PKGS list],
[p=${PACKAGE-default}
case "$enableval" in
yes) enable_shared=yes ;;
no) enable_shared=no ;;
*)
  enable_shared=no
  # Look at the argument we got.  We use all the common list separators.
  IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS="${IFS}:,"
  for pkg in $enableval; do
    if test "X$pkg" = "X$p"; then
      enable_shared=yes
    fi
  done
  IFS="$ac_save_ifs"
  ;;
esac],
enable_shared=AM_ENABLE_SHARED_DEFAULT)dnl
])

# AM_DISABLE_SHARED - set the default shared flag to --disable-shared
AC_DEFUN([AM_DISABLE_SHARED],
[AM_ENABLE_SHARED(no)])

# AM_DISABLE_STATIC - set the default static flag to --disable-static
AC_DEFUN([AM_DISABLE_STATIC],
[AM_ENABLE_STATIC(no)])

# AM_ENABLE_STATIC - implement the --enable-static flag
# Usage: AM_ENABLE_STATIC[(DEFAULT)]
#   Where DEFAULT is either `yes' or `no'.  If omitted, it defaults to
#   `yes'.
AC_DEFUN([AM_ENABLE_STATIC],
[define([AM_ENABLE_STATIC_DEFAULT], ifelse($1, no, no, yes))dnl
AC_ARG_ENABLE(static,
changequote(<<, >>)dnl
<<  --enable-static         build static libraries [default=>>AM_ENABLE_STATIC_DEFAULT]
changequote([, ])dnl
[  --enable-static=PKGS    only build shared libraries if the current package
                          appears as an element in the PKGS list],
[p=${PACKAGE-default}
case "$enableval" in
yes) enable_static=yes ;;
no) enable_static=no ;;
*)
  enable_static=no
  # Look at the argument we got.  We use all the common list separators.
  IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS="${IFS}:,"
  for pkg in $enableval; do
    if test "X$pkg" = "X$p"; then
      enable_static=yes
    fi
  done
  IFS="$ac_save_ifs"
  ;;
esac],
enable_static=AM_ENABLE_STATIC_DEFAULT)dnl
])


# AM_PROG_LD - find the path to the GNU or non-GNU linker
AC_DEFUN([AM_PROG_LD],
[AC_ARG_WITH(gnu-ld,
[  --with-gnu-ld           assume the C compiler uses GNU ld [default=no]],
test "$withval" = no || with_gnu_ld=yes, with_gnu_ld=no)
AC_REQUIRE([AC_PROG_CC])
ac_prog=ld
if test "$ac_cv_prog_gcc" = yes; then
  # Check if gcc -print-prog-name=ld gives a path.
  AC_MSG_CHECKING([for ld used by GCC])
  ac_prog=`($CC -print-prog-name=ld) 2>&5`
  case "$ac_prog" in
  # Accept absolute paths.
  /* | [A-Za-z]:\\*)
    test -z "$LD" && LD="$ac_prog"
    ;;
  "")
    # If it fails, then pretend we aren't using GCC.
    ac_prog=ld
    ;;
  *)
    # If it is relative, then search for the first ld in PATH.
    with_gnu_ld=unknown
    ;;
  esac
elif test "$with_gnu_ld" = yes; then
  AC_MSG_CHECKING([for GNU ld])
else
  AC_MSG_CHECKING([for non-GNU ld])
fi
AC_CACHE_VAL(ac_cv_path_LD,
[if test -z "$LD"; then
  IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS="${IFS}:"
  for ac_dir in $PATH; do
    test -z "$ac_dir" && ac_dir=.
    if test -f "$ac_dir/$ac_prog"; then
      ac_cv_path_LD="$ac_dir/$ac_prog"
      # Check to see if the program is GNU ld.  I'd rather use --version,
      # but apparently some GNU ld's only accept -v.
      # Break only if it was the GNU/non-GNU ld that we prefer.
      if "$ac_cv_path_LD" -v 2>&1 < /dev/null | egrep '(GNU|with BFD)' > /dev/null; then
	test "$with_gnu_ld" != no && break
      else
        test "$with_gnu_ld" != yes && break
      fi
    fi
  done
  IFS="$ac_save_ifs"
else
  ac_cv_path_LD="$LD" # Let the user override the test with a path.
fi])
LD="$ac_cv_path_LD"
if test -n "$LD"; then
  AC_MSG_RESULT($LD)
else
  AC_MSG_RESULT(no)
fi
test -z "$LD" && AC_MSG_ERROR([no acceptable ld found in \$PATH])
AC_SUBST(LD)
AM_PROG_LD_GNU
])

AC_DEFUN([AM_PROG_LD_GNU],
[AC_CACHE_CHECK([if the linker ($LD) is GNU ld], ac_cv_prog_gnu_ld,
[# I'd rather use --version here, but apparently some GNU ld's only accept -v.
if $LD -v 2>&1 </dev/null | egrep '(GNU|with BFD)' 1>&5; then
  ac_cv_prog_gnu_ld=yes
else
  ac_cv_prog_gnu_ld=no
fi])
])

# AM_PROG_NM - find the path to a BSD-compatible name lister
AC_DEFUN([AM_PROG_NM],
[AC_MSG_CHECKING([for BSD-compatible nm])
AC_CACHE_VAL(ac_cv_path_NM,
[case "$NM" in
/* | [A-Za-z]:\\*)
  ac_cv_path_NM="$NM" # Let the user override the test with a path.
  ;;
*)
  IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS="${IFS}:"
  for ac_dir in /usr/ucb /usr/ccs/bin $PATH /bin; do
    test -z "$ac_dir" && ac_dir=.
    if test -f $ac_dir/nm; then
      # Check to see if the nm accepts a BSD-compat flag.
      # Adding the `sed 1q' prevents false positives on HP-UX, which says:
      #   nm: unknown option "B" ignored
      if ($ac_dir/nm -B /dev/null 2>&1 | sed '1q'; exit 0) | egrep /dev/null >/dev/null; then
        ac_cv_path_NM="$ac_dir/nm -B"
      elif ($ac_dir/nm -p /dev/null 2>&1 | sed '1q'; exit 0) | egrep /dev/null >/dev/null; then
        ac_cv_path_NM="$ac_dir/nm -p"
      else
        ac_cv_path_NM="$ac_dir/nm"
      fi
      break
    fi
  done
  IFS="$ac_save_ifs"
  test -z "$ac_cv_path_NM" && ac_cv_path_NM=nm
  ;;
esac])
NM="$ac_cv_path_NM"
AC_MSG_RESULT([$NM])
AC_SUBST(NM)
])
