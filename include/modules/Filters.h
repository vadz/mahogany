/*-*- c++ -*-********************************************************
 * Filters - a pluggable module for filtering messages              *
 *                                                                  *
 * (C) 1999 by Karsten Ballüder (karsten@phy.hw.ac.uk)              *
 *                                                                  *
 * $Id$
 *******************************************************************/

#ifndef FILTERSMODULE_H
#define FILTERSMODULE_H

#include "MModule.h"
#include "Mcommon.h"

/// The name of the interface provided.
#define MMODULE_INTERFACE_FILTERS   "Filters"

/** Filtering Module class. */
class MModule_Filters : public MModule
{
public:
   /** Takes a string representation of a filterrule and compiles it
       into a class FilterRule object.
   */
   virtual class FilterRule * GetFilter(const String &filterrule)
      const = 0;

   /** To easily obtain a filter module: */
   static MModule_Filters *GetModule(void)
      {
         return (MModule_Filters *)
           MModule::GetProvider(MMODULE_INTERFACE_FILTERS);
      }
};

/** Parsed representation of a filtering rule to be applied to a
    message.
*/
class FilterRule : public MObjectRC
{
public:
   /** Apply the filter to a single message, returns 0 on success. */
   virtual int Apply(class Message *msg) const = 0;
   /** Apply the filter to the messages in a folder.
       @param folder - the MailFolder object
       @param NewOnly - if true, apply it only to new messages
       @return 0 on success
   */
   virtual int Apply(class MailFolder *folder, bool NewOnly = true) const = 0;
};


#endif // FILTERSMODULE_H
