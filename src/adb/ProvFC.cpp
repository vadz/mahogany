///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   adb/ProvFC.cpp - wxFileConfig based AdbDataProvider
// Purpose:     simple implementations of AdbBook and AdbDataProvider
// Author:      Vadim Zeitlin
// Modified by: 
// Created:     09.08.98
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

/*
  This module works with files in wxFileConfig format. The file structure is
  as following:

  [ADB_Header]
  # it's the name which is shown in the tree
  Name = Psion Address Book
  Description = The ADB imported from Psion

  # groups are supported as in wxFileconfig
  [ADB_Entries/Friends/Old]
  # format is 'alias = colon delimited list of fields'
  # colon itself should be escaped with backslash ('\') if it appears inside
  # a field. Trailing colons can be omitted if all the trailing fields are
  # empty. Multiple e-mail addresses are comma-delimited, if a comma appears
  # inside an address, it should be also escaped with a backslash. Of course,
  # backslash itself should be escaped as well (in fact, it escapes any
  # character, not just ',' and ':')
  Mike = Full Name:First Name:Family Name:Prefix:Title:Organization:...
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
#  include "guidef.h"

#  include "strutil.h"

#  include <ctype.h>
#endif //USE_PCH

// wxWindows
#include "wx/string.h"
#include "wx/log.h"
#include "wx/intl.h"
#include "wx/dynarray.h"
#include "wx/config.h"
#include "wx/file.h"
#include "wx/textfile.h"
#include "wx/fileconf.h"


#include "adb/AdbManager.h"
#include "adb/AdbEntry.h"
#include "adb/AdbBook.h"
#include "adb/AdbDataProvider.h"


// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

#define   ADB_HEADER        "ADB_Header"
#define   ADB_HEADER_NAME   "Name"
#define   ADB_HEADER_DESC   "Description"

#define   ADB_ENTRIES       "ADB_Entries"

// ----------------------------------------------------------------------------
// private classes (FC prefix stands for wxFileConfig)
// ----------------------------------------------------------------------------

// fwd decl
class FCEntry;
class FCEntryGroup;
class FCBook;
class FCDataProvider;

// our AdbEntryData implementation
class FCEntry : public AdbEntry
{
public:
  // ctor
  FCEntry(FCEntryGroup *pGroup, const String& strName, bool bNew = FALSE);

  // implement interface methods
    // AdbEntry
  virtual AdbEntryGroup *GetGroup() const;

  virtual void GetField(size_t n, String *pstr) const;

  virtual size_t GetEMailCount() const           { return m_astrEmails.Count(); }
  virtual void GetEMail(size_t n, String *pstr) const { *pstr = m_astrEmails[n]; }

  virtual void ClearDirty()    { m_bDirty = FALSE; }   
  virtual bool IsDirty() const { return m_bDirty; }

  virtual void SetField(size_t n, const String& strValue);
  virtual void AddEMail(const String& strEMail)
    { m_astrEmails.Add(strEMail); m_bDirty = TRUE; }
  virtual void ClearExtraEMails();

  virtual bool Matches(const char *str, int where, int how);

  // pack/unpack data: we store it as a colon delimited list of values, but
  // it's too slow to modify it in place, so we unpack it to an array of
  // strings on creation and pack it back if it must be saved
    // strValue doesn't contain the name of the entry which is always the field
    // 0 and is set in the ctor
  void Load(const String& strValue);
    // save the data
  bool Save();

  // an easier to use GetName()
  const char *GetName() const { return m_astrFields[0]; }

  // get path to our entry in config file
  wxString GetPath() const;

  // if it's not, we will be deleted, so it really must be something fatal
  bool IsOk() const { return m_pGroup != NULL; }

  MOBJECT_DEBUG

private:
  virtual ~FCEntry();

  wxArrayString m_astrFields; // all text entries (some may be not present)
  wxArrayString m_astrEmails; // all email addresses except for the first one

  bool m_bDirty;              // dirty flag

  FCEntryGroup *m_pGroup;     // the group which contains us (never NULL)
};

// our AdbEntryGroup implementation: it maps on a wxFileConfig group
class FCEntryGroup : public AdbEntryGroup
{
public:
  // ctors
    // the normal one
  FCEntryGroup(FCEntryGroup *pParent, const wxString& strName,
               bool bNew = FALSE);
    // this one is only used for the root group
  FCEntryGroup(wxFileConfig *pConfig);

  // implement interface methods
    // AdbEntryGroup
  virtual AdbEntryGroup *GetGroup() const { return m_pParent; }

  virtual size_t GetEntryNames(wxArrayString& aNames) const;
  virtual size_t GetGroupNames(wxArrayString& aNames) const;

  virtual AdbEntry *GetEntry(const String& name) const;
  virtual AdbEntryGroup *GetGroup(const String& name) const;

  virtual bool Exists(const String& path) const;

  virtual AdbEntry *CreateEntry(const String& strName);
  virtual AdbEntryGroup *CreateGroup(const String& strName);

  virtual void DeleteEntry(const String& strName);
  virtual void DeleteGroup(const String& strName);

  virtual AdbEntry *FindEntry(const char *szName);

  // gte the config object
  wxFileConfig *GetConfig() const { return m_pConfig; }

  // get the full path to our group (not '/' terminated)
  wxString GetPath() const;

  // set the path corresponding to this group
  void SetOurPath() const { GetConfig()->SetPath(GetPath()); }

  // if it's not, we will be deleted, so it really must be something fatal
  bool IsOk() const { return m_pConfig != NULL; }

  MOBJECT_DEBUG

private:
  virtual ~FCEntryGroup();

  wxString      m_strName;      // our name
  wxFileConfig *m_pConfig;      // our config object
  FCEntryGroup *m_pParent;      // the parent group (never NULL)
};

// our AdbBook implementation: it maps to a disk file here
class FCBook : public AdbBook
{
public:
  FCBook(const String& filename);

  // implement interface methods
    // AdbElement
  virtual AdbEntryGroup *GetGroup() const { return NULL; }

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
    { return m_pRootGroup->GetGroup(name); }

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

    // AdbBook
  virtual const char *GetName() const;

  virtual void SetUserName(const String& name);
  virtual const char *GetUserName() const;

  virtual void SetDescription(const String& desc);
  virtual const char *GetDescription() const;

  virtual size_t GetNumberOfEntries() const;

  virtual bool IsLocal() const { return TRUE; }
  virtual bool IsReadOnly() const;

  MOBJECT_DEBUG

private:
  virtual ~FCBook();

  wxString m_strFile,       // our (full) config file name
           m_strFileName;   // name only (without path and extension)
  wxFileConfig *m_pConfig;  // our config object (on our file)

  FCEntryGroup *m_pRootGroup; // the ADB_Entries group
};

// our AdbDataProvider implementation
class FCDataProvider : public AdbDataProvider
{
public:
  // implement interface methods
  virtual AdbBook *CreateBook(const String& name);
  virtual bool EnumBooks(wxArrayString& aNames);
  virtual bool DeleteBook(AdbBook *book);
  virtual bool IsSupportedFormat(const String& name);

  MOBJECT_DEBUG

  DECLARE_ADB_PROVIDER(FCDataProvider);
};

IMPLEMENET_ADB_PROVIDER(FCDataProvider, TRUE, "Native format", Name_File);

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// FCEntry
// ----------------------------------------------------------------------------

FCEntry::FCEntry(FCEntryGroup *pGroup, const String& strName, bool bNew)
{
  m_pGroup = pGroup;
  m_astrFields.Add(strName);

  if ( bNew ) {
    // force the creation of our entry by writing something (writing empty
    // string will be ignored)
    if ( !pGroup->GetConfig()->Write(GetPath(), ":") ) {
      // also if it fails it means that something is wrong and this entry
      // can't be created, so be sure that our IsOk() will return FALSE
      m_pGroup = NULL;
    }
  }
  else {
    // we already exist in the config file, load our value
    wxString strValue;
    pGroup->GetConfig()->Read(&strValue, GetPath());
    Load(strValue);
  }

  m_bDirty = FALSE;
}

FCEntry::~FCEntry()
{
  if ( m_bDirty )
    Save();
}

wxString FCEntry::GetPath() const
{
  wxString strPath = ((FCEntryGroup *)GetGroup())->GetPath();
  strPath << '/' << m_astrFields[0];

  return strPath;
}

AdbEntryGroup *FCEntry::GetGroup() const
{
  return m_pGroup;
}

// we store only the fields which were non-empty, so check the index
void FCEntry::GetField(size_t n, String *pstr) const
{
  if ( n < m_astrFields.Count() )
    *pstr = m_astrFields[n];
  else
    pstr->Empty();
}

// the problem here is that we may have only several first strings in the
// m_astrFields array, so we need to add some before setting n-th field
void FCEntry::SetField(size_t n, const wxString& strValue)
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

void FCEntry::ClearExtraEMails()
{ 
  if ( !m_astrEmails.IsEmpty() ) {
    m_astrEmails.Empty();
    m_bDirty = TRUE;
  }
  //else: don't set dirty flag if it didn't change anything
}

bool FCEntry::Matches(const char *szWhat, int where, int how)
{
  wxString strWhat;

  // substring lookup looks for a part of the string, otherwise the entire
  // string should be matched by the pattern
  if ( how & AdbLookup_Substring )
    strWhat << '*' << szWhat << '*';
  else
    strWhat = szWhat;

  // if the search is not case sensitive, transform everything to lower case
  if ( (how & AdbLookup_CaseSensitive) == 0 )
    strWhat.MakeLower();

  wxString strField;

  #define CHECK_MATCH(field)                                        \
    if ( where & AdbLookup_##field ) {                              \
      strField = m_astrFields[AdbField_##field];                    \
      if ( (how & AdbLookup_CaseSensitive) == 0 )                   \
        strField.MakeLower();                                       \
      if ( strField.Matches(strWhat) )                              \
        return TRUE;                                                \
    }

  CHECK_MATCH(NickName);
  CHECK_MATCH(FullName);
  CHECK_MATCH(Organization);
  CHECK_MATCH(HomePage);

  // special case: we have to look in _all_ e-mail addresses
  // NB: this search is always case insensitive, because e-mail addresses are
  if ( where & AdbLookup_EMail ) {
    if ( m_astrFields.Count() > AdbField_EMail ) {
      strField = m_astrFields[AdbField_EMail];
      strField.MakeLower();
      if ( strField.Matches(strWhat) )
        return TRUE;
    }

    // look in additional email addresses too
    size_t nCount = m_astrEmails.Count();
    for ( size_t n = 0; n < nCount; n++ ) {
      strField = m_astrEmails[n];
      strField.MakeLower();
      if ( strField.Matches(strWhat) )
        return TRUE;
    }
  }

  return FALSE;
}

// load the data from file and parse it
void FCEntry::Load(const String& strValue)
{
  wxASSERT( AdbField_NickName == 0 ); // must be always the first one
  size_t nField = 1;

  // first read all the fields (up to AdbField_Max)
  wxString strCurrent;
  for ( const char *pc = strValue; ; pc++ ) {
    if ( *pc == ':' || *pc == '\0' ) {
      SetField(nField++, strCurrent);
      strCurrent.Empty();
      if ( *pc == '\0' )
        break;

      if ( nField == AdbField_Max ) {
        wxLogWarning(_("Entry '%s' has too many fields, extra ones ignored."),
                     GetName());
        break;
      }
    }
    else {
      // process escaped characters
      if ( *pc == '\\' ) {
        switch ( *++pc ) {
          case '\0':
            wxLogWarning(_("Trailing backslash discarded in entry '%s'."),
                         GetName());
            break;

          case 'n':
            strCurrent += '\n';
            break;

          case ':':
          case '\\':
            strCurrent += *pc;
            break;

          default:
            strCurrent << '\\' << *pc;
        }
      }
      else // not preceded by backslash
        strCurrent += *pc;
    }
  }

  // now deal with the email addresses (if any)
  if ( nField >= AdbField_OtherEMails ) {
    strCurrent.Empty();
    wxString str;
    GetField(AdbField_OtherEMails, &str);
    for ( const char *pc = str; ; pc++ ) {
      if ( *pc == ',' || *pc == '\0' ) {
        if ( !strCurrent.IsEmpty() )
          AddEMail(strCurrent);
        strCurrent.Empty();
        if ( *pc == '\0' )
          break;
      }
      else if ( *pc == '\\' ) {
        if ( *++pc == ',' )
          strCurrent += ',';
        else if ( *pc == '\0' )
          strCurrent << '\\';
        else
          strCurrent << '\\' << *pc;
      }
      else
        strCurrent += *pc;
    }
  }
  //else: no additional email addresses at all

  m_bDirty = FALSE;
}

// save entry to wxFileConfig (doesn't check if it's modified or not)
bool FCEntry::Save()
{
  wxCHECK_MSG( m_bDirty, TRUE, "shouldn't save unmodified FCEntry" );

  size_t nFieldMax = m_astrFields.Count();

  wxASSERT( nFieldMax <= AdbField_Max ); // too many fields?
  wxASSERT( AdbField_NickName == 0 );    // nickname must be always the first one

  wxString str, strValue;
  for ( size_t nField = 1; nField < nFieldMax; nField++ ) {
    // @@ in fact, it should be done if ms_aFields[nField].type == FieldList
    //    and not only for the emails, but so far it's the only list field we
    //    have
    if ( nField == AdbField_OtherEMails ) {
      // concatenate all e-mail addresses
      size_t nEMailCount = GetEMailCount();
      str.Empty();
      for ( size_t nEMail = 0; nEMail < nEMailCount; nEMail++ ) {
        if ( nEMail > 0 )
          str += ',';
        for ( const char *pc = m_astrEmails[nEMail]; *pc != '\0'; pc++ ) {
          if ( *pc == ',' )
            str +=  "\\,";
          else
            str += *pc;
        }
      }
    }
    else {
      GetField(nField, &str);
    }

    // escape special characters
    if ( nField > 1 )
      strValue += ':';
    for ( const char *pc = str; *pc != '\0'; pc++ ) {
      switch ( *pc ) {
        case ':':
          strValue << "\\:";
          break;

        case '\n':
          strValue << "\\n";
          break;

        default:
          strValue << *pc;
      }
    }
  }

  wxFileConfig *pConf = m_pGroup->GetConfig();

  return pConf->Write(GetPath(), strValue);
}

// ----------------------------------------------------------------------------
// FCEntryGroup
// ----------------------------------------------------------------------------

FCEntryGroup::FCEntryGroup(wxFileConfig *pConfig)
{
  m_pConfig = pConfig;
  m_pParent = NULL;
}

FCEntryGroup::FCEntryGroup(FCEntryGroup *pParent,
                           const wxString& strName,
                           bool bNew)
            : m_strName(strName)
{
  m_pConfig = pParent->GetConfig();
  m_pParent = pParent;

  if ( bNew ) {
    // force creation of the group
    if ( !m_pConfig->Write(GetPath(), "") ) {
      // something went wrong, don't create this group, the next line ensures
      // that IsOk() will return FALSE
      m_pConfig = NULL;
    }
  }
}

FCEntryGroup::~FCEntryGroup()
{
}

wxString FCEntryGroup::GetPath() const
{
  wxString strPath;
  if ( m_pParent == NULL )
    strPath = "/ADB_Entries";
  else
    strPath << m_pParent->GetPath() << "/" << m_strName;

  return strPath;
}

size_t FCEntryGroup::GetEntryNames(wxArrayString& aNames) const
{
  SetOurPath();

  aNames.Empty();

  wxString str;
  long dummy;

  // load all entries
  bool bCont = GetConfig()->GetFirstEntry(str, dummy);
  while ( bCont ) {
    aNames.Add(str);

    bCont = GetConfig()->GetNextEntry(str, dummy);
  }

  return aNames.Count();
}

size_t FCEntryGroup::GetGroupNames(wxArrayString& aNames) const
{
  SetOurPath();

  aNames.Empty();

  wxString str;
  long dummy;

  // load all subgroups
  bool bCont = GetConfig()->GetFirstGroup(str, dummy);
  while ( bCont ) {
    aNames.Add(str);

    bCont = GetConfig()->GetNextGroup(str, dummy);
  }

  return aNames.Count();
}

AdbEntry *FCEntryGroup::GetEntry(const String& name) const
{
  FCEntry *pEntry = new FCEntry((FCEntryGroup *)this, name);
  if ( !pEntry->IsOk() ) {
    pEntry->Unlock();
    pEntry = NULL;
  }

  return pEntry;
}

bool FCEntryGroup::Exists(const String& path) const
{
  SetOurPath();
  return GetConfig()->Exists(path);
}

AdbEntryGroup *FCEntryGroup::GetGroup(const String& name) const
{
  FCEntryGroup *pGroup = new FCEntryGroup((FCEntryGroup *)this, name);
  if ( !pGroup->IsOk() ) {
    pGroup->Unlock();
    pGroup = NULL;
  }

  return pGroup;
}

AdbEntry *FCEntryGroup::CreateEntry(const String& name)
{
  FCEntry *pEntry = new FCEntry((FCEntryGroup *)this, name, TRUE /* new */);
  if ( !pEntry->IsOk() ) {
    pEntry->Unlock();
    pEntry = NULL;
  }

  return pEntry;
}

