# $Id: pthread.m4,v 1.3 2004/11/20 23:33:25 jonz Exp $
# Autuconf macros for checking how to compile with pthreads
# Jonathan Zdziarski <jonathan@nuclearelephant.com>
#
#   Public available macro:
#       DS_PTHREADS([pthreads_cppflags_out],
#                   [pthreads_ldflags_out],
#                   [pthreads_libs_out],
#                   [additional-action-if-found],
#                   [additional-action-if-not-found])
#
AC_DEFUN([DS_PTHREADS],
[
ds_pthreads_save_CPPFLAGS="$CPPFLAGS"
ds_pthreads_save_LDFLAGS="$LDFLAGS"
ds_pthreads_save_LIBS="$LIBS"

ds_pthreads_CPPFLAGS=""
ds_pthreads_LDFLAGS=""
ds_pthreads_LIBS=""

AC_MSG_CHECKING([how you like your pthreads])
pthreads_success="no"

LIBS="$ds_pthreads_save_LIBS -pthread"

AC_RUN_IFELSE([AC_LANG_SOURCE([[
 #include <pthread.h>
 #include <stdlib.h>

 int main() {

   pthread_mutex_t m;
   pthread_mutex_init(&m, NULL);
   pthread_exit(0);
   exit(EXIT_FAILURE);
 }

]])],
  [
    pthreads_success="yes"
    ds_pthreads_LIBS="-pthread"
    AC_MSG_RESULT([-pthread])
  ],
  [ ])

if test x"$pthreads_success" = xno
then
  LIBS="$ds_pthreads_save_LIBS -lpthread" 

AC_RUN_IFELSE([AC_LANG_SOURCE([[
  #include <pthread.h>
  #include <stdlib.h>

  int main() {
    pthread_mutex_t m;
    pthread_mutex_init(&m, NULL);
    pthread_exit(0);
    exit(EXIT_FAILURE);
  }

]])],
  [
    pthreads_success="yes"
    ds_pthreads_LIBS="-lpthread" 
    AC_MSG_RESULT([-lpthread])
  ],
  [ ])
fi

if test x"$pthreads_success" = xno
then
  LIBS="$ds_pthreads_save_LIBS -lpthread -mt" 

AC_RUN_IFELSE([AC_LANG_SOURCE([[
  #include <pthread.h>
  #include <stdlib.h>

  int main() {
    pthread_mutex_t m;
    pthread_mutex_init(&m, NULL);
    pthread_exit(0);
    exit(EXIT_FAILURE);
  }

]])],
  [
    pthreads_success="yes"
    ds_pthreads_LIBS="-lpthread -mt"
    AC_MSG_RESULT([-lpthread -mt])
  ],
  [ ])
fi

if test x"$pthreads_success" = xno
then
  AC_MSG_RESULT([unknown])
  AC_MSG_ERROR([Unable to determine how to compile with pthreads])
fi

CPPFLAGS="$ds_pthreads_save_CPPFLAGS"
LDFLAGS="$ds_pthreads_save_LDFLAGS"
LIBS="$ds_pthreads_save_LIBS"

if test x"$pthreads_success" = xyes
then
    ifelse([$1], [], [:], [$1="$ds_pthreads_CPPFLAGS"])
    ifelse([$2], [], [:], [$2="$ds_pthreads_LDFLAGS"])
    ifelse([$3], [], [:], [$3="$ds_pthreads_LIBS"])
    ifelse([$4], [], [:], [$4])
    :
else
    ifelse([$5], [], [:], [$5])
    :
fi
])
