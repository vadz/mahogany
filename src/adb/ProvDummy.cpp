///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   adb/ProvDummy.cpp - dummy AdbDataProvider
// Purpose:     this is a dummy provider implementation for testing only
// Author:      Vadim Zeitlin
// Modified by:
// Created:     09.08.98
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

/*
  This module always returns the same data
*/

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// M
#include "Mpch.h"

#ifndef  USE_PCH
#  include "Mcommon.h"

#  include <wx/string.h>                // for wxString
#endif //USE_PCH

#include "adb/AdbManager.h"
#include "adb/AdbEntry.h"
#include "adb/AdbBook.h"
#include "adb/AdbDataProvider.h"

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// fwd decl
class DummyEntry;
class DummyEntryGroup;
class DummyBook;
class DummyDataProvider;

// our AdbEntryData implementation
class DummyEntry : public AdbEntry
{
public:
  // ctor
  DummyEntry(DummyEntryGroup *pGroup, const String& strName);

  // implement interface methods
    // AdbEntry
  virtual AdbEntryGroup *GetGroup() const;

  virtual void GetFieldInternal(size_t n, String *pstr) const
    { GetField(n, pstr); }
  virtual void GetField(size_t n, String *pstr) const;

  virtual size_t GetEMailCount() const           { return m_astrEmails.Count(); }
  virtual void GetEMail(size_t n, String *pstr) const { *pstr = m_astrEmails[n]; }

  virtual void ClearDirty()    { m_bDirty = FALSE; }
  virtual bool IsDirty() const { return m_bDirty; }

  virtual void SetField(size_t n, const String& strValue);
  virtual void AddEMail(const String& strEMail)
    { m_astrEmails.Add(strEMail); m_bDirty = TRUE; }
  virtual void ClearExtraEMails();

  virtual int Matches(const wxChar *str, int where, int how) const;

  // an easier to use GetName()
  const wxChar *GetName() const { return m_astrFields[0]; }

  // get path to our entry in config file
  wxString GetPath() const;

private:
  wxArrayString m_astrFields; // all text entries (some may be not present)
  wxArrayString m_astrEmails; // all email addresses except for the first one

  bool m_bDirty;              // dirty flag

  DummyEntryGroup *m_pGroup;     // the group which contains us (NULL for root)
};

// our AdbEntryGroup implementation
class DummyEntryGroup : public AdbEntryGroupCommon
{
public:
  // ctors
    // the normal one
  DummyEntryGroup(DummyEntryGroup *pParent, const wxString& strName);

  // implement interface methods
    // AdbEntryGroup
  virtual AdbEntryGroup *GetGroup() const { return m_pParent; }
  virtual String GetName() const { return m_strName; }

  virtual size_t GetEntryNames(wxArrayString& aNames) const;
  virtual size_t GetGroupNames(wxArrayString& aNames) const;

  virtual AdbEntry *GetEntry(const String& name);
  virtual AdbEntryGroup *GetGroup(const String& name) const;

  virtual bool Exists(const String& path);

  virtual AdbEntry *CreateEntry(const String& strName);
  virtual AdbEntryGroup *CreateGroup(const String& strName);

  virtual void DeleteEntry(const String& strName);
  virtual void DeleteGroup(const String& strName);

  virtual AdbEntry *FindEntry(const wxChar *szName);

  // get the full path to our group (not '/' terminated)
  wxString GetPath() const;

private:
  virtual ~DummyEntryGroup();

  wxString         m_strName;      // our name
  DummyEntryGroup *m_pParent;      // the parent group (never NULL)

  GCC_DTOR_WARN_OFF
};

// our AdbBook implementation: it maps to a disk file here
class DummyBook : public AdbBook
{
public:
  DummyBook(const String& filename);

  // implement interface methods
    // AdbElement
  virtual AdbEntryGroup *GetGroup() const { return NULL; }

    // AdbEntryGroup
  virtual AdbEntry *GetEntry(const String& name)
    { return m_pRootGroup->GetEntry(name); }

