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
#endif // USE_PCH

#include "adb/AdbExport.h"
#include "adb/AdbImpExp.h"

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// public interface
// ----------------------------------------------------------------------------

bool AdbExport(AdbEntryGroup& group,
               AdbExporter& exporter)
{
   // simple...
   return exporter.Export(group, _T(""));
}
