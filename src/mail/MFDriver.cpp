//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   mail/MFDriver.cpp: MFDriver class implementation
// Purpose:     MFDriver is a class for creating MailFolders
// Author:      Vadim Zeitlin
// Modified by:
// Created:     05.07.02
// CVS-ID:      $Id$
// Copyright:   (C) 2002 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

#ifdef __GNUG__
   #pragma implementation "MFDriver.h"
#endif

#include  "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"
#endif // USE_PCH

#include "mail/Driver.h"

// ============================================================================
// implementation
// ============================================================================

MFDriver *MFDriver::ms_drivers = NULL;

/* static */
MFDriver *MFDriver::Get(const char *name)
{
   for ( MFDriver *driver = GetHead();
         driver;
         driver = driver->GetNext() )
   {
      if ( strcmp(driver->GetName(), name) == 0 )
      {
         return driver;
      }
   }

   return NULL;
}

