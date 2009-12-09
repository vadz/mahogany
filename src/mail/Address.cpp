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

#include  "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"

   #include "strutil.h"
   #include "Mdefaults.h"
   #include "MApplication.h"
#endif // USE_PCH

#include <wx/hashmap.h>

#include "Address.h"

// hash type associates the list of the address equivalent to the given one
// (used as the key)
WX_DECLARE_STRING_HASH_MAP(wxArrayString, AddressHash);

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_ADD_DEFAULT_HOSTNAME;
extern const MOption MP_EQUIV_ADDRESSES;
extern const MOption MP_FROM_REPLACE_ADDRESSES;
extern const MOption MP_FROM_ADDRESS;
extern const MOption MP_HOSTNAME;
extern const MOption MP_LIST_ADDRESSES;
extern const MOption MP_PERSONALNAME;

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// handling of address equivalence hash table
// ----------------------------------------------------------------------------

static const AddressHash& GetAddressHash()
{
   static AddressHash s_hash;

   static bool s_hashInitDone = false;
   if ( !s_hashInitDone )
   {
      s_hashInitDone = true;

      const wxArrayString
         equivPairs = strutil_restore_array(READ_APPCONFIG(MP_EQUIV_ADDRESSES));

      const size_t count = equivPairs.size();
      for ( size_t n = 0; n < count; n++ )
      {
         wxArrayString equiv = strutil_restore_array(equivPairs[n], '=');
         if ( equiv.size() != 2 )
         {
            wxLogWarning(_("Invalid address equivalence option \"%s\""),
                         equivPairs[n].c_str());
            continue;
         }

         s_hash[equiv[0]].push_back(equiv[1]);
         s_hash[equiv[1]].push_back(equiv[0]);
      }
   }

   return s_hash;
}

// ----------------------------------------------------------------------------
// Address
// ----------------------------------------------------------------------------

bool Address::operator==(const String& address) const
{
   CHECK( IsValid(), false, _T("can't compare invalid addresses") );

   AddressList_obj addrList(address);
   Address *addr = addrList->GetFirst();

   return addr && !addrList->HasNext(addr) && IsSameAs(*addr);
}

/* static */
bool Address::Compare(const String& address1, const String& address2)
{
   AddressList_obj addrList1(address1),
                   addrList2(address2);
   const Address * const addr1 = addrList1->GetFirst(),
                 * const addr2 = addrList2->GetFirst();

   if ( !addr1 || !addr2 )
      return false;

   // currently multiple addresses always compare differently because address2
   // is always a single address
   if ( addrList1->HasNext(addr1) || addrList2->HasNext(addr2) )
      return false;

   if ( *addr1 == *addr2 )
      return true;

   // check also for equivalent addresses
   const AddressHash& hash = GetAddressHash();
   AddressHash::const_iterator it = hash.find(addr2->GetEMail());
   if ( it != hash.end() )
   {
      const wxArrayString& equivAddresses = it->second;
      const size_t count = equivAddresses.size();
      for ( size_t n = 0; n < count; n++ )
      {
         if ( *addr1 == equivAddresses[n] )
            return true;
      }
   }

   return false;
}

Address::~Address()
{
}

// ----------------------------------------------------------------------------
// Address static helpers
// ----------------------------------------------------------------------------

