///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   adb/ProvBbdb.cpp - AdbDataProvider interfacing to
//              .bbdb from emacs
// Author:      Karsten Ballüder
// Modified by:
// Created:     20 Jan 1999
// CVS-ID:      $Id$
// Copyright:   (c) 1999 Karsten Ballüder <Ballueder@usa.net>
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
#  include "Profile.h"
#  include "MApplication.h"
#  include "strutil.h"
#  include "sysutil.h"
#  include "kbList.h"                   // for KBLIST_DEFINE
#  include "Mdefaults.h"
#  include "guidef.h"

#  include <wx/string.h>                // for wxString
#  include <wx/log.h>                   // for wxLogWarning
#endif //USE_PCH

#include "gui/wxMDialogs.h"

#include "adb/AdbManager.h"
#include "adb/AdbEntry.h"
#include "adb/AdbBook.h"
#include "adb/AdbDataProvider.h"

// wxWindows
#include <wx/file.h>

#if wxUSE_IOSTREAMH
#  include <fstream.h>                  // for ifstream
#else
#  include <fstream>                    // for ifstream
#endif

class MOption;

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_BBDB_ANONYMOUS;
extern const MOption MP_BBDB_GENERATEUNIQUENAMES;
extern const MOption MP_BBDB_IGNOREANONYMOUS;
extern const MOption MP_BBDB_SAVEONEXIT;

// ----------------------------------------------------------------------------
// persistent msgboxes we use here
// ----------------------------------------------------------------------------

extern const MPersMsgBox *M_MSGBOX_BBDB_SAVE_DIALOG;



/** BBDB Addressbook format:
    Record Vectors
..............

Name      Type           Accessor and Modifier      Description
First     String         `bbdb-record-firstname'    Entity's first name
name                     `bbdb-record-set-firstname'
Last name String         `bbdb-record-lastname'     Entity's last name
                         `bbdb-record-set-lastname'
AKAs      List of        `bbdb-record-aka'          Alternate names for
          Strings        `bbdb-record-set-aka'      entity
Company   String         `bbdb-record-company'      Company with which
                         `bbdb-record-set-company'  entity is associated
Phones    List of        `bbdb-record-phones'       List of phone number
          Vectors        `bbdb-record-set-phones'   vectors
Addresses List of        `bbdb-record-addresses'    List of address vectors
          Vectors        `bbdb-record-set-addresses'
Net       List of        `bbdb-record-net'          List of network
address   Strings        `bbdb-record-set-net'      addresses
Notes     String or      `bbdb-record-raw-notes'    String or Association
          Alist          `bbdb-record-set-raw-notes'list of note fields
                                                    (strings)
Cache     Vector         `bbdb-record-cache'        Record cache.
                         `bbdb-record-set-cache'    Internal version only.
*/

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// fwd decl
class BbdbEntry;
class BbdbEntryGroup;
class BbdbBook;
class BbdbDataProvider;

KBLIST_DEFINE(kbStringListList, kbStringList);
KBLIST_DEFINE(kbStringListListList, kbStringListList);

// our AdbEntryData implementation
class BbdbEntry : public AdbEntryStoredInMemory
{
public:
   // ctor
   BbdbEntry(BbdbEntryGroup *pGroup, const String& strName);

   /// create an empty entry from a line from the .bbdb file
   BbdbEntry(BbdbEntryGroup *pGroup);
   /// parse a .bbdb file line
   static BbdbEntry *ParseLine(BbdbEntryGroup *pGroup, String * line);

   // implement interface methods
   // AdbEntry
   virtual AdbEntryGroup *GetGroup() const;

   // an easier to use GetName()
   const wxChar *GetName() const
      { return m_astrFields[0]; }

   /**@name the parser */
   //@{
   enum FieldTypes { Field_String, Field_Integer};
   static const String ReadString(String *string, bool *success = NULL);
   static kbStringList *ReadListOfStrings(String *string, bool *success = NULL);
   static kbStringListList *ReadVector(String *string);
   static kbStringListListList *ReadListOfVectors(String *string);

