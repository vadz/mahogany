# $Id: ldap.m4,v 1.1 2005/06/08 02:15:09 jonz Exp $
# m4/ldap.m4
# Jonathan A. Zdziarski <jonathan@nuclearelephant.com>
#
#   DS_LDAP()
#
#   Activate libldap/liblber extensions from OpenLDAP
#
AC_DEFUN([DS_LDAP],
[

  AC_ARG_ENABLE(ldap,
      [AS_HELP_STRING(--enable-ldap,
                        Enable LDAP support via libldap
                      )])
  AC_MSG_CHECKING([whether to enable LDAP support])
  case x"$enable_ldap" in
      xyes)   # ldap enabled explicity
              ;;
      xno)    # ldap disabled explicity
              ;;
      x)      # ldap disabled by default
              enable_ldap=no
              ;;
      *)      AC_MSG_ERROR([unexpected value $enable_ldap for --{enable,disable}-ldap configure option])
              ;;
  esac
  if test x"$enable_ldap" != xyes
  then
      enable_ldap=no
  else
      enable_ldap=yes    # overkill, but convenient
      AC_DEFINE(USE_LDAP, 1, [Defined if ldap is enabled])

      LIBS="$LIBS -lldap -llber"
  fi
  AC_MSG_RESULT([$enable_ldap])
])
