/*
 *  Compface - 48x48x1 image compression and decompression
 *
 *  Copyright (c) James Ashton - Sydney University - June 1990.
 *
 *  Written 11th November 1989.
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

#define MAIN

#include "compface_private.h"

int
uncompface(fbuf)
char *fbuf;
{
	if (!(status = setjmp(comp_env)))
	{
		UnCompAll(fbuf);/* compress otherwise */
		UnGenFace();
		WriteFace(fbuf);
	}
	return status;
}
