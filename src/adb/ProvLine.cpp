///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   adb/ProvLine.cpp: simple one address per line format
// Purpose:     One address per line format is easy to process by various
//              scripts and it is also easy to edit in text editor.
// Author:      Robert Vazan
// Modified by:
// Created:     14.08.2003
// CVS-ID:      $Id$
// Copyright:   (c) 2003 Robert Vazan <robertvazan@privateweb.sk>
// Licence:     M licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef  USE_PCH
#  include "Mcommon.h"
#  include "MApplication.h"
#  include "sysutil.h"
#  include "strutil.h"
#endif //USE_PCH

#include <wx/file.h>
#include <wx/hashmap.h>

#include "adb/AdbEntry.h"
#include "adb/AdbBook.h"
#include "adb/AdbDataProvider.h"

#include <fstream>

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// fwd decl
DECLARE_REF_COUNTER(LineBook)
DECLARE_REF_COUNTER(LineEntry)
DECLARE_REF_COUNTER(LineEntryData)

WX_DECLARE_STRING_HASH_MAP(RefCounter<LineEntryData>,LineEntryArray);

class LineBook : public AdbBook
{
public:
   // get the full file name - it's the same as the filename passed in for the
   // absolute pathes or the name with M local dir prepended for the other ones
   static String GetFullAdbPath(const String& filename);
   
   LineBook(const String& file);

   // implement interface methods
   // ---------------------------

   // AdbElement
   virtual AdbEntryGroup *GetGroup() const { return NULL; }

   // AdbEntryGroup
   virtual AdbEntry *GetEntry(const String& name);

   virtual bool Exists(const String& path);

   virtual size_t GetEntryNames(wxArrayString& names) const;
   virtual size_t GetGroupNames(wxArrayString& names) const
      { names.Empty(); return 0; }

   virtual AdbEntryGroup *GetGroup(const String&) const
      { FAIL_MSG( _T("LineBook::GetGroup was called.") ); return NULL; }

   virtual AdbEntry *CreateEntry(const String& name);
   virtual AdbEntryGroup *CreateGroup(const String&)
   {
      FAIL_MSG(
         _T("Nobody asked LineBook whether it supports CreateGroup.") );
      return NULL;
   }

   virtual void DeleteEntry(const String& name);
   virtual void DeleteGroup(const String&)
      { FAIL_MSG( _T("LineBook::DeleteGroup was called.") ); }

   virtual AdbEntry *FindEntry(const wxChar *name);

   // AdbBook
   virtual bool IsSameAs(const String& name) const;

   virtual String GetFileName() const { return m_file; }

   virtual void SetName(const String&)
   {
      FAIL_MSG( _T("Nobody asked LineBook whether it supports SetName.") );
   }
   virtual String GetName() const;

   virtual void SetDescription(const String&)
   {
      FAIL_MSG(
         _T("Nobody asked LineBook whether it supports SetDescription.") );
   }
   virtual String GetDescription() const { return m_file; }

   virtual size_t GetNumberOfEntries() const { return m_entries.size(); }

   virtual bool IsLocal() const { return true; }
   virtual bool IsReadOnly() const;
   
   virtual bool Flush();
   
   bool IsBad() const { return m_bad; }
   
   bool IsDirty() const;
   void ClearDirty();

   bool ReadFromStream(std::istream &stream);
   
private:
   virtual ~LineBook();
   
   String m_file;
   LineEntryArray m_entries;
   bool m_bad;
   bool m_dirty;

   GCC_DTOR_WARN_OFF
};

// Separated from LineEntry to avoid cyclic links with LineBook
class LineEntryData : public MObjectRC
{
public:
   LineEntryData() : m_dirty(false) {}
   
   void GetField(size_t n, String *pstr) const;

   void ClearDirty() { m_dirty = false; }
   bool IsDirty() const { return m_dirty; }

   void SetField(size_t n, const String& strValue);

   int Matches(const wxChar *str, int where, int how) const;

   WeakRef<LineEntry> m_handle;

private:
   String m_address;
   bool m_dirty;
};

std::ostream& operator << (std::ostream& out, const LineEntryData& entry);

// our AdbEntryData implementation
class LineEntry : public AdbEntry
{
public:
   // ctor
   LineEntry(RefCounter<LineBook> parent,RefCounter<LineEntryData> data);

   // implement interface methods

   // AdbEntry
   
   // Don't call IncRef
   virtual AdbEntryGroup *GetGroup() const { return m_book.get(); }

   virtual void GetFieldInternal(size_t n, String *pstr) const
      { GetField(n, pstr); }
   virtual void GetField(size_t n, String *pstr) const
      { m_data->GetField(n, pstr); }

   virtual size_t GetEMailCount() const { return 0; }
   virtual void GetEMail(size_t, String *) const
      { FAIL_MSG( _T("LineEntry::GetEMail was called.") ); }

   virtual void ClearDirty() { m_data->ClearDirty(); }
   virtual bool IsDirty() const { return m_data->IsDirty(); }

