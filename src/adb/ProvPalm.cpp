///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   adb/ProvPalm.cpp - Palm based AdbDataProvider
// Purpose:     simple implementations of AdbBook and AdbDataProvider
// Author:      Daniel Seifert
// Modified by:
// Created:     15.02.00
// CVS-ID:      
// Copyright:   (c) 2000 Daniel Seifert <dseifert@gmx.de>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

/*
  This is the AdbDataProvider for the PalmOS Addressbook. 
*/

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#if EXPERIMENTAL

// M
#include "Mpch.h"

#ifndef  USE_PCH
#  include "Mcommon.h"
#  include "guidef.h"
#  include "strutil.h"
#  include "sysutil.h"
#  include "MApplication.h"
#  include <ctype.h>
#endif //USE_PCH

// wxWindows
#include <wx/string.h>
#include <wx/log.h>
#include <wx/intl.h>
#include <wx/dynarray.h>
#include <wx/file.h>
#include <wx/textfile.h>
#include <wx/confbase.h>
#include <wx/fileconf.h>

#include "adb/AdbManager.h"
#include "adb/AdbEntry.h"
#include "adb/AdbBook.h"
#include "adb/AdbDataProvider.h"

#include <pi-address.h>
#include "MModule.h"
#include "kbList.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

#define   ADB_HEADER        "ADB_Header"
#define   ADB_HEADER_NAME   "Name"
#define   ADB_HEADER_DESC   "Description"

#define   ADB_ENTRIES       "ADB_Entries"

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// fwd decl
class PalmEntry;
class PalmEntryGroup;
class PalmBook;
class PalmDataProvider;

class PalmOSModule : public MModule
{
public:
   void Synchronise(PalmBook *p_Book);
};

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
  const char *GetName() const { return m_astrFields[0]; }

  // if it's not, we will be deleted, so it really must be something fatal
  bool IsOk() const { return m_pGroup != NULL; }

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

  virtual AdbEntry *FindEntry(const char *szName);

  MOBJECT_DEBUG(PalmEntry)

private:
  virtual ~PalmEntryGroup();

  wxString        m_strName;      // our name
  PalmEntryGroup *m_pParent;      // the parent group (never NULL)
  PalmEntryList  *m_entries;      // a list of all entries
  GCC_DTOR_WARN_OFF
};

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

  virtual AdbEntry *FindEntry(const char *szName)
    { return m_pRootGroup->FindEntry(szName); }

  // there can be only one PalmADB
  virtual bool IsSameAs(const String& name) const
    { return TRUE; }
  virtual String GetName() const;

  virtual void SetUserName(const String& name);
  virtual String GetUserName() const;

  virtual void SetDescription(const String& desc);
  virtual String GetDescription() const;

  virtual size_t GetNumberOfEntries() const;

  virtual bool IsLocal() const { return TRUE; }
  virtual bool IsReadOnly() const;

  virtual bool Flush();

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

IMPLEMENT_ADB_PROVIDER(PalmDataProvider, TRUE, "PalmOS-ADB (Broken! Do not use!)", Name_No);

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// PalmEntry
// ----------------------------------------------------------------------------

PalmEntry::PalmEntry(PalmEntryGroup *pGroup, const String& strName, bool bNew = false)
{
  m_pGroup = pGroup;
  m_astrFields.Add(strName);

  if (m_pGroup)
    m_pGroup->IncRef();

  m_bDirty = FALSE;
}

PalmEntry::~PalmEntry()
{
  if ( m_bDirty )
    Save();
  if (m_pGroup) m_pGroup->DecRef();
}
AdbEntryGroup *PalmEntry::GetGroup() const
{
  return m_pGroup;
}

// 
void PalmEntry::Load(struct Address a)
{
  wxASSERT( AdbField_NickName == 0 ); // must be always the first one

  int i = 0;
  String alias = "";

  while (alias == "") {
    alias = a.entry[i++];
  }

  m_astrFields.Add(alias);
  SetField(AdbField_FirstName, a.entry[0]);
  SetField(AdbField_FamilyName, a.entry[1]);
  SetField(AdbField_Title, a.entry[2]);
  SetField(AdbField_Organization, a.entry[3]);
// usw    Setfield(AdbField_

  m_bDirty = FALSE;
}

// save entry to wxFileConfig (doesn't check if it's modified or not)
bool PalmEntry::Save()
{
  wxCHECK_MSG( m_bDirty, TRUE, "shouldn't save unmodified PalmEntry" );

  // TODO: save changed entries to Palm
  return FALSE;
}

// ----------------------------------------------------------------------------
// PalmEntryGroup (maps to a category in the Palm ADB)
// ----------------------------------------------------------------------------

PalmEntryGroup::PalmEntryGroup(void)
{
  m_pParent = NULL;
  m_strName = "Palm ADB";
  m_entries = new PalmEntryList(false);
}

PalmEntryGroup::PalmEntryGroup(PalmEntryGroup *pParent,
                               const wxString& strName,
                               bool bNew)
               : m_strName(strName)
{
  m_pParent = pParent;
  m_strName = *strName;
  m_entries = new PalmEntryList(false);
   
  if ( bNew ) {
    // force creation of the group
  }
}

PalmEntryGroup::~PalmEntryGroup()
{
}

size_t PalmEntryGroup::GetEntryNames(wxArrayString& aNames) const
{
  aNames.Empty();
  PalmEntryList::iterator i;
  for(i = m_entries->begin(); i != m_entries->end(); i++)
     aNames.Add((**i).GetName());
  return aNames.Count();
}

