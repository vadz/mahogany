///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   adb/AdbImport.cpp - generic algorithm for ADB import
// Purpose:     the generic function AdbImport which may use any AdbImporter
//              implementation to import data from foreign ADB format into the
//              native one
// Author:      Vadim Zeitlin
// Modified by:
// Created:     04.07.99
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

#ifdef __GNUG__
   #pragma implementation "AdbImport.h"
#endif

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"
   #include "MApplication.h"

   #include <wx/log.h>          // for wxLogDebug
#endif // USE_PCH

#include "gui/wxMDialogs.h"

#include "adb/AdbDataProvider.h"
#include "adb/AdbManager.h"
#include "adb/AdbBook.h"
#include "adb/AdbImport.h"
#include "adb/AdbFrame.h"     // for AddBookToAdbEditor()

#ifdef USE_ADB_MODULES
   #include "MModule.h"
#endif // USE_ADB_MODULES

// ----------------------------------------------------------------------------
// persistent msgboxes we use here
// ----------------------------------------------------------------------------

extern const MPersMsgBox *M_MSGBOX_CONFIRM_ADB_IMPORTER;

// ----------------------------------------------------------------------------
// function prototypes
// ----------------------------------------------------------------------------

// recurisive import helper
static bool AdbImportGroup(AdbImporter   *importer,  // from
                           AdbEntryGroup *group,     // to
                           const String& path);      // starting at

// return the importer for this file (tries all available importers if none is
// given or just checks that this one can import it otherwise)
static AdbImporter *FindImporter(const String& filename,
                                 AdbImporter *importer);

// common part of both AdbImport()
static bool DoAdbImport(const String& filename,
                        AdbEntryGroup *group,
                        AdbImporter *importer,
                        String *errMsg = NULL);

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// global functions
// ----------------------------------------------------------------------------

static bool AdbImportGroup(AdbImporter   *importer,  // from
                           AdbEntryGroup *group,     // to
                           const String& path)       // starting at
{
   // import entries
   size_t nAnonIndex = 0;
   wxArrayString entries;
   size_t nEntryCount = importer->GetEntryNames(path, entries);
   for ( size_t nEntry = 0; nEntry < nEntryCount; nEntry++ )
   {
      String entryName = entries[nEntry];

      if ( !entryName )
      {
         wxLogDebug(_T("Autogenerating nickname for nameless address entry %lu in '%s'"),
                    (unsigned long)nEntry, path.c_str());

         entryName.Printf(_("Nameless entry %d"), ++nAnonIndex);
      }

      AdbEntry *entry = group->CreateEntry(entryName);
      if ( !entry )
      {
         wxLogError(_("Import error: cannot create entry '%s/%s'."),
                    path.c_str(), entryName.c_str());

         return FALSE;
      }

      if ( !importer->ImportEntry(path, nEntry, entry) )
      {
         // importer logged the error already
         entry->DecRef();

         return FALSE;
      }

      entry->DecRef();
   }

   // and now the groups
   wxArrayString groups;
   size_t nGroupCount = importer->GetGroupNames(path, groups);
   for ( size_t nGroup = 0; nGroup < nGroupCount; nGroup++ )
   {
      String groupName = groups[nGroup];
      AdbEntryGroup *subgroup = group->CreateGroup(groupName);
      if ( !subgroup )
      {
         wxLogError(_("Import error: cannot create group '%s/%s'."),
                    path.c_str(), groupName.c_str());

         return FALSE;
      }

      String pathSubgroup;
      pathSubgroup << path << '/' << groupName;
      if ( !AdbImportGroup(importer, subgroup, pathSubgroup) )
      {
         return FALSE;
      }

      subgroup->DecRef();
   }

   return TRUE;
}

AdbImporter *FindImporter(const String& filename, AdbImporter *importer)
{
   // first find the importer we will use
   if ( !importer )
   {
      // try to find it
      AdbModule::AdbModuleInfo *info =
         AdbModule::GetAdbModuleInfo(ADB_IMPORTER_INTERFACE);
      while ( info )
      {
         importer = (AdbImporter *)info->CreateModule();
         if ( importer->CanImport(filename) )
         {
            // found
            break;
         }

         importer->DecRef();
         importer = NULL;

         info = info->next;
      }

      AdbModule::FreeAdbModuleInfo(info);
   }
   else
   {
      // verify that the given importer can import this file
      if ( !importer->CanImport(filename) )
      {
         // TODO give the user a possibility to choose the importer...

         wxString msg;
         msg.Printf(_("It seems that the file '%s' is not in the format '%s',\n"
                      "do you still want to try to import it?"),
                    filename.c_str(), importer->GetFormatDesc());

         if ( !MDialog_YesNoDialog(msg, NULL, _("Address book import"),
                                   M_DLG_NO_DEFAULT,
                                   M_MSGBOX_CONFIRM_ADB_IMPORTER) )
         {
            // cancelled by user
            mApplication->SetLastError(M_ERROR_CANCEL);

            return NULL;
         }
      }

      // the pointer will be returned to the outside world
      importer->IncRef();
   }

   return importer;
}

