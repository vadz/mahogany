/*-*- c++ -*-********************************************************
 * MailFolder class: ABC defining the interface to mail folders     *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *******************************************************************/

%module MailFolder

%{
#include   "Mpch.h"
#include   "Mcommon.h"   
#include   "MailFolder.h"
#include   "Profile.h"   
%}

%import MString.i
%import Profile.i
%import Message.i

// forward declarations
class FolderView;

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
      <li>MF_POP:   unused
      <li>MF_IMAP:  folder path
      <li>MF_NNTP:  unused
      <ul>
      @param type one of the supported types
      @param path either a hostname or filename depending on type
      @param profile parent profile
      @param server
      @param login only used for POP,IMAP and NNTP (as the newsgroup name)
      @param password only used for POP, IMAP
      
   */
   static MailFolder * OpenFolder(MailFolder::Type type,
                                  String &path,
                                  ProfileBase *profile = NULL,
                                  String &server = NULLstring,
                                  String &login = NULLstring,
                                  String &password = NULLstring);

   // currently we need to Close() and DecRef() it, that's not
   // nice. We need to change that.
   //@}

   /// check wether object is initialised
   virtual bool  IsInitialised(void);

   /** Register a FolderView derived class to be notified when
       folder contents change.
       @param  view the FolderView to register
       @param reg if false, unregister it
   */
   virtual void RegisterView(FolderView *view, bool reg = true);
   
   /** is mailbox "ok", i.e. was there an error or not?
       @return  true if everything succeeded
   */
   virtual bool IsOk(void);

   /** get name of mailbox
       @return the symbolic name of the mailbox
   */
   virtual String GetName(void);

   /** get number of messages
       @return number of messages
   */
   virtual long CountMessages(void);

   /** Check whether mailbox has changed. */
   virtual void Ping(void);
   
   /** get the message with number msgno
       @param msgno sequence number
       @return message handler
   */
   virtual class Message *GetMessage(unsigned long msgno);

   /** Delete a message.
       @param index the sequence number
   */
   virtual void DeleteMessage(unsigned long index);

   /** UnDelete a message.
       @param index the sequence number
   */
   virtual void UnDeleteMessage(unsigned long index);

   /** Set flags on a sequence of messages. Possible flag values are MSG_STAT_xxx
       @param sequence the IMAP sequence
       @param flag flag to be set, e.g. "\\Deleted"
       @param set if true, set the flag, if false, clear it
   */
   virtual void SetSequenceFlag(String &sequence,
                                int flag,
                                bool set);

   /** Appends the message to this folder.
       @param msg the message to append
   */
   virtual void AppendMessage(Message &msg);

   /** Appends the message to this folder.
       @param msg text of the  message to append
   */
   //FIXME virtual void AppendMessage(String &msg);

   /** Expunge messages.
     */
   virtual void ExpungeMessages(void);
   
   /** Get the profile.
       @return Pointer to the profile.
   */
   inline ProfileBase *GetProfile(void) { return m_Profile; }

   /// return class name
   char *GetClassName(void) const
      { return "MailFolder"; }
   /// Get update interval in seconds
   inline int GetUpdateInterval(void) { return m_UpdateInterval; }
protected:
   /**@name Accessor methods */
   //@{
   /// Set update interval in seconds
   // not yet inline void SetUpdateInterval(void);
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
