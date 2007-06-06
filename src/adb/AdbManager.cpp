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
#  include "Mcommon.h"
#  include "Profile.h"
#  include "MApplication.h"
#  include "guidef.h"
#  include "Moptions.h"		// for MOption
#  include "Mdefaults.h"	// for READ_CONFIG

#  include <wx/log.h>           // for wxLogStatus
#  include <wx/config.h>
#  include <wx/dynarray.h>      // for wxArrayString
#endif // USE_PCH

#include "adb/AdbEntry.h"
#include "adb/AdbFrame.h"        // for GetAdbEditorConfigPath()
#include "adb/AdbBook.h"
#include "adb/AdbManager.h"
#include "adb/AdbDataProvider.h"
#include "adb/AdbDialogs.h"

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_ADB_SUBSTRINGEXPANSION;

// ----------------------------------------------------------------------------
// types
// ----------------------------------------------------------------------------

// cache: we store pointers to all already created ADBs here (it means that
// once created they're not deleted until the next call to ClearCache)
static ArrayAdbBooks gs_booksCache;

// and the provider (names) for each book
static wxArrayString gs_provCache;

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// helper function: does a recursive search for entries/groups matching the
// given pattern (see AdbManager.h for the possible values of where and how
// paramaters)
static void GroupLookup(
                        ArrayAdbEntries& aEntries,
                        ArrayAdbEntries *aMoreEntries,
                        AdbEntryGroup *pGroup,
                        const String& what,
                        int where,
                        int how,
                        ArrayAdbGroups *aGroups = NULL
                       );

// search in the books specified or in all loaded books otherwise
//
// return true if anything was found, false otherwise
static bool AdbLookupForEntriesOrGroups(
                                        ArrayAdbEntries& aEntries,
                                        ArrayAdbEntries *aMoreEntries,
                                        const String& what,
                                        int where,
                                        int how,
                                        const ArrayAdbBooks *paBooks,
                                        ArrayAdbGroups *aGroups = NULL
                                       );

#define CLEAR_ADB_ARRAY(entries)            \
  {                                         \
    size_t nCount = entries.GetCount();     \
    for ( size_t n = 0; n < nCount; n++ ) { \
      entries[n]->DecRef();                 \
    }                                       \
  }

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// global functions
// ----------------------------------------------------------------------------

// recursive (depth first) search
static void GroupLookup(ArrayAdbEntries& aEntries,
                        ArrayAdbEntries *aMoreEntries,
                        AdbEntryGroup *pGroup,
                        const String& what,
                        int where,
                        int how,
                        ArrayAdbGroups *aGroups)
{
  // check if this book doesn't match itself: the other groups are checked in
  // the parent one but the books don't have a parent
  String nameMatch;
  if ( aGroups ) {
     // we'll use it below
     nameMatch = what.Lower() + _T('*');

     // is it a book?
     if ( !((AdbElement *)pGroup)->GetGroup() ) {
       AdbBook *book = (AdbBook *)pGroup;

       if ( book->GetName().Lower().Matches(nameMatch) ) {
         pGroup->IncRef();

         aGroups->Add(pGroup);
       }
     }
  }

  wxArrayString aNames;
  size_t nGroupCount = pGroup->GetGroupNames(aNames);
  for ( size_t nGroup = 0; nGroup < nGroupCount; nGroup++ ) {
    AdbEntryGroup *pSubGroup = pGroup->GetGroup(aNames[nGroup]);

    GroupLookup(aEntries, aMoreEntries, pSubGroup, what, where, how);

    // groups are matched by name only (case-insensitive)
    if ( aGroups && aNames[nGroup].Lower().Matches(nameMatch) ) {
      aGroups->Add(pSubGroup);
    }
    else {
      pSubGroup->DecRef();
    }
  }

  aNames.Empty();
  size_t nEntryCount = pGroup->GetEntryNames(aNames);
  for ( size_t nEntry = 0; nEntry < nEntryCount; nEntry++ ) {
    AdbEntry *pEntry = pGroup->GetEntry(aNames[nEntry]);

    if ( pEntry->GetField(AdbField_ExpandPriority) == _T("-1") )
    {
      // never use this one for expansion
      pEntry->DecRef();
      continue;
    }

    // we put the entries which match with their nick name in one array and
    // the entries which match anywhere else in the other one: see AdbExpand()
    // to see why
    switch ( pEntry->Matches(what, where, how) ) {
      default:                  // matches elsewhere
        if ( aMoreEntries ) {
          aMoreEntries->Add(pEntry);
          break;
        }
        // else: fall through

      case AdbLookup_NickName:  // match in the entry name
        aEntries.Add(pEntry);
        break;

      case 0:                   // not found at all
        pEntry->DecRef();
        break;
    }
  }
}

