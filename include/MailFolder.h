/*-*- c++ -*-********************************************************
 * MailFolder class: ABC defining the interface to mail folders     *
 *                                                                  *
 * (C) 1998-2000 by Karsten Ballüder (karsten@phy.hw.ac.uk)         *
 *                                                                  *
 * $Id$
 *******************************************************************/

/**
   @package Mailfolder access
   @version $Revision$
   @author  Karsten Ballüder
*/

#ifndef MAILFOLDER_H
#define MAILFOLDER_H

#ifdef __GNUG__
#   pragma interface "MailFolder.h"
#endif

#include "FolderType.h"
#include "kbList.h"

#include <wx/fontenc.h>

/** The UIdArray define is a class which is an integer array. It needs
    to provide a int Count() method to return the number of elements
    and an int operator[int] to access them.
    We use wxArrayInt for this.
    @deffunc UIdArray
*/
#include <wx/dynarray.h>
WX_DEFINE_ARRAY(UIdType, UIdArray);

/// Long enough for file offsets or pointers
#define FolderDataType unsigned long


// forward declarations
class FolderView;
class Profile;
class MWindow;
class Message;


#define MF_LOGCIRCLE_SIZE   10
/** A small class to hold the last N log messages for analysis.
 */
class MLogCircle
{
public:
   MLogCircle(int n);
   ~MLogCircle();
   void Add(const String &txt);
   bool Find(const String needle, String *store = NULL) const;
   String GetLog(void) const;
   void Clear(void);
   /// Looks at log data and guesses what went wrong.
   String GuessError(void) const;
private:
   int m_N, m_Next;
   String *m_Messages;
};

