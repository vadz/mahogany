///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   adb/ImportXFMail.cpp - import ADB data from XFMail ADB
// Purpose:     XFMail stores its ADB in the files ~/.xfmail/.xfbook<NAME> in
//              a simple text format. In its current (1.3) version, it doesn't
//              support hierarchical ADB organisation.
// Author:      Vadim Zeitlin
// Modified by:
// Created:     07.07.99
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"

   #include <wx/log.h>          // for wxLogVerbose
#endif // USE_PCH

#include <wx/confbase.h>      // for wxExpandEnvVars
#include <wx/textfile.h>

#include "adb/AdbEntry.h"
#include "adb/AdbImport.h"

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

class AdbXFMailImporter : public AdbImporter
{
public:
   AdbXFMailImporter() { }

   // implement base class pure virtuals
   virtual String GetDefaultFilename() const;
   virtual bool CanImport(const String& filename);
   virtual bool StartImport(const String& filename);
   virtual size_t GetEntryNames(const String& path,
                                wxArrayString& entries) const;
   virtual size_t GetGroupNames(const String& path,
                                wxArrayString& groups) const;
   virtual bool ImportEntry(const String& path,
                            size_t index,
                            AdbEntry *entry);

   DECLARE_ADB_IMPORTER();

protected:
   wxArrayInt m_lineNumbers;
   wxTextFile m_textfile;

   DECLARE_NO_COPY_CLASS(AdbXFMailImporter)
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// macros for dynamic importer creation
// ----------------------------------------------------------------------------

IMPLEMENT_ADB_IMPORTER(AdbXFMailImporter,
                       gettext_noop("XFMail address book import module"),
                       gettext_noop("XFMail address book"),
                       "Vadim Zeitlin <vadim@wxwindows.org>");

// ----------------------------------------------------------------------------
// AdbXFMailImporter
// ----------------------------------------------------------------------------

String AdbXFMailImporter::GetDefaultFilename() const
{
   String location;

#ifdef OS_UNIX
   // the default location for Unix is $HOME/.xfmail/.xfbook
   location = wxExpandEnvVars("$HOME/.xfmail/.xfbook");

   if ( !wxFile::Exists(location) )
   {
      // nice try, but it's not there - so we don't know
      wxLogVerbose(_("Didn't find the XFMail address book in the default "
                     "location (%s)."), location.c_str());

      location.Empty();
   }
#else // !Unix
   // don't know where it can be (XFMail only runs under Unix, so if the user
   // tries to import its ADB from other OS, it means that he transfered it
   // himself somewhere...)
#endif // Unix/!Unix

   return location;
}

bool AdbXFMailImporter::CanImport(const String& filename)
{
   // all is in the name - AFAIK, XFMail always stores its ADB data in the
   // files named .xfbook.<NAME> (and .xfbook for the default ADB)
   wxString name = filename.AfterLast('/');

   return strncmp(name, ".xfbook", 7) == 0;
}

bool AdbXFMailImporter::StartImport(const String& filename)
{
   if ( filename == m_textfile.GetName() )
   {
      // already loaded
      return TRUE;
   }

   if ( !CanImport(filename) )
   {
      // don't even try
      return FALSE;
   }

   // so that ImportEntry() won't try to use old, invalid data if it's
   // (erroneously) called before GetEntryNames()
   m_lineNumbers.Empty();

   // load the file into memory
   return m_textfile.Open(filename);
}

size_t
AdbXFMailImporter::GetEntryNames(const String& WXUNUSED_UNLESS_DEBUG(path),
                                 wxArrayString& entries) const
{
   ASSERT_MSG( !path, _T("where did this path come from?") );

   entries.Empty();

   wxArrayInt& lineNumbers = (wxArrayInt &)m_lineNumbers; // const_cast
   lineNumbers.Empty();

   bool newEntry = FALSE;
   size_t nLines = m_textfile.GetLineCount();
   for ( size_t nLine = 0; nLine < nLines; nLine++ )
   {
      wxString line(m_textfile[nLine]);

      if ( !line )
      {
         continue;
      }

      if ( line[0u] == '@' )
      {
         // this is the entry separator
         newEntry = TRUE;

         continue;
      }

      if ( newEntry )
      {
         // the format is ' nickname <address>' and the spaces are *not*
         // escaped
         wxString nickname;
         const char *pc = line;

         // skip the spaces in the start of line
         while ( isspace(*pc) )
            pc++;

         // even though nothing prevents XFMail ADB entries from having
         // multiple '<' in them (except the RFC 822), we stop at the first
         // one
         while ( *pc && *pc != '<' )
         {
            nickname += *pc++;
         }

         // may be we took some extra spaces...
         nickname.Trim();

         // remember the line number for ImportEntry()
         lineNumbers.Add(nLine);

         // put the entry name into output array
         entries.Add(nickname);

         // if there are any additional lines, we will ignore them
         newEntry = FALSE;
      }
      //else: some unknown line, silently ignore it
   }

   return entries.GetCount();
}

size_t
AdbXFMailImporter::GetGroupNames(const String& WXUNUSED_UNLESS_DEBUG(path),
                                 wxArrayString& /* groups */) const
{
   ASSERT_MSG( !path, _T("where did this path come from?") );

   // no groups
   return 0;
}

bool AdbXFMailImporter::ImportEntry(const String& /* path */,
                                    size_t index,
                                    AdbEntry *entry)
{
   CHECK( index < m_lineNumbers.GetCount(), FALSE, _T("invalid entry index") );

   wxString line = m_textfile.GetLine((size_t)m_lineNumbers[index]);
   if ( !line )
   {
      // hmm... empty address book entry?
      return FALSE;
   }

   // start from the end and take as the email address everything between <>
   // or the whole string (may be there is no address at all)
   int nStart = line.Find('<', TRUE);
   size_t nLen = line.Len();
   if ( nStart == wxNOT_FOUND )
   {
      // well, take everything (nLen is already set)
      nStart = 0;
   }
   else
   {
      // skip '<'
      nStart++;
      nLen -= nStart;
      if ( line.Last() == '>' )
      {
         // don't take the closing '>'
         nLen--;
      }
   }

   wxString email(line.c_str() + (size_t)nStart, nLen);

   // XFMail stores only the nickname and the email address
   entry->SetField(AdbField_EMail, email);

   return TRUE;
}

