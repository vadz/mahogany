/*-*- c++ -*-********************************************************
 * MMailFolder class: Mahogany's own mailfolder formats             *
 *                                                                  *
 * (C) 2000 by Karsten Ballüder (ballueder@gmx.net)                 *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/

/**
   @package Mailfolder access
   @author  Karsten Ballüder
*/

#ifndef MMAILFOLDER_H
#define MMAILFOLDER_H

#ifdef __GNUG__
#   pragma interface "MMailFolder.h"
#endif

#ifndef   USE_PCH
#   include  "kbList.h"
#   include  "MObject.h"
#   include  "guidef.h"
#endif

#include  "MailFolderCmn.h"
#include  "FolderView.h"
#include  "MFolder.h"

/** Base class for Mahogany MailFolders */
class MMailFolderBase : public MailFolderCmn
{
public:
   /** Get name of mailbox.
       @return the symbolic name of the mailbox
   */
   virtual String GetName(void) const { return m_MFolder->GetName(); }

protected:
   /// Constructor
   MMailFolderBase(const MFolder *mfolder)
      : MailFolderCmn()
      {
         m_MFolder = mfolder;
         SafeIncRef((MFolder *)m_MFolder);
         m_Profile = ProfileBase::CreateProfile(mfolder->GetName());
         UpdateConfig();
      }

   /// Destructor
   virtual ~MMailFolderBase()
      {
         SafeDecRef((MFolder *)m_MFolder);
         SafeDecRef(m_Profile);
      }

   /** Get the profile.
       @return Pointer to the profile.
   */
   virtual inline ProfileBase *GetProfile(void)
      { return m_Profile; }
      
private:
   const MFolder *m_MFolder;
   ProfileBase *m_Profile;
};

/**
   Proprietary MailFolder class.
*/
class MMailFolder : public MMailFolderBase
{
public:
   /** Get a MMailFolder object */
   static MailFolder * OpenFolder(const MFolder *mfolder);

   /** Phyically deletes this folder.
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
   static bool CreateFolder(const String &name,
                            FolderType type = MF_MFILE,
                            int flags = MF_FLAGS_DEFAULT,
                            const String &path = "",
                            const String &comment = "");

   /** Checks if it is OK to exit the application now.
       @param which Will either be set to empty or a '\n' delimited
       list of folders which are in critical sections.
   */
   static bool CanExit(String *which);

   /** Utiltiy function to get a textual representation of a message
       status.
       @param message status
       @param mf optional pointer to folder to treat e.g. NNTP separately
       @return string representation
   */
   static String ConvertMessageStatusToString(int status,
                                              MailFolder *mf = NULL);
   /** Forward one message.
       @param message message to forward
       @param profile environment
       @param parent window for dialog
   */
   static void ForwardMessage(class Message *msg,
                              ProfileBase *profile = NULL,
                              MWindow *parent = NULL);
   /** Reply to one message.
       @param message message to reply to
       @param flags 0, or REPLY_FOLLOWUP
       @param profile environment
       @param parent window for dialog
   */
   static void ReplyMessage(class Message *msg,
                            int flags = 0,
                            ProfileBase *profile = NULL,
                            MWindow *parent = NULL);
   /**@name Subscription management */
   //@{
   /** Subscribe to a given mailbox (related to the
       mailfolder/mailstream underlying this folder.
       @param host the server host, or empty for local newsspool
       @param protocol MF_IMAP or MF_NNTP or MF_NEWS
       @param mailboxname name of the mailbox to subscribe to
       @param bool if true, subscribe, else unsubscribe
       @return true on success
   */
   static bool Subscribe(const String &host,
                         FolderType protocol,
                         const String &mailboxname,
                         bool subscribe = true);
   /** Get a listing of all mailboxes.

       DO NOT USE THIS FUNCTION, BUT ASMailFolder::ListFolders instead!!!

       @param asmf the ASMailFolder initiating the request
       @param pattern a wildcard matching the folders to list
       @param subscribed_only if true, only the subscribed ones
       @param reference implementation dependend reference
    */
   virtual void ListFolders(class ASMailFolder *asmf,
                            const String &pattern = "*",
                            bool subscribed_only = false,
                            const String &reference = "",
                            UserData ud = 0,
                            Ticket ticket = ILLEGAL_TICKET);
   //@}
   //@}

