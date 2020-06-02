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

dnl M_ADD_CXXFLAGS_IF_SUPPORTED(OPTION)
dnl
dnl Check if the C++ compiler supports the given option and adds it to CXXFLAGS
dnl if it does.
AC_DEFUN([M_ADD_CXXFLAGS_IF_SUPPORTED], [
   AX_CXX_CHECK_FLAG($1,,,[CXXFLAGS="$CXXFLAGS $1"])
])
