///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   adb/ImportMailrc.cpp - import ADB data from .mailrc file
// Purpose:     many Unix utilities lookup the address in the .mailrc file
//              usually found in the users home directory
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

   #include <wx/log.h>          // for wxLogWarning
#endif // USE_PCH

#include <wx/confbase.h>      // for wxExpandEnvVars
#include <wx/textfile.h>

#include "adb/AdbEntry.h"
#include "adb/AdbImport.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

static const size_t lenAlias = 5;   // strlen("alias")

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

class AdbMailrcImporter : public AdbImporter
{
public:
   AdbMailrcImporter() { }

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
   // parses one .mailrc line, returns FALSE if the syntax is invalid
   // (although most errors are just silently ignored). The nickname is always
   // extracted, but if the addresses pointer is NULL, the addresses are not
   // parsed.
   bool ParseMailrcAliasLine(const wxString& line,
                             wxString *nickname,
                             wxArrayString *addresses = NULL) const;

   // the indices of the alias line in m_textfile
   wxArrayInt m_lineNumbers;

   // the file we're working with
   wxTextFile m_textfile;

   // the first "interesting" (i.e. non blank, non comment) line in the file
   size_t m_nLineStart;

   DECLARE_NO_COPY_CLASS(AdbMailrcImporter)
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// macros for dynamic importer creation
// ----------------------------------------------------------------------------

IMPLEMENT_ADB_IMPORTER(AdbMailrcImporter,
                       gettext_noop("Unix .mailrc file import module"),
                       gettext_noop("Unix .mailrc file"),
                       _T("Vadim Zeitlin <vadim@wxwindows.org>"));


// ----------------------------------------------------------------------------
// helpers
// ----------------------------------------------------------------------------

bool AdbMailrcImporter::ParseMailrcAliasLine(const wxString& line,
                                             wxString *nickname,
                                             wxArrayString *addresses) const
{
   const wxChar *pc = line.c_str() + lenAlias;
   while ( isspace(*pc) )
      pc++;

   bool quoted = *pc == '"';
   if ( quoted )
   {
      // skip the quote
      pc++;
   }

   // first extract the nickname
   for ( bool cont = TRUE; cont; pc++ )
   {
      switch ( *pc )
      {
         case '"':
            if ( quoted )
            {
               // end of string
               cont = FALSE;

               break;
            }

            // fall through

         case '\0':
            // entries with unmatched quotes are ignored by mailx(1), so why
            // shouldn't we ignore them
            wxLogWarning(_("Invalid mailrc alias entry '%s' discarded."),
                         line.c_str());

            return FALSE;

         case '\\':
            // it quotes the next character
            *nickname += *++pc;
            break;

         case ' ':
            if ( !quoted )
            {
               // field delimiter
               cont = FALSE;

               break;
            }
            // fall through

         default:
            *nickname += *pc;
      }
   }

   // extract addresses too?
   if ( addresses )
   {
      while ( isspace(*pc) )
         pc++;

      addresses->Empty();

      // here everything is simple: mail addresses don't contain spaces nor
      // quotes
      wxString address;
      for ( ; ; pc++ )
      {
         if ( *pc == ' ' || !*pc )
         {
            addresses->Add(address);

            if ( !*pc )
            {
               // end of string
               break;
            }

            address.Empty();
         }
         else
         {
            address += *pc;
         }
      }

      if ( addresses->GetCount() == 0 )
      {
         wxLogWarning(_("Mailrc entry '%s' doesn't have any addresses and "
                        "will be ignored."), line.c_str());

         return FALSE;
      }
   }

   return TRUE;
}

// ----------------------------------------------------------------------------
// AdbImporter methods
// ----------------------------------------------------------------------------

String AdbMailrcImporter::GetDefaultFilename() const
{
   String location;

#ifdef OS_UNIX
   // the default location for Unix is $HOME/.mailrc
   location = wxExpandEnvVars(_T("$HOME/.mailrc"));

   if ( !wxFile::Exists(location) )
   {
      // nice try, but it's not there - so we don't know
      wxLogVerbose(_("Didn't find the mailrc address book in the default "
                     "location (%s)."), location.c_str());

      location.Empty();
   }
#else // !Unix
   // don't know where it can be (.mailrc only exists under Unix, so if the user
   // tries to import its ADB from other OS, it means that he transfered it
   // himself somewhere...)
#endif // Unix/!Unix

   return location;
}

bool AdbMailrcImporter::CanImport(const String& filename)
{
   // load the file into memory
   if ( !m_textfile.Open(filename) )
       return FALSE;

   // according to the mailrc docs I could find, the only allowed keywords in
   // mailrc file are alias, ignore, set and unset, so find the first non
   // comment line and check if it has this format
   size_t nLines = m_textfile.GetLineCount();
   for ( m_nLineStart = 0; m_nLineStart < nLines; m_nLineStart++ )
   {
      wxString line(m_textfile[m_nLineStart]);

      if ( !line || line[0u] == '#' )
      {
         // skip empty lines and comments
         continue;
      }

      line.Trim(FALSE /* from left */);

      if ( line.StartsWith(_T("alias ")) ||
           line.StartsWith(_T("ignore ")) ||
           line.StartsWith(_T("set ")) ||
           line.StartsWith(_T("unset ")) )
      {
         // good line, assume it's ok
         break;
      }

      // unexpected line
      return FALSE;
   }

   // do return TRUE even if we broke out of the loop because we exhausted all
   // lines - this allows us to import successfully even a default ~/.mailrc
   return TRUE;
}

bool AdbMailrcImporter::StartImport(const String& filename)
{
   if ( m_textfile.GetName() == filename )
   {
      // already have it
      return TRUE;
   }

   return CanImport(filename);
}

size_t
AdbMailrcImporter::GetEntryNames(const String& WXUNUSED_UNLESS_DEBUG(path),
                                 wxArrayString& entries) const
{
   ASSERT_MSG( !path, _T("where did this path come from?") );

   entries.Empty();

   wxArrayInt& lineNumbers = (wxArrayInt &)m_lineNumbers; // const_cast
   lineNumbers.Empty();

   size_t nLines = m_textfile.GetLineCount();
   for ( size_t nLine = m_nLineStart; nLine < nLines; nLine++ )
   {
      wxString line(m_textfile[nLine]);

      if ( !line || line[0u] == '#' )
      {
         // skip empty lines and comments
         continue;
      }

      line.Trim(FALSE /* from left */);

      if ( wxStrncmp(line, _T("alias"), lenAlias) != 0 )
      {
         // we're only interested in lines starting with "alias"
         continue;
      }

      wxString nickname;
      if ( ParseMailrcAliasLine(line, &nickname) )
      {
         lineNumbers.Add(nLine);
         entries.Add(nickname);
      }
      //else: just skip invalid entry
   }

   return entries.GetCount();
}

size_t AdbMailrcImporter::GetGroupNames(const String& path,
                                        wxArrayString& /* groups */) const
{
   if ( !path.empty() )
   {
       FAIL_MSG( _T("where did this path come from?") );
   }

   return 0;
}

bool AdbMailrcImporter::ImportEntry(const String& /* path */,
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

   wxString nickname;
   wxArrayString addresses;
   if ( !ParseMailrcAliasLine(line, &nickname, &addresses) )
   {
      return FALSE;
   }

#ifdef DEBUG
   wxString nicknameReal;
   entry->GetField(AdbField_NickName, &nicknameReal);
   ASSERT_MSG( nicknameReal == nickname, _T("importing wrong entry?") );
#endif // DEBUG

   // normally addresses shouldn't be empty, but better be safe...
   size_t nAddresses = addresses.GetCount();
   entry->SetField(AdbField_EMail, nAddresses == 0 ? nickname
                                                   : addresses[0]);

   for ( size_t nAddress = 1; nAddress < nAddresses; nAddress++ )
   {
      entry->AddEMail(addresses[nAddress]);
   }

   return TRUE;
}

