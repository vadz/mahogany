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

// forward declarations
class FolderView;
class ProfileBase;

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

   // currently we need to Close() and DecRef() it, that's not
   // nice. We need to change that.
   //@}

   /** Register a FolderViewBase derived class to be notified when
       folder contents change.
       @param  view the FolderView to register
       @param reg if false, unregister it
   */
   virtual void RegisterView(FolderView *view, bool reg = true) = 0;

   /** get name of mailbox
       @return the symbolic name of the mailbox
   */
   virtual String GetName(void) const = 0;

   /** get number of messages
       @return number of messages
   */
   virtual long CountMessages(void) const = 0;

   /** Check whether mailbox has changed. */
   virtual void Ping(void) = 0;
   
   /** get the message with number msgno
       @param msgno sequence number
       @return message handler
   */
   virtual class Message *GetMessage(unsigned long msgno) = 0;

   /** Delete a message.
       @param index the sequence number
   */
   virtual void DeleteMessage(unsigned long index) = 0;

   /** UnDelete a message.
       @param index the sequence number
   */
   virtual void UnDeleteMessage(unsigned long index) = 0;

   /** Set flags on a messages. Possible flag values are MSG_STAT_xxx
       @param sequence number of the message
       @param flag flag to be set, e.g. "\\Deleted"
       @param set if true, set the flag, if false, clear it
   */
   virtual void SetMessageFlag(unsigned long msgno,
                               int flag,
                               bool set = true) = 0;

   /** Set flags on a sequence of messages. Possible flag values are MSG_STAT_xxx
       @param sequence the IMAP sequence
       @param flag flag to be set, e.g. "\\Deleted"
       @param set if true, set the flag, if false, clear it
   */
   virtual void SetSequenceFlag(const String &sequence,
                                int flag,
                                bool set = true) = 0;
   /** Appends the message to this folder.
       @param msg the message to append
   */
   virtual void AppendMessage(const Message &msg) = 0;

   /** Appends the message to this folder.
       @param msg text of the  message to append
   */
   virtual void AppendMessage(const String &msg) = 0;

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
   int GetUpdateInterval(void) const { return m_UpdateInterval; }

   /** Utiltiy function to get a textual representation of a message
       status.
       @param message status
       @return string representation
   */
   static String ConvertMessageStatusToString(int status);

   /** Toggle sending of new mail events.
       @param send if true, send them
   */
   virtual void EnableNewMailEvents(bool send = true) = 0;
   /** Query whether foldre is sending new mail events.
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

protected:
   /**@name Accessor methods */
   //@{
   /// Set update interval in seconds
   // not yet inline void SetUpdateInterval(void) = 0;
   /// Get authorisation information
   inline void GetAuthInfo(String *login, String *password) const
      { *login = m_Login; *password = m_Password; }
   //@}
   /**@name Common variables might or might not be used */
   //@{
   /// Update interval for checking folder content
   int m_UpdateInterval;
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
   virtual int GetStatus(void) const = 0;
   virtual unsigned long const &GetSize(void) const = 0;
   virtual ~HeaderInfo() {}
};


#endif
