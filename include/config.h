/* include/config.h.  Generated automatically by configure.  */
/*-*- c++ -*-********************************************************
 * config.h.in: config.h template file for use with autoconf        *
 *                                                                  *
 * (C) 1998,1999 by Karsten Ballüder (Ballueder@usa.net)            *
 *                                                                  *
 * $Id$
 *******************************************************************/
/// @package M
/// @author Karsten Ballüder (Ballueder@usa.net)
/**@name config.h.in: Defines generated automatically from configure.in by autoheader.  */
//@{
#ifndef __cplusplus

/** Define to empty if the keyword does not work.  */
/* #undef const */

/** Define as __inline if that's what the C compiler calls it.  */
/* #undef inline */

#endif /* !C++ */

/** Define if you need to in order for stat and other things to work.  */
/* #undef _POSIX_SOURCE */

/** Define to `unsigned' if <sys/types.h> doesn't define.  */
/* #undef size_t */

/** Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/** Define if you have the strsep function.  */
#define HAVE_STRSEP 1

/** Define if you have the <compface.h> header file.  -- we always have it */
#define HAVE_COMPFACE_H 1

/** Do we have the pisock library and headers? */
#define USE_PISOCK 1

/** Do we have a pisock library with setmaxspeed? */
#define HAVE_PI_SETMAXSPEED 1

/** Do we have a pisock library with pi_accept_to? */
#define HAVE_PI_ACCEPT_TO 1

/*@name These are obsolete and always defined. Other configurations
  are no longer supported.
*/
//@{
/** Define if you use wxWindows. */
#define   USE_WXWINDOWS 1

/** Define if you use wxWindows, version 2. */
#define    USE_WXWINDOWS2 1

/** Define if you use wxWindows, wxGTK port. */
#define   USE_WXGTK 1

/** Define if you want to use wxString instead of C++ string */
#define   USE_WXSTRING 1

/** Define this if you want to use wxConfig instead of AppConfig.
    Note that this is only available for wxWindows version 2 */
#define   USE_WXCONFIG 1
//@}

/** Define if you use Python. */
#define USE_PYTHON 1

/** Define if you have libswigpy */
/* #undef HAVE_SWIGLIB */

/** Define if the C++ compiler supports prototypes (of course it
    does!) - required for Python.h */
#define HAVE_PROTOTYPES 1

/* Define if your <unistd.h> contains bad prototypes for exec*()
   (as it does on SGI IRIX 4.x) */
#define BAD_EXEC_PROTOTYPES 1

/* Define if your compiler supports variable length function prototypes
   (e.g. void fprintf(FILE *, char *, ...);) *and* <stdarg.h> */
#define HAVE_STDARG_PROTOTYPES 1

/** Define if you want to use the global Mpch.h header file (e.g. for
    precompiling headers */
/* #undef USE_PCH */

/** Define if you want to use wxThreads. (experimental) */
/* #undef USE_THREADS */

/** Define if you want to use Modules. */
#define USE_MODULES 1

/** Define if you want to link Modules statically. */
#define USE_MODULES_STATIC 1

/** Define this if you want to use RBL to lookup SPAM domains. */
#define USE_RBL 1

/** Define this if you want to compile in SSL support. */
#define USE_SSL 1

/** Define this if you want to compile in SSL support without DSA. */
#define NO_DSA 1

/** Define this if you want to compile in SSL support without IDEA. */
#define NO_IDEA 1

/** Define this if you want to compile in SSL support without RSA. */
#define NO_RSA 1

/** Define this if you have POSIX.2 regexp suport. */
#define HAVE_POSIX_REGEX 1

/** Define this if you want debugging logic included. */
/* #undef DEBUG */

/** Define this if you want experimental logic included. */
/* #undef EXPERIMENTAL */

/** This define contains the installation prefix where Mahogany
    is installed.
*/
#define M_PREFIX "/usr"

/** This define contains the full canonical string that specifies
    the machine/system/OS/compiler
*/
#define M_CANONICAL_HOST "i686-pc-linux-gnu"

/** This define carries some information about the operating system
    and release for which M is compiled.
*/
#define M_OSINFO   "Linux 2.2.17 i686"
//@}
