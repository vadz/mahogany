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
#   include "guidef.h"    // only for high-level functions
#   include "strutil.h"
#   include "Profile.h"
#   include "MEvent.h"
#endif

#include "Mdefaults.h"
#include "Message.h"

#include "MailFolder.h"
#include "ASMailFolder.h"
#include "MailFolderCC.h"


/// Call this always before using it.
#ifdef DEBUG
#   define   AScheck()   { MOcheck(); ASSERT(m_MailFolder); }
#else
#   define   AScheck()
#endif



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
   /// destructor
   ~ASMailFolderImpl();
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
   virtual void Ping(void) { m_MailFolder->Ping(); } //FIXME
   /** Delete a message.
       @param uid the message uid
   */
   virtual void DeleteMessage(unsigned long uid);

   /** UnDelete a message.
       @param uid the message uid
       @return ResultInt with boolean success value
   */
   virtual void UnDeleteMessage(unsigned long uid) ;

   /** get the message with unique id uid
       @param uid message uid
       @return ResultMessage with boolean success value
   */
   virtual Ticket GetMessage(unsigned long uid, UserData ud = 0);
   /** Set flags on a messages. Possible flag values are MSG_STAT_xxx
       @param uid the message uid
       @param flag flag to be set, e.g. "\\Deleted"
       @param set if true, set the flag, if false, clear it
   */
   virtual void SetMessageFlag(unsigned long uid,
                               int flag,
                               bool set = true) ;

   /** Set flags on a sequence of messages. Possible flag values are MSG_STAT_xxx
       @param sequence the IMAP sequence of uids
       @param flag flag to be set, e.g. "\\Deleted"
       @param set if true, set the flag, if false, clear it
   */
   virtual void SetSequenceFlag(const String &sequence,
                                int flag,
                                bool set = true) ;
   /** Appends the message to this folder.
       @param msg the message to append
       @return true on success
   */
   virtual Ticket AppendMessage(/* const */ Message *msg, UserData ud = 0) ;

   /** Appends the message to this folder.
       @param msg text of the  message to append
       @return true on success
   */
   virtual Ticket AppendMessage(const String &msg, UserData ud = 0) ;

   /** Expunge messages.
     */
   virtual void ExpungeMessages(void) ;

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
   virtual Ticket SaveMessages(const INTARRAY *selections,
                               String const & folderName,
                               bool isProfile,
                               bool updateCount = true,
                               UserData ud = 0);

   /** Mark messages as deleted.
       @param messages pointer to an array holding the message numbers
       @return ResultInt boolean
   */
   virtual Ticket DeleteMessages(const INTARRAY *messages, UserData ud = 0);

   /** Mark messages as no longer deleted.
       @param messages pointer to an array holding the message numbers
       @return ResultInt boolean
   */
   virtual Ticket UnDeleteMessages(const INTARRAY *messages, UserData ud = 0);

   /** Save messages to a file.
       @param messages pointer to an array holding the message numbers
       @parent parent window for dialog
       @return ResultInt boolean
   */
   virtual Ticket SaveMessagesToFile(const INTARRAY *messages, MWindow
                                     *parent = NULL, UserData ud = 0);

   /** Save messages to a folder.
       @param messages pointer to an array holding the message numbers
       @param parent window for dialog
       @return true if messages got saved
   */
   virtual Ticket SaveMessagesToFolder(const INTARRAY *messages, MWindow *parent = NULL, UserData ud = 0);

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
      { AScheck(); return m_MailFolder->GetName(); }
   /** Get number of messages which have a message status of value
       when combined with the mask. When mask = 0, return total
       message count.
       @param mask is a (combination of) MessageStatus value(s) or 0 to test against
       @param value the value of MessageStatus AND mask to be counted
       @return number of messages
   */
   virtual unsigned long CountMessages(int mask = 0, int value = 0) const
      { AScheck(); return m_MailFolder->CountMessages(); }
   
   /** Returns a HeaderInfo structure for a message with a given
       sequence number. This can be used to obtain the uid.
       @param msgno message sequence number, starting from 0
       @return a pointer to the messages current header info entry
   */
   virtual const class HeaderInfo *GetHeaderInfo(unsigned long msgno)
      const
      { AScheck(); return m_MailFolder->GetHeaderInfo(msgno); }
   
   /** Get the profile.
       @return Pointer to the profile.
   */
   inline ProfileBase *GetProfile(void) const
      { AScheck(); return m_MailFolder->GetProfile(); }

   /// Get update interval in seconds
   virtual int GetUpdateInterval(void) const
      { AScheck(); return m_MailFolder->GetUpdateInterval(); }

   /** Toggle sending of new mail events.
       @param send if true, send them
       @param update if true, update internal message count
   */
   virtual void EnableNewMailEvents(bool send = true, bool update =
                                    true)
      { AScheck(); m_MailFolder->EnableNewMailEvents(send, update); }

   /** Query whether folder is sending new mail events.
       @return if true, folder sends them
   */
   virtual bool SendsNewMailEvents(void) const
      { AScheck(); return m_MailFolder->SendsNewMailEvents(); }
   
   /**@name Functions to get an overview of messages in the folder. */
   //@{
   /// Return a pointer to the first message's header info.
   virtual const class HeaderInfo *GetFirstHeaderInfo(void) const
      { AScheck(); return m_MailFolder->GetFirstHeaderInfo(); }
   /// Return a pointer to the next message's header info.
   virtual const class HeaderInfo *GetNextHeaderInfo(const class
                                                     HeaderInfo* hi)
      const
      { AScheck(); return m_MailFolder->GetNextHeaderInfo(hi); }
   //@}
   /// Return the folder's type.
   virtual FolderType GetType(void) const
      { AScheck(); return m_MailFolder->GetType(); }
   /** Sets a maximum number of messages to retrieve from server.
       @param nmax maximum number of messages to retrieve, 0 for no limit
   */
   virtual void SetRetrievalLimit(unsigned long nmax)
      { AScheck(); m_MailFolder->SetRetrievalLimit(nmax); }
   /// Set update interval in seconds, 0 to disable
   virtual void SetUpdateInterval(int secs)
      { AScheck(); m_MailFolder->SetUpdateInterval(secs); }
   //@}