size_t PalmEntryGroup::GetGroupNames(wxArrayString& aNames) const
{
  aNames.Empty();
  return aNames.Count();
}

AdbEntry *PalmEntryGroup::GetEntry(const String& name) const
{
  PalmEntryList::iterator i;
  for(i = m_entries->begin(); i != m_entries->end(); i++)
  {
    if((**i).GetName() == name)
    {
      (**i).IncRef();
      return *i;
    }
  }
  return NULL;
} 

bool PalmEntryGroup::Exists(const String& path) const
{
   // TODO
   return FALSE;
}

AdbEntryGroup *PalmEntryGroup::GetGroup(const String& name) const
{
  PalmEntryGroup *pGroup = new PalmEntryGroup((PalmEntryGroup *)this, name);
  if (pGroup != NULL) {
    pGroup->DecRef();
    pGroup = NULL;
  }

  return pGroup;
}

void PalmEntryGroup::AddEntry(PalmEntry* p_Entry) 
{
  ASSERT_MSG (!!m_entries, "AddEntry: non-initialized m_entries" );
  ASSERT_MSG (!!p_Entry, "AddEntry: non-initialized p_Entry" );
//  m_entries->push_back(p_Entry);
}

AdbEntry *PalmEntryGroup::CreateEntry(const String& name)
{
  CHECK( !!name, NULL, "can't create entries with empty names" );

  PalmEntry *pEntry = new PalmEntry((PalmEntryGroup *)this, name, TRUE /* new */);
  if ( !pEntry->IsOk() ) {
    pEntry->DecRef();
    pEntry = NULL;
  }

  return pEntry;
}

AdbEntryGroup *PalmEntryGroup::CreateGroup(const String& name)
{
  // there must be only one level of entries!
//  if (m_pRoot == (PalmEntryGroup *)this)
    return NULL;
    
  // creating new groups (e.g. categories on the Palm) is 
  // currently unsupported
  // FIXME???
  return NULL;
}

void PalmEntryGroup::DeleteEntry(const String& strName)
{
  // deleting entries (on the Palm) is currently unsupported
  return;
}

void PalmEntryGroup::DeleteGroup(const String& strName)
{
  // deleting groups (== categories on the Palm) is currently unsupported
}

AdbEntry *PalmEntryGroup::FindEntry(const char * /* szName */)
{
  // currently not supported
  return NULL;
}

// ----------------------------------------------------------------------------
// PalmBook
// ----------------------------------------------------------------------------

// Ctor
PalmBook::PalmBook(const String& strName)
{
  // create the root group, in our case this is the only group
  // allowed to contain subgroups!
  m_pRootGroup = new PalmEntryGroup();

  m_strName = strName;

  // read addresses and store to appropriate entries
  // TODO
}

PalmBook::~PalmBook()
{
  SafeDecRef(m_pRootGroup);
}

String PalmBook::GetName() const
{
  return m_strName;
}

void PalmBook::SetUserName(const String& strUserName)
{
  m_strUserName = strUserName;
}

String PalmBook::GetUserName() const
{
  return m_strUserName;
}

void PalmBook::SetDescription(const String& strDesc)
{
  m_strDescription = strDesc;
}

String PalmBook::GetDescription() const
{
  return m_strDescription;
}

size_t PalmBook::GetNumberOfEntries() const
{
  // there can be only one Palm-ADB
  return 1;
}

bool PalmBook::IsReadOnly() const
{
  // writing to the Palm is currently unsupported
  return true;
}

bool PalmBook::Flush()
{
   // I don´t have the slightest idea what this method should do
   return true;
}

// ----------------------------------------------------------------------------
// PalmDataProvider
// ----------------------------------------------------------------------------

AdbBook *PalmDataProvider::CreateBook(const String& name)
{
  PalmBook *p_Book = new PalmBook(name);

  MModule *palmModule = MModule::GetProvider("HandheldSynchronise");
  if(palmModule)
  {
     // calling MMOD_FUNC_USER function in PalmOSModule synchronises
     // the addressbook:
    palmModule->Entry(MMOD_FUNC_USER, p_Book);
    palmModule->DecRef();
    return p_Book;
  }
  else
    return NULL;
}

bool PalmDataProvider::EnumBooks(wxArrayString& /* aNames */)
{
  return FALSE;
}

bool PalmDataProvider::TestBookAccess(const String& name, AdbTests test)
{
  // TODO: Test, whether the PalmOS module is available
  return FALSE;

  switch ( test )
  {
    case Test_Open:
    case Test_OpenReadOnly:
    case Test_Create:
    default:
      FAIL_MSG("invalid test in TestBookAccess");
  }
}

bool PalmDataProvider::DeleteBook(AdbBook * /* book */)
{
  // Palm-ADB is currently only available in memory
  // TODO: Delete all entries in memory
  return FALSE;
}

// ----------------------------------------------------------------------------
// debugging support
// ----------------------------------------------------------------------------

#ifdef DEBUG

String PalmEntry::DebugDump() const
{
  String str = MObjectRC::DebugDump();
  str << "name = '" << GetName() << '\'';

  return str;
}

String PalmEntryGroup::DebugDump() const
{
  String str = MObjectRC::DebugDump();
//  str << "path = '" << GetPath() << '\'';

  return str;
}

String PalmBook::DebugDump() const
{
  String str = MObjectRC::DebugDump();
//  str << "file = '" << m_strFile << '\'';

  return str;
}

String PalmDataProvider::DebugDump() const
{
  String str = MObjectRC::DebugDump();

  return str;
}

#endif // DEBUG

#endif // EXPERIMENTAL
