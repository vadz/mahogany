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

// forward declarations
class FolderViewBase;
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
   <ul>
*/
class MailFolder : public MObjectRC
{
public:   
   /** @name Constants and Types */
   //@{
   /** Which type is this mailfolder? (consistent with c-client?)
       Only the lower byte is a type, the upper one is for additional
       flags.
   */
   enum Type
   {
      /// system inbox
      MF_INBOX = 0,
      /// mbox file
      MF_FILE = 1,
      /// pop3
      MF_POP = 2,
      /// imap
      MF_IMAP = 3,
      /// newsgroup
      MF_NNTP = 4,
      /// read type etc from profile
      MF_PROFILE = 5,
      /// use this with AND to obtain pure type
      MF_TYPEMASK = 255,
      /// from here on it's flags
      MF_FLAGS = 256
   };

   /// what's the status of a given message in the folder?
   enum Status
   {
      /// message is unread
      MSG_STAT_UNREAD = 1,
      /// message is deleted
      MSG_STAT_DELETED = 2,
      /// message has been replied to
      MSG_STAT_REPLIED = 4,
      /// message is "recent" (what's this?)
      MSG_STAT_RECENT = 8,
      /// message matched a search
      MSG_STAT_SEARCHED = 16
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
      <li>MF_POP:   hostname
      <li>MF_IMAP:  hostname
      <li>MF_NNTP:  newshost
      <ul>
      @param type one of the supported types
      @param path either a hostname or filename depending on type
      @param profile parent profile
      @param login only used for POP,IMAP and NNTP (as the newsgroup name)
      @param password only used for POP, IMAP
      
   */
   static MailFolder * OpenFolder(MailFolder::Type type,
                                  String const &path,
                                  ProfileBase *profile = NULL,
                                  String const &login = NULLstring,
                                  String const &password = NULLstring);

   // currently we need to Close() and DecRef() it, that's not
   // nice. We need to change that.
   //@}

   /// check wether object is initialised
   virtual bool  IsInitialised(void) const = 0;

   /** Register a FolderViewBase derived class to be notified when
       folder contents change.
       @param  view the FolderView to register
       @param reg if false, unregister it
   */
   virtual void RegisterView(FolderViewBase *view, bool reg = true) = 0;
   
   /** is mailbox "ok", i.e. was there an error or not?
       @return  true if everything succeeded
   */
   virtual bool IsOk(void) const = 0;

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
   
   /** Get status of message.
       @param  msgno sequence no of message
       @param size if not NULL, size in bytes gets stored here
       @param day to store day (1..31)
       @param month to store month (1..12)
       @param year to store year (19xx)
       @return flags of message
   */
   virtual int GetMessageStatus(
      unsigned int msgno,
      unsigned long *size = NULL,
      unsigned int *day = NULL,
      unsigned int *month = NULL,
      unsigned int *year = NULL) = 0;
   
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

   /** Set a message flag. Possible flag values are MSG_STAT_xxx
       @param index the sequence number
       @param flag flag to be set, e.g. "\\Deleted"
       @param set if true, set the flag, if false, clear it
   */
   virtual void SetMessageFlag(unsigned long index, int flag, bool set =
                true) = 0;

   /** Appends the message to this folder.
       @param msg the message to append
   */
   virtual void AppendMessage(Message const &msg) = 0;

   /** Appends the message to this folder.
       @param msg text of the  message to append
   */
   virtual void AppendMessage(String const &msg) = 0;

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
   inline int GetUpdateInterval(void) const { return m_UpdateInterval; }
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
#endif