protected:
   /** Used to obtain the next Ticked id for events. */
   static Ticket GetTicket(void);
private:
   /// The synchronous mailfolder that we map to.
   MailFolder *m_MailFolder;
   /// Temporary, to control access:
   bool m_Locked;
   /// Next ticket Id to use.
   static Ticket ms_Ticket;
};


ASMailFolderImpl::ASMailFolderImpl(MailFolder *mf)
{
   ASSERT(mf);
   m_MailFolder = mf;
   m_MailFolder->IncRef();
   m_Locked = false;
}

ASMailFolderImpl::~ASMailFolderImpl()
{
   ASSERT(m_MailFolder);
   m_MailFolder->DecRef();
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
   AScheck();
   ASSERT(m_Locked == false);
   m_Locked = true;
}

void
ASMailFolderImpl::UnLockFolder(void)
{
   AScheck();
   ASSERT(m_Locked == true);
   m_Locked = false;
}

void
ASMailFolderImpl::SendEvent(Result *result)
{
   ASSERT(result);
   // now we sent an  event to update folderviews etc
   MEventManager::Send(new MEventASFolderResultData (result) );
   result->DecRef(); // we no longer need it
}


void
ASMailFolderImpl::SetMessageFlag(unsigned long uid,
                                 int flag,
                                 bool set)
{
   LockFolder();
   m_MailFolder->SetMessageFlag(uid, flag, set);
   UnLockFolder();
}

