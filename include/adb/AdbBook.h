// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //////
// Project:     M - cross platform e-mail GUI client
// File name:   adb/AdbBook.h - address book public interface
// Purpose:     AdbBook defines an abstract interface for any ADB (sic!)
// Author:      Vadim Zeitlin
// Modified by:
// Created:     09.08.98
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //////

#ifndef   _ADBBOOK_H
#define   _ADBBOOK_H

#include "adb/AdbEntry.h"    // the base class declaration

/**
  An address book contains entries and groups of entries. There are two sorts
  of address book: local and others. We assume that the entries of the local
  books can be enumerated quickly and that there are relatively few of them, so
  we show the local address books in the wxAdbEditFrame. The others (non local)
  might represent, for example, global Internet directory services.

  AdbBook is an interface class which doesn't provide any mean to create the
  books: this is done with AdbManager which in turn uses (one of)
  AdbDataProviders to actually create the book.

  This class derives from MObjectRC and uses reference counting, see
  the comments in MObject.h for more details about it.
*/

class AdbBook : public AdbEntryGroupCommon
{
public:
  // accessors
    // compare this book with the book named 'name' and return TRUE if it's the
    // same (dumb name comparaison doesn't work for file based books)
  virtual bool IsSameAs(const String& name) const = 0;
    // get the "file name" (which uniquely identifies the book), return an
    // empty string for the non file based books
  virtual String GetFileName() const = 0;

    // get/set ADB name (shown in the ADB tree)
  virtual void SetName(const String& desc) = 0;
  virtual String GetName() const = 0;

    // get/set the book description (shown in the ADB properties dialog)
  virtual void SetDescription(const String& desc) = 0;
  virtual String GetDescription() const = 0;

    // get the total number of enters (for information purposes only, may be
    // not supported, especially for non local books)
  virtual size_t GetNumberOfEntries() const = 0;

    // is this address book local (see comments before the class declaration)?
  virtual bool IsLocal() const = 0;

    // can we edit it?
  virtual bool IsReadOnly() const = 0;

  // operations
    // flush changes to the persistent storage
  virtual bool Flush() { return true; }
};

#endif  //_ADBBOOK_H
