/*-*- c++ -*-********************************************************
 * Message class: entries for message header                        *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *******************************************************************/

#ifdef __GNUG__
#pragma implementation "MessageCC.h"
#endif

#include   "Mpch.h"
#include   "Mcommon.h"
#include   "Message.h"

#include   "strutil.h"

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

