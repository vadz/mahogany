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

/**
   This is the interface for the Mahogany Filters module.
*/
class MModule_Filters : public MModule
{
public:
   /** Takes a string representation of a filterrule and compiles it
       into a class FilterRule object.
   */
   virtual FilterRule * GetFilter(const String &filterrule) = 0;
};

#define MMODULE_INTERFACE_FILTERS   "Filters"

#endif // FILTERSMODULE_H
