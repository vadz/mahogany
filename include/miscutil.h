/*-*- c++ -*-********************************************************
 * miscutil.h : miscellaneous utility functions                     *
 *                                                                  *
 * (C) 1999-2000 by Karsten Ballüder (Ballueder@gmx.net)            *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/


#ifndef MISCUTIL_H
#define MISCUTIL_H

#ifndef  USE_PCH
#   include "Mconfig.h"
#   include "FolderType.h"
#   include "Profile.h"
#endif

class WXDLLEXPORT wxArrayString;
class ASMailFolder;
class UIdArray;

/**@name Miscellaneous utility functions */
//@{
/** Automatic collection of an email address/name pair into the given subgroup
    of the given addressbook.
    @param email email address string
    @param name  User name or empty
    @param autocollectFlag the 0,1,2 value whether to do auto collection or not
    @param collectNamed if true, only collect addresses with a name
    @param bookName the address book name for auto collection
    @param groupName the subgroup name in bookName where to put new entries
    @param frame optional pointer to a frame for displaying status messages
*/
extern void AutoCollectAddresses(const String &email,
                                 String name,
                                 int autocollectFlag,
                                 bool collectNamed,
                                 const String& bookName,
                                 const String& groupName,
                                 class wxFrame *frame = NULL);

/**
   Interactive collection of email addresses: present the user with a dialog
   allowing to choose any of the provided addresses and save them into an
   address book/group of choice.

   @param addresses contains all addresses to collect
   @param bookName the default address book name for auto collection
   @param groupName the default subgroup name in bookName where to put new entries
   @param frame optional pointer to a frame for displaying status messages
   @return the number of the addresses saved, -1 if cancelled
 */
extern int InteractivelyCollectAddresses(const wxArrayString& addresses,
                                         const String& bookName,
                                         const String& groupName,
                                         wxFrame *parent = NULL);

/// construct the full email address of the form "name <email>"
inline String GetFullEmailAddress(const String& name, const String& email)
{
   if ( !name )
      return email;

   String address(name);
   if ( !email.empty() )
   {
      if ( !name.empty() )
         address += " <";

      address += email;

      if ( !name.empty() )
         address += '>';
   }

   return address;
}

class wxFrame;
class MailFolder;

/// set the title and statusbar to show the number of messages in folder
extern void UpdateTitleAndStatusBars(const String& title,
                                     const String& status,
                                     wxFrame *frame,
                                     const MailFolder *folder);

class wxColour;

/// get the colour by name which may be either a colour or RGB specification
extern bool ParseColourString(const String& name, wxColour* colour = NULL);

/// get the colour name - pass it to ParseColorString to get the same colour
extern String GetColourName(const wxColour& color);

/// get the colour by name and fallback to default (warning the user) if failed
extern void GetColourByName(wxColour *colour,
                            const String& name,
                            const String& defaultName);


/// Converts an UIdArray to a sequence string (for backward compatibility):
extern String GetSequenceString(const UIdArray *sequence);

/// Gets an UIdArray with all message uids in the folder:
extern UIdArray *GetAllMessagesSequence(ASMailFolder *mf);

/// Get the user's From mail address from a profile
extern String miscutil_GetFromAddress(Profile *p,
                                      String * personal = NULL,
                                      String * email = NULL);
/// Get the user's reply address from a profile
extern String miscutil_GetReplyAddress(Profile *p);
                                       //String * personal = NULL,
                                       //String * email = NULL);
/// Expand a local (user-only) mail address
extern String miscutil_ExpandLocalAddress(Profile *p, const String &mbox);

/// Get Default hostname or empty for mail addresses
extern String miscutil_GetDefaultHost(Profile *p);
//@}
#endif // MISCUTIL_H

