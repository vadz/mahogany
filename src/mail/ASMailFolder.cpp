/*-*- c++ -*-********************************************************
 * ASMailFolder class: asynchronous handling of mail folders        *
 *                                                                  *
 * (C) 1999-2000 by Karsten Ballüder (ballueder@gmx.net)            *
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

#include "miscutil.h"

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

static Ticket gs_Ticket = ILLEGAL_TICKET;

Ticket
ASMailFolder::GetTicket(void)
{
   if(gs_Ticket == ILLEGAL_TICKET) gs_Ticket = 0;
   return gs_Ticket++;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

   MailThread - one thread for each operation to be done

 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifdef USE_THREADS
#   include <wx/thread.h>
#endif


static
UIdArray *Copy(const UIdArray *old)
{
   UIdArray *newarray = new UIdArray;
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
   MailThread(ASMailFolder *mf, UserData ud)
      {
         m_ASMailFolder = mf;
         SafeIncRef(mf);
         m_MailFolder = mf ? mf->GetMailFolder() : NULL;
         m_UserData = ud;
      }

   Ticket Start(void)
      {
         m_Ticket = ASMailFolder::GetTicket();
#ifdef USE_THREADS
         if ( Create() != wxTHREAD_NO_ERROR )
         {
            wxLogError(_("Cannot create thread!"));
         }
#endif
         Run();

         Ticket ticketSave = m_Ticket;  // can't use m_XXX after delete this
#ifndef USE_THREADS
         // We are not using wxThread, so we must delete this when we
         // are done.
         delete this;
#endif
         return ticketSave;
      }

//#ifndef USE_THREADS
   virtual ~MailThread()
      {
         SafeDecRef(m_MailFolder);
         SafeDecRef(m_ASMailFolder);
      }
//#endif

protected:
   void SendEvent(ASMailFolder::Result *result);
   inline void LockFolder(void)
      { if ( m_ASMailFolder ) m_ASMailFolder->LockFolder(); };
   inline void UnLockFolder(void)
      { if ( m_ASMailFolder ) m_ASMailFolder->UnLockFolder(); };

   virtual void *Entry();
#ifndef USE_THREADS
   virtual void Run(void) { Entry(); }
#endif
   
   virtual void  WorkFunction(void) = 0;

protected:
   class ASMailFolder *m_ASMailFolder;
   MailFolder            *m_MailFolder;
   UserData m_UserData;
   Ticket   m_Ticket;
   static Ticket ms_Ticket;
};


void *
MailThread::Entry()
{
   //FIXME-MT: IS THIS TRUE? MailFolderCC does sufficient locking
   //LockFolder();
   WorkFunction();
   //UnLockFolder();
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
                 UserData ud,
                 const UIdArray *selections)
      : MailThread(mf, ud)
      {
         m_Seq = Copy(selections);
      }
   ~MailThreadSeq()
      {
         // We don't delete the m_Seq array, this will be deleted by
         // the Result object when no longer needed.
         // After passing it to the Result, we set it to NULL to check.
         ASSERT(m_Seq == NULL);
      }
protected:
   UIdArray *m_Seq;
};

class MT_Ping : public MailThread
{
public:
   MT_Ping(ASMailFolder *mf,
           UserData ud)
      : MailThread(mf, ud) {}
   virtual void WorkFunction(void)
      {  // Ping() does its own locking
         //m_MailFolder->UnLock();
         m_MailFolder->Ping();
         //m_MailFolder->Lock();
      }
};

class MT_SetSequenceFlag : public MailThread
{
public:
   MT_SetSequenceFlag(ASMailFolder *mf, UserData ud,
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
   MT_SetFlag(ASMailFolder *mf, UserData ud,
              const UIdArray *sequence,
              int flag, bool set)
      : MailThreadSeq(mf, ud, sequence)
      {
         m_Flag = flag;
         m_Set = set;
      }
   virtual void WorkFunction(void)
      {
         m_MailFolder->SetFlag(m_Seq, m_Flag, m_Set);
         delete m_Seq;
#ifdef DEBUG
         m_Seq = NULL;
#endif
      }
protected:
   int m_Flag;
   bool m_Set;
};

class MT_DeleteOrTrashMessages : public MailThreadSeq
{
public:
   MT_DeleteOrTrashMessages(ASMailFolder *mf, UserData ud,
                            const UIdArray *sequence)
      : MailThreadSeq(mf, ud, sequence)
      {
      }
   virtual void WorkFunction(void)
      {
         m_MailFolder->DeleteOrTrashMessages(m_Seq);
         // we don´t send a result event, so we need to delete it:
         delete m_Seq;
#ifdef DEBUG
         m_Seq = NULL;
#endif
      }
protected:
   String m_Sequence;
};

class MT_DeleteMessages : public MailThreadSeq
{
public:
   MT_DeleteMessages(ASMailFolder *mf, UserData ud,
                     const UIdArray *sequence,
                     bool expunge)
      : MailThreadSeq(mf, ud, sequence)
      {
         m_expunge = expunge;
      }
   virtual void WorkFunction(void)
      {
         m_MailFolder->DeleteMessages(m_Seq, m_expunge);
         // we don´t send a result event, so we need to delete it:
         delete m_Seq;
#ifdef DEBUG
         m_Seq = NULL;
#endif
      }
protected:
   String m_Sequence;
   bool   m_expunge;
};

class MT_GetMessage : public MailThread
{
public:
   MT_GetMessage(ASMailFolder *mf,
                 UserData ud,
                 unsigned long uid)

      : MailThread(mf, ud)
      {
         m_UId = uid;
      }
   virtual void WorkFunction(void)
      {
         Message *msg = m_MailFolder->GetMessage(m_UId);
         SendEvent(ASMailFolder::ResultMessage::Create(
            m_ASMailFolder, m_Ticket, NULL,
            msg, m_UId, m_UserData));
      }
protected:
   unsigned long m_UId;
};


class MT_AppendMessage : public MailThread
{
public:
   MT_AppendMessage(ASMailFolder *mf,
                    UserData ud,
                    const Message *msg)

      : MailThread(mf, ud)
      {
         m_Message = (Message *)msg;
         m_Message->IncRef();
      }
   MT_AppendMessage(ASMailFolder *mf,
                    UserData ud,
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
                          ASMailFolder::Op_AppendMessage,  NULL,
                          rc,
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

class MT_SearchMessages : public MailThread
{
public:
   MT_SearchMessages(ASMailFolder *mf, UserData ud, const class SearchCriterium *crit)
      : MailThread(mf, ud) { m_Criterium = *crit;}
   virtual void WorkFunction(void)
      {
         UIdArray *msgs = m_MailFolder->SearchMessages(&m_Criterium);
         SendEvent(ASMailFolder::ResultInt::Create(m_ASMailFolder,
                                                   m_Ticket,
                                                   ASMailFolder::Op_SearchMessages,
                                                   msgs,
                                                   msgs->Count(), m_UserData));
      }
private:
   SearchCriterium m_Criterium;
};

class MT_SaveMessages : public MailThreadSeq
{
public:
   MT_SaveMessages(ASMailFolder *mf, UserData ud,
                   const UIdArray *selections,
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
         int rc = m_MailFolder->SaveMessages(m_Seq, m_MfName,
                                             m_IsProfile, m_Update); 
         SendEvent(ASMailFolder::ResultInt
                   ::Create(m_ASMailFolder,
                            m_Ticket,
                            ASMailFolder::Op_SaveMessages,
                            m_Seq,
                            rc, m_UserData));
#ifdef DEBUG
         m_Seq = NULL;
#endif
      }
private:
   String    m_MfName;
   bool      m_IsProfile;
   bool      m_Update;
};

class MT_SaveMessagesToFile : public MailThreadSeq
{
public:
   MT_SaveMessagesToFile(ASMailFolder *mf, UserData ud,
                         const UIdArray *selections,
                         const String &fileName)
      : MailThreadSeq(mf, ud, selections)
      {
         m_Name = fileName;
      }
   virtual void WorkFunction(void)
      {
         int rc = m_MailFolder->SaveMessagesToFile(m_Seq, m_Name);
         SendEvent(ASMailFolder::ResultInt::Create(m_ASMailFolder,
                                                   m_Ticket,
                                                   ASMailFolder::Op_SaveMessagesToFile,
                                                   m_Seq,
                                                   rc, m_UserData));
#ifdef DEBUG
         m_Seq = NULL;
#endif
      }
private:
   String    m_Name;
};

class MT_SaveMessagesToFileOrFolder : public MailThreadSeq
{
public:
   MT_SaveMessagesToFileOrFolder(ASMailFolder *mf,
                                 UserData ud,
                                 ASMailFolder::OperationId op,
                                 const UIdArray *selections,
                                 MWindow *parent,
                                 MFolder *folder = NULL)
      : MailThreadSeq(mf, ud, selections)
      {
         m_Parent = parent;
         m_Folder = folder;
         m_Op = op;
         ASSERT(m_Op == ASMailFolder::Op_SaveMessagesToFile ||
                m_Op == ASMailFolder::Op_SaveMessagesToFolder);
      }
   virtual void WorkFunction(void)
      {
         int rc = m_Op == ASMailFolder::Op_SaveMessagesToFile ?
            m_MailFolder->SaveMessagesToFile(m_Seq, m_Parent)
            : m_MailFolder->SaveMessagesToFolder(m_Seq, m_Parent, m_Folder);
         SendEvent(ASMailFolder::ResultInt::Create(m_ASMailFolder,
                                                   m_Ticket, m_Op,
                                                   m_Seq,
                                                   rc, m_UserData));
#ifdef DEBUG
         m_Seq = NULL;
#endif
      }
private:
   MWindow *m_Parent;
   MFolder *m_Folder;
   ASMailFolder::OperationId m_Op;
};

class MT_ReplyForwardMessages : public MailThreadSeq
{
public:
   MT_ReplyForwardMessages(ASMailFolder *mf, UserData ud,
                           ASMailFolder::OperationId op,
                           const UIdArray *selections,
                           MWindow *parent,
                           const MailFolder::Params& params)
      : MailThreadSeq(mf, ud, selections), m_Params(params)
      {
         m_Parent = parent;
         m_Op = op;
         ASSERT(m_Op == ASMailFolder::Op_ReplyMessages ||
                m_Op == ASMailFolder::Op_ForwardMessages);
      }
   virtual void WorkFunction(void)
      {
         if(m_Op == ASMailFolder::Op_ReplyMessages)
            m_MailFolder->ReplyMessages(m_Seq, m_Params, m_Parent);
         else
            m_MailFolder->ForwardMessages(m_Seq, m_Params, m_Parent);
         delete m_Seq;
#ifdef DEBUG
         m_Seq = NULL;
#endif
      }
private:
   ASMailFolder::OperationId m_Op;
   MWindow *m_Parent;
   MailFolder::Params m_Params;
};

class MT_ApplyFilterRules : public MailThreadSeq
{
public:
   MT_ApplyFilterRules(ASMailFolder *mf, UserData ud,
                       const UIdArray *selections)
      : MailThreadSeq(mf, ud, selections)
      { }
   virtual void WorkFunction(void)
      {
         int result = m_MailFolder->ApplyFilterRules(*m_Seq);
         SendEvent(ASMailFolder::ResultInt::Create(
            m_ASMailFolder, m_Ticket, ASMailFolder::Op_ApplyFilterRules, m_Seq,
            result, m_UserData));
#ifdef DEBUG
         m_Seq = NULL;
#endif
      }
private:
   ASMailFolder::OperationId m_Op;
   MWindow *m_Parent;
   int m_Flags;
};

class MT_Subscribe : public MailThread
{
public:
   MT_Subscribe(ASMailFolder *mf, UserData ud,
                  const String &host,
                  FolderType protocol,
                const String &mailboxname,
                bool subscribe)
      : MailThread(mf, ud)
      {
         m_Host = host;
         m_Prot = protocol;
         m_Name = mailboxname;
         m_Sub = subscribe;
      }
   virtual void WorkFunction(void)
      {
         int rc = m_MailFolder->Subscribe(m_Host, m_Prot, m_Name, m_Sub);
         SendEvent(ASMailFolder::ResultInt::Create(m_ASMailFolder,
                                                   m_Ticket,
                                                   ASMailFolder::Op_Subscribe,
                                                   NULL, rc, m_UserData));
      }
private:
   String  m_Host;
   FolderType m_Prot;
   String  m_Name;
   bool    m_Sub;
};

class MT_ListFolders : public MailThread
{
public:
   MT_ListFolders(ASMailFolder *mf,
                  UserData ud,
                  const String &pattern,
                  bool sub_only,
                  const String &reference)
      : MailThread(mf, ud),
        m_Pattern(pattern),
        m_Reference(reference)
      {
         m_SubOnly = sub_only;
      }

   virtual void WorkFunction(void)
      {
         m_MailFolder->ListFolders(m_ASMailFolder,
                                   m_Pattern,
                                   m_SubOnly,
                                   m_Reference,
                                   m_UserData, m_Ticket);
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
   /// Returns true if we have obtained the lock.
   bool LockFolder(void);
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
   virtual Ticket SetSequenceFlag(const UIdArray *sequence,
                                  int flag,
                                  bool set)
      {
         return (new MT_SetSequenceFlag(this, NULL,
                                        GetSequenceString(sequence),flag,set))->Start(); 
      }

   /** Set flags on a sequence of messages. Possible flag values are MSG_STAT_xxx
       @param sequence the IMAP sequence of uids
       @param flag flag to be set, e.g. "\\Deleted"
       @param set if true, set the flag, if false, clear it
   */
   virtual Ticket SetFlag(const UIdArray *sequence, int flag, bool set)
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


   /** Search Messages.
       @return a Result with a sequence of matching uids.
    */
   virtual Ticket SearchMessages(const class SearchCriterium *crit, UserData ud)
      {
         return (new MT_SearchMessages(this, ud, crit))->Start();
      }
   
   /** Old-style interface on single messages. */
   //@{
   /** Delete a message.
       @param uid the message uid
   */
   virtual Ticket DeleteMessage(unsigned long uid)
      {
         UIdArray uids;
         uids.Add(uid);
         return DeleteMessages(&uids, false, NULL);
      }

   /** UnDelete a message.
       @param uid the message uid
       @return ResultInt with boolean success value
   */
   virtual Ticket UnDeleteMessage(unsigned long uid)
      {
         UIdArray uids;
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
         UIdArray *ia = new UIdArray;
         ia->Add(uid);
         SetSequenceFlag(ia, flag, set);
         delete ia;
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
   virtual Ticket SaveMessages(const UIdArray *selections,
                               String const & folderName,
                               bool isProfile,
                               bool updateCount,
                               UserData ud)
      {
         return (new MT_SaveMessages(this, ud, selections, folderName,
                                     isProfile, updateCount))->Start();
      }

   /** Mark messages as deleted or move them to trash.
       @param messages pointer to an array holding the message numbers
       @return ResultInt boolean
   */
   virtual Ticket DeleteOrTrashMessages(const UIdArray *messages,
                                        UserData ud)
      {
         return (new MT_DeleteOrTrashMessages(this, ud, messages))->Start();
      }
   /** Mark messages as deleted.
       @param messages pointer to an array holding the message numbers
       @return ResultInt boolean
   */
   virtual Ticket DeleteMessages(const UIdArray *messages,
                                 bool expunge,
                                 UserData ud)
      {
         return (new MT_DeleteMessages(this, ud, messages, expunge))->Start();
      }

   /** Mark messages as no longer deleted.
       @param messages pointer to an array holding the message numbers
       @return ResultInt boolean
   */
   virtual Ticket UnDeleteMessages(const UIdArray *messages, UserData ud)
      {
         return SetFlag(messages, MailFolder::MSG_STAT_DELETED, false);
      }

   /** Save messages to a file.
       @param messages pointer to an array holding the message numbers
       @parent parent window for dialog
       @return ResultInt boolean
   */
   virtual Ticket SaveMessagesToFile(const UIdArray *messages,
                                     const String &fileName, UserData ud)
      {
         return (new MT_SaveMessagesToFile(this, ud,
                                           messages, fileName))->Start();
      }

   /** Save messages to a file.
       @param messages pointer to an array holding the message numbers
       @parent parent window for dialog
       @return ResultInt boolean
   */
   virtual Ticket SaveMessagesToFile(const UIdArray *messages,
                                     MWindow *parent, UserData ud)
      {
         return (new MT_SaveMessagesToFileOrFolder(this, ud,
                                                   Op_SaveMessagesToFile,
                                                   messages, parent))->Start();
      }

   /** Save messages to a folder.
       @param messages pointer to an array holding the message numbers
       @param parent window for dialog
       @param folder is the folder to save to, ask the user if NULL
       @return true if messages got saved
   */
   virtual Ticket SaveMessagesToFolder(const UIdArray *messages,
                                       MWindow *parent,
                                       MFolder *folder,
                                       UserData ud)
      {
         return (new MT_SaveMessagesToFileOrFolder(this, ud,
                                                   Op_SaveMessagesToFolder,
                                                   messages,
                                                   parent,
                                                   folder))->Start();
      }

   /** Reply to selected messages.
       @param messages pointer to an array holding the message numbers
       @param parent window for dialog
   */
   virtual Ticket ReplyMessages(const UIdArray *messages,
                                const MailFolder::Params& params,
                                MWindow *parent,
                                UserData ud)
   {
      return (new MT_ReplyForwardMessages(this, ud,
                                          Op_ReplyMessages,
                                          messages,
                                          parent,
                                          params))->Start();
   }

   /** Forward selected messages.
       @param messages pointer to an array holding the message numbers
       @param parent window for dialog
   */
   virtual Ticket ForwardMessages(const UIdArray *messages,
                                  const MailFolder::Params& params,
                                  MWindow *parent,
                                  UserData ud)
   {
      return (new MT_ReplyForwardMessages(this, ud,
                                          Op_ForwardMessages,
                                          messages, parent,
                                          params))->Start();
   }
   /** Apply filter rules to the folder.
       Applies the rule to all messages listed in msgs.
       @return -1 if no filter module exists, return code otherwise
   */
   virtual Ticket ApplyFilterRules(const UIdArray * msgs, UserData ud)
   {
      return (new MT_ApplyFilterRules(this, ud, msgs))->Start();
   } 

   /**@name Subscription management */
   //@{
   /** Subscribe to a given mailbox (related to the
       mailfolder/mailstream underlying this folder.
       @param host the server host, or empty for local newsspool
       @param protocol MF_IMAP or MF_NNTP or MF_NEWS
       @param mailboxname name of the mailbox to subscribe to
       @param bool if true, subscribe, else unsubscribe
   */
   static Ticket Subscribe(const String &host,
                           FolderType protocol,
                           const String &mailboxname,
                           bool subscribe,
                           UserData ud)
      {
         return (new MT_Subscribe(NULL, ud, host, protocol,
                                  mailboxname, subscribe))->Start();
      }
   /** Get a listing of all mailboxes.
       @param pattern a wildcard matching the folders to list
       @param subscribed_only if true, only the subscribed ones
       @param reference implementation dependend reference
    */
   Ticket ListFolders(const String &pattern,
                      bool subscribed_only,
                      const String &reference,
                      UserData ud)
   {
      return (new MT_ListFolders(this, ud,
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
      { AScheck(); return m_MailFolder->CountMessages(mask, value); }

   /** Get the profile.
       @return Pointer to the profile.
   */
   inline Profile *GetProfile(void) const
      { AScheck(); return m_MailFolder->GetProfile(); }




   /** Toggle update behaviour flags.
       @param updateFlags the flags to set
   */
   virtual void SetUpdateFlags(int updateFlags)
      { AScheck(); m_MailFolder->SetUpdateFlags(updateFlags); }
   /// Get the current update flags
   virtual int  GetUpdateFlags(void) const
      { AScheck(); return m_MailFolder->GetUpdateFlags(); }

   /**@name Functions to get an overview of messages in the folder. */
   //@{
   /** Returns a listing of the folder. Must be DecRef'd by caller. */
   virtual class HeaderInfoList *GetHeaders(void) const
      { AScheck(); return m_MailFolder->GetHeaders(); }
   //@}
   /// Return the folder's type.
   virtual FolderType GetType(void) const
      { AScheck(); return m_MailFolder->GetType(); }
   /** Sets a maximum number of messages to retrieve from server.
       @param nmax maximum number of messages to retrieve, 0 for no limit
   */
   virtual void SetRetrievalLimit(unsigned long nmax)
      { AScheck(); m_MailFolder->SetRetrievalLimits(nmax, 0); }
   /// Returns the underlying MailFolder object.
   virtual MailFolder *GetMailFolder(void) const
      { AScheck(); m_MailFolder->IncRef(); return m_MailFolder;}
   //@}
private:
   /// The synchronous mailfolder that we map to.
   MailFolder *m_MailFolder;
   /// Temporary, to control access:
   bool m_Locked;
   /// Next ticket Id to use.
   static Ticket ms_Ticket;

   MOBJECT_DEBUG(ASMailFolderImpl)
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

bool
ASMailFolderImpl::LockFolder(void)
{
   AScheck();
   return m_MailFolder->Lock();
}

void
ASMailFolderImpl::UnLockFolder(void)
{
   AScheck();
   m_MailFolder->UnLock();
}



/* static */
ASMailFolder *
ASMailFolder::Create(MailFolder *mf)
{
   return new ASMailFolderImpl(mf);
}


class ASTicketListImpl : public ASTicketList
{
public:
   virtual bool Contains(Ticket t) const
      {
         for(size_t i = 0; i < m_Tickets.Count(); i++)
            if( m_Tickets[i] == t)
               return true;
         return false;
      }
   virtual void Add(Ticket t)
      {
         ASSERT(!Contains(t));
         m_Tickets.Add(t);
      }
   virtual void Remove(Ticket t)
      {
         ASSERT(Contains(t));

         // be careful to cast to int to invoke the "Remove-by-value" version,
         // not RemoveAt()
         m_Tickets.Remove( (int) t);
      }
   virtual void Clear(void)
      {
         m_Tickets.Clear();
      }

   virtual bool IsEmpty(void) const { return m_Tickets.IsEmpty(); }
   virtual Ticket Pop(void)
   {
      size_t n = m_Tickets.GetCount();
      CHECK( n > 0, ILLEGAL_TICKET, "ticket list is empty in Pop()" );

      Ticket t = m_Tickets.Last();
      m_Tickets.RemoveAt(n - 1);

      return t;
   }

private:
   wxArrayInt m_Tickets;
};

/* static */
ASTicketList * ASTicketList::Create(void)
{
   return new ASTicketListImpl();
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

   Static functions from ASMailFolder base class.

 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/** Subscribe to a given mailbox (related to the
    mailfolder/mailstream underlying this folder.
    @param mailboxname name of the mailbox to subscribe to
    @param bool if true, subscribe, else unsubscribe
*/
/* static */
Ticket
ASMailFolder::Subscribe(const String &host,
                        FolderType protocol,
                        const String &mailboxname,
                        bool subscribe,
                        UserData ud)
{
   return ASMailFolderImpl::Subscribe(host, protocol, mailboxname, subscribe, ud);
}

Ticket
ASMailFolder::ListFolders(const String &pattern,
                          bool subscribed_only,
                          const String &reference,
                          UserData ud)
{
   return ((ASMailFolderImpl *)this)->ListFolders(pattern,
                                                  subscribed_only,
                                                  reference, ud);
}

char ASMailFolder::GetFolderDelimiter() const
{
   MailFolder *mf = GetMailFolder();

   CHECK( mf, '\0', "no folder in GetFolderDelimiter" );

   char chDelim = mf->GetFolderDelimiter();
   mf->DecRef();

   return chDelim;
}

String ASMailFolder::GetImapSpec(void) const
{
   String spec;
   MailFolder *mf = GetMailFolder();
   if ( mf )
   {
      spec = ((MailFolderCC *)mf)->GetImapSpec();

      mf->DecRef();
   }

   return spec;
}

// ----------------------------------------------------------------------------
// debugging functions
// ----------------------------------------------------------------------------

#ifdef DEBUG

String ASMailFolderImpl::DebugDump() const
{
   String s1 = MObjectRC::DebugDump(), s2;
   s2.Printf("name '%s'", GetName().c_str());

   return s1 + s2;
}

String ASMailFolder::ResultImpl::DebugDump() const
{
   String s1 = MObjectRC::DebugDump(), s2;
   s2.Printf("operation id = %d, folder '%s'", m_Id, m_Mf->GetName().c_str());

   return s1 + s2;
}

#endif // DEBUG
