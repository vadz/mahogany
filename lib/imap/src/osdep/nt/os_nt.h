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
 * Date:	11 May 1989
 * Last Edited:	30 August 2006
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys\types.h>
#include <time.h>
#include <io.h>
#include <conio.h>
#include <process.h>
#undef ERROR			/* quell conflicting defintion warning */
#include <windows.h>
#include <lm.h>
#undef ERROR
#define ERROR (long) 2		/* must match mail.h */


#include "env_nt.h"
#include "fs.h"
#include "ftl.h"
#include "nl.h"
#include "tcp.h"
#include "yunchan.h"

#undef noErr
#undef MAC

typedef int (*select_t) (struct _finddata_t *name);
typedef int (*compar_t) (const void *d1,const void *d2); 
int scandir (char *dirname,struct _finddata_t ***namelist,select_t select, compar_t compar);