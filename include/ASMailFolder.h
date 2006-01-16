/*-*- c++ -*-********************************************************
 * ASMailFolder class: ABC defining the interface for asynchronous  *
 *                     mail folders                                 *
 *                                                                  *
 * (C) 1999-2000 by Karsten Ballüder (balluder@gmx.net)             *
 *                                                                  *
 * $Id$
 *******************************************************************/

/**
   @package Mailfolder access
   @author  Karsten Ballüder
*/
#ifndef ASMAILFOLDER_H
#define ASMAILFOLDER_H

#ifdef __GNUG__
#   pragma interface "ASMailFolder.h"
#endif


// forward declarations
class FolderView;
class Profile;
class MailFolder;
class wxWindow;

#include "MailFolder.h"
#include "Message.h"

class ASMailFolderResult;
class ASMailFolderResultImpl;
class ASMailFolderResultInt;
class ASMailFolderResultUIdType;
class ASMailFolderResultMessage;
class ASMailFolderResultFolderExists;

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
   //@{
   // the enum values are the same as cclient LATT_XXX constants
   enum
   {
      ATT_NOINFERIORS = 1,
      ATT_NOSELECT = 2,
      ATT_MARKED = 4,
      ATT_UNMARKED = 8,
      ATT_REFERRAL = 16
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
      Op_SaveMessagesToFile,
      Op_ReplyMessages,
      Op_ForwardMessages,
      Op_Subscribe,
      Op_ListFolders,
      Op_SearchMessages,
      Op_ApplyFilterRules,
      Op_MarkRead,
      Op_MarkUnread
   };

   typedef ASMailFolderResult Result;
   typedef ASMailFolderResultImpl ResultImpl;
   typedef ASMailFolderResultInt ResultInt;
   typedef ASMailFolderResultUIdType ResultUIdType;
   typedef ASMailFolderResultMessage ResultMessage;
   typedef ASMailFolderResultFolderExists ResultFolderExists;
   
   //@}


   static ASMailFolder * Create(MailFolder *mf);

   /** Open a folder
    */
   static
   ASMailFolder *OpenFolder(const MFolder *mfolder,
                            MailFolder::OpenMode openmode = MailFolder::Normal)
      {
         MailFolder *mf = MailFolder::OpenFolder(mfolder, openmode);
         if ( !mf )
            return NULL;

         ASMailFolder *asmf = Create(mf);
         mf->DecRef();
         return asmf;
      }

   /// and a function to create a half opened folder
   static ASMailFolder *HalfOpenFolder(const MFolder *mfolder)
      { return OpenFolder(mfolder, MailFolder::HalfOpen); }

   /** Used to obtain the next Ticked id for events.
       Only for use by ASMailFolder.cpp and MailFolderCC.cpp internally.
   */
   static Ticket GetTicket(void);

   /**@name Asynchronous Access Functions, returning results in events.*/
   //@{
   /** Check whether mailbox has changed.
       @return void, but causes update events to be sent if necessary.
   */
   virtual void Ping(void) = 0;
   /** Delete a message.
       @param uid the message uid
   */
   virtual Ticket DeleteMessage(unsigned long uid) = 0;

   /** UnDelete a message.
       @param uid the message uid
   */
   virtual Ticket UnDeleteMessage(unsigned long uid) = 0;

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

   /** Set flag for all messages
    */
   virtual Ticket SetFlagForAll(int flag, bool set = true) = 0;

   /** Set flags on a sequence of messages. Possible flag values are MSG_STAT_xxx
       @param sequence the IMAP sequence of uids
       @param flag flag to be set, e.g. "\\Deleted"
       @param set if true, set the flag, if false, clear it
   */
   virtual Ticket SetSequenceFlag(MailFolder::SequenceKind kind,
                                  const Sequence& sequence,
                                  int flag,
                                  bool set = true) = 0;

   /** Appends the message to this folder.
       @param msg the message to append
       @return ResultInt with boolean success value
   */
   virtual Ticket AppendMessage(const Message *msg, UserData ud = 0) = 0;

   /** Appends the message to this folder.
       @param msg text of the  message to append
       @return ResultInt with boolean success value
   */
   virtual Ticket AppendMessage(const String &msg, UserData ud = 0) = 0;

   /** Expunge messages.
     */
   virtual Ticket ExpungeMessages(void) = 0;

   /** Mark messages read/unread.
       @param selections the message indices which will be converted using the current listing
       @param read true if messages must be marked read
     */
   virtual Ticket MarkRead(const UIdArray *selections,
                           UserData ud,
                           bool read) = 0;

   /** Search Messages for certain criteria.
       @return UIdArray with UIds of matching messages
   */
   virtual Ticket SearchMessages(const SearchCriterium *crit, UserData ud) = 0;

   /**@name Some higher level functionality implemented by the
      MailFolder class on top of the other functions.
      These functions are not used by anything else in the MailFolder
      class and can easily be removed if needed.
   */
   //@{
   /** Save the messages to a folder.
       @param selections the message indices which will be converted using the current listing
       @param folderName the name of the folder to save to
       @return ResultInt boolean
   */
   virtual Ticket SaveMessages(const UIdArray *selections,
                               String const & folderName,
                               UserData ud = 0) = 0;

   /** Save the messages to a file.
       @param selections the message indices which will be converted using the current listing
       @param fileName the name of the folder to save to
       @return ResultInt boolean
   */
   virtual Ticket SaveMessagesToFile(const UIdArray *selections,
                                     String const & folderName,
                                     UserData ud = 0) = 0;

   /** Mark messages as deleted or move them to trash.
       @param messages pointer to an array holding the message numbers
       @return ResultInt boolean
   */
   virtual Ticket DeleteOrTrashMessages(const UIdArray *messages,
                                        int flags = MailFolder::DELETE_ALLOW_TRASH,
                                        UserData ud = 0) = 0;

   /** Mark messages as deleted.
       @param messages pointer to an array holding the message numbers
       @param expunge expunge deleted messages
       @return ResultInt boolean
   */
   virtual Ticket DeleteMessages(const UIdArray *messages,
                                 int flags = MailFolder::DELETE_NO_EXPUNGE,
                                 UserData ud = 0) = 0;

   /** Mark messages as no longer deleted.
       @param messages pointer to an array holding the message numbers
       @return ResultInt boolean
   */
   virtual Ticket UnDeleteMessages(const UIdArray *messages, UserData ud = 0) = 0;

   /** Save messages to a file.
       @param messages pointer to an array holding the message numbers
       @parent parent window for dialog
       @return ResultInt boolean
   */
   virtual Ticket SaveMessagesToFile(const UIdArray *messages,
                                     wxWindow *parent = NULL,
                                     UserData ud = 0) = 0;

   /** Save messages to a folder.
       @param messages pointer to an array holding the message numbers
       @param parent window for dialog
       @param folder is the folder to save to, ask the user if NULL
       @return true if messages got saved
   */
   virtual Ticket SaveMessagesToFolder(const UIdArray *messages,
                                       wxWindow *parent = NULL,
                                       MFolder *folder = NULL,
                                       UserData ud = 0) = 0;

   /** Reply to selected messages.
       @param messages pointer to an array holding the message numbers
       @param params reply parameters
       @param parent window for dialog
   */
   virtual Ticket ReplyMessages(const UIdArray *messages,
                                const MailFolder::Params& params,
                                wxWindow *parent = NULL,
                                UserData ud = 0) = 0;

   /** Forward selected messages.
       @param messages pointer to an array holding the message numbers
       @param parent window for dialog
   */
   virtual Ticket ForwardMessages(const UIdArray *messages,
                                  const MailFolder::Params& params,
                                  wxWindow *parent = NULL,
                                  UserData ud = 0) = 0;

   /** Apply filter rules to the folder.
       Applies the rule to all messages listed in msgs.
       @return a ResultInt object
   */
   virtual Ticket ApplyFilterRules(const UIdArray *msgs,
                                   UserData ud = 0) = 0;

   /**@name Subscription management.
      These functions are statically defined and are implemented in
      ASMailFolder.cpp.
   */
   //@{
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
                           bool subscribe = true,
                           UserData ud = 0);
   /** Get a listing of all mailboxes.
       @param pattern a wildcard matching the folders to list
       @param subscribed_only if true, only the subscribed ones
       @param reference the path to start from
    */
   Ticket ListFolders(const String &pattern = _T("*"),
                      bool subscribed_only = false,
                      const String &reference = wxEmptyString,
                      UserData ud = 0);

   //@}
   //@}

   /** @name Synchronous Access Functions

       FIXME: I wonder what are they doing here?
    */
   //@{
   /**
      Returns the delimiter used to separate the components of the folder
      name

      @return character dependind on the folder type and server
    */
   char GetFolderDelimiter() const;

   /** Get name of mailbox.
       @return the symbolic name of the mailbox
   */
   virtual String GetName(void) const = 0;

   /** Get the profile.
       @return Pointer to the profile.
   */
   virtual Profile *GetProfile(void) const = 0;

   /** Returns a listing of the folder. Must be DecRef'd by caller. */
   virtual class HeaderInfoList *GetHeaders(void) const = 0;

   /// Return the folder's type.
   virtual MFolderType GetType(void) const = 0;
   /// Returns the underlying MailFolder object and increfs it.
   virtual MailFolder *GetMailFolder(void) const = 0;
   //@}

   /**@name Function for access control and event handling. */
   //@{
   /// Returns true if we have obtained the lock.
   virtual bool LockFolder(void) = 0;
   virtual void UnLockFolder(void) = 0;
   //@}

};

 /** A structure containing the return values from an operation.
     This will get passed in an MEvent to notify other parts of the
     application that an operation has finished.
 */
