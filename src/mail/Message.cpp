//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   mail/Message.cpp: implements some static Message methods
// Purpose:     Message is an ABC but it provides some utility functions which
//              we implement here, the rest is in MessageCC
// Author:      Karsten Ballüder
// Modified by:
// Created:     1998
// CVS-ID:      $Id$
// Copyright:   (c) 1998-2001 M-Team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#ifdef __GNUG__
#   pragma implementation "Message.h"
#endif

#include "Mpch.h"

#ifndef  USE_PCH
   #include "Mcommon.h"
#endif // USE_PCH

#include "Message.h"

// should be always defined now
#define USE_ADDRESS_CLASS

#ifdef USE_ADDRESS_CLASS
   #include "Address.h"
#endif // USE_ADDRESS_CLASS

// ----------------------------------------------------------------------------
// private functions prototypes
// ----------------------------------------------------------------------------

/// extract first and last names from an address
static void SplitAddress(const String& addr,
                         String *firstName,
                         String *lastName);

/// extract the full name from the address
static void SplitAddress(const String& addr,
                         String *fullname);

/// extract the email address (without <>) from the address
static void ExtractAddress(const String& addr,
                           String *email);


// ============================================================================
// implementation of Message methods for working with addresses
// ============================================================================

// ----------------------------------------------------------------------------
// address helper functions
// ----------------------------------------------------------------------------

#ifdef USE_ADDRESS_CLASS

static void
SplitAddress(const String& address, String *fullname)
{
   CHECK_RET( fullname, _T("SplitAddress(): fullname param can't be NULL") );

   AddressList_obj addrList(address);

   fullname->clear();

   Address *addr = addrList->GetFirst();
   while ( addr )
   {
      String name = addr->GetName();

      // ignore addresses without names here
      if ( !name.empty() )
      {
         // separate from the previous one
         if ( !fullname->empty() )
         {
            *fullname += _T(", ");
         }

         *fullname += name;
      }

      addr = addrList->GetNext(addr);
   }
}

static void
ExtractAddress(const String& address, String *email)
{
   CHECK_RET( email, _T("ExtractAddress(): email param can't be NULL") );

   AddressList_obj addrList(address);

   Address *addr = addrList->GetFirst();
   if ( !addr )
   {
      wxLogDebug(_T("Invalid address '%s'"), address.c_str());

      email->clear();
   }
   else // have at least one address
   {
      *email = addr->GetEMail();

      // check that there are no more as this function doesn't work correctly
      // (and shouldn't be used) with multiple addresses
      ASSERT_MSG( !addrList->HasNext(addr),
                  _T("extra addresses ignored in ExtractAddress") );
   }
}

static void
SplitAddress(const String& address, String *firstName, String *lastName)
{
   String fullname;
   SplitAddress(address, &fullname);

   const wxChar *start = fullname.c_str();

   // the last name is the last word in the name part
   String last;
   const wxChar *p = start + fullname.length() - 1;
   while ( p >= start && !wxIsspace(*p) )
      last += *p--;

   // first name(s) is everything preceding the last name
   String first(start, p);
   first.Trim();

   if ( firstName )
      *firstName = first;
   if ( lastName )
      *lastName = last;
}

#else // !USE_ADDRESS_CLASS

// old code, unused any more

static void
SplitAddress(const String& addr,
             String *fullname)
{
   CHECK_RET( fullname, _T("fullname param can't be NULL") );

   // The code below will crash for empty addresses
   if ( addr.length() == 0 )
   {
      *fullname = "";
      return;
   }

   // handle not only addresses of the form
   //         B. L. User <foo@bar>
   // but also the alternative form specified by RFC 822
   //         foo@bar (B. L. User)

   // first check for the most common form
   const char *addrStart = strchr(addr, '<');
   if ( addrStart )
   {
      // return the part before '<'
      *fullname = String(addr.c_str(), addrStart);
   }
   else // no '<' in address
   {
      const char *namestart = strchr(addr, '(');
      const char *nameend = namestart ? strchr(++namestart, ')') : NULL;
      if ( !namestart || !nameend || namestart == nameend )
      {
         // take the entire string as we don't which part of it is address and
         // which is name
         *fullname = addr;
      }
      else
      {
         // return the part between '(' and ')'
         *fullname = String(namestart, nameend - 1);
      }
   }

   fullname->Trim();

   if ( fullname->empty() )
   {
      *fullname = addr;

      fullname->Trim();
   }
}

static void
ExtractAddress(const String& addr,
               String *email)
{
   *email = "";

   if ( !addr )
      return;

   // first check for the most common form
   const char *addrStart = strchr(addr, '<');
   const char *addrEnd;
   if ( addrStart )
   {
      addrStart++; // pointed at '<'
      // return the part before the next '>'
      addrEnd = strchr(addrStart, '>');
      if ( !addrEnd )
      {
         wxLogError(_("Unmatched '<' in the email address '%s'."),
                    addr.c_str());

         return;
      }
   }
   else // no '<' in address
   {
      // address starts at the very beginning
      addrStart = addr.c_str();

      addrEnd = strchr(addr, '(');
      if ( !addrEnd )
      {
         // just take all
         *email = addr;
      }
   }

   // take the part of the string
   if ( addrStart && addrEnd )
   {
      *email = String(addrStart, addrEnd);
   }

   // trim the address from both sides
   email->Trim(TRUE);
   email->Trim(FALSE);
}

