//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   Address.h: represents an rfc822 address header
// Purpose:     an Address object represents one or several addresses
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2001
// CVS-ID:      $Id$
// Copyright:   (c) 2001 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _ADDRESS_H_
#define _ADDRESS_H_

#ifdef __GNUG__
   #pragma interface "Address.h"
#endif

#include "MObject.h"

/**
   Note on variable names: we have a lot of addresses here (strings, objects of
   address class, cclient addresses...) so we name them in a consitent manner
   to minimize the confusion:

   address is a string
   addr is Address class object
   addrCC is AddressCC class object (which can also be called addr sometimes)
   adr (single 'd'!) is a cclient ADDRESS
*/

// ----------------------------------------------------------------------------
// Address
// ----------------------------------------------------------------------------

class Address : public MObjectRC
{
public:
   /// create the address from string
   static Address *Create(const String& address);

   /// is the address valid?
   virtual bool IsValid() const = 0;

   /// get the full address as a string
   virtual String GetAddress() const = 0;

   /// get the name (comment) part of the address
   virtual String GetName() const = 0;

   /// get the address part
   virtual String GetEMail() const = 0;

   /// compare 2 addresses for equality
   bool operator==(const Address& addr) const { return IsSameAs(addr); }

   /// compare address with a string
   bool operator==(const String& address) const;

protected:
   /// comparison function
   virtual bool IsSameAs(const Address& addr) const = 0;
};

/// declare Address_obj class, smart reference to Address
BEGIN_DECLARE_AUTOPTR(Address);
public:
   Address_obj(const String& address) { m_ptr = Address::Create(address); }
END_DECLARE_AUTOPTR();

#endif // _ADDRESS_H_

