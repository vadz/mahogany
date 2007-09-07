///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   adb/ImportPine.cpp - import ADB data from PINE addressbook
// Purpose:     Import data from a local PINE address book. This is
//              "interesting" because PINE address book may contain the "mailing
//              list" entries which map to ADB groups, i.e. unlike many other
//              ADB formats this one isn't flat.
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

   #include <wx/log.h>
   #include <wx/dynarray.h>
   #include <wx/utils.h>
#endif // USE_PCH

#include <wx/confbase.h>      // for wxExpandEnvVars
#include <wx/textfile.h>

#include "adb/AdbEntry.h"
#include "adb/AdbImport.h"

extern void WXDLLEXPORT wxSplitPath(wxArrayString&, const wxChar *);

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

class AdbPineImporter : public AdbImporter
{
public:
   AdbPineImporter() { }

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
   // parse the entries in PINE address book files: some of them are ADB
   // entries, some of them are ADB groups and only one kind of them will be
   // returned depending on the value of chooseEntries parameter.
   size_t GetEntriesOrGroups(wxArrayString& names, bool chooseEntries) const;

   // get all addresses (as one  unsplitted string) for the given path (it
   // should be of the form /group - the function checks for this). Returns an
   // empty string on error.
   wxString GetAddressesOfGroup(const wxString& path) const;

   // splits the string containing all addresses of the mailing list into
   // 2 arrays: nicknames and email address for each of them (last argument
   // may be NULL, the second - not). Returns the number of addresses in this
   // mailing list.
   size_t SplitMailingListAddresses(const wxString& addresses,
                                    wxArrayString *nicks,
                                    wxArrayString *emails = NULL) const;

   // parses one addressbook file entry (it may be one or more lines), returns
   // FALSE if the syntax is invalid (although most errors are just silently
   // ignored). All out parameters except index may be NULL in which case the
   // corresponding information won't be returned. The index is modified if the
   // line is continued and corresponds to the index of the last continuation
   // line in the file.
   bool ParsePineADBEntry(size_t *index,
                          wxString *nickname = NULL,
                          wxString *addresses = NULL,
                          wxString *fullname = NULL,
                          wxString *comment = NULL) const;

   // ParsePineADBEntry helper: checks whether the entry in line "*index"
   // continues to the next line: if it does, return TRUE and change index and
   // line, otherwise return FALSE and leave them unchanged.
   //
   // It should be called after finishing with parsing of a field, '*ppc' is
   // supposed to point at the '\t' character which separates the fields.
   bool CheckHasNextField(size_t *index,
                          wxString *line,
                          const wxChar **ppc) const;

   // another ParsePineADBEntry helper: extracts one field which is supposed to
   // start at the current position
   wxString ExtractField(size_t *index,
                         wxString *line,
                         const wxChar **ppc) const;

   // the starting lines for the entries and groups
   wxArrayInt m_entriesLineNumbers,
              m_groupLineNumbers;

   // the array of group names to make it easy to map names to indices in
   // m_groupLineNumbers
   wxArrayString m_groupNames;

   // the data
   wxTextFile m_textfile;

