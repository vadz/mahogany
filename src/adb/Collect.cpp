///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   adb/Collect.cpp - address (auto) collection routines
// Purpose:     this module defines functions to automatically or interactively
//              collect (i.e. save in ADB) a set of email addresses
// Author:      Vadim Zeitlin
// Modified by:
// Created:     09.11.00
// CVS-ID:      $Id$
// Copyright:   (c) 2000 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"

   #include <wx/log.h>
   #include <wx/frame.h>
#endif // USE_PCH

#include "gui/wxMDialogs.h"

#include "Address.h"
#include "Collect.h"
#include "Message.h"
#include "MailFolder.h"
#include "pointers.h"

#include "adb/AdbManager.h"
#include "adb/AdbBook.h"
#include "adb/AdbDataProvider.h"
#include "adb/AdbEntry.h"
#include "adb/AdbImport.h"

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

DECLARE_AUTOPTR(AdbEntry);
DECLARE_AUTOPTR(AdbEntryGroup);
DECLARE_AUTOPTR(AdbBook);

// ============================================================================
// implementation of our public API
// ============================================================================

void AutoCollectAddresses(const Message *message,
                          int autocollectFlag,
                          bool senderOnly,
                          bool collectNamed,
                          const String& bookName,
                          const String& groupName,
                          wxFrame *frame)
{
   static const MessageAddressType addressTypesToCollect[] =
   {
      // In this array, the values corresponding to 'Sender' headers
      // (e.g. From and ReplyTo) must appear before the others. And if
      // some other 'sender' headers must be taken into account, the ending
      // index in the for loop below (named stopAt) must be changed.
      MAT_REPLYTO,
      MAT_FROM,
      MAT_TO,
      MAT_CC,
   };

   // the email addresses we have already seen
   wxArrayString addressesSeen;

   const size_t stopAt = senderOnly ? 2 : WXSIZEOF(addressTypesToCollect);
   for ( size_t n = 0; n < stopAt; n++ )
   {
      AddressList *addrList = message->GetAddressList(addressTypesToCollect[n]);
      if ( !addrList )
         continue;

      for ( Address *addr = addrList->GetFirst();
            addr;
            addr = addrList->GetNext(addr) )
      {
         const String email = addr->GetEMail();
         if ( addressesSeen.Index(email) == wxNOT_FOUND )
         {
            addressesSeen.Add(email);

            AutoCollectAddress(email,
                               addr->GetName(),
                               autocollectFlag,
                               collectNamed,
                               bookName,
                               groupName,
                               frame);
         }
      }

      addrList->DecRef();
   }
}

