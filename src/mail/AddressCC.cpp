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
#include "Mdefaults.h"
#include "Profile.h"

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_ADD_DEFAULT_HOSTNAME;
extern const MOption MP_FROM_ADDRESS;
extern const MOption MP_HOSTNAME;
extern const MOption MP_PERSONALNAME;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// Adr2String parameters
enum Adr2StringWhich
{
   Adr2String_All,
   Adr2String_FirstOnly
};

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// return string containing all addresses or just the first one from this list
static String Adr2String(ADDRESS *adr,
                         Adr2StringWhich which = Adr2String_All,
                         bool *error = NULL);

// return the string containing just the email part of the address (this always
// works with one address only, not the entire list)
static String Adr2Email(ADDRESS *adr);

// ============================================================================
// AddressCC implementation
// ============================================================================

// ----------------------------------------------------------------------------
// AddressCC creation and destruction
// ----------------------------------------------------------------------------

AddressCC::AddressCC(ADDRESS *adr)
{
   ASSERT_MSG( adr && !adr->error, _T("invalid ADDRESS in AddressCC ctor") );

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
   return Adr2String(m_adr, Adr2String_FirstOnly);
}

String AddressCC::GetName() const
{
   String personal;
   CHECK( m_adr, personal, _T("invalid address") );

   personal = m_adr->personal;

   return personal;
}

String AddressCC::GetMailbox() const
{
   String mailbox;
   CHECK( m_adr, mailbox, _T("invalid address") );

   mailbox = m_adr->mailbox;

   return mailbox;
}

String AddressCC::GetDomain() const
{
   String host;
   CHECK( m_adr, host, _T("invalid address") );

   host = m_adr->host;

   return host;
}

String AddressCC::GetEMail() const
{
   return Adr2Email(m_adr);
}

// ----------------------------------------------------------------------------
// AddressCC comparison
// ----------------------------------------------------------------------------

// case insensitive string compare of strings which may be NULL
static inline bool SafeCompare(const char *s1, const char *s2)
{
   if ( !s1 || !s2 )
      return !s1 == !s2;

   return wxStricmp(s1, s2) == 0;
}