static bool
AdbLookupForEntriesOrGroups(ArrayAdbEntries& aEntries,
                            ArrayAdbEntries *aMoreEntries,
                            const String& what,
                            int where,
                            int how,
                            const ArrayAdbBooks *paBooks,
                            ArrayAdbGroups *aGroups)
{
  wxASSERT( aEntries.IsEmpty() );

  if ( paBooks == NULL || paBooks->IsEmpty() )
    paBooks = &gs_booksCache;

  size_t nBookCount = paBooks->Count();
  for ( size_t nBook = 0; nBook < nBookCount; nBook++ ) {
    GroupLookup(aEntries, aMoreEntries,
                (*paBooks)[nBook], what, where, how, aGroups);
  }

  // return true if something found
  return !aEntries.IsEmpty() ||
         (aMoreEntries && !aMoreEntries->IsEmpty()) ||
         (aGroups && !aGroups->IsEmpty());
}

bool AdbLookup(ArrayAdbEntries& aEntries,
               const String& what,
               int where,
               int how,
               AdbEntryGroup *group)
{
   if ( !group )
      return AdbLookupForEntriesOrGroups(aEntries, NULL, what, where, how, NULL);

   // look just in this group
   GroupLookup(aEntries, NULL, group, what, where, how);

   return !aEntries.IsEmpty();
}

bool
AdbExpand(wxArrayString& results, const String& what, int how, wxFrame *frame)
{
  AdbManager_obj manager;
  CHECK( manager, FALSE, _T("can't expand address: no AdbManager") );

  results.Empty();

  if ( what.empty() )
     return FALSE;

  manager->LoadAll();

  static const int lookupMode = AdbLookup_NickName |
                                AdbLookup_FullName |
                                AdbLookup_EMail;

  // check for a group match too
  ArrayAdbGroups aGroups;
  ArrayAdbEntries aEntries,
                  aMoreEntries;

  // expansion may take a long time ...
  if ( frame )
  {
    wxLogStatus(frame,
                _("Looking for matches for '%s' in the address books..."),
                what.c_str());
  }

  MBusyCursor bc;

  if ( AdbLookupForEntriesOrGroups(aEntries, &aMoreEntries, what, lookupMode,
                                   how, NULL, &aGroups ) ) {
    // first of all, if we had any nick name matches, ignore all the other ones
    // but otherwise use them as well
    if ( aEntries.IsEmpty() ) {
      aEntries = aMoreEntries;

      // prevent the dialog below from showing "more matches" button
      aMoreEntries.Clear();
    }

    // merge both arrays into one big one: notice that the order is important,
    // the groups should come first (see below)
    ArrayAdbElements aEverything;
    size_t n;

    size_t nGroupCount = aGroups.GetCount();
    for ( n = 0; n < nGroupCount; n++ ) {
      aEverything.Add(aGroups[n]);
    }

    wxArrayString emails;
    wxString email;
    size_t nEntryCount = aEntries.GetCount();
    for ( n = 0; n < nEntryCount; n++ ) {
      AdbEntry *entry = aEntries[n];

      entry->GetField(AdbField_EMail, &email);
      if ( emails.Index(email) == wxNOT_FOUND ) {
        emails.Add(email);
        aEverything.Add(entry);
      }
      else { // don't propose duplicate entries
        // need to free it here as it won't be freed with all other entries
        // from aEverything below
        entry->DecRef();
      }
    }

    // let the user choose the one he wants
    int rc = AdbShowExpandDialog(aEverything, aMoreEntries, nGroupCount, frame);

    if ( rc != -1 ) {
      size_t index = (size_t)rc;

      if ( index < nGroupCount ) {
        // it's a group, take all entries from it
        AdbEntryGroup *group = aGroups[index];
        wxArrayString aEntryNames;
        size_t count = group->GetEntryNames(aEntryNames);
        for ( n = 0; n < count; n++ ) {
          AdbEntry *entry = group->GetEntry(aEntryNames[n]);
          if ( entry ) {
            results.Add(entry->GetDescription());

            entry->DecRef();
          }
        }

        if ( frame ) {
          wxLogStatus(frame, _("Expanded '%s' using entries from group '%s'"),
                      what.c_str(), group->GetDescription().c_str());
        }
      }
      else {
        // one entry, but in which array?
        size_t count = aEverything.GetCount();
        AdbEntry *entry = index < count ? (AdbEntry *)aEverything[index]
                                        : aMoreEntries[index - count];
        results.Add(entry->GetDescription());

        if ( frame ) {
          String name;
          entry->GetField(AdbField_FullName, &name);
          wxLogStatus(frame, _("Expanded '%s' using entry '%s'"),
                      what.c_str(), name.c_str());
        }
      }
    }
    //else: cancelled by user

    // free all entries and groups
    CLEAR_ADB_ARRAY(aMoreEntries);
    CLEAR_ADB_ARRAY(aEverything);
  }
  else {
    if ( frame )
      wxLogStatus(frame, _("No matches for '%s'."), what.c_str());
  }

  return !results.IsEmpty();
}

