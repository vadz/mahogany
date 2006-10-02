
/*  @(#)cmain.c 1.7 91/10/24
 *
 *  Compface - 48x48x1 image compression.
 *
 *  Copyright (c) James Ashton - Sydney University - June 1990.
 *
 *  Written 11th November 1889.
 *
 *  Permission is given to distribute these sources, as long as the
 *  copyright messages are not removed, and no monies are exchanged. 
 *
 *  No responsibility is taken for any errors on inaccuracies inherent
 *  either to the comments or the code of this program, but if reported
 *  to me, then an attempt will be made to fix them.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "compface.h"

/* the buffer is longer than needed to handle sparse input formats */
#define FACEBUFLEN 2048
char fbuf[FACEBUFLEN];

/* IO file descriptors and their names */
int infile    = 0;
char *inname  = "<stdin>";
int outfile   = 1;
char *outname = "<stdout>";

/* basename of executable */
char *cmdname;

#ifdef HAVE_STRERROR
#ifdef STDC_HEADERS
char *strerrorwrap(int err);
#else
char *strerrorwrap();
#endif
#endif

/* This really shouldn't be done with cpp */
#ifndef HAVE_STRERROR
#define ERR ((errno < sys_nerr) ? sys_errlist[errno] : "")
#else
#define ERR (strerrorwrap(errno))
#endif

#define INITERR(s) {(void)strcpy(fbuf, cmdname); (void)strcat(fbuf, ": ");\
					(void)strcat(fbuf, (s));}
#define ADDERR(s) (void)strcat(fbuf, (s));
#define ERROR {(void)strcat(fbuf, "\n");\
				(void)write(2, fbuf, strlen(fbuf)); exit(1);}
#define INITWARN(s) {(void)strcpy(fbuf, cmdname);\
					(void)strcat(fbuf, ": (warning) ");\
					(void)strcat(fbuf, (s));}
#define ADDWARN(s) (void)strcat(fbuf, (s));
#define WARN {(void)strcat(fbuf, "\n"); (void)write(2, fbuf, strlen(fbuf));}

int
main(argc, argv)
int argc;
char *argv[];
{
  cmdname = *argv;
  while (**argv)
    if (*((*argv)++) == '/')
      cmdname = *argv;               /* find the command's basename */

  if (argc > 3)
    {
      INITERR("usage: ")
      ADDERR(cmdname)
      ADDERR(" [infile [outfile]]")
      ERROR
    }

  if ((argc > 1) && strcmp(*++argv, "-"))
    {
      inname = *argv;
      if ((infile = open(inname, O_RDONLY)) == -1)
        {
          INITERR(inname)
          ADDERR(": ")
          ADDERR(ERR)
          ERROR
        }
    }

  if (argc > 2)
    {
      outname = *++argv;
      if ((outfile = open(outname, O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1)
        {
          INITERR(outname)
          ADDERR(": ")
          ADDERR(ERR)
          ERROR
        }
    }

  (void) ReadBuf();
  switch (compface(fbuf))
    {
      case -2 : INITERR("internal error")
                ERROR
      case -1 : INITERR(inname)
                ADDERR(": insufficient or invalid data")
                ERROR
      case  1 : INITWARN(inname)
                ADDWARN(": excess data ignored")
                WARN
      default : ;
    }
  (void) WriteBuf();
  exit(0);
/*NOTREACHED*/
}


int
WriteBuf()
{
	register char *s, *t;
	register int len;

	s = fbuf;
	t = s + strlen(s);
	while (s < t)
	{
		if ((len = write(outfile, s, t - s)) == -1)
		{
			INITERR(outname)
			ADDERR(": ")
			ADDERR(ERR)
			ERROR
		}
		s += len;
	}
	return 0;
}


int
ReadBuf()
{
	register int count, len;
	register char *t;

	count = 0;
	t = fbuf;
	while (len = read(infile, t, FACEBUFLEN - count))
	{
		if (len == -1)
		{
			INITERR(inname)
			ADDERR(": ")
			ADDERR(ERR)
			ERROR
		}
		t += len;
		if ((count += len) >= FACEBUFLEN)
		{
			INITWARN(inname)
			ADDWARN(" exceeds internal buffer size.  Data may be lost")
			WARN
			break;
		}
	}
	*t = '\0';
	return count;
}

#ifdef HAVE_STRERROR
char *strerrorwrap(err)
int err;
{
	char *c = strerror(err);
	return ((c) ? (c) : "" );
}
#endif
