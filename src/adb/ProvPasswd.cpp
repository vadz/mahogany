///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   adb/ProvPasswd.cpp: provides address info from /etc/passwd
// Purpose:     on a Unix system this module provides access to the addresses
//              and personal names (gecos field) from /etc/passwd via
//              AdbDataProvider interface
// Author:      Vadim Zeitlin
// Modified by:
// Created:     21.08.01
// CVS-ID:      $Id$
// Copyright:   (c) 2001 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// M
#include "Mpch.h"

#if defined(OS_UNIX) || defined(__CYGWIN__)

#ifndef  USE_PCH
#  include "Mcommon.h"
// FIXME: we shouldn't use the apps functions from modules...
#  include "MApplication.h"
#  include "Profile.h"
#  include "Mdefaults.h"

#  include <wx/string.h>                // for wxString
#  include <wx/dynarray.h>              // for wxSortedArrayString
#endif //USE_PCH

#include "Address.h"

#include "adb/AdbManager.h"
#include "adb/AdbEntry.h"
#include "adb/AdbBook.h"
#include "adb/AdbDataProvider.h"

#include <pwd.h>
#include <sys/types.h>

extern const MOption MP_HOSTNAME;

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// fwd decl
class PasswdEntry;
class PasswdEntryGroup;
class PasswdBook;
class PasswdDataProvider;

// our AdbEntryData implementation
class PasswdEntry : public AdbEntry
{
public:
   // ctor
   PasswdEntry(PasswdEntryGroup *pGroup,
               const String& login, const String& name);

   // implement interface methods

   // AdbEntry
   virtual AdbEntryGroup *GetGroup() const { return m_pGroup; }

   virtual void GetFieldInternal(size_t n, String *pstr) const
      { GetField(n, pstr); }
   virtual void GetField(size_t n, String *pstr) const;

   virtual size_t GetEMailCount() const { return 1; }
   virtual void GetEMail(size_t n, String *pstr) const;

   virtual void ClearDirty() { }
   virtual bool IsDirty() const { return false; }

   virtual void SetField(size_t n, const String& strValue) { }
   virtual void AddEMail(const String& strEMail) { }
   virtual void ClearExtraEMails() { }

   virtual int Matches(const wxChar *str, int where, int how) const;

private:
   // user (==login) name and real name
   wxString m_username,
            m_realname;

   // the group which contains us (NULL for root)
   AdbEntryGroup *m_pGroup;
};

// our AdbEntryGroup implementation
class PasswdEntryGroup : public AdbEntryGroupCommon
{
public:
   // ctor
   PasswdEntryGroup(AdbEntryGroup *pParent) { m_pParent = pParent; }

   // implement interface methods

   // AdbEntryGroup
   virtual AdbEntryGroup *GetGroup() const { return m_pParent; }
   virtual String GetName() const { return _("Local Users"); }

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

private:
   // read /etc/passwd
   void ReadPasswdDb();

   // the parent group (well, book, in fact)
   AdbEntryGroup *m_pParent;

   // the arrays containing the user/real names from /etc/passwd
   wxSortedArrayString m_names;
   wxArrayString m_gecos;

   GCC_DTOR_WARN_OFF
};

// our AdbBook implementation: it maps to the entire /etc/passwd file which is
// accessed via its only group
class PasswdBook : public AdbBook
{
public:
   PasswdBook();

   // implement interface methods
   // ---------------------------

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

   virtual bool IsLocal() const { return true; }
   virtual bool IsReadOnly() const { return true; }

private:
   virtual ~PasswdBook();

   // the one and only group we have
   PasswdEntryGroup *m_pRootGroup;

   GCC_DTOR_WARN_OFF
};

// our AdbDataProvider implementation
class PasswdDataProvider : public AdbDataProvider
{
public:
   // implement interface methods
   virtual AdbBook *CreateBook(const String& name);
   virtual bool EnumBooks(wxArrayString& aNames);
   virtual bool DeleteBook(AdbBook *book);
   virtual bool TestBookAccess(const String& name, AdbTests test);

   virtual bool HasField(AdbField field) const;
   virtual bool HasMultipleEMails() const { return false; }

   DECLARE_ADB_PROVIDER(PasswdDataProvider);
};

IMPLEMENT_ADB_PROVIDER(PasswdDataProvider, true, "Unix /etc/passwd", Name_No);

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// PasswdEntry
// ----------------------------------------------------------------------------

PasswdEntry::PasswdEntry(PasswdEntryGroup *pGroup,
                         const String& login,
                         const String& name)
           : m_username(login), m_realname(name)
{
   m_pGroup = pGroup;
}

void PasswdEntry::GetField(size_t n, String *pstr) const
{
   switch ( n )
   {
      case AdbField_NickName:
         *pstr = m_username;
         break;

      case AdbField_FullName:
         *pstr = m_realname;
         break;

      case AdbField_EMail:
         GetEMail(0, pstr);
         break;

      default:
         pstr->clear();
   }
}

void PasswdEntry::GetEMail(size_t n, String *pstr) const
{
   CHECK_RET( !n, _T("we have only one email") );

   AddressList_obj addrList(m_username, READ_APPCONFIG(MP_HOSTNAME));

   Address *addr = addrList->GetFirst();
   if ( addr )
   {
      *pstr = addr->GetAddress();
   }
}

