///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   modules/PineImport.cpp - import everything from Pine
// Purpose:     we currently only import folders, some settings and ADB, but
//              we could also import filter rules which Pine supports in newer
//              versions
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

#include <wx/textfile.h>
#include <wx/dir.h>
#include <wx/confbase.h>

#include "adb/AdbImport.h"
#include "adb/AdbImpExp.h"
#include "MFolder.h"
#include "MEvent.h"
#include "MImport.h"

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_COMPOSE_SIGNATURE;
extern const MOption MP_EXTERNALEDITOR;
extern const MOption MP_NNTPHOST;
extern const MOption MP_PERSONALNAME;
extern const MOption MP_POLLINCOMINGDELAY;
extern const MOption MP_REPLY_PREFIX;
extern const MOption MP_SMTPHOST;
extern const MOption MP_VIEW_WRAPMARGIN;
extern const MOption MP_WRAPMARGIN;

// ----------------------------------------------------------------------------
// the importer class
// ----------------------------------------------------------------------------

class MPineImporter : public MImporter
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
   bool ImportSettingsFromFile(const wxString& pinerc);

   // line is the line number (from 1) for debug messages
   void ImportSetting(const wxString& pinerc, size_t line,
                      const wxString& var, const wxString& value);
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// generic
// ----------------------------------------------------------------------------

IMPLEMENT_M_IMPORTER(MPineImporter, "Pine",
                     gettext_noop("Import settings from PINE"));

int MPineImporter::GetFeatures() const
{
   return Import_Folders | Import_Settings | Import_ADB;
}

// ----------------------------------------------------------------------------
// determine if Pine is installed
// ----------------------------------------------------------------------------

bool MPineImporter::Applies() const
{
   // just check for ~/.pinerc file
   return wxFile::Exists(wxExpandEnvVars("$HOME/.pinerc"));
}

// ----------------------------------------------------------------------------
// import the Pine ADB
// ----------------------------------------------------------------------------

bool MPineImporter::ImportADB()
{
   // import the PINE address book from ~/.addressbook
   AdbImporter *importer = AdbImporter::GetImporterByName("AdbPineImporter");
   if ( !importer )
   {
      wxLogError(_("%s address book import module not found."), "PINE");

      return FALSE;
   }

   wxString filename = importer->GetDefaultFilename();
   wxLogMessage(_("Starting importing %s address book '%s'..."),
                "PINE", filename.c_str());
   bool ok = AdbImport(filename, "pine.adb", "PINE Address Book", importer);

   importer->DecRef();

   if ( ok )
      wxLogMessage(_("Successfully imported %s address book."), "PINE");
   else
      wxLogError(_("Failed to import %s address book."), "PINE");

   return ok;
}

// ----------------------------------------------------------------------------
// import the Pine folders in MBOX format
// ----------------------------------------------------------------------------

bool MPineImporter::ImportFolders(MFolder *folderParent, int flags)
{
   // create a folder for each mbox file in ~/mail

   // start by enumerating all files there
   wxString dirname = wxExpandEnvVars("$HOME/mail");
   wxDir dir(dirname);
   wxArrayString mboxFiles;
   if ( dir.IsOpened() )
   {
      wxString filename;
      bool cont = dir.GetFirst(&filename, "*", wxDIR_FILES);
      while ( cont )
      {
         mboxFiles.Add(filename);

         cont = dir.GetNext(&filename);
      }
   }

   // if no such files found - nothing to do
   size_t count = mboxFiles.GetCount();
   if ( !count )
   {
      wxLogMessage(_("No local %s folders found."), "PINE");
   }
   else // we do have some folders
   {
      wxLogMessage(_("Starting importing local %s mail folders."), "PINE");

      // the parent for all folders
      MFolder *parent = (flags & ImportFolder_AllUseParent)
                           == ImportFolder_AllUseParent ? folderParent : NULL;

      // create the folder tree entries for them
      size_t nImported = 0;
      for ( size_t n = 0; n < count; n++ )
      {
         const wxString& name = mboxFiles[n];
         wxString path;
         path << dirname << '/' << name;

         MFolder *folder = CreateFolderTreeEntry
                           (
                            parent,    // parent may be NULL
                            name,      // the folder name
                            MF_FILE,   //            type
                            0,         //            flags
                            path,      //            path
                            FALSE      // don't notify
                           );
         if ( folder )
         {
            wxLogMessage(_("Imported folder '%s'."), path.c_str());

            nImported++;

            folder->DecRef();
         }
         else
         {
            wxLogError(_("Error importing folder '%s'."), path.c_str());
         }
      }

      if ( !nImported )
      {
         wxLogError(_("Folder import failed."));

         return FALSE;
      }
      else
      {
         // refresh the tree(s)
         MEventManager::Send
          (
            new MEventFolderTreeChangeData
               (
                parent ? parent->GetFullName() : String(""),
                MEventFolderTreeChangeData::CreateUnder
               )
          );

         wxLogMessage(_("Successfully imported %u %s folders."),
                      nImported, "PINE");
      }
   }

   return TRUE;
}

