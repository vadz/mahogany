///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   modules/XFMailImport.cpp - import everything from XFMail
// Purpose:     import config settings from XFMail
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
   #include "Mdefaults.h"
   #include "MApplication.h"
#endif // USE_PCH

#include <wx/dir.h>
#include <wx/tokenzr.h>
#include <wx/confbase.h>
#include <wx/textfile.h>

#include "adb/AdbImport.h"
#include "adb/AdbImpExp.h"
#include "MImport.h"
#include "MFilter.h"
#include "Message.h"
#include "MFolder.h"

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_FROM_ADDRESS;
extern const MOption MP_NNTPHOST;
extern const MOption MP_PERSONALNAME;

// ----------------------------------------------------------------------------
// the importer class
// ----------------------------------------------------------------------------

class MXFMailImporter : public MImporter
{
public:
   virtual bool Applies() const;
   virtual int GetFeatures() const;
   virtual bool ImportADB();
   virtual bool ImportFolders(MFolder *folderParent, int flags);
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
                     gettext_noop("Import settings from XFMail"))

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
   Profile *profile = mApplication->GetProfile();
   if ( var == "nntphost" )
   {
      profile->writeEntry(MP_NNTPHOST, value);
      wxLogMessage(_("Imported NNTP host setting from %s: %s."),
                   "XFMail", value.c_str());
   }
   else if ( var == "nntpuser" )
   {
      // TODO
   }
   else if ( var == "from" )
   {
      String personalName = Message::GetNameFromAddress(value);
      if ( !!personalName )
      {
         profile->writeEntry(MP_PERSONALNAME, personalName);
         wxLogMessage(_("Imported name setting from %s: %s."),
                      "XFMail", personalName.c_str());
      }
   }
   else if ( var == "replyexand" )
   {
      profile->writeEntry(MP_FROM_ADDRESS, value);
      wxLogMessage(_("Imported return address setting from %s: %s."),
                   "XFMail", value.c_str());
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
      wxLogError(_("Failed to open %s configuration file '%s'."),
                 "XFMail",filename.c_str());

      return FALSE;
   }

   size_t nLines = file.GetLineCount();
   for ( size_t nLine = 0; nLine < nLines; nLine++ )
   {
      const wxString& line = file[nLine];

      int nEq = line.Find('=');
      if ( nEq == wxNOT_FOUND )
      {
         wxLogTrace(_T("importxfmail"),
                    _T("%s(%lu): missing '=' sign."),
                    filename.c_str(), (unsigned long)nLine + 1);

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

bool MXFMailImporter::ImportFolders(MFolder *folderParent, int flagsImport)
{
   wxString foldersfilename = GetXFMailDir() + ".xfmfolders";
   wxTextFile foldersfile(foldersfilename);
   if ( !foldersfile.Open() )
   {
      wxLogError(_("Failed to open %s folders file"), "XFMail");

      return FALSE;
   }

   // find the directory where XFMail folders live by default (i.e. the
   // relative paths are relative to this maildir)
   wxString filenamerc = GetXFMailDir() + ".xfmailrc";
   wxTextFile filerc(filenamerc);
   if ( filerc.Open() )
   {
      size_t nLines = filerc.GetLineCount();
      for ( size_t nLine = 0; nLine < nLines; nLine++ )
      {
         const wxString& line = filerc[nLine];

         int nEq = line.Find('=');
         if ( nEq == wxNOT_FOUND )
         {
            wxLogTrace(_T("importxfmail"),
                       _T("%s(%lu): missing '=' sign."),
                       filenamerc.c_str(), (unsigned long)nLine + 1);

            // skip line
            continue;
         }

         wxString var(line, (size_t)nEq), value = line.c_str() + nEq + 1;
         if ( var == "maildir" && !!value )
         {
            ImportSetting(filenamerc, nLine + 1, var, value);
         }
      }
   }

   if ( !m_mailDir )
      m_mailDir = wxExpandEnvVars("$HOME/Mail");

   if ( m_mailDir.Last() != '/' )
      m_mailDir += '/';

   bool error = FALSE;
   size_t nImported = 0;
   size_t nLines = foldersfile.GetLineCount();
   for ( size_t nLine = 0; nLine < nLines; nLine++ )
   {
      const wxString& line = foldersfile[nLine];

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

      if ( !folderName )
      {
         wxLogTrace(_T("importxfmail"),
                    _T("%s(%lu): empty folder name, skipping."),
                    foldersfilename.c_str(), (unsigned long)nLine + 1);
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
         // the folder name should not start with a slash
         if ( folderName[0u] == '/' )
         {
            wxLogTrace(_T("importxfmail"),
                       _T("%s(%lu): folder '%s' assumed to be a spool, skipping."),
                       foldersfilename.c_str(), (unsigned long)nLine + 1,
                       folderName.c_str());
            continue;
         }

         name = folderName;
         path << m_mailDir << name;
      }
      folderName = name; //now name==folderName in both cases

      // now get the folder type

      p++;  // skip space

      wxString typeString;
      while ( isdigit(*p) )
         typeString += *p++;

      // we only know about types 1 (MH) and 8 (MBOX) and don't support 2
      // (IMAP) yet (mainly because I don't have any IMAP folders in XFMail)
      unsigned long nType;
      if ( !typeString.ToULong(&nType) || (nType != 1 && nType != 8) )
      {
         wxLogTrace(_T("importxfmail"),
                    _T("%s(%lu): unrecognized folder type %s, skipping."),
                    foldersfilename.c_str(), (unsigned long)nLine + 1,
                    typeString.c_str());
         continue;
      }

      // all XFMail flag values
#define SYSTEM  0x00000001   /* system folder , can not be deleted or renamed */
#define SORTED  0x00000002   /* messages are sorted */
#define OPENED  0x00000004   /* folder is opened */
#define SEARCH  0x00000008   /* folder contains messages marked during search */
#define FRONLY  0x00000010   /* read only folder */
#define NOINFR  0x00000020   /* no inferiors (subfolders) */
#define FHIDDN  0x00000040   /* hidden folder */
#define NOTRASH 0x00000080   /* don't move messages to trash */
#define FRESCAN 0x00000100   /* folder contents has changed, update required */
#define FSHORTH 0x00000200   /* parse only part of message headers */
#define FMRKTMP 0x00000400   /* marked temporarily */
#define FUNREAD 0x00000800   /* only unread messages are opened */
#define FREMOTE 0x00001000   /* remote (IMAP, NEWS, etc...) folder */
#define FLOCKED 0x00002000   /* locked folder (MBOX) */
#define FREWRTE 0x00004000   /* needs to be rewritten (MBOX) */
#define FNOCLSE 0x00008000   /* don't close (don't discard messages) */
#define FDUMMY  0x00010000   /* dummy folder, can not be manipulated */
#define FSKIP   0x00020000   /* skip (don't show) the folder */
#define FRECNT  0x00040000   /* contains recently retrieved messages */
#define FALIAS  0x00080000   /* sname is an alias */
#define FNSCAN  0x00100000   /* don't rescan folder */
#define FEXPNG  0x00200000   /* EXPUNGE when deselection/closing (IMAP) */
#define FNOMOD  0x00400000   /* don't modify (ask if modify) */
#define FTOP    0x00800000   /* top folder of the hierarchy */
#define FSUBS   0x01000000   /* subscribed folder */

      p++;  // skip space

      // now deal with flags
      wxString flagsString;
      while ( isdigit(*p) )
         flagsString += *p++;

      unsigned long flags;
      if ( !flagsString.ToULong(&flags) )
      {
         wxLogTrace(_T("importxfmail"),
                    _T("%s(%lu): not numeric folder flags %s, skipping."),
                    foldersfilename.c_str(), (unsigned long)nLine + 1,
                    flagsString.c_str());
         continue;
      }

      // find the parent for the folder we're going to import
      MFolder *parent = NULL;
      if ( (flags & SYSTEM) ||
            folderName == "inbox" ||
            folderName == "outbox" ||
            folderName == "trash" ||
            folderName == "sent_mail" ||
            folderName == "draft" ||
            folderName == "template" )
      {
         wxLogTrace(_T("importxfmail"),
                    _T("%s(%lu): folder %s is a system folder."),
                    foldersfilename.c_str(), (unsigned long)nLine + 1,
                    folderName.c_str());

         // do we import system folders at all?
         if ( !(flagsImport & ImportFolder_SystemImport) )
         {
            wxLogTrace(_T("importxfmail"), _T("Skipping XFMail system folder."));

            continue;
         }

         // do we put under the given parent folder?
         if ( (flagsImport & ImportFolder_SystemUseParent)
                == ImportFolder_SystemUseParent )
            parent = folderParent;
      }
      else // not system folder
      {
         if ( (flagsImport & ImportFolder_AllUseParent)
                  == ImportFolder_AllUseParent )
            parent = folderParent;
      }

      // do create the folder
      MFolderType type = nType == 1 ? MF_MH : MF_FILE;

      MFolder *folder = CreateFolderTreeEntry
                        (
                         parent,    // parent folder
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
         // set the error flag, but continue with the other folders
         error = TRUE;
         wxLogError(_("Error importing folder '%s'."), folderName.c_str());
      }
   }

   if ( !nImported )
   {
      if ( error )
      {
         wxLogError(_("%s folder import from '%s' failed."), "XFMail",
                    m_mailDir.BeforeLast('/').c_str());

         return FALSE;
      }
      else
      {
         wxLogMessage(_("No %s folders to import were found."), "XFMail");
      }
   }
   else // we did import something
   {
      // refresh the tree(s)
      String nameParent;
      if ( (flagsImport & ImportFolder_AllUseParent) && folderParent )
         nameParent = folderParent->GetFullName();
      //else: at least some folders were added under root

      MEventManager::Send
       (
         new MEventFolderTreeChangeData
            (
             nameParent,
             MEventFolderTreeChangeData::CreateUnder
            )
       );

      wxLogMessage(_("Successfully imported %u %s folders."), nImported, "XFMail");
   }

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
      wxLogError(_("Couldn't find any %s address books in '%s'."),
                 "XFMail", dirname.c_str());

      return FALSE;
   }

   AdbImporter *importer = AdbImporter::GetImporterByName("AdbXFMailImporter");
   if ( !importer )
   {
      wxLogError(_("%s address book import module not found."),"XFMail");

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
         nImported++;
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
      wxLogError(_("Failed to open %s filter rules file."), "XFMail");

      return FALSE;
   }

   size_t nFilter = 0;
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
         wxLogTrace(_T("importxfmail"),
                    _T("%s(%lu): rule line doesn't start with '@', skipping."),
                    filename.c_str(), (unsigned long)nLine + 1);

         continue;
      }

      // tokenize the string
      wxStringTokenizer tk(p, " ");
      if ( tk.CountTokens() != 5 )
      {
         wxLogTrace(_T("importxfmail"),
                    _T("%s(%lu): rule line doesn't contain exactly 5 tokens, skipping."),
                    filename.c_str(), (unsigned long)nLine + 1);

         continue;
      }

      wxString name = tk.GetNextToken();
      unsigned long action, flags;
      if ( !tk.GetNextToken().ToULong(&action) ||
           !tk.GetNextToken().ToULong(&flags) )
      {
         wxLogTrace(_T("importxfmail"),
                    _T("%s(%lu): non numeric rule action or flags, skipping."),
                    filename.c_str(), (unsigned long)nLine + 1);

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
         "Sender",
         "Recipients"
      };
      MFDialogTarget where = ORC_W_Illegal;
      for ( size_t n = 0; n < WXSIZEOF(headers); n++ )
      {
         if ( fmatch == headers[n] )
         {
            where = (MFDialogTarget)n;

            break;
         }
      }

      if ( where == ORC_W_Illegal )
      {
         wxLogTrace(_T("importxfmail"),
                    _T("%s(%lu): unrecognized rule header '%s', skipping."),
                    filename.c_str(), (unsigned long)nLine + 1,
                    fmatch.c_str());

         continue;
      }

      MFDialogAction what = OAC_T_Illegal;
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
      MFilter_obj filter(name);
      MFilterDesc fd;
      MFDialogSettings *settings = MFDialogSettings::Create();
      fd.SetName(name);
      settings->AddTest(ORC_L_None, false, ORC_T_MatchRegEx, where, tmatch);
      settings->SetAction(what, data);
      fd.Set(settings);
      filter->Set(fd);

      nFilter++;

      wxLogVerbose(_("Imported %s filter rule '%s'."), "XFMail", name.c_str());
   }

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
