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
 * Program:	Operating-system dependent routines -- Linux version
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	10 September 1993
 * Last Edited:	30 August 2006
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>		/* for struct tm */
#include <fcntl.h>
#include <syslog.h>
#include <sys/file.h>

/* Real declaration in utime.h is compatible, but different. */
extern int utime(const char *filename, const time_t *times);

/* Linux gets this wrong */

#define setpgrp setpgid

#define direct dirent

extern int safe_flock (int fd,int op);
#define flock safe_flock


#include "env_unix.h"
#include "fs.h"
#include "ftl.h"
#include "nl.h"
#include "tcp.h"
