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

#include "Mcclient.h"

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// return string containing all addresses or just the first one from this list
static String Adr2String(ADDRESS *adr, bool all = true);

// ============================================================================
// AddressCC implementation
// ============================================================================

// ----------------------------------------------------------------------------
// AddressCC creation and destruction
// ----------------------------------------------------------------------------

AddressCC::AddressCC(ADDRESS *adr)
{
   ASSERT_MSG( adr && !adr->error, "invalid ADDRESS in AddressCC ctor" );

   m_adr = adr;
   m_addrNext = NULL;
}

// ----------------------------------------------------------------------------
// AddressCC accessors
// ----------------------------------------------------------------------------

bool AddressCC::IsValid() const
{
   return m_adr && !m_adr->error;
}

String AddressCC::GetAddress() const
{
   return Adr2String(m_adr, false /* just the first one */);
}

String AddressCC::GetName() const
{
   String personal;
   CHECK( m_adr, personal, "invalid address" );

   personal = m_adr->personal;

   return personal;
}

String AddressCC::GetMailbox() const
{
   String mailbox;
   CHECK( m_adr, mailbox, "invalid address" );

   mailbox = m_adr->mailbox;

   return mailbox;
}

String AddressCC::GetDomain() const
{
   String host;
   CHECK( m_adr, host, "invalid address" );

   host = m_adr->host;

   return host;
}

String AddressCC::GetEMail() const
{
   String email;

   // FIXME: again, c-client doesn't check size of the buffer!
   char *buf = email.GetWriteBuf(2048);
   *buf = '\0';
   rfc822_address(buf, m_adr);
   email.UngetWriteBuf();
   email.Shrink();

   return email;
}

// ----------------------------------------------------------------------------
// AddressCC comparison
// ----------------------------------------------------------------------------

bool AddressCC::IsSameAs(const Address& addr) const
{
   CHECK( IsValid() && addr.IsValid(), false,
          "can't compare invalid addresses" );

   const AddressCC& addrCC = (const AddressCC&)addr;

   // ignore the personal name part
   //
   // we also ignore adl field - IMHO the addresses differing only by source
   // route should be considered identical, shouldn't they?
   return wxStricmp(m_adr->mailbox, addrCC.m_adr->mailbox) == 0 &&
          wxStricmp(m_adr->host, addrCC.m_adr->host) == 0;
}

// ============================================================================
// AddressListCC implementation
// ============================================================================

// ----------------------------------------------------------------------------
// AddressListCC creation/destruction
// ----------------------------------------------------------------------------

AddressListCC::AddressListCC(mail_address *adr)
{
   m_addrCC = NULL;
   AddressCC *addrCur = m_addrCC;

   while ( adr )
   {
      if ( adr->error )
      {
         DBGMESSAGE(("Skipping bad address '%s'.", Adr2String(adr).c_str()));
      }
      else // good address, store
      {
         AddressCC *addr = new AddressCC(adr);

         if ( !addrCur )
         {
            // first address, remember as head of the linked list
            m_addrCC = addr;
         }
         else // not first address, add to the linked list
         {
            addrCur->m_addrNext = addr;
         }

         addrCur = addr;
      }

      adr = adr->next;
   }
}

AddressListCC::~AddressListCC()
{
   if ( m_addrCC )
   {
      // this will free all ADDRESS structs as they are linked together
      mail_free_address(&m_addrCC->m_adr);

      // and now delete our wrappers
      while ( m_addrCC )
      {
         AddressCC *addr = m_addrCC->m_addrNext;

         delete m_addrCC;
         m_addrCC = addr;
      }
   }
}

// ----------------------------------------------------------------------------
// AddressListCC enumeration
// ----------------------------------------------------------------------------

/* static */
AddressList *AddressList::Create(const String& address)
{
   ADDRESS *adr = NULL;

   if ( !address.empty() )
   {
      // rfc822_parse_adrlist() modifies the string passed in
      char *addressCopy = strdup(address);
      rfc822_parse_adrlist(&adr, addressCopy, NULL /* default host */);
      free(addressCopy);

      if ( !adr || adr->error )
      {
         DBGMESSAGE(("Invalid RFC822 address '%s'.", address.c_str()));

         return NULL;
      }
   }

   return new AddressListCC(adr);
}

Address *AddressListCC::GetFirst() const
{
   return m_addrCC;
}

Address *AddressListCC::GetNext(const Address *addr) const
{
   CHECK( addr, NULL, "NULL address in AddressList::GetNext" );

   return ((AddressCC *)addr)->m_addrNext;
}

// ----------------------------------------------------------------------------
// AddressListCC other operations
// ----------------------------------------------------------------------------

String AddressListCC::GetAddresses() const
{
   String address;
   if ( m_addrCC )
   {
      address = Adr2String(m_addrCC->m_adr);
   }

   return address;
}

bool AddressListCC::IsSameAs(const AddressList *addrListOther) const
{
   Address *addr = GetFirst();
   Address *addrOther = addrListOther->GetFirst();

   // empty address list is invalid, so it should never compare equally with
   // anything else
   if ( !addr )
   {
      return !addrOther;
   }

   while ( addr && addrOther && *addr == *addrOther )
   {
      addr = GetNext(addr);
      addrOther = addrListOther->GetNext(addrOther);
   }

   // return true if all addresses were equal and if there are no more left
   return !addr && !addrOther;
}

#ifdef DEBUG

String AddressListCC::DebugDump() const
{
   String str = MObjectRC::DebugDump();
   str << "address list";
   if ( m_addrCC )
   {
      str << " (" << Adr2String(m_addrCC->m_adr) << ')';
   }

   return str;
}

#endif // DEBUG

// ============================================================================
// private functions implementation
// ============================================================================

static String Adr2String(ADDRESS *adr, bool all)
{
   String address;
   CHECK( adr, address, "invalid address" );

   // if we need only one address, prevent rfc822_write_address() from
   // traversing the entire address list by temporarily setting the next
   // pointer to NULL
   ADDRESS *adrNextOld = all ? NULL : adr->next;
   if ( adrNextOld )
      adr->next = NULL;

   // FIXME: there is no way to get the address length from c-client, how to
   //        prevent it from overwriting our buffer??
   char *buf = address.GetWriteBuf(4096);
   *buf = '\0';
   rfc822_write_address(buf, adr);
   address.UngetWriteBuf();
   address.Shrink();

   if ( adrNextOld )
      adr->next = adrNextOld;

   return address;
}

