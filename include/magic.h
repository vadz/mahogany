/*-*- c++ -*-********************************************************
 * Magic: define magic numbers for object validation                *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.1  1998/03/14 12:21:13  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifndef MAGIC_H
#define	MAGIC_H

#define	DEFINE_MAGIC(classname,magic)	unsigned long classname::cb_class_magic = magic;
#define	SET_MAGIC(magic)		cb_magic = magic;

#define	MAGIC_COMMONBASE	12345678



#endif	// MAGIC_H
