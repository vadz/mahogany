/*-*- c++ -*-********************************************************
 * miscutil.h : miscellaneous utility functions                     *
 *                                                                  *
 * (C) 1999 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/


#ifndef MISCUTIL_H
#define MISCUTIL_H

#ifndef  USE_PCH
#  include  "Mconfig.h"
#endif

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
                                 class MFrame *frame = NULL);

/// construct the full email address of the form "name <email>"
inline String GetFullEmailAddress(const String& name, const String& email)
{
   if ( !name )
      return email;

   String address(name);
   address << " <" << email << '>';

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



//@}
#endif // MISCUTIL_H

