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

#include "adb/AdbEntry.h"
#include "adb/AdbBook.h"
#include "adb/AdbDataProvider.h"

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// fwd decl
class LineEntry;

WX_DEFINE_SORTED_ARRAY(LineEntry *,LineEntryArray);

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
   virtual AdbEntry *GetEntry(const String& name) const;

   virtual bool Exists(const String& path) const;

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

   virtual AdbEntry *FindEntry(const wxChar *name) const;

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

   virtual size_t GetNumberOfEntries() const { return m_entries.GetCount(); }

   virtual bool IsLocal() const { return true; }
   virtual bool IsReadOnly() const;
   
   virtual bool Flush();
   
   int IndexByName(const String &name) const;
   LineEntry *FindLineEntry(const String& name) const;
   
   bool IsBad() const { return m_bad; }
   
   bool IsDirty() const;
   void ClearDirty();

   bool ReadFromStream(istream &stream);
   
private:
   virtual ~LineBook();
   
   String m_file;
   LineEntryArray m_entries;
   bool m_bad;
   bool m_dirty;

   GCC_DTOR_WARN_OFF
};

// our AdbEntryData implementation
class LineEntry : public AdbEntry
{
public:
   // ctor
   LineEntry(LineBook *parent,const String& name);

   // implement interface methods

   // AdbEntry
   virtual AdbEntryGroup *GetGroup() const { return m_book; }

   virtual void GetFieldInternal(size_t n, String *pstr) const
      { GetField(n, pstr); }
   virtual void GetField(size_t n, String *pstr) const;

   virtual size_t GetEMailCount() const { return 0; }
   virtual void GetEMail(size_t, String *) const
      { FAIL_MSG( _T("LineEntry::GetEMail was called.") ); }

   virtual void ClearDirty() { m_dirty = false; }
   virtual bool IsDirty() const { return m_dirty; }

   virtual void SetField(size_t n, const String& strValue);
   virtual void AddEMail(const String&)
   {
      FAIL_MSG( _T("Nobody asked LineBook whether it supports AddEMail.") );
   }
   virtual void ClearExtraEMails() { }

   virtual int Matches(const wxChar *str, int where, int how) const;

   int Compare(LineEntry *right) { return m_name.compare(right->m_name); }
   const String& GetName() const { return m_name; }
   
   static String StripSpace(const String &address);

private:
   LineBook *m_book;
   String m_name;
   String m_address;
   bool m_dirty;
};

ostream& operator << (ostream& out, const LineEntry& entry);

// our AdbDataProvider implementation
class LineDataProvider : public AdbDataProvider
{
public:
   // implement interface methods
   virtual AdbBook *CreateBook(const String& name);
   virtual bool EnumBooks(wxArrayString&) { return false; }
   virtual bool DeleteBook(AdbBook *book);
   virtual bool TestBookAccess(const String& name, AdbTests test);

   DECLARE_ADB_PROVIDER(LineDataProvider);
};

IMPLEMENT_ADB_PROVIDER(LineDataProvider, true,
   _T("One Address per Line"), Name_File);

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// LineBook
// ----------------------------------------------------------------------------

static int LineEntryCompare(LineEntry *left, LineEntry *right)
{
   return left->Compare(right);
}

// Duplicate of FCBook::GetFullAdbPath. It should be shared somehow.
String LineBook::GetFullAdbPath(const String& filename)
{
   CHECK( !filename.empty(), _T(""), _T("ADB without name?") );
   
   String path;
   if ( wxIsAbsolutePath(filename) )
      path = filename;
   else
      path << mApplication->GetLocalDir() << DIR_SEPARATOR << filename;
   
   return path;
}