class ASMailFolderResult : public MObjectRC
{
public:
   /// Returns the user data.
   virtual UserData GetUserData(void) const = 0;
   /** Returns a pointer to the ASMailFolder from which this
       event originated.
   */
   /// Returns the ticket for this operation.
   virtual Ticket GetTicket(void) const = 0;
   /// Returns the ASMailFolder issuing this result.
   virtual ASMailFolder *GetFolder(void) const = 0;
   /// Returns an ASMailFolder::OperationId to tell what happened.
   virtual ASMailFolder::OperationId GetOperation(void) const = 0;
   /// Returns the list of message uids affected by the operation.
   virtual UIdArray * GetSequence(void) const = 0;
};
   
/** Common code shared by all class Result implementations. */
class ASMailFolderResultImpl : public ASMailFolder::Result
{
public:
   virtual UserData GetUserData(void) const { return m_UserData; }
   virtual Ticket GetTicket(void) const { return m_Ticket; }
   virtual ASMailFolder::OperationId GetOperation(void) const { return m_Id; }
   virtual ASMailFolder *GetFolder(void) const { return m_Mf; }
   virtual UIdArray * GetSequence(void) const { return m_Seq; }
protected:
   ASMailFolderResultImpl(ASMailFolder *mf,
              Ticket t,
              ASMailFolder::OperationId id,
              UIdArray * mc,
              UserData ud)
      {
         m_Id = id;
         m_Ticket = t;
         m_UserData = ud;
         m_Seq = mc;

         m_Mf = mf;
         if ( m_Mf )
            m_Mf->IncRef();
      }

