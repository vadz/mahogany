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

class Profile;

// see near definition of this symbol in src/modules/PalmOS.cpp
#ifndef M_DONT_DEFINE_ADDRESS

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

/**
  Address class represents a single RFC 822 address. The RFC 822 groups are not
  (well) supported for the moment, i.e. the group information is lost.
 */
class Address
{
public:
   /// is the address valid?
   virtual bool IsValid() const = 0;

   /// get the full address as a string ("Vadim Zeitlin <vadim@wxwindows.org>")
   virtual String GetAddress() const = 0;

   /// get the personal name part, may be empty ("Vadim Zeitlin")
   virtual String GetName() const = 0;

   /// get the local part of the address ("vadim")
   virtual String GetMailbox() const = 0;

   /// get the domain part of the address ("wxwindows.org")
   virtual String GetDomain() const = 0;

   /// get the address part ("vadim@wxwindows.org")
   virtual String GetEMail() const = 0;

   /// compare 2 addresses for equality
   bool operator==(const Address& addr) const { return IsSameAs(addr); }

   /// compare address with a string
   bool operator==(const String& address) const;

   /**
     Removes Re: Re[n]: Re(n): and the local translation of
     it from the beginning of a subject line - used to convert the subject to
     the canonical form
    */
   static String NormalizeSubject(const String& subject);

   /**
    */
   static String BuildFullForm(const String& personal, const String& address);

protected:
   /// must have default ctor because we declare copy ctor private
   Address() { }

   /// normally the user code doesn't delete us
   virtual ~Address();

   /// comparison function
   virtual bool IsSameAs(const Address& addr) const = 0;

private:
   /// no assignment operator/copy ctor as AddressList handles us
   DECLARE_NO_COPY_CLASS(Address)

   GCC_DTOR_WARN_OFF
};

// ----------------------------------------------------------------------------
// AddressList: an object representing a comma separated string of Addresses
// ----------------------------------------------------------------------------

/**
  AddressList basicly converts between string representation and the list of
  addresses. Note that the pointers returned by GetFirst()/GetNext() belong to
  the list and will be deleted by it, so you can *not* use them after deleting
  the list.
 */
class AddressList : public MObjectRC
{
public:
   /// create the address list from string (may be empty)
   static AddressList *Create(const String& address,
                              const String& defhost = wxEmptyString);

   /// create the "From" address using settings in this profile
   static AddressList *CreateFromAddress(Profile *profile);

   /// get the first address in the list, return NULL if list is empty
   virtual Address *GetFirst() const = 0;

   /// get the next address in the list, return NULL if no more
   virtual Address *GetNext(const Address *addr) const = 0;

   /// check if there is a next address
   bool HasNext(const Address *addr) const;

   /// get the comma separated string containing all addresses
   virtual String GetAddresses() const = 0;

   /// comparison function
   virtual bool IsSameAs(const AddressList *addr) const = 0;

protected:
   /// must have default ctor because we declarae copy ctor private
   AddressList() { }

private:
   /// no assignment operator/copy ctor as we always use pointers, not objects
   DECLARE_NO_COPY_CLASS(AddressList)

   GCC_DTOR_WARN_OFF
};

/// declare AddressList_obj class, smart reference to AddressList
BEGIN_DECLARE_AUTOPTR(AddressList);
public:
   AddressList_obj(const String& address, const String& defhost = wxEmptyString)
      { m_ptr = AddressList::Create(address, defhost); }
END_DECLARE_AUTOPTR();

/// declare global comparison operator for addresses
extern bool operator==(const AddressList_obj& addrList1,
                       const AddressList_obj& addrList2);

#endif // M_DONT_DEFINE_ADDRESS


/// Values for ContainsOwnAddress() last parameter
enum OwnAddressKind
{
   /// The addresses the user uses for "From:"
   OwnAddress_From,

   /// The addresses which can reach this user (this includes MLs, ...)
   OwnAddress_To,

   /// End of enum marker
   OwnAddress_Max
};

/**
   Check whether the given string contains at least one of our own addresses.

   Check if the given address apepars in list of our own addresses, as defined
   by kind parameter.

   @param str the address to check
   @param profile to get the own addresses from
   @param kind of the "own" addresses to check
   @param own the address which did match, if any
 */
extern bool ContainsOwnAddress(const String& str,
                               Profile *profile,
                               OwnAddressKind kind = OwnAddress_To,
                               String *own = NULL);

/// Modifies <mailto:(.*)> to <$1> and removes other <[a-z]+:.*> parts
extern String FilterAddressList(const String& original);

#endif // _ADDRESS_H_

