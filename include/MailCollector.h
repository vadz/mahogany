/*-*- c++ -*-********************************************************
 * MailCollector.cpp                                                *
 *                                                                  *
 * (C) 1999 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *******************************************************************/

#ifndef MAILCOLLECTOR_H
#define MAILCOLLECTOR_H

#ifdef __GNUG__
#   pragma interface "MailCollector.h"
#endif

class MailFolder;

/** One object of this class is created by MApplication to collect new 
    mail from all incoming folders.
*/
class MailCollector
{
public:
   MailCollector();
   ~MailCollector();
   /// Returns true if the mailfolder mf is an incoming folder.
   bool IsIncoming(MailFolder *mf);
   /** Collect all mail from folder mf.
       @param mf the folder to collect from
       @return true on success
   */
   bool Collect(MailFolder *mf = NULL);
   /** Tells the object about a new new mail folder.
       @param name use this folder as the new mail folder
   */
   void SetNewMailFolder(const String &name);
   /** Adds a new incoming folder to the list.
       @param name folder to collect from
       @return true on success, false if folder was not found
   */
   bool AddIncomingFolder(const String &name);
   /** Removes an incoming folder from the list.
       @param name no longer collect from this folder
       @return true on success, false if folder was not found
   */
   bool RemoveIncomingFolder(const String &name);
   /** Returns true if the collector is locked.
       @return true if collecting
   */
   bool IsLocked(void) const { return m_IsLocked; }
   /** Locks or unlocks the mail collector. If it is locked, it simly
       does nothing.
       @param lock true=lock, false=unlock
       @return the old state
   */
   bool Lock(bool lock = true)
      { bool rc = m_IsLocked; m_IsLocked =lock; return rc; }
protected:
   /// Collect mail from this one folder.
   bool CollectOneFolder(MailFolder *mf);
private:
   /// a list of folder names and mailfolder pointers
   class MailCollectorFolderList *m_list;
   /// the folder to save new mail to
   MailFolder     *m_NewMailFolder;
   /// profile for the new mail folder
   ProfileBase    *m_NewMailProfile;
   /// are we not supposed to collect anything?
   bool           m_IsLocked;
   /// The message for the new mail dialog.
   String         m_Message;
   /// The number of new messages.
   unsigned long  m_Count;
};

#endif