   virtual ~ASMailFolderResultImpl();

private:
   ASMailFolder::OperationId   m_Id;
   ASMailFolder *m_Mf;
   Ticket        m_Ticket;
   UserData      m_UserData;
   UIdArray *    m_Seq;

   MOBJECT_DEBUG(ASMailFolder::ResultImpl)
};
/** Holds the result from an operation which can be expressed as an
    integer value. Used for all boolean success values.
*/
class ASMailFolderResultInt : public ASMailFolderResultImpl
{
public:
   int GetValue(void) const { return m_Value; }
   static ASMailFolder::ResultInt *Create(ASMailFolder *mf,
                            Ticket t,
                            ASMailFolder::OperationId id,
                            UIdArray * mc,
                            int value,
                            UserData ud)
      { return new ASMailFolder::ResultInt(mf, t, id, mc, value, ud); }
protected:
   ASMailFolderResultInt(ASMailFolder *mf,
             Ticket t,
             ASMailFolder::OperationId id,
             UIdArray * mc,
             int value,
             UserData ud)
      : ASMailFolderResultImpl(mf, t, id, mc, ud)
      { m_Value = value; }
private:
   int         m_Value;
};
/** Holds the result from an operation which can be expressed as an
    UIdType value. Used for all boolean success values.
*/
class ASMailFolderResultUIdType : public ASMailFolderResultImpl
{
public:
   UIdType GetValue(void) const { return m_Value; }
   static ASMailFolder::ResultUIdType *Create(ASMailFolder *mf,
                                Ticket t,
                                ASMailFolder::OperationId id,
                                UIdArray * mc,
                                UIdType value,
                                UserData ud)
      { return new ASMailFolder::ResultUIdType(mf, t, id, mc, value, ud); }
protected:
   ASMailFolderResultUIdType(ASMailFolder *mf,
                 Ticket t,
                 ASMailFolder::OperationId id,
                 UIdArray * mc,
                 int value,
                 UserData ud)
      : ASMailFolderResultImpl(mf, t, id, mc, ud)
      { m_Value = value; }
private:
   UIdType         m_Value;
};
/** Holds the result from a GetMessage() and returns the message pointer.
*/
class ASMailFolderResultMessage : public ASMailFolderResultImpl
{
public:
   static ASMailFolder::ResultMessage *Create(ASMailFolder *mf,
                                Ticket t,
                                UIdArray * mc,
                                Message *msg,
                                UIdType uid,
                                UserData ud)
      { return new ASMailFolder::ResultMessage(mf, t, mc, msg, uid, ud); }
   Message * GetMessage(void) const { return m_Message; }
   unsigned long GetUId(void) const { return m_uid; }
protected:
   ASMailFolderResultMessage(ASMailFolder *mf, Ticket t,
                 UIdArray * mc, Message *msg,
                 UIdType uid, UserData ud)
      : ASMailFolderResultImpl(mf, t, ASMailFolder::Op_GetMessage, mc, ud)
      {
         m_Message = msg;
         if(m_Message) m_Message->IncRef();
         m_uid =  uid;
      }
   ~ASMailFolderResultMessage() { if(m_Message) m_Message->DecRef(); }
private:
   Message *m_Message;
   UIdType  m_uid;
};

