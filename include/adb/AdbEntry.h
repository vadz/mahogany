///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   adb/AdbEntry.h - ADB data record interface
// Purpose:     
// Author:      Vadim Zeitlin
// Modified by: 
// Created:     09.07.98
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef   _ADBENTRY_H
#define   _ADBENTRY_H

#include "MObject.h"    // the base class declaration

// forward declaration for classes we use
class AdbElement;
class AdbEntry;
class AdbEntryGroup;

class wxArrayString;

// ============================================================================
// ADB constants
// ============================================================================

// all fields
// NB: these constants must be kept in sync with AdbEntry::ms_aFields
enum AdbField
{
  AdbField_NamePageFirst,                     // NB: must be 0!
  AdbField_NickName = AdbField_NamePageFirst, // NB: and this one also
  AdbField_FullName,                          //
  AdbField_FirstName,                         //
  AdbField_FamilyName,                        //
  AdbField_Prefix,                            // M., Mme, ...
  AdbField_Title,                             //
  AdbField_Organization,                      //
  AdbField_Birthday,                          // date
  AdbField_Comments,                          // arbitrary (multi line) text
  AdbField_NamePageLast,                      //

  AdbField_EMailPageFirst = AdbField_NamePageLast,
  AdbField_EMail = AdbField_EMailPageFirst,   // main e-mail address
  AdbField_HomePage,                          // WWW
  AdbField_ICQ,                               // ICQ number
  AdbField_PrefersHTML,                       // if not, send text only
  AdbField_OtherEMails,                       // additional e-mail addresses
  AdbField_EMailPageLast,

  // 2 copies of address fields: for home (_H_) and for the office (_O_)
  AdbField_H_AddrPageFirst = AdbField_EMailPageLast,
  AdbField_H_StreetNo = AdbField_H_AddrPageFirst,
  AdbField_H_Street,
  AdbField_H_Locality,
  AdbField_H_City,
  AdbField_H_Postcode,
  AdbField_H_Country,
  AdbField_H_POBox,                       // @@@ Karsten, what this one is?
  AdbField_H_Phone,
  AdbField_H_Fax,
  AdbField_H_AddrPageLast,

  AdbField_O_AddrPageFirst = AdbField_H_AddrPageLast,
  AdbField_O_StreetNo = AdbField_O_AddrPageFirst,
  AdbField_O_Street,
  AdbField_O_Locality,
  AdbField_O_City,
  AdbField_O_Postcode,
  AdbField_O_Country,
  AdbField_O_POBox,
  AdbField_O_Phone,
  AdbField_O_Fax,
  AdbField_O_AddrPageLast,

  AdbField_Max = AdbField_O_AddrPageLast
};

// where AdbEntry::Matches() should look for a match
enum
{
  AdbLookup_NickName      = 0x0001,
  AdbLookup_FullName      = 0x0002,
  AdbLookup_Organization  = 0x0004,
  AdbLookup_EMail         = 0x0008,
  AdbLookup_HomePage      = 0x0010,
  AdbLookup_Everywhere    = 0xffff
};

// how should it do it
enum
{
  AdbLookup_CaseSensitive = 0x0001,
  AdbLookup_Substring     = 0x0002 // match "foo" as "*foo*"
};

// ============================================================================
// ADB classes
// ============================================================================

/**
  The common base class for AdbEntry and AdbEntryGroup.

  As this class derives from MObject, both AdbEntry and AdbEntryGroup.do too,
  so they use reference counting: see the comments in MObject.h for more
  details about it.
*/
class AdbElement : public MObject
{
  /// the group this entry/group belongs to (never NULL for these classes)
  virtual AdbEntryGroup *GetGroup() const = 0;
};

/**
  Data stored for each entry in the ADB. It can be read an modified, in the
  latter case the entry is responsible for saving it (i.e. there is no separate
  Save() function).
*/
class AdbEntry : public AdbElement
{
public:
  // accessors
    /// retrieve the value of a field (see enum AdbField for index values)
  virtual void GetField(uint n, String *pstr) const = 0;
    /// get the count of additional e-mail addresses (i.e. except the 1st one)
  virtual uint GetEMailCount() const = 0;
    /// get an additional e-mail adderss
  virtual void GetEMail(uint n, String *pstr) const = 0;

  // changing the data
    /// set any text field with this function
  virtual void SetField(uint n, const String& strValue) = 0;
    /// add an additional e-mail (primary one is changed with SetField)
  virtual void AddEMail(const String& strEMail) = 0;
    /// delete all additional e-mails
  virtual void ClearExtraEMails() = 0;

  // if dirty flag is set, the entry will automatically save itself when
  // deleted (the flag is set automatically when the entry is modified)
    /// has the entry been changed?
  virtual bool IsDirty() const = 0;
    /// prevent the entry from saving itself by resetting the dirty flag
  virtual void ClearDirty() = 0;

  // other operations
    /// check whether we match the given string (see AdbLookup_xxx constants)
  virtual bool Matches(const char *str, int where, int how) = 0;
};

/**
  A group of ADB entries which contains the entries and other groups.

  This class derives from MObject and uses reference counting, see the comments
  in MObject.h for more details about it.
*/
class AdbEntryGroup : public AdbElement
{
public:
  // accessors
    /// get the names of all entries, returns the number of them
  virtual uint GetEntryNames(wxArrayString& aNames) const = 0;

    /// get entry by name
  virtual AdbEntry *GetEntry(const String& name) const = 0;

    /// check whether an entry or group by this name exists
  virtual bool Exists(const String& path) const = 0;

    /// get the names of all groups, returns the number of them
  virtual uint GetGroupNames(wxArrayString& aNames) const = 0;

    /// get the name of the group/group by name
  virtual AdbEntryGroup *GetGroup(const String& name) const = 0;

  // operations
    /// create entry/subgroup
  virtual AdbEntry *CreateEntry(const String& strName) = 0;
  virtual AdbEntryGroup *CreateGroup(const String& strName) = 0;

    /// delete entry/subgroup
  virtual void DeleteEntry(const String& strName) = 0;
  virtual void DeleteGroup(const String& strName) = 0;

    /// find entry by name (returns NULL if not found)
  virtual AdbEntry *FindEntry(const char *szName) = 0;
};

// ----------------------------------------------------------------------------
// AdbDataArray: array of AdbEntry
// ----------------------------------------------------------------------------
WX_DEFINE_ARRAY(AdbEntry *, AdbDataArrayBase);

class AdbDataArray : public AdbDataArrayBase
{
public:
  // unlock all elements in the array
  void UnlockAll() { for ( uint n = 0; n < Count(); n++ ) Item(n)->Unlock(); }

  // warning: base class dtor is not virtual, so the dtor only will be called
  // for non-polymorphic pointers!
  ~AdbDataArray() { UnlockAll(); }
};


#endif  //_ADBENTRY_H
