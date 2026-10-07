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
   bool IsValid() const override;
   String GetAddress() const override;
   String GetName() const override;
   String GetMailbox() const override;
   String GetDomain() const override;
   String GetEMail() const override;

protected:
   bool IsSameAs(const Address& addr) const override;

private:
   // the cclient ADDRESS struct we correspond to (we own and will delete it!)
   mail_address *m_adr;

   // the next address (NB: we always have "m_addrNext->m_adr == m_adr->next")
   AddressCC *m_addrNext;

   // it accesses both m_adr and m_addrNext
   friend class AddressListCC;

   DECLARE_NO_COPY_CLASS(AddressCC)
};

// ----------------------------------------------------------------------------
// AddressListCC
// ----------------------------------------------------------------------------

class AddressListCC : public AddressList
{
public:
   // create by copying an already existing ADDRESS struct (we don't take
   // ownership of it)
   static AddressList *Create(const mail_address *adr);

   // implement the base class pure virtuals
   Address *GetFirst() const override;
   Address *GetNext(const Address *addr) const override;
   String GetAddresses() const override;
   bool IsSameAs(const AddressList *addr) const override;

private:
   // create the address from cclient ADDRESS struct, we take ownership of it!
   AddressListCC(mail_address *adr);

   // free the addresses
   virtual ~AddressListCC();

   // pointer to the head of the linked list of addresses
   AddressCC *m_addrCC;

   // the header from which the addresses were extracted (may be empty)
   String m_addressHeader;

   // these methods use our private ctor
   friend AddressList *AddressList::Create(const String& address,
                                           const String& defHost,
                                           wxFontEncoding enc);

   MOBJECT_DEBUG(AddressListCC)
   DECLARE_NO_COPY_CLASS(AddressListCC)
};

// wrapper around rfc822_parse_adrlist() c-client function
extern
mail_address *ParseAddressList(const String& address,
                               const String& defhost,
                               wxFontEncoding enc);

#endif // _ADDRESSCC_H_


