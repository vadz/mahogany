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
#include "Mcommon.h"

class MailFolder;

/// The name of the interface provided.
#define MMODULE_INTERFACE_FILTERS   "Filters"

/** Filtering Module class. */
class MModule_Filters : public MModule
{
public:
   /** Takes a string representation of a filterrule and compiles it
       into a class FilterRule object.
   */
   virtual class FilterRule * GetFilter(const String &filterrule) const = 0;

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

   /** Apply the filter to a single message
       @param folder - the MailFolder object
       @param uid - the uid of the message
       @return combination of the flags defined above
    */
   virtual int Apply(MailFolder *mf, UIdType uid) = 0;

   /** Apply the filter to all [new] messages in a folder.
       @param folder - the MailFolder object
       @param newOnly - if true, apply it only to new messages
       @return combination of the flags defined above
   */
   virtual int Apply(MailFolder *folder, bool newOnly = true) = 0;

   /** Apply the filter to the selected messages in a folder.
       @param folder - the MailFolder object
       @param msgs - the list of messages to apply to
       @param ignoreDeleted - do not filter messages marked as deleted
       @return combination of the flags defined above
   */
   virtual int Apply(MailFolder *folder,
                     UIdArray msgs,
                     bool ignoreDeleted = TRUE) = 0;

   MOBJECT_NAME(FilterRule)
};

#endif // FILTERSMODULE_H
