/*-*- c++ -*-********************************************************
 * miscutil.cpp : miscellaneous utility functions                   *
 *                                                                  *
 * (C) 1999 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *                                                                  *
 *******************************************************************/

#include "Mpch.h"
#include "Mcommon.h"

#ifndef   USE_PCH
#   include "sysutil.h"
#   include "strutil.h"
#   include "guidef.h"
#   include "gui/wxMFrame.h"
#   include "MApplication.h"
#endif

#include <wx/colour.h>
#include <wx/bitmap.h>
#include <wx/image.h>

#include "gui/wxIconManager.h"

#include "adb/AdbManager.h"
#include "adb/AdbBook.h"

#include "MDialogs.h"
#include "MailFolder.h"    // UpdateTitleAndStatusBars uses it
#include "ASMailFolder.h"
#include "miscutil.h"

#include <ctype.h>

void
AutoCollectAddresses(const String &email,
                     String name,
                     int autocollectFlag,
                     bool collectNamed,
                     const String& bookName,
                     const String& groupName,
                     MFrame *frame)
{
   // we need an address and a name
   bool hasEmailAndName = true;
   if ( email.IsEmpty() )
   {
      // won't do anything without email address
      hasEmailAndName = false;
   }
   else if ( name.IsEmpty() )
   {
      // if there is no name and we want to autocollect such addresses
      // (it's a global option), take the first part of the e-mail address
      // for the name
      if ( !collectNamed )
      {
         // will return the whole string if '@' not found - ok
         name = email.BeforeFirst('@');
      }
      else
         hasEmailAndName = false;
   }

   if ( hasEmailAndName )
   {
      AdbManager_obj manager;
      CHECK_RET( manager, "can't get AdbManager" );

      // load all address books mentioned in the profile
      manager->LoadAll();

      // and also explicitly load autocollect book - it might have not been
      // loaded by LoadAll(), yet we want to search in it too
      // won't recreate it if it already exists
      AdbBook *autocollectbook = manager->CreateBook(bookName);

      if ( !autocollectbook )
      {
         wxLogError(_("Failed to create the address book '%s' "
                      "for autocollected e-mail addresses."),
                      bookName.c_str());

         // TODO ask the user if he wants to disable autocollec?
      }

      ArrayAdbEntries matches;
      if( !AdbLookup(matches, email, AdbLookup_EMail, AdbLookup_Match) )
      {
         if ( AdbLookup(matches, name, AdbLookup_FullName, AdbLookup_Match) )
         {
            // found: add another e-mail (it can't already have it, otherwise
            // our previous search would have succeeded)
            AdbEntry *entry = matches[0];
            entry->IncRef();
            entry->AddEMail(email);

            wxString name;
            entry->GetField(AdbField_NickName, &name);
            if ( frame )
            {
               wxLogStatus(frame,
                           _("Auto collected e-mail address '%s' "
                             "(added to the entry '%s')."),
                           email.c_str(), name.c_str());
            }
            entry->DecRef();
         }
         else // no such address, no such name - create a new entry
         {
            // the value is either 1 or 2, if 1 we have to ask
            bool askUser = autocollectFlag == 1;
            if (
               !askUser ||
               MDialog_YesNoDialog(_("Add new e-mail entry to database?"),
                                   frame)
               )
            {
               // avoid creating groups with '/'s in the names - this will
               // create nested groups!
               wxString adbGroupName;
               if ( groupName[0u] == '/' )
                  adbGroupName = groupName.c_str() + 1;
               else
                  adbGroupName = groupName;
               adbGroupName.Replace("/", "_");
               // VZ: another possibility might be:
               //adbGroupName = groupName.AfterLast('/')
               // I don't know what's better...

               AdbEntryGroup *group = autocollectbook->CreateGroup(adbGroupName);
               if ( !group )
               {
                  // fall back to the root
                  group = autocollectbook;
               }

               AdbEntry *entry = group->CreateEntry(name);

               if ( !entry )
               {
                  wxLogError(_("Couldn't create an entry in the address "
                               "book '%s' for autocollected address."),
                             bookName.c_str());

                  // TODO ask the user if he wants to disable autocollec?
               }
               else
               {
                  entry->SetField(AdbField_NickName, name);
                  entry->SetField(AdbField_FullName, name);
                  entry->SetField(AdbField_EMail, email);

                  wxString comment;
                  {
                     // get the timestamp
                     time_t timeNow;
                     time(&timeNow);
                     struct tm *ptmNow = localtime(&timeNow);

                     char szBuf[128];
                     strftime(szBuf, WXSIZEOF(szBuf),
                              "%d/%b/%Y %H:%M:%S", ptmNow);

                     comment.Printf(_("This entry was automatically added on %s."), szBuf);
                  }

                  entry->SetField(AdbField_Comments, comment);
                  entry->DecRef();

                  if ( frame )
                  {
                     wxLogStatus(frame,
                                 _("Auto collected e-mail address '%s' "
                                   "(created new entry '%s' in group '%s')."),
                                 email.c_str(),
                                 name.c_str(),
                                 group->GetName().c_str());
                  }
               }

               if ( group != autocollectbook )
               {
                  group->DecRef();
               }
            }
         }
      }
      else if ( matches.GetCount() == 1 )
      {
         // there is already an entry which has this e-mail, don't create
         // another one (even if the name is different it's more than likely
         // that it's the same person) -- but check that the existing entry
         // does have a name, and set it if it doesn't but we have the "real"
         // name (i.e. not the one extracted from email address)
         String nameFromEmail = email.BeforeFirst('@');
         if ( name != nameFromEmail )
         {
            AdbEntry *entry = matches[0];
            wxString nameOld;
            entry->GetField(AdbField_FullName, &nameOld);

            // autocreated entries have the full name == nick name
            if ( !nameOld || nameOld == nameFromEmail )
            {
               entry->SetField(AdbField_FullName, name);
            }
         }
         //else: we don't have the real fullname anyhow
      }
      else
      {
         // more than one match for this email - hence at least some of them
         // were created by the user (we wouldn't do it automatically) and we
         // won't change them
      }

      // release the found items (if any)
      size_t count = matches.Count();
      for ( size_t n = 0; n < count; n++ )
      {
         matches[n]->DecRef();
      }

      // release the book
      autocollectbook->DecRef();
   }
   else
   {
      // it's not very intrusive and the user might wonder why the address
      // wasn't autocollected otherwise
      if ( frame )
      {
         wxLogStatus(frame,
                  _("'%s': the name is missing, address was not "
                    "autocollected."), email.c_str());
      }
   }
}