   DECLARE_NO_COPY_CLASS(AdbPineImporter)
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// macros for dynamic importer creation
// ----------------------------------------------------------------------------

IMPLEMENT_ADB_IMPORTER(AdbPineImporter,
                       gettext_noop("PINE address book import module"),
                       gettext_noop("PINE address book"),
                       "Vadim Zeitlin <vadim@wxwindows.org>");


// ----------------------------------------------------------------------------
// helpers
// ----------------------------------------------------------------------------

wxString AdbPineImporter::ExtractField(size_t *index,
                                       wxString *line,
                                       const wxChar **ppc) const
{
   wxString field;
   const wxChar *pc = *ppc;

   bool cont = TRUE;

   while ( cont )
   {
      while ( *pc && *pc != '\t' )
      {
         field += *pc++;
      }

      cont = FALSE;

      // the mailing list addresses field may be split on several lines at
      // commas - check for this
      if ( !*pc && *(pc - 1) == ',' )
      {
         // the line should continue
         pc--;

         if ( CheckHasNextField(index, line, &pc) )
         {
            cont = TRUE;
         }
         else
         {
            wxLogWarning(_("Unterminated mailing address list at line %d "
                           "in the PINE address book file '%s'."),
                         *index, line->c_str());
         }
      }
   }

   *ppc = pc;

   return field;
}

bool AdbPineImporter::CheckHasNextField(size_t *index,
                                        wxString *line,
                                        const wxChar **ppc) const
{
   const wxChar *pc = *ppc;

   if ( !*pc )
   {
      wxLogWarning(_("TAB character expected at position %d in line %d of "
                     "the address book file '%s'; the entry will be ignored."),
                   pc - line->c_str(),
                   index,
                   m_textfile.GetName());

      return FALSE;
   }

   // skip '\t'
   if ( !*++pc )
   {
      bool continued;

      size_t n = *index + 1;
      if ( n < m_textfile.GetLineCount() )
      {
         wxString lineNext = m_textfile[n];

         // my PINE 4.10 inserts 3 spaces in the beginning of the continued lines
         // but I don't know whether all versions do it
         if ( wxStrncmp(lineNext, _T("   "), 3) == 0 )
         {
            // yes, this entry is continued on the next line
            continued = TRUE;
         }
         else
         {
            // no, next line doesn't start with the continuation prefix
            continued = FALSE;
         }

         *index = n;
         *line = lineNext.c_str() + 3; // skip these 3 spaces
         pc = line->c_str();
      }
      else
      {
         // can't be continued because this was the last line of the file
         continued = FALSE;
      }

      if ( !continued )
      {
         wxLogWarning(_("Unexpected line end at position %d in line %d of the "
                        "address book file '%s'; the entry will be ignored."),
                      pc - line->c_str(),
                      index,
                      m_textfile.GetName());

         return FALSE;
      }
   }

   *ppc = pc;

   return TRUE;
}

bool AdbPineImporter::ParsePineADBEntry(size_t *index,
                                        wxString *nickname,
                                        wxString *addresses,
                                        wxString *fullname,
                                        wxString *comment) const
{
   // format is: nick TAB fullname TAB address TAB fcc TAB comment
   //
   // the main difficulty is that the entry may be broken by '\n' in any place
   // and continued on the next line - for this it's enough that the next line
   // starts with 3 spaces (not sure about 3, not sure about whether TABs work -
   // this is just what my PINE does)
   //
   // another subtlety is that the address may be a (,) bracketed list of
   // addresses each of which has the form of "[name] <address>". In this case
   // this is not a single ADB entry but a "mailing list" for PINE, i.e. an ADB
   // group for us (we can't really map it to an ADB entry because different
   // addresses have different full names)

   wxString line = m_textfile[*index];

   const wxChar *pc = line.c_str();

   if ( !*pc || wxIsspace(*pc) )
   {
      // this is not the start of an entry
      wxLogWarning(_("Unrecognized address book entry '%s'."), pc);

      return FALSE;
   }

   wxString tmp ;

   // first extract the nickname
   tmp = ExtractField(index, &line, &pc);
   if ( nickname )
      *nickname = tmp;

   if ( !CheckHasNextField(index, &line, &pc) )
      return FALSE;

   // extract fullname?
   tmp = ExtractField(index, &line, &pc);
   if ( fullname )
      *fullname = tmp;

   if ( !CheckHasNextField(index, &line, &pc) )
      return FALSE;

   // extract addresses
   tmp = ExtractField(index, &line, &pc);
   if ( addresses )
      *addresses = tmp;

   // extract comment - notice that we must do it even if we're not interested
   // in extracting the comment because we must call CheckHasNextField() to skip
   // the line with comment if the entry continues to the next line!
   {
      // disable the log messages because it's possible that there is just no
      // comment field
      wxLogNull noLog;

      if ( CheckHasNextField(index, &line, &pc) )
      {
         // FCC field - we're not interested in it
         (void)ExtractField(index, &line, &pc);

         if ( CheckHasNextField(index, &line, &pc) )
         {
            if ( comment )
               *comment = ExtractField(index, &line, &pc);
         }
      }
   }

   return TRUE;
}

size_t AdbPineImporter::GetEntriesOrGroups(wxArrayString& names,
                                           bool chooseEntries) const
{
   names.Empty();

   // const_cast
   wxArrayInt& lineNumbers = (wxArrayInt &)
                             (chooseEntries ? m_entriesLineNumbers
                                            : m_groupLineNumbers);
   lineNumbers.Empty();

   size_t nLines = m_textfile.GetLineCount();
   for ( size_t nLine = 0; nLine < nLines; nLine++ )
   {
      size_t nStartLine = nLine;

      wxString nickname, addresses;
      if ( ParsePineADBEntry(&nLine, &nickname, &addresses) )
      {
         if ( !addresses )
         {
            // what can we do with an address book entry without address?
            continue;
         }

         bool isEntry = addresses[0u] != '(';
         if ( isEntry == chooseEntries )
         {
            if ( !isEntry )
            {
               // remember the group name
               ((wxArrayString &)m_groupNames).Add(nickname); // const_cast
            }

            // remember the starting line index
            lineNumbers.Add(nStartLine);

            names.Add(nickname);
         }
         //else: we're not interested in this kind of entries
      }
      //else: warning already given
   }

   return names.GetCount();
}

wxString AdbPineImporter::GetAddressesOfGroup(const wxString& path) const
{
   wxArrayString components;
   wxSplitPath(components, path);

   if ( components.GetCount() != 1 )
   {
      FAIL_MSG( _T("we may only have simple subgroups in PINE addressbooks") );

      return wxEmptyString;
   }

   int indexGroup = m_groupNames.Index(components[0u]);
   if ( indexGroup == wxNOT_FOUND )
   {
      FAIL_MSG( _T("unknown group") );

      return wxEmptyString;
   }

   wxString addresses;
   size_t indexLine = m_groupLineNumbers[(size_t)indexGroup];
   if ( !ParsePineADBEntry(&indexLine, NULL, &addresses) )
   {
      return wxEmptyString;
   }

   return addresses;
}

size_t AdbPineImporter::SplitMailingListAddresses(const wxString& addresses,
                                                  wxArrayString *nicks,
                                                  wxArrayString *emails) const
{
   CHECK( nicks, 0, _T("must pass a valid pointer to nicknames array") );

   if ( !addresses || addresses[0u] != '(' || addresses.Last() != ')' )
   {
      wxLogWarning(_("Invalid format for list of addresses of PINE mailing "
                     "list entry: '%s'."), addresses.c_str());

      return 0;
   }

   // split over commas - what will happen if the names contain commas? I don't
   // know, but PINE doesn't handle it correctly, so don't try to do anything
   // extraordinarily smart neither.

   wxString address;
   const wxChar *pc = addresses.c_str() + 1;   // skip '('
   for ( ; ; pc++ )
   {
      if ( *pc == ')' || *pc == ',' )
      {
         // split one address
         if ( !address )
         {
            wxLogDebug(_T("Empty address in the PINE mailing list entry ignored."));
         }
         else
         {
            wxString nick = address.BeforeLast('<');
            wxString email = address.AfterLast('<');
            if ( !nick )
            {
               // take the first part of the address
               nick = email.BeforeLast('@');
            }
            else
            {
               nick.Trim();

               // email is not empty because address is not
               if ( email.Last() != '>' )
               {
                  wxLogWarning(_("No matching '>' in the address '%s'."),
                               address.c_str());
               }
               else
               {
                  // throw away last char
                  email.Truncate(email.Len() - 1);
               }
            }

            nicks->Add(nick);
            if ( emails )
               emails->Add(email);
         }

         if ( *pc == ')' )
         {
            // end of line
            break;
         }

         address.Empty();
      }
      else
      {
         // normal char
         address += *pc;
      }
   }

   return nicks->GetCount();
}

// ----------------------------------------------------------------------------
// AdbImporter methods
// ----------------------------------------------------------------------------

bool AdbPineImporter::CanImport(const String& filename)
{
   if ( m_textfile.GetName() != filename )
   {
      if ( !m_textfile.Open(filename) )
      {
         // failed to read, so how will we import it?
         return FALSE;
      }

      // disable the log messages - we're only testing
      wxLogNull noLog;

      // try the first 10 entries and see how many of them are ok
      size_t nEntry = 0, nEntriesOk = 0;
      size_t nLine = 0, nLines = m_textfile.GetLineCount();
      while ( (nEntry < 10) && (nLine < nLines) )
      {
         if ( ParsePineADBEntry(&nLine) )
            nEntriesOk++;

         nLine++;
         nEntry++;
      }

      // test is purely heuristic
      if ( nEntriesOk < wxMax(nEntry / 2, 1) )
      {
         // too few good entries
         return FALSE;
      }
   }
   //else: we already loaded it, hence it's ok

   return TRUE;
}

bool AdbPineImporter::StartImport(const String& filename)
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

