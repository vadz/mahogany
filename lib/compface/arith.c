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

#include "compface.h"

void
RevPush(p)
Prob *p;
{
	if (NumProbs >= PIXELS * 2 - 1)
		longjmp(comp_env, ERR_INTERNAL);
	ProbBuf[NumProbs++] = p;
}
 
void
BigPush(p)
Prob *p;
{
	static WORD tmp;

	BigDiv(p->p_range, &tmp);
	BigMul(0);
	BigAdd(tmp + p->p_offset);
}

int
BigPop(p)
register Prob *p; 
{
	static WORD tmp;
	register int i;

	BigDiv(0, &tmp);
	i = 0;
	while ((tmp < p->p_offset) || (tmp >= p->p_range + p->p_offset))
	{
		p++;
		i++;
	}
	BigMul(p->p_range);
	BigAdd(tmp - p->p_offset);
	return i;
}

#ifdef DEBUG
/* Print a BigInt in HexaDecimal
 */
void
BigPrint()
{
	register int i, c, count;
	register WORD *w;

	count = 0;
	w = B.b_word + (i = B.b_words);
	while (i--)
	{
		w--;
		c = *((*w >> 4) + HexDigits);
		putc(c, stderr);
		c = *((*w & 0xf) + HexDigits);
		putc(c, stderr);
		if (++count >= 36)
		{
			putc('\\', stderr);
			putc('\n', stderr);
			count = 0;
		}
	}
	putc('\n', stderr);
}
#endif

/* Divide B by a storing the result in B and the remainder in the word
 * pointer to by r
 */
void
BigDiv(a, r)
register WORD a, *r;
{
	register int i;
	register WORD *w;
	register COMP c, d;

	a &= WORDMASK;
	if ((a == 1) || (B.b_words == 0))
	{
		*r = 0;
		return;
	}
	if (a == 0)	/* treat this as a == WORDCARRY */
	{			/* and just shift everything right a WORD */
		i = --B.b_words;
		w = B.b_word;
		*r = *w;
		while (i--)
		{
			*w = *(w + 1);
			w++;
		}
		*w = 0;
		return;
	}
	w = B.b_word + (i = B.b_words);
	c = 0;
	while (i--)
	{
		c <<= BITSPERWORD;
		c += (COMP)*--w;
		d = c / (COMP)a;
		c = c % (COMP)a;
		*w = (WORD)(d & WORDMASK);
	}
	*r = c;
	if (B.b_word[B.b_words - 1] == 0)
		B.b_words--;
}

/* Multiply a by B storing the result in B
 */
void
BigMul(a)
register WORD a;
{
	register int i;
	register WORD *w;
	register COMP c;

	a &= WORDMASK;
	if ((a == 1) || (B.b_words == 0))
		return;
	if (a == 0)	/* treat this as a == WORDCARRY */
	{			/* and just shift everything left a WORD */
		if ((i = B.b_words++) >= MAXWORDS - 1)
			longjmp(comp_env, ERR_INTERNAL);
		w = B.b_word + i;
		while (i--)
		{
			*w = *(w - 1);
			w--;
		}
		*w = 0;
		return;
	}
	i = B.b_words;
	w = B.b_word;
	c = 0;
	while (i--)
	{
		c += (COMP)*w * (COMP)a;
		*(w++) = (WORD)(c & WORDMASK);
		c >>= BITSPERWORD;
	}
	if (c)
	{
		if (B.b_words++ >= MAXWORDS)
			longjmp(comp_env, ERR_INTERNAL);
		*w = (COMP)(c & WORDMASK);
	}
}

#if 0
/* Subtract a from B storing the result in B
 */
void
BigSub(a)
WORD a;
{
	register int i;
	register WORD *w;
	register COMP c;

	a &= WORDMASK;
	if (a == 0)
		return;
	i = 1;
	w = B.b_word;
	c = (COMP)*w - (COMP)a;
	*w = (WORD)(c & WORDMASK);
	while (c & WORDCARRY)
	{
		if (i >= B.b_words)
			longjmp(comp_env, ERR_INTERNAL);
		c = (COMP)*++w - 1;
		*w = (WORD)(c & WORDMASK);
		i++;
	}
	if ((i == B.b_words) && (*w == 0) && (i > 0))
		B.b_words--;
}
#endif

/* Add to a to B storing the result in B
 */
void
BigAdd(a)
WORD a;
{
	register int i;
	register WORD *w;
	register COMP c;

	a &= WORDMASK;
	if (a == 0)
		return;
	i = 0;
	w = B.b_word;
	c = a;
	while ((i < B.b_words) && c)
	{
		c += (COMP)*w;
		*w++ = (WORD)(c & WORDMASK);
		c >>= BITSPERWORD;
		i++;
	}
	if ((i == B.b_words) && c)
	{
		if (B.b_words++ >= MAXWORDS)
			longjmp(comp_env, ERR_INTERNAL);
		*w = (COMP)(c & WORDMASK);
	}
}

void
BigClear()
{
	B.b_words = 0;
}
