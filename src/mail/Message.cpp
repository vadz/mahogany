/*-*- c++ -*-********************************************************
 * Message class: entries for message header                        *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
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

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

static void SplitAddress(const String& addr,
                         String *firstName,
                         String *lastName);

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
Message::ExpandParameter(MessageParameterList const & list, String
                         const &parameter,
                         String *value)
{
   MessageParameterList::iterator i;
   String par = parameter;
   strutil_toupper(par);
   
   for(i = list.begin(); i != list.end(); i++)
   {
      if(strutil_cmp(par,(*i)->name))
      {
         *value = (*i)->value;
         return true;
      }
   }
   return false;
}

// ----------------------------------------------------------------------------
// working with email addresses
// ----------------------------------------------------------------------------

static void SplitAddress(const String& addr,
                         String *firstName,
                         String *lastName)
{
   // assume that in general the email address will have the form "First Last
   // <address>", but be prepared to handle anything here

   // first look whether we have "<address>" part at all
   const char *addrStart = strchr(addr, '<');
   if ( !addrStart )
   {
      addrStart = addr.c_str() + addr.length();
   }

   // the last name is the last word in the name part
   const char *lastNameStart = addrStart;
   while ( lastNameStart >= addr.c_str() && !isspace(*lastNameStart) )
   {
      lastNameStart--;
   }

   if ( isspace(lastNameStart) )
   {
      lastNameStart++;
   }

   String last(lastNameStart, addrStart);

   // first name(s) is everything preceding the last name
   String first(addr.c_str(), lastNameStart);

   if ( firstName )
      *firstName = first;
   if ( lastName )
      *lastName = last;
}

/* static */ String Message::GetFirstNameFromAddress(const String& address)
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

// ----------------------------------------------------------------------------
// Message creation
// ----------------------------------------------------------------------------

/*
  The following function is implemented in MessageCC.cpp:
  static class Message *Message::Create(const char * itext,
                       UIdType uid, ProfileBase *iprofile)
*/
