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

#include "compface.h"

#define EXTERN
#define INIT(x) = x

/* need to know how many bits per hexadecimal digit for io */
EXTERN char HexDigits[] INIT("0123456789ABCDEF");

/* internal face representation - 1 char per pixel is faster */
EXTERN char F[PIXELS];

EXTERN BigInt B;

/* data.h was established by sampling over 1000 faces and icons */
EXTERN Guesses G
=
#include "data.h"
;

/* A stack of probability values */
EXTERN Prob *ProbBuf[PIXELS * 2];
EXTERN int NumProbs INIT(0);

EXTERN Prob levels[4][3]
=
{
	{{1, 255},	{251, 0},	{4, 251}},	/* Top of tree almost always grey */
	{{1, 255},	{200, 0},	{55, 200}},
	{{33, 223},	{159, 0},	{64, 159}},
	{{131, 0},	{0, 0}, 	{125, 131}}	/* Grey disallowed at bottom */
}
;

EXTERN Prob freqs[16]
=
{
	{0, 0}, 	{38, 0},	{38, 38},	{13, 152},
	{38, 76},	{13, 165},	{13, 178},	{6, 230},
	{38, 114},	{13, 191},	{13, 204},	{6, 236},
	{13, 217},	{6, 242},	{5, 248},	{3, 253}
}
;

EXTERN int status;

EXTERN jmp_buf comp_env;
