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
#   include "MApplication.h"
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

 ASMailFolderImpl creates a MailThread object for each operation
 that it wants to perform. These MailThread objects run either
 synchronously or asynchronously (derived from wxThread), depending
 on whether Mahogany was configured --with-threads or without.
                                                                     
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

   MailThread - one thread for each operation to be done

 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifdef USE_THREADS
#   include <wx/thread.h>
#endif


static
INTARRAY *Copy(const INTARRAY *old)
{
   INTARRAY *newarray = new INTARRAY;
   for(size_t i = 0; i < old->Count(); i++)
      newarray->Add( (*old)[i] );
   return newarray;
}

class MailThread

#ifdef USE_THREADS
   : public wxThread
#endif

{
public:
   MailThread(ASMailFolder *mf, ASMailFolder::UserData ud)
      {
         m_ASMailFolder = mf;
         m_MailFolder = mf->GetMailFolder();
         m_UserData = ud;
      }

   ASMailFolder::Ticket Start(void)
      {
         m_Ticket = GetTicket();
         Run();
#ifndef USE_THREADS
         // We are not using wxThread, so we must delete this when we
         // are done.
         delete this;
#endif
         return m_Ticket;
      }
#ifndef USE_THREADS
   virtual ~MailThread() { }
#endif
   
protected:
   void SendEvent(ASMailFolder::Result *result);
   inline void LockFolder(void)
      { m_ASMailFolder->LockFolder(); };
   inline void UnLockFolder(void)
      { m_ASMailFolder->UnLockFolder(); };

   static ASMailFolder::Ticket GetTicket(void)
      { return ms_Ticket++;}

#ifndef USE_THREADS
   virtual void *Entry();
#endif
   virtual void Run(void) { Entry(); }

   virtual void  WorkFunction(void) = 0;

protected:
   class ASMailFolder *m_ASMailFolder;
   MailFolder            *m_MailFolder;
   ASMailFolder::UserData m_UserData;
   ASMailFolder::Ticket   m_Ticket;
   static ASMailFolder::Ticket ms_Ticket;
};

/* static */
ASMailFolder::Ticket
MailThread::ms_Ticket = 0;

void *
MailThread::Entry()
{
   LockFolder();
   WorkFunction();
   UnLockFolder();
   return NULL; // no return value
}