/**
   MailFolder base class, represents anything containig mails.

   This class defines the interface for all MailFolder classes. It can
   represent anything which contains messages, e.g. folder files, POP3
   or IMAP connections or even newsgroups.


   To open a MailFolder via OpenFolder() one must do either of the
   following:
   <ul>
   <li>New and preferred way which will soon be the only one: pass it
       a pointer to MFolder
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

   /** Flags for several operations. */
   enum Flags
   {
      NONE = 0,
      REPLY_FOLLOWUP
   };

   /** the structure containing the parameters for some mail operations */
   struct Params
   {
      Params(int f = 0) { flags = f; }
      Params(const String& t, int f = 0) : templ(t) { flags = f; }

      int flags;        // see Flags enum above
      String templ;     // the template to use for the new message
   };
   //@}

   /** @name Static functions, implemented in MailFolder.cpp */
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
      @param halfopen to only half open the folder
   */
   static MailFolder * OpenFolder(int typeAndFlags,
                                  const String &path,
                                  Profile *profile = NULL,
                                  const String &server = NULLstring,
                                  const String &login = NULLstring,
                                  const String &password = NULLstring,
                                  const String &symbolicname = NULLstring,
                                  bool halfopen = FALSE);

   /** The same OpenFolder function, but taking all arguments from a
       MFolder object. */
   static MailFolder * OpenFolder(const class MFolder *mfolder);

   /** This opens a mailfolder from either a profile of that name, or,
       if it is an absolute path, from a file of that name.
       Profile parameter is only used when name is a filename.
   */
   static MailFolder * OpenFolder(const String &name,
                                  Profile *profile = NULL);

   /** Half open the folder using paremeters from MFolder object. */
   static MailFolder * HalfOpenFolder(const class MFolder *mfolder,
                                      Profile *profile = NULL);

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
                            FolderType type = MF_FILE,
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
       @param params is the Params struct to use
       @param profile environment
       @param parent window for dialog
   */
   static void ForwardMessage(class Message *msg,
                              const Params& params,
                              Profile *profile = NULL,
                              MWindow *parent = NULL);
   /** Reply to one message.
       @param message message to reply to
       @param params is the Params struct to use
       @param profile environment
       @param parent window for dialog
   */
   static void ReplyMessage(class Message *msg,
                            const Params& params,
                            Profile *profile = NULL,
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
                            Ticket ticket = ILLEGAL_TICKET) = 0;
   //@}
   //@}

   /// Returns the shared logcircle object
   static MLogCircle & GetLogCircle(void)
      {
         return ms_LogCircle;
      }
   /** Get name of mailbox.
       @return the symbolic name of the mailbox
   */
   virtual String GetName(void) const = 0;

   /// Return IMAP spec
   virtual String GetImapSpec(void) const = 0;


   /** Get number of messages which have a message status of value
       when combined with the mask. When mask = 0, return total
       message count.
       @param mask is a (combination of) MessageStatus value(s) or 0 to test against
       @param value the value of MessageStatus AND mask to be counted
       @return number of messages
   */
   virtual unsigned long CountMessages(int mask = 0, int value = 0) const = 0;

   /// Count number of new messages.
   virtual unsigned long CountNewMessages(void) const = 0;

   /// Count number of recent messages.
   virtual unsigned long CountRecentMessages(void) const = 0;

   /** Count number of new messages but only if a listing is
       available, returns UID_ILLEGAL otherwise.
   */
   virtual unsigned long CountNewMessagesQuick(void) const = 0;

   /** Check whether mailbox has changed. */
   virtual void Ping(void) = 0;

   /** Perform a checkpoint on the folder. */
   virtual void Checkpoint(void) = 0;

   /** get the message with unique id uid
       @param uid message uid
       @return message handler
   */
   virtual Message *GetMessage(unsigned long uid) = 0;

   /** Delete a message, really delete, not move to trash. UNSUPPORTED!
       @param uid the message uid
       @return true on success
   */
   virtual bool DeleteMessage(unsigned long uid) = 0;

   /** Delete duplicate messages by Message-Id
    @return number of messages removed or UID_ILLEGAL on error*/
   virtual UIdType DeleteDuplicates() = 0;

   /** UnDelete a message.  UNSUPPORTED!
       @param uid the message uid
       @return true on success
   */
   virtual bool UnDeleteMessage(unsigned long uid) = 0;

   /** Set flags on a messages. Possible flag values are MSG_STAT_xxx
       @param uid the message uid
       @param flag flag to be set, e.g. "\\Deleted"
       @param set if true, set the flag, if false, clear it
       @return true on success
   */
   virtual bool SetMessageFlag(unsigned long uid,
                               int flag,
                               bool set = true) = 0;

   /** Set flags on a sequence of messages. Possible flag values are MSG_STAT_xxx
       @param sequence the IMAP sequence of uids
       @param flag flag to be set, e.g. "\\Deleted"
       @param set if true, set the flag, if false, clear it
       @return true on success
   */
   virtual bool SetFlag(const UIdArray *sequence,
                        int flag,
                        bool set = true) = 0;

   /** Set flags on a sequence of messages. Possible flag values are MSG_STAT_xxx
       @param sequence the IMAP sequence of uids
       @param flag flag to be set, e.g. "\\Deleted"
       @param set if true, set the flag, if false, clear it
       @return true on success
   */
   virtual bool SetSequenceFlag(const String &sequence,
                                int flag,
                                bool set = true) = 0;
   /** Appends the message to this folder.
       @param msg the message to append
       @param update update mailbox status
       @return true on success
   */
   virtual bool AppendMessage(const Message &msg, bool update = TRUE) = 0;

   /** Appends the message to this folder.
       @param msg text of the  message to append
       @param update update mailbox status
       @return true on success
   */
   virtual bool AppendMessage(const String &msg, bool update = TRUE) = 0;

   /** Expunge messages.
     */
   virtual void ExpungeMessages(void) = 0;

   /** Search Messages for certain criteria.
       @return UIdArray with UIds of matching messages, caller must
       free it
   */
   virtual UIdArray *SearchMessages(const class SearchCriterium *crit) = 0;

   /** Get the profile.
       @return Pointer to the profile.
   */
   virtual inline Profile *GetProfile(void) = 0;

   /// return class name
   const char *GetClassName(void) const
      { return "MailFolder"; }

   /// Flags controlling update behaviour
   enum UpdateFlags
   {
      /// undefined flags, used as default arg
      UF_Undefined   =   1,
      /// update the message counter
      UF_UpdateCount =   2,
      /// detect new messages as new
      UF_DetectNewMail = 4,
      /// default setting
      UF_Default = (UF_UpdateCount+UF_DetectNewMail)
   };
   /** Toggle update behaviour flags.
       @param updateFlags the flags to set
   */
   virtual void SetUpdateFlags(int updateFlags) = 0;
   /// Get the current update flags
   virtual int  GetUpdateFlags(void) const = 0;

   /**@name Some higher level functionality implemented by the
      MailFolder class on top of the other functions.
      These functions are not used by anything else in the MailFolder
      class and can easily be removed if needed.
   */
   //@{
   /** Save messages to a folder identified by MFolder

       NB: this function should eventually replace the other SaveMessages()
   */
   virtual bool SaveMessages(const UIdArray *selections,
                             MFolder *folder,
                             bool updateCount = true) = 0;

   /** Save the messages to a folder.
       @param selections the message indices which will be converted using the current listing
       @param folderName the name of the folder to save to
       @param isProfile if true, the folderName will be interpreted as
       a symbolic folder name, otherwise as a filename
       @param updateCount If true, the number of messages in the
       folder is updated. If false, they will be detected as new
       messages.
       @return true on success
   */
   virtual bool SaveMessages(const UIdArray *selections,
                             String const & folderName,
                             bool isProfile,
                             bool updateCount = true) = 0;
   /** Save the messages to a folder.
       @param selections the message indices which will be converted using the current listing
       @param fileName the name of the folder to save to
       @return true on success
   */
   virtual bool SaveMessagesToFile(const UIdArray *selections,
                                   String const & fileName) = 0;

   /** Mark messages as deleted or move them to trash.
       @param messages pointer to an array holding the message numbers
       @return true on success
   */
   virtual bool DeleteOrTrashMessages(const UIdArray *messages) = 0;

   /** Mark messages as deleted.
       @param messages pointer to an array holding the message numbers
       @param expunge expunge deleted messages
       @return true on success
   */
   virtual bool DeleteMessages(const UIdArray *messages,
                               bool expunge = false) = 0;

   /** Mark messages as no longer deleted.
       @param messages pointer to an array holding the message numbers
       @return true on success
   */
   virtual bool UnDeleteMessages(const UIdArray *messages) = 0;

   /** Save messages to a file.
       @param messages pointer to an array holding the message numbers
       @parent parent window for dialog
       @return true if messages got saved
   */
   virtual bool SaveMessagesToFile(const UIdArray *messages, MWindow
                                   *parent = NULL) = 0;

   /** Save messages to a folder.
       @param messages pointer to an array holding the message numbers
       @param parent window for dialog
       @param folder is the folder to save to, ask the user if NULL
       @return true if messages got saved
   */
   virtual bool SaveMessagesToFolder(const UIdArray *messages,
                                     MWindow *parent = NULL,
                                     MFolder *folder = NULL) = 0;

   /** Reply to selected messages.
       @param messages pointer to an array holding the message numbers
       @param params is the Params struct to use
       @param parent window for dialog
   */
   virtual void ReplyMessages(const UIdArray *messages,
                              const Params& params,
                              MWindow *parent = NULL) = 0;

   /** Forward selected messages.
       @param messages pointer to an array holding the message numbers
       @param params is the Params struct to use
       @param parent window for dialog
   */
   virtual void ForwardMessages(const UIdArray *messages,
                                const Params& params,
                                MWindow *parent = NULL) = 0;

   //@}

   /**@name Access control */
   //@{
   /** Locks a mailfolder for exclusive access. In multi-threaded mode
       it really locks it and always returns true. In single-threaded
       mode it returns false if we cannot get a lock.
       @return TRUE if we have obtained the lock
   */
   virtual bool Lock(void) const = 0;
   /** Releases the lock on the mailfolder. */
   virtual void UnLock(void) const = 0;
   /// Is folder locked?
   virtual bool IsLocked(void) const = 0;
   //@}
   /**@name Functions to get an overview of messages in the folder. */
   //@{
   /** Returns a listing of the folder. Must be DecRef'd by caller. */
   virtual class HeaderInfoList *GetHeaders(void) const = 0;
   //@}
   /// Return the folder's type.
   virtual FolderType GetType(void) const = 0;
   /// return the folder flags
   virtual int GetFlags(void) const = 0;

   /// Does the folder need a working network to be accessed?
   virtual bool NeedsNetwork(void) const
      {
         return
            (GetType() == MF_NNTP
             || GetType() == MF_IMAP
             || GetType() == MF_POP)
            && ! (GetFlags() & MF_FLAGS_ISLOCAL);
      }
   /** Sets a maximum number of messages to retrieve from server.
       @param nmax maximum number of messages to retrieve, 0 for no limit
   */
   virtual void SetRetrievalLimit(unsigned long nmax) = 0;

   /**@name Accessor methods */
   //@{
   /// Get authorisation information
   virtual inline void GetAuthInfo(String *login, String *password)
      const = 0;
   //@}

   /** Apply any filter rules to the folder. Only does anything if a
       filter module is loaded and a filter configured.
       @param NewOnly if true, only apply filter to recent messages
       @return -1 if no filter module exists, return code otherwise
   */
   virtual int ApplyFilterRules(bool NewOnly = true) = 0;
   /** Apply any filter rules to the folder.
       Applies the rule to all messages listed in msgs.
       @return -1 if no filter module exists, return code otherwise
   */
   virtual int ApplyFilterRules(UIdArray msgs) = 0;
   /** Request update. Causes the mailfolder to update its internal
       status information when required. If sendEvent is TRUE, it will
       send out an event that its info changed to cause immediate
       update. */
   virtual void RequestUpdate(bool sendEvent = FALSE) = 0;
   /// Process all internal update events in the queue.
   static void ProcessEventQueue(void);

   /// Clean up for program exit.
   static void CleanUp(void);