// show the number of new/unread/total messages in the title and status bars
void UpdateTitleAndStatusBars(const String& title,
                              const String& status,
                              wxFrame *frame,
                              const MailFolder *mailFolder)
{
   unsigned long total = mailFolder->CountMessages(),
                 recent =  mailFolder->CountMessages(MailFolder::MSG_STAT_RECENT |
                                                     MailFolder::MSG_STAT_SEEN,
                                                     MailFolder::MSG_STAT_RECENT |
                                                     MailFolder::MSG_STAT_SEEN),
                 // recent & !seen --> new
                 newmsgs = mailFolder->CountMessages(MailFolder::MSG_STAT_RECENT |
                                                     MailFolder::MSG_STAT_SEEN,
                                                     MailFolder::MSG_STAT_RECENT);

   // contruct the messages
   wxString tmp; // string we use for numeric => string conversion
   wxString titleMsg(title), statusMsg(status);

   if ( total > 1 )
   {
      tmp.Printf("%lu messages", total);
   }
   else if ( total == 1 )
   {
      tmp = _("1 message");
   }
   else // total == 0
   {
      tmp = _("no messages");
   }
   statusMsg << " (" << tmp;

   // don't say "0 recent messages", just don't include this part of the
   // message if there are none of them
   if ( recent > 0 )
   {
      tmp.Printf(_(", %lu recent"), recent);

      statusMsg += tmp;
   }

#if 0
   // and the same for new messages. Also, in the title bar we show
   // either "new/total" or just "total" (or nothing at all if there are
   // no messages)
   if ( newmsgs > 0 )
   {
      tmp.Printf(_(", %lu new"), newmsgs);
      statusMsg += tmp;

      tmp.Printf(" (%lu/%lu)", newmsgs, total);
      titleMsg += tmp;
   }
   else if ( total > 0 )
   {
      tmp.Printf(" (%lu)", total);

      titleMsg += tmp;
   }
#endif
   tmp.Printf(" (%lu/%lu)", total, newmsgs);
   titleMsg += tmp;
   statusMsg << ')';

   wxLogStatus(frame, statusMsg);
   frame->SetTitle(titleMsg);

   if(newmsgs > 0)
      frame->SetIcon( frame == mApplication->TopLevelFrame() ?
                      ICON("MainFrameNewMail") : ICON("MFrameNewMail"));
   else
      frame->SetIcon( frame == mApplication->TopLevelFrame() ?
                      ICON("MainFrame") : ICON("MFrame"));
}

// ---------------------------------------------------------------------------
// colour to strign conversion
// ---------------------------------------------------------------------------

static const char *rgbSpecificationString = gettext_noop("RGB(%d, %d, %d)");

bool ParseColourString(const String& name, wxColour* colour)
{
   wxString customColourString(wxGetTranslation(rgbSpecificationString));

   // first check if it's a RGB specification
   int red, green, blue;
   if ( sscanf(name, customColourString, &red, &green, &blue) == 3 )
   {
      // it's a custom colour
      if ( colour )
         colour->Set(red, green, blue);

      return TRUE;
   }
   else // a colour name
   {
      wxColour col(name);
      if ( col.Ok() && colour )
         *colour = col;

      return col.Ok();
   }
}

String GetColourName(const wxColour& colour)
{
   wxString colName(wxTheColourDatabase->FindName(colour));
   if ( !colName )
   {
      // no name for this colour
      colName.Printf(wxGetTranslation(rgbSpecificationString),
                     colour.Red(), colour.Green(), colour.Blue());
   }
   else
   {
      // at least under X the string returned is always capitalized,
      // convert it to lower case (it doesn't really matter, but capitals
      // look ugly)
      colName.MakeLower();
   }

   return colName;
}

// get the colour by name and warn the user if it failed
void GetColourByName(wxColour *colour,
                     const String& name,
                     const String& def)
{
   if ( !ParseColourString(name, colour) )
   {
      wxLogError(_("Cannot find a colour named '%s', using default instead.\n"
                   "(please check the settings)"), name.c_str());
      *colour = def;
   }
}



String GetSequenceString(const INTARRAY *array)
{
   String sequence;
   for(size_t i = 0; i < array->Count(); i++)
   {
      sequence += strutil_ultoa((*array)[i]);
      sequence += ',';
   }
   sequence = sequence.substr(0,sequence.Length()-1); //strip off comma
   return sequence;
}

INTARRAY *GetAllMessagesSequence(ASMailFolder *mf)
{
   ASSERT(mf);
   INTARRAY *sequence = new INTARRAY;
   HeaderInfoList *hil = mf->GetHeaders();
   if(hil)
   {
      for(size_t i = 0; i < hil->Count(); i++)
         sequence->Add((*hil)[i]->GetUId());
      hil->DecRef();
   }
   return sequence;
}
