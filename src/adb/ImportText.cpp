///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   adb/ImportText.cpp - import ADB data from text (CSV/TAB) files
// Purpose:     this module defines AdbImporters for text files containing
//              comma (CSV) or TAB separated (TAB/TXT) fields. Such files may
//              be produced by "Export" feature of other e-mail clients (e.g.,
//              netscape)
// Author:      Vadim Zeitlin
// Modified by:
// Created:     05.07.99
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

/*
 * NB: although this code aims to be as generic as possible, so far it has only
 *     been tested with the CSV and TAB files produced by Netscape Communicator
 *     4.5 and will probably not work with others programs files. In
 *     particular, "mailing lists" are almost sure special to Netscape.
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

   #include <wx/log.h>
   #include <wx/dynarray.h>
#endif // USE_PCH

#include <wx/textfile.h>
#include <wx/utils.h>

#include "adb/AdbEntry.h"
#include "adb/AdbImport.h"

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// the base class for importing ADB data in text format
class AdbTextImporter : public AdbImporter
{
public:
   AdbTextImporter();

   // implement base class pure virtuals
   virtual String GetDefaultFilename() const { return ""; }
   virtual bool CanImport(const String& filename);
   virtual bool StartImport(const String& filename);
   virtual size_t GetEntryNames(const String& path,
                                wxArrayString& entries) const;
   virtual size_t GetGroupNames(const String& path,
                                wxArrayString& groups) const;
   virtual bool ImportEntry(const String& path,
                            size_t index,
                            AdbEntry *entry);

protected:
   // split one field from the line, modify the pointer to point at the start
   // of the next field if !NULL
   wxString SplitField(const char *start, const char **next = NULL) const;

   // split the line of the input file into fields using our delimiter, return
   // the number of fields
   size_t SplitLine(const wxString& line, wxArrayString *fields) const;

   // try to find the correct delimiter by looking at the first few lines of
   // m_textfile - if we think we found one, return TRUE and set m_chDelimiter,
   // otherwise return FALSE
   bool TestDelimiter(char chDelimiter);

private:
   // the character delimiting different fields
   char m_chDelimiter;

   // the textfile containing the text of the file last used by CanImport()
   wxTextFile m_textfile;

   // the last result of CanImport()
   bool m_lastTestResult;

   DECLARE_ADB_IMPORTER();
   DECLARE_NO_COPY_CLASS(AdbTextImporter)
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// macros for dynamic importer creation
// ----------------------------------------------------------------------------

IMPLEMENT_ADB_IMPORTER(AdbTextImporter,
                       gettext_noop("Comma/TAB separated text format address book import module"),
                       gettext_noop("Comma/TAB separated values"),
                       "Vadim Zeitlin <vadim@wxwindows.org>");

// ----------------------------------------------------------------------------
// AdbTextImporter
// ----------------------------------------------------------------------------

AdbTextImporter::AdbTextImporter()
{
   m_chDelimiter = 0;
}

wxString AdbTextImporter::SplitField(const char *start,
                                     const char **next) const
{
   wxString curField;
   const char *pc;
   for ( pc = start; ; pc++ )
   {
      if ( *pc == m_chDelimiter || !*pc )
      {
         break;
      }
      else if ( *pc == '"' )
      {
         // deal with quoting - eat until the end of quoted string
         pc++; // skip opening quote
         while ( *pc && *pc != '"' )
         {
            curField += *pc++;
         }

         if ( !*pc )
         {
            // we don't give any message to the user because we don't have
            // enough info to say anything clever here
            wxLogDebug(_T("AdbTextImporter: unterminated quote."));

            break;
         }
      }
      else
      {
         // normal char
         curField += *pc;
      }
   }

   if ( next )
   {
      // the start of the next field
      *next = *pc ? pc + 1 : pc;
   }

   return curField;
}

size_t AdbTextImporter::SplitLine(const wxString& line,
                                  wxArrayString *fields) const
{
   fields->Empty();

   const char *pc = line.c_str();
   for ( ;; )
   {
      wxString field = SplitField(pc, &pc);

      if ( !*pc )
         break;

      fields->Add(field);
   }

   return fields->GetCount();
}

bool AdbTextImporter::TestDelimiter(char chDelimiter)
{
   // test first few lines
   size_t nLine, nLines = wxMin(10, m_textfile.GetLineCount());
   size_t nDelimitersTotal = 0;
   size_t *nDelimiters = new size_t[nLines];
   for ( nLine = 0; nLine < nLines; nLine++ )
   {
      // count the number of delimiter chars in this line
      size_t nDelimitersInLine = 0;
      wxString line = m_textfile[nLine];
      for ( const char *pc = line.c_str(); *pc; pc++ )
      {
         if ( *pc == chDelimiter )
            nDelimitersInLine++;
      }

      nDelimitersTotal += nDelimitersInLine;
      nDelimiters[nLine] = nDelimitersInLine;
   }

   if ( (nLines > 0) && (nDelimitersTotal > nLines) )
   {
      // if there are more or less the same number of the delmiters per
      // line, assume it's ok
      size_t nDelimitersAverage = nDelimitersTotal / nLines;
      size_t maxDiff = nDelimitersAverage / 10;
      for ( nLine = 0; m_lastTestResult && (nLine < nLines); nLine++ )
      {
         if ( abs((int)(nDelimiters[nLine] - nDelimitersAverage)) > (int)maxDiff )
         {
            // this line looks strange...
            return FALSE;
         }
      }
   }
   else
   {
      // the file is empty or has too few delimiters
      return FALSE;
   }

   // looks ok
   m_chDelimiter = chDelimiter;

   return TRUE;
}

bool AdbTextImporter::CanImport(const String& filename)
{
   if ( filename == m_textfile.GetName() )
   {
      // we've already examined this file
      return m_lastTestResult;
   }

   if ( !m_textfile.Open(filename) )
   {
      m_lastTestResult = FALSE;
   }
   else
   {
      // try first TABs, then commas
      m_lastTestResult = TestDelimiter('\t') || TestDelimiter(',');
   }

   return m_lastTestResult;
}

bool AdbTextImporter::StartImport(const String& filename)
{
   if ( filename == m_textfile.GetName() )
   {
      // we already loaded it
      return TRUE;
   }

   return m_textfile.Open(filename);
}

size_t AdbTextImporter::GetEntryNames(const String& path,
                                      wxArrayString& entries) const
{
   if ( !!path )
   {
      // the text file is flat, there are nothing in subgroups

      FAIL_MSG(_T("how did we get here in the first place?"));

      return 0;
   }

   CHECK( m_textfile.IsOpened(), 0, _T("forgot to call StartImport?") );

   entries.Empty();
   size_t nLines = m_textfile.GetLineCount();
   for ( size_t nLine = 0; nLine < nLines; nLine++ )
   {
      entries.Add(SplitField(m_textfile[nLine]));
   }

   return entries.GetCount();
}

size_t AdbTextImporter::GetGroupNames(const String& /* path */,
                                      wxArrayString& /* groups */) const
{
   // the "mailing list" info is lost when exporting into CSV format, there are
   // no groups left - the file is flat

   return 0;
}

