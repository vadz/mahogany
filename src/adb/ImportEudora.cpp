///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   adb/ImportEudora.cpp - import Eudora address books
// Purpose:     Eudora is a MUA for Win16/32 which stores its ADBs in files
//              (2 files per ADB: adbname.toc and adbname.txt). The format is
//              undocumented, but, although .TOC files are binary, .TXT files
//              seem to have a quite simple format (see below).
// Author:      Vadim Zeitlin
// Modified by:
// Created:     10.07.99
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

/** Format description:

  1. File names.

  The main, default ADB is stored in files nndbase.toc/txt under Eudora
  installation directory. The other address books are stored in the
  subdirectory "Nickname": an address book "Foo" is stored in the files foo.toc
  (binary) and foo.txt (ASCII).

  The format is quasi-flat: there are no real subgroups, only support for
  different address books.

  2. TOC files.

  The format is unknown, but they all seem to start from "2A 00" and seem to
  consist from (variable length) entries which start from "entryname 0D 0A" for
  each entryname in the ADB.

  3. TXT files.

  Simple text format: each entry takes 2 lines (may be only 1 is possible),
  like this:

  alias Testnick testnick@testaddress
  note Testnick <fax:test fax><phone:test phone><address:test postal address><name:test name>test note

  The list of known "tags" (the words before ':' in the 2nd line above) is in
  the ParseTagValue function below. After all <tag:value> pairs follows the
  comment field.

  Quoting mechanism is unknown. (TODO: check)
 */

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"

   #include <wx/log.h>                  // for wxLogWarning
#endif // USE_PCH

#include <wx/textfile.h>

#include "adb/AdbEntry.h"
#include "adb/AdbImport.h"

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

class AdbEudoraImporter : public AdbImporter
{
public:
   AdbEudoraImporter() { }

   // implement base class pure virtuals
   virtual bool CanImport(const String& filename);
   virtual bool StartImport(const String& filename);
   virtual size_t GetEntryNames(const String& path,
                                wxArrayString& entries) const;
   virtual size_t GetGroupNames(const String& path,
                                wxArrayString& groups) const;
   virtual bool ImportEntry(const String& path,
                            size_t index,
                            AdbEntry *entry);
   virtual String GetDefaultFilename() const;

   DECLARE_ADB_IMPORTER();

protected:
   // parse one "<tag:value>" pair of Eudora ADB
   bool ParseTagValue(const char **ppc, AdbEntry *entry) const;

   // parses one Eudora ADB entry which starts in the line "index" and stores
   // the data in nickname and, if it's not NULL, in entry.
   bool ParseEudoraAdbEntry(size_t index,
                            wxString *nickname,
                            AdbEntry *entry = NULL) const;

   wxArrayInt m_lineNumbers;
   wxTextFile m_textfile;

