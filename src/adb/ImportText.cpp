///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   adb/ImportText.cpp - import ADB data from text (CSV/TAB) files
// Purpose:     this module defines AdbImporters for text files containing
//              comma (CSV) or TAB separated (TAB/TXT) fields. Such files may
//              be produced by "Export" feature of other e-mail clients (e.g.,
//              netscape)
// Author:      Vadim Zeitlin
// Modified by:
// Created:     05.07.99
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

#include "adb/AdbImport.h"

// ============================================================================
// implementation
// ============================================================================
