///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   mail/AddressCC.cpp: Address class implementation using cclient
// Purpose:     AddressCC maps more or less directly onto cclient ADDRESS
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
   #pragma implementation "AddressCC.h"
#endif

#include  "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"
#endif // USE_PCH

#include "AddressCC.h"

#include <c-client.h>

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// AddressCC creation
// ----------------------------------------------------------------------------

/* static */
AddressCC *AddressCC::Create(const ADDRESS *adr)
{
   CHECK( adr, NULL, "can't create AddressCC from NULL ADDRESS" );

   AddressCC *addrCC = new AddressCC;
   if ( adr->error )
   {
      // this is not supposed to happen - if there is an error in the address,
      // it should be the last one
      ASSERT_MSG( !adr->next, "unexpected kind of invalid address" );

      addrCC->m_adr = NULL;
   }
   else // valid address
   {
      addrCC->m_adr = rfc822_cpy_adr((ADDRESS *)adr); // const_cast
   }

   return addrCC;
}

AddressCC::~AddressCC()
{
   mail_free_address(&m_adr);
}

// ----------------------------------------------------------------------------
// AddressCC accessors
// ----------------------------------------------------------------------------

bool AddressCC::IsValid() const
{
   return m_adr != NULL;
}

String AddressCC::GetAddress() const
{
   String address;
   CHECK( IsValid(), address, "invalid address" );

   rfc822_write_address(address.GetWriteBuf(1024), m_adr);
   address.UngetWriteBuf();

   return address;
}

String AddressCC::GetName() const
{
}

String AddressCC::GetEMail() const
{
}

// ----------------------------------------------------------------------------
// AddressCC comparison
// ----------------------------------------------------------------------------

bool AddressCC::IsSameAs(const Address& addr) const
{
   CHECK( addr.IsValid(), false, "can't compare invalid addresses" );

   // ignore the name parts when comparing
   return false;
}

