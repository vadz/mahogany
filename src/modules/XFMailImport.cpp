///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   modules/XFMailImport.cpp - import everything from XFMail
// Purpose:     import confi settings from XFMail
// Author:      Vadim Zeitlin
// Modified by:
// Created:     23.05.00
// CVS-ID:      $Id$
// Copyright:   (c) 2000 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
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

   #include "MFolder.h"

   #include <wx/log.h>
   #include <wx/file.h>       // for wxFile::Exists
   #include <wx/textfile.h>
#endif // USE_PCH

#include <wx/dir.h>
#include <wx/confbase.h>      // for wxExpandEnvVars
#include <wx/tokenzr.h>

#include "adb/AdbImport.h"
#include "adb/AdbImpExp.h"
#include "MImport.h"

#include "gui/wxFiltersDialog.h"

// ----------------------------------------------------------------------------
// the importer class
// ----------------------------------------------------------------------------

class MXFMailImporter : public MImporter
{
public:
   virtual bool Applies() const;
   virtual int GetFeatures() const;
   virtual bool ImportADB();
   virtual bool ImportFolders();
   virtual bool ImportSettings();
   virtual bool ImportFilters();

   DECLARE_M_IMPORTER()

private:
   // get the path (with the terminating '/') of XFMail directory
   static wxString GetXFMailDir();

   // import one setting from .xfmailrc
   void ImportSetting(const wxString& xfmailrc, size_t line,
                      const wxString& var, const wxString& value);