/*
   Here is the extract of IMAP SORT Extension Internet Draft which specifies
   the algorithm used for subject mangling by IMAP SORT:

Extracted Subject Text

   The "SUBJECT" SORT criteria uses a version of the subject which has
   specific subject artifacts of deployed Internet mail software
   removed.  Due to the complexity of these artifacts, the formal syntax
   for the subject extraction rules is ambiguous.  The following
   procedure is followed to determine the actual "base subject" which is
   used to sort by subject:

        (1) Convert any RFC 2047 encoded-words in the subject to
        UTF-8.  Convert all tabs and continuations to space.
        Convert all multiple spaces to a single space.

        (2) Remove all trailing text of the subject that matches
        the subj-trailer ABNF, repeat until no more matches are
        possible.

        (3) Remove all prefix text of the subject that matches the
        subj-leader ABNF.

        (4) If there is prefix text of the subject that matches the
        subj-blob ABNF, and removing that prefix leaves a non-empty
        subj-base, then remove the prefix text.

        (5) Repeat (3) and (4) until no matches remain.

   Note: it is possible to defer step (2) until step (6), but this
   requires checking for subj-trailer in step (4).

        (6) If the resulting text begins with the subj-fwd-hdr ABNF
        and ends with the subj-fwd-trl ABNF, remove the
        subj-fwd-hdr and subj-fwd-trl and repeat from step (2).

        (7) The resulting text is the "base subject" used in the
        SORT.

   All servers and disconnected clients MUST use exactly this algorithm
   when sorting by subject.  Otherwise there is potential for a user to
   get inconsistent results based on whether they are running in
   connected or disconnected IMAP mode.

   The following syntax describes subject extraction rules (2)-(6):

   subject         = *subj-leader [subj-middle] *subj-trailer

   subj-refwd      = ("re" / ("fw" ["d"])) *WSP [subj-blob] ":"

   subj-blob       = "[" *BLOBCHAR "]" *WSP

   subj-fwd        = subj-fwd-hdr subject subj-fwd-trl

   subj-fwd-hdr    = "[fwd:"

   subj-fwd-trl    = "]"

   subj-leader     = (*subj-blob subj-refwd) / WSP

   subj-middle     = *subj-blob (subj-base / subj-fwd)
                   ; last subj-blob is subj-base if subj-base would
                   ; otherwise be empty

   subj-trailer    = "(fwd)" / WSP

   subj-base       = NONWSP *([*WSP] NONWSP)
                   ; can be a subj-blob

   BLOBCHAR        = %x01-5a / %x5c / %x5e-7f
                   ; any CHAR except '[' and ']'

   NONWSP          = %x01-08 / %x0a-1f / %x21-7f
                   ; any CHAR other than WSP


   TODO: make NormalizeSubject() work like this instead of whatever it does
         currently!
 */

/* static */
String Address::NormalizeSubject(const String& subjectOrig)
{
   /* Removes Re: Re[n]: Re(n): and the local translation of
      it. */
   String subject = subjectOrig;
   subject.Trim(true).Trim(false);  // from both sides

   String trPrefix = _("Re");

   size_t idx = 0;

   if(subject.Length() > strlen("Re")
      && wxStricmp(subject.Left(strlen("Re")), _T("Re")) == 0)
      idx = strlen("Re");
   else if(subject.Length() > trPrefix.Length()
           && wxStricmp(subject.Left(trPrefix.Length()), trPrefix) == 0)
      idx = trPrefix.Length();

   if(idx == 0)
      return subject;

   char needs = '\0';
   while(idx < subject.Length())
   {
      if(subject[idx] == '(')
      {
         if(needs == '\0')
            needs = ')';
         else
            return subject; // syntax error
      }
      else if(subject[idx] == '[')
      {
         if(needs == '\0')
            needs = ']';
         else
            return subject; // syntax error
      }
      else if(subject[idx] == ':' && needs == '\0')
         break; //done
      else if(! isdigit(subject[idx]))
      {
         if(subject[idx] != needs)
            return subject; // syntax error
         else
            needs = '\0';
      }
      idx++;
   }
   subject = subject.Mid(idx+1);
   subject.Trim(true).Trim(false);  // from both sides

   return subject;
}