void
MailThread::SendEvent(ASMailFolder::Result *result)
{
   ASSERT(result);
   // now we sent an  event to update folderviews etc
   MEventManager::Send(new MEventASFolderResultData (result) );
   result->DecRef(); // we no longer need it
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

   MailThread classes - the individual operations performed on a folder

 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


/// Common code for MailThreads taking a sequence array
class MailThreadSeq : public MailThread
{
public:
   MailThreadSeq(ASMailFolder *mf,
                 ASMailFolder::UserData ud,
                 const INTARRAY *selections)
      : MailThread(mf, ud)
      {
         m_Seq = Copy(selections);
      }
   ~MailThreadSeq() { delete m_Seq; }
protected:
   INTARRAY *m_Seq;
};

class MT_Ping : public MailThread
{
public:
   MT_Ping(ASMailFolder *mf,
           ASMailFolder::UserData ud)
      : MailThread(mf, ud) {}
   virtual void WorkFunction(void)
      { m_MailFolder->Ping(); }
};

class MT_SetSequenceFlag : public MailThread
{
public:
   MT_SetSequenceFlag(ASMailFolder *mf, ASMailFolder::UserData ud,
                      const String &sequence,
                      int flag, bool set)
      : MailThread(mf, ud)
      {
         m_Sequence = sequence;
         m_Flag = flag;
         m_Set = set;
      }
   virtual void WorkFunction(void)
      {
         m_MailFolder->SetSequenceFlag(m_Sequence, m_Flag, m_Set);
      }
protected:
   String m_Sequence;
   int m_Flag;
   bool m_Set;
};

class MT_SetFlag : public MailThreadSeq
{
public:
   MT_SetFlag(ASMailFolder *mf, ASMailFolder::UserData ud,
              const INTARRAY *sequence,
              int flag, bool set)
      : MailThreadSeq(mf, ud, sequence)
      {
         m_Flag = flag;
         m_Set = set;
      }
   virtual void WorkFunction(void)
      {
         m_MailFolder->SetFlag(m_Seq, m_Flag, m_Set);
      }
protected:
   int m_Flag;
   bool m_Set;
};

class MT_GetMessage : public MailThread
{
public:
   MT_GetMessage(ASMailFolder *mf,
                 ASMailFolder::UserData ud,
                 unsigned long uid)
                 
      : MailThread(mf, ud)
      {
         m_UId = uid;
      }
   virtual void WorkFunction(void)
      {
         Message *msg = m_MailFolder->GetMessage(m_UId);
         SendEvent(ASMailFolder::ResultMessage::Create(
            m_ASMailFolder, m_Ticket,
            msg, m_UId, m_UserData));
      }
protected:
   unsigned long m_UId;
};


class MT_AppendMessage : public MailThread
{
public:
   MT_AppendMessage(ASMailFolder *mf,
                    ASMailFolder::UserData ud,
                    const Message *msg)
                 
      : MailThread(mf, ud)
      {
         m_Message = (Message *)msg;
         m_Message->IncRef();
      }
   MT_AppendMessage(ASMailFolder *mf,
                    ASMailFolder::UserData ud,
                    const String &msgstr)
                 
      : MailThread(mf, ud)
      {
         m_Message = NULL;
         m_MsgString = msgstr;

      }
   ~MT_AppendMessage()
      {
         if(m_Message) m_Message->DecRef();
      }
   virtual void WorkFunction(void)
      {
         int rc = m_Message ?
            m_MailFolder->AppendMessage(*m_Message)
            : m_MailFolder->AppendMessage(m_MsgString);
         SendEvent(ASMailFolder::ResultInt::
                   Create(m_ASMailFolder, m_Ticket,
                          ASMailFolder::Op_AppendMessage,  rc,
                          m_UserData));  
      }
protected:
   Message *m_Message;
   String         m_MsgString;
};

class MT_Expunge : public MailThread
{
public:
   MT_Expunge(ASMailFolder *mf)
      : MailThread(mf, NULL) {}
   virtual void WorkFunction(void)
      { m_MailFolder->ExpungeMessages(); }
};


class MT_SaveMessages : public MailThreadSeq
{
public:
   MT_SaveMessages(ASMailFolder *mf, ASMailFolder::UserData ud,
                   const INTARRAY *selections,
                   const String &folderName, 
                   bool isProfile, bool updateCount)
      : MailThreadSeq(mf, ud, selections)
      {
         m_MfName = folderName;
         m_IsProfile = isProfile;
         m_Update = updateCount;
      }
   virtual void WorkFunction(void)
      {
         int rc = m_MailFolder->SaveMessages(m_Seq, m_MfName, m_IsProfile, m_Update);
         SendEvent(ASMailFolder::ResultInt::Create(m_ASMailFolder,
                                                   m_Ticket,
                                                   ASMailFolder::Op_SaveMessages,
                                                   rc, m_UserData)); 
      }
private:
   String    m_MfName;
   bool      m_IsProfile;
   bool      m_Update;
};


class MT_SaveMessagesToFileOrFolder : public MailThreadSeq
{
public:
   MT_SaveMessagesToFileOrFolder(ASMailFolder *mf,
                                 ASMailFolder::UserData ud,
                                 ASMailFolder::OperationId op,
                                 const INTARRAY *selections,
                                 MWindow *parent)
      : MailThreadSeq(mf, ud, selections)
      {
         m_Parent = parent;
         m_Op = op;
         ASSERT(m_Op == ASMailFolder::Op_SaveMessagesToFile ||
                m_Op == ASMailFolder::Op_SaveMessagesToFolder);
      }
   virtual void WorkFunction(void)
      {
         int rc = m_Op == ASMailFolder::Op_SaveMessagesToFile ?
            m_MailFolder->SaveMessagesToFile(m_Seq, m_Parent)
            : m_MailFolder->SaveMessagesToFolder(m_Seq, m_Parent);
         SendEvent(ASMailFolder::ResultInt::Create(m_ASMailFolder, m_Ticket, m_Op, rc, m_UserData));
      }
private:
   MWindow *m_Parent;
   ASMailFolder::OperationId m_Op;
};

class MT_ReplyForwardMessages : public MailThreadSeq
{
public:
   MT_ReplyForwardMessages(ASMailFolder *mf, ASMailFolder::UserData ud,
                           ASMailFolder::OperationId op,
                           const INTARRAY *selections,
                           MWindow *parent, ProfileBase *profile,
                           int flags )
      : MailThreadSeq(mf, ud, selections)
      {
         m_Profile = profile;
         if(m_Profile) m_Profile->IncRef();
         m_Parent = parent;
         m_Flags = flags;
         m_Op = op;
         ASSERT(m_Op == ASMailFolder::Op_ReplyMessages ||
                m_Op == ASMailFolder::Op_ForwardMessages);
      }
   ~MT_ReplyForwardMessages()
      { if(m_Profile) m_Profile->DecRef(); }
   virtual void WorkFunction(void)
      {
         if(m_Op == ASMailFolder::Op_ReplyMessages)
            m_MailFolder->ReplyMessages(m_Seq, m_Parent, m_Profile, m_Flags);
         else
            m_MailFolder->ForwardMessages(m_Seq, m_Parent, m_Profile);
      }
private:
   ASMailFolder::OperationId m_Op;
   ProfileBase *m_Profile;
   MWindow *m_Parent;
   int m_Flags;
};

class MT_Subscribe : public MailThread
{
public:
   MT_Subscribe(ASMailFolder *mf, ASMailFolder::UserData ud,
                const String &mailboxname,
                bool subscribe)
      : MailThread(mf, ud)
      {
         m_Name = mailboxname;
         m_Sub = subscribe;
      }
   virtual void WorkFunction(void)
      {
         int rc = m_MailFolder->Subscribe(m_Name, m_Sub);
         SendEvent(ASMailFolder::ResultInt::Create(m_ASMailFolder, m_Ticket, ASMailFolder::Op_Subscribe, rc, m_UserData));
      }
private:
   String  m_Name;
   bool    m_Sub;
};

class MT_ListFolders : public MailThread
{
public:
   MT_ListFolders(ASMailFolder *mf,
                  ASMailFolder::UserData ud,
                  const String &pattern,
                  bool sub_only,
                  const String &reference)
      : MailThread(mf, ud)
      {
         m_Pattern = pattern;
         m_SubOnly = sub_only;
         m_Reference = reference;
      }
   virtual void WorkFunction(void)
      {
         FolderListing *fl = m_MailFolder->ListFolders(m_Pattern, m_SubOnly, m_Reference);
         SendEvent(ASMailFolder::ResultFolderListing::Create(
            m_ASMailFolder, m_Ticket, fl, m_UserData));
      }
private:
   String  m_Pattern;
   String  m_Reference;
   bool  m_SubOnly;
};

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
   //@}

   /**@name Asynchronous Access Functions, returning results in events.*/
   //@{
   /** Check whether mailbox has changed.
       @return void, but causes update events to be sent if necessary.
   */
   virtual void Ping(void)
      { (void) (new MT_Ping(this, NULL))->Start(); }


   /** get the message with unique id uid
       @param uid message uid
       @return ResultMessage with boolean success value
   */
   virtual Ticket GetMessage(unsigned long uid, UserData ud)
      {
         return (new MT_GetMessage(this, ud, uid))->Start();
      }

   /** Set flags on a sequence of messages. Possible flag values are MSG_STAT_xxx
       @param sequence the IMAP sequence of uids
       @param flag flag to be set, e.g. "\\Deleted"
       @param set if true, set the flag, if false, clear it
   */
   virtual Ticket SetSequenceFlag(const String &sequence,
                                  int flag,
                                  bool set)
      {
         return (new MT_SetSequenceFlag(this, NULL, sequence,flag,set))->Start();
      }

   /** Set flags on a sequence of messages. Possible flag values are MSG_STAT_xxx
       @param sequence the IMAP sequence of uids
       @param flag flag to be set, e.g. "\\Deleted"
       @param set if true, set the flag, if false, clear it
   */
   virtual Ticket SetFlag(const INTARRAY *sequence, int flag, bool set)
      {
         return (new MT_SetFlag(this, NULL, sequence,flag,set))->Start();
      }


   /** Appends the message to this folder.
       @param msg the message to append
       @return true on success
   */
   virtual Ticket AppendMessage(const Message *msg, UserData ud )
      {
         return (new MT_AppendMessage(this, ud, msg))->Start();
      }

   /** Appends the message to this folder.
       @param msg text of the  message to append
       @return true on success
   */
   virtual Ticket AppendMessage(const String &msg, UserData ud)
      {
         return (new MT_AppendMessage(this, ud, msg))->Start();
      }

   /** Expunge messages.
    */
   virtual Ticket ExpungeMessages(void)
      {
         return (new MT_Expunge(this))->Start();
      }

   /** Old-style interface on single messages. */
   //@{
   /** Delete a message.
       @param uid the message uid
   */
   virtual Ticket DeleteMessage(unsigned long uid)
      {
         INTARRAY uids;
         uids.Add(uid);
         return DeleteMessages(&uids, NULL);
      }

   /** UnDelete a message.
       @param uid the message uid
       @return ResultInt with boolean success value
   */
   virtual Ticket UnDeleteMessage(unsigned long uid)
      {
         INTARRAY uids;
         uids.Add(uid);
         return UnDeleteMessages(&uids, NULL);
      }

   /** Set flags on a messages. Possible flag values are MSG_STAT_xxx
       @param uid the message uid
       @pa   ram flag flag to be set, e.g. "\\Deleted"
       @param se   t if true, set the flag, if false, clear it
   */
   virtual void SetMessageFlag(unsigned long uid,
                               int flag, bool set)
      {
         String sequence; sequence.Printf("%lu", uid);
         SetSequenceFlag(sequence, flag, set);
      }
   //@}


   /** @name Some higher level functionality implemented by the
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
                               bool updateCount,
                               UserData ud)
      {
         return (new MT_SaveMessages(this, ud, selections, folderName,
                                     isProfile, updateCount))->Start();
      }

   /** Mark messages as deleted.
       @param messages pointer to an array holding the message numbers
       @return ResultInt boolean
   */
   virtual Ticket DeleteMessages(const INTARRAY *messages,
                                 UserData ud)
      {
         return SetFlag(messages, MailFolder::MSG_STAT_DELETED,true);
      }

   /** Mark messages as no longer deleted.
       @param messages pointer to an array holding the message numbers
       @return ResultInt boolean
   */
   virtual Ticket UnDeleteMessages(const INTARRAY *messages, UserData ud)
      {
         return SetFlag(messages, MailFolder::MSG_STAT_DELETED, false);
      }

   /** Save messages to a file.
       @param messages pointer to an array holding the message numbers
       @parent parent window for dialog
       @return ResultInt boolean
   */
   virtual Ticket SaveMessagesToFile(const INTARRAY *messages,
                                     MWindow *parent, UserData ud)
      {
         return (new MT_SaveMessagesToFileOrFolder(this, ud,
                                                   Op_SaveMessagesToFile,
                                                   messages, parent))->Start();
      }

   /** Save messages to a folder.
       @param messages pointer to an array holding the message numbers
       @param parent window for dialog
       @return true if messages got saved
   */
   virtual Ticket SaveMessagesToFolder(const INTARRAY *messages,
                                       MWindow *parent,
                                       UserData ud)
      {
         return (new MT_SaveMessagesToFileOrFolder(this, ud,
                                                   Op_SaveMessagesToFolder,
                                                   messages, parent))->Start();
      }

   /** Reply to selected messages.
       @param messages pointer to an array holding the message numbers
       @param parent window for dialog
       @param profile pointer for environment
   */
   virtual Ticket ReplyMessages(const INTARRAY *messages,
                                MWindow *parent,
                                ProfileBase *profile,
                                int flags,
                                UserData ud)
   {
      return (new MT_ReplyForwardMessages(this, ud,
                                          Op_ReplyMessages,
                                          messages, parent,
                                          profile, flags))->Start();
   }

   /** Forward selected messages.
       @param messages pointer to an array holding the message numbers
       @param parent window for dialog
       @param profile pointer for environment
   */
   virtual Ticket ForwardMessages(const INTARRAY *messages,
                                  MWindow *parent,
                                  ProfileBase *profile,
                                  UserData ud)
   {
      return (new MT_ReplyForwardMessages(this, ud,
                                          Op_ForwardMessages,
                                          messages, parent,
                                          profile, 0))->Start();
   }

   /**@name Subscription management */
   //@{
   /** Subscribe to a given mailbox (related to the
       mailfolder/mailstream underlying this folder.
       @param mailboxname name of the mailbox to subscribe to
       @param bool if true, subscribe, else unsubscribe
   */
   virtual Ticket Subscribe(const String &mailboxname,
                            bool subscribe,
                            UserData ud) const
      {
         return (new MT_Subscribe((ASMailFolder *)this, ud, mailboxname,
                            subscribe))->Start();
      }
   /** Get a listing of all mailboxes.
       @param pattern a wildcard matching the folders to list
       @param subscribed_only if true, only the subscribed ones
       @param reference implementation dependend reference
    */
   virtual Ticket ListFolders(const String &pattern,
                              bool subscribed_only,
                              const String &reference,
                              ASMailFolder::UserData ud) const
   {
      return (new MT_ListFolders((ASMailFolder *)this, ud,
                                 pattern,
                                 subscribed_only,
                                 reference))->Start();
   }
   //@}

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
   virtual unsigned long CountMessages(int mask, int value) const
      { AScheck(); return m_MailFolder->CountMessages(); }
   
   /** Returns a HeaderInfo structure for a message with a given
       sequence number. This can be used to obtain the uid.
       @param msgno message sequence number, starting from 0
       @return a pointer to the messages current header info entry
   */
   virtual const class HeaderInfo *GetHeaderInfo(unsigned long msgno) const
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
   virtual void EnableNewMailEvents(bool send = true,
                                    bool update = true)
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
                                                     HeaderInfo* hi) const
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
   /// Returns the underlying MailFolder object.
   virtual MailFolder *GetMailFolder(void) const
      { AScheck(); return m_MailFolder;}
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
   /// A mutex to control access to the folder
   MMutex m_Mutex;
};


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

   ASMailFolderImpl

 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

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
   m_Mutex.Lock();
}

void
ASMailFolderImpl::UnLockFolder(void)
{
   AScheck();
   ASSERT(m_Locked == true);
   m_Locked = false;
   m_Mutex.Unlock();
}



/* static */
ASMailFolder *
ASMailFolder::Create(MailFolder *mf)
{
   return new ASMailFolderImpl(mf);
}