   /** Get number of messages which have a message status of value
       when combined with the mask. When mask = 0, return total
       message count.
       @param mask is a (combination of) MessageStatus value(s) or 0 to test against
       @param value the value of MessageStatus AND mask to be counted
       @return number of messages
   */
   virtual unsigned long CountMessages(int mask = 0, int value = 0) const;

   /// Count number of recent messages.
   virtual unsigned long CountRecentMessages(void) const;

   /** Count number of new messages but only if a listing is
       available, returns UID_ILLEGAL otherwise.
   */
   virtual unsigned long CountNewMessagesQuick(void) const;

   /** Check whether mailbox has changed. */
   virtual void Ping(void);

   /** get the message with unique id uid
       @param uid message uid
       @return message handler
   */
   virtual Message *GetMessage(unsigned long uid);

   /** Set flags on a sequence of messages. Possible flag values are MSG_STAT_xxx
       @param sequence the IMAP sequence of uids
       @param flag flag to be set, e.g. "\\Deleted"
       @param set if true, set the flag, if false, clear it
       @return true on success
   */
   virtual bool SetFlag(const UIdArray *sequence,
                        int flag,
                        bool set = true);

   /** Set flags on a sequence of messages. Possible flag values are MSG_STAT_xxx
       @param sequence the IMAP sequence of uids
       @param flag flag to be set, e.g. "\\Deleted"
       @param set if true, set the flag, if false, clear it
       @return true on success
   */
   virtual bool SetSequenceFlag(const String &sequence,
                                int flag,
                                bool set = true);
   /** Appends the message to this folder.
       @param msg the message to append
       @return true on success
   */
   virtual bool AppendMessage(const Message &msg);

   /** Appends the message to this folder.
       @param msg text of the  message to append
       @return true on success
   */
   virtual bool AppendMessage(const String &msg);

   /** Expunge messages.
     */
   virtual void ExpungeMessages(void);

   /** Search Messages for certain criteria.
       @return UIdArray with UIds of matching messages, caller must
       free it
   */
   virtual UIdArray *SearchMessages(const class SearchCriterium *crit);

   /// return class name
   virtual const char *GetClassName(void) const
      { return "MMailFolder"; }

   /**@name Access control */
   //@{
   /** Locks a mailfolder for exclusive access. In multi-threaded mode
       it really locks it and always returns true. In single-threaded
       mode it returns false if we cannot get a lock.
       @return TRUE if we have obtained the lock
   */
   virtual bool Lock(void) const;
   /** Releases the lock on the mailfolder. */
   virtual void UnLock(void) const;
   /// Is folder locked?
   virtual bool IsLocked(void) const;
   //@}
   /**@name Functions to get an overview of messages in the folder. */
   //@{
   /** Returns a listing of the folder. Must be DecRef'd by caller. */
   virtual class HeaderInfoList *GetHeaders(void) const;
   //@}
   /// Return the folder's type.
   virtual FolderType GetType(void) const;
   /// return the folder flags
   virtual int GetFlags(void) const;

   /** Sets a maximum number of messages to retrieve from server.
       @param nmax maximum number of messages to retrieve, 0 for no limit
   */
   virtual void SetRetrievalLimit(unsigned long nmax);
   /// Request update
   virtual void RequestUpdate(void);
protected:
   MMailFolder(const MFolder *mf);
   ~MMailFolder();

   
   /**@name MailFolderCmn API: */
   //@{
   /// Update the folder status, number of messages, etc
   virtual void UpdateStatus(void);

   /** Check if this message is a "New Message" for generating new
       mail event. */
   virtual bool IsNewMessage(const HeaderInfo * hi);
   //@}

   /// MailFolder listing cache
   class MCache * m_Cache;
   /// IO handler for reading/writing messages
   class MsgHandler *m_MsgHandler;
};
#endif
