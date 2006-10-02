# $Id: libtool_hack.m4,v 1.1 2004/10/24 20:48:37 jonz Exp $
# Libtool-related hack.
# Andrew W. Nosenko <awn@bcs.zp.ua>
#

#   DS_LIBTOOL_RUN_IFELSE(PROGRAM,
#                         [ACTION-IF-TRUE], [ACTION-IF-FALSE],
#                         [ACTION-IF-CROSS-COMPILING = RUNTIME-ERROR])
#
#   Compile, link, and run.
#   Similar to AC_RUN_IFELSE, but links test program over libtool.
#
#   Considered as hack because:
#   1. uses knowledge about AC_RUN_IFELSE() internals (redefining ac_link
#      variable), and
#   2. uses unrecommended way for linkink libtool's .la libraries
#      (`-L/path -llib' instead of `/path/liblib.la')
#
AC_DEFUN([DS_LIBTOOL_RUN_IFELSE],
[
    AC_REQUIRE([AC_PROG_LIBTOOL])
    AS_VAR_PUSHDEF([ds_ac_link_save], [ds_libtool_run_ifelse_ac_link_save])

    if test x"$ac_link" = x
    then
        AC_MSG_ERROR([DS@&t@_LIBTOOL_RUN_IFELSE: internal error: ac_link variable is empty])
    fi
    ds_ac_link_save="$ac_link"
    ac_link="./libtool --mode=link $ac_link -no-install"
    AC_LINK_IFELSE($@)
    ac_link="$ds_ac_link_save"
    AS_VAR_POPDEF([ds_ac_link_save])
])
