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
#   include "Profile.h"
#   include "MApplication.h"
#endif // USE_PCH

#include <wx/log.h>
#include <wx/dynarray.h>
 
#include "adb/AdbEntry.h"
#include "adb/AdbBook.h"
#include "adb/AdbManager.h"
#include "adb/AdbDataProvider.h"

#include "guidef.h"
#include "MDialogs.h"

// ----------------------------------------------------------------------------
// types
// ----------------------------------------------------------------------------

// cache: we store pointers to all already created ADBs here (it means that
// once created they're not deleted until the next call to ClearCache)
static ArrayAdbBooks gs_cache;

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// AdbLookup helper
static void GroupLookup(ArrayAdbEntries& aEntries,
                        AdbEntryGroup *pGroup, 
                        const String& what,
                        int where,
                        int how);

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// global functions
// ----------------------------------------------------------------------------

// recursive (depth first) search
static void GroupLookup(ArrayAdbEntries& aEntries,
                        AdbEntryGroup *pGroup, 
                        const String& what,
                        int where,
                        int how)
{
  wxArrayString aNames;
  size_t nGroupCount = pGroup->GetGroupNames(aNames);
  for ( size_t nGroup = 0; nGroup < nGroupCount; nGroup++ ) {
    AdbEntryGroup *pSubGroup = pGroup->GetGroup(aNames[nGroup]);

    GroupLookup(aEntries, pSubGroup, what, where, how);

    pSubGroup->DecRef();
  }

  aNames.Empty();
  size_t nEntryCount = pGroup->GetEntryNames(aNames);
  for ( size_t nEntry = 0; nEntry < nEntryCount; nEntry++ ) {
    AdbEntry *pEntry = pGroup->GetEntry(aNames[nEntry]);

    if ( pEntry->Matches(what, where, how) ) {
      aEntries.Add(pEntry);
    }
    else {
      pEntry->DecRef();
    }
  }
}

bool AdbLookup(ArrayAdbEntries& aEntries,
               const String& what,
               int where,
               int how,
               const ArrayAdbBooks *paBooks)
{
  wxASSERT( aEntries.IsEmpty() );

  if ( paBooks == NULL || paBooks->IsEmpty() )
    paBooks = &gs_cache;

  size_t nBookCount = paBooks->Count();
  for ( size_t nBook = 0; nBook < nBookCount; nBook++ ) {
    GroupLookup(aEntries, (*paBooks)[nBook], what, where, how);
  }

  // return true if something found
  return !aEntries.IsEmpty();
}

String AdbExpand(const String& what, wxFrame *frame)
{
  String result;

  AdbManager_obj manager;
  CHECK( manager, result, "can't expand address: no AdbManager" );
  
  manager->LoadAll();

  ArrayAdbEntries aEntries;
  if ( AdbLookup(aEntries, what) )
  {
    int rc = MDialog_AdbLookupList(aEntries, frame);
    
    if ( rc != -1 )
    {
      AdbEntry *pEntry = aEntries[rc];
      pEntry->GetField(AdbField_EMail, &result);
      
      wxString str;
      pEntry->GetField(AdbField_NickName, &str);
      if ( frame )
      {
        wxLogStatus(frame, _("Expanded '%s' using entry '%s'"),
                    what.c_str(), str.c_str());
      }
    }
    
    // free all entries
    size_t nCount = aEntries.Count();
    for ( size_t n = 0; n < nCount; n++ )
    {
      aEntries[n]->DecRef();
    }
  }
  else
  {
    if ( frame )
      wxLogStatus(frame, _("No matches for '%s'."), what.c_str());
  }

  return result;
}

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
    ms_pManager->IncRef();
  }

  wxASSERT( ms_pManager != NULL );

  return ms_pManager;
}

// decrement the ref count of the manager object
void AdbManager::Unget()
{
  wxCHECK_RET( ms_pManager,
               "AdbManager::Unget() called without matching Get()!" );

  if ( !ms_pManager->DecRef() ) {
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
    book->IncRef();

    return book;
  }

  // no, must create a new one
  AdbDataProvider *prov = provider;
  if ( prov == NULL ) {
    // try to find it
    AdbDataProvider::AdbProviderInfo *info = AdbDataProvider::ms_listProviders;
    while ( info ) {
      prov = info->CreateProvider();
      if ( prov->TestBookAccess(name, AdbDataProvider::Test_Create) ) {
        if ( providerName ) {
          // return the provider name if asked for it
          *providerName = info->szName;
        }
        break;
      }

      prov->DecRef();
      prov = NULL;

      info = info->pNext;
    }
  }

  if ( prov ) {
    book = prov->CreateBook(name);
    if ( provider == NULL ) {
      // only if it's the one we created, not the one which was passed in!
      prov->DecRef();
    }
  }

  if ( book == NULL ) {
    wxLogError(_("Can't open the address book '%s'."), name.c_str());
  }
  else {
    book->IncRef();
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
  book->IncRef();

  return book;
}

// FIXME it shouldn't even know about AdbEditor existence! but this would
// involve changing AdbFrame.cpp to use this function somehow and I don't have
// the time to do it right now...
void AdbManager::LoadAll()
{
  ProfileBase & conf = *mApplication->GetProfile();
  conf.SetPath("/AdbEditor");

  wxArrayString astrAdb, astrProv;
  RestoreArray(conf, astrAdb, "AddressBooks");
  RestoreArray(conf, astrProv, "Providers");

  wxString strProv;
  AdbDataProvider *pProvider;
  size_t nCount = astrAdb.Count();
  for ( size_t n = 0; n < nCount; n++ ) {
    if ( n < astrProv.Count() )
      strProv = astrProv[n];
    else
      strProv.Empty();
    
    if ( strProv.IsEmpty() )
      pProvider = NULL;
    else
      pProvider = AdbDataProvider::GetProviderByName(strProv);
    
    // it's getting worse and worse... we're using our knowledge of internal
    // structure of AdbManager here: we know the book will not be deleted
    // after this DecRef because the cache also has a lock on it
    SafeDecRef(CreateBook(astrAdb[n], pProvider));

    SafeDecRef(pProvider);
  }
}

void AdbManager::ClearCache()
{
  size_t nCount = gs_cache.Count();
  for ( size_t n = 0; n < nCount; n++ ) {
    gs_cache[n]->DecRef();
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
  size_t nCount = gs_cache.Count();
  for ( size_t n = 0; n < nCount; n++ ) {
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
