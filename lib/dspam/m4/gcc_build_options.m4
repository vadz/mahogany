dnl $Id: gcc_build_options.m4,v 1.1 2004/10/24 20:48:37 jonz Exp $
dnl gcc_build_options.m4
dnl

AC_DEFUN([GCC_BUILD_OPTIONS],
[

if test x$GCC = xyes
then

#
# Enable Compiler Warnings
#

AC_ARG_ENABLE(warnings,
[[  --enable-warnings[={no|[{yes|error}][,proto]}]
                          Disable (no) or enable (yes) more warnings
                          or enable and treat warnings as errors (error).
                          Simple --enable-warnings is the same
                          as --enable-warnings=yes.
                          You can add ',proto' to 'yes' or 'error' option
                          for turning on additional '-Wstrict-prototypes'
                          flag.
                          Have effect for GCC compilers only.
  --disable-warnings      Same as --enable-warnings=no [default]]])
                                                                                
    gcc_param=",$enable_warnings,"
    gcc_enable_warnings=`echo $gcc_param|grep ',no,' >/dev/null 2>&1 && echo no || echo yes`
    gcc_enable_error=`echo $gcc_param|grep ',error,' >/dev/null 2>&1 && echo yes || echo no`
    gcc_enable_strict_proto=`echo $gcc_param|grep ',proto,' >/dev/null 2>&1 && echo yes || echo no`
                                                                                
        warn_flags='-Wall -Wmissing-prototypes -Wmissing-declarations'
                                                                                
    if test x$gcc_enable_strict_proto != xno
    then
        warn_flags="$warn_flags -Wstrict-prototypes"
    fi
                                                                                
    if test x$gcc_enable_error != xno
    then
        warn_flags="$warn_flags -Werror"
    fi
                                                                                
    if test x$gcc_enable_warnings != xno
    then
        CFLAGS="$CFLAGS $warn_flags"
        CXXFLAGS="$CXXFLAGS $warn_flags"
    fi

#
#   Enable Profiling Support
#
AC_ARG_ENABLE(profiling,
    [AS_HELP_STRING(--enable-profiling,
                       Disable (no) or enable (yes) performance profiling.
                          Generate extra code to write profile information
                          suitable for the analysis program gprof.
                          Has effect for GCC compilers only.
                    )])
AC_MSG_CHECKING([whether to enable profiling output])
case x"$enable_profiling" in
    xyes)   # profiling output enabled explicity
            ;;
    xno)    # profiling output disabled explicity
            ;;
    x)      # profiling output disabled by default
            enable_profiling=no
            ;;
    *)      AC_MSG_ERROR([unexpected value $enable_profiling for --{enable,disable}-profiling configure option])
            ;;
esac
if test x"$enable_profiling" != xyes
then
    enable_profiling=no
else
    enable_profiling=yes    # overkill, but convenient
     CFLAGS="$CFLAGS -pg"
     CXXFLAGS="$CXXFLAGS -pg"
fi
AC_MSG_RESULT([$enable_profiling])

# GCC
fi

])

