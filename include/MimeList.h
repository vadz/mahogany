/*-*- c++ -*-********************************************************
 * MimeList: an persistent list of Mime mappings                    *
 *                                                                  *
 * (C) 1997,1998 by Karsten Ballüder (Ballueder@usa.net)            *
 *                                                                  *
 * $Id$
 *******************************************************************/

#ifndef MIMELIST_H
#define MIMELIST_H

#ifdef __GNUG__
#pragma interface "MimeList.h"
#endif

#ifndef   MIME_UNKNOWN_ICON
/// the icon to use if no other is known
#   define   MIME_UNKNOWN_ICON   "unknown"
#endif

#ifndef   MIME_UNKNOWN_COMMAND
/// this marks an unknown Mime command
#   define   MIME_UNKNOWN_COMMAND   ""
#endif

#ifndef   MIME_DEFAULT_FLAGS
/// this marks an empty flags entry
#   define   MIME_DEFAULT_FLAGS    ""
#endif

#ifndef  USE_PCH
#  include "kbList.h"
#endif

/**
   MimeEntry - holds information about a Mime type
*/
class MimeEntry : public MObject
{
      
   /// type
   String   type;
   /// command to handle it
   String   command;
   /// flags settings
   String   flags;
   friend class MimeList;
public:
   MimeEntry();
   MimeEntry(String const & type,
             String const & command = MIME_UNKNOWN_COMMAND,
             String const & flags = MIME_DEFAULT_FLAGS);

   /** Create an entry from a mailcap line
       @param str the line from mailcap
       @return true if a new entry was found
   */
   bool   Parse(String const & str);
};

/**
   MimeList - mapping of Mime types to icons and handlers
*/

KBLIST_DEFINE(MimeEntryList, MimeEntry);

class MimeList : public MObject, public MimeEntryList
{
//   DECLARE_CLASS(MimeList)
   
public:
   /** Constructor
     */
   MimeList(void);

   /** Destructor
       writes back all entries
   */
   ~MimeList();

   /** Get command and flags for this type.
       @param type   the type we are looking for
       @param command   string reference where to store command
       @param flags   string reference where to store flags
       @return true if found
   */
   bool GetCommand(String const & type, String &command, String
                   &flags);

   /** Expands a command line.
       @param commandline string including %s
       @param filename the file to handle
       @param mimetype the mime type name
       @return the command line
   */
   String ExpandCommand(String const &commandline,
                        String const &filename,
                        String const &mimetype = "");
   
};

#endif
