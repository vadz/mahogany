dnl $Id$
dnl local macro definitions for M's configure.in

AC_DEFUN(M_OVERRIDES_PREPARE,
[
rm -f ${OSTYPE}.system.cache.tmp
touch ${OSTYPE}.system.cache.tmp
touch ${OSTYPE}.system.cache
])

AC_DEFUN(M_OVERRIDES_DONE,
[
mv ${OSTYPE}.system.cache.tmp ${OSTYPE}.system.cache
])

dnl package,message,helpmessage,variable
AC_DEFUN(M_OVERRIDES,
[
AC_MSG_CHECKING("for $2")
AC_ARG_WITH($1,$3,
[if test "x$with_$1" = xyes; then
  ac_cv_use_$1='$4="1"'
else
  ac_cv_use_$1='$4="0"'
fi],
[
  LINE=`grep "$4" ${OSTYPE}.system.cache`
  if test "x$LINE" != x ; then
    eval "DEFAULT_$LINE"
  fi
  ac_cv_use_$1='$4='$DEFAULT_$4
])
eval "$ac_cv_use_$1"
echo $ac_cv_use_$1 >> ${OSTYPE}.system.cache.tmp
if test "$$4" = 1; then
  AC_MSG_RESULT(yes)
else
  AC_MSG_RESULT(no)
fi
])

dnl M_CHECK_MYLIB(LIBRARY, FUNCTION, LIBPATHLIST [, ACTION-IF-FOUND 
dnl            [, ACTION-IF-NOT-FOUND [, OTHER-LIBRARIES]]])

AC_DEFUN(M_CHECK_MYLIB,
[AC_MSG_CHECKING([for $2 in -l$1])
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
]ifelse(AC_LANG, CPLUSPLUS, [#ifdef __cplusplus
extern "C"
#endif
])dnl
[/* We use char because int might match the return type of a gcc2
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
  AC_MSG_RESULT(yes)
  eval eval "`echo '$m_cv_lib_'$m_lib_var`"
  dnl  LIBS="`echo '$libpath_'$m_lib_var` -l$1 $LIBS"
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


dnl M_CHECK_MYHEADER(HEADER-FILE, LIBPATHLIST, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
AC_DEFUN(M_CHECK_MYHEADER,
  [ m_safe=`echo "$1" | sed 'y%./+-%__p_%'`
    AC_MSG_CHECKING([for $1])
    AC_CACHE_VAL(m_cv_header_$m_safe,
      [ m_save_CPPFLAGS="$CPPFLAGS"
        for j in "." $2 ; do
           CPPFLAGS="-I$j $CPPFLAGS"
           AC_TRY_CPP(dnl
             [#include <$1>], 
             [ dnl Do NOT allow -I/usr/include as include path since gcc might get problems
               if test "$j" != "/usr/include" ; then
                  eval "m_cv_header_$m_safe=\"headerpath_$m_safe=-I$j\""
                  eval eval "`echo '$m_cv_header_'$m_safe`"
               fi
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
       AC_MSG_RESULT(yes)
       eval eval "`echo '$m_cv_header_'$m_safe`"
       eval "CPPFLAGS=\"`echo '$headerpath_'$m_safe` $CPPFLAGS\""
       ifelse([$3], , :, [$3])
    else
       AC_MSG_RESULT(no)
       ifelse([$4], , , [$4])dnl
    fi
  ]
)