   // file already loaded by CanImport()
   return TRUE;
}

size_t AdbPineImporter::GetEntryNames(const String& path,
                                       wxArrayString& entries) const
{
   if ( !path )
   {
      return GetEntriesOrGroups(entries, TRUE /* entries */);
   }
   else
   {
      wxString addresses = GetAddressesOfGroup(path);
      if ( !addresses )
         return 0;

      // split all addresses
      return SplitMailingListAddresses(addresses, &entries);
   }
}

size_t AdbPineImporter::GetGroupNames(const String& path,
                                       wxArrayString& groups) const
{
   if ( !path )
   {
      return GetEntriesOrGroups(groups, FALSE /* groups */);
   }
   else
   {
      // no subgroups
      return 0;
   }
}

bool AdbPineImporter::ImportEntry(const String& path,
                                  size_t index,
                                  AdbEntry *entry)
{
   wxString nickname;

   if ( !path )
   {
      // a top level entry
      CHECK( index < m_entriesLineNumbers.GetCount(), FALSE,
             _T("invalid entry index") );

      size_t nLine = m_entriesLineNumbers[index];

      wxString email, fullname, comment;
      if ( !ParsePineADBEntry(&nLine, &nickname, &email, &fullname, &comment) )
      {
         return FALSE;
      }

      entry->SetField(AdbField_EMail, email);
      entry->SetField(AdbField_FullName, fullname);
      entry->SetField(AdbField_Comments, comment);
   }
   else
   {
      // this is an entry from the mailing list
      wxString addresses = GetAddressesOfGroup(path);
      if ( !addresses )
         return FALSE;

      // split all addresses
      wxArrayString nicks, emails;
      size_t count = SplitMailingListAddresses(addresses, &nicks, &emails);

      CHECK( index < count, FALSE, _T("invalid entry index") );

#ifdef DEBUG
      nickname = nicks[index];
#endif // DEBUG

      entry->SetField(AdbField_EMail, emails[index]);
   }

   // verify that this is indeed the right entry
#ifdef DEBUG
   wxString nickReal;
   entry->GetField(AdbField_NickName, &nickReal);

   ASSERT_MSG( nickname == nickReal, _T("wrong index or wrong entry") );
#endif // DEBUG

   return TRUE;
}

String AdbPineImporter::GetDefaultFilename() const
{
   String location;

#ifdef OS_UNIX
   // the default location for Unix is $HOME/.addresbook
   location = wxExpandEnvVars(_T("$HOME/.addresbook"));

   if ( !wxFile::Exists(location) )
   {
      // nice try, but it's not there - so we don't know
      wxLogVerbose(_("Didn't find the PINE address book in the default "
                     "location (%s)."), location.c_str());

      location.Empty();
   }
#else // !Unix
   // have no idea where PC Pine stores its address book...
#endif // Unix/!Unix

   return location;
}
