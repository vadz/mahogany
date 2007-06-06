///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   adb/AdbModule.cpp - implementation of ADB modules subsystem
// Purpose:     AdbModule class allows to load ADB importers/exporters from the
//              modules at run-time if M is compiled with modules support or
//              just created them "pseudo-dynamically" otherwise which means
//              that the modules can be created by name (or even without knowing
//              its name at all)
// Author:      Vadim Zeitlin
// Modified by:
// Created:     19.05.00 (extracted from AdbImport.cpp)
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

   #include <wx/log.h>          // for wxLogDebug
#endif // USE_PCH

#include "adb/AdbModule.h"

#ifdef USE_ADB_MODULES
   #include "MModule.h"
#endif // USE_ADB_MODULES

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// static AdbImporter functions
// ----------------------------------------------------------------------------

size_t AdbModule::EnumModules(const wxChar *kind,
                              wxArrayString& names, wxArrayString& descs)
{
   names.Empty();
   descs.Empty();

   AdbModule *importer = NULL;
   AdbModule::AdbModuleInfo *info = AdbModule::GetAdbModuleInfo(kind);
   while ( info )
   {
      importer = info->CreateModule();
      if ( importer )
      {
         names.Add(importer->GetName());
         descs.Add(importer->GetFormatDesc());

         importer->DecRef();
         importer = NULL;
      }
      else
      {
         wxLogDebug(_T("Failed to load ADB importer '%s'."), info->name.c_str());
      }

      info = info->next;
   }

   FreeAdbModuleInfo(info);

   return names.GetCount();
}

AdbModule *AdbModule::GetModuleByName(const wxChar *kind, const String& name)
{
   AdbModule *importer = NULL;
   AdbModule::AdbModuleInfo *info = AdbModule::GetAdbModuleInfo(kind);
   while ( info )
   {
      importer = info->CreateModule();

      if ( importer )
      {
         if ( importer->GetName() == name )
            break;

         importer->DecRef();
      }
      else
      {
         wxLogDebug(_T("Failed to load ADB importer '%s'."), info->name.c_str());
      }

      importer = NULL;

      info = info->next;
   }

   FreeAdbModuleInfo(info);

   return importer;
}

// ----------------------------------------------------------------------------
// AdbModule functions for working with AdbModuleInfo
// ----------------------------------------------------------------------------

AdbModule::AdbModuleInfo *AdbModule::GetAdbModuleInfo(const wxChar *kind)
{
#ifdef USE_ADB_MODULES
   ASSERT_MSG( !ms_listModules, _T("forgot to call FreeAdbModuleInfo()!") );

   // find all modules implementing AdbModule interface
   MModuleListing *listing = MModule::ListAvailableModules(kind);
   if ( listing )
   {
      size_t count = listing->Count();
      for ( size_t n = 0; n < count; n++ )
      {
         const MModuleListingEntry& entry = (*listing)[n];

         // the object will automatically put itself into the linked list
         new AdbModuleInfo(entry.GetName(), entry.GetShortDescription());
      }

      listing->DecRef();
   }
#endif // USE_ADB_MODULES

   return ms_listModules;
}

void AdbModule::FreeAdbModuleInfo(AdbModule::AdbModuleInfo *info)
{
#ifdef USE_ADB_MODULES
   AdbModuleInfo *next;
   while ( info )
   {
      next = info->next;
      delete info;
      info = next;
   }

   ms_listModules = NULL;
#else // !USE_ADB_MODULES
   // nothing to do, the struct is static and no memory is allocated or freed
#endif // USE_ADB_MODULES/!USE_ADB_MODULES
}

// ----------------------------------------------------------------------------
// AdbModuleInfo
// ----------------------------------------------------------------------------

AdbModule::AdbModuleInfo *AdbModule::ms_listModules = NULL;

AdbModule::AdbModuleInfo::AdbModuleInfo(const wxChar *name_,
#ifndef USE_ADB_MODULES
                                        Constructor ctor,
#endif // !USE_ADB_MODULES
                                        const wxChar *desc_)
{
   // init member vars
   name = name_;
   desc = desc_;
#ifndef USE_ADB_MODULES
   CreateModule = ctor;
#endif // !USE_ADB_MODULES

   // insert us in the linked list (in the head because it's simpler and we
   // don't lose anything - order of insertion of different importers is
   // undefined anyhow)
   next = AdbModule::ms_listModules;
   AdbModule::ms_listModules = this;
}

#ifdef USE_ADB_MODULES

AdbModule *AdbModule::AdbModuleInfo::CreateModule() const
{
   MModule *module = MModule::LoadModule(name);

   return (AdbModule *)module;
}

#endif // USE_ADB_MODULES