protected:
   static MLogCircle ms_LogCircle;
};

/** This class temporarily locks a mailfolder */
class MailFolderLock
{
public:
   /// Attempts to lock the folder.
   MailFolderLock(const MailFolder *mf)
      { m_Mf = mf; m_Locked = mf->Lock(); }
   /// Automatically releases lock.
   ~MailFolderLock()
      { if(m_Mf) m_Mf->UnLock(); }
   /// Call this to check that we have a lock.
   bool Locked(void) { return m_Locked; }
private:
   const MailFolder *m_Mf;
   bool              m_Locked;
};


/** This class holds information about a single folder. */
class FolderListingEntry
{
public:
   /// The folder's name.
   virtual const String &GetName(void) const = 0;
   /// The folder's attribute.
   virtual long GetAttribute(void) const = 0;
   virtual ~FolderListingEntry() {}
};

KBLIST_DEFINE(FolderListingList, FolderListingEntry);

/** This class holds the listings of a server's folders. */
class FolderListing
{
public:
   typedef FolderListingList::iterator iterator;
   /// Return the delimiter character for entries in the hierarchy.
   virtual char GetDelimiter(void) const = 0;
   /// Returns the number of entries.
   virtual size_t CountEntries(void) const = 0;
   /// Returns the first entry.
   virtual const FolderListingEntry *
   GetFirstEntry(FolderListing::iterator &i) const = 0;
   /// Returns the next entry.
   virtual const FolderListingEntry *
   GetNextEntry(FolderListing::iterator &i) const = 0;
   virtual ~FolderListing() {}
};

