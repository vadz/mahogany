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

/** The INTARRAY define is a class which is an integer array. It needs
    to provide a int Count() method to return the number of elements
    and an int operator[int] to access them.
    We use wxArrayInt for this.
    @deffunc INTARRAY
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

   /** Flags for several operations. */
   enum Flags
   {
      NONE = 0,
      REPLY_FOLLOWUP
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

   */
   static MailFolder * OpenFolder(int typeAndFlags,
                                  const String &path,
                                  ProfileBase *profile = NULL,
                                  const String &server = NULLstring,
                                  const String &login = NULLstring,
                                  const String &password = NULLstring);

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
       @return string representation
   */
   static String ConvertMessageStatusToString(int status);
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
                            Ticket ticket = ILLEGAL_TICKET) = 0;
   //@}
   //@}

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

   /// Count number of new messages.
   virtual unsigned long CountNewMessages(void) const = 0;

   /** Check whether mailbox has changed. */
   virtual void Ping(void) = 0;

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
   virtual bool SetFlag(const INTARRAY *sequence,
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

   /** Search Messages for certain criteria.
       @return INTARRAY with UIds of matching messages, caller must
       free it
   */
   virtual INTARRAY *SearchMessages(const class SearchCriterium *crit) = 0;
   
   /** Get the profile.
       @return Pointer to the profile.
   */
   virtual inline ProfileBase *GetProfile(void) = 0;

   /// return class name
   const char *GetClassName(void) const
      { return "MailFolder"; }

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
       folder is updated. If false, they will be detected as new
       messages.
       @return true on success
   */
   virtual bool SaveMessages(const INTARRAY *selections,
                             String const & folderName,
                             bool isProfile,
                             bool updateCount = true) = 0;
   /** Save the messages to a folder.
       @param selections the message indices which will be converted using the current listing
       @param fileName the name of the folder to save to
       @return true on success
   */
   virtual bool SaveMessagesToFile(const INTARRAY *selections,
                                   String const & fileName) = 0;

   /** Mark messages as deleted or move them to trash.
       @param messages pointer to an array holding the message numbers
       @return true on success
   */
   virtual bool DeleteOrTrashMessages(const INTARRAY *messages) = 0;

   /** Mark messages as deleted.
       @param messages pointer to an array holding the message numbers
       @param expunge expunge deleted messages
       @return true on success
   */
   virtual bool DeleteMessages(const INTARRAY *messages,
                               bool expunge = false) = 0;

   /** Mark messages as no longer deleted.
       @param messages pointer to an array holding the message numbers
       @return true on success
   */
   virtual bool UnDeleteMessages(const INTARRAY *messages) = 0;

   /** Save messages to a file.
       @param messages pointer to an array holding the message numbers
       @parent parent window for dialog
       @return true if messages got saved
   */
   virtual bool SaveMessagesToFile(const INTARRAY *messages, MWindow
                                   *parent = NULL) = 0;

   /** Save messages to a folder.
       @param messages pointer to an array holding the message numbers
       @param parent window for dialog
       @return true if messages got saved
   */
   virtual bool SaveMessagesToFolder(const INTARRAY *messages, MWindow *parent
                                     = NULL) = 0;

   /** Reply to selected messages.
       @param messages pointer to an array holding the message numbers
       @param parent window for dialog
       @param profile pointer for environment
       @param flags 0, or REPLY_FOLLOWUP
   */
   virtual void ReplyMessages(const INTARRAY *messages,
                              MWindow *parent = NULL,
                              int flags = 0) = 0;

   /** Forward selected messages.
       @param messages pointer to an array holding the message numbers
       @param parent window for dialog
   */
   virtual void ForwardMessages(const INTARRAY *messages,
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
       @param NewOnly if true, only apply filter to new messages
       @return -1 if no filter module exists, return code otherwise
   */
   virtual int ApplyFilterRules(bool NewOnly = true) = 0;
protected:
   /// Request update
   virtual void RequestUpdate(void) = 0;
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
   virtual const String &GetSubject(void) const = 0;
   virtual const String &GetFrom(void) const = 0;
   virtual time_t GetDate(void) const = 0;
   virtual const String &GetId(void) const = 0;
   virtual const String &GetReferences(void) const = 0;
   virtual UIdType GetUId(void) const = 0;
   virtual int GetStatus(void) const = 0;
   virtual unsigned long const &GetSize(void) const = 0;
   virtual size_t SizeOf(void) const = 0;
   HeaderInfo() {}
   virtual ~HeaderInfo() {}
   /// Return the indentation level for message threading.
   virtual unsigned GetIndentation() const = 0;
   /// Set the indentation level for message threading.
   virtual void SetIndentation(unsigned level) = 0;
private:
   /// Disallow copy construction
   HeaderInfo(const HeaderInfo &);
   GCC_DTOR_WARN_OFF
};

/** This class holds a complete list of all messages in the folder.
    It must be an array!*/
class HeaderInfoList : public MObjectRC
{
public:
   /// Count the number of messages in listing.
   virtual size_t Count(void) const = 0;
   /// Returns the n-th entry.
   virtual const HeaderInfo * operator[](size_t n) const = 0;
   /// Returns the n-th entry.
   virtual HeaderInfo * operator[](size_t n) = 0;
   /// Returns pointer to array of data:
   virtual HeaderInfo * GetArray(void) = 0;
   /// Swaps two elements:
   virtual void Swap(size_t index1, size_t index2) = 0;
   /** Sets a translation table re-mapping index values.
       Will be freed in destructor.
       @param array an array of indices or NULL to remove it.
   */
   virtual void SetTranslationTable(size_t array[] = NULL) = 0;
   MOBJECT_NAME(HeaderInfoList)
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
   /// no sorting
   MSO_NONE,
   /// date or reverse date
   MSO_DATE, MSO_DATE_REV,
   MSO_SUBJECT, MSO_SUBJECT_REV,
   MSO_AUTHOR, MSO_AUTHOR_REV,
   MSO_STATUS, MSO_STATUS_REV,
   MSO_SCORE, MSO_SCORE_REV,
   MSO_THREAD, MSO_THREAD_REV
};


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
#endif