AdbEntryGroup *FCEntryGroup::CreateGroup(const String& name)
{
  FCEntryGroup *pGroup = new FCEntryGroup((FCEntryGroup *)this, name, TRUE);
  if ( !pGroup->IsOk() ) {
    pGroup->Unlock();
    pGroup = NULL;
  }

  return pGroup;
}

void FCEntryGroup::DeleteEntry(const String& strName)
{
  SetOurPath();
  GetConfig()->DeleteEntry(strName, FALSE /* don't delete group */);
}

void FCEntryGroup::DeleteGroup(const String& strName)
{
  SetOurPath();
  GetConfig()->DeleteGroup(strName);
}

AdbEntry *FCEntryGroup::FindEntry(const char * /* szName */)
{
  return NULL;
}

// ----------------------------------------------------------------------------
// FCBook
// ----------------------------------------------------------------------------

FCBook::FCBook(const String& filename)
      : m_strFile(filename)
{
  m_strFileName = strutil_getfilename(m_strFile).Left('.');

  // we must load the file here because we need the ADB's name and description
  m_pConfig = new wxFileConfig(m_strFile, wxGetEmptyString());

  // create the root group
  m_pRootGroup = new FCEntryGroup(m_pConfig);
}

FCBook::~FCBook()
{
  SafeUnlock(m_pRootGroup);
  delete m_pConfig;
}

