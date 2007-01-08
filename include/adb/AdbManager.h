// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //////
// Project:     M - cross platform e-mail GUI client
// File name:   adb/AdbManager.h - ADB manager public interface
// Purpose:     AdbManager manages all AdbBooks used by the application
// Author:      Vadim Zeitlin
// Modified by:
// Created:     09.08.98
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //////

#ifndef   _ADBMANAGER_H
#define   _ADBMANAGER_H

#include "MObject.h"        // the base class declaration

#include "adb/AdbEntry.h"   // for AdbLookup_xxx constants

#include "RecipientType.h"

#ifndef USE_PCH
#  include <wx/dynarray.h>
#endif // USE_PCH

// forward declaration for classes we use
class AdbBook;
class AdbDataProvider;

class WXDLLEXPORT wxFrame;

// arrays
WX_DEFINE_ARRAY(AdbBook *, ArrayAdbBooks);
WX_DEFINE_ARRAY(AdbEntryGroup *, ArrayAdbGroups);
WX_DEFINE_ARRAY(AdbEntry *, ArrayAdbEntries);
WX_DEFINE_ARRAY(AdbElement *, ArrayAdbElements);

/**
  A book corresponds to a physical medium (disk file, database...), the
  manager provides methods to access all the books used as well as methods for
  creation of the new books and deletion of the existing ones. For this, it
  uses the AdbDataProviders each of which can work with the address books of
  the given type. In the most common case, an address book is just a disk file
  and a data provider is a class which understands the format of this file and
  can read/write it. However, a book might as well correspond to a database
  table or even to a public directory service on the Internet.

  See AdbDataProvider.h for more about data providers (and especially how to
  create them).

  There is always (at most) one AdbManager object which can be accessed with
  the static Get function. Get() increments the ref count of the object if
  it already exists, so you should call Unget() when you're done with it
  (internally, each Get() does a Lock() and Unget() - Unlock()).

  Please see AdbManager_obj class below for an easy way to never forget to
  call Unget().

  @@@ This seems a bit complicated but I don't know how to ensure that there is
      at most one object of this type otherwise...
*/
class AdbManager : public MObjectRC
{
public:
  // static functions
    /// retrieve the manager object, creating one if it doesn't yet exist
  static AdbManager *Get();
    /// decrement the ref count of the manager object
    //  NB: you can't use the pointer returned by Get() after Unget()!
  static void Unget();
    // this function is for private usage (of MApplication) only, it deletes the
    // ADB manager (regardless of the number of references to it) on application
    // exit ensuring that all ADB data is saved, even if there is a bug in ref
    // counting code somewhere
  static void Delete();

  // accessors
    /// get the number of books currently open
  size_t GetBookCount() const;
    /// get the pointer to the given book
  AdbBook *GetBook(size_t n) const;

  // operations
    /// create a new book and return it (if provider == NULL all are tried)
    //  if providerName != NULL it's filled with the name of provider used
    //  to create the book
  AdbBook *CreateBook(const String& name,
                      AdbDataProvider *provider = NULL,
                      String *providerName = NULL);
    /// delete the given book
  void DeleteBook(size_t n);

    /// load all books stored in our config file
  void LoadAll();

    /// clear the cache contents freeing memory used by all ADBs in it
  void ClearCache();

  MOBJECT_DEBUG(AdbManager)

private:
  // dtor
  virtual ~AdbManager();

  // ctor is private because we're only created by Get()
  AdbManager() { }

  // helper
  AdbBook *FindInCache(const String& name,
                       const AdbDataProvider *provider) const;

   static AdbDataProvider *AutodetectFormat(const String& name);
   static AdbDataProvider *CreateNative(const String& name);
   static AdbDataProvider *MatchBookName(const String& name);

  static AdbManager *ms_pManager;

  GCC_DTOR_WARN_OFF
};

