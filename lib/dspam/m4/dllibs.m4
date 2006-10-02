# $Id: dllibs.m4,v 1.1 2005/09/29 16:36:42 jonz Exp $
# m4/dl.m4
#
#   DS_DLLIBS([libs_out], [action-if-found], [action-if-not-found])
#
#   Search for additional libraries containing dlopen().
#
#   Typically, this is either -ldl or libc
#
AC_DEFUN([DS_DLLIBS],
[
    AS_VAR_PUSHDEF([save_LIBS], [ds_dllibs_save_LIBS])
    AS_VAR_PUSHDEF([dllibs], [ds_dllibs_dllibs])
    AS_VAR_PUSHDEF([success], [ds_dllibs_success])

    save_LIBS="$LIBS"
    LIBS=''
    success=yes

    AC_SEARCH_LIBS([dlopen], [dl], [success=yes], [success=no])

    dllibs="$LIBS"
    LIBS="$save_LIBS"

    AS_VAR_POPDEF([save_LIBS])
    AS_VAR_POPDEF([dllibs])
    AS_VAR_POPDEF([success])

    if test x"$ds_dllibs_success" = xyes
    then
        ifelse([$1], [], [:], [$1="$ds_dllibs_dllibs"])
        ifelse([$2], [], [:], [$2])
    else
        ifelse([$3], [], [:], [$3])
    fi
])
