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

   #include <wx/log.h>
#endif // USE_PCH

#include "MDialogs.h"

#include "adb/AdbDataProvider.h"
#include "adb/AdbManager.h"
#include "adb/AdbBook.h"
#include "adb/AdbImport.h"

#ifdef USE_ADB_MODULES
   #include "MModule.h"
#endif // USE_ADB_MODULES

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// recurisive import helper
static bool AdbImportGroup(AdbImporter   *importer,  // from
                           AdbEntryGroup *group,     // to
                           const String& path);      // starting at

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
         wxLogDebug("Autogenerating nickname for nameless address "
                    "entry %d in '%s'", nEntry, path.c_str());

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

bool AdbImport(const String& filename,
               const String& adbname,
               AdbImporter *importer)
{
   // first find the importer we will use
   if ( !importer )
   {
      // try to find it
      AdbImporter::AdbImporterInfo *info = AdbImporter::GetAdbImpporterInfo();
      while ( info )
      {
         importer = info->CreateImporter();
         if ( importer->CanImport(filename) )
         {
            // found
            break;
         }

         importer->DecRef();
         importer = NULL;

         info = info->next;
      }

      AdbImporter::FreeAdbImporterInfo(info);
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
                    filename.c_str(), importer->GetDescription());

         if ( !MDialog_YesNoDialog(msg, NULL, _("Address book import"),
                                   FALSE, "ConfirmAdbImporter") )
         {
            // cancelled by user
            return FALSE;
         }
      }

      // to match the DecRef() at the end of the function - it's easier to just
      // call IncRef() once more than to track if we're using the importer
      // passed in as argument or the one we found ourselves.
      importer->IncRef();
   }

   AdbBook *adbBook = NULL;

   bool ok = TRUE;
   wxString errMsg;

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
   //       (needed for automatic reckognition of ADB format later)
   adbBook->SetUserName(adbBook->GetUserName());

   // load the data
   ok = importer->StartImport(filename);
   if ( !ok )
   {
      errMsg.Printf(_("couldn't start importing from file '%s'."),
                    filename.c_str());

      goto exit;
   }

   // start importing: recursively copy all entries from the foreign ADB into
   // the native one
   ok = AdbImportGroup(importer, adbBook, "");

exit:
   if ( !!errMsg )
   {
      // if we have an error message, something went wrong
      ok = FALSE;
   }

   if ( ok )
   {
      adbBook->Flush();

      wxLogMessage(_("Successfully imported address book from file '%s' "
                     "(format '%s')"),
                   adbname.c_str(),
                   importer->GetDescription());
   }
   else
   {
      wxString errImport = _("Import of address book from file '%s' failed");
      if ( !errMsg )
      {
         // nothing else...
         errImport += '.';

         wxLogError(errImport, adbname.c_str());
      }
      else
      {
         // add the detailed error message
         errImport += ": %s";

         wxLogError(errImport, adbname.c_str(), errMsg.c_str());
      }
   }

   // free the book
   SafeDecRef(adbBook);

   // if we created it, let it go away - otherwise it matches IncRef() done
   // above
   SafeDecRef(importer);

   return ok;
}

// ----------------------------------------------------------------------------
// static AdbImporter functions
// ----------------------------------------------------------------------------

size_t AdbImporter::EnumImporters(wxArrayString& names, wxArrayString& descs)
{
   names.Empty();
   descs.Empty();

   AdbImporter *importer = NULL;
   AdbImporter::AdbImporterInfo *info = AdbImporter::GetAdbImpporterInfo();
   while ( info )
   {
      importer = info->CreateImporter();
      if ( importer )
      {
         names.Add(importer->GetName());
         descs.Add(importer->GetDescription());

         importer->DecRef();
         importer = NULL;
      }
      else
      {
         wxLogDebug("Failed to load ADB importer '%s'.", info->name);
      }

      info = info->next;
   }

   FreeAdbImporterInfo(info);

   return names.GetCount();
}

AdbImporter *AdbImporter::GetImporterByName(const String& name)
{
   AdbImporter *importer = NULL;
   AdbImporter::AdbImporterInfo *info = AdbImporter::GetAdbImpporterInfo();
   while ( info )
   {
      importer = info->CreateImporter();

      if ( importer->GetName() == name )
         break;

      importer->DecRef();
      importer = NULL;

      info = info->next;
   }

   FreeAdbImporterInfo(info);

   return importer;
}

// ----------------------------------------------------------------------------
// AdbImporter functions for working with AdbImporterInfo
// ----------------------------------------------------------------------------

AdbImporter::AdbImporterInfo *AdbImporter::GetAdbImpporterInfo()
{
#ifdef USE_ADB_MODULES
   ASSERT_MSG( !ms_listImporters, "forgot to call FreeAdbImporterInfo()!" );

   // find all modules implementing AdbImporter interface
   MModuleListing *listing = MModule::ListAvailableModules("AdbImporter");
   if ( listing )
   {
      size_t count = listing->Count();
      for ( size_t n = 0; n < count; n++ )
      {
         // the object will automatically put itself into the linked list
         new AdbImporterInfo((*listing)[n].GetName(),
                             (*listing)[n].GetShortDescription());
      }

      listing->DecRef();
   }
#endif // USE_ADB_MODULES

   return ms_listImporters;
}

void AdbImporter::FreeAdbImporterInfo(AdbImporter::AdbImporterInfo *info)
{
#ifdef USE_ADB_MODULES
   AdbImporterInfo *next;
   while ( info )
   {
      next = info->next;
      delete info;
      info = next;
   }

   ms_listImporters = NULL;
#else // !USE_ADB_MODULES
   // nothing to do, the struct is static and no memory is allocated or freed
#endif // USE_ADB_MODULES/!USE_ADB_MODULES
}

// ----------------------------------------------------------------------------
// AdbImporterInfo
// ----------------------------------------------------------------------------

AdbImporter::AdbImporterInfo *AdbImporter::ms_listImporters = NULL;

AdbImporter::AdbImporterInfo::AdbImporterInfo(const char *name_,
#ifndef USE_ADB_MODULES
                                              Constructor ctor,
#endif // !USE_ADB_MODULES
                                              const char *desc_)
{
   // init member vars
   name = name_;
   desc = desc_;
#ifndef USE_ADB_MODULES
   CreateImporter = ctor;
#endif // !USE_ADB_MODULES

   // insert us in the linked list (in the head because it's simpler and we
   // don't lose anything - order of insertion of different importers is
   // undefined anyhow)
   next = AdbImporter::ms_listImporters;
   AdbImporter::ms_listImporters = this;
}

#ifdef USE_ADB_MODULES

AdbImporter *AdbImporter::AdbImporterInfo::CreateImporter() const
{
   MModule *module = MModule::LoadModule(name);

   return (AdbImporter *)module;
}

#endif // USE_ADB_MODULES

