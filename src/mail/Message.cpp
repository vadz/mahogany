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
                         String *value) const
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
                         String *fullname)
{
   ASSERT(fullname);
   // The code below will crash for empty addresses
   if(addr.Length() == 0)
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

static void SplitAddress(const String& addr,
                         String *firstName,
                         String *lastName)
{
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

// ----------------------------------------------------------------------------
// Message creation
// ----------------------------------------------------------------------------

/*
  The following function is implemented in MessageCC.cpp:
  static class Message *Message::Create(const char * itext,
                       UIdType uid, Profile *iprofile)
*/
