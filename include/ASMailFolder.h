/*-*- c++ -*-********************************************************
 * ASMailFolder class: ABC defining the interface for asynchronous  *
 *                     mail folders                                 *
 *                                                                  *
 * (C) 1999 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *******************************************************************/



#ifndef ASMAILFOLDER_H
#define ASMAILFOLDER_H

#ifdef __GNUG__
#   pragma interface "ASMailFolder.h"
#endif

#include "FolderType.h"

/** The INTARRAY define is a class which is an integer array. It needs
    to provide a int Count() method to return the number of elements
    and an int operator[int] to access them.
    We use wxArrayInt for this.
*/
#define INTARRAY wxArrayInt

// forward declarations
class FolderView;
class ProfileBase;
class INTARRAY;
class MWindow;

#include "Message.h"
//class Message;


/**
   ASMailFolder abstract base class, represents anything containig mails.

   This class defines the interface for all ASMailFolder classes. It can
   represent anything which contains messages, e.g. folder files, POP3
   or IMAP connections or even newsgroups.


   To open a MailFolder via OpenFolder() one must do either of the
   following:
   <ul>
   <li>Use it with a MF_PROFILE type and provide a profile name to be
       used.
   <li>Use it with a different type and set other options such as
       login/password manually. This will use a dummy profile
       inheriting from the global profile section.
   </ul>
*/
class ASMailFolder : public MObjectRC
{
public:
   /** @name Constants and Types */
   /** Each operation returns a unique number, to identify it. */
   typedef int Ticket;

   /** Each operation can carry some user data. */
   typedef void * UserData;
   
   //@{
   /** What is the status of a given message in the folder?
       Recent messages are those that we never saw in a listing
       before. Once we open a folder, the messages will no longer be
       recent when we close it. Seen are only those that we really
       looked at. */
   enum MessageStatus
   {
      /// empty message status
      MSG_STAT_NONE = 0,
      /// message has been seen
      MSG_STAT_SEEN = 1,
      /// message is deleted
      MSG_STAT_DELETED = 2,
      /// message has been replied to
      MSG_STAT_ANSWERED = 4,
      /// message is "recent" (see RFC 2060)
      MSG_STAT_RECENT = 8,
      /// message matched a search
      MSG_STAT_SEARCHED = 16,
      /// message has been flagged for something
      MSG_STAT_FLAGGED = 64
   };

   /**@name Result classes containing return values from
   operations. */
   //@{
   /** For the Result class, we need one identifier for every
       asynchronous operation to identify what happened.
   */
   enum OperationId
   {
      Op_Ping,
      Op_GetMessage,
      Op_AppendMessage,
      Op_DeleteMessages,
      Op_UnDeleteMessages,
      Op_SaveMessages,
      Op_SaveMessagesToFolder,
      Op_SaveMessagesToFile
      
   };
    /** A structure containing the return values from an operation.
        This will get passed in an MEvent to notify other parts of the
        application that an operation has finished.
    */
   class Result : public MObjectRC
   {
   public:
      /// Returns the user data.
      virtual UserData GetUserData(void) const = 0;
      /// Returns the ticket id.
      virtual Ticket GetTicket(void) const = 0;
      /** Returns a pointer to the ASMailFolder from which this
          event originated.
      */
      virtual ASMailFolder *GetFolder(void) const = 0;
      /// Returns an OperationId to tell what happened.
      virtual OperationId GetOperation(void) const = 0;
   };

   /** Common code shared by all class Result implementations. */
   class ResultImpl : public Result
   {
   public:
      virtual UserData GetUserData(void) const { return m_UserData; }
      virtual Ticket GetTicket(void) const { return m_Ticket; }
      virtual OperationId   GetOperation(void) const { return m_Id; }
      virtual ASMailFolder *GetFolder(void) const{ return m_Mf; }
   protected:
      ResultImpl(ASMailFolder *mf, Ticket t, OperationId id,
                 UserData ud)
         {
            m_Id = id;
            m_Ticket = t;
            m_Mf = mf;
            m_UserData = ud;
            if(m_Mf) m_Mf->IncRef();
         }
      virtual ~ResultImpl()
         { if(m_Mf) m_Mf->DecRef(); }
   private:
      OperationId   m_Id;
      ASMailFolder *m_Mf;
      Ticket        m_Ticket;
      UserData      m_UserData;
   };
   /** Holds the result from an operation which can be expressed as an 
       integer value. Used for all boolean success values.
   */
   class ResultInt : public ResultImpl
   {
   public:
      int GetValue(void) const { return m_Value; }
      static ResultInt *Create(ASMailFolder *mf,
                               Ticket t,
                               OperationId id,
                               int value,
                               UserData ud)
         { return new ResultInt(mf, t, id, value, ud); }
   protected:
      ResultInt(ASMailFolder *mf,
                Ticket t,
                OperationId id,
                int value, 
                UserData ud)
         : ResultImpl(mf, t, id, ud)
         { m_Value = value; }
   private:
      int         m_Value;
   };
   /** Holds the result from a GetMessage() and returns the message pointer.
   */
   class ResultMessage : public ResultImpl
   {
   public:
      static ResultMessage *Create(ASMailFolder *mf,
                                   Ticket t,
                                   Message *msg,
                                   unsigned long uid,
                                   UserData ud)
         { return new ResultMessage(mf, t, msg, uid, ud); }
      Message * GetMessage(void) const { return m_Message; }
      unsigned long GetUId(void) const { return m_uid; }
   protected:
      ResultMessage(ASMailFolder *mf, Ticket t, Message *msg, unsigned
                    long uid, UserData ud)
         : ResultImpl(mf, t, Op_GetMessage, ud)
         {
            m_Message = msg;
            if(m_Message) m_Message->IncRef();
            m_uid =  uid;
         }
      ~ResultMessage() { if(m_Message) m_Message->DecRef(); }
   private:
      Message *m_Message;
      unsigned long m_uid;
   };

