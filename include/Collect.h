///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   Collect.h - functions for address auto collection
// Purpose:     we have 2 simple functions to collect (i.e. remember in the
//              address book) address(es)
// Author:      Vadim Zeitlin
// Modified by:
// Created:     18.08.01 (extracted from miscutil.h)
// CVS-ID:      $Id$
// Copyright:   (c) 2001 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M licence
///////////////////////////////////////////////////////////////////////////////

class Message;

/** Automatically collect all interesting addresses from this message

    @param message the message to collect addresses from
    @param autocollectFlag the 0,1,2 value whether to do auto collection or not
    @param collectNamed if true, only collect addresses with a name
    @param bookName the address book name for auto collection
    @param groupName the subgroup name in bookName where to put new entries
    @param frame optional pointer to a frame for displaying status messages
*/
extern void AutoCollectAddresses(const Message *message,
                                 int autocollectFlag,
                                 bool collectNamed,
                                 const String& bookName,
                                 const String& groupName,
                                 class wxFrame *frame = NULL);

/** Automatic collection of one email address/name pair into the given subgroup
    of the given addressbook.
    @param email email address string
    @param name  User name or empty
    @param autocollectFlag the 0,1,2 value whether to do auto collection or not
    @param collectNamed if true, only collect addresses with a name
    @param bookName the address book name for auto collection
    @param groupName the subgroup name in bookName where to put new entries
    @param frame optional pointer to a frame for displaying status messages
*/
extern void AutoCollectAddress(const String& email,
                               const String& name,
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