static void
SplitAddress(const String& addr,
             String *firstName,
             String *lastName)
{
   if ( addr.length() == 0 )
   {
      if(firstName) *firstName = _T("");
      if(lastName) *lastName = _T("");
      return;
   }

   String fullname;
   SplitAddress(addr, &fullname);

   const char *start = fullname.c_str();

   // the last name is the last word in the name part
   String last;
   const char *p = start + fullname.length() - 1;
   while ( p >= start && !isspace(*p) )
      last += *p--;

   // first name(s) is everything preceding the last name
   String first(start, p);
   first.Trim();

   if ( firstName )
      *firstName = first;
   if ( lastName )
      *lastName = last;
}

#endif // USE_ADDRESS_CLASS

// ----------------------------------------------------------------------------
// Message methods or working with addresses
// ----------------------------------------------------------------------------

/* static */ String
Message::GetFirstNameFromAddress(const String& address)
{
   String first;
   SplitAddress(address, &first, NULL);

   return first;
}

/* static */ String Message::GetLastNameFromAddress(const String& address)
{
   String last;
   SplitAddress(address, NULL, &last);

   return last;
}

/* static */ String Message::GetNameFromAddress(const String& address)
{
   String name;
   SplitAddress(address, &name);

   return name;
}

// ----------------------------------------------------------------------------
// convenience wrappers
// ----------------------------------------------------------------------------

String Message::GetAddressesString(MessageAddressType type) const
{
   String address;
   AddressList_obj addrList(GetAddressList(type));
   if ( addrList )
   {
      address = addrList->GetAddresses();
   }

   return address;
}

/* static */
String Message::GetEMailFromAddress(const String &address)
{
   String email;
   ExtractAddress(address, &email);
   return email;
}

// ----------------------------------------------------------------------------
// Message: address comparison
// ----------------------------------------------------------------------------

/* static */
bool Message::CompareAddresses(const String& adr1, const String& adr2)
{
#ifdef USE_ADDRESS_CLASS
   AddressList_obj addrList1(adr1), addrList2(adr2);
   return addrList1 == addrList2;
#else // !USE_ADDRESS_CLASS
   String email1, email2;

   ExtractAddress(adr1, &email1);
   ExtractAddress(adr2, &email2);

   String mailbox, host;

   // turn the hostname part to lower case:
   mailbox = strutil_before(email1,'@');
   host    = strutil_after(email1, '@');
   strutil_tolower(host);
   email1 = mailbox;
   if(host[0u]) email1 << '@' << host;

   mailbox = strutil_before(email2,'@');
   host    = strutil_after(email2, '@');
   strutil_tolower(host);
   email2 = mailbox;
   if(host[0u]) email2 << '@' << host;

   // TODO the address foo.bar@baz.com should be considered the same as
   //      bar@baz.com, for now it is not...

   return email1 == email2;
#endif // USE_ADDRESS_CLASS
}

/* static */
int Message::FindAddress(const wxArrayString& addresses, const String& addr)
{
   size_t count = addresses.GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      if ( CompareAddresses(addresses[n], addr) )
         return n;
   }

   return wxNOT_FOUND;
}

// ----------------------------------------------------------------------------
// address extraction
// ----------------------------------------------------------------------------

size_t Message::ExtractAddressesFromHeader(wxArrayString& addresses)
{
   // first get all possible addresses
   GetAddresses(MAT_FROM, addresses);
   GetAddresses(MAT_SENDER, addresses);
   GetAddresses(MAT_REPLYTO, addresses);
   GetAddresses(MAT_RETURNPATH, addresses);
   GetAddresses(MAT_TO, addresses);
   GetAddresses(MAT_CC, addresses);
   GetAddresses(MAT_BCC, addresses);

   // now copy them to the output array filtering the copies
   return addresses.GetCount();
}

// TODO: write a function to extract addresses from the body as well

// ============================================================================
// implementation of Message methods for working with headers
// ============================================================================

// ----------------------------------------------------------------------------
// getting all the headers
// ----------------------------------------------------------------------------

size_t
Message::GetAllHeaders(wxArrayString *names, wxArrayString *values) const
{
   return GetHeaderIterator().GetAll(names, values);
}

// ----------------------------------------------------------------------------
// wrapper around GetHeaderLines
// ----------------------------------------------------------------------------

bool Message::GetHeaderLine(const String& line,
                            String& value,
                            wxFontEncoding *encoding) const
{
   const wxChar *headers[2];
   headers[0] = line.c_str();
   headers[1] = NULL;

   wxArrayString values;
   if ( encoding )
   {
      wxArrayInt encodings;
      values = GetHeaderLines(headers, &encodings);

      *encoding = encodings.IsEmpty() ? wxFONTENCODING_SYSTEM
                                      : (wxFontEncoding)encodings[0];
   }
   else // don't need the encoding
   {
      values = GetHeaderLines(headers);
   }

   if ( values.IsEmpty() )
   {
      value.clear();
   }
   else
   {
      value = values[0];
   }

   return !value.empty();
}

// ----------------------------------------------------------------------------
// Message creation
// ----------------------------------------------------------------------------

/*
  The following function is implemented in MessageCC.cpp:
  static class Message *Message::Create(const char * itext,
                       UIdType uid, Profile *iprofile)
*/

Message::~Message()
{
}

