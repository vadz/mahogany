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

   #include <wx/log.h>
   #include <wx/file.h>       // for wxFile::Exists
#endif // USE_PCH

#include <wx/confbase.h>      // for wxExpandEnvVars

#include "MImport.h"

// ----------------------------------------------------------------------------
// the importer class
// ----------------------------------------------------------------------------

class MPineImporter : public MImporter
{
public:
   virtual bool Applies() const;
   virtual int GetFeatures() const;
   virtual bool ImportADB();
   virtual bool ImportFolders();
   virtual bool ImportSettings();
   virtual bool ImportFilters();

   DECLARE_M_IMPORTER()
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
   return FALSE;
}

// ----------------------------------------------------------------------------
// import the Pine folders in MBOX format
// ----------------------------------------------------------------------------

bool MPineImporter::ImportFolders()
{
   return FALSE;
}

// ----------------------------------------------------------------------------
// import the Pine settings
// ----------------------------------------------------------------------------

bool MPineImporter::ImportSettings()
{
   return FALSE;
}

// ----------------------------------------------------------------------------
// import the Pine filters
// ----------------------------------------------------------------------------

bool MPineImporter::ImportFilters()
{
   return FALSE;
}