LineBook::LineBook(const String& file)
   : m_file(GetFullAdbPath(file)), m_entries(LineEntryCompare)
{
   m_bad = false;
   m_dirty = false;
   
   ifstream stream(m_file.fn_str());
   if ( !stream.good() )
   {
      ofstream create(m_file.fn_str(), ios::out|ios::ate);
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
   wxLogError(_("Cannot open file %s."), m_file.c_str());
   m_bad = true;
}

LineBook::~LineBook()
{
   Flush();
   
   for ( size_t eachEntry = m_entries.GetCount(); eachEntry > 0;
      --eachEntry )
   {
      LineEntry *deleted = m_entries.Item(eachEntry-1);
      m_entries.RemoveAt(eachEntry-1);
      deleted->DecRef();
   }
}

AdbEntry *LineBook::GetEntry(const String& name) const
{
   LineEntry *found = FindLineEntry(name);
   CHECK( found != NULL, NULL, _T("Asked for non-existent entry.") );
   
   return found;
}

bool LineBook::Exists(const String& path) const
{
   return IndexByName(path) != wxNOT_FOUND;
}

size_t LineBook::GetEntryNames(wxArrayString& names) const
{
   names.Empty();
   
   for ( size_t eachEntry = 0; eachEntry < m_entries.GetCount();
      ++eachEntry )
   {
      names.Add(m_entries.Item(eachEntry)->GetName());
   }
   
   return names.GetCount();
}

AdbEntry *LineBook::CreateEntry(const String& name)
{
   CHECK( IndexByName(name) == wxNOT_FOUND, GetEntry(name),
      _T("Create entry with duplicate name.") );
   
   LineEntry *newEntry = new LineEntry(this,name);
   
   m_entries.Add(newEntry);
   m_dirty = true;
   
   newEntry->IncRef();
   return newEntry;
}

void LineBook::DeleteEntry(const String& name)
{
   int index = IndexByName(name);
   ASSERT( index != wxNOT_FOUND );
   
   LineEntry *reference = m_entries.Item(index);
   
   m_entries.RemoveAt(index);
   m_dirty = true;
   
   reference->DecRef();
}

AdbEntry *LineBook::FindEntry(const wxChar *name) const
{
   return FindLineEntry(name);
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
      String commit = wxFileName::CreateTempFileName(_T(""));
      
      ofstream stream(commit.fn_str());
      if ( !stream.good() )
         goto FileError;
      
      for ( size_t eachEntry = 0; eachEntry < m_entries.GetCount();
         ++eachEntry )
      {
         stream << *m_entries.Item(eachEntry);
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
   wxLogError(_("Cannot write to file %s."), m_file.c_str());
   return false;
}

int LineBook::IndexByName(const String& name) const
{
   LineEntry *key = new LineEntry(NULL,name);

#if wxCHECK_VERSION(2,5,0) // Bug in wxWindows 2.4 version of Index()
   int index = m_entries.Index(key);
#else
   int low = 0; // Key is at position greater or equal to "low"
   int high = m_entries.GetCount(); // Key is below "high"
   while ( low < high - 1 ) // There are items to compare
   {
      int middle = (low + high) / 2; // Candidate key match
      int compare = LineEntryCompare(key,m_entries.Item(middle));
      if ( compare < 0 ) // Key is below "middle"
         high = middle; // "high" changes
      else if ( compare > 0 ) // Key is above "middle"
         low = middle + 1; // "low" changes
      else // Key is at "middle", we can set "low" and "high" exactly
      {
         low = middle;
         high = middle + 1;
      }
   }
   
   int index;
   if ( low < high && LineEntryCompare(key,m_entries.Item(low)) == 0 )
      index = low;
   else
      index = wxNOT_FOUND;
#endif
   key->DecRef();
   
   return index;
}

LineEntry *LineBook::FindLineEntry(const String& name) const
{
   int index = IndexByName(name);
   if ( index == wxNOT_FOUND )
      return NULL;
   
   LineEntry *found = m_entries.Item(index);
   found->IncRef();
   
   return found;
}

bool LineBook::IsDirty() const
{
   bool anythingDirty = m_dirty;
   for ( size_t eachEntry = 0; eachEntry < m_entries.GetCount();
      ++eachEntry )
   {
      anythingDirty = anythingDirty || m_entries.Item(eachEntry)->IsDirty();
   }
   return anythingDirty;
}

void LineBook::ClearDirty()
{
   m_dirty = false;
   for ( size_t eachEntry = 0; eachEntry < m_entries.GetCount();
      ++eachEntry )
   {
      m_entries.Item(eachEntry)->ClearDirty();
   }
}

bool LineBook::ReadFromStream(istream &stream)
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
         AdbEntry *add = CreateEntry(nonspace);
         add->SetField(AdbField_EMail, nonspace);
         add->ClearDirty();
         add->DecRef();
      }
   }
   
   m_dirty = false;
   
   return true;
}

// ----------------------------------------------------------------------------
// LineEntry
// ----------------------------------------------------------------------------

LineEntry::LineEntry(LineBook *parent,const String& name)
{
   m_book = parent;
   m_name = name;
   m_dirty = false;
}

void LineEntry::GetField(size_t n, String *pstr) const
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

void LineEntry::SetField(size_t n, const String& strValue)
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

int LineEntry::Matches(const wxChar *what, int /* where */, int how) const
{
   wxString whatCopy(what);
   wxString addressCopy(m_address);
   
   if ( !(how & AdbLookup_CaseSensitive) )
   {
      whatCopy.MakeLower();
      addressCopy.MakeLower();
   }

   bool match;
   if ( how & AdbLookup_Substring )
   {
      match = addressCopy.find(whatCopy) != addressCopy.npos;
   }
   else if ( how & AdbLookup_StartsWith )
   {
      match = addressCopy.size() >= whatCopy.size()
         && addressCopy.compare(0, whatCopy.size(), whatCopy);
   }
   else
   {
      match = addressCopy == whatCopy;
   }
   
   return match ? AdbLookup_EMail : 0;
}

ostream& operator << (ostream& out, const LineEntry& entry)
{
   String address;
   entry.GetField(AdbField_EMail, &address);
   
   String nonspace = LineEntry::StripSpace(address);
   
   if ( nonspace.size() > 0 )
      out << nonspace << endl;
   
   return out;
}

String LineEntry::StripSpace(const String &address)
{
   String nonspace;
   
   for ( size_t character = 0; character < address.size(); ++character )
   {
      if ( !isspace(address[character]) )
         nonspace.append(1, address[character]);
   }

   return nonspace;
}

// ----------------------------------------------------------------------------
// LineDataProvider
// ----------------------------------------------------------------------------

AdbBook *LineDataProvider::CreateBook(const String& name)
{
   LineBook *book = new LineBook(name);
   if ( book->IsBad() )
   {
      book->DecRef();
      return NULL;
   }
   return book;
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
         FILE *fp = fopen(fullname.fn_str(), Test_Open ? "a" : "r");
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
