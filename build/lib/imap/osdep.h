
/* Minimal osdep.h for Unix builds */
#ifndef OSDEP_H
#define OSDEP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

/* Basic type definitions */
typedef unsigned long ulong;
typedef unsigned int uint;

/* Minimal function declarations */
#define NIL 0
#define T 1

/* Disable some features that need full OS support */
#define DISABLE_POP
#define DISABLE_IMAP
#define DISABLE_SMTP
#define DISABLE_NNTP

#endif /* OSDEP_H */