bool DoAdbImport(const String& filename,
                 AdbEntryGroup *group,
                 AdbImporter *importer,
                 String *errMsg)
{
   bool ok = importer->StartImport(filename);
   if ( !ok )
   {
      if ( errMsg )
      {
         errMsg->Printf(_("couldn't start importing from file '%s'."),
                        filename.c_str());
      }
   }
   else
   {
      // start importing: recursively copy all entries from the foreign ADB into
      // the native one
      ok = AdbImportGroup(importer, group, _T(""));
   }

   return ok;
}

// ----------------------------------------------------------------------------
// public API
// ----------------------------------------------------------------------------

bool AdbImport(const String& filename,
               const String& adbname,
               const String& desc,
               AdbImporter *importer)
{
   // reset the last error first as we check it below
   mApplication->SetLastError(M_ERROR_OK);

   importer = FindImporter(filename, importer);

   AdbBook *adbBook = NULL;

   bool ok = TRUE;
   wxString errMsg, provname;

   // can't conntinue without an importer
   if ( !importer )
   {
      errMsg = _("unsupported format.");

      goto exit;
   }

   // get the address book we will use
   {
      AdbManager_obj adbManager;
      if ( adbManager )
      {
         AdbDataProvider *providerNative = AdbDataProvider::GetNativeProvider();
         adbBook = adbManager->CreateBook(adbname, providerNative);
         provname = providerNative->GetProviderName();
         SafeDecRef(providerNative);
      }
   }

   if ( !adbBook )
   {
      errMsg.Printf(_("cannot create native address book '%s'."),
                     adbname.c_str());

      goto exit;
   }

   // FIXME a hack which ensure that the header is written before the entries
   //       (needed for automatic recognition of ADB format later)
   {
      wxString adbDesc = desc;
      if ( !adbDesc )
         adbDesc = adbBook->GetDescription();
      adbBook->SetDescription(adbDesc);
   }

   // load the data
   ok = DoAdbImport(filename, adbBook, importer, &errMsg);

exit:
   if ( !!errMsg )
   {
      // if we have an error message, something went wrong
      ok = FALSE;
   }

   if ( ok )
   {
      ok = adbBook->Flush();

      errMsg = _("failed to save the address book.");
   }

   if ( ok )
   {
      // the new ADB will be opened next time the editor opens
      AddBookToAdbEditor(adbname, provname);

      wxLogMessage(_("Successfully imported address book from file '%s' "
                     "(format '%s')"),
                   adbname.c_str(),
                   importer->GetFormatDesc());
   }
   else // an error occured
   {
      // only give the message if the operation wasn't cancelled by the user,
      // otherwise it's superfluous
      if ( mApplication->GetLastError() != M_ERROR_CANCEL )
      {
         wxString errImport = _("Import of address book from file '%s' failed");
         if ( !errMsg )
         {
            // nothing else...
            errImport += _T('.');

            wxLogError(errImport, adbname.c_str());
         }
         else
         {
            // add the detailed error message
            errImport += _T(": %s");

            wxLogError(errImport, adbname.c_str(), errMsg.c_str());
         }
      }
   }

   // free the book
   SafeDecRef(adbBook);

   // if we created it, let it go away - otherwise it matches IncRef() done
   // above
   SafeDecRef(importer);

   return ok;
}

bool AdbImport(const String& filename,
               AdbEntryGroup *group,
               AdbImporter *importer)
{
   importer = FindImporter(filename, importer);
   if ( !importer )
   {
      // build the message from the pieces already translated (used above)
      wxString errImport = _("Import of address book from file '%s' failed");
      errImport += _T(": %s");

      wxLogError(errImport, filename.c_str(), _("unsupported format."));

      return false;
   }

   bool ok = DoAdbImport(filename, group, importer);

   importer->DecRef();

   return ok;
}
