///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   adb/AdbManager.cpp - implementation of AdbManager class
// Purpose:     AdbManager manages all AdbBooks used by the application
// Author:      Vadim Zeitlin
// Modified by: 
// Created:     09.08.98
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
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
#   include "Mcommon.h"
#endif // USE_PCH

#include "adb/AdbDataProvider.h"

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// AdbProviderInfo
// ----------------------------------------------------------------------------
AdbDataProvider::AdbProviderInfo *AdbDataProvider::ms_listProviders = NULL;

AdbDataProvider::AdbProviderInfo::AdbProviderInfo(const char *name,
                                                  Constructor ctor,
                                                  bool canCreate,
                                                  const char *formatName,
                                                  AdbNameFormat adbFormat)
{
  bSupportsCreation = canCreate;
  nameFormat = adbFormat;
  szFmtName = formatName;
  szName = name;
  CreateProvider = ctor;

  // insert us in the linked list (in the head because it's simpler)
  pNext = AdbDataProvider::ms_listProviders;
  AdbDataProvider::ms_listProviders = this;
}

// ----------------------------------------------------------------------------
// static AdbDataProvider functions
// ----------------------------------------------------------------------------

AdbDataProvider *AdbDataProvider::GetNativeProvider()
{
   AdbDataProvider *provider = GetProviderByName(_T("FCDataProvider"));

   ASSERT_MSG( provider, _T("native ADB data provider not linked in??") );

   return provider;
}

// return provider by name (currently, this function always creates a new
// object, but in the future it might maintain a cache of providers...)
AdbDataProvider *AdbDataProvider::GetProviderByName(const String& name)
{
  AdbProviderInfo *info = ms_listProviders;
  while ( info ) {
    if ( name == info->szName )
      break;

    info = info->pNext;
  }

  if ( !info ) {
    // no provider with such name
    return NULL;
  }

  return info->CreateProvider();
}