   static void WriteString(ostream &out, String const &string);

   static bool ReadNil(String *line);
   static bool ReadHeader(String *version, String *line);
   static bool ReadToken(wxChar token, String *string);

   static int m_IgnoreAnonymous; // really a bool,set to -1 at beginnin
   static String m_AnonymousName;
   static bool m_EnforceUnique;
//@}

private:
   BbdbEntryGroup *m_pGroup;     // the group which contains us (NULL for root)
};

int BbdbEntry::m_IgnoreAnonymous = -1;
String BbdbEntry::m_AnonymousName;
bool BbdbEntry::m_EnforceUnique;


KBLIST_DEFINE(BbdbEntryList, BbdbEntry);

// our AdbEntryGroup implementation
class BbdbEntryGroup : public AdbEntryGroup
{
public:
   // ctors
   // the normal one
   BbdbEntryGroup(BbdbEntryGroup *pParent, const wxString& strName);

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
   virtual AdbEntryGroup *CreateGroup(const String& strName);

   virtual void DeleteEntry(const String& strName);
   virtual void DeleteGroup(const String& strName);

   virtual AdbEntry *FindEntry(const wxChar *szName) const;

private:
   virtual ~BbdbEntryGroup();

   BbdbEntryList    *m_entries;
   wxString         m_strName;      // our name
   BbdbEntryGroup   *m_pParent;      // the parent group (never NULL)
   GCC_DTOR_WARN_OFF
};

// our AdbBook implementation: it maps to a disk file here
class BbdbBook : public AdbBook
{
public:
   BbdbBook(const String& filename);

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

   virtual AdbEntry *FindEntry(const wxChar *szName) const
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

   /** Return the icon name if set. The numeric return value must be -1
     for the default, or an index into the image list in AdbFrame.cpp.
    */
   virtual int GetIconId() const { return 6; }

private:
   virtual ~BbdbBook();

   wxString m_strFileName,
            m_strName,
            m_strDesc;

   BbdbEntryGroup *m_pRootGroup; // the ADB_Entries group

   GCC_DTOR_WARN_OFF
};

// our AdbDataProvider implementation
class BbdbDataProvider : public AdbDataProvider
{
public:
   // implement interface methods
   virtual AdbBook *CreateBook(const String& name);
   virtual bool EnumBooks(wxArrayString& aNames);
   virtual bool DeleteBook(AdbBook *book);
   virtual bool TestBookAccess(const String& name, AdbTests test);

   DECLARE_ADB_PROVIDER(BbdbDataProvider);
};

IMPLEMENT_ADB_PROVIDER(BbdbDataProvider, TRUE, _T("BBDB version 2"), Name_File);

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// BbdbEntry
// ----------------------------------------------------------------------------

BbdbEntry::BbdbEntry(BbdbEntryGroup *pGroup, const String& strName)
{
   m_pGroup = pGroup;
   m_bDirty = FALSE;
}

BbdbEntry::BbdbEntry(BbdbEntryGroup *pGroup)
{
   m_pGroup = pGroup;
   m_bDirty = FALSE;
}

bool
BbdbEntry::ReadToken(wxChar token, String * line)
{
   if(!line || line->empty())
      return false;

   strutil_delwhitespace(*line);
   if(*(line->c_str())==token)
   {
      *line = line->Right(line->Length()-1);
      return true;
   }
   return false;
}

bool
BbdbEntry::ReadHeader(String *version, String *line)
{
   if(!ReadToken(';', line))
      return false;

   while(ReadToken(';', line))
      ;
   strutil_delwhitespace(*line);

   if(line->Left(wxStrlen(_T("file-version"))) != _T("file-version"))
      return false;

   *line = line->Right(line->Length()-wxStrlen(_T("file-version")));

   if(! ReadToken(':', line))
      return false;

   strutil_delwhitespace(*line);
   if(version)
      *version = *line;
   return true;
}

bool
BbdbEntry::ReadNil(String *line)
{
   strutil_delwhitespace(*line);
   if(line->Length() >= 3 && line->Left(3) == _T("nil"))
   {
      *line = line->Right(line->Length()-3);
      return true;
   }
   return false;
}

