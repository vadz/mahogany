/*-*- c++ -*-********************************************************
 * PersistentList.cc - a persistent STL compatible list class       *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.3  2002/10/19 17:40:39  nerijus
 * using wxUSE_IOSTREAMH for including <iostream.h> or <iostream>
 *
 * Revision 1.2  1998/03/26 23:05:39  VZ
 * Necessary changes to make it compile under Windows (VC++ only)
 * Header reorganization to be able to use precompiled headers
 *
 * Revision 1.1  1998/03/14 12:21:19  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#include <PersistentList.h>

#if wxUSE_IOSTREAMH
#  include <fstream.h>
#else
#  include <fstream>
#endif

PersistentList::PersistentList(String const &ifilename)
   : list<PLEntry *>()
{
   filename = ifilename;

   ifstream	istr(filename.c_str());
   
}

PersistentList::~PersistentList()
{
}
