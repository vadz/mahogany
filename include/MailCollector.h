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

/**
  One object of this class is created by MApplication to collect new 
  mail from all incoming folders.

  The name is a misnomer: it should be called MailMonitor, it doesn't actually
  collect anything anywhere by itself...
*/
class MailCollector : public MObjectRC
{
public:
   /// Use this to get an object:
   static MailCollector *Create(void);

   /**
     Adds or removes this folder to/from the  list of folders to check.

     @param folder the folder to monitor or stop monitoring
     @return true if ok, false on error
    */
   static bool AddOrRemoveIncoming(MFolder *folder, bool add);

   /** Collect all mail from folder mf.
       @param mf the folder to collect from
       @return true on success
   */
   virtual bool Collect(MailFolder *mf = NULL) = 0;
};

#endif