String const
BbdbEntry::ReadString(String * line, bool *success)
{
   bool isnumber = false;
   if(success) *success = false;

   if(! ReadToken('"', line))
   {
      // numbers are treated as strings, but have no quotes
      if(line[0u]>=_T('0') && line[0u] <= _T('9'))
         isnumber = true;
      else
      {
         if(success)
            *success = ReadNil(line);
         else
            ReadNil(line);
         return _T("");
      }
   }

   strutil_delwhitespace(*line);
   String str = _T("");
   const wxChar *cptr = line->c_str();
   bool escaped = false;

   while(*cptr)
   {
      if(! isnumber && (*cptr == '"' && !escaped))
         break;
      if(isnumber && (*cptr < '0' || *cptr > '9'))
         break;
      if(*cptr == '\\' && ! escaped)
         escaped = true;
      else
      {
         str += *cptr;
         escaped = false;
      }
      cptr++;
   }
   if(!isnumber && *cptr == '"')
      cptr++;
   *line = cptr;
   if(success) *success = true;
   return str;
}

void
BbdbEntry::WriteString(ostream &out, String const &string)
{
   const wxChar *cptr;

   if(string.empty())
   {
      out << "nil";
      return;
   }
   out << '"';
   cptr = string.c_str();
   while(*cptr)
   {
      if(*cptr == '"')
         out << '\\';
      out << *cptr;
      cptr++;
   }
   out << "\" ";
}

kbStringList *
BbdbEntry::ReadListOfStrings(String *string, bool *mysuccess)
{
   kbStringList *list = new kbStringList;
   String        str;
   bool success;
   if(mysuccess) *mysuccess = false;

   if(! ReadToken('(', string))
   {
      str = ReadString(string, &success); // single string
      if(success)
         list->push_back(new String(str));
      else
         success = ReadNil(string);
      if(mysuccess) *mysuccess = success;
      return list;  // empty
   }

   strutil_delwhitespace(*string);

   for(;;)
   {
      if(ReadToken(')',string))
         break;
      str = ReadString(string, &success);
      if(success)
         list->push_back(new String(str));
      else
         break;
   }
   if(mysuccess) *mysuccess = success;
   return list;
}

kbStringListListList *
BbdbEntry::ReadListOfVectors(String *string)
{
   kbStringListListList *vlist = new kbStringListListList;
   kbStringListList     *slist;

   String        str;

   if(! ReadToken('(', string))
   {
      ReadNil(string);
      return vlist;
   }

   do
   {
      slist = ReadVector(string);
      vlist->push_back(slist);
   }
   while(!slist->empty());

   if(! ReadToken(')', string))
      wxLogWarning(_("Bbdb::ReadListOfVectors expected ')', found '%s'"), string->c_str());

   return vlist;
}

kbStringListList *
BbdbEntry::ReadVector(String *string)
{
   kbStringListList * ll = new kbStringListList;
   kbStringList *l;
   bool success;

   if(! ReadToken('[', string))
      return ll;
   do
   {
      if(ReadToken(']', string))
         break;
      ll->push_back(l = ReadListOfStrings(string, &success));
   }while(success);
   return ll;
}


BbdbEntry *
BbdbEntry::ParseLine(BbdbEntryGroup *pGroup, String * line)