   // XFMail directory for folders
   wxString m_mailDir;
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// generic
// ----------------------------------------------------------------------------

IMPLEMENT_M_IMPORTER(MXFMailImporter, "XFMail",
                     gettext_noop("Import settings from XFMail"));

int MXFMailImporter::GetFeatures() const
{
   return Import_Folders | Import_Settings | Import_ADB | Import_Filters;
}

// ----------------------------------------------------------------------------
// determine if XFMail is installed
// ----------------------------------------------------------------------------

/* static */ wxString MXFMailImporter::GetXFMailDir()
{
   return wxExpandEnvVars("$HOME/.xfmail/");
}

bool MXFMailImporter::Applies() const
{
   return wxDir::Exists(GetXFMailDir());
}

// ----------------------------------------------------------------------------
// import XFMail settings
// ----------------------------------------------------------------------------

void MXFMailImporter::ImportSetting(const wxString& xfmailrc,
                                    size_t line,
                                    const wxString& var,
                                    const wxString& value)
{
   if ( var == "nntphost" )
   {
      mApplication->GetProfile()->writeEntry(MP_NNTPHOST, value);
      wxLogMessage(_("Imported NNTP host setting from XFMail: %s."),
                   value.c_str());
   }
   else if ( var == "nntpuser" )
   {
      // TODO
   }
   else if ( var == "replyexand" )
   {
      mApplication->GetProfile()->writeEntry(MP_RETURN_ADDRESS, value);
      wxLogMessage(_("Imported return address setting from XFMail: %s."),
                   value.c_str());
   }
   else if ( var == "myface" )
   {
      // TODO
   }
   else if ( var == "maildir" )
   {
      // remember it for ImportFolders()
      m_mailDir = value;
   }
}

bool MXFMailImporter::ImportSettings()
{
   wxString filename = GetXFMailDir() + ".xfmailrc";
   wxTextFile file(filename);
   if ( !file.Open() )
   {
      wxLogError(_("Failed to open XFMail configuration file '%s'."),
                 filename.c_str());

      return FALSE;
   }

   size_t nLines = file.GetLineCount();
   for ( size_t nLine = 0; nLine < nLines; nLine++ )
   {
      const wxString& line = file[nLine];

      int nEq = line.Find('=');
      if ( nEq == wxNOT_FOUND )
      {
         wxLogDebug("%s(%u): missing '=' sign.", filename.c_str(), nLine + 1);

         // skip line
         continue;
      }

      wxString var(line, (size_t)nEq), value = line.c_str() + nEq + 1;
      if ( !!value )
      {
         ImportSetting(filename, nLine + 1, var, value);
      }
   }

   return TRUE;
}

// ----------------------------------------------------------------------------
// import XFMail folders
// ----------------------------------------------------------------------------

bool MXFMailImporter::ImportFolders()
{
   wxString filename = GetXFMailDir() + ".xfmfolders";
   wxTextFile file(filename);
   if ( !file.Open() )
   {
      wxLogError(_("Failed to open XFMail folders file"));

      return FALSE;
   }

   // FIXME: should parse .xfmailrc from here to find maildir value
   if ( !m_mailDir )
      m_mailDir = wxExpandEnvVars("$HOME/Mail");

   if ( m_mailDir.Last() != '/' )
      m_mailDir += '/';

   size_t nImported = 0;
   bool bLastWasSystem = FALSE;
   size_t nLines = file.GetLineCount();
   for ( size_t nLine = 0; nLine < nLines; nLine++ )
   {
      const wxString& line = file[nLine];

      if ( line.length() < 3 || line[0] != '@' || line[1] != ' ' )
      {
         // the folder lines start with '@' in the first column followed by a
         // space, so just ignore this one
         continue;
      }

      // first get the folder name
      wxString folderName;
      const char *p = line.c_str() + 2;
      while ( *p != ' ' )
         folderName += *p++;

      // don't import "system" XFMail folders
      if ( folderName == "inbox" ||
           folderName == "outbox" ||
           folderName == "trash" ||
           folderName == "sent_mail" ||
           folderName == "draft" ||
           folderName == "template" )
      {
         bLastWasSystem = TRUE;

         continue;
      }
      else if ( bLastWasSystem )
      {
         // the first folder immediately following the system folders is the
         // spool, skip it too
         //
         // FIXME should parse .xfmsources instead of guessing that there is
         //       only one spool!
         bLastWasSystem = FALSE;

         continue;
      }

      // if the name starts with m_mailDir, the folder name is just the part
      // of it - but if it does not, then the name is entire string and to
      // obtain path we must prefix it with m_mailDir
      wxString path, name;
      if ( folderName.StartsWith(m_mailDir, &name) )
      {
         path = folderName;
      }
      else
      {
         name = folderName;
         path << m_mailDir << name;
      }

      // now get the folder type

      p++;  // skip space

      wxString typeString;
      while ( isdigit(*p) )
         typeString += *p++;

      // we only know about types 1 (MH) and 8 (MBOX)
      unsigned long nType;
      if ( !typeString.ToULong(&nType) || (nType != 1 && nType != 8) )
      {
         wxLogDebug("%s(%u): unreckognized folder type %s, skipping.",
                    filename.c_str(), nLine + 1, typeString.c_str());
         continue;
      }

      FolderType type = nType == 1 ? MF_MH : MF_FILE;

      MFolder *folder = CreateFolderTreeEntry
                        (
                         NULL,      // no parent
                         name,      // the folder name
                         type,      //            type
                         0,         //            flags
                         path,      //            path
                         FALSE      // don't notify
                        );
      if ( folder )
      {
         wxLogMessage(_("Imported folder '%s'."), folderName.c_str());

         nImported++;

         folder->DecRef();
      }
      else
      {
         wxLogError(_("Error importing folder '%s'."), folderName.c_str());
      }
   }

   if ( !nImported )
   {
      wxLogError(_("XFMail folder import failed."));

      return FALSE;
   }

   // refresh the tree(s)
   MEventManager::Send
    (
      new MEventFolderTreeChangeData
         (
          "",
          MEventFolderTreeChangeData::CreateUnder
         )
    );

   wxLogMessage(_("Successfully imported %u XFMail folders."), nImported);

   return TRUE;
}

// ----------------------------------------------------------------------------
// import XFMail address book(s)
// ----------------------------------------------------------------------------

bool MXFMailImporter::ImportADB()
{
   // first, find all XFMail ADBs
   wxArrayString xfmailADBs;

   const wxString XFMAIL_ADB_PREFIX = ".xfbook";
   wxString dirname = GetXFMailDir();
   wxDir dir(dirname);
   if ( dir.IsOpened() )
   {
      wxString filename;
      bool cont = dir.GetFirst(&filename, XFMAIL_ADB_PREFIX + '*',
                               wxDIR_FILES | wxDIR_HIDDEN);
      while ( cont )
      {
         xfmailADBs.Add(filename);

         cont = dir.GetNext(&filename);
      }
   }

   size_t count = xfmailADBs.GetCount();
   if ( !count )
   {
      wxLogError(_("Couldn't find any XFMail address books in '%s'."),
                 dirname.c_str());

      return FALSE;
   }

   AdbImporter *importer = AdbImporter::GetImporterByName("AdbXFMailImporter");
   if ( !importer )
   {
      wxLogError(_("XFMail address book import module not found."));

      return FALSE;
   }

   dirname += '/';
   size_t nImported = 0;
   for ( size_t n = 0; n < count; n++ )
   {
      wxString adbusername,
               adbname = xfmailADBs[n].c_str() + XFMAIL_ADB_PREFIX.length();
      if ( !adbname )
      {
         adbname = "xfmail";
         adbusername = _("Default XFMail address book");
      }
      else
      {
         // +1 for the separating dot
         adbname = adbname.c_str() + 1;
         adbusername = adbname;
      }
      adbname += ".adb";

      wxString path = dirname + xfmailADBs[n];
      if ( AdbImport(path, adbname, adbusername, importer) )
      {
         wxLogMessage(_("Successfully imported XFMail address book '%s'."),
                      path.c_str());

         nImported++;
      }
      else
      {
         wxLogError(_("Failed to import XFMail address book '%s'."),
                    path.c_str());
      }
   }

   importer->DecRef();

   return nImported != 0;
}

// ----------------------------------------------------------------------------
// import XFMail filters
// ----------------------------------------------------------------------------

bool MXFMailImporter::ImportFilters()
{
   // read the XFMail rules file
   wxString filename = GetXFMailDir() + ".xfmrules";
   wxTextFile file(filename);
   if ( !file.Open() )
   {
      wxLogError(_("Failed to open XFMail filter rules file."));

      return FALSE;
   }

   // find the profile where we will store our rules: as we want them to apply
   // to all incoming messages, create them under "New Mail"

   wxString newmail = READ_APPCONFIG(MP_NEWMAIL_FOLDER);
   if ( !newmail )
      newmail = "INBOX";
   newmail += "/Filters";
   ProfileBase *profileNewMailFilters = ProfileBase::CreateProfile(newmail);

   // find the first "unused" filter number
   size_t nFilter = 0;
   for ( ;; )
   {
      if ( !profileNewMailFilters->HasGroup(wxString::Format("%u", nFilter)) )
         break;

      nFilter++;
   }

   size_t nFilterFirst = nFilter;

   size_t nLines = file.GetLineCount();
   for ( size_t nLine = 0; nLine < nLines; nLine++ )
   {
      // each XFMail rule takes 2 lines in the following format:
      //
      //    @name action flags data fmatch
      //    tmatch
      //
      // where the fields come from the following struct (from XFMail sources)

#define R_DISCARD    0x01   /* delete it */
#define R_MOVE       0x02   /* move to folder specified in data */
#define R_FORWARD    0x03   /* forward to address specified in data */
#define R_VACATION   0x04   /* reply with vacation messages */
#define R_RESEND     0x05   /* resend message */
#define R_EXECUTE    0x06   /* execute program/script with message */

#define CASE_I       0x0001   /* case insensitive ( only on regexp ) */
#define R_VAC_KEEP   0x0002   /* don't delete message */
#define R_SAVE_ADDR  0x0004   /* save addresses in address book */
#define R_MARK_READ  0x0008   /* mark matching message as read */
#define R_SILENT     0x0010   /* don't count if incoming message */
#define R_NINCOMING  0x0020   /* apply by special request */
#define R_LOG        0x0040   /* log when matches */
#define R_OUTGOING   0x0080   /* outgoing rule */
#define R_EMODIFY    0x0100   /* allow external program to modify msg. */

#if 0
typedef struct _xf_rule {
   char name[16];                   /* rule name */
   char fmatch[MAX_FIELD_NAME_LEN]; /* field to look in the mail header */
   char tmatch[255];                /* text to match in the field (regexp) */
   char data[64];                   /* misc data */
   u_int action;                    /* what to do with the message */
   u_int flags;                     /* special flags */
   } xf_rule;
#endif // 0

      const wxString& line = file[nLine];
      const char *p = line.c_str();
      if ( *p++ != '@' )
      {
         wxLogDebug("%s(%u): rule line doesn't start with '@', skipping.",
                    filename.c_str(), nLine + 1);

         continue;
      }

      // tokenize the string
      wxStringTokenizer tk(p, " ");
      if ( tk.CountTokens() != 5 )
      {
         wxLogDebug("%s(%u): rule line doesn't contain exactly 5 tokens, skipping.",
                    filename.c_str(), nLine + 1);

         continue;
      }

      wxString name = tk.GetNextToken();
      unsigned long action, flags;
      if ( !tk.GetNextToken().ToULong(&action) ||
           !tk.GetNextToken().ToULong(&flags) )
      {
         wxLogDebug("%s(%u): non numeric rule action or flags, skipping.",
                    filename.c_str(), nLine + 1);

         continue;
      }

      wxString data = tk.GetNextToken(),
               fmatch = tk.GetNextToken();

      // read the text to match from the next line
      const wxString& tmatch = file[++nLine];

      // translate XFMail rule to our form

      static const char *headers[] =
      {
         "Subject",
         "Header",
         "From",
         "Body",
         "Message",
         "To",
         "Sender"
      };
      ORC_Where_Enum where = ORC_W_Invalid;
      wxASSERT_MSG( WXSIZEOF(headers) == ORC_W_Max, "should be in sync" );
      for ( size_t n = 0; n < ORC_W_Max; n++ )
      {
         if ( fmatch == headers[n] )
         {
            where = (ORC_Where_Enum)n;

            break;
         }
      }

      if ( where == ORC_W_Invalid )
      {
         wxLogDebug("%s(%u): unreckognized rule header '%s', skipping.",
                    filename.c_str(), nLine + 1, fmatch.c_str());

         continue;
      }

      OAC_Types_Enum what;
      switch ( action )
      {
         case R_DISCARD:
            what = OAC_T_Delete;
            break;

         case R_MOVE:
            what = OAC_T_MoveTo;
            break;

         case R_FORWARD:
         case R_VACATION:
         case R_RESEND:
         case R_EXECUTE:
            wxLogMessage(_("Unsupported XFMail rule type, skipping."));

            continue;
      }

      // now create our rule from this data
      wxString filterName = wxString::Format("%u", nFilter++);
      ProfileBase *profile = ProfileBase::CreateProfile(filterName,
                                                        profileNewMailFilters);
      wxString program;
      SaveSimpleFilter(
                       profile,
                       name,
                       ORC_T_MatchRegEx,
                       where,
                       tmatch,
                       what,
                       data,
                       &program
                      );
      profile->DecRef();

      wxLogVerbose(_("Imported XFMail filter rule '%s'."), program.c_str());
   }

   profileNewMailFilters->DecRef();

   nFilter -= nFilterFirst;
   if ( !nFilter )
   {
      wxLogMessage(_("No filters were imported."));

      return FALSE;
   }
   else
   {
      wxLogMessage(_("Successfully imported %u filters."), nFilter);

      return TRUE;
   }
}