bool
AdbExpandSingleAddress(String *address,
                       String *subject,
                       Profile *profile,
                       wxFrame *win)
{
   CHECK( address && subject, false, "NULL input address" );

   subject->clear();

   String textOrig = *address;

   // remove "mailto:" prefix if it's there - this is convenient when you paste
   // in an URL from the web browser
   String newText;
   if ( !textOrig.StartsWith(_T("mailto:"), &newText) )
   {
      // if the text already has a '@' inside it and looks like a full email
      // address assume that it doesn't need to be expanded (this saves a lot
      // of time as expanding a non existing address looks through all address
      // books...)
      size_t pos = textOrig.find('@');
      if ( pos != String::npos && pos > 0 && pos < textOrig.length() )
      {
         // also check that the host part of the address is expanded - it
         // should contain at least one dot normally
         if ( wxStrchr(textOrig.c_str() + pos + 1, '.') )
         {
            // looks like a valid address - nothing to do
            newText = textOrig;
         }
      }

      if ( newText.empty() )
      {
         wxArrayString expansions;

         if ( !AdbExpand(expansions,
                         textOrig,
                         READ_CONFIG(profile, MP_ADB_SUBSTRINGEXPANSION)
                           ? AdbLookup_Substring
                           : AdbLookup_StartsWith,
                         win) )
         {
            // cancelled, indicate it by returning invalid recipient type
            return false;
         }

         // construct the replacement string(s)
         size_t nExpCount = expansions.GetCount();
         for ( size_t nExp = 0; nExp < nExpCount; nExp++ )
         {
            if ( nExp > 0 )
               newText += ", ";

            newText += expansions[nExp];
         }
      }
   }
   else // address following mailto: doesn't need to be expanded
   {
      // this is a non-standard but custom extension: mailto URLs can have
      // extra parameters introduced by '?' and separated by '&'
      const size_t posQuestion  = newText.find('?');
      if ( posQuestion != String::npos )
      {
        size_t posParamStart = posQuestion;
        do
        {
           posParamStart++;
           size_t posEq = newText.find('=', posParamStart);
           if ( posEq != String::npos )
           {
              size_t posParamEnd = newText.find('&', posEq);

              const String param(newText, posParamStart, posEq - posParamStart),
                           value(newText, posEq + 1,
                                 posParamEnd == String::npos
                                   ? posParamEnd
                                   : posParamEnd - posEq - 1);

              if ( wxStricmp(param, _T("subject")) == 0 )
              {
                 *subject = value;
              }
              else
              {
                 // at least cc, bcc and body are also possible but we don't
                 // handle them now
                 wxLogDebug("Ignoring unknown mailto: URL parameter %s=\"%s\"",
                            param.c_str(), value.c_str());
              }

              posParamStart = posParamEnd;
           }
           else // unknown parameter
           {
              wxLogDebug("Ignoring unknown mailto: URL parameter without value");

              posParamStart = newText.find('&', posParamStart);
           }
        }
        while ( posParamStart != String::npos );

        newText.erase(posQuestion, String::npos);
      }
   }

   *address = newText;

   return true;
}

RecipientType
AdbExpandSingleRecipient(String *address,
                         String *subject,
                         Profile *profile,
                         wxFrame *win)
{
   CHECK( address, Recipient_Max, "NULL input address" );

   String text = *address;
   RecipientType rcptType = Recipient_None;
   if ( text.length() > 3 )
   {
      // check for to:, cc: or bcc: prefix
      if ( text[2u] == ':' )
      {
         if ( toupper(text[0u]) == 'T' && toupper(text[1u]) == 'O' )
            rcptType = Recipient_To;
         else if ( toupper(text[0u]) == 'C' && toupper(text[1u]) == 'C' )
            rcptType = Recipient_Cc;
      }
      else if ( text[3u] == ':' && text(0, 3).Upper() == _T("BCC") )
      {
         rcptType = Recipient_Bcc;
      }

      if ( rcptType != Recipient_None )
      {
         // erase the prefix (length 2 or 3) with the colon
         text.erase(0, rcptType == Recipient_Bcc ? 4 : 3);
      }
   }

   if ( !AdbExpandSingleAddress(&text, subject, profile, win) )
     return Recipient_Max;

   *address = text;

   return rcptType;
}