{
   if(m_IgnoreAnonymous == -1) // we need to initialise some things
   {
      // these values are cached for later use
      m_IgnoreAnonymous = READ_APPCONFIG(MP_BBDB_IGNOREANONYMOUS);
      m_AnonymousName = READ_APPCONFIG(MP_BBDB_ANONYMOUS).GetTextValue();
      m_EnforceUnique = READ_APPCONFIG(MP_BBDB_GENERATEUNIQUENAMES);
   }

   // line must start with '['
   if(! ReadToken('[', line))
      return NULL;

   static int count = 0;
   String tmp;
   String first_name = ReadString(line);
   String last_name = ReadString(line);
   AdbEntry *e_exists;
   String alias, temp;

   if(first_name.empty() && last_name.empty())
   {
      if(m_IgnoreAnonymous)
         return NULL;
      else
         alias = m_AnonymousName;
   }
   else
      alias << first_name << _T("_") << last_name;

   if(m_EnforceUnique)
   {
      temp = alias;
      while((e_exists = pGroup->GetEntry(alias)) != NULL) // duplicate entry
      {
         e_exists->DecRef(); // GetEntry() does an IncRef()
         tmp.Printf(_T("%d"), count);
         alias = temp;
         alias << _T("_") << tmp;
         count++;
      }
   }

   BbdbEntry *e = new BbdbEntry(pGroup);
   e->m_astrFields.Add(alias);
   e->SetField(AdbField_FirstName, first_name);
   e->SetField(AdbField_FamilyName, last_name);

   delete  ReadListOfStrings(line); // kbStringList

   e->SetField(AdbField_Organization, ReadString(line));

   kbStringListListList *phonelist = e->ReadListOfVectors(line);
   {
      // each vector contains of one-element lists of numbers
      kbStringListListList::iterator i;
      kbStringListList *ll;
      kbStringListList::iterator j;
      kbStringList::iterator k;
      String str;
      int count;
      for(count = 0, i = phonelist->begin(); i != phonelist->end() && count < 2;
          i++, count++)
      {
         ll = *i; // each phone number has a list of strings for itself
         str = _T("");
         j = ll->begin(); j++; // skip description
         for(; j != ll->end(); j++)
         {
            k = (**j).begin(); // the entries are lists with one string each
            if(k != (**j).end())
               str << **k << ' ';
         }
         str = str.Left(str.Length()-1);
         e->SetField(count == 0 ? AdbField_H_Phone : AdbField_O_Phone, str);
      }
   }
   delete phonelist;

#define ADDRFIELD(x) (field + (AdbField_H_##x - AdbField_H_AddrPageFirst))
   kbStringListListList *addresses = e->ReadListOfVectors(line);
   {
      // each vector contains of one-element lists of numbers
      kbStringListListList::iterator i;
      kbStringListList *ll;
      kbStringListList::iterator j;
      kbStringList::iterator k;
      String str;
      int count, count2;
      size_t field;
      for(count = 0, i = addresses->begin(); i != addresses->end() && count < 2;
          i++, count++)
      {
         if(count == 0)
            field = AdbField_H_AddrPageFirst;
         else
            field = AdbField_O_AddrPageFirst;

         ll = *i; // each address number has a list of strings for itself
         str = _T("");
         j = ll->begin(); j++; // skip description
         for(count2 = 1; j != ll->end(); j++,count2++)
         {
            k = (**j).begin(); // the entries are lists with one string each
            str = _T("");
            for(;k != (**j).end(); k++)
               str << **k << ' ';
            str = str.Left(str.Length()-1);
            switch(count2)
            {
            case 1: e->SetField(ADDRFIELD(POBox), str); break;
            case 2: e->SetField(ADDRFIELD(Street), str); break;
            case 3: e->SetField(ADDRFIELD(Locality), str); break;
            case 4: e->SetField(ADDRFIELD(City), str); break;
            case 5: e->SetField(ADDRFIELD(Country), str); break;
            case 6: e->SetField(ADDRFIELD(Postcode), str); break;
            default:
               // should never happen
               ;
            }
         }
      }
   }

   delete addresses;

   kbStringList *mail_addresses = e->ReadListOfStrings(line);
   kbStringList::iterator i = mail_addresses->begin();
   if(i != mail_addresses->end())
      e->SetField(AdbField_EMail, **i);
   i++;
   for(; i != mail_addresses->end(); i++)
      e->AddEMail(**i);
   delete mail_addresses;

   e->ClearDirty();
   return e;
}

AdbEntryGroup *
BbdbEntry::GetGroup() const
{
   MOcheck();
   return m_pGroup;
}

// ----------------------------------------------------------------------------
// BbdbEntryGroup
// ----------------------------------------------------------------------------