  virtual bool Exists(const String& path)
    { return m_pRootGroup->Exists(path); }

  virtual size_t GetEntryNames(wxArrayString& aNames) const
    { return m_pRootGroup->GetEntryNames(aNames); }
  virtual size_t GetGroupNames(wxArrayString& aNames) const
    { return m_pRootGroup->GetGroupNames(aNames); }

  virtual AdbEntryGroup *GetGroup(const String& name) const
    { return m_pRootGroup->GetGroup(name); }

  virtual AdbEntry *CreateEntry(const String& strName)
    { return m_pRootGroup->CreateEntry(strName); }
  virtual AdbEntryGroup *CreateGroup(const String& strName)
    { return m_pRootGroup->CreateGroup(strName); }

  virtual void DeleteEntry(const String& strName)
    { m_pRootGroup->DeleteEntry(strName); }
  virtual void DeleteGroup(const String& strName)
    { m_pRootGroup->DeleteGroup(strName); }

  virtual AdbEntry *FindEntry(const wxChar *szName)
    { return m_pRootGroup->FindEntry(szName); }

    // AdbBook
  virtual bool IsSameAs(const String& name) const;
  virtual String GetFileName() const;

  virtual void SetName(const String& name);
  virtual String GetName() const;

  virtual void SetDescription(const String& desc);
  virtual String GetDescription() const;

  virtual size_t GetNumberOfEntries() const;

  virtual bool IsLocal() const { return TRUE; }
  virtual bool IsReadOnly() const;

private:
  virtual ~DummyBook();

  wxString m_strName,
           m_strDesc;

  DummyEntryGroup *m_pRootGroup; // the ADB_Entries group

  GCC_DTOR_WARN_OFF
};

// our AdbDataProvider implementation
class DummyDataProvider : public AdbDataProvider
{
public:
  // implement interface methods
  virtual AdbBook *CreateBook(const String& name);
  virtual bool EnumBooks(wxArrayString& aNames);
  virtual bool DeleteBook(AdbBook *book);
  virtual bool TestBookAccess(const String& name, AdbTests test);

  virtual bool HasField(AdbField /* field */) const { return true; }
  virtual bool HasMultipleEMails() const { return true; }

  DECLARE_ADB_PROVIDER(DummyDataProvider);
};

IMPLEMENT_ADB_PROVIDER(DummyDataProvider, TRUE, "Dummy", Name_String);

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// DummyEntry
// ----------------------------------------------------------------------------

DummyEntry::DummyEntry(DummyEntryGroup *pGroup, const String& strName)
{
  m_pGroup = pGroup;
  m_astrFields.Add(strName);

  SetField(AdbField_FirstName, _T("Dummy"));
  SetField(AdbField_FamilyName, _T("entry"));
  SetField(AdbField_Comments, _T("some\ndummy\ncomments"));
  SetField(AdbField_EMail, _T("email@nowhere"));

  m_bDirty = FALSE;
}

AdbEntryGroup *DummyEntry::GetGroup() const
{
  return m_pGroup;
}

// we store only the fields which were non-empty, so check the index
void DummyEntry::GetField(size_t n, String *pstr) const
{
  if ( n < m_astrFields.Count() )
    *pstr = m_astrFields[n];
  else
    pstr->Empty();
}

// the problem here is that we may have only several first strings in the
// m_astrFields array, so we need to add some before setting n-th field
void DummyEntry::SetField(size_t n, const wxString& strValue)
{
  size_t nCur = m_astrFields.Count();
  // add some empty fields if needed
  for ( int nAdd = 0; nAdd < (int)(n - nCur + 1); nAdd++ )
    m_astrFields.Add(wxGetEmptyString());

  if ( m_astrFields[n] != strValue ) {
    m_astrFields[n] = strValue;
    m_bDirty = TRUE;
  }
}

void DummyEntry::ClearExtraEMails()
{
}