/**
  This class is a "smart reference" to AdbManager. It acquires a pointer to
  AdbManager in ctor with AdbManager::Get() and releases it in dtor with
  AdbManager::Unget(). It's usage is highly recommended!

  Example:
   void Foo()
   {
      AdbManager_obj adbManager;

      ...
      adbManager->LoadAll();
      nBooks = adbManager->GetBookCount();
      ...

      // nothing to do, Unget() called automagically on scope exit
   }
*/
class AdbManager_obj
{
public:
   // ctor & dtor
   AdbManager_obj() { m_manager = AdbManager::Get(); }
   ~AdbManager_obj() { AdbManager::Unget(); }

   // provide access to the real thing via operator->
   AdbManager *operator->() const { return m_manager; }

   // testing for validity
   operator bool() const { return m_manager != NULL; }

private:
   AdbManager *m_manager;
};

/**
  Looks in the specified address book(s) for the match for the given string.
  If the array of books is empty (or the pointer is NULL), all currently
  opened books are searched.

  All pointers returned in aEntries must be Unlock()ed by the caller (as usual)

  @param aEntries will be filled with the results (should be empty on input)
  @param what - what to look for
  @param where - in which fields to look for this string? default is everywhere
  @param how - search options, default is case insensitive substring search
  @param group specifies where to search, default is everywhere (NULL group)

  @return FALSE if no matches, TRUE otherwise
*/
extern bool AdbLookup(ArrayAdbEntries& aEntries,
                      const String& what,
                      int where = AdbLookup_NickName |
                                  AdbLookup_FullName |
                                  AdbLookup_EMail,
                      int how = AdbLookup_Substring,
                      AdbEntryGroup *group = NULL);

/**
  Expand the abbreviated address: i.e. looks for an address entry which starts
  with the specified text and returns it if found. If more than one address
  match, the user is proposed with a listbox from which one or more items can be
  chosen. If a group matches, all the items of this group are returned.

  @param results is the array filled with the expanded addresses
  @param what is the string to look for
  @param how AdbLookup_ value to tell it how to look up the name
  @param frame is used as parent for possible dialog boxes and also to log
         status messages (may be NULL)

  @return FALSE if no matches, TRUE if at least one item matched
*/
extern bool AdbExpand(wxArrayString& results,
                      const String& what,
                      int how,
                      wxFrame *frame);

/**
   Expand a single address coming from the user input.

   The user input can be a full address, a mailto: URL or a string to be
   expanded using AdbExpand().

   This function doesn't support multiple comma-separated addresses, use
   AdbExpandRecipients() for this. Nor does it support "cc:"-like
   prefixes selecting the recipient type, as AdbExpandSingleRecipient() does.

   @param address the address string, modified in place by this function
   @param subject filled with the subject on output if it's specified as
                  part of the address (currently only happens with mailto)
                  and is left empty otherwise
   @param profile to use for expansion options
   @param parent window to use as parent for the dialogs
   @return true if the address was expanded, false if the expansion was
           cancelled (address is unchanged then)
 */
bool AdbExpandSingleAddress(String *address,
                            String *subject,
                            Profile *profile,
                            wxFrame *win);

/**
   Expand a single address coming from the user input, possibly with a
   recipient type string.

   This function does the same thing as AdbExpandSingleAddress() except that
   accepts "to:", "cc:" &c prefixes.

   Returns the recipient type (which can be Recipient_None if none was
   explicitly specified) or Recipient_Max is expansion was cancelled and the
   address is unchanged.
 */
RecipientType AdbExpandSingleRecipient(String *address,
                                       String *subject,
                                       Profile *profile,
                                       wxFrame *win);

/**
   Expand all addresses in the specified string.

   Returns the number of addresses which can be 0 if the control was empty
   or -1 if the expansion was cancelled.

   @param text contains the addresses to expand
   @param addresses filled in with the addresses on return
   @param rcptTypes filled in with RecipientType enum elements
   @param subject filled with the subject on output
   @param profile to use for expansion options
   @param parent window to use as parent for the dialogs
   @return number of addresses expanded or -1 if cancelled
 */
int AdbExpandAllRecipients(const String& text,
                           wxArrayString& addresses,
                           wxArrayInt& rcptTypes,
                           String *subject,
                           Profile *profile,
                           wxFrame *win);

#endif  //_ADBMANAGER_H
