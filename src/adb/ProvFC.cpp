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
#  include "strutil.h"
#  include "sysutil.h"
#  include "MApplication.h"

#  include <wx/string.h>                // for wxString
#  include <wx/log.h>                   // for wxLogWarning
#endif //USE_PCH

// wxWindows
#include <wx/file.h>                    // for wxFile
#include <wx/confbase.h>
#include <wx/fileconf.h>

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
class FCEntry : public AdbEntryStoredInMemory
{
public:
  // ctor
  FCEntry(FCEntryGroup *pGroup, const String& strName, bool bNew = FALSE);

  // implement interface methods
    // AdbEntry
  virtual AdbEntryGroup *GetGroup() const;

  // pack/unpack data: we store it as a colon delimited list of values, but
  // it's too slow to modify it in place, so we unpack it to an array of
  // strings on creation and pack it back if it must be saved
    // strValue doesn't contain the name of the entry which is always the field
    // 0 and is set in the ctor
  void Load(const String& strValue);
    // save the data
  bool Save();

  // an easier to use GetName()
  const wxChar *GetName() const { return m_astrFields[0]; }

  // get path to our entry in config file
  wxString GetPath() const;

  // if it's not, we will be deleted, so it really must be something fatal
  bool IsOk() const { return m_pGroup != NULL; }

  MOBJECT_DEBUG(FCEntry)

private:
  virtual ~FCEntry();

  FCEntryGroup *m_pGroup;     // the group which contains us (never NULL)

  GCC_DTOR_WARN_OFF
};

// our AdbEntryGroup implementation: it maps on a wxFileConfig group
class FCEntryGroup : public AdbEntryGroupCommon
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

  // gte the config object
  wxFileConfig *GetConfig() const { return m_pConfig; }

  // get the full path to our group (not '/' terminated)
  wxString GetPath() const;

  // set the path corresponding to this group
  void SetOurPath() const { GetConfig()->SetPath(GetPath()); }

  // if it's not, we will be deleted, so it really must be something fatal
  bool IsOk() const { return m_pConfig != NULL; }

  // recursively call Inc/DecRef() on this group and all its parents: this is
  // necessary to ensure that the group stays alive as long as it has any
  // [grand] children
  void IncRefRecursively()
  {
     FCEntryGroup *group = this;
     do
     {
        // we can't use group->m_pParent after calling DecRef() on it as it
        // might be already deleted then
        FCEntryGroup *parent = group->m_pParent;
        group->IncRef();
        group = parent;
     }
     while ( group );
  }

  void DecRefRecursively()
  {
     FCEntryGroup *group = this;
     do
     {
        FCEntryGroup *parent = group->m_pParent;
        group->DecRef();
        group = parent;
     }
     while ( group );
  }

  MOBJECT_DEBUG(FCEntryGroup)

private:
  virtual ~FCEntryGroup();

  wxString      m_strName;      // our name
  wxFileConfig *m_pConfig;      // our config object
  FCEntryGroup *m_pParent;      // the parent group (never NULL)

  GCC_DTOR_WARN_OFF
};

// our AdbBook implementation: it maps to a disk file here
class FCBook : public AdbBook
{
public:
  // get the full file name - it's the same as the filename passed in for the
  // absolute pathes or the name with M local dir prepended for the other ones
  static String GetFullAdbPath(const String& filename);

  FCBook(const String& filename);

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

  virtual bool Flush();

  MOBJECT_DEBUG(FCBook)

private:
  virtual ~FCBook();

  wxString m_strFile,       // our (full) config file name
           m_strFileName;   // name only (without path and extension)
  wxFileConfig *m_pConfig;  // our config object (on our file)

  FCEntryGroup *m_pRootGroup; // the ADB_Entries group

  GCC_DTOR_WARN_OFF
};

// our AdbDataProvider implementation
class FCDataProvider : public AdbDataProvider
{
public:
  // implement interface methods
  virtual AdbBook *CreateBook(const String& name);
  virtual bool EnumBooks(wxArrayString& aNames);
  virtual bool DeleteBook(AdbBook *book);
  virtual bool TestBookAccess(const String& name, AdbTests test);
   
  // Our entry is derived from AdbEntryStoredInMemory
  virtual bool HasField(AdbField /* field */) const { return true; }
  virtual bool HasMultipleEMails() const { return true; }

  MOBJECT_DEBUG(FCDataProvider)

  DECLARE_ADB_PROVIDER(FCDataProvider);
};

IMPLEMENT_ADB_PROVIDER(FCDataProvider, TRUE, "Native format", Name_File);

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
    if ( !pGroup->GetConfig()->Write(GetPath(), wxString(_T(":"))) ) {
      // also if it fails it means that something is wrong and this entry
      // can't be created, so be sure that our IsOk() will return FALSE
      m_pGroup = NULL;
    }
  }
  else {
    // we already exist in the config file, load our value
    wxString strValue;
    pGroup->GetConfig()->Read(GetPath(), &strValue);
    Load(strValue);
  }

  m_pGroup->IncRefRecursively();
  m_bDirty = FALSE;
}

