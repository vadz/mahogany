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

#include "Mcommon.h"
#include <wx/log.h>
#include <wx/dynarray.h>
 
#include "adb/AdbBook.h"
#include "adb/AdbManager.h"
#include "adb/AdbDataProvider.h"

// ----------------------------------------------------------------------------
// types
// ----------------------------------------------------------------------------

// array of AdbBooks
WX_DEFINE_ARRAY(AdbBook *, ArrayBooks);

// cache: we store pointers to all already created ADBs here (it means that
// once created they're not deleted until the next call to ClearCache)
static ArrayBooks gs_cache;

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// AdbManager static functions and variables
// ----------------------------------------------------------------------------

AdbManager *AdbManager::ms_pManager = NULL;

// create the manager object if it doesn't exist yet and return it
AdbManager *AdbManager::Get()
{
  if ( ms_pManager ==  NULL ) {
    // create it
    ms_pManager = new AdbManager;
  }
  else {
    // just inc ref count on the existing one
    ms_pManager->Lock();
  }

  wxASSERT( ms_pManager != NULL );

  return ms_pManager;
}

// decrement the ref count of the manager object
void AdbManager::Unget()
{
  wxCHECK_RET( ms_pManager,
               "AdbManager::Unget() called without matching Get()!" );

  if ( !ms_pManager->Unlock() ) {
    // the object deleted itself
    ms_pManager = NULL;
  }
}

// ----------------------------------------------------------------------------
// AdbManager public methods
// ----------------------------------------------------------------------------

// create a new address book using specified provider or trying all of them
// if it's NULL
AdbBook *AdbManager::CreateBook(const String& name, AdbDataProvider *provider,
                                String *providerName)
{
  // first see if we don't already have it
  AdbBook *book = FindInCache(name);
  if ( book ) {
    book->Lock();

    return book;
  }

  // no, must create a new one
  AdbDataProvider *prov = provider;
  if ( prov == NULL ) {
    // try to find it
    AdbDataProvider::AdbProviderInfo *info = AdbDataProvider::ms_listProviders;
    while ( info ) {
      prov = info->CreateProvider();
      if ( prov->IsSupportedFormat(name) ) {
        if ( providerName ) {
          // return the provider name if asked for it
          *providerName = info->szName;
        }
        break;
      }

      prov->Unlock();
      prov = NULL;

      info = info->pNext;
    }
  }

  if ( prov ) {
    book = prov->CreateBook(name);
    prov->Unlock();
  }

  if ( book == NULL ) {
    wxLogError(_("Can't open the address book '%s'."), name.c_str());
  }
  else {
    book->Lock();
    gs_cache.Add(book);
  }

  return book;
}

size_t AdbManager::GetBookCount() const
{
  return gs_cache.Count();
}

AdbBook *AdbManager::GetBook(size_t n) const
{
  AdbBook *book = gs_cache[n];
  book->Lock();

  return book;
}

void AdbManager::ClearCache()
{
  uint nCount = gs_cache.Count();
  for ( uint n = 0; n < nCount; n++ ) {
    gs_cache[n]->Unlock();
  }

  gs_cache.Clear();
}

AdbManager::~AdbManager()
{
  ClearCache();
}

// ----------------------------------------------------------------------------
// AdbManager private methods
// ----------------------------------------------------------------------------
AdbBook *AdbManager::FindInCache(const String& name) const
{
  AdbBook *book;
  uint nCount = gs_cache.Count();
  for ( uint n = 0; n < nCount; n++ ) {
    book = gs_cache[n];
    if ( name == book->GetName() )
      return book;
  }

  return NULL;
}

// ----------------------------------------------------------------------------
// AdbManager debugging support
// ----------------------------------------------------------------------------
#ifdef DEBUG

String AdbManager::Dump() const
{
  String str;
  str.Printf("AdbManager: m_nRef = %d, %d books in cache",
             m_nRef, gs_cache.Count());

  return str;
}

#endif