int
AdbExpandAllRecipients(const String& text,
                       wxArrayString& addresses,
                       wxArrayInt& rcptTypes,
                       String *subject,
                       Profile *profile,
                       wxFrame *win)
{
   addresses.clear();
   rcptTypes.clear();

   int count = 0;
   bool expandedSomething = false; // will be set to true if expand any address

   // expand all addresses, one by one
   const size_t len = text.length();
   String address;
   address.reserve(len);
   bool quoted = false;
   for ( size_t n = 0; n <= len; n++ )
   {
      switch ( const wxChar ch = n == len ? '\0' : (wxChar)text[n] )
      {
         case '"':
            quoted = !quoted;
            break;

         case '\\':
            // backslash escapes the next character (if there is one), so
            // append it without special handling, even if it's a quote or
            // address separator
            if ( n < len - 1 )
               address += text[++n];
            else
               wxLogWarning(_("Trailing backslash ignored."));
            break;

         case ' ':
            // skip leading spaces unless they're quoted (note that we don't
            // ignore trailing spaces because we want "Dan " to expand into
            // "Dan Black" but not into "Danish guy")
            if ( quoted || !address.empty() )
               address += ch;
            break;

         case '\0':
            if ( quoted )
            {
               wxLogWarning(_("Closing open quote implicitly."));
               quoted = false;
            }
            // fall through

         case ',':
         case ';':
            if ( !quoted )
            {
               // end of this address component, expand and remember it
               RecipientType rcptType =
                 AdbExpandSingleRecipient(&address, subject, profile, win);
               if ( rcptType != Recipient_Max )
               {
                  expandedSomething = true;
               }
               else // expansion of this address failed
               {
                  // still continue with the other ones but don't set
                  // expandedSomething so that our caller will know if we
                  // didn't do anything at all
                  rcptType = Recipient_None;
               }

               addresses.push_back(address);
               rcptTypes.push_back(rcptType);
               count++;
               address.clear();
               break;
            }
            //else: quoted address separators are not special, fall through

         default:
            // just accumulate in the current address
            address += ch;
      }
   }

   return expandedSomething ? count : -1;
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

    // artificially bump up the ref count so it will be never deleted (if the
    // calling code behaves properly) until the very end of the application
    ms_pManager->IncRef();

    wxLogTrace(_T("adb"), _T("AdbManager created."));
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
               _T("AdbManager::Unget() called without matching Get()!") );

  if ( !ms_pManager->DecRef() ) {
    // the object deleted itself
    ms_pManager = NULL;

    wxLogTrace(_T("adb"), _T("AdbManager deleted."));
  }
}

// force the deletion of ms_pManager
void AdbManager::Delete()
{
  // we should only be called when the program terminates, otherwise some
  // objects could still have references to us
  ASSERT_MSG( !mApplication->IsRunning(),
              _T("AdbManager::Delete() called, but the app is still running!") );

#ifdef DEBUG
  size_t count = 0;
#endif

  while ( ms_pManager ) {
    #ifdef DEBUG
      count++;
    #endif

    Unget();
  }

  // there should be _exactly_ one extra IncRef(), not several
  ASSERT_MSG( count < 2, _T("Forgot AdbManager::Unget() somewhere") );
}

// ----------------------------------------------------------------------------
// AdbManager public methods
// ----------------------------------------------------------------------------

// create a new address book using specified provider or trying all of them
// if it's NULL
AdbBook *AdbManager::CreateBook(const String& name,
                                AdbDataProvider *provider,
                                String *providerName)
{
   AdbBook *book = NULL;

   // first see if we don't already have it
   book = FindInCache(name, provider);
   if ( book )
   {
      book->IncRef();
      return book;
   }

   // no, must create a new one
   AdbDataProvider *prov = NULL;
   if ( provider )
   {
      prov = provider;
      prov->IncRef();
   }
   if ( prov == NULL )
      prov = AutodetectFormat(name);
   if ( prov == NULL )
      prov = CreateNative(name);
   if ( prov == NULL )
      prov = MatchBookName(name);

   if ( providerName && prov )
   {
      // return the provider name if asked for it
      *providerName = prov->GetProviderName();
   }
   
   if ( prov ) {
      book = prov->CreateBook(name);
   }

   if ( book == NULL ) {
      wxLogError(_("Can't open the address book '%s'."), name.c_str());
   }
   else {
      book->IncRef();
      gs_booksCache.Add(book);
      gs_provCache.Add(prov->GetProviderName());
   }

   if ( prov )
      prov->DecRef();

   return book;
}

