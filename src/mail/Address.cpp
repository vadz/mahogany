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
   Address *addr = addrList->GetFirst();
   if ( !addr || addrList->HasNext(addr) )
   {
      return false;
   }

   return addr->IsSameAs(*this);
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
      && Stricmp(subject.Left(strlen("Re")), "Re") == 0)
      idx = strlen("Re");
   else if(subject.Length() > trPrefix.Length()
           && Stricmp(subject.Left(trPrefix.Length()), trPrefix) == 0)
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
   bool doQuote = strpbrk(name, ",;\"") != (const char *)NULL;
   if ( doQuote )
   {
      address = '"';

      // escape all quotes
      personal.Replace("\"", "\\\"");
   }

   address += personal;

   if ( doQuote )
   {
      address += '"';
   }

   if ( !email.empty() )
   {
      address << " <" << email << '>';
   }
   //else: can it really be empty??

   return address;
}

// ----------------------------------------------------------------------------
// AddressList
// ----------------------------------------------------------------------------

bool AddressList::HasNext(const Address *addr) const
{
   return GetNext(addr) != NULL;
}

extern bool operator==(const AddressList_obj& addrList1,
                       const AddressList_obj& addrList2)
{
   return addrList1->IsSameAs(addrList2.operator->());
}

