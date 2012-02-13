/*-*- c++ -*-********************************************************
 * Filters - a pluggable module for filtering messages              *
 *                                                                  *
 * (C) 1999-2000 by Karsten Ballüder (ballueder@gmx.net)            *
 *                                                                  *
 * $Id$
 *******************************************************************/

#ifndef FILTERSMODULE_H
#define FILTERSMODULE_H

#include "MModule.h"

class FilterRule;
class MailFolder;
class UIdArray;

/// The name of the interface provided.
#define MMODULE_INTERFACE_FILTERS   "Filters"

/** Filtering Module class. */
class MModule_Filters : public MModule
{
public:
   /**
     Takes a string representation of a filterrule and compiles it
     into a class FilterRule object.
    
     @param filterrule the text of the filter program
     @return a filter rule on success or NULL on error
   */
   virtual FilterRule * GetFilter(const char* filterrule) const = 0;

   /** To easily obtain a filter module: */
   static MModule_Filters *GetModule(void)
      {
         return (MModule_Filters *)
           MModule::GetProvider(MMODULE_INTERFACE_FILTERS);
      }

   MOBJECT_NAME(MModule_Filters)
};

/** Parsed representation of a filtering rule to be applied to a
    message.
*/
class FilterRule : public MObjectRC
{
public:
   /// the bit fields from which the return code of Apply() is composed
   enum
   {
      /// the messages should be expunged
      Expunged = 0x0001,

      /// some messages were deleted
      Deleted = 0x0002,

      /// an error happened while evaluating the filters
      Error = 0xf000
   };
   
   /**
     Apply the filter to the selected messages in a folder.
     The messages deleted by the filters are removed from msgs array.

     @param folder - the MailFolder object
     @param msgs - the list of messages to apply to
     @return combination of the flags defined above
   */
   virtual int Apply(MailFolder *folder, UIdArray& msgs) = 0;

   MOBJECT_NAME(FilterRule)
};

#endif // FILTERSMODULE_H
