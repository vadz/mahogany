///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   adb/ProvPalm.cpp - Palm based AdbDataProvider
// Purpose:     implementation of PalmAdbBook and PalmAdbDataProvider
// Author:      Daniel Seifert
// Modified by:
// Created:     15.02.00
// CVS-ID:      $Id$
// Copyright:   (c) 2000 Daniel Seifert <dseifert@gmx.de>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

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
#include "adb/ProvPalm.h"   // class declarations

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

#define   ADB_HEADER        "ADB_Header"
#define   ADB_HEADER_NAME   "Name"
#define   ADB_HEADER_DESC   "Description"
#define   ADB_ENTRIES       "ADB_Entries"

IMPLEMENT_ADB_PROVIDER(PalmDataProvider, TRUE, "PalmOS-ADB (ReadOnly)", Name_No);

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

/*
  if (m_pGroup)
    m_pGroup->IncRef();
*/
  m_bDirty = FALSE;
}

PalmEntry::~PalmEntry()
{
  if ( m_bDirty )
    Save();
//  if (m_pGroup) m_pGroup->DecRef();
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
  SetField(AdbField_FamilyName, a.entry[0]);
  SetField(AdbField_FirstName, a.entry[1]);
  SetField(AdbField_Organization, a.entry[2]);
  SetField(AdbField_O_Phone, a.entry[3]);
  SetField(AdbField_H_Phone, a.entry[4]);
  SetField(AdbField_H_Fax, a.entry[5]);
  SetField(AdbField_EMail, a.entry[7]);
  SetField(AdbField_H_Street, a.entry[8]);
  SetField(AdbField_H_City, a.entry[9]);

  // Locality == state?
  SetField(AdbField_H_Locality, a.entry[10]);
  SetField(AdbField_H_Postcode, a.entry[11]);
  SetField(AdbField_H_Country, a.entry[12]);
  SetField(AdbField_Title, a.entry[13]);
  SetField(AdbField_Comments, a.entry[18]);
  
  // where to write "other number"?
  // SetField(???, a.entry[6]);

  // write "user defined 1-4" to comments!
  
  m_bDirty = FALSE;
}

bool PalmEntry::Save()
{
  // TODO: save changed entries to Palm
  return FALSE;
}

// ----------------------------------------------------------------------------
// PalmEntryGroup (maps to a category in the Palm ADB)
// ----------------------------------------------------------------------------

PalmEntryGroup::PalmEntryGroup(void)
{
   m_strName = "Palm ADB";
   m_entries = new PalmEntryList(false);
   m_groups  = new PalmGroupList(false);
}

PalmEntryGroup::PalmEntryGroup(PalmEntryGroup *pParent,
                               const wxString& strName,
                               bool bNew)
               : m_strName(strName)
{
  m_pParent = pParent;
  m_strName = strName;
  m_entries = new PalmEntryList(false);
  m_groups  = new PalmGroupList(false);

  if ( bNew ) {
    // ??
    // force creation of the group
  }
}

PalmEntryGroup::~PalmEntryGroup()
{
   PalmEntryList::iterator i;
   for(i = m_entries->begin(); i != m_entries->end(); i++)
      (**i).DecRef();
   
   PalmGroupList::iterator j;
   PalmEntryGroup *group = NULL;
   for(j = m_groups->begin(); j != m_groups->end(); j++)
   {
      group = *j;
      group->DecRef();
   }
   delete m_entries;
   delete m_groups;
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
  PalmGroupList::iterator i;
  for(i = m_groups->begin(); i != m_groups->end(); i++)
     aNames.Add((**i).GetName());
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
  PalmGroupList::iterator i;
  for(i = m_groups->begin(); i != m_groups->end(); i++)
  {
    if((**i).GetName() == name)
    {
      (**i).IncRef();
      return *i;
    }
  }
  return NULL;
}

void PalmEntryGroup::AddEntry(PalmEntry* p_Entry) 
{
  ASSERT_MSG (!!m_entries, _T("AddEntry: non-initialized m_entries") );
  ASSERT_MSG (!!p_Entry, _T("AddEntry: non-initialized p_Entry") );
  m_entries->push_back(p_Entry);
}

AdbEntry *PalmEntryGroup::CreateEntry(const String& name)
{
  CHECK( !!name, NULL, _T("can't create entries with empty names") );

  PalmEntry *pEntry = new PalmEntry((PalmEntryGroup *)this, name, TRUE /* new */);
  if ( !pEntry->IsOk() ) {
    pEntry->DecRef();
    pEntry = NULL;
  }

  return pEntry;
}

AdbEntryGroup *PalmEntryGroup::CreateGroup(const String& name)
{
  if (this->m_pParent == NULL) {
    PalmEntryGroup* p_Group = new PalmEntryGroup(this, name);
    m_groups->push_back(p_Group);
    return p_Group;
  } else
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

AdbEntry *PalmEntryGroup::FindEntry(const wxChar * /* szName */) const
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
  m_pRootGroup = new PalmEntryGroup(NULL, strName);

  SetName(_("PalmOS Addressbook"));
  SetDescription(_("PalmOS Addressbook"));
  m_strName = strName;
}

PalmBook::~PalmBook()
{
  SafeDecRef(m_pRootGroup);
}

String PalmBook::GetFileName() const
{
   return "";
}

String PalmBook::GetName() const
{
  return m_strName;
}

void PalmBook::SetName(const String& strName)
{
  m_strName = strName;
}

String PalmBook::GetName() const
{
  return m_strName;
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
   // this does nothing for us as we're read only
   return true;
}

// ----------------------------------------------------------------------------
// PalmDataProvider
// ----------------------------------------------------------------------------

#define PALMOS_ADB_NAME "PalmOS:PalmADB"
AdbBook *PalmDataProvider::CreateBook(const String& name)
{
  PalmBook *p_Book = new PalmBook(PALMOS_ADB_NAME);

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
  MModule *palmModule = MModule::GetProvider("HandheldSynchronise");
  bool rc = palmModule != NULL;
  if(! rc)
     return FALSE;
  
  palmModule->DecRef();
  rc = name == PALMOS_ADB_NAME;
  switch ( test )
  {
  case Test_Open:
  case Test_OpenReadOnly:
       return rc;
  case Test_Create:
     return FALSE;
  case Test_AutodetectCapable:
  case Test_RecognizesName:
    return true;
  default:
     FAIL_MSG(_T("invalid test in TestBookAccess"));
     return FALSE;
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

#endif // USE_PISOCK