BbdbEntryGroup::BbdbEntryGroup(BbdbEntryGroup *, const String& strName)
{
   MOcheck();

   MBeginBusyCursor();

   m_entries = new BbdbEntryList(false);

   m_strName = strName; // there is only one group so far
   m_pParent = NULL;

   BbdbEntry *e;
   wxString line, version;
   int ignored = 0, entries_read = 0;
   ifstream file(strName.mb_str());
   int length = 0;

   file.seekg(0, ios::end);
   length = (int) (file.tellg() / 1024);
   file.seekg(0, ios::beg);

   // read the header info
   strutil_getstrline(file,line);
   if(! BbdbEntry::ReadHeader(&version, &line))
   {
      wxLogError(_("BBDB: file has wrong header line: '%s'"),
                 line.c_str());
      MEndBusyCursor();
      return;
   }
   else
   {
      LOGMESSAGE((M_LOG_WINONLY, _("BBDB: file format version '%s'"), version.c_str()));
   }

   MProgressDialog status_frame(_T("BBDB import"), _T("Importing..."),
                                 length, NULL);// open a status window:
   do
   {
      strutil_getstrline(file, line);
      if(file && ! file.fail() && ! file.eof())
      {
         e = BbdbEntry::ParseLine(this, &line);
         status_frame.Update((int)(file.tellg()/1024));
         if(e)
         {
            m_entries->push_back(e);
            entries_read ++;
         }
         else
            ignored ++;
      }
   }while(! (file.eof() || file.fail()));
   LOGMESSAGE((M_LOG_WINONLY, _("BBDB: read %d entries."), entries_read));
   if(ignored > 0)
      wxLogWarning(_("BBDB: ignored %d entries with neither first nor last names."),
                   ignored);
   MEndBusyCursor();
}

#define   SAVE_FIELD(x)  e->GetField(x, &str);BbdbEntry::WriteString(out, str);
#define   APPEND_FIELD(x, y)  e->GetField(x, &str);y << str;