   DECLARE_NO_COPY_CLASS(AdbEudoraImporter)
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// macros for dynamic importer creation
// ----------------------------------------------------------------------------

IMPLEMENT_ADB_IMPORTER(AdbEudoraImporter,
                       gettext_noop("Eudora address book importer"),
                       gettext_noop("Eudora for Windows address book"),
                       "Vadim Zeitlin <vadim@wxwindows.org>");


// ----------------------------------------------------------------------------
// helpers
// ----------------------------------------------------------------------------

bool AdbEudoraImporter::ParseTagValue(const char **ppc,
                                      AdbEntry *entry) const
{
   const char *pc = *ppc;

   ASSERT_MSG( *pc == '<', _T("should be at the start of the entry!") );

   // skip '<'
   pc++;

   // extract tag
   wxString tag;
   while ( *pc && *pc != ':' )
      tag += *pc++;

   if ( !*pc )
   {
      return FALSE;
   }

   // skip ':'
   pc++;

   // extract value
   wxString value;
   while ( *pc && *pc != '>' )
      value += *pc++;

   if ( !*pc )
   {
      return FALSE;
   }

   // skip '>'
   pc++;

   // is this tag known?
   AdbField field = AdbField_Max;
   tag.MakeLower();
   if ( tag == "name" )
      field = AdbField_FullName;
   else if ( tag == "phone" )
      field = AdbField_H_Phone;
   else if ( tag == "fax" )
      field = AdbField_H_Fax;
   else if ( tag == "address" )
      field = AdbField_H_Locality;

   if ( field != AdbField_Max )
   {
      entry->SetField(field, value);
   }
   else
   {
      wxLogWarning(_("Unknown tag '%s' in Eudora address book file ignored."),
                   tag.c_str());
   }

   *ppc = pc;

   return TRUE;
}

bool AdbEudoraImporter::ParseEudoraAdbEntry(size_t nLine,
                                            wxString *nickname,
                                            AdbEntry *entry) const
{
   static const size_t lenAlias = 5;   // strlen("alias")
   static const size_t lenNote = 4;   // strlen("note")

   wxString line = m_textfile[nLine];

   if ( strncmp(line, "alias ", lenAlias) != 0 )
   {
      // doesn't seem like the good starting line
      return FALSE;
   }

   // skip spaces (normally only one, but be careful)
   const char *pc = line.c_str() + lenAlias;
   while( wxIsspace(*pc) )
      pc++;

   // next goes the nickname
   nickname->Empty();
   while ( *pc && !wxIsspace(*pc) )
      *nickname += *pc++;

   // parse all other data only on request
   if ( entry )
   {
      // next is the address (don't know whether multiple addresses are
      // allowed)
      while ( wxIsspace(*pc) )
         pc++;

      wxString address;
      while ( *pc && !wxIsspace(*pc) )
         address += *pc++;

      entry->SetField(AdbField_EMail, address);

      // all other info is in the next line
      if ( ++nLine < m_textfile.GetLineCount() )
      {
         line = m_textfile[nLine];

         if ( strncmp(line, "note", lenNote) == 0 )
         {
            pc = line.c_str() + lenNote;
            while( wxIsspace(*pc) )
               pc++;

            // next goes the nickname (again)
            wxString nick2;
            while ( *pc && !wxIsspace(*pc) )
               nick2 += *pc++;

            if ( nick2 == *nickname )
            {
               while ( wxIsspace(*pc) )
                  pc++;

               // now parse all these <tag:value> pairs
               while ( *pc == '<' )
               {
                  if ( !ParseTagValue(&pc, entry) )
                  {
                     wxLogWarning(_("Incorrect entry at line %d in the Eudora "
                                    "address book file '%s' ignored."),
                                    nLine,
                                    m_textfile.GetName());

                     // otherwise we might find ourself in an infinite loop
                     break;
                  }
               }

               // what's left is the comment
               entry->SetField(AdbField_Comments, pc);
            }
            //else: something weird - note for another alias?
         }
         //else: doesn't look like continuation of the entry
      }
      // else: it was the last line
   }

   return TRUE;
}

// ----------------------------------------------------------------------------
// AdbImporter methods
// ----------------------------------------------------------------------------

bool AdbEudoraImporter::CanImport(const String& filename)
{
   wxString path, name, ext;
   wxSplitPath(filename, &path, &name, &ext);

   if ( ext != "txt" )
   {
      // Eudora ADB files also have this extension, apparently
      return FALSE;
   }

   // check for existence of the .toc file nearby
   wxLogNull noLog;

   wxString fileTocName;
   fileTocName << path << '/' << name << ".toc";
   wxFile fileToc(fileTocName);
   if ( !fileToc.IsOpened() )
   {
      // no such file?
      return FALSE;
   }

   char sig[2];
   if ( fileToc.Read(sig, 2) != 2 )
   {
      // can't read?
      return FALSE;
   }

   // check signature
   return (sig[0] == 0x2A) && (sig[1] == 0x00);
}

bool AdbEudoraImporter::StartImport(const String& filename)
{
   if ( m_textfile.GetName() == filename )
   {
      // already have it
      return TRUE;
   }

   if ( !CanImport(filename) )
   {
      // don't even try
      return FALSE;
   }

   // load the file into memory
   return m_textfile.Open(filename);
}

size_t
AdbEudoraImporter::GetEntryNames(const String& WXUNUSED_UNLESS_DEBUG(path),
                                 wxArrayString& entries) const
{
   ASSERT_MSG( !path, _T("where did this path come from?") );

   entries.Empty();

   wxArrayInt& lineNumbers = (wxArrayInt &)m_lineNumbers; // const_cast
   lineNumbers.Empty();

   size_t nLines = m_textfile.GetLineCount();
   for ( size_t nLine = 0; nLine < nLines; nLine++ )
   {
      wxString nickname;
      if ( ParseEudoraAdbEntry(nLine, &nickname) )
      {
         entries.Add(nickname);
         lineNumbers.Add(nLine);
      }
      //else: something unknown
   }

   return entries.GetCount();
}

// TODO: we might fetch the other address books when asked for the groups at
//       top level
size_t AdbEudoraImporter::GetGroupNames(const String& path,
                                        wxArrayString& /* groups */) const
{
   if ( !path.empty() )
   {
       FAIL_MSG( _T("where did this path come from?") );
   }

   return 0;
}

bool AdbEudoraImporter::ImportEntry(const String& /* path */,
                                    size_t index,
                                    AdbEntry *entry)
{
   CHECK( index < m_lineNumbers.GetCount(), FALSE, _T("invalid entry index") );

   wxString nickname;
   if ( !ParseEudoraAdbEntry((size_t)m_lineNumbers[index], &nickname, entry) )
   {
      return FALSE;
   }

#ifdef DEBUG
   wxString nicknameReal;
   entry->GetField(AdbField_NickName, &nicknameReal);
   ASSERT_MSG( nicknameReal == nickname, _T("importing wrong entry?") );
#endif // DEBUG

   return TRUE;
}

String AdbEudoraImporter::GetDefaultFilename() const
{
   // TODO: get the installation directory from the registry...
   return "";
}