   virtual void SetField(size_t n, const String& strValue)
      { m_data->SetField(n, strValue); }

   virtual void AddEMail(const String&)
   {
      FAIL_MSG( _T("Nobody asked LineBook whether it supports AddEMail.") );
   }
   virtual void ClearExtraEMails() { }

   virtual int Matches(const wxChar *str, int where, int how) const
      { return m_data->Matches(str, where, how); }
   
   static String StripSpace(const String &address);

private:
   RefCounter<LineBook> m_book;
   RefCounter<LineEntryData> m_data;
};

// our AdbDataProvider implementation
class LineDataProvider : public AdbDataProvider
{
public:
   // implement interface methods
   virtual AdbBook *CreateBook(const String& name);
   virtual bool EnumBooks(wxArrayString&) { return false; }
   virtual bool DeleteBook(AdbBook *book);
   virtual bool TestBookAccess(const String& name, AdbTests test);

   virtual bool HasField(AdbField field) const
      { return field == AdbField_EMail; }
   virtual bool HasMultipleEMails() const { return false; }

   DECLARE_ADB_PROVIDER(LineDataProvider);
};

IMPLEMENT_ADB_PROVIDER(LineDataProvider, true,
   "One Address per Line", Name_File);

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// LineBook
// ----------------------------------------------------------------------------

DEFINE_REF_COUNTER(LineBook)

// Duplicate of FCBook::GetFullAdbPath. It should be shared somehow.
String LineBook::GetFullAdbPath(const String& filename)
{
   CHECK( !filename.empty(), wxEmptyString, _T("ADB without name?") );
   
   String path;
   if ( wxIsAbsolutePath(filename) )
      path = filename;
   else
      path << mApplication->GetLocalDir() << DIR_SEPARATOR << filename;
   
   return path;
}

LineBook::LineBook(const String& file)
   : m_file(GetFullAdbPath(file))
{
   m_bad = false;
   m_dirty = false;
   
   std::ifstream stream(m_file.fn_str());
   if ( !stream.good() )
   {
      std::ofstream create(m_file.fn_str(), std::ios::out|std::ios::ate);
      if ( !create.good() )
         goto FileError;
      create.close();
      if ( !create.good() )
         goto FileError;
   }
   else
   {
      if ( !ReadFromStream(stream) )
         goto FileError;
   }

   return;
FileError:
   wxLogError(_("Cannot open file %s."), m_file);
   m_bad = true;
}

LineBook::~LineBook()
{
   Flush();
}

AdbEntry *LineBook::GetEntry(const String& name)
{
   AdbEntry *found = FindEntry(name.c_str());
   CHECK( found != NULL, NULL, _T("Asked for non-existent entry.") );
   return found;
}

bool LineBook::Exists(const String& path)
{
   return m_entries.find(path) != m_entries.end();
}

size_t LineBook::GetEntryNames(wxArrayString& names) const
{
   names.Empty();
   
   for( LineEntryArray::const_iterator each = m_entries.begin();
      each != m_entries.end(); ++each )
   {
      names.Add(each->first);
   }
   
   return names.GetCount();
}

AdbEntry *LineBook::CreateEntry(const String& name)
{
   CHECK( !Exists(name), GetEntry(name),
      _T("Create entry with duplicate name.") );
   
   RefCounter<LineEntryData> data(new LineEntryData);
   RefCounter<LineEntry> entry(new LineEntry(
      RefCounter<LineBook>::convert(this),data));
   
   LineEntryArray::value_type pair(name,data);
   m_entries.insert(pair);

   m_dirty = true;
   
   return entry.release();
}

void LineBook::DeleteEntry(const String& name)
{
   LineEntryArray::iterator index = m_entries.find(name);
   CHECK_RET( index != m_entries.end(), _T("Deleted non-existent entry") );

   m_entries.erase(index);

   m_dirty = true;
}

AdbEntry *LineBook::FindEntry(const wxChar *name)
{
   LineEntryArray::iterator found = m_entries.find(name);
   if( found == m_entries.end() )
      return NULL;
      
   RefCounter<LineEntry> entry(found->second->m_handle);
   if( !entry )
   {
      entry.attach(new LineEntry(
            RefCounter<LineBook>::convert(this),found->second));
   }

   return entry.release();
}

bool LineBook::IsSameAs(const String& name) const
{
   String path = GetFullAdbPath(name);

   return sysutil_compare_filenames(m_file, path);
}

String LineBook::GetName() const
{
   wxFileName parser(m_file);
   
   return parser.GetFullName();
}

bool LineBook::IsReadOnly() const
{
  return !wxFile::Access(m_file, wxFile::write);
}

