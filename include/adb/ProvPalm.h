// -*- c++ -*-/// //// //// //// //// //// //// //// //// //// //// //// //////
// Project:     M - cross platform e-mail GUI client
// File name:   adb/ProvPalm.h 
// Purpose:     Palm-ADBProvider defines an interface for the Palm ADB
// Author:      Daniel Seifert
// Modified by:
// Created:     18.02.2000
// CVS-ID:      $Id: 
// Copyright:   (c) 2000 Daniel Seifert <dseifert@gmx.de>
// Licence:     M license
// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //////

#ifndef   _PROVPALM_H
#define   _PROVPALM_H

// M
#include "Mpch.h"

#ifdef USE_PISOCK

#ifndef  USE_PCH
#  include "Mcommon.h"
#  include "guidef.h"
#  include "strutil.h"
#  include "sysutil.h"
#  include "MApplication.h"
#  include <ctype.h>
#endif //USE_PCH

#include "adb/AdbManager.h"
#include "adb/AdbEntry.h"
#include "adb/AdbBook.h"
#include "adb/AdbDataProvider.h"

#include <pi-address.h>
#include "MModule.h"
#include "kbList.h"

// fwd decl
class PalmEntry;
class PalmEntryGroup;
class PalmGroupList;
class PalmBook;
class PalmDataProvider;

// our AdbEntryData implementation
class PalmEntry : public AdbEntryStoredInMemory
{
public:
  // ctor
  PalmEntry(PalmEntryGroup *pGroup, const String& strName, bool bNew = FALSE);
  virtual ~PalmEntry(void);

  // implement interface methods
    // AdbEntry
  virtual AdbEntryGroup *GetGroup() const;

  void Load(struct Address a);
  bool Save();

  // an easier to use GetName()
  const wxChar *GetName() const { return m_astrFields[0]; }

  // if it's not, we will be deleted, so it really must be something fatal
  bool IsOk() const { return m_pGroup != NULL; }
    /// is the entry read-only?
  virtual bool IsReadOnly() const;

  MOBJECT_DEBUG(PalmEntry)

private:
  PalmEntryGroup *m_pGroup;     // the group which contains us (never NULL)

  GCC_DTOR_WARN_OFF
};

KBLIST_DEFINE(PalmEntryList, PalmEntry);

// our AdbEntryGroup implementation
class PalmEntryGroup : public AdbEntryGroup
{
public:
  // ctors
    // the normal one
  PalmEntryGroup(PalmEntryGroup *pParent, const wxString& strName,
               bool bNew = FALSE);
    // this one is only used for the root group
  PalmEntryGroup(void);
  virtual ~PalmEntryGroup();

  // implement interface methods
    // AdbEntryGroup
  virtual AdbEntryGroup *GetGroup() const { return m_pParent; }
  virtual String GetName() const { return m_strName; }

  virtual size_t GetEntryNames(wxArrayString& aNames) const;
  virtual size_t GetGroupNames(wxArrayString& aNames) const;

  virtual AdbEntry *GetEntry(const String& name) const;
  virtual AdbEntryGroup *GetGroup(const String& name) const;

  virtual bool Exists(const String& path) const;

  virtual AdbEntry *CreateEntry(const String& strName);
  virtual void AddEntry(PalmEntry* p_Entry);
  virtual AdbEntryGroup *CreateGroup(const String& strName);

  virtual void DeleteEntry(const String& strName);
  virtual void DeleteGroup(const String& strName);

  virtual AdbEntry *FindEntry(const wxChar *szName) const;
  virtual bool IsReadOnly() const
     {
        /*ASSERT(m_pParent); return m_pParent->IsReadOnly();*/
#ifdef EXPERIMENTAL
        return FALSE;
#else
        return TRUE;
#endif
     }

  MOBJECT_DEBUG(PalmEntry)

private:
  wxString        m_strName;      // our name
  PalmEntryGroup *m_pParent;      // the parent group (never NULL)
  PalmEntryList  *m_entries;      // a list of all entries
  PalmGroupList  *m_groups;       // a list of the subgroups
  
  GCC_DTOR_WARN_OFF
};

inline  bool
PalmEntry::IsReadOnly() const
{
   ASSERT(m_pGroup);
   return m_pGroup->IsReadOnly();
}


KBLIST_DEFINE(PalmGroupList, PalmEntryGroup);

// our AdbBook implementation
class PalmBook : public AdbBook
{
public:
  PalmBook(const String& filename);

  // implement interface methods
    // AdbElement
  virtual AdbEntryGroup *GetGroup() const { return m_pRootGroup; }

    // AdbEntryGroup
  virtual AdbEntry *GetEntry(const String& name) const
    { return m_pRootGroup->GetEntry(name); }

  virtual bool Exists(const String& path) const
    { return m_pRootGroup->Exists(path); }

  virtual size_t GetEntryNames(wxArrayString& aNames) const
    { return m_pRootGroup->GetEntryNames(aNames); }
  virtual size_t GetGroupNames(wxArrayString& aNames) const
    { return m_pRootGroup->GetGroupNames(aNames); }

  virtual AdbEntryGroup *GetGroup(const String& name) const
    { return m_pRootGroup->GetGroup(name);}

  virtual AdbEntry *CreateEntry(const String& strName)
    { return m_pRootGroup->CreateEntry(strName); }
  virtual AdbEntryGroup *CreateGroup(const String& strName)
    { return m_pRootGroup->CreateGroup(strName); }

  virtual void DeleteEntry(const String& strName)
    { m_pRootGroup->DeleteEntry(strName); }
  virtual void DeleteGroup(const String& strName)
    { m_pRootGroup->DeleteGroup(strName); }

  virtual AdbEntry *FindEntry(const wxChar *szName) const
    { return m_pRootGroup->FindEntry(szName); }

  // there can be only one PalmADB
  virtual bool IsSameAs(const String& name) const
    { return TRUE; }
  virtual String GetFileName() const;

  virtual void SetName(const String& name);
  virtual String GetName() const;

  virtual void SetDescription(const String& desc);
  virtual String GetDescription() const;

  virtual size_t GetNumberOfEntries() const;

  virtual bool IsLocal() const { return TRUE; }
  virtual bool IsReadOnly() const;

  virtual bool Flush();

  /** Return the icon name if set. The numeric return value must be -1 
      for the default, or an index into the image list in AdbFrame.cpp.
  */
  virtual int GetIconId() const { return 5; }
  MOBJECT_DEBUG(PalmBook)

private:
  virtual ~PalmBook();

  wxString m_strName,
           m_strUserName,
           m_strDescription;
           
  PalmEntryGroup *m_pRootGroup; // the ADB_Entries group
  GCC_DTOR_WARN_OFF
};

// our AdbDataProvider implementation
class PalmDataProvider : public AdbDataProvider
{
public:
  // implement interface methods
  virtual AdbBook *CreateBook(const String& name);
  virtual bool EnumBooks(wxArrayString& aNames);
  virtual bool DeleteBook(AdbBook *book);
  virtual bool TestBookAccess(const String& name, AdbTests test);

  MOBJECT_DEBUG(PalmDataProvider)

  DECLARE_ADB_PROVIDER(PalmDataProvider);
};

#endif // USE_PISOCK

#endif  //_PROVPALM_H
