///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   mail/Address.cpp: Address class implementation
// Purpose:     basic address functions, all interesting stuff is in AddressCC
// Author:      Vadim Zeitlin
// Modified by:
// Created:     04.05.01
// CVS-ID:      $Id$
// Copyright:   (c) 2001 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#ifdef __GNUG__
   #pragma implementation "Address.h"
#endif

#include  "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"
#endif // USE_PCH

#include "Address.h"

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// Address
// ----------------------------------------------------------------------------

bool Address::operator==(const String& address) const
{
   CHECK( IsValid(), false, "can't compare invalid addresses" );

   AddressList_obj addrList(address);
   Address_obj addr = addrList->GetFirst();
   if ( !addr || addrList->HasNext(addr) )
   {
      return false;
   }

   return addr->IsSameAs(*this);
}

// ----------------------------------------------------------------------------
// AddressList
// ----------------------------------------------------------------------------

bool AddressList::HasNext(const Address *addr) const
{
   Address *addrNext = GetNext(addr);
   if ( !addrNext )
      return false;

   addrNext->DecRef();

   return true;
}