/// construct the full email address of the form "name <email>"
/* static */
String Address::BuildFullForm(const String& name, const String& email)
{
   if ( !name )
   {
      // don't use unnecessary angle quotes then
      return email;
   }

   String personal = name,
          address;

   // we need to quote the personal part if it's not an atext as defined by RFC
   // 2822 (TODO: reuse IsATextChar() from matchurl.cpp!)
   bool doQuote = wxStrpbrk(name, ",;\"") != NULL;
   if ( doQuote )
   {
      address = _T('"');

      // escape all quotes
      personal.Replace(_T("\""), _T("\\\""));
   }

   address += personal;

   if ( doQuote )
   {
      address += _T('"');
   }

   if ( !email.empty() )
   {
      address << _T(" <") << email << _T('>');
   }
   //else: can it really be empty??

   return address;
}

/* static */
bool
Address::IsInList(const wxArrayString& addresses,
                  const String& address,
                  String *match)
{
   const size_t count = addresses.size();

   String mailbox,
          domain;

   AddressList_obj addrList(address);
   for ( Address *addr = addrList->GetFirst();
         addr;
         addr = addrList->GetNext(addr) )
   {
      for ( size_t n = 0; n < count; n++ )
      {
         const wxChar * const start = addresses[n];

         // first obtain the address part only: we don't care about everything
         // else, "Foo <foobar@my.org>" and "Bar <foobar@my.org>" are the same
         //
         // NB: this is of course not a correct way to parse an address but we
         //     can't use AddressList here as this might be just a domain name
         //     and not a valid address
         const wxChar *startAddr = wxStrchr(start, _T('<')),
                      *endAddr = NULL;
         if ( startAddr )
            endAddr = wxStrchr(++startAddr, _T('>'));
         else
            startAddr = start;

         if ( !endAddr )
            endAddr = start + addresses[n].length();

         // next tokenize this string: it can be a full address or domain name
         // only and it may contain '?' and '*' shell pattern metacharacters
         const wxChar *p = wxStrchr(startAddr, _T('@'));
         if ( p )
         {
            mailbox.assign(startAddr, p - startAddr);
            domain.assign(p + 1, endAddr);
         }
         else // no mailbox part, domain only given
         {
            mailbox = _T('*');
            domain.assign(startAddr, endAddr);
         }

         // now we can finally compare the addresses
         if ( addr->GetMailbox().Matches(mailbox) &&
                  addr->GetDomain().Matches(domain) )
         {
            if ( match )
               *match = addr->GetAddress();

            return true;
         }
      }
   }

   return false;
}

/* static */
String Address::GetSenderAddress(const Profile *profile)
{
   String email(READ_CONFIG_TEXT(profile, MP_FROM_ADDRESS));

   // check that the email address has the domain part
   if ( email.find('@') == String::npos )
   {
      String host;
      if ( READ_CONFIG(profile, MP_ADD_DEFAULT_HOSTNAME) )
      {
         host = READ_CONFIG_TEXT(profile, MP_HOSTNAME);
      }

      // append '@' even if host is empty: this tricks c-client into accepting
      // addresses without host names instead of using a stupid
      // MISSING.WHATEVER instead of the host part
      email << '@' << host;
   }

   return BuildFullForm(READ_CONFIG(profile, MP_PERSONALNAME), email);
}

// ----------------------------------------------------------------------------
// AddressList
// ----------------------------------------------------------------------------

bool AddressList::HasNext(const Address *addr) const
{
   return GetNext(addr) != NULL;
}

bool
operator==(const AddressList_obj& addrList1, const AddressList_obj& addrList2)
{
   return addrList1->IsSameAs(addrList2.operator->());
}

// ----------------------------------------------------------------------------
// global functions
// ----------------------------------------------------------------------------

bool
ContainsOwnAddress(const String& str, Profile *profile, String *own)
{
   // get the list of our own addresses
   String returnAddrs = READ_CONFIG(profile, MP_FROM_REPLACE_ADDRESSES);
   if ( returnAddrs == GetStringDefault(MP_FROM_REPLACE_ADDRESSES) )
   {
      // the default for this option is just the return address
      returnAddrs = READ_CONFIG_TEXT(profile, MP_FROM_ADDRESS);
   }

   wxArrayString addresses = strutil_restore_array(returnAddrs);

   return Address::IsInList(addresses, str, own);
}