/*static*/ AdbDataProvider *AdbManager::AutodetectFormat(const String& name)
{
   for ( AdbDataProvider::AdbProviderInfo *info
      = AdbDataProvider::ms_listProviders; info; info = info->pNext )
   {
      AdbDataProvider *prov = info->CreateProvider();
      
      if ( prov->TestBookAccess(name,
         AdbDataProvider::Test_AutodetectCapable)
         && prov->TestBookAccess(name, AdbDataProvider::Test_OpenReadOnly) )
      {
         return prov;
      }

      prov->DecRef();
   }
   
   return NULL;
}

/*static*/ AdbDataProvider *AdbManager::CreateNative(const String& name)
{
   AdbDataProvider *prov = AdbDataProvider::GetNativeProvider();
   
   if ( prov->TestBookAccess(name, AdbDataProvider::Test_Create) )
      return prov;
      
   prov->DecRef();
   return NULL;
}

/*static*/ AdbDataProvider *AdbManager::MatchBookName(const String& name)
{
   for ( AdbDataProvider::AdbProviderInfo *info
      = AdbDataProvider::ms_listProviders; info; info = info->pNext )
   {
      AdbDataProvider *prov = info->CreateProvider();
      
      if ( prov->TestBookAccess(name, AdbDataProvider::Test_RecognizesName)
         && prov->TestBookAccess(name, AdbDataProvider::Test_Create) )
         return prov;

      prov->DecRef();
   }
   
   return NULL;
}

size_t AdbManager::GetBookCount() const
{
  ASSERT_MSG( gs_provCache.Count() == gs_booksCache.Count(),
              _T("mismatch between gs_booksCache and gs_provCache!") );

  return gs_booksCache.Count();
}

AdbBook *AdbManager::GetBook(size_t n) const
{
  AdbBook *book = gs_booksCache[n];
  book->IncRef();

  return book;
}

// FIXME it shouldn't even know about AdbEditor existence! but this would
// involve changing AdbFrame.cpp to use this function somehow and I don't have
// the time to do it right now...
void AdbManager::LoadAll()
{
  wxConfigBase *conf = mApplication->GetProfile()->GetConfig();
  conf->SetPath(GetAdbEditorConfigPath());

  wxArrayString astrAdb, astrProv;
  RestoreArray(conf, astrAdb, _T("AddressBooks"));
  RestoreArray(conf, astrProv, _T("Providers"));

  wxString strProv;
  AdbDataProvider *pProvider;
  size_t nCount = astrAdb.Count();
  for ( size_t n = 0; n < nCount; n++ ) {
    if ( n < astrProv.Count() )
      strProv = astrProv[n];
    else
      strProv.Empty();

    if ( strProv.empty() )
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
  size_t nCount = gs_booksCache.Count();
  for ( size_t n = 0; n < nCount; n++ ) {
    gs_booksCache[n]->DecRef();
  }

  gs_booksCache.Clear();

  gs_provCache.Clear();
}

AdbManager::~AdbManager()
{
  ClearCache();
}

// ----------------------------------------------------------------------------
// AdbManager private methods
// ----------------------------------------------------------------------------

AdbBook *AdbManager::FindInCache(const String& name,
                                 const AdbDataProvider *provider) const
{
  AdbBook *book;
  size_t nCount = gs_booksCache.Count();
  for ( size_t n = 0; n < nCount; n++ ) {
    if ( provider && provider->GetProviderName() != gs_provCache[n] ) {
      // don't compare books belonging to different providers
      continue;
    }

    book = gs_booksCache[n];
    if ( book->IsSameAs(name) )
      return book;
  }

  return NULL;
}

// ----------------------------------------------------------------------------
// AdbManager debugging support
// ----------------------------------------------------------------------------

#ifdef DEBUG

String AdbManager::DebugDump() const
{
  String str = MObjectRC::DebugDump();
  str << (int)gs_booksCache.Count() << _T("books in cache");

  return str;
}

#endif // DEBUG

/* vi: set ts=2 sw=2: */