void
ASMailFolderImpl::SetSequenceFlag(const String &sequence,
                                  int flag,
                                  bool set)
{
   LockFolder();
   m_MailFolder->SetSequenceFlag(sequence, flag, set);
   UnLockFolder();
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


ASMailFolder::Ticket
ASMailFolderImpl::AppendMessage(/* const */ Message *msg, UserData ud)
{
   Ticket t = GetTicket();
   msg->IncRef();
   LockFolder();
   int rc = m_MailFolder->AppendMessage(*msg);
   SendEvent(ResultInt::Create(this, t, Op_AppendMessage, rc, ud));
   UnLockFolder();
   msg->DecRef();
   return t;
}

ASMailFolder::Ticket
ASMailFolderImpl::AppendMessage(const String &msg, UserData ud)
{
   Ticket t = GetTicket();
   LockFolder();
   int rc = m_MailFolder->AppendMessage(msg);
   SendEvent(ResultInt::Create(this, t, Op_AppendMessage, rc, ud));
   UnLockFolder();
   return t;
}


void
ASMailFolderImpl::ExpungeMessages(void)
{
   LockFolder();
   m_MailFolder->ExpungeMessages();
   UnLockFolder();
}

ASMailFolder::Ticket
ASMailFolderImpl::GetMessage(unsigned long uid, UserData ud)
{
   Ticket t = GetTicket();
   LockFolder();
   Message *msg = m_MailFolder->GetMessage(uid);
   UnLockFolder();
   SendEvent(ResultMessage::Create(this, t, msg, uid, ud));
   return t;
}

ASMailFolder::Ticket
ASMailFolderImpl::SaveMessages(const INTARRAY *selections,
                               String const & folderName,
                               bool isProfile,
                               bool updateCount, UserData ud)
{
   Ticket t = GetTicket();
   LockFolder();
   int rc = m_MailFolder->SaveMessages(selections, folderName, isProfile, updateCount);
   SendEvent(ResultInt::Create(this, t, Op_SaveMessages, rc, ud));
   UnLockFolder();
   return t;
}

ASMailFolder::Ticket
ASMailFolderImpl::DeleteMessages(const INTARRAY *messages, UserData ud)
{
   Ticket t = GetTicket();
   LockFolder();
   int rc = m_MailFolder->DeleteMessages(messages);
   SendEvent(ResultInt::Create(this, t, Op_DeleteMessages, rc, ud));
   UnLockFolder();
   return t;
}

ASMailFolder::Ticket
ASMailFolderImpl::UnDeleteMessages(const INTARRAY *messages, UserData ud)
{
   Ticket t = GetTicket();
   LockFolder();
   int rc = m_MailFolder->UnDeleteMessages(messages);
   SendEvent(ResultInt::Create(this, t, Op_DeleteMessages, rc, ud));
   UnLockFolder();
   return t;
}

ASMailFolder::Ticket
ASMailFolderImpl::SaveMessagesToFile(const INTARRAY *messages, MWindow
                                     *parent, UserData ud)
{
   Ticket t = GetTicket();
   LockFolder();
   int rc = m_MailFolder->SaveMessagesToFile(messages, parent);
   SendEvent(ResultInt::Create(this, t, Op_SaveMessagesToFile, rc, ud));
   UnLockFolder();
   return t;
}

ASMailFolder::Ticket
ASMailFolderImpl::SaveMessagesToFolder(const INTARRAY *messages,
                                       MWindow *parent, UserData ud)
{
   Ticket t = GetTicket();
   LockFolder();
   int rc = m_MailFolder->SaveMessagesToFolder(messages, parent);
   SendEvent(ResultInt::Create(this, t, Op_SaveMessagesToFolder, rc, ud));
   UnLockFolder();
   return t;
}

void
ASMailFolderImpl::ReplyMessages(const INTARRAY *messages,
                                MWindow *parent,
                                ProfileBase *profile)
{
   LockFolder();
   m_MailFolder->ReplyMessages(messages, parent, profile);
   UnLockFolder();
}

void
ASMailFolderImpl::ForwardMessages(const INTARRAY *messages,
                                  MWindow *parent,
                                  ProfileBase *profile)
{
   LockFolder();
   m_MailFolder->ForwardMessages(messages, parent, profile);
   UnLockFolder();
}

/* static */
ASMailFolder::Ticket
ASMailFolderImpl::GetTicket(void)
{
   return ms_Ticket++;
}

ASMailFolder::Ticket
ASMailFolderImpl::ms_Ticket = 0;

/* static */
ASMailFolder *
ASMailFolder::Create(MailFolder *mf)
{
   return new ASMailFolderImpl(mf);
}

