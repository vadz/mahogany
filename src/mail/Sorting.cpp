///////////////////////////////////////////////////////////////////////////////
// Project:     Mahogany - cross platform e-mail GUI client
// File name:   mail/Sorting.cpp - sorting related stuff
// Purpose:     implements SortParams and a couple of related functions
// Author:      Vadim Zeitlin
// Modified by:
// Created:     30.08.01
// CVS-ID:      $Id$
// Copyright:   (c) 2001 Vadim Zeitlin
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
   #include "Sorting.h"
   #include "Mdefaults.h"
   #include "strutil.h"    // for strutil_restore_array()
#endif // USE_PCH

// ----------------------------------------------------------------------------
// options we use
// ----------------------------------------------------------------------------

extern const MOption MP_MSGS_SORTBY;
extern const MOption MP_FVIEW_FROM_REPLACE;
extern const MOption MP_FROM_ADDRESS;
extern const MOption MP_FROM_REPLACE_ADDRESSES;

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// SortParams
// ----------------------------------------------------------------------------

SortParams::SortParams()
{
   sortOrder = MSO_NONE;

   detectOwnAddresses = false;
}

void SortParams::Read(Profile *profile)
{
   sortOrder = READ_CONFIG(profile, MP_MSGS_SORTBY);
   detectOwnAddresses = READ_CONFIG_BOOL(profile, MP_FVIEW_FROM_REPLACE);
   if ( detectOwnAddresses )
   {
      String returnAddrs = READ_CONFIG(profile, MP_FROM_REPLACE_ADDRESSES);
      if ( returnAddrs == GetStringDefault(MP_FROM_REPLACE_ADDRESSES) )
      {
         // the default for this option is just the return address
         returnAddrs = READ_CONFIG_TEXT(profile, MP_FROM_ADDRESS);
      }

      ownAddresses = strutil_restore_array(returnAddrs);
   }
}

bool SortParams::operator==(const SortParams& other) const
{
   return sortOrder == other.sortOrder &&
          detectOwnAddresses == other.detectOwnAddresses &&
          (!detectOwnAddresses || ownAddresses == other.ownAddresses);
}

// ----------------------------------------------------------------------------
// sort order conversions: array <-> long
// ----------------------------------------------------------------------------

// split a long value (as read from profile) into (several) sort orders
wxArrayInt SplitSortOrder(long sortOrder)
{
   wxArrayInt sortOrders;
   while ( sortOrder )
   {
      sortOrders.Add(GetSortCrit(sortOrder));
      sortOrder = GetSortNextCriterium(sortOrder);
   }

   return sortOrders;
}

// combine several (max 8) sort orders into one value
long BuildSortOrder(const wxArrayInt& sortOrders)
{
   long sortOrder = 0l;

   size_t count = sortOrders.GetCount();
   if ( count > 8 )
      count = 8;
   for ( size_t n = count + 1; n > 1; n-- )
   {
      sortOrder <<= 4;
      sortOrder |= sortOrders[n - 2];
   }

   return sortOrder;
}

