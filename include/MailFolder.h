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

#include "MObject.h"
#include "FolderType.h"
#include "kbList.h"

class HeaderInfoList;

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
class MFrame;
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
  MailFolderStatus contains the "interesting" and often changign information
  about the folder such as the number of new/unread messages in it
*/

struct MailFolderStatus
{
   MailFolderStatus() { Init(); }

   void Init()
   {
      total = UID_ILLEGAL;
      newmsgs =
      recent =
      unseen =
      flagged =
      searched = 0;
   }

   bool IsValid() const { return total != UID_ILLEGAL; }

   bool operator==(const MailFolderStatus& status)
   {
      return total == status.total &&
             newmsgs == status.newmsgs &&
             recent == status.recent &&
             unseen == status.unseen &&
             flagged == status.flagged &&
             searched == status.searched;
   }

   // the total number of messages and the number of new, recent, unseen,
   // important (== flagged) and searched (== results of search) messages in
   // this folder
   unsigned long total,
                 newmsgs,
                 recent,
                 unseen,
                 flagged,
                 searched;
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

   /// default ctor
   MailFolder() { m_frame = NULL; }

   /** @name Static functions, implemented in MailFolder.cpp */
   //@{

   /** @name Opening folders */
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
   //@}

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

       @param message flags
       @return string representation
   */
   static String ConvertMessageStatusToString(int status);

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

   /** Count the number of new, recent and unseen messages: this is equivelent
       to calling CountNewMessages(), CountRecentMessages() and
       CountUnseenMessages() in succession but may be 3 times faster.

       NB: this is pure virtual although could be implemented in the base class

       @param status is the struct to fill with the folder info
       @return true if any new/recent/unseen messages found, false otherwise
   */
   virtual bool CountInterestingMessages(MailFolderStatus *status) const = 0;

   /// Count number of new messages (== RECENT && !SEEN)
   unsigned long CountNewMessages(void) const
   {
      return CountMessages(MSG_STAT_RECENT | MSG_STAT_SEEN, MSG_STAT_RECENT);
   }

   /// Count number of recent messages.
   unsigned long CountRecentMessages(void) const
      { return CountMessages(MSG_STAT_RECENT, MSG_STAT_RECENT); }

   /// Count number of unread messages
   unsigned long CountUnseenMessages(void) const
      { return CountMessages(MSG_STAT_SEEN, 0); }

   /** Check whether mailbox has changed.
       @return FALSE on error, TRUE otherwise
   */
   virtual bool Ping(void) = 0;

   /** Call Ping() on all opened mailboxes. */
   static bool PingAllOpened(void);

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

   /** UnDelete a message.
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

   /** Get a string uniquely identifying the message in this folder, will be
       empty if not supported by this folder type

       @param msgno the number of the message in folder
       @return string uniquely identifying the message in this folder
   */
   virtual String GetMessageUID(unsigned long msgno) const = 0;

   /** Get the profile.
       @return Pointer to the profile.
   */
   virtual Profile *GetProfile(void) = 0;

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

   /** This function takes a header listing and sorts and threads it.
   */
   virtual void ProcessHeaderListing(HeaderInfoList *hilp) = 0;

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
   /// returns true if the folder is opened
   virtual bool HasHeaders() const = 0;
   /** Returns a listing of the folder. Must be DecRef'd by caller. */
   virtual HeaderInfoList *GetHeaders(void) const = 0;
   //@}
   /// Return the folder's type.
   virtual FolderType GetType(void) const = 0;
   /// return the folder flags
   virtual int GetFlags(void) const = 0;

   /// Does the folder need a working network to be accessed?
   virtual bool NeedsNetwork(void) const
   {
      return FolderNeedsNetwork(GetType(), GetFlags());
   }

   /** Sets limits for the number of headers to retrieve: if hard limit is not
       0, we will never retrieve more than that many messages even without
       asking the user (soft limit is ignored). Otherwise, we will ask the
       user if the soft limit is exceeded.

       @param soft maximum number of messages to retrieve without askin
       @param hard maximum number of messages to retrieve, 0 for no limit
   */
   virtual void SetRetrievalLimits(unsigned long soft, unsigned long hard) = 0;

   /**@name Accessor methods */
   //@{
   /// Get authorisation information
   virtual void GetAuthInfo(String *login, String *password) const = 0;
   /**
      Returns the delimiter used to separate the components of the folder
      name

      @return character dependind on the folder type and server
    */
   virtual char GetFolderDelimiter() const;
   //@}

   /** Apply any filter rules to the folder.
       Applies the rule to all messages listed in msgs.
       @return -1 if no filter module exists, return code otherwise
   */
   virtual int ApplyFilterRules(UIdArray msgs) = 0;

   /** Request update: sends an event telling everyone that the mail folder
       listing has changed.
   */
   virtual void RequestUpdate() = 0;

   /** @name Various static functions

       They are implemented elsewhere but declared here to minimize the header
       dependencies.
    */
   //@{
   /// returns TRUE if we have any MH folders on this system
   static bool ExistsMH();

