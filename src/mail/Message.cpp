/*-*- c++ -*-********************************************************
 * Message class: entries for message header                        *
 *                                                                  *
 * (C) 1998-2000 by Karsten Ballüder (Ballueder@gmx.net)            *
 *                                                                  *
 * $Id$
 *******************************************************************/

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#ifdef __GNUG__
#   pragma implementation "Message.h"
#endif

#include   "Mpch.h"
#include   "Mcommon.h"
#include   "Message.h"

#include   "strutil.h"

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// parameters handling
// ----------------------------------------------------------------------------

/** Get a parameter value from the list.
    @param list a MessageParameterList
    @param value set to new value if found
    @return true if found
*/
bool
Message::ExpandParameter(MessageParameterList const & list,
                         String const &parameter,
                         String *value) const
{
   MessageParameterList::iterator i;

   for(i = list.begin(); i != list.end(); i++)
   {
      // parameter names are not case-sensitive, i.e. "charset" == "CHARSET"
      if ( parameter.CmpNoCase((*i)->name) == 0 )
      {
         // found
         *value = (*i)->value;
         return true;
      }
   }

   return false;
}

// ----------------------------------------------------------------------------
// working with email addresses
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


static void
SplitAddress(const String& addr,
             String *fullname)
{
   CHECK_RET( fullname, "fullname param can't be NULL" );

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
      if(firstName) *firstName = "";
      if(lastName) *lastName = "";
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

String Message::GetAddressFirstName(MessageAddressType type) const
{
   String addr;
   Address(addr, type);

   return GetFirstNameFromAddress(addr);
}

String Message::GetAddressLastName(MessageAddressType type) const
{
   String addr;
   Address(addr, type);

   return GetLastNameFromAddress(addr);
}

/* static */
String Message::GetEMailFromAddress(const String &address)
{
   String email;
   ExtractAddress(address, &email);
   return email;
}

// ----------------------------------------------------------------------------
// address comparison
// ----------------------------------------------------------------------------

/* static */
bool Message::CompareAddresses(const String& adr1, const String& adr2)
{
   String email1, email2;

   ExtractAddress(adr1, &email1);
   ExtractAddress(adr2, &email2);

   String mailbox, host;

   // turn the hostname part to lower case:
   mailbox = strutil_before(email1,'@');
   host    = strutil_after(email1, '@');
   strutil_tolower(host);
   email1 = mailbox;
   if(host[0]) email1 << '@' << host;

   mailbox = strutil_before(email2,'@');
   host    = strutil_after(email2, '@');
   strutil_tolower(host);
   email2 = mailbox;
   if(host[0]) email2 << '@' << host;

   // TODO the address foo.bar@baz.com should be considered the same as
   //      bar@baz.com, for now it is not...

   return email1 == email2;
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

// ----------------------------------------------------------------------------
// Message creation
// ----------------------------------------------------------------------------

/*
  The following function is implemented in MessageCC.cpp:
  static class Message *Message::Create(const char * itext,
                       UIdType uid, Profile *iprofile)
*/