FCEntry::~FCEntry()
{
  if ( m_bDirty )
    Save();

  m_pGroup->DecRefRecursively();
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

// load the data from file and parse it
void FCEntry::Load(const String& strValue)
{
  wxASSERT( AdbField_NickName == 0 ); // must be always the first one
  size_t nField = 1;

  // first read all the fields (up to AdbField_Max)
  wxString strCurrent;
  for ( const wxChar *pc = strValue; ; pc++ ) {
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
    for ( const wxChar *pc = str; ; pc++ ) {
      if ( *pc == ',' || *pc == '\0' ) {
        if ( !strCurrent.empty() )
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
  wxCHECK_MSG( m_bDirty, TRUE, _T("shouldn't save unmodified FCEntry") );

  size_t nFieldMax = m_astrFields.Count();

  wxASSERT( nFieldMax <= AdbField_Max ); // too many fields?
  wxASSERT( AdbField_NickName == 0 );    // nickname must be always the first one

  // optimize a bit: remove all trailing empty fields
  wxString strField;
  while ( strField.empty() )
  {
      GetField(--nFieldMax, &strField);
  }

  nFieldMax++; // compensate for the last '--'

  if ( nFieldMax <= AdbField_OtherEMails && m_bEMailDirty ) {
     // otherwise we wouldn't save the emails at all even though we should
     nFieldMax = AdbField_OtherEMails + 1;
  }

  wxString strValue;
  for ( size_t nField = 1; nField < nFieldMax; nField++ ) {
    // FIXME in fact, it should be done if ms_aFields[nField].type == FieldList
    //       and not only for the emails, but so far it's the only list field
    //       we have
    if ( nField == AdbField_OtherEMails ) {
      // concatenate all e-mail addresses
      size_t nEMailCount = GetEMailCount();
      strField.Empty();
      for ( size_t nEMail = 0; nEMail < nEMailCount; nEMail++ ) {
        if ( nEMail > 0 )
          strField += ',';
        for ( const wxChar *pc = m_astrEmails[nEMail]; *pc != '\0'; pc++ ) {
          if ( *pc == ',' )
            strField +=  _T("\\,");
          else
            strField += *pc;
        }
      }
    }
    else {
      GetField(nField, &strField);
    }

    // escape special characters
    if ( nField > 1 )
      strValue += ':';
    for ( const wxChar *pc = strField; *pc != '\0'; pc++ ) {
      switch ( *pc ) {
        case ':':
          strValue << _T("\\:");
          break;

        case '\n':
          strValue << _T("\\n");
          break;

        default:
          strValue << *pc;
      }
    }
  }

  wxFileConfig *pConf = m_pGroup->GetConfig();

  bool ok = pConf->Write(GetPath(), strValue);

  return ok;
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
    String path = GetPath();
    if ( !path || path.Last() != '/' )
       path += '/';
    if ( !m_pConfig->Write(path, wxEmptyString) ) {
      // something went wrong, don't create this group, the next line ensures
      // that IsOk() will return FALSE
      m_pConfig = NULL;
    }
  }

  if ( m_pParent )
     m_pParent->IncRefRecursively();
}

FCEntryGroup::~FCEntryGroup()
{
  if ( m_pParent )
     m_pParent->DecRefRecursively();
}

wxString FCEntryGroup::GetPath() const
{
  wxString strPath;
  if ( m_pParent == NULL )
    strPath = _T("/ADB_Entries");
  else
    strPath << m_pParent->GetPath() << _T("/") << m_strName;

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

AdbEntry *FCEntryGroup::GetEntry(const String& name)
{
  FCEntry *pEntry = new FCEntry((FCEntryGroup *)this, name);
  if ( !pEntry->IsOk() ) {
    pEntry->DecRef();
    pEntry = NULL;
  }

  return pEntry;
}

bool FCEntryGroup::Exists(const String& path)
{
  SetOurPath();
  return GetConfig()->Exists(path);
}

AdbEntryGroup *FCEntryGroup::GetGroup(const String& name) const
{
  FCEntryGroup *pGroup = new FCEntryGroup((FCEntryGroup *)this, name);
  if ( !pGroup->IsOk() ) {
    pGroup->DecRef();
    pGroup = NULL;
  }

  return pGroup;
}

AdbEntry *FCEntryGroup::CreateEntry(const String& name)
{
  CHECK( !!name, NULL, _T("can't create entries with empty names") );

  FCEntry *pEntry = new FCEntry((FCEntryGroup *)this, name, TRUE /* new */);
  if ( !pEntry->IsOk() ) {
    pEntry->DecRef();
    pEntry = NULL;
  }

  return pEntry;
}

AdbEntryGroup *FCEntryGroup::CreateGroup(const String& name)
{
  FCEntryGroup *pGroup = new FCEntryGroup((FCEntryGroup *)this, name, TRUE);
  if ( !pGroup->IsOk() ) {
    pGroup->DecRef();
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

AdbEntry *FCEntryGroup::FindEntry(const wxChar * /* szName */)
{
  return NULL;
}

// ----------------------------------------------------------------------------
// FCBook
// ----------------------------------------------------------------------------

String FCBook::GetFullAdbPath(const String& filename)
{
  CHECK( !filename.empty(), wxEmptyString, _T("ADB without name?") );

  String path;
  if ( wxIsAbsolutePath(filename) )
    path = filename;
  else
    path << mApplication->GetLocalDir() << DIR_SEPARATOR << filename;

  return path;
}

FCBook::FCBook(const String& filename)
      : m_strFile(GetFullAdbPath(filename))
{
  String file = strutil_getfilename(filename);
  String fileBaseName = file.BeforeLast('.');
  m_strFileName = fileBaseName.empty() ? file : fileBaseName;

  // we must load the file here because we need the ADB's name and description
  m_pConfig = new wxFileConfig(wxGetEmptyString(), wxGetEmptyString(),
                               m_strFile);

  // the ADB files contain arbitrary data, don't try to expand env vars in
  // them
  m_pConfig->SetExpandEnvVars(FALSE);

  // create the root group
  m_pRootGroup = new FCEntryGroup(m_pConfig);
}

FCBook::~FCBook()
{
  SafeDecRef(m_pRootGroup);
  delete m_pConfig;
}

bool FCBook::IsSameAs(const String& name) const
{
   String path = GetFullAdbPath(name);

   return sysutil_compare_filenames(m_strFile, path);
}

String FCBook::GetFileName() const
{
  return m_strFile;
}

void FCBook::SetDescription(const String& strAdb)
{
  m_pConfig->Write("/" ADB_HEADER "/" ADB_HEADER_DESC, strAdb);
}

String FCBook::GetDescription() const
{
  return m_pConfig->Read("/" ADB_HEADER "/" ADB_HEADER_DESC, m_strFileName);
}

void FCBook::SetName(const String& strDesc)
{
  m_pConfig->Write("/" ADB_HEADER "/" ADB_HEADER_NAME, strDesc);
}

String FCBook::GetName() const
{
  return m_pConfig->Read("/" ADB_HEADER "/" ADB_HEADER_NAME, m_strFile);
}

size_t FCBook::GetNumberOfEntries() const
{
  m_pConfig->SetPath("/" ADB_ENTRIES);
  return m_pConfig->GetNumberOfEntries(TRUE);
}

bool FCBook::IsReadOnly() const
{
  // if the file doesn't exist yet, we can't use Access() (it would return
  // false) but we can be sure that we're not read only as a new book can be
  // successfully opened with a non existing file name only for writing
  return wxFile::Exists(m_strFile) && !wxFile::Access(m_strFile, wxFile::write);
}

bool FCBook::Flush()
{
   if ( !m_pConfig->Flush() )
   {
      wxLogError(_("Couldn't create or write address book file '%s'."),
                 m_strFile.c_str());

      return false;
   }

   return true;
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

bool FCDataProvider::TestBookAccess(const String& name, AdbTests test)
{
  String fullname = FCBook::GetFullAdbPath(name);

  bool ok = FALSE;

  switch ( test )
  {
    case Test_Open:
    case Test_OpenReadOnly:
      {
        // the test is not 100% fool proof...
        FILE *fp = wxFopen(fullname, _T("rt"));
        if ( fp != NULL )
        {
          char buf[1024];
          while ( fgets(buf, WXSIZEOF(buf), fp) ) {
            const char *pc = buf;
            while ( isspace(*pc) )
              pc++;

            // is it a wxFileConfig comment or is the line empty?
            if ( *pc == '#' || *pc == ';' || *pc == '\n' )
              continue; // yes, ignore this line

            ok = strstr(pc, ADB_HEADER) || strstr(pc, ADB_ENTRIES);
            break;
          }

          fclose(fp);
        }
      }
      break;

    case Test_Create:
      {
        // no error messages please
        wxLogNull nolog;

        // it's the only portable way to test for it I can think of
        wxFile file;
        if ( !file.Create(fullname, FALSE /* !overwrite */) )
        {
          // either it already exists or we don't have permission to create
          // it there. Check whether it exists now.
          ok = file.Open(fullname, wxFile::write_append);
        }
        else
        {
          // it hadn't existed an we managed to create it - so we do have
          // permissions. Don't forget to remove it now.
          ok = TRUE;

          file.Close();
          wxRemove(fullname);
        }
      }
      break;

    case Test_AutodetectCapable:
      ok = true;
      break;
      
    case Test_RecognizesName:
      ok = false;
      break;
      
    default:
      FAIL_MSG(_T("invalid test in TestBookAccess"));
  }

  return ok;
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

String FCEntry::DebugDump() const
{
  String str = MObjectRC::DebugDump();
  str << _T("name = '") << GetName() << '\'';

  return str;
}

String FCEntryGroup::DebugDump() const
{
  String str = MObjectRC::DebugDump();
  str << _T("path = '") << GetPath() << '\'';

  return str;
}

String FCBook::DebugDump() const
{
  String str = MObjectRC::DebugDump();
  str << _T("file = '") << m_strFile << '\'';

  return str;
}

String FCDataProvider::DebugDump() const
{
  String str = MObjectRC::DebugDump();

  return str;
}

#endif // DEBUG