   /**
      initialize the MH driver (it's safe to call it more than once) - has a
      side effect of returning the MHPATH which is the root path under which
      all MH folders should be situated on success. Returns empty string on
      failure.

      If the string is not empty it will be '/' terminated.
   */
   static const String& InitializeMH();

   /**
      initialize the NEWS driver (it's safe to call it more than once).

      @return the path to local news spool or empty string on failure
   */
   static const String& InitializeNewsSpool();

   /**
      build the IMAP spec for its components

      @return the full IMAP spec for the folder
   */
   static String GetImapSpec(int type, int flags,
                             const String &name,
                             const String &server,
                             const String &login);

   /** Extracts the folder name from the folder specification string used by
       cclient (i.e. {nntp/xxx}news.answers => news.answers and also #mh/Foo
       => Foo)

       @param specification the full cclient folder specification
       @param folderType the (supposed) type of the folder
       @param name the variable where the folder name will be returned
       @return TRUE if folder name could be successfully extracted
    */
   static bool SpecToFolderName(const String& specification,
                                FolderType folderType,
                                String *name);

   /** A helper function: remove the MHPATH prefix from the path and return
       TRUE or return FALSE if the path is absolute but doesn't start with
       MHPATH. Don't change anything for relative paths.

       @param path: full path to the MH folder on input, folder name on output
       @return TRUE if path was a valid MH folder
   */
   static bool GetMHFolderName(String *path);

   /** imports either just the top-level MH folder or it and all MH subfolders
       under it
   */
   static bool ImportFoldersMH(const String& root, bool allUnder = true);

   //@}

   /// Initialize the underlying mail library
   static bool Init();

   /// Clean up for program exit.
   static void CleanUp(void);

   /** @name Interactivity control */
   //@{
   /**
      Folder opening functions work differently if SetInteractive() is set:
      they will put more messages into status bar and possibly ask questions to
      the user while in non interactive mode this will never be done. This
      method associates such frame with the folder with given name.

      @param frame the window where the status messages should go (may be NULL)
      @param foldername the folder for which we set this frame
   */
   static void SetInteractive(MFrame *frame, const String& foldername);

   /**
      Undo SetInteractive()
   */
   static void ResetInteractive();

   /**
      Set interactive frame for this folder only: long operations on this folder
      will put the status messages in this frames status bar.

      @return the old interactive frame for this folder
   */
   MFrame *SetInteractiveFrame(MFrame *frame);

   /**
      Get the frame to use for interactive messages. May return NULL.
   */
   MFrame *GetInteractiveFrame() const;
   //@}

protected:
   /// the helper class for determining the exact error msg from cclient log
   static MLogCircle ms_LogCircle;

   /// the folder for which we had set default interactive frame
   static String ms_interactiveFolder;

   /// the frame to which interactive messages for ms_interactiveFolder go
   static MFrame *ms_interactiveFrame;

   /// the frame to which messages for this folder go by default
   MFrame *m_frame;
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

   /// subject
   MSO_SUBJECT, MSO_SUBJECT_REV,

   /// from
   MSO_AUTHOR, MSO_AUTHOR_REV,

   /// status (deleted < answered < unread < new)
   MSO_STATUS, MSO_STATUS_REV,

   /// score
   MSO_SCORE, MSO_SCORE_REV,

   /// size in bytes
   MSO_SIZE, MSO_SIZE_REV

   // NB: the code in wxFolderListCtrl::OnColumnClick() and ComparisonFunction()
   //     relies on MSO_XXX_REV immediately following MSO_XXX, so don't change
   //     the values of the elements of this enum and always add MSO_XXX and
   //     MSO_XXX in this order.
   // Don't forget to add to sortCriteria[] in wxMDialogs.cpp
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

/**
   Formats a message replacing the occurences of the format specifiers with the
   data. The format specifiers (listed with their meanings) are:

      %f          folder name
      %t          total number of messages in the folder
      %n          number of new messages in the folder
      %r          number of recent messages in the folder
      %u          number of unseen/unread messages in the folder
      %%          percent sign

   The function uses MailFolderStatus oassed to it and will fill it if it is in
   unfilled state and if it needs any of the data in it - so you should reuse
   the same struct when calling this function several times.if possible.

   @param format the format string
   @param folderName the full folder name
   @param status the status struct to use and fill
   @param mf the mail folder to use, may be NULL (then folderName is used)
   @return the formatted string
 */
String FormatFolderStatusString(const String& format,
                                const String& folderName,
                                MailFolderStatus *status,
                                const MailFolder *mf = NULL);

// ----------------------------------------------------------------------------
// global (but private to MailFolder) functions
//
// unfortunately for now these functions have to live here, it would be much
// better to have some less clumsy initialization code... (FIXME)
// ----------------------------------------------------------------------------

/// init common MF code
extern bool MailFolderCmnInit();

// clean up common MF code data
extern void MailFolderCmnCleanup();

/// init cclient
extern bool MailFolderCCInit();

/// shutdown cclient
extern void MailFolderCCCleanup();

#endif /// MAILFOLDER_H