BbdbEntryGroup::~BbdbEntryGroup()
{
   BbdbEntryList::iterator i;
   BbdbEntry *e;
   bool dirty = false;
   bool save;
   int saveonexit;

   for(i = m_entries->begin(); i != m_entries->end(); i++)
      if( (dirty = (**i).IsDirty()) == true)
         break;

   if(dirty)
   {
      saveonexit = READ_APPCONFIG(MP_BBDB_SAVEONEXIT);
      switch(saveonexit)
      {
      case M_ACTION_PROMPT:
      {
         String str;
         str.Printf(_("Save BBDB address book '%s'?\n"
                      "This might lead to loss of some of the original data."),
                    m_strName.c_str());
         save = MDialog_YesNoDialog(str,NULL,_("BBDB"),
                                    M_DLG_YES_DEFAULT,
                                    M_MSGBOX_BBDB_SAVE_DIALOG);
         break;
      }
      case M_ACTION_ALWAYS:
         save = true;
         break;
      default:
      case M_ACTION_NEVER:
         save = false;
      }
      if(save)
      {
         int length = 0, count = 0;
         for(i = m_entries->begin(); i != m_entries->end(); i++)
            length++;
         MProgressDialog status_frame(_T("BBDB"), _T("Saving..."),
                                      length, NULL);// open a status window:

         String str;
         ofstream out(m_strName.mb_str());
         size_t n,m;
         out << ";;; file-version: 2" << endl;
         for(i = m_entries->begin(); i != m_entries->end(); i++)
         {
            e = *i;
            out << '[';
            SAVE_FIELD(AdbField_FirstName); out << ' ';
            SAVE_FIELD(AdbField_FamilyName);out << ' ';
            out << "nil "; // AKA list
            SAVE_FIELD(AdbField_Organization); out << ' ';
//FIXME: different phone number format
#if 0
            int phone1, phone2, phone3, phone4;
            out << "([ \"home\" "; // phone numbers
            e->GetField(AdbField_H_Phone, &str);
            phone1 = phone2 = phone3 = phone4 = 0;
            //FIXME: do something more clever here!
            sscanf(str.c_str(), "%d %d %d %d", &phone1, &phone2,&phone3, &phone4);
            out << phone1 << ' ' << phone2 << ' ' << phone3 << ' '
                << phone4 << "] ";
            out << "[ \"work\" "; // phone numbers
            e->GetField(AdbField_O_Phone, &str);
            phone1 = phone2 = phone3 = phone4 = 0;
            //FIXME: do something more clever here!
            sscanf(str.c_str(), "%d %d %d %d", &phone1, &phone2,&phone3, &phone4);
            out << phone1 << ' ' << phone2 << ' ' << phone3 << ' '
                << phone4 << "]) ";
#endif
            out << "nil ";
            String home;
            home = _T("");
            APPEND_FIELD(AdbField_H_POBox, home);
            APPEND_FIELD(AdbField_H_Street, home);
            APPEND_FIELD(AdbField_H_Locality, home);
            APPEND_FIELD(AdbField_H_City, home);
            APPEND_FIELD(AdbField_H_Country, home);
            APPEND_FIELD(AdbField_O_POBox, home);
            APPEND_FIELD(AdbField_O_Street, home);
            APPEND_FIELD(AdbField_O_Locality, home);
            APPEND_FIELD(AdbField_O_City, home);
            APPEND_FIELD(AdbField_O_Country, home);
            if(!home.empty())
            {
               out << '(';
               out << "[ \"home\" "; // Home Address
               SAVE_FIELD(AdbField_H_POBox);   out << ' ';
               SAVE_FIELD(AdbField_H_Street);  out << ' ';
               SAVE_FIELD(AdbField_H_Locality);out << ' ';
               SAVE_FIELD(AdbField_H_City);    out << ' ';
               SAVE_FIELD(AdbField_H_Country); out << ' ';
               out << '(';
               SAVE_FIELD(AdbField_H_Postcode);out << ' ';
               out << ")]";
               out << "[ \"work\" ";
               SAVE_FIELD(AdbField_O_POBox);   out << ' ';
               SAVE_FIELD(AdbField_O_Street);  out << ' ';
               SAVE_FIELD(AdbField_O_Locality);out << ' ';
               SAVE_FIELD(AdbField_O_City);    out << ' ';
               SAVE_FIELD(AdbField_O_Country); out << ' ';
               out << '(';
               SAVE_FIELD(AdbField_O_Postcode);out << ' ';
               out << ")]";
               out << ")";
            }
            else
               out << "nil";
            out << " ("; // net addresses
            SAVE_FIELD(AdbField_EMail);    out << ' ';
            n = e->GetEMailCount();
            for(m = 0; m < n; m++)
            {
               e->GetEMail(m, &str);
               out << '"' << str << "\" ";
            }
            out << ") nil nil]" << endl;
         }
         status_frame.Update(++count);
      }
   }
   for(i = m_entries->begin(); i != m_entries->end(); i++)
      (**i).DecRef();
   delete m_entries;

   BbdbEntry::m_IgnoreAnonymous = -1; // re-read values on next opening
}

size_t
BbdbEntryGroup::GetEntryNames(wxArrayString& aNames) const
{
   MOcheck();

   aNames.Empty();
   BbdbEntryList::iterator i;
   for(i = m_entries->begin(); i != m_entries->end(); i++)
      aNames.Add((**i).GetName());
   return aNames.Count();
}

size_t
BbdbEntryGroup::GetGroupNames(wxArrayString& aNames) const
{
   MOcheck();

   aNames.Empty();
   return aNames.Count();
}

AdbEntry *
BbdbEntryGroup::GetEntry(const String& name) const
{
   MOcheck();

   BbdbEntryList::iterator i;

//   wxLogDebug(_T("BbdbEntryGroup::GetEntry() called with: %s"), name.c_str());
   for(i = m_entries->begin(); i != m_entries->end(); i++)
   {
      (**i).MOcheck();
      if((**i).GetName() == name)
      {
         (**i).IncRef();
         return *i;
      }
   }
   return NULL;
}