const char *FCBook::GetName() const
{
  return m_strFile.c_str();
}

void FCBook::SetUserName(const String& strAdb)
{
  m_pConfig->Write("/" ADB_HEADER "/" ADB_HEADER_NAME, strAdb);
}

const char *FCBook::GetUserName() const
{
  return m_pConfig->Read("/" ADB_HEADER "/" ADB_HEADER_NAME, m_strFileName);
}

void FCBook::SetDescription(const String& strDesc)
{
  m_pConfig->Write("/" ADB_HEADER "/" ADB_HEADER_DESC, strDesc);
}

const char *FCBook::GetDescription() const
{
  return m_pConfig->Read("/" ADB_HEADER "/" ADB_HEADER_DESC, m_strFile);
}

size_t FCBook::GetNumberOfEntries() const
{
  m_pConfig->SetPath("/" ADB_ENTRIES);
  return m_pConfig->GetNumberOfEntries(TRUE);
}

bool FCBook::IsReadOnly() const
{
  return !wxFile::Access(m_strFile, wxFile::write);
}

// ----------------------------------------------------------------------------
// FCDataProvider
// ----------------------------------------------------------------------------

AdbBook *FCDataProvider::CreateBook(const String& name)
{
  return new FCBook(name);
}

bool FCDataProvider::EnumBooks(wxArrayString& /* aNames */)
{
  // TODO
  return FALSE;
}