/**
  Holds either a single folder name returned by ListFolders() call or indicates
  that no more folders are available.
*/
class ASMailFolderResultFolderExists : public ASMailFolderResultImpl
{
public:
   static ASMailFolder::ResultFolderExists *
   Create(ASMailFolder *mf,
          Ticket t,
          const String &name,
          char delimiter,
          long attrib,
          UserData ud)
   {
      return new
         ASMailFolderResultFolderExists(mf, t, name, delimiter, attrib, ud);
   }

   static ASMailFolder::ResultFolderExists *
   CreateNoMore(ASMailFolder *mf, Ticket t, UserData ud)
   {
      return new ASMailFolderResultFolderExists(mf, t, ud);
   }

   String GetName() const { return m_Name; }
   char GetDelimiter() const { return m_Delim; }
   long GetAttributes() const { return m_Attrib; }

   bool NoMore() const { return m_NoMore; }

protected:
   // ctor for "no more folders" result
   ASMailFolderResultFolderExists(ASMailFolder *mf,
                                  Ticket t,
                                  UserData ud)
      : ASMailFolderResultImpl(mf, t, ASMailFolder::Op_ListFolders, NULL, ud)
   {
      m_NoMore = true;
   }

   // ctor for "folder available" result
   ASMailFolderResultFolderExists(ASMailFolder *mf,
                                  Ticket t,
                                  const String& name,
                                  char delimiter,
                                  long attrib,
                                  UserData ud)
      : ASMailFolderResultImpl(mf, t, ASMailFolder::Op_ListFolders, NULL, ud)
   {
      m_Name = name;
      m_Delim = delimiter;
      m_Attrib = attrib;
      m_NoMore = false;
   }

private:
   String m_Name;
   long m_Attrib;
   char m_Delim;
   bool m_NoMore;
};

/** A useful helper class to keep tickets for us. */
class ASTicketList : public MObjectRC
{
public:
   virtual bool Contains(Ticket t) const = 0;
   virtual void Add(Ticket t) = 0;
   virtual void Remove(Ticket t) = 0;
   virtual void Clear(void) = 0;
   virtual bool IsEmpty(void) const = 0;

   /// return the last ticket in the list and remove it from list
   virtual Ticket Pop(void) = 0;

   static ASTicketList * Create(void);
};

#endif // ASMAILFOLDER_H

