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
   // create the address from cclient ADDRESS struct
   AddressCC(mail_address *adr);

   // implement the base class pure virtuals  
   virtual bool IsValid() const;
   virtual String GetAddress() const;
   virtual String GetName() const;
   virtual String GetMailbox() const;
   virtual String GetDomain() const;
   virtual String GetEMail() const;

protected:
   virtual bool IsSameAs(const Address& addr) const;

private:
   // the cclient ADDRESS struct we correspond to (we own and will delete it!)
   mail_address *m_adr;

   // the next address (NB: we always have "m_addrNext->m_adr == m_adr->next")
   AddressCC *m_addrNext;

   // it accesses both m_adr and m_addrNext
   friend class AddressListCC;
};

// ----------------------------------------------------------------------------
// AddressListCC
// ----------------------------------------------------------------------------

class AddressListCC : public AddressList
{
public:
   // create the address from cclient ADDRESS struct
   AddressListCC(mail_address *adr);

   // free the addresses
   virtual ~AddressListCC();

   // implement the base class pure virtuals  
   virtual Address *GetFirst() const;
   virtual Address *GetNext(const Address *addr) const;
   virtual String GetAddresses() const;
   virtual bool IsSameAs(const AddressList *addr) const;

private:
   // pointer to the head of the linked list of addresses
   AddressCC *m_addrCC;

   MOBJECT_DEBUG(AddressListCC)
};

#endif // _ADDRESSCC_H_


