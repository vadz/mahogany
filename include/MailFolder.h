/*-*- c++ -*-********************************************************
 * MailFolder class: ABC defining the interface to mail folders     *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *******************************************************************/



#ifndef MAILFOLDER_H
#define MAILFOLDER_H

#ifdef __GNUG__
#   pragma interface "MailFolder.h"
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
class Message;

/**
   MailFolder base class, represents anything containig mails.

   This class defines the interface for all MailFolder classes. It can
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
class MailFolder : public MObjectRC
{
public:
   /** @name Constants and Types */
   //@{
   // compatibility
   typedef FolderType Type;

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

   //@}

   /** @name Constructors and destructor */
   //@{

   /**
      Opens an existing mail folder of a certain type.
      The path argument is as follows:
      <ul>
      <li>MF_INBOX: unused
      <li>MF_FILE:  filename, either relative to MP_MBOXDIR (global
                    profile) or absolute
      <li>MF_POP:   unused
      <li>MF_IMAP:  folder path
      <li>MF_NNTP:  newgroup
      </ul>
      @param type one of the supported types
      @param path either a hostname or filename depending on type
      @param profile parent profile
      @param server hostname
      @param login only used for POP,IMAP and NNTP (as the newsgroup name)
      @param password only used for POP, IMAP

   */
   static MailFolder * OpenFolder(int typeAndFlags,
                                  const String &path,
                                  ProfileBase *profile = NULL,
                                  const String &server = NULLstring,
                                  const String &login = NULLstring,
                                  const String &password = NULLstring);

   //@}

   /** Register a FolderViewBase derived class to be notified when
       folder contents change.
       @param  view the FolderView to register
       @param reg if false, unregister it
   */
   virtual void RegisterView(FolderView *view, bool reg = true) = 0;

   /** Get name of mailbox.
       @return the symbolic name of the mailbox
   */
   virtual String GetName(void) const = 0;

   /** Sets the symbolic name.
    */
   virtual void SetName(const String &name) = 0;
   
   /** Get number of messages which have a message status of value
       when combined with the mask. When mask = 0, return total
       message count.
       @param mask is a (combination of) MessageStatus value(s) or 0 to test against
       @param value the value of MessageStatus AND mask to be counted
       @return number of messages
   */
   virtual unsigned long CountMessages(int mask = 0, int value = 0) const = 0;

   /** Check whether mailbox has changed. */
   virtual void Ping(void) = 0;

   /** Returns a HeaderInfo structure for a message with a given
       sequence number. This can be used to obtain the uid.
       @param msgno message sequence number, starting from 0
       @return a pointer to the messages current header info entry
   */
   virtual const class HeaderInfo *GetHeaderInfo(unsigned long msgno) const = 0;

   /** get the message with unique id uid
       @param uid message uid
       @return message handler
   */
   virtual Message *GetMessage(unsigned long uid) = 0;

   /** Delete a message.
       @param uid the message uid
   */
   virtual void DeleteMessage(unsigned long uid) = 0;

   /** UnDelete a message.
       @param uid the message uid
   */
   virtual void UnDeleteMessage(unsigned long uid) = 0;

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

   /** Get the profile.
       @return Pointer to the profile.
   */
   inline ProfileBase *GetProfile(void) { return m_Profile; }

   /// return class name
   const char *GetClassName(void) const
      { return "MailFolder"; }

   /// Get update interval in seconds
   virtual int GetUpdateInterval(void) const = 0;

   /** Utiltiy function to get a textual representation of a message
       status.
       @param message status
       @return string representation
   */
   static String ConvertMessageStatusToString(int status);

   /** Toggle sending of new mail events.
       @param send if true, send them
       @param update if true, update internal message count
   */
   virtual void EnableNewMailEvents(bool send = true, bool update = true) = 0;
   /** Query whether folder is sending new mail events.
       @return if true, folder sends them
   */
   virtual bool SendsNewMailEvents(void) const = 0;


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
       @return true on success
   */
   bool SaveMessages(const INTARRAY *selections,
                     String const & folderName,
                     bool isProfile,
                     bool updateCount = true);

   /** Mark messages as deleted.
       @param messages pointer to an array holding the message numbers
   */
   void DeleteMessages(const INTARRAY *messages);

   /** Mark messages as no longer deleted.
       @param messages pointer to an array holding the message numbers
   */
   void UnDeleteMessages(const INTARRAY *messages);

   /** Save messages to a file.
       @param messages pointer to an array holding the message numbers
       @parent parent window for dialog
       @return true if messages got saved
   */
   bool SaveMessagesToFile(const INTARRAY *messages, MWindow *parent = NULL);

   /** Save messages to a folder.
       @param messages pointer to an array holding the message numbers
       @param parent window for dialog
       @return true if messages got saved
   */
   bool SaveMessagesToFolder(const INTARRAY *messages, MWindow *parent = NULL);

   /** Reply to selected messages.
       @param messages pointer to an array holding the message numbers
       @param parent window for dialog
       @param profile pointer for environment
   */
   void ReplyMessages(const INTARRAY *messages,
                      MWindow *parent = NULL,
                      ProfileBase *profile = NULL);

   /** Forward selected messages.
       @param messages pointer to an array holding the message numbers
       @param parent window for dialog
       @param profile pointer for environment
   */
   void ForwardMessages(const INTARRAY *messages,
                        MWindow *parent = NULL,
                        ProfileBase *profile = NULL);

   //@}

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

protected:
   /**@name Accessor methods */
   //@{
   /// Set update interval in seconds, 0 to disable
   virtual void SetUpdateInterval(int secs) = 0;
   /// Get authorisation information
   inline void GetAuthInfo(String *login, String *password) const
      { *login = m_Login; *password = m_Password; }
   //@}
   /**@name Common variables might or might not be used */
   //@{
   /// Login for password protected mail boxes.
   String m_Login;
   /// Password for password protected mail boxes.
   String m_Password;
   /// a profile
   ProfileBase *m_Profile;
   //@}
};


/** This class essentially maps to the c-client Overview structure,
    which holds information for showing lists of messages. */
class HeaderInfo
{
public:
   virtual const String &GetSubject(void) const = 0;
   virtual const String &GetFrom(void) const = 0;
   virtual const String &GetDate(void) const = 0;
   virtual const String &GetId(void) const = 0;
   virtual const String &GetReferences(void) const = 0;
   virtual unsigned long GetUId(void) const = 0;
   virtual int GetStatus(void) const = 0;
   virtual unsigned long const &GetSize(void) const = 0;
   virtual ~HeaderInfo() {}
};


#endif
