/*-*- c++ -*-********************************************************
 * ASMailFolder class: asynchronous handling of mail folders        *
 *                                                                  *
 * (C) 1999 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *******************************************************************/

#ifdef __GNUG__
#   pragma implementation "ASMailFolder.h"
#endif

#include  "Mpch.h"

#ifndef USE_PCH
#   include "Mcommon.h"
#   include "strutil.h"
#   include "Profile.h"
#   include "MEvent.h"
#endif

#include "Mdefaults.h"

#include "Message.h"

#include "MailFolder.h"
#include "ASMailFolder.h"
#include "MailFolderCC.h"


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

   ASMailFolderImpl implementation, common code

 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

class ASMailFolderImpl : public ASMailFolder
{
public:
   /** @name Constructors and destructor */
   //@{

   /**
      Creates an asynchronous mailfolder object on top of a synchronous
      one.
   */
   ASMailFolderImpl(MailFolder *mf);
   //@}


   /**@name Function for access control and event handling. */
   //@{
   void LockFolder(void);
   void UnLockFolder(void);
   void SendEvent(Result *result);
   //@}

   
   /**@name Asynchronous Access Functions, returning results in events.*/
   //@{
   /** Check whether mailbox has changed.
       @return void, but causes update events to be sent if necessary.
   */
   virtual void Ping(void) = 0;
   /** Delete a message.
       @param uid the message uid
   */
   virtual void DeleteMessage(unsigned long uid);

   /** UnDelete a message.
       @param uid the message uid
       @return ResultInt with boolean success value
   */
   virtual void UnDeleteMessage(unsigned long uid) = 0;

   /** get the message with unique id uid
       @param uid message uid
       @return ResultMessage with boolean success value
   */
   virtual void GetMessage(unsigned long uid) = 0;
   /** Set flags on a messages. Possible flag values are MSG_STAT_xxx
       @param uid the message uid
       @param flag flag to be set, e.g. "\\Deleted"
       @param set if true, set the flag, if false, clear it
   */
   virtual void SetMessageFlag(unsigned long uid,
                               int flag,
                               bool set = true) = 0;

   /** Set flags on a sequence of messages. Possible flag values are MSG_STAT_xxx
       @param sequence the IMAP sequence of uids
       @param flag flag to be set, e.g. "\\Deleted"
       @param set if true, set the flag, if false, clear it
   */
   virtual void SetSequenceFlag(const String &sequence,
                                int flag,
                                bool set = true) = 0;
   /** Appends the message to this folder.
       @param msg the message to append
       @return true on success
   */
   virtual bool AppendMessage(const Message &msg) = 0;

   /** Appends the message to this folder.
       @param msg text of the  message to append
       @return true on success
   */
   virtual bool AppendMessage(const String &msg) = 0;

   /** Expunge messages.
     */
   virtual void ExpungeMessages(void) = 0;

   /**@name Some higher level functionality implemented by the
      MailFolder class on top of the other functions.
      These functions are not used by anything else in the MailFolder
      class and can easily be removed if needed.
   */
   //@{
   /** Save the messages to a folder.
       @param selections the message indices which will be converted using the current listing
       @param folderName the name of the folder to save to
       @param isProfile if true, the folderName will be interpreted as
       a symbolic folder name, otherwise as a filename
       @param updateCount If true, the number of messages in the
       folder is updated. If false, they will be detected as new messages.
       @return ResultInt boolean
   */
   virtual void SaveMessages(const INTARRAY *selections,
                             String const & folderName,
                             bool isProfile,
                             bool updateCount = true);

   /** Mark messages as deleted.
       @param messages pointer to an array holding the message numbers
       @return ResultInt boolean
   */
   virtual void DeleteMessages(const INTARRAY *messages);

   /** Mark messages as no longer deleted.
       @param messages pointer to an array holding the message numbers
       @return ResultInt boolean
   */
   virtual void UnDeleteMessages(const INTARRAY *messages);

   /** Save messages to a file.
       @param messages pointer to an array holding the message numbers
       @parent parent window for dialog
       @return ResultInt boolean
   */
   virtual void SaveMessagesToFile(const INTARRAY *messages, MWindow *parent = NULL);

   /** Save messages to a folder.
       @param messages pointer to an array holding the message numbers
       @param parent window for dialog
       @return true if messages got saved
   */
   virtual void SaveMessagesToFolder(const INTARRAY *messages, MWindow *parent = NULL);

   /** Reply to selected messages.
       @param messages pointer to an array holding the message numbers
       @param parent window for dialog
       @param profile pointer for environment
   */
   virtual void ReplyMessages(const INTARRAY *messages,
                      MWindow *parent = NULL,
                      ProfileBase *profile = NULL);

   /** Forward selected messages.
       @param messages pointer to an array holding the message numbers
       @param parent window for dialog
       @param profile pointer for environment
   */
   virtual void ForwardMessages(const INTARRAY *messages,
                                MWindow *parent = NULL,
                                ProfileBase *profile = NULL);

