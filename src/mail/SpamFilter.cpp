///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   src/mail/SpamFilter.cpp
// Purpose:     implements static methods of SpamFilter
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2004-07-04
// CVS-ID:      $Id$
// Copyright:   (c) 2004 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#include "pointers.h"
#include "MAtExit.h"

#include "SpamFilter.h"

// ----------------------------------------------------------------------------
// local globals
// ----------------------------------------------------------------------------

// ensure that filters are cleaned up on exit
static MRunFunctionAtExit gs_runFilterCleanup(SpamFilter::UnloadAll);

// ============================================================================
// SpamFilter implementation
// ============================================================================

SpamFilter *SpamFilter::ms_first = NULL;
bool SpamFilter::ms_loaded = false;

// ----------------------------------------------------------------------------
// forward the real operations to all filters in the spam chain
// ----------------------------------------------------------------------------

/* static */
void SpamFilter::ReclassifyAsSpam(const Message& msg, bool isSpam)
{
   LoadAll();

   for ( SpamFilter *p = ms_first; p; p = p->m_next )
   {
      p->Reclassify(msg, isSpam);
   }
}

/* static */
bool SpamFilter::CheckIfSpam(const Message& msg, float *probability)
{
   LoadAll();

   for ( SpamFilter *p = ms_first; p; p = p->m_next )
   {
      if ( p->Process(msg, probability) )
         return true;
   }

   return false;
}

// ----------------------------------------------------------------------------
// loading/unloading spam filters
// ----------------------------------------------------------------------------

/* static */
void SpamFilter::DoLoadAll()
{
   ms_loaded = true;

   // load the listing of all available spam filters
   RefCounter<MModuleListing>
      listing(MModule::ListAvailableModules(SPAM_FILTER_INTERFACE));
   if ( !listing )
      return;

   // found some, now create them
   const size_t count = listing->Count();
   for ( size_t n = 0; n < count; n++ )
   {
      const MModuleListingEntry& entry = (*listing)[n];
      const String& name = entry.GetName();
      MModule * const module = MModule::LoadModule(name);
      if ( !module )
      {
         wxLogError(_("Failed to load spam filter \"%s\"."), name.c_str());
         continue;
      }

      SpamFilterFactory * const
         factory = static_cast<SpamFilterFactory *>(module);

      // create the filter and insert it into the linked list
      SpamFilter *next = ms_first;
      ms_first = factory->Create();
      ms_first->m_next = next;

      factory->DecRef();
   }
}

/* static */
void SpamFilter::UnloadAll()
{
   if ( ms_loaded )
   {
      // free all currently loaded filters
      while ( ms_first )
      {
         SpamFilter *p = ms_first;
         ms_first = p->m_next;

         p->DecRef();
      }

      ms_loaded = false;
   }
}