   //@}


   static ASMailFolder * Create(MailFolder *mf);

   
   /**@name Asynchronous Access Functions, returning results in events.*/
   //@{
   /** Check whether mailbox has changed.
       @return void, but causes update events to be sent if necessary.
   */
   virtual void Ping(void) = 0;
   /** Delete a message.
       @param uid the message uid
   */
   virtual void DeleteMessage(unsigned long uid) = 0;

   /** UnDelete a message.
       @param uid the message uid
   */
   virtual void UnDeleteMessage(unsigned long uid) = 0;

   /** get the message with unique id uid
       @param uid message uid
       @return ResultInt with boolean success value
   */
   virtual Ticket GetMessage(unsigned long uid, UserData ud = 0) = 0;
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
       @return ResultInt with boolean success value
   */
   virtual Ticket AppendMessage( /* const */ Message *msg, UserData ud = 0) = 0;

   /** Appends the message to this folder.
       @param msg text of the  message to append
       @return ResultInt with boolean success value
   */
   virtual Ticket AppendMessage(const String &msg, UserData ud = 0) = 0;

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
   virtual Ticket SaveMessages(const INTARRAY *selections,
                             String const & folderName,
                             bool isProfile,
                               bool updateCount = true,
                               UserData ud = 0) = 0;

   /** Mark messages as deleted.
       @param messages pointer to an array holding the message numbers
       @return ResultInt boolean
   */
   virtual Ticket DeleteMessages(const INTARRAY *messages, UserData ud = 0) = 0;

   /** Mark messages as no longer deleted.
       @param messages pointer to an array holding the message numbers
       @return ResultInt boolean
   */
   virtual Ticket UnDeleteMessages(const INTARRAY *messages, UserData ud = 0) = 0;

   /** Save messages to a file.
       @param messages pointer to an array holding the message numbers
       @parent parent window for dialog
       @return ResultInt boolean
   */
   virtual Ticket SaveMessagesToFile(const INTARRAY *messages, MWindow
                                   *parent = NULL, UserData ud = 0) = 0;

   /** Save messages to a folder.
       @param messages pointer to an array holding the message numbers
       @param parent window for dialog
       @return true if messages got saved
   */
   virtual Ticket SaveMessagesToFolder(const INTARRAY *messages, MWindow
                                     *parent = NULL, UserData ud = 0) = 0;

   /** Reply to selected messages.
       @param messages pointer to an array holding the message numbers
       @param parent window for dialog
       @param profile pointer for environment
   */
   virtual void ReplyMessages(const INTARRAY *messages,
                      MWindow *parent = NULL,
                      ProfileBase *profile = NULL) = 0;

   /** Forward selected messages.
       @param messages pointer to an array holding the message numbers
       @param parent window for dialog
       @param profile pointer for environment
   */
   virtual void ForwardMessages(const INTARRAY *messages,
                                MWindow *parent = NULL,
                                ProfileBase *profile = NULL) = 0;

   //@}   
   //@}
   /**@name Synchronous Access Functions */
   //@{
   /** Get name of mailbox.
       @return the symbolic name of the mailbox
   */
   virtual String GetName(void) const = 0;
   /** Get number of messages which have a message status of value
       when combined with the mask. When mask = 0, return total
       message count.
       @param mask is a (combination of) MessageStatus value(s) or 0 to test against
       @param value the value of MessageStatus AND mask to be counted
       @return number of messages
   */
   virtual unsigned long CountMessages(int mask = 0, int value = 0) const = 0;
   /** Returns a HeaderInfo structure for a message with a given
       sequence number. This can be used to obtain the uid.
       @param msgno message sequence number, starting from 0
       @return a pointer to the messages current header info entry
   */
   virtual const class HeaderInfo *GetHeaderInfo(unsigned long msgno) const = 0;
   /** Get the profile.
       @return Pointer to the profile.
   */
   virtual inline ProfileBase *GetProfile(void) const = 0;

   /// Get update interval in seconds
   virtual int GetUpdateInterval(void) const = 0;

   /** Toggle sending of new mail events.
       @param send if true, send them
       @param update if true, update internal message count
   */
   virtual void EnableNewMailEvents(bool send = true, bool update = true) = 0;
   /** Query whether folder is sending new mail events.
       @return if true, folder sends them
   */
   virtual bool SendsNewMailEvents(void) const = 0;
   /**@name Functions to get an overview of messages in the folder. */
   //@{
   /// Return a pointer to the first message's header info.
   virtual const class HeaderInfo *GetFirstHeaderInfo(void) const = 0;
   /// Return a pointer to the next message's header info.
   virtual const class HeaderInfo *GetNextHeaderInfo(const class HeaderInfo*) const = 0;
   //@}
   /// Return the folder's type.
   virtual FolderType GetType(void) const = 0;
   /** Sets a maximum number of messages to retrieve from server.
       @param nmax maximum number of messages to retrieve, 0 for no limit
   */
   virtual void SetRetrievalLimit(unsigned long nmax) = 0;
   /// Set update interval in seconds, 0 to disable
   virtual void SetUpdateInterval(int secs) = 0;
   //@}
};

#endif