bool
BbdbEntryGroup::Exists(const String& path) const
{
   MOcheck();
   return GetEntry(path) != NULL;
}

AdbEntryGroup *BbdbEntryGroup::GetGroup(const String& name) const
{
   MOcheck();
//   wxLogDebug(_T("BbdbEntryGroup::GetGroup() called with: %s"), name.c_str());
   return NULL;
}

AdbEntry *
BbdbEntryGroup::CreateEntry(const String& strName)
{
   MOcheck();
   BbdbEntry *e = new BbdbEntry(this, strName);
   return  e;
}

AdbEntryGroup *BbdbEntryGroup::CreateGroup(const String& strName)
{
   MOcheck();
   return NULL;
}

void
BbdbEntryGroup::DeleteEntry(const String& strName)
{
   MOcheck();
   BbdbEntryList::iterator i;
   for(i = m_entries->begin(); i != m_entries->end(); i++)
   {
      if((**i).GetName() == strName)
      {
         (**i).DecRef();
         m_entries->erase(i);
         return;
      }
   }
}

void
BbdbEntryGroup::DeleteGroup(const String& strName)
{
   MOcheck();
   wxFAIL_MSG(_T("Not implemented"));
}

AdbEntry *
BbdbEntryGroup::FindEntry(const wxChar *szName) const
{
   MOcheck();
//   wxLogDebug(_T("BbdbEntryGroup::FindEntry() called with: %s"), szName);
   return NULL;
}

// ----------------------------------------------------------------------------
// BbdbBook
// ----------------------------------------------------------------------------

BbdbBook::BbdbBook(const String& name)
{
   MOcheck();

   m_strFileName = name;

   wxSplitPath(m_strFileName, NULL, &m_strName, NULL);

   m_strDesc << m_strName << _(" (Emacs BBDB addressbook)");

   // create the root group
   m_pRootGroup = new BbdbEntryGroup(NULL, name); // there is only one group
}

BbdbBook::~BbdbBook()
{
   MOcheck();
   SafeDecRef(m_pRootGroup);
}

bool
BbdbBook::IsSameAs(const String& name) const
{
   return sysutil_compare_filenames(m_strFileName, name);
}

String
BbdbBook::GetFileName() const
{
   MOcheck();
   return m_strFileName;
}

void
BbdbBook::SetName(const String& strName)
{
   MOcheck();
   m_strName = strName;
}

String BbdbBook::GetName() const
{
   MOcheck();
   return m_strName;
}

void
BbdbBook::SetDescription(const String& strDesc)
{
   MOcheck();
   m_strDesc = strDesc;
}

String
BbdbBook::GetDescription() const
{
   MOcheck();
   return m_strDesc;
}

size_t
BbdbBook::GetNumberOfEntries() const
{
   MOcheck();
   return 1;
}

bool
BbdbBook::IsReadOnly() const
{
   MOcheck();
   return !wxFile::Access(m_strFileName, wxFile::write);
}

// ----------------------------------------------------------------------------
// BbdbDataProvider
// ----------------------------------------------------------------------------

AdbBook *
BbdbDataProvider::CreateBook(const String& name)
{
   return new BbdbBook(name);
}

bool
BbdbDataProvider::EnumBooks(wxArrayString& aNames)
{
   return FALSE;
}

bool
BbdbDataProvider::TestBookAccess(const String& name, AdbTests test)
{
   switch ( test )
   {
      case Test_Create:
      case Test_OpenReadOnly:
      case Test_Open:
         // FIXME: Different code for Test_OpenReadOnly, Test_Open,
         // and Test_Create
         if(wxFileExists(name))
         {
            ifstream file(name.mb_str());
            String line;
            strutil_getstrline(file, line);
            return BbdbEntry::ReadHeader(NULL, &line);
         }
         return false;
      case Test_AutodetectCapable:
         return true;
      case Test_RecognizesName:
         return false;
   }
   return false;
}

bool
BbdbDataProvider::DeleteBook(AdbBook *book)
{
   return FALSE;
}
