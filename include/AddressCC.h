//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   AddressCC.h: Address class implementation using cclient
// Purpose:
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2001
// CVS-ID:      $Id$
// Copyright:   (c) 2001 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _ADDRESSCC_H_
#define _ADDRESSCC_H_

#ifdef __GNUG__
   #pragma interface "AddressCC.h"
#endif

#include "Address.h"

// cclient's ADDRESS is #defined as struct mail_address, but we don't have this
// declaration here and we can't forward declare a #define
struct mail_address;

// ----------------------------------------------------------------------------
// AddressCC
// ----------------------------------------------------------------------------

class AddressCC : public Address
{
public:
   /// create the address from cclient ADDRESS struct
   static AddressCC *Create(const mail_address *adr);

   /// is the address valid?
   virtual bool IsValid() const;

   /// get the full address as a string
   virtual String GetAddress() const;

   /// get the name (comment) part of the address
   virtual String GetName() const;

   /// get the address part
   virtual String GetEMail() const;

protected:
   /// comparison function
   virtual bool IsSameAs(const Address& addr) const;

   /// dtor deletes m_adr too!
   virtual ~AddressCC();

private:
   /// the cclient ADDRESS (we own it and will delete it)
   mail_address *m_adr;
};

#endif // _ADDRESSCC_H_


