/* include/config.h.  Generated automatically by configure.  */
/*-*- c++ -*-********************************************************
 * config.h.in: config.h template file for use with autoconf        *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$           *
 *******************************************************************/
/**@name config.h.in: Defines generated automatically from configure.in by autoheader.  */
//@{
/** Define to empty if the keyword does not work.  */
/* #undef const */

/** Define as __inline if that's what the C compiler calls it.  */
/* #undef inline */

/** Define if your C compiler doesn't accept -c and -o together.  */
/* #undef NO_MINUS_C_MINUS_O */

/** Define if you need to in order for stat and other things to work.  */
/* #undef _POSIX_SOURCE */

/** Define to `unsigned' if <sys/types.h> doesn't define.  */
/* #undef size_t */

/** Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/** Define if you have the strerror function.  */
#define HAVE_STRERROR 1

/** Define if you have the strstr function.  */
#define HAVE_STRSTR 1

/** Define if you have the strsep function.  */
#define HAVE_STRSEP 1

/** Define if you have the <libintl.h> header file.  */
#define HAVE_LIBINTL_H 1

/** Define if you have the <compface.h> header file.  */
#define HAVE_COMPFACE_H 1

/** Define if you have the <strings.h> header file.  */
#define HAVE_STRINGS_H 1

/* Define if you have the <stdarg.h> header file.  */
#define HAVE_STDARG_H 1

/** Define if you have the <stdlib.h> header file - required for 
    Python.h .  */
#define HAVE_STDLIB_H 1

/** Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1

/** Define if you use wxWindows. */
#define USE_WXWINDOWS 1

/** Define if you use wxWindows, version 2. */
/* #undef USE_WXWINDOWS2 */

/** Define if you use wxWindows, wxXt port. */
#define USE_WXXT 1

/** Define if you use wxWindows, wxGTK port. */
/* #undef USE_WXGTK */

/** Define if you want to use wxString instead of C++ string */
/* #undef USE_WXSTRING */

/** Define if you use Python. */
#define USE_PYTHON 1

/** Define if the C++ compiler supports prototypes (of course it
    does!) - required for Python.h */
#define HAVE_PROTOTYPES 1

/* Define if your <unistd.h> contains bad prototypes for exec*()
   (as it does on SGI IRIX 4.x) */
/* #undef BAD_EXEC_PROTOTYPES */

/* Define if your compiler supports variable length function prototypes
   (e.g. void fprintf(FILE *, char *, ...);) *and* <stdarg.h> */
#define HAVE_STDARG_PROTOTYPES 1

/** Define if you want to use the global Mpch.h header file (e.g. for
    precompiling headers */
/* #undef USE_PCH */

//@}