/** Sort order enum for sorting message listings. */
enum MessageSortOrder
{
   /// no sorting (i.e. sorting in the arrival order or reverse arrival order)
   MSO_NONE, MSO_NONE_REV,

   /// date or reverse date
   MSO_DATE, MSO_DATE_REV,
   MSO_SUBJECT, MSO_SUBJECT_REV,
   MSO_AUTHOR, MSO_AUTHOR_REV,
   MSO_STATUS, MSO_STATUS_REV,
   MSO_SCORE, MSO_SCORE_REV,
   MSO_THREAD, MSO_THREAD_REV

   // NB: the code in wxFolderListCtrl::OnColumnClick() relies on MSO_XXX_REV
   //     immediately following MSO_XXX
};

/// split a long value (as read from profile) into (several) sort orders
extern wxArrayInt SplitSortOrder(long sortOrder);

/// combine several (max 8) sort orders into one value
extern long BuildSortOrder(const wxArrayInt& sortOrders);

/** Search criterium for searching folder for certain messages. */
class SearchCriterium
{
public:
   SearchCriterium()
      {
         m_What = SC_ILLEGAL;
      }

   enum Type { SC_ILLEGAL = -1,
               SC_FULL = 0, SC_BODY, SC_HEADER, SC_SUBJECT, SC_TO,
               SC_FROM, SC_CC} m_What;
   bool m_Invert;
   String m_Key;
};

#include "HeaderInfo.h"

#endif