void AutoCollectAddress(const String& email,
                        const String& nameOrig,
                        int autocollectFlag,
                        bool collectNamed,
                        const String& bookName,
                        const String& groupName,
                        wxFrame *frame)
{
   String name = nameOrig;
   name = MailFolder::DecodeHeader(name, 0);

   // we need an address and a name
   bool hasEmailAndName = true;
   if ( email.empty() )
   {
      // won't do anything without email address
      hasEmailAndName = false;
   }
   else if ( name.empty() )
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
      CHECK_RET( manager, _T("can't get AdbManager") );

      String providerName;
      
      AdbBook *autocollectbook = manager->CreateBook(
         bookName, NULL, &providerName );
         
      RefCounter<AdbDataProvider> bookProvider(
         AdbDataProvider::GetProviderByName(providerName));

      if ( !autocollectbook )
      {
         wxLogError(_("Failed to create the address book '%s' "
                      "for autocollected e-mail addresses."),
                      bookName.c_str());

         // TODO ask the user if he wants to disable autocollect?
         return;
      }

      // NB: we only look for this entry in the group where we're going to
      //     create it as looking for it in all address books (or even just in
      //     the entire autocollectbook) is too slow currently - and this
      //     is, anyhow, not needed in 99% of cases because you either get a
      //     message from a person who had already posted to this folder or
      //     from a person you don't know, usually. And even if/when you do get
      //     a mail from someone whose address you already have, it's still
      //     better to create a duplicate entry than to annoy the user with a
      //     long delay

      AdbEntryGroup *group;
      if( !groupName.empty() )
      {
         wxString adbGroupName;
         if ( groupName[0u] == '/' )
            adbGroupName = groupName.c_str() + 1;
         else
            adbGroupName = groupName;

         group = autocollectbook->CreateGroup(adbGroupName);
      }
      else
         group = NULL;

      if ( !group )
      {
         // fall back to the root
         group = autocollectbook;
      }

      ArrayAdbEntries matches;
      if( !AdbLookup(matches, email,
                     AdbLookup_EMail, AdbLookup_Match,
                     group) )
      {
         if ( AdbLookup(matches, name,
                        AdbLookup_FullName, AdbLookup_Match,
                        group) )
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
                  MDialog_YesNoDialog
                  (
                     String::Format
                     (
                        _("Add new e-mail entry '%s' for '%s' to the database?"),
                        email.c_str(),
                        name.c_str()
                     ),
                     frame
                  )
               )
            {
               AdbEntry *entry = group->CreateEntry(name);

               if ( !entry )
               {
                  wxLogError(_("Couldn't create an entry in the address "
                               "book '%s' for autocollected address."),
                             bookName.c_str());

                  // TODO ask the user if he wants to disable autocollect?
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

      // release the objects we created
      if ( group != autocollectbook )
      {
         group->DecRef();
      }

      autocollectbook->DecRef();
   }
   else // invalid address
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

int InteractivelyCollectAddresses(const wxArrayString& addresses,
                                  const String& bookNameOrig,
                                  const String& groupNameOrig,
                                  wxFrame *parent)
{
    // by default, select all addresses
    wxArrayInt selections;
    size_t count = addresses.GetCount();
    for ( size_t n = 0; n < count; n++ )
    {
       selections.Add(n);
    }

    if ( count > 1 )
    {
       // now ask the user which ones does he really want
       count = MDialog_GetSelections
                      (
                       _("Please select the addresses to save"),
                       _("Save addresses"),
                       addresses,
                       &selections,
                       parent,
                       _T("AddrExtract")
                      );
    }
    //else: don't ask the user to choose when there is one address only

    if ( count > 0 )
    {
       // ask the user for the book and group names to use
       //
       // TODO propose something better - ADB tree dialog for example
       wxString bookName(bookNameOrig),
                groupName(groupNameOrig);
       if ( MDialog_GetText2FromUser
            (
               _("Please select the location in the address\n"
                 "book to save the selected entries to:"),
               _("Save addresses"),
               _("Address book name:"), &bookName,
               _("Group name:"), &groupName,
               parent
            ) )
       {
         AdbManager_obj manager;
         CHECK( manager, -1, _T("can't get AdbManager") );

         AdbBook_obj autocollectbook(manager->CreateBook(bookName));

         if ( !autocollectbook )
         {
            wxLogError(_("Failed to create the address book '%s' "
                         "for autocollected e-mail addresses."),
                         bookName.c_str());

            // TODO ask the user for another book name
            return -1;
         }

         AdbEntryGroup_obj group(autocollectbook->CreateGroup(groupName));
         if ( !group )
         {
            wxLogError(_("Failed to create group '%s' in the address "
                         "book '%s'."),
                         groupName.c_str(), bookName.c_str());

            return -1;
         }

         // create all entries in this group
         for ( size_t n = 0; n < count; n++ )
         {
            wxString addr = addresses[selections[n]];
            wxString name = Message::GetNameFromAddress(addr),
                     email = Message::GetEMailFromAddress(addr);

            if ( name.empty() || (name == email) )
            {
               name = email.BeforeFirst('@');
            }

            AdbEntry_obj entry(group->CreateEntry(name));
            entry->SetField(AdbField_NickName, name);
            entry->SetField(AdbField_FullName, name);
            entry->SetField(AdbField_EMail, email);
         }

         wxLogStatus(parent, _("Saved %u addresses."), count);
       }
       //else: cancelled
    }
    else // cancelled by user or nothing selected
    {
       wxLogStatus(parent, _("No addresses to save."));
    }

    return count;
}
