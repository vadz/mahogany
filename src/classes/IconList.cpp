/*-*- c++ -*-********************************************************
 * PersistentList.cc - a persistent STL compatible list class       *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.1  1998/03/14 12:21:19  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifdef __GNUG__
#pragma implementation "PersistentList.h"
#endif

#include <PersistentList.h>

#include <fstream.h>

PersistentList::PersistentList(String const &ifilename)
   : list<PLEntry *>()
{
   filename = ifilename;

   ifstream	istr(filename.c_str());
   
}

PersistentList::~PersistentList()
{
}
