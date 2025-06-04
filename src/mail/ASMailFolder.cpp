/*-*- c++ -*-********************************************************
 * ASMailFolder class: asynchronous handling of mail folders        *
 *                                                                  *
 * (C) 1999-2000 by Karsten Ballüder (ballueder@gmx.net)            *
 *                                                                  *
 * $Id$
 *******************************************************************/

#include  "Mpch.h"

#ifndef USE_PCH
#   include "Mcommon.h"
#   include "guidef.h"    // only for high-level functions
#endif // USE_PCH

#include "Sequence.h"
#include "UIdArray.h"

#include "MSearch.h"

#include "ASMailFolder.h"
#include "MailFolderCC.h"

/// Call this always before using it.
#ifdef DEBUG
#   define   AScheck()   { MOcheck(); ASSERT(m_MailFolder); }
#else
#   define   AScheck()
#endif

// subscription code is not used for now and probably broken, so it is not
// compiled in -- but keep it for the future
//#define USE_SUBSCRIBE

ASMailFolderResultImpl::~ASMailFolderResultImpl()
{
   if ( m_Mf )
      m_Mf->DecRef();

   delete m_Seq;
}

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
#   include <wx/thread.h>       // for wxThread
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
   MBusyCursor bc;

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
   MT_SetSequenceFlag(ASMailFolder *mf,
                      UserData ud,
                      MailFolder::SequenceKind kind,
                      const Sequence& sequence,
                      int flag, bool set)
      : MailThread(mf, ud), m_Sequence(sequence)
      {
         m_Kind = kind;
         m_Flag = flag;
         m_Set = set;
      }
   virtual void WorkFunction(void)
      {
         m_MailFolder->SetSequenceFlag(m_Kind, m_Sequence, m_Flag, m_Set);
      }

protected:
   const Sequence m_Sequence;
   MailFolder::SequenceKind m_Kind;
   int m_Flag;
   bool m_Set;

   DECLARE_NO_COPY_CLASS(MT_SetSequenceFlag)
};

class MT_SetFlagForAll : public MailThread
{
public:
   MT_SetFlagForAll(ASMailFolder *mf,
                    UserData ud,
                    int flag, bool set)
      : MailThread(mf, ud)
      {
         m_Flag = flag;
         m_Set = set;
      }
   virtual void WorkFunction(void)
      {
         m_MailFolder->SetFlagForAll(m_Flag, m_Set);
      }

protected:
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
                            const UIdArray *sequence,
                            int flags)
      : MailThreadSeq(mf, ud, sequence)
      {
         m_Flags = flags;
      }

   virtual void WorkFunction(void)
      {
         bool rc = m_MailFolder->DeleteOrTrashMessages(m_Seq, m_Flags);
         SendEvent(ASMailFolder::ResultInt::Create
                   (
                     m_ASMailFolder,
                     m_Ticket,
                     ASMailFolder::Op_DeleteMessages,
                     m_Seq,
                     rc,
                     m_UserData
                    )
                   );
#ifdef DEBUG
         m_Seq = NULL;
#endif
      }

private:
   int m_Flags;
};