int DummyEntry::Matches(const wxChar * /* what */,
                        int /* where */,
                        int /* how */) const
{
  return 0;
}

// ----------------------------------------------------------------------------
// DummyEntryGroup
// ----------------------------------------------------------------------------

DummyEntryGroup::DummyEntryGroup(DummyEntryGroup *, const String& strName)
               : m_strName(strName)
{
}

DummyEntryGroup::~DummyEntryGroup()
{
}

size_t DummyEntryGroup::GetEntryNames(wxArrayString& aNames) const
{
  aNames.Empty();
  aNames.Add(_T("Dummy entry"));

  return aNames.Count();
}

size_t DummyEntryGroup::GetGroupNames(wxArrayString& aNames) const
{
  aNames.Empty();

  return aNames.Count();
}

AdbEntry *DummyEntryGroup::GetEntry(const String& /* name */)
{
  return new DummyEntry((DummyEntryGroup *)this, _T("Dummy entry"));
}

bool DummyEntryGroup::Exists(const String& path)
{
  return path == _T("Dummy entry");
}

AdbEntryGroup *DummyEntryGroup::GetGroup(const String& /* name */) const
{
  return NULL;
}

AdbEntry *DummyEntryGroup::CreateEntry(const String& strName)
{
  return GetEntry(strName);
}

AdbEntryGroup *DummyEntryGroup::CreateGroup(const String& strName)
{
  return GetGroup(strName);
}

void DummyEntryGroup::DeleteEntry(const String& /* strName */)
{
  wxFAIL_MSG(_T("Not implemented"));
}

void DummyEntryGroup::DeleteGroup(const String& /* strName */)
{
  wxFAIL_MSG(_T("Not implemented"));
}

AdbEntry *DummyEntryGroup::FindEntry(const wxChar * /* szName */)
{
  return NULL;
}

// ----------------------------------------------------------------------------
// DummyBook
// ----------------------------------------------------------------------------

DummyBook::DummyBook(const String& name)
         : m_strName(name), m_strDesc(name)
{
  // create the root group
  m_pRootGroup = new DummyEntryGroup(NULL, _T("Dummy group"));
}

DummyBook::~DummyBook()
{
  SafeDecRef(m_pRootGroup);
}

bool DummyBook::IsSameAs(const String& name) const
{
   return m_strName == name;
}

String DummyBook::GetFileName() const
{
  return m_strName;
}

void DummyBook::SetName(const String& strName)
{
  m_strName = strName;
}

String DummyBook::GetName() const
{
  return m_strName.c_str();
}

void DummyBook::SetDescription(const String& strDesc)
{
  m_strDesc = strDesc;
}

String DummyBook::GetDescription() const
{
  return m_strDesc.c_str();
}

size_t DummyBook::GetNumberOfEntries() const
{
  return 1;
}

bool DummyBook::IsReadOnly() const
{
  return TRUE;
}

// ----------------------------------------------------------------------------
// DummyDataProvider
// ----------------------------------------------------------------------------

AdbBook *DummyDataProvider::CreateBook(const String& name)
{
  return new DummyBook(name);
}

bool DummyDataProvider::EnumBooks(wxArrayString& /* aNames */)
{
  return FALSE;
}

#ifdef EXPERIMENTAL_adbtest

bool DummyDataProvider::TestBookAccess(const String& name, AdbTests test)
{
  String str;
  str.Printf("Return TRUE from DummyDataProvider::TestBookAccess(%d) "
             " for '%s'?",
             test, name.c_str());
  return MDialog_YesNoDialog(str);
}

#else // !EXPERIMENTAL_adbtest

bool
DummyDataProvider::TestBookAccess(const String& /* name*/, AdbTests /* test */)
{
  return FALSE;
}

#endif // EXPERIMENTAL_adbtest/!EXPERIMENTAL_adbtest

bool DummyDataProvider::DeleteBook(AdbBook * /* book */)
{
  return FALSE;
}
