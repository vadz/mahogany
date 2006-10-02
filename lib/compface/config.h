#ifdef	STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#else	/* Not STDC_HEADERS */
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
extern void exit ();
extern char *malloc ();
#endif	/* STDC_HEADERS */

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_IO_H
#include <io.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#else
#ifndef HAVE_STRERROR
extern int errno, sys_nerr;
extern char *sys_errlist[];
#else
extern int errno;
char *strerror();
#endif
#endif

#include <stdio.h>