bool FCDataProvider::IsSupportedFormat(const String& name)
{
  // the test is not 100% fool proof...
  FILE *fp = fopen(name, "rt");
  if ( fp == NULL )
    return FALSE;

  char buf[1024];
  bool bOk = FALSE;
  while ( fgets(buf, WXSIZEOF(buf), fp) ) {
    const char *pc = buf;
    while ( isspace(*pc) )
      pc++;

    // is it a wxFileConfig comment or is the line empty?
    if ( *pc == '#' || *pc == ';' || *pc == '\0' )
      continue; // yes, ignore this line

    bOk = strstr(pc, ADB_HEADER) || strstr(pc, ADB_ENTRIES);
    break;
  }

  fclose(fp);

  return bOk;
}

bool FCDataProvider::DeleteBook(AdbBook * /* book */)
{
  // TODO
  return FALSE;
}

// ----------------------------------------------------------------------------
// debugging support
// ----------------------------------------------------------------------------

#ifdef DEBUG

String FCEntry::Dump() const
{
  String str;
  str.Printf("FC: name = '%s', m_nRef = %d", GetName(), m_nRef);

  return str;
}

String FCEntryGroup::Dump() const
{
  String str;
  str.Printf("FCEntryGroup: path = '%s', m_nRef = %d",
              GetPath().c_str(), m_nRef);

  return str;
}

String FCBook::Dump() const
{
  String str;
  str.Printf("FCBook: file = '%s', m_nRef = %d", m_strFile.c_str(), m_nRef);

  return str;
}

String FCDataProvider::Dump() const
{
  String str;
  str.Printf("FCDataProvider: m_nRef = %d", m_nRef);

  return str;
}

#endif // DEBUG
