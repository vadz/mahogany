/*-*- c++ -*-********************************************************
 * MailFolderCC class: handling of mail folders through C-Client lib*
 *                                                                  *
 * (C) 1997-2000 by Karsten Ballüder (ballueder@gmx.net)            *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/

/**
   @package Mailfolder access
   @author  Karsten Ballüder
*/

#ifndef MAILFOLDERCC_H
#define MAILFOLDERCC_H

#ifdef __GNUG__
#   pragma interface "MailFolderCC.h"
#endif

#ifndef   USE_PCH
#   include  "kbList.h"
#   include  "MObject.h"

// includes for c-client library
#include "Mcclient.h"
#endif

#include  "MailFolderCmn.h"
#include  "FolderView.h"
#include  "MFolder.h"

#include  <wx/fontenc.h>

/// To really clean up left over memory, call this function at program
/// end:
extern void CC_Cleanup();

// fwd decl needed to define StreamListType before MailFolderCC
// (can't be defined inside the class - VC++ 5.0 can't compile it)
class MailFolderCC;

/// structure to hold MailFolder pointer and associated mailstream pointer
struct StreamConnection
{
   /// pointer to a MailFolderCC object
   MailFolderCC    *folder;
   /// pointer to the associated MAILSTREAM
   MAILSTREAM   const *stream;
   /// name of the MailFolderCC object
   String name;
   /// for POP3/IMAP/NNTP: login or newsgroup
   String login;
};

KBLIST_DEFINE(StreamConnectionList, StreamConnection);
KBLIST_DEFINE(FolderViewList, FolderView);


/**
   MailFolder class, implemented with the C-client library.

   This class implements the functionality defined by the MailFolder
   parent class by using the c-client library for mailbox access. It
   handles all the callbacks from the library and routes them to the
   relevant objects by using a static list of objects and associated
   MAILSTREAM pointers.
*/
class MailFolderCC : public MailFolderCmn
{
public:
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
      <li>MF_IMAP:  path
      <li>MF_NNTP:  newsgroup
      </ul>
      @param type one of the supported types
      @param path either a hostname or filename depending on type
      @param profile parent profile
      @param server server host
      @param login only used for POP,IMAP and NNTP (as the newsgroup name)
      @param password only used for POP, IMAP
      @param halfopen to only half open the folder

   */
   static MailFolderCC * OpenFolder(int typeAndFlags,
                                    String const &path,
                                    Profile *profile,
                                    String const &server,
                                    String const &login,
                                    String const &password,
                                    String const &symname,
                                    bool halfopen);

   //@}

   /** Phyically deletes this folder.
       @return true on success
   */
   static bool DeleteFolder(const class MFolder *mfolder);

   /// return the directory of the newsspool:
   static String GetNewsSpool(void);

   /// checks whether a folder with that path exists
   static MailFolderCC *FindFolder(String const &path,
                                   String const &login);

   /** Checks if it is OK to exit the application now.
        @param which Will either be set to empty or a '\n' delimited
        list of folders which are in critical sections.
   */
   static bool CanExit(String *which);

   /** return a symbolic name for mail folder
       @return the folder's name
   */
   inline String GetName(void) const { return m_Name; }

   /** Get the profile.
       @return Pointer to the profile.
   */
   virtual inline Profile *GetProfile(void) { return m_Profile; }

   /// Checks if the folder is in a critical section.
   inline bool InCritical(void) const { return m_InCritical; }

   /** Get number of messages which have a message status of value
       when combined with the mask. When mask = 0, return total
       message count.
       @param mask is a (combination of) MessageStatus value(s) or 0 to test against
       @param value the value of MessageStatus AND mask to be counted
       @return number of messages
   */
   virtual unsigned long CountMessages(int mask = 0, int value = 0) const;

   /// return number of recent messages
   virtual unsigned long CountRecentMessages(void) const;

   /** Count number of new messages but only if a listing is
       available, returns UID_ILLEGAL otherwise.
   */
   virtual unsigned long CountNewMessagesQuick(void) const;

   /** get message header
       @param uid mesage uid
       @return message header information class
   */
   class Message *GetMessage(unsigned long uid);

