/*-*- c++ -*-********************************************************
 * MimeTypes.h : a list representing the mimetypes database         *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.1  1998/03/14 12:21:12  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifndef MIMETYPES_H
#define MIMETYPES_H

#ifdef __GNUG__
#pragma interface "MimeTypes.h"
#endif

#include	<Mcommon.h>
#include	<CommonBase.h>

#include	<list>
#include	<iostream.h>

/**
   MimeTypes - maps file extensions to mime types.
*/
class MimeTEntry
{
   /// type
   String	type;
   /// list of extensions for this type
   list<String> extensions;
   friend class MimeTypes;
public:
   MimeTEntry();
   /** Create an entry from a mime.types line
       @param str the line from mime.types
       @return true if a new entry was created
   */
   bool	Parse(String const & str);
   bool Match(String const & extension, String &mimeType);
};

/**
   MimeTypes - mapping of Mime types to icons and handlers
*/

class MimeTypes : public list<MimeTEntry>, public CommonBase
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