bool LineBook::Flush()
{
   if ( IsDirty() )
   {
      String commit = wxFileName::CreateTempFileName(wxEmptyString);
      
      std::ofstream stream(commit.fn_str());
      if ( !stream.good() )
         goto FileError;
      
      for( LineEntryArray::iterator each = m_entries.begin();
         each != m_entries.end(); ++each )
      {
         stream << *each->second;
         if ( !stream.good() )
            goto FileError;
      }
      
      stream.close();
      if ( !stream.good() )
         goto FileError;
         
      if ( rename(commit.fn_str(), m_file.fn_str()) )
         goto FileError;
         
      ClearDirty();
   }
   
   return true;
FileError:
   wxLogError(_("Cannot write to file %s."), m_file);
   return false;
}

bool LineBook::IsDirty() const
{
   bool anythingDirty = m_dirty;
   for( LineEntryArray::const_iterator each = m_entries.begin();
      each != m_entries.end(); ++each )
   {
      anythingDirty = anythingDirty || each->second->IsDirty();
   }
   return anythingDirty;
}

void LineBook::ClearDirty()
{
   m_dirty = false;
   for( LineEntryArray::iterator each = m_entries.begin();
      each != m_entries.end(); ++each )
   {
      each->second->ClearDirty();
   }
}

bool LineBook::ReadFromStream(std::istream &stream)
{
   for(;;)
   {
      String line;
      strutil_getstrline(stream, line);
      
      if ( stream.eof() )
         break;
      if ( !stream.good() )
         return false;
         
      String nonspace = LineEntry::StripSpace(line);
      if ( nonspace.size() > 0 && !Exists(nonspace) )
      {
         RefCounter<AdbEntry> add(CreateEntry(nonspace));
         add->SetField(AdbField_EMail, nonspace);
         add->ClearDirty();
      }
   }
   
   m_dirty = false;
   
   return true;
}

// ----------------------------------------------------------------------------
// LineEntry
// ----------------------------------------------------------------------------

DEFINE_REF_COUNTER(LineEntry)

LineEntry::LineEntry(RefCounter<LineBook> parent,
   RefCounter<LineEntryData> data)
   : m_book(parent), m_data(data)
{
   m_data->m_handle = RefCounter<LineEntry>::convert(this);
}

String LineEntry::StripSpace(const String &address)
{
   String nonspace;
   
   for ( size_t character = 0; character < address.size(); ++character )
   {
      if ( !wxIsspace(address[character]) )
         nonspace.append(1, address[character]);
   }

   return nonspace;
}

// ----------------------------------------------------------------------------
// LineEntryData
// ----------------------------------------------------------------------------

DEFINE_REF_COUNTER(LineEntryData)

void LineEntryData::GetField(size_t n, String *pstr) const
{
   switch ( n )
   {
      case AdbField_EMail:
         *pstr = m_address;
         break;

      default:
         pstr->clear();
   }
}

void LineEntryData::SetField(size_t n, const String& strValue)
{
   switch ( n )
   {
      case AdbField_EMail:
         m_address = strValue;
         m_dirty = true;
         break;
         
      default:
         FAIL_MSG(
            _T("Nobody asked LineBook whether it supports this field.") );
         break;
   }
}

int LineEntryData::Matches(const wxChar *what, int /*where*/, int how) const
{
   wxString whatCopy;
   wxString addressCopy(m_address);
   
   if ( how & AdbLookup_Substring )
      whatCopy << _T('*');
   whatCopy << what;
   if ( how & AdbLookup_Substring || how & AdbLookup_StartsWith )
      whatCopy << _T('*');

   if ( !(how & AdbLookup_CaseSensitive) )
   {
      whatCopy.MakeLower();
      addressCopy.MakeLower();
   }

   return addressCopy.Matches(whatCopy) ? AdbLookup_EMail : 0;
}

std::ostream& operator << (std::ostream& out, const LineEntryData& entry)
{
   String address;
   entry.GetField(AdbField_EMail, &address);
   
   String nonspace = LineEntry::StripSpace(address);
   
   if ( nonspace.size() > 0 )
      out << nonspace << std::endl;
   
   return out;
}

// ----------------------------------------------------------------------------
// LineDataProvider
// ----------------------------------------------------------------------------

AdbBook *LineDataProvider::CreateBook(const String& name)
{
   RefCounter<LineBook> book(new LineBook(name));
   CHECK ( !book->IsBad(), NULL, _T("Cannot create LineBook") );
   return book.release();
}

bool LineDataProvider::TestBookAccess(const String& name, AdbTests test)
{
   String fullname = LineBook::GetFullAdbPath(name);
   
   bool ok = false;
   
   switch ( test )
   {
      case Test_Open:
      case Test_OpenReadOnly:
      {
         FILE *fp = fopen(fullname.fn_str(), test == Test_Open ? "a" : "r");
         if ( fp != NULL )
         {
            fclose(fp);
            ok = true;
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
            ok = true;
            
            file.Close();
            wxRemove(fullname);
         }
      }
      break;

      case Test_AutodetectCapable:
      case Test_RecognizesName:
         ok = false;
         break;
      
      default:
         FAIL_MSG(_T("invalid test in TestBookAccess"));
   }
   
   return ok;
}

bool LineDataProvider::DeleteBook(AdbBook * /* book */)
{
   // TODO
   return false;
}
