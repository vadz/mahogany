/*-*- c++ -*-********************************************************
 * MailFolder class: ABC defining the interface to mail folders     *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *******************************************************************/

%nodefault;

%module MailFolder

%{
#include   "Mswig.h"
#include   "Mcommon.h"
#include   "MailFolder.h"
#include   "Profile.h"
#include   "HeaderInfo.h"
#include   "UIdArray.h"
#include   "Sequence.h"
%}

%import MString.i
%import MProfile.i
%import Message.i

// forward declarations
class FolderView;
class Profile;
class wxWindow;
class Message;


/**
   MailFolder base class, represents anything containing mails.

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
   /** What is the status of a given message in the folder?
       Recent messages are those that we never saw in a listing
       before. Once we open a folder, the messages will no longer be
       recent when we close it. Seen are only those that we really
       looked at. */
   enum MessageStatus
   {
      /// message is new
      MSG_STAT_NEW = 0,
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

   /**
     Different ways to reply to a message

     NB: don't change the numeric values of the enum elements, they are
         stored in the profile and also because wxOptionsDlg relies on
         them being what they are!
    */
   enum ReplyKind
   {
      /// reply to the sender of this message only
      REPLY_SENDER,

      /// reply to all recipients of this message
      REPLY_ALL,

      /// reply to the mailing list this message was sent to
      REPLY_LIST,

      /// follow-up to newsgroup
      FOLLOWUP_TO_NEWSGROUP,

      /// default reply: may go to the sender, all or list
      REPLY
   };
   //@}

   /** @name Static functions, implemented in MailFolder.cpp */
   //@{

   /** The same OpenFolder function, but taking all arguments from a
       MFolder object. */
   static MailFolder * OpenFolder(const class MFolder *mfolder);

   /** Physically deletes this folder.
       @return true on success
   */
   static bool DeleteFolder(const MFolder *mfolder);
   /**
         Creates a mailbox profile and checks the settings to be
         sensible.
         @param name name of new folder profile
         @param type type of folder
         @param flags folder flags
         @param optional path for folder
         @param comment optional comment for the folder
         @return false on error or true on success
   */

   static bool CanExit(String *which);

   /** Get a listing of all mailboxes.

       DO NOT USE THIS FUNCTION, BUT ASMailFolder::ListFolders instead!!!

       @param asmf the ASMailFolder initiating the request
       @param pattern a wildcard matching the folders to list
       @param subscribed_only if true, only the subscribed ones
       @param reference implementation dependend reference
    */
   virtual void ListFolders(class ASMailFolder *asmf,
                            const String &pattern,
                            bool subscribed_only,
                            const String &reference,
                            UserData ud,
                            Ticket ticket);
   //@}
   //@}

   /** Get name of mailbox.
       @return the symbolic name of the mailbox
   */
   virtual String GetName(void);

   /// Count number of new messages.
   virtual unsigned long CountNewMessages(void);

   /// Count number of recent messages.
   virtual unsigned long CountRecentMessages(void);

   /** Check whether mailbox has changed. */
   virtual void Ping(void);

   /** get the message with unique id uid
       @param uid message uid
       @return message handler
   */
   virtual Message *GetMessage(unsigned long uid);

   /** Delete a message, really delete, not move to trash. UNSUPPORTED!
       @param uid the message uid
       @return true on success
   */
   virtual bool DeleteMessage(unsigned long uid);

   /** UnDelete a message.  UNSUPPORTED!
       @param uid the message uid
       @return true on success
   */
   virtual bool UnDeleteMessage(unsigned long uid);

   /** Set flags on a messages. Possible flag values are MSG_STAT_xxx
       @param uid the message uid
       @param flag flag to be set, e.g. "\\Deleted"
       @param set if true, set the flag, if false, clear it
       @return true on success
   */
   virtual bool SetMessageFlag(unsigned long uid,
                               int flag,
                               bool set = true);

   /** Set flags on a sequence of messages. Possible flag values are MSG_STAT_xxx
       @param sequence the IMAP sequence of uids
       @param flag flag to be set, e.g. "\\Deleted"
       @param set if true, set the flag, if false, clear it
       @return true on success
   */
   virtual bool SetFlag(const UIdArray *sequence,
                        int flag,
                        bool set = true);

   virtual bool AppendMessage(const Message &msg);

   /** Expunge messages.
     */
   virtual void ExpungeMessages(void);

   /** Search Messages for certain criteria.
       @return UIdArray with UIds of matching messages, caller must
       free it
   */
   virtual UIdArray *SearchMessages(const class SearchCriterium *crit);

   /** Get the profile.
       @return Pointer to the profile.
   */
   virtual inline Profile *GetProfile(void);

   /// return class name
   const char *GetClassName(void) { return "MailFolder"; }

   /**@name Some higher level functionality implemented by the
      MailFolder class on top of the other functions.
      These functions are not used by anything else in the MailFolder
      class and can easily be removed if needed.
   */
   //@{
   /** Save the messages to a folder.
       @param selections the message indices which will be converted using the current listing
       @param folderName the name of the folder to save to
       @return true on success
   */
   virtual bool SaveMessages(const UIdArray *selections,
                             const String & folderName);

   /** Save the messages to a folder.
       @param selections the message indices which will be converted using the current listing
       @param fileName the name of the folder to save to
       @return true on success
   */
   virtual bool SaveMessagesToFile(const UIdArray *selections,
                                   const String& fileName,
                                   wxWindow *parent);

   /** Mark messages as deleted or move them to trash.
       @param messages pointer to an array holding the message numbers
       @return true on success
   */
   virtual bool DeleteOrTrashMessages(const UIdArray *messages);

   /** Mark messages as deleted.
       @param messages pointer to an array holding the message numbers
       @param expunge expunge deleted messages
       @return true on success
   */
   virtual bool DeleteMessages(const UIdArray *messages,
                               bool expunge);

   /** Mark messages as no longer deleted.
       @param messages pointer to an array holding the message numbers
       @return true on success
   */
   virtual bool UnDeleteMessages(const UIdArray *messages);

   /** Reply to selected messages.
       @param messages pointer to an array holding the message numbers
       @param params is the Params struct to use
       @param parent window for dialog
   */
   virtual void ReplyMessages(const UIdArray *messages,
                              const MailFolder::Params& params,
                              wxWindow *parent);

   /** Forward selected messages.
       @param messages pointer to an array holding the message numbers
       @param params is the Params struct to use
       @param parent window for dialog
   */
   virtual void ForwardMessages(const UIdArray *messages,
                                const MailFolder::Params& params,
                                wxWindow *parent);

   //@}

   /**@name Access control */
   //@{
   /** Locks a mailfolder for exclusive access. In multi-threaded mode
       it really locks it and always returns true. In single-threaded
       mode it returns false if we cannot get a lock.
       @return TRUE if we have obtained the lock
   */
   virtual bool Lock(void);
   /** Releases the lock on the mailfolder. */
   virtual void UnLock(void);
   /// Is folder locked?
   virtual bool IsLocked(void);
   //@}
   /**@name Functions to get an overview of messages in the folder. */
   //@{
   /** Returns a listing of the folder. Must be DecRef'd by caller. */
   virtual class HeaderInfoList *GetHeaders(void);
   //@}
   /// Return the folder's type.
   virtual MFolderType GetType(void);
   /// return the folder flags
   virtual int GetFlags(void);

   /** Apply any filter rules to the folder.
       Applies the rule to all messages listed in msgs.
       @return -1 if no filter module exists, return code otherwise
   */
   virtual int ApplyFilterRules(UIdArray msgs);
};

/** This class essentially maps to the c-client Overview structure,
    which holds information for showing lists of messages.

    IMPORTANT: When sorting messages, the instances of this class will
    be copied around in memory bytewise, but not duplicates of the
    object will be created. So the reference counting in wxString
    objects should be compatible with this, as at any time only one
    object exists.
*/
class HeaderInfo
{
public:
   virtual const String &GetSubject(void);
   virtual const String &GetFrom(void);
   virtual time_t GetDate(void);
   virtual const String &GetId(void);
   virtual const String &GetReferences(void);
   virtual UIdType GetUId(void);
   virtual int GetStatus(void);
   virtual unsigned long GetSize(void);
   virtual unsigned long GetLines(void);
private:
   /// Disallow copy construction
   HeaderInfo(const HeaderInfo &);
};

/** This class holds a complete list of all messages in the folder. */
class HeaderInfoList : public MObjectRC
{
public:
   /// Count the number of messages in listing.
   virtual size_t Count(void);
   /// Get the item displayed at given position
   virtual HeaderInfo *GetItem(size_t n);
};