int PasswdEntry::Matches(const wxChar *what, int where, int how) const
{
   // substring lookup looks for a part of the string, "starts with" means
   // what is says, otherwise the entire string should be matched by the
   // pattern
   wxString strWhat;
   if ( how & AdbLookup_Substring )
      strWhat << '*' << what << '*';
   else if ( how & AdbLookup_StartsWith )
      strWhat << what << '*';
   else
      strWhat = what;

   // if the search is not case sensitive, transform everything to lower case
   if ( !(how & AdbLookup_CaseSensitive) )
      strWhat.MakeLower();

   wxString strField;

   #define CHECK_MATCH(field)                                        \
      if ( where & AdbLookup_##field ) {                             \
         GetField(AdbField_##field, &strField);                      \
         if ( (how & AdbLookup_CaseSensitive) == 0 )                 \
            strField.MakeLower();                                    \
         if ( strField.Matches(strWhat) )                            \
            return AdbLookup_##field;                                \
   }

   CHECK_MATCH(NickName);
   CHECK_MATCH(FullName);
   CHECK_MATCH(EMail);

   return 0;
}

// ----------------------------------------------------------------------------
// PasswdEntryGroup
// ----------------------------------------------------------------------------

void PasswdEntryGroup::ReadPasswdDb()
{
   struct passwd *pwd;
   while ( (pwd = getpwent()) != NULL )
   {
      size_t index = m_names.Add(wxSafeConvertMB2WX(pwd->pw_name));

      // at least under Linux gecos fields of the password database often have
      // trailing commas to indicate missing fields but we don't want to use
      // them as part of "real name"
      wxString gecos(wxSafeConvertMB2WX(pwd->pw_gecos));
      gecos.EndsWith(",,,", &gecos);

      m_gecos.Insert(gecos, index);
   }

   endpwent();
}

size_t PasswdEntryGroup::GetEntryNames(wxArrayString& names) const
{
   if ( m_names.IsEmpty() )
   {
      ((PasswdEntryGroup *)this)->ReadPasswdDb(); // const_cast
   }

   names = m_names;

   return names.Count();
}

size_t PasswdEntryGroup::GetGroupNames(wxArrayString& names) const
{
   // /etc/passwd is not hierarchical
   names.Empty();

   return 0;
}

AdbEntry *PasswdEntryGroup::GetEntry(const String& name)
{
   int n = m_names.Index(name);
   if ( n == wxNOT_FOUND )
      return NULL;

   return new PasswdEntry((PasswdEntryGroup *)this,
                          m_names[(size_t)n],
                          m_gecos[(size_t)n]);
}

bool PasswdEntryGroup::Exists(const String& name)
{
   return m_names.Index(name) != wxNOT_FOUND;
}

AdbEntryGroup *PasswdEntryGroup::GetGroup(const String& name) const
{
   return NULL;
}

AdbEntry *PasswdEntryGroup::CreateEntry(const String& strName)
{
   // we're read only
   return NULL;
}

AdbEntryGroup *PasswdEntryGroup::CreateGroup(const String& strName)
{
   // we're read only
   return NULL;
}

void PasswdEntryGroup::DeleteEntry(const String& strName)
{
   // we're read only
}

void PasswdEntryGroup::DeleteGroup(const String& strName)
{
   // we're read only
}

AdbEntry *PasswdEntryGroup::FindEntry(const wxChar *szName)
{
   // TODO
   return NULL;
}

// ----------------------------------------------------------------------------
// PasswdBook
// ----------------------------------------------------------------------------

PasswdBook::PasswdBook()
{
   // create the root group
   m_pRootGroup = new PasswdEntryGroup(this);
}

PasswdBook::~PasswdBook()
{
   SafeDecRef(m_pRootGroup);
}

bool PasswdBook::IsSameAs(const String& name) const
{
   // there is only one password book, it's never the same as anything else
   return false;
}

String PasswdBook::GetFileName() const
{
   return wxEmptyString;
}

void PasswdBook::SetName(const String& WXUNUSED(strName))
{
}

String PasswdBook::GetName() const
{
   // it just returns "Local Users" string
   return m_pRootGroup->GetName();
}

void PasswdBook::SetDescription(const String& WXUNUSED(strDesc))
{
}

String PasswdBook::GetDescription() const
{
   return _("The users with accounts on this system");
}

size_t PasswdBook::GetNumberOfEntries() const
{
   // the main group only
   return 1;
}

// ----------------------------------------------------------------------------
// PasswdDataProvider
// ----------------------------------------------------------------------------

AdbBook *PasswdDataProvider::CreateBook(const String& name)
{
   ASSERT_MSG( name.empty(), _T("book name is ignored by PasswdDataProvider") );

   return new PasswdBook();
}

bool PasswdDataProvider::EnumBooks(wxArrayString& names)
{
   return false;
}

bool PasswdDataProvider::TestBookAccess(const String& name, AdbTests test)
{
   // we can't create nor write to /etc/passwd
   switch ( test )
   {
      default:
         FAIL_MSG( _T("unknown access method") );
         // fall through

      case Test_Create:
      case Test_Open:
         return false;

      case Test_OpenReadOnly:
         return name.empty();
         
      case Test_AutodetectCapable:
      case Test_RecognizesName:
         return true;
   }
}

bool PasswdDataProvider::DeleteBook(AdbBook *book)
{
   return false;
}

bool PasswdDataProvider::HasField(AdbField field) const
{
   bool has;
   
   switch ( field )
   {
      case AdbField_NickName:
      case AdbField_FullName:
      case AdbField_EMail:
         has = true;
         break;

      default:
         has = false;
         break;
   }
   
   return has;
}

#endif // OS_UNIX