bool AddressCC::IsSameAs(const Address& addr) const
{
   CHECK( IsValid() && addr.IsValid(), false,
          _T("can't compare invalid addresses") );

   const AddressCC& addrCC = (const AddressCC&)addr;

   // ignore the personal name part
   //
   // we also ignore adl field - IMHO the addresses differing only by source
   // route should be considered identical, shouldn't they?
   return SafeCompare(m_adr->mailbox, addrCC.m_adr->mailbox) &&
          SafeCompare(m_adr->host, addrCC.m_adr->host);
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
      // c-client mungles unqualified addresses, undo it
      if ( adr->host && !strcmp(adr->host, BADHOST) )
      {
         // the only way to get an address without the hostname is to use
         // '@' as hostname (try to find this in c-client docs!)
         fs_give((void **)&(adr->host));
         adr->host = (char *)fs_get(2);
         adr->host[0] = '@';
         adr->host[1] = '\0';;
      }

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

/* static */
AddressList *AddressListCC::Create(const mail_address *adr)
{
   ADDRESS *adrCopy;
   if ( adr )
   {
      // copy the address as AddressListCC takes ownership of it and will free
      // it
      adrCopy = rfc822_cpy_adr((ADDRESS *)adr);
   }
   else
   {
      // it's not an error, this may happen if there are no valid addresses in
      // the message
      adrCopy = NULL;
   }

   return new AddressListCC(adrCopy);
}

/* static */
AddressList *AddressList::CreateFromAddress(Profile *profile)
{
   // it is a bit difficult for From because we have 2 entries in config to
   // specify it (for historic reasons mainly, I don't think this is actually
   // useful) and so we must combine them together

   // set personal name
   ADDRESS *adr = mail_newaddr();
   adr->personal = cpystr(READ_CONFIG_TEXT(profile, MP_PERSONALNAME));

   // set mailbox/host
   String email = READ_CONFIG(profile, MP_FROM_ADDRESS);
   size_t pos = email.find('@');
   if ( pos != String::npos )
   {
      adr->mailbox = cpystr(email.substr(0, pos).c_str());
      adr->host = cpystr(email.c_str() + pos + 1);
   }
   else // no '@'?
   {
      adr->mailbox = cpystr(email);

      String host;
      if ( READ_CONFIG(profile, MP_ADD_DEFAULT_HOSTNAME) )
      {
         host = READ_CONFIG_TEXT(profile, MP_HOSTNAME);
      }

      if ( host.empty() )
      {
         // trick c-client into accepting addresses without host names
         // instead of using a stupid MISSING.WHATEVER instead of the host
         // part
         host = '@';
      }

      adr->host = cpystr(host);
   }

   return new AddressListCC(adr);
}

/* static */
AddressList *AddressList::Create(const String& address, const String& defhost)
{
   ADDRESS *adr = NULL;

   if ( !address.empty() )
   {
      adr = ParseAddressList(address, defhost);

      if ( !adr || adr->error )
      {
         DBGMESSAGE(("Invalid RFC822 address '%s'.", address.c_str()));

         return NULL;
      }
   }

   AddressListCC *addrList = new AddressListCC(adr);
   addrList->m_addressHeader = address;

   return addrList;
}

// ----------------------------------------------------------------------------
// AddressListCC enumeration
// ----------------------------------------------------------------------------

Address *AddressListCC::GetFirst() const
{
   return m_addrCC;
}

Address *AddressListCC::GetNext(const Address *addr) const
{
   CHECK( addr, NULL, _T("NULL address in AddressList::GetNext") );

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
      bool error;
      address = Adr2String(m_addrCC->m_adr, Adr2String_All, &error);

      // when an error occurs, prefer to show the original address as is, if we
      // have it - this gives max info to the user
      if ( error && !m_addressHeader.empty() )
      {
         address = m_addressHeader;
      }
   }
   //else: no valid addresses at all

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
// global functions implementation
// ============================================================================

// we reimplement rfc822_write_address() and related functions here because the
// c-client function doesn't respect the buffer size which leads to easily
// reproducible buffer overflows and MRC refuses to fix it! <sigh>

// wspecials and rspecials string from c-client
static const char *WORD_SPECIALS = " ()<>@,;:\\\"[]";
const char *ALL_SPECIALS =  "()<>@,;:\\\"[].";

// this one is the replacement for rfc822_cat()
static String Rfc822Quote(const char *src, const char *specials)
{
   String dest;

   // do we have any specials at all?
   if ( strpbrk(src, specials) )
   {
      // need to quote
      dest = '"';

      while ( *src )
      {
         switch ( *src )
         {
            case '\\':
            case '"':
               // need to quote
               dest += '\\';
               break;
         }

         dest += *src++;
      }

      // closing quote
      dest += '"';
   }
   else // no specials at all, easy case
   {
      dest = src;
   }

   return dest;
}

// rfc822_address() replacement
static String Adr2Email(ADDRESS *adr)
{
   String email;

   // do we have email at all?
   if ( adr && adr->host )
   {
      email.reserve(256);

      // deal with the A-D-L
      if ( adr->adl )
      {
         email << adr->adl << ':';
      }

      // and now the mailbox name: we quote all characters forbidden in a word
      email << Rfc822Quote(adr->mailbox, WORD_SPECIALS);

      // passing the NULL host suppresses printing the full address
      if ( *adr->host != '@' )
      {
         email << '@' << adr->host;
      }
   }

   return email;
}

// rfc822_write_address() replacement with some extra functionality
static String Adr2String(ADDRESS *adr, Adr2StringWhich which, bool *error)
{
   String address;
   address.reserve(1024);

   if ( error )
   {
      // not yet
      *error = false;
   }

   bool first = true;
   for ( size_t groupDepth = 0; adr; adr = adr->next )
   {
      // is this a valid address?
      if ( adr->host && !strcmp(adr->host, ERRHOST) )
      {
         // tell the caller that we had a problem
         if ( error )
            *error = true;

         // stop at the first invalid address, there is nothing (but garbage in
         // the worst case) following it anyhow
         break;
      }

      if ( first )
      {
         // no more during the next iteration
         first = false;
      }
      else // not the first address
      {
         // do we need the subsequent ones?
         if ( which != Adr2String_All )
         {
            // no
            break;
         }

         if ( !groupDepth )
         {
            // separate from the previous one
            address << ", ";
         }
      }

      // ordinary address?
      if ( adr->host )
      {
         // simple case?
         if ( !(adr->personal || adr->adl) )
         {
            address << Adr2Email(adr);
         }
         else // no, must use phrase <route-addr> form
         {
            if ( adr->personal )
               address << Rfc822Quote(adr->personal, ALL_SPECIALS);

            address << " <" << Adr2Email(adr) << '>';
         }
      }
      else if ( adr->mailbox ) // start of group?
      {
         // yes, write group name
         address << Rfc822Quote(adr->mailbox, ALL_SPECIALS) << ": ";

         // in a group
         groupDepth++;
      }
      else if ( groupDepth ) // must be end of group (but be paranoid)
      {
         address += ';';

         groupDepth--;
      }
   }

   return address;
}

extern ADDRESS *ParseAddressList(const String& address, const String& defhost)
{
   // NB: rfc822_parse_adrlist() modifies the string passed in, copy them!

   char *addressCopy = strdup(address);

   // use '@' to trick c-client into accepting addresses without host names
   char *defhostCopy = strdup(defhost.empty() ? "@" : defhost.c_str());

   ADDRESS *adr = NULL;
   rfc822_parse_adrlist(&adr, addressCopy, defhostCopy);

   free(defhostCopy);
   free(addressCopy);

   return adr;
}

