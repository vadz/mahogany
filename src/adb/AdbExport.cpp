///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   adb/AdbExport.cpp - static AdbExporter functions
// Purpose:     dynamic creation for AdbExporter and AdbExport implementation
// Author:      Vadim Zeitlin
// Modified by:
// Created:     25.07.99
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

#ifdef __GNUG__
   #pragma implementation "AdbExport.h"
#endif

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"

   #include <wx/log.h>
#endif // USE_PCH

#include "adb/AdbExport.h"
#include "adb/AdbImpExp.h"

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// static AdbExporter functions
// ----------------------------------------------------------------------------

size_t AdbExporter::EnumExporters(wxArrayString& names, wxArrayString& descs)
{
   names.Empty();
   descs.Empty();

   AdbExporter *exporter = NULL;
   AdbExporter::AdbExporterInfo *info = AdbExporter::ms_listExporters;
   while ( info )
   {
      exporter = info->CreateExporter();

      names.Add(exporter->GetName());
      descs.Add(exporter->GetDescription());

      exporter->DecRef();
      exporter = NULL;

      info = info->next;
   }

   return names.GetCount();
}

AdbExporter *AdbExporter::GetExporterByName(const String& name)
{
   AdbExporter *exporter = NULL;
   AdbExporter::AdbExporterInfo *info = AdbExporter::ms_listExporters;
   while ( info )
   {
      exporter = info->CreateExporter();

      if ( exporter->GetName() == name )
         break;

      exporter->DecRef();
      exporter = NULL;

      info = info->next;
   }

   return exporter;
}

// ----------------------------------------------------------------------------
// AdbExporterInfo
// ----------------------------------------------------------------------------

AdbExporter::AdbExporterInfo *AdbExporter::ms_listExporters = NULL;

AdbExporter::AdbExporterInfo::AdbExporterInfo(const char *name_,
                                              Constructor ctor,
                                              const char *desc_)
{
   name = name_;
   desc = desc_;
   CreateExporter = ctor;

   // insert us in the linked list (in the head because it's simpler and we
   // don't lose anything - order of insertion of different exporters is
   // undefined anyhow)
   next = AdbExporter::ms_listExporters;
   AdbExporter::ms_listExporters = this;
}

// ----------------------------------------------------------------------------
// public interface
// ----------------------------------------------------------------------------

bool AdbExport(const AdbEntryGroup& group, AdbExporter& exporter)
{
   // simple...
   return exporter.Export(group);
}
