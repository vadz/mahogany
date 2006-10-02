# $Id: ncore.m4,v 1.3 2005/08/28 01:35:49 jonz Exp $
# m4/ncore.m4
# Jonathan A. Zdziarski <jonathan@nuclearelephant.com>
#
#   DS_NCORE()
#
#   Activate NodalCore(tm) C-Series Hardware Extensions
#
AC_DEFUN([DS_NCORE],
[

  AC_ARG_ENABLE(nodalcore,
      [AS_HELP_STRING(--enable-nodalcore,
                        Enable NodalCore(tm) C-Series Hardware Extensions
                      )])
  AC_MSG_CHECKING([whether to enable NodalCore(tm) C-Series Hardware Extensions])
  case x"$enable_nodalcore" in
      xyes)   # nodalcore output enabled explicity
              ;;
      xno)    # nodalcore output disabled explicity
              ;;
      x)      # nodalcore output disabled by default
              enable_nodalcore=no
              ;;
      *)      AC_MSG_ERROR([unexpected value $enable_nodalcore for --{enable,disable}-nodalcore configure option])
              ;;
  esac
  if test x"$enable_nodalcore" != xyes
  then
      enable_nodalcore=no
  else
      enable_nodalcore=yes    # overkill, but convenient
      AC_DEFINE(NCORE, 1, [Defined if nodalcore output is enabled])

      adapter_objects=ncore_adp.lo
      NCORELIBS="-lncore -lncorefw -lpthread"
      build_ncore=yes

      AC_SUBST(NCORELIBS)
      AC_SUBST(adapter_objects)
  fi
  AC_MSG_RESULT([$enable_nodalcore])
])