   //@}   
   //@}
   /**@name Synchronous Access Functions */
   //@{
   /** Get name of mailbox.
       @return the symbolic name of the mailbox
   */
   virtual String GetName(void) const
      { return m_MailFolder->GetName(); }
   /** Get number of messages which have a message status of value
       when combined with the mask. When mask = 0, return total
       message count.
       @param mask is a (combination of) MessageStatus value(s) or 0 to test against
       @param value the value of MessageStatus AND mask to be counted
       @return number of messages
   */
   virtual unsigned long CountMessages(int mask = 0, int value = 0) const
      { return m_MailFolder->CountMessages(); }
   
   /** Returns a HeaderInfo structure for a message with a given
       sequence number. This can be used to obtain the uid.
       @param msgno message sequence number, starting from 0
       @return a pointer to the messages current header info entry
   */
   virtual const class HeaderInfo *GetHeaderInfo(unsigned long msgno)
      const
      { return m_MailFolder->GetHeaderInfo(msgno); }
   
   /** Get the profile.
       @return Pointer to the profile.
   */
   inline ProfileBase *GetProfile(void) const
      { return m_MailFolder->GetProfile(); }

   /// Get update interval in seconds
   virtual int GetUpdateInterval(void) const
      { return m_MailFolder->GetUpdateInterval(); }

   /** Toggle sending of new mail events.
       @param send if true, send them
       @param update if true, update internal message count
   */
   virtual void EnableNewMailEvents(bool send = true, bool update =
                                    true)
      { m_MailFolder->EnableNewMailEvents(send, update); }

   /** Query whether folder is sending new mail events.
       @return if true, folder sends them
   */
   virtual bool SendsNewMailEvents(void) const
      { return m_MailFolder->SendsNewMailEvents(); }
   
   /**@name Functions to get an overview of messages in the folder. */
   //@{
   /// Return a pointer to the first message's header info.
   virtual const class HeaderInfo *GetFirstHeaderInfo(void) const
      { return m_MailFolder->GetFirstHeaderInfo(); }
   /// Return a pointer to the next message's header info.
   virtual const class HeaderInfo *GetNextHeaderInfo(const class
                                                     HeaderInfo* hi)
      const
      { return m_MailFolder->GetNextHeaderInfo(hi); }
   //@}
   /// Return the folder's type.
   virtual FolderType GetType(void) const
      { return m_MailFolder->GetType(); }
   /** Sets a maximum number of messages to retrieve from server.
       @param nmax maximum number of messages to retrieve, 0 for no limit
   */
   virtual void SetRetrievalLimit(unsigned long nmax)
      { m_MailFolder->SetRetrievalLimit(nmax); }
   /// Set update interval in seconds, 0 to disable
   virtual void SetUpdateInterval(int secs)
      { m_MailFolder->SetUpdateInterval(secs); }
   //@}
private:
   /// The synchronous mailfolder that we map to.
   MailFolder *m_MailFolder;
   /// Temporary, to control access:
   bool m_Locked;
};


ASMailFolderImpl::ASMailFolderImpl(MailFolder *mf)
{
   m_MailFolder = mf;
   m_Locked = false;
}


/* For now the following lock/unlock functions don't do
   anything. Later, they need to control access to the same underlying 
   m_MailFolder via a mutex. For this, no more than one
   ASMailFolderImpl object must use the same MailFolder object or
   things will go wrong.
*/
void
ASMailFolderImpl::LockFolder(void)
{
   ASSERT(m_Locked == false);
   m_Locked = true;
}

void
ASMailFolderImpl::UnLockFolder(void)
{
   ASSERT(m_Locked == true);
   m_Locked = false;
}

void
ASMailFolderImpl::SendEvent(Result *result)
{
   // now we sent an  event to update folderviews etc
   MEventManager::Send(new MEventASFolderResult (result) );
   result->DecRef(); // we no longer need it
}

void
ASMailFolderImpl::DeleteMessage(unsigned long uid)
{
   LockFolder();
   m_MailFolder->DeleteMessage(uid);
   UnLockFolder();
}

void
ASMailFolderImpl::UnDeleteMessage(unsigned long uid)
{
   LockFolder();
   m_MailFolder->UnDeleteMessage(uid);
   UnLockFolder();
}

void
ASMailFolderImpl::GetMessage(unsigned long uid)
{
   LockFolder();
   Message *msg = m_MailFolder->GetMessage(uid);
   UnLockFolder();
   SendEvent(ResultMessage::Create(this, msg, uid));
}

void
ASMailFolderImpl::SaveMessages(const INTARRAY *selections,
                             String const & folderName,
                             bool isProfile,
                               bool updateCount = true)
{
}

void
ASMailFolderImpl::DeleteMessages(const INTARRAY *messages)
{
}

void
ASMailFolderImpl::UnDeleteMessages(const INTARRAY *messages)
{
}

void
ASMailFolderImpl::SaveMessagesToFile(const INTARRAY *messages, MWindow
                                     *parent = NULL)
{
}

void
ASMailFolderImpl::SaveMessagesToFolder(const INTARRAY *messages,
                                       MWindow *parent = NULL)
{
}

void
ASMailFolderImpl::ReplyMessages(const INTARRAY *messages,
                                MWindow *parent = NULL,
                                ProfileBase *profile = NULL)
{
}

void
ASMailFolderImpl::ForwardMessages(const INTARRAY *messages,
                                  MWindow *parent = NULL,
                                  ProfileBase *profile = NULL)
{
}

