/*-*- c++ -*-********************************************************
 * MailCollector.cpp                                                *
 *                                                                  *
 * (C) 1999-2000 by Karsten Ballüder (Ballueder@gmx.net)            *
 *                                                                  *
 * $Id$
 *******************************************************************/

#ifndef MAILCOLLECTOR_H
#define MAILCOLLECTOR_H

#ifdef __GNUG__
#   pragma interface "MailCollector.h"
#endif

class MailFolder;
class MFolder;

/** One object of this class is created by MApplication to collect new 
    mail from all incoming folders.
*/
class MailCollector : public MObjectRC
{
public:
   /// Use this to get an object:
   static MailCollector *Create(void);

   /**
     Adds or removes this folder to/from the  list of folders to check.
    */
   static bool AddOrRemoveIncoming(MFolder *folder, bool add);

   /// Returns true if the mailfolder mf is an incoming folder.
   virtual bool IsIncoming(MailFolder *mf) = 0;

   /** Collect all mail from folder mf.
       @param mf the folder to collect from
       @return true on success
   */
   virtual bool Collect(MailFolder *mf = NULL) = 0;

   /** Adds a new incoming folder to the list.
       @param name folder to collect from
       @return true on success, false if folder was not found
   */
   virtual bool AddIncomingFolder(const String &name) = 0;

   /** Removes an incoming folder from the list.
       @param name no longer collect from this folder
       @return true on success, false if folder was not found
   */
   virtual bool RemoveIncomingFolder(const String &name) = 0;

   /** Ask the MailCollector to re-initialise on next collection.
    */
   virtual void RequestReInit(void) = 0;
};

#endif
