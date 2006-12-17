/* ========================================================================
 * Copyright 1988-2006 University of Washington
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * 
 * ========================================================================
 */

/*
 * Program:	Operating-system dependent routines -- NT version
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	11 April 1989
 * Last Edited:	30 August 2006
 */

#include "tcp_nt.h"		/* must be before osdep includes tcp.h */
#undef	ERROR			/* quell conflicting def warning */
#include "mail.h"
#include "osdep.h"
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <sys\timeb.h>
#include <fcntl.h>
#include <sys\stat.h>
#include "misc.h"
#include "mailfile.h"

#include "fs_nt.c"
#include "ftl_nt.c"
#include "nl_nt.c"
#include "yunchan.c"
#include "tcp_nt.c"		/* must be before env_nt.c */
#include "env_nt.c"
#include "ssl_nt.c"

/* Emulator for BSD scandir() call
 * Accepts: directory name
 *	    destination pointer of names array
 *	    selection function
 *	    comparison function
 * Returns: number of elements in the array or -1 if error
 */

int scandir (char *dirname,struct _finddata_t ***namelist,select_t select,
	     compar_t compar)
{
  struct _finddata_t *p, d, **names;
  int nitems;
  long nlmax;
  long hDir;
  char *filemask = (char *)fs_get((strlen(dirname) + 5) * sizeof(char));
  strcpy(filemask, dirname);
  strcat(filemask, "/*.*");
  hDir = _findfirst(filemask, &d);/* open directory */
  fs_give((void **)&filemask);
  if (hDir == -1) return -1;
  nlmax = 100;			/* FIXME how to get number of files in dir? */
  names = (struct _finddata_t **) fs_get (nlmax * sizeof (struct _finddata_t *));
  nitems = 0;			/* initially none found */
  do {				/* loop over all files */
				/* matches select criterion? */
    if (select && !(*select) (&d)) continue;
    p = (struct _finddata_t *)fs_get(sizeof(d));
    strcpy (p->name,d.name);
    if (++nitems >= nlmax) {	/* if out of space, try bigger guesstimate */
      nlmax *= 2;		/* double it */
      fs_resize ((void **) &names,nlmax * sizeof (struct _finddata_t *));
    }
    names[nitems - 1] = p;	/* store this file there */
  }
  while (_findnext(hDir, &d) == 0);
  _findclose(hDir);		/* done with directory */
				/* sort if necessary */
  if (nitems && compar) qsort (names,nitems,sizeof (struct _finddata_t *),compar);
  *namelist = names;		/* return directory */
  return nitems;		/* and size */
}
 