class MT_DeleteMessages : public MailThreadSeq
{
public:
   MT_DeleteMessages(ASMailFolder *mf, UserData ud,
                     const UIdArray *sequence,
                     int flags)
      : MailThreadSeq(mf, ud, sequence)
      {
         m_Flags = flags;
      }
   virtual void WorkFunction(void)
      {
         bool rc = m_MailFolder->DeleteMessages(m_Seq, m_Flags);
         SendEvent(ASMailFolder::ResultInt::Create
                   (
                     m_ASMailFolder,
                     m_Ticket,
                     ASMailFolder::Op_DeleteMessages,
                     m_Seq,
                     rc,
                     m_UserData
                    )
                   );
#ifdef DEBUG
         m_Seq = NULL;
#endif
      }

protected:
   int m_Flags;
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
         SendEvent(ASMailFolder::ResultMessage::Create
                   (
                     m_ASMailFolder,
                     m_Ticket,
                     NULL,
                     msg,
                     m_UId,
                     m_UserData
                    )
                  );
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
         m_Message->DecRef();
      }
   virtual void WorkFunction(void)
      {
         int rc = m_Message ? m_MailFolder->AppendMessage(*m_Message)
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

class MT_MarkRead : public MailThreadSeq
{
private:
   bool m_read;
public:
   MT_MarkRead(ASMailFolder *mf,
               UserData ud,
               const UIdArray *selections,
               bool read)
      : MailThreadSeq(mf, ud, selections) 
      , m_read(read)
   {}
   virtual void WorkFunction(void)
   { 
      bool rc = m_MailFolder->SetFlag(m_Seq, MailFolder::MSG_STAT_SEEN, m_read);
      //bool rc = m_MailFolder->MarkRead(m_Seq, (bool)m_UserData); 
      SendEvent(ASMailFolder::ResultInt::Create
                (
                  m_ASMailFolder,
                  m_Ticket,
                  (m_read ? ASMailFolder::Op_MarkRead : ASMailFolder::Op_MarkUnread),
                  m_Seq,
                  rc,
                  m_UserData
                 )
               );
#ifdef DEBUG
         m_Seq = NULL;
#endif
   }
};

class MT_SearchMessages : public MailThread
{
public:
   MT_SearchMessages(ASMailFolder *mf, UserData ud, const SearchCriterium *crit)
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
   MT_SaveMessages(ASMailFolder *mf,
                   UserData ud,
                   const UIdArray *selections,
                   const String &folderName)
      : MailThreadSeq(mf, ud, selections)
      {
         m_MfName = folderName;
      }
   virtual void WorkFunction(void)
      {
         int rc = m_MailFolder->SaveMessages(m_Seq, m_MfName);
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
                                 wxWindow *parent,
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
         int rc = m_Op == ASMailFolder::Op_SaveMessagesToFile
                  ? m_MailFolder->SaveMessagesToFile(m_Seq, wxEmptyString, m_Parent)
                  : m_MailFolder->SaveMessages(m_Seq, m_Folder);
         SendEvent(ASMailFolder::ResultInt::Create(m_ASMailFolder,
                                                   m_Ticket, m_Op,
                                                   m_Seq,
                                                   rc, m_UserData));
#ifdef DEBUG
         m_Seq = NULL;
#endif
      }
private:
   wxWindow *m_Parent;
   MFolder *m_Folder;
   ASMailFolder::OperationId m_Op;
};

class MT_ReplyForwardMessages : public MailThreadSeq
{
public:
   MT_ReplyForwardMessages(ASMailFolder *mf, UserData ud,
                           ASMailFolder::OperationId op,
                           const UIdArray *selections,
                           wxWindow *parent,
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
   wxWindow *m_Parent;
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
   wxWindow *m_Parent;
   int m_Flags;
};

#ifdef USE_SUBSCRIBE

class MT_Subscribe : public MailThread
{
public:
   MT_Subscribe(ASMailFolder *mf, UserData ud,
                const String &host,
                MFolderType protocol,
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
   MFolderType m_Prot;
   String  m_Name;
   bool    m_Sub;
};

#endif // USE_SUBSCRIBE

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

   virtual Ticket SetFlagForAll(int flag, bool set = true)
      {
         return (new MT_SetFlagForAll(this, NULL, flag, set))->Start();
      }

   /** Set flags on a sequence of messages. Possible flag values are MSG_STAT_xxx
       @param sequence the IMAP sequence of uids
       @param flag flag to be set, e.g. "\\Deleted"
       @param set if true, set the flag, if false, clear it
   */
   virtual Ticket SetSequenceFlag(MailFolder::SequenceKind kind,
                                  const Sequence& sequence,
                                  int flag,
                                  bool set)
      {
         return (new MT_SetSequenceFlag(this, NULL,
                                        kind,sequence,flag,set))->Start();
      }

   /** Set flags on a sequence of messages. Possible flag values are MSG_STAT_xxx
       @param sequence the IMAP sequence of uids
       @param flag flag to be set, e.g. "\\Deleted"
       @param set if true, set the flag, if false, clear it
   */
   virtual Ticket SetFlag(const UIdArray *sequence, int flag, bool set)
      {
         return (new MT_SetFlag(this, NULL, sequence, flag, set))->Start();
      }


   /** Appends the message to this folder.
       @param msg the message to append
       @return true on success
   */
   virtual Ticket AppendMessage(const Message *msg, UserData ud )
      {
         CHECK(msg, ILLEGAL_TICKET, "NULL message");

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

   /** Mark messages read/unread.
       @param selections the message indices which will be converted using the current listing
       @param read true if messages must be marked read
     */
   virtual Ticket MarkRead(const UIdArray *selections,
                           UserData ud,
                           bool read)
      {
         return (new MT_MarkRead(this, ud, selections, read))->Start();
      }

   /** Search Messages.
       @return a Result with a sequence of matching uids.
    */
   virtual Ticket SearchMessages(const SearchCriterium *crit, UserData ud)
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
         UIdArray ua;
         ua.Add(uid);

         SetFlag(&ua, flag, set);
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
       @return ResultInt boolean
   */
   virtual Ticket SaveMessages(const UIdArray *selections,
                               String const & folderName,
                               UserData ud)
      {
         return (new MT_SaveMessages(this, ud, selections, folderName))->Start();
      }

   /** Mark messages as deleted or move them to trash.
       @param messages pointer to an array holding the message numbers
       @param flags combination of MailFolder::DELETE_XXX bit flags
       @return ResultInt boolean
   */
   virtual Ticket DeleteOrTrashMessages(const UIdArray *messages,
                                        int flags,
                                        UserData ud)
      {
         return (new MT_DeleteOrTrashMessages(this, ud, messages, flags))->Start();
      }

   /** Mark messages as deleted.
       @param messages pointer to an array holding the message numbers
       @return ResultInt boolean
   */
   virtual Ticket DeleteMessages(const UIdArray *messages,
                                 int flags,
                                 UserData ud)
      {
         return (new MT_DeleteMessages(this, ud, messages, flags))->Start();
      }

   /** Mark messages as no longer deleted.
       @param messages pointer to an array holding the message numbers
       @return ResultInt boolean
   */
   virtual Ticket UnDeleteMessages(const UIdArray *messages, UserData /* ud */)
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
                                     wxWindow *parent, UserData ud)
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
                                       wxWindow *parent,
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
                                wxWindow *parent,
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
                                  wxWindow *parent,
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
#ifdef USE_SUBSCRIBE
   /** Subscribe to a given mailbox (related to the
       mailfolder/mailstream underlying this folder.
       @param host the server host, or empty for local newsspool
       @param protocol MF_IMAP or MF_NNTP or MF_NEWS
       @param mailboxname name of the mailbox to subscribe to
       @param bool if true, subscribe, else unsubscribe
   */
   static Ticket Subscribe(const String &host,
                           MFolderType protocol,
                           const String &mailboxname,
                           bool subscribe,
                           UserData ud)
      {
         return (new MT_Subscribe(NULL, ud, host, protocol,
                                  mailboxname, subscribe))->Start();
      }
#endif // USE_SUBSCRIBE

   /** Get a listing of all mailboxes.
       @param pattern a wildcard matching the folders to list
       @param subscribed_only return only the folders we're subscribed to if true
       @param reference the path to start from
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
   /** Get the profile.
       @return Pointer to the profile.
   */
   Profile *GetProfile(void) const
      { AScheck(); return m_MailFolder->GetProfile(); }

   /**@name Functions to get an overview of messages in the folder. */
   //@{
   /** Returns a listing of the folder. Must be DecRef'd by caller. */
   virtual HeaderInfoList *GetHeaders(void) const
      { AScheck(); return m_MailFolder->GetHeaders(); }
   //@}
   /// Return the folder's type.
   virtual MFolderType GetType(void) const
      { AScheck(); return m_MailFolder->GetType(); }
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
      CHECK( n > 0, ILLEGAL_TICKET, _T("ticket list is empty in Pop()") );

      Ticket t = m_Tickets.Last();
      m_Tickets.RemoveAt(n - 1);

      return t;
   }

private:
   wxArrayInt m_Tickets;

   MOBJECT_DEBUG(ASTicketListImpl)
};

/* static */
ASTicketList * ASTicketList::Create(void)
{
   return new ASTicketListImpl();
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

   Static functions from ASMailFolder base class.

 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifdef USE_SUBSCRIBE

/** Subscribe to a given mailbox (related to the
    mailfolder/mailstream underlying this folder.
    @param mailboxname name of the mailbox to subscribe to
    @param bool if true, subscribe, else unsubscribe
*/
/* static */
Ticket
ASMailFolder::Subscribe(const String &host,
                        MFolderType protocol,
                        const String &mailboxname,
                        bool subscribe,
                        UserData ud)
{
   return ASMailFolderImpl::Subscribe(host, protocol, mailboxname, subscribe, ud);
}

#endif // USE_SUBSCRIBE

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

   CHECK( mf, '\0', _T("no folder in GetFolderDelimiter") );

   char chDelim = mf->GetFolderDelimiter();
   mf->DecRef();

   return chDelim;
}

// ----------------------------------------------------------------------------
// debugging functions
// ----------------------------------------------------------------------------

#ifdef DEBUG

String ASMailFolderImpl::DebugDump() const
{
   String s1 = MObjectRC::DebugDump(), s2;
   s2.Printf(_T("name '%s'"), GetName().c_str());

   return s1 + s2;
}

String ASMailFolder::ResultImpl::DebugDump() const
{
   String s1 = MObjectRC::DebugDump(), s2;
   s2.Printf(_T("operation id = %d, folder '%s'"), m_Id, m_Mf->GetName().c_str());

   return s1 + s2;
}

String ASTicketListImpl::DebugDump() const
{
   return ASTicketList::DebugDump() << m_Tickets.GetCount() << _T(" tickets");
}

#endif // DEBUG