// ----------------------------------------------------------------------------
// import the Pine settings
// ----------------------------------------------------------------------------

void MPineImporter::ImportSetting(const wxString& pinerc,
                                  size_t line,
                                  const wxString& var,
                                  const wxString& value)
{
   // TODO
   //
   // 1. handle global-address-book and address-book here to allow importing
   //    all ADBs, not just the main one
   // 2. handle customized-hdrs (ex: customized-hdrs=Reply-to: me@elsehwere)
   // 3. inbox-path?
   // 4. printer
   // 5. some options from feature-list

   if ( var == "composer-wrap-column" )
   {
      unsigned long wrap;
      if ( value.ToULong(&wrap) )
      {
         mApplication->GetProfile()->writeEntry(MP_WRAPMARGIN, wrap);
         mApplication->GetProfile()->writeEntry(MP_VIEW_WRAPMARGIN, wrap);

         wxLogMessage(_("Imported wrap margin setting from %s: %lu."),
                      "PINE", wrap);
      }
      else
      {
         wxLogDebug(_T(".pinerc(%lu): non numeric composer-wrap-column value."),
                    (unsigned long)line);
      }
   }
   else if ( var == "editor" )
   {
      wxString editor;
      editor << value << " %s";

      mApplication->GetProfile()->writeEntry(MP_EXTERNALEDITOR, editor);

      wxLogMessage(_("Imported external editor setting from %s: %s."),
                   "PINE", editor.c_str());
   }
   else if ( var == "mail-check-interval" )
   {
      unsigned long delay;
      if ( value.ToULong(&delay) )
      {
         mApplication->GetProfile()->writeEntry(MP_POLLINCOMINGDELAY, delay);

         wxLogMessage(_("Imported mail check interval setting from %s: %lu."),
                      "PINE", delay);
      }
      else
      {
         wxLogDebug(_T(".pinerc(%lu): non numeric mail-check-interval value."),
                    (unsigned long)line);
      }
   }
   else if ( var == "nntp-server" )
   {
      mApplication->GetProfile()->writeEntry(MP_NNTPHOST, value);
      wxLogMessage(_("Imported NNTP host setting from %s: %s."),
                   "PINE", value.c_str());
   }
   else if ( var == "personal-name" )
   {
      mApplication->GetProfile()->writeEntry(MP_PERSONALNAME, value);
      wxLogMessage(_("Imported personal name setting from %s: %s."),
                   "PINE", value.c_str());
   }
   else if ( var == "reply-indent-string" )
   {
      mApplication->GetProfile()->writeEntry(MP_REPLY_PREFIX, value);
      wxLogMessage(_("Imported reply prefix setting from %s: %s."),
                   "PINE", value.c_str());
   }
   else if ( var == "signature-file" )
   {
      mApplication->GetProfile()->writeEntry(MP_COMPOSE_SIGNATURE, value);
      wxLogMessage(_("Imported signature location from %s: %s."),
                   "PINE", value.c_str());
   }
   else if ( var == "smtp-server" )
   {
      // FIXME this is a list and the entries may contain port numbers too!
      mApplication->GetProfile()->writeEntry(MP_SMTPHOST, value);
      wxLogMessage(_("Imported SMTP server setting from %s: %s."),
                   "PINE", value.c_str());
   }
}

bool MPineImporter::ImportSettings()
{
   // parse the $HOME/.pinerc file and pick the settings of interest to us
   // from it
   wxString filename;

   filename = "/usr/lib/pine.conf";
   if ( wxFile::Exists(filename) )
      (void)ImportSettingsFromFile(filename);

   // this one must exist
   filename = wxExpandEnvVars("$HOME/.pinerc");

   return ImportSettingsFromFile(filename);
}

bool MPineImporter::ImportSettingsFromFile(const wxString& filename)
{
   wxTextFile file(filename);
   if ( !file.Open() )
   {
      wxLogError(_("Couldn't open %s configuration file '%s'."),
                 "PINE", filename.c_str());

      return FALSE;
   }

   size_t nLines = file.GetLineCount();
   for ( size_t nLine = 0; nLine < nLines; nLine++ )
   {
      const wxString& line = file[nLine];

      // skip empty lines and the comments
      if ( !line || line[0] == '#' )
      {
         continue;
      }

      // extract variable name and its value
      int nEq = line.Find('=');
      if ( nEq == wxNOT_FOUND )
      {
         wxLogDebug(_T("%s(%lu): missing '=' sign."),
                    filename.c_str(),
                    (unsigned long)nLine + 1);

         // skip line
         continue;
      }

      wxString var(line, (size_t)nEq),
               value(line.c_str() + nEq + 1);
      if ( !value.empty() )
      {
         ImportSetting(filename, nLine + 1, var, value);
      }
   }

   return TRUE;
}

// ----------------------------------------------------------------------------
// import the Pine filters
// ----------------------------------------------------------------------------

bool MPineImporter::ImportFilters()
{
   return FALSE;
}