   /** Set flags on a sequence of messages. Possible flag values are MSG_STAT_xxx
       @param sequence the IMAP sequence
       @param flag flag to be set, e.g. "\\Deleted"
       @param set if true, set the flag, if false, clear it
       @return always true UNSUPPORTED!
   */
   virtual bool SetSequenceFlag(String const &sequence,
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

   /** Appends the message to this folder.
       @param msg the message to append
       @return true on success
   */
   virtual bool AppendMessage(Message const &msg);

   /** Appends the message to this folder.
       @param msg text of the  message to append
       @return true on success
   */
   virtual bool AppendMessage(String const &msg);

   /** Expunge messages.
     */
   void ExpungeMessages(void);

   /** Search Messages for certain criteria.
       @return UIdArray with UIds of matching messages
   */
   virtual UIdArray *SearchMessages(const class SearchCriterium *crit);

   /** Check whether mailbox has changed. */
   void Ping(void);

   /** Perform a checkpoint on the folder. */
   virtual void Checkpoint(void);

   /**@name Semi-public functions, shouldn't usually be called by
      normal code. */
   //@{
   /** Most of the Ping() functionality, but without updating the
       mailbox status (mail_check()), a bit quicker. Returns true if
       mailstream is still valid.
       Just pings the stream and tries to reopen it if needed
       @return true if stream still valid
   */
   bool PingReopen(void) const;
   /** Like PingReopen() but works on all folders, returns true if all
       folders are fine.
   */
   static bool PingReopenAll(void);

   //@}

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
   static  bool Subscribe(const String &host,
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
   void ListFolders(class ASMailFolder *asmf,
                    const String &pattern = "*",
                    bool subscribed_only = false,
                    const String &reference = "",
                    UserData ud = 0,
                    Ticket ticket = ILLEGAL_TICKET);

   //@}
   /**@name Access control */
   //@{
   /** Locks a mailfolder for exclusive access. In multi-threaded mode
       it really locks it and always returns true. In single-threaded
       mode it returns false if we cannot get a lock.
       @return TRUE if we have the lock
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
   /** Sets a maximum number of messages to retrieve from server.
       @param nmax maximum number of messages to retrieve, 0 for no limit
   */
   virtual void SetRetrievalLimit(unsigned long nmax)
      { m_RetrievalLimit = nmax; }

   /** RFC 2047 compliant message decoding: all encoded words from the header
       are decoded, but only the encoding of the first of them is returned in
       encoding output parameter (if non NULL).
     
       @param in the RFC 2047 header string
       @param encoding the pointer to the charset of the string (may be NULL)
       @return the full 8bit decoded string
   */
   static String DecodeHeader(const String &in,
                              wxFontEncoding *encoding = NULL);

   /// return TRUE if CClient lib had been initialized
   static bool IsInitialized() { return cclientInitialisedFlag; }

   /** @name Folder names and specifications */
   //@{
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
   //@}

   /** @name MH folders support */
   //@{
   /// returns TRUE if we have any MH folders on this system
   static bool ExistsMH();

   /** imports either just the top-level MH folder or it and all MH subfolders
       under it
   */
   static bool ImportFoldersMH(const String& root, bool allUnder = true);

   /**
      initialize the MH driver (it's safe to call it more than once) - has a
      side effect of returning the MHPATH which is the root path under which
      all MH folders should be situated on success. Returns empty string on
      failure.

      If the string is not empty it will be '/' terminated.
   */
   static const String& InitializeMH();
   //@}

   /**
      initialize the NEWS driver (it's safe to call it more than once).

      @return the path to local news spool or empty string on failure
   */
   static const String& InitializeNewsSpool();

private:
   /// private constructor, does basic initialisation
   MailFolderCC(int typeAndFlags,
                String const &path,
                Profile *profile,
                String const &server,
                String const &login,
                String const &password);

   /// Common code for constructors
   void Create(int typeAndFlags);

   /** Try to open the mailstream for this folder.
       @return true on success
   */
   bool Open(void);

   /// half open the folder
   bool HalfOpen(void);


   /// The following is also called by SendMessageCC for ESMTP authentication
   static void SetLoginData(const String &user, const String &pw);
   friend class SendMessageCC;

   /// for POP/IMAP boxes, this holds the user id for the callback
   static String MF_user;
   /// for POP/IMAP boxes, this holds the password for the callback
   static String MF_pwd;

   /// which type is this mailfolder?
   FolderType   m_folderType;

   ///   mailstream associated with this folder
   MAILSTREAM   *m_MailStream;

   /// PingReopen() protection against recursion
   bool m_PingReopenSemaphore;
   /// BuildListing() protection against recursion
   bool m_BuildListingSemaphore;
   /// Do we need to update folder listing?
   bool m_UpdateNeeded;
   /// Do we need an update?
   bool UpdateNeeded(void) const { return m_UpdateNeeded; }
   /// number of messages in mailbox
   unsigned long m_nMessages;
   /// number or recent messages in mailbox
   unsigned long m_nRecent;
   /// last seen UID
   UIdType m_LastUId;
   /// last number of messages
   unsigned long m_OldNumOfMessages;
   /// set to true before we get the very first folder info
   bool m_FirstListing;
   /// Path to mailbox
   String   m_MailboxPath;

   /// The symbolic name of the folder
   String m_Name;

   /** If we are searching, this points to an UIdArray where to store
       the entries found.
   */
   UIdArray *m_SearchMessagesFound;

   /** @name functions for mapping mailstreams and objects
       These functions enable the class to map incoming events from
       the  c-client library to the object associated with that event.
       */
   //@{

   /// a pointer to the object to use as default if lookup fails
   static MailFolderCC   *streamListDefaultObj;

   /// mapping MAILSTREAM* to objects of this class and their names
   static StreamConnectionList   streamList;

   /// has c-client library been initialised?
   static bool   cclientInitialisedFlag;

   /// initialise c-client library
   static void CClientInit(void);

   /// adds this object to Map
   void   AddToMap(MAILSTREAM const *stream) const;

   /// remove this object from Map
   void   RemoveFromMap(void) const;

   /// Gets first mailfolder in map or NULL.
   static MailFolderCC * GetFirstMapEntry(StreamConnectionList::iterator &i);
   /// Gets next mailfolder in map or NULL
   static MailFolderCC * GetNextMapEntry(StreamConnectionList::iterator &i);

   /** set the default object in Map
       @param setit if false, erase default object
   */

   void SetDefaultObj(bool setit = true) const;

   /// lookup object in Map
   static MailFolderCC *LookupObject(MAILSTREAM const *stream,
                                     const char *name = NULL);
   //@}
   /** for use by class MessageCC
       @return MAILSTREAM of the folder
   */
   inline MAILSTREAM   *Stream(void) const{  return m_MailStream; }
   friend class MessageCC;

   /// return the folder type
   FolderType GetType(void) const { return m_folderType; }
   /// return the folder flags
   int GetFlags(void) const { return m_FolderFlags; }
protected:
   /// Request update
   virtual void RequestUpdate(bool sendEvents = false);
   /// Update the folder status, number of messages, etc
   virtual void UpdateStatus(void);
   /// Update the timeout values from a profile
   void UpdateTimeoutValues(void);
   /// apply all timeout values
   void ApplyTimeoutValues(void);
   void SetType(FolderType type) { m_folderType = type; }

   /// Check if this message is a "New Message":
   virtual bool IsNewMessage(const HeaderInfo * hi);

   /* Handles the mm_overview_header callback on a per folder basis.
      It returns 0 to abort overview generation, 1 to continue.*/
   int OverviewHeaderEntry (unsigned long uid, OVERVIEW *ov);
   /// closes the mailstream
   void Close(void);

#ifdef USE_THREADS
   /// A Mutex to control access to this folder.
   class MMutex *m_Mutex;
#else
   /// A Mutex to control access to this folder.
   bool m_Mutex;
#endif
   /*@name Handling of MailFolderCC internal events.
     Callbacks from the c-client library cannot directly be used to
     call other functions as this might lead to a lock up or recursion
     in the mail handling routines. Therefore all callbacks add events
     to a queue which will be processed after calls to c-client
     return.
   */
   //@{

public:
   /// Process all events in the queue.
   static void ProcessEventQueue(void);

   /// Type of the event, one for each callback.
   enum EventType
   {
      Searched, Exists, Expunged, Flags, Notify, List,
      LSub, Status, Log, DLog, Update
   };
   /// A structure for passing arguments.
   union EventArgument
   {
      char            m_char;
      String        * m_str;
      int             m_int;
      long            m_long;
      unsigned long   m_ulong;
      MAILSTATUS    * m_status;
   };
   /// The event structure.
   struct Event
   {
#ifndef DEBUG
      Event(MAILSTREAM *stream, EventType type, int /*caller*/)
         {
            m_stream = stream; m_type = type;
         }
#else
      Event(MAILSTREAM *stream, EventType type,
            int line)
         {
            m_stream = stream; m_type = type;
            m_caller.Printf("line %d", line);
            m_folderName = stream ? stream->mailbox : "no mailbox";
         }
#endif
      /// The type.
      EventType   m_type;
      /// The stream it relates to.
      MAILSTREAM *m_stream;
      /// The data structure, no more than three members needed
      EventArgument m_args[3];
#ifdef DEBUG
      /// where it was called from:
      String m_caller;
      /// the name of the folder:
      String m_folderName;
#endif
   };
   KBLIST_DEFINE(EventQueue, Event);
   /**    Add an event to the queue, called from (C) mm_ callback
          routines.
          @param pointer to a new MailFolderCC::Event structure, will be freed by event handling mechanism.
   */
   static void QueueEvent(Event *evptr)
      { ms_EventQueue.push_back(evptr); }

protected:
   /// Updates the status of a single message.
   void UpdateMessageStatus(unsigned long seqno);
   /// Gets a complete folder listing from the stream.
   void BuildListing(void);
   /// The list of events to be processed.
   static EventQueue ms_EventQueue;
   /** The index of the next entry in list to fill. Only used for
       BuildListing()/OverviewHeader() interaction. */
   unsigned long      m_BuildNextEntry;
   /// The maximum number of messages to retrive or 0
   unsigned long m_RetrievalLimit;
   //@}
public:
   /// re-read some cclient settings
   static void UpdateCClientConfig(void);

   /** @name common callback routines
       They all take a stram argument and the number of a message.
       Do not call them, they are only for use by the c-client library!
   */
   //@{

   /// this message matches a search
   static void mm_searched(MAILSTREAM *stream, unsigned long number);

   /// tells program that there are this many messages in the mailbox
   static void mm_exists(MAILSTREAM *stream, unsigned long number);

   /** deliver stream message event
       @param stream mailstream
       @param str message str
       @param errflg error level
       */
   static void mm_notify(MAILSTREAM *stream, String str, long
                         errflg);

   /** this mailbox name matches a listing request
       @param stream mailstream
       @param delim   character that separates hierarchies
       @param name    mailbox name
       @param attrib   mailbox attributes
       */
   static void mm_list(MAILSTREAM *stream, char delim, String name,
                       long attrib);

   /** matches a subscribed mailbox listing request
       @param stream   mailstream
       @param delim   character that separates hierarchies
       @param name   mailbox name
       @param attrib   mailbox attributes
       */
   static void mm_lsub(MAILSTREAM *stream, char delim, String name,
                       long attrib);
   /** status of mailbox has changed
       @param stream   mailstream
       @param mailbox    mailbox name for this status
       @param status   structure with new mailbox status
       */
   static void mm_status(MAILSTREAM *stream, String mailbox, MAILSTATUS *status);

   /** log a message
       @param str   message string
       @param errflg   error level
       @param mf if non-NULL the folder
       */
   static void mm_log(String str, long errflg, MailFolderCC *mf = NULL);

   /** log a debugging message
       @param str    message string
       */
   static void mm_dlog(String str);

   /** get user name and password
       @param   mb   parsed mailbox specification
       @param   user    pointer where to return username
       @param   pwd   pointer where to return password
       @param    trial   number of prior login attempts
       */
   static void mm_login(NETMBX *mb, char *user, char *pwd, long trial);

   /** alert that c-client will run critical code
       @param   stream   mailstream
   */
   static void mm_critical(MAILSTREAM *stream);

   /**   no longer running critical code
   @param   stream mailstream
     */
   static void mm_nocritical(MAILSTREAM *stream);

   /** unrecoverable write error on mail file
       @param   stream   mailstream
       @param   errcode   OS error code
       @param   serious   non-zero: c-client cannot undo error
       @return   abort flag: if serious error and abort non-zero: abort, else retry
       */
   static long mm_diskerror(MAILSTREAM *stream, long errcode, long serious);

   /** program is about to crash!
       @param   str   message string
       */
   static void mm_fatal(char *str);

   /// gets called when flags for a message have changed
   static void mm_flags(MAILSTREAM *stream, unsigned long number);

   /* Handles the mm_overview_header callback on a per folder basis. */
   static int OverviewHeader (MAILSTREAM *stream, unsigned long uid, OVERVIEW *ov);

//@}
private:
   /// destructor
   ~MailFolderCC();

   /// a profile
   Profile *m_Profile;
   /// The current listing of the folder
   class HeaderInfoListCC *m_Listing;
   /// Do we need to generate a new listing?
   bool m_NeedFreshListing;
   /// do we suppress listing udpates?
   bool m_ListingFrozen;
   /// Is this folder in a critical c-client section?
   bool m_InCritical;
   /** We remember the last folder to enter a critical section, helps
       to find crashes.*/
   static String ms_LastCriticalFolder;
   /// folder flags
   int  m_FolderFlags;
   /** @name Global settings, timeouts for c-client lib */
   //@{
   /// TCP/IP open timeout in seconds.
   int ms_TcpOpenTimeout;
   /// TCP/IP read timeout in seconds.
   int ms_TcpReadTimeout;
   /// TCP/IP write timeout in seconds.
   int ms_TcpWriteTimeout;
   /// TCP/IP close timeout in seconds.
   int ms_TcpCloseTimeout;
   /// rsh connection timeout in seconds.
   int ms_TcpRshTimeout;
   //@}

   /// Used by the subscription management.
   class FolderListingCC *m_FolderListing;

   /// Counts the number of new mails
   UIdType CountNewMessages(void) const;

   /**@name only used for ListFolders: */
   //@{
   UserData m_UserData;
   Ticket   m_Ticket;
   class ASMailFolder *m_ASMailFolder;
   //@}

   // used by InitializeMH() only
   static String ms_MHpath;

   // used by InitializeNewsSpool() only
   static String ms_NewsPath;

public:
   DEBUG_DEF
   MOBJECT_DEBUG(MailFolderCC)
};

#endif
