///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   adb/AdbManager.h - ADB manager public interface
// Purpose:     AdbManager manages all AdbBooks used by the application
// Author:      Vadim Zeitlin
// Modified by: 
// Created:     09.08.98
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef   _ADBMANAGER_H
#define   _ADBMANAGER_H

#include "MObject.h"        // the base class declaration

#include "adb/AdbEntry.h"   // for AdbLookup_xxx constants

// forward declaration for classes we use
class AdbBook;
class AdbDataProvider;
class ArrayAdbEntries;
class ArrayAdbBooks;

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

  @@@ This seems a bit complicated but I don't know how to ensure that there is
      at most one object of this type otherwise...
*/
class AdbManager : public MObject
{
public:
  // static functions
    /// retrieve the manager object, creating one if it doesn't yet exist
  static AdbManager *Get();
    /// decrement the ref count of the manager object
    //  NB: you can't use the pointer returned by Get() after Unget()!
  static void Unget();

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

  MOBJECT_DEBUG

private:
  // dtor
  virtual ~AdbManager();

  // ctor is private because we're only created by Get()
  AdbManager() { }

  // helper
  AdbBook *FindInCache(const String& name) const;

  static AdbManager *ms_pManager;
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
  @param paBooks is the array of books to search the entry in and may be NULL

  @return FALSE if no matches, TRUE otherwise
*/
bool AdbLookup(ArrayAdbEntries& aEntries,
               const String& what,
               int where = AdbLookup_NickName |
                           AdbLookup_FullName |
                           AdbLookup_EMail,
               int how = AdbLookup_Substring,
               const ArrayAdbBooks *paBooks = NULL);

#endif  //_ADBMANAGER_H
