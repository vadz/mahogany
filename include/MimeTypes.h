/*-*- c++ -*-********************************************************
 * MimeTypes.h : a list representing the mimetypes database         *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.5  1998/05/24 14:47:12  KB
 * lots of progress on Python, but cannot call functions yet
 * kbList fixes again?
 *
 * Revision 1.4  1998/05/13 19:01:40  KB
 * added kbList, adapted MimeTypes for it, more python, new icons
 *
 * Revision 1.3  1998/04/22 19:54:47  KB
 * Fixed _lots_ of problems introduced by Vadim's efforts to introduce
 * precompiled headers. Compiles and runs again under Linux/wxXt. Header
 * organisation is not optimal yet and needs further
 * cleanup. Reintroduced some older fixes which apparently got lost
 * before.
 *
 * Revision 1.2  1998/03/26 23:05:37  VZ
 * Necessary changes to make it compile under Windows (VC++ only)
 * Header reorganization to be able to use precompiled headers
 *
 * Revision 1.1  1998/03/14 12:21:12  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifndef MIMETYPES_H
#define MIMETYPES_H

#ifdef __GNUG__
#pragma interface "MimeTypes.h"
#endif

#include   "kbList.h"

/**
   MimeTypes - maps file extensions to mime types.
*/
class MimeTEntry
{
   /// type
   String type;
   /// list of extensions for this type
   kbStringList extensions;
   friend class MimeTypes;
public:
   MimeTEntry();
   /** Create an entry from a mime.types line
       @param str the line from mime.types
   */
   MimeTEntry(String const & str);
   /** Create an entry from a mime.types line
       @param str the line from mime.types
       @return true if a new entry was created
   */
   bool	Parse(String const & str);
   bool Match(String const & extension, String &mimeType);

   IMPLEMENT_DUMMY_COMPARE_OPERATORS(MimeTEntry);
};

/**
   MimeTypes - mapping of Mime types to icons and handlers
*/

KBLIST_DEFINE(MimeTEntryList, MimeTEntry);
class MimeTypes : public MimeTEntryList, public CommonBase
{
public:
   /** Constructor
   */
   MimeTypes(void);

   /** Destructor
       writes back all entries
   */
   ~MimeTypes();

   /** Get command and flags for this type.
       @param filename  the filename to map
       @param mimeType	the string which gets set to the mime type
       @param numericType a pointer where to return the numeric type
       @return true if found
   */
   bool Lookup(String const & filename, String &mimeType,
	       int *numericType = NULL);

   /// always initialised
   bool	IsInitialised(void) const { return true; }
   CB_DECLARE_CLASS(IconList,CommonBase);
};

#endif