bool AdbTextImporter::ImportEntry(const String& /* path */,
                                  size_t index,
                                  AdbEntry *entry)
{
   // this is for Netscape Communicator 4.5
   enum
   {
      DisplayName,
      LastName,
      FirstName,
      Notes,
      City,
      State,
      Email,
      Title,
      Unknown,      // this field is always empty, as much as I try...
      Address,
      Zip,
      Country,
      PhoneWork,
      Fax,
      PhoneHome,
      Organization,
      Nickname,
      Cellular,
      Pager,
      FieldsTotal
   };

   wxArrayString fields;
   size_t nFields = SplitLine(m_textfile[index], &fields);
   if ( nFields == 0 )
   {
      // should have at least something!
      return FALSE;
   }

   #define COPY_FIELD(our, foreign)                      \
      if ( foreign < nFields )                           \
         entry->SetField(AdbField_##our, fields[foreign])

   COPY_FIELD(FullName, DisplayName);
   // no analogue of LastName in our ADB
   COPY_FIELD(FirstName, FirstName);
   COPY_FIELD(Comments, Notes);
   COPY_FIELD(Title, Title);

   COPY_FIELD(EMail, Email);

   // address is taken to be home address
   COPY_FIELD(H_Street, Address);
   COPY_FIELD(H_City, City);
   COPY_FIELD(H_Locality, State);
   COPY_FIELD(H_Postcode, Zip);
   COPY_FIELD(H_Country, Country);

   COPY_FIELD(H_Phone, PhoneHome);
   COPY_FIELD(O_Phone, PhoneWork);
   COPY_FIELD(H_Fax, Fax);

   // no cellular/pager entries in ADB
   #undef COPY_FIELD

   // this exposes a nice (and huge) bug in ADB/Config code, to be fixed later
#if 0
   // if the nickname is empty, take the display name
   String nick;
   if ( Nickname < nFields )
   {
      nick = fields[Nickname];
   }
   else
   {
      // take something as default...
      nick = fields[DisplayName];
   }

   entry->SetField(AdbField_NickName, nick);
#endif

   return TRUE;
}

