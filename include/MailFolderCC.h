//////////////////////////////////////////////////////////////////////////////
// Project:     Mahogany - cross platform e-mail GUI client
// File name:   MailFolderCC.h: declaration of MailFolderCC class
// Purpose:     handling of mail folders with c-client lib
// Author:      Karsten Ballüder
// Modified by:
// Created:     1997
// CVS-ID:      $Id$
// Copyright:   (C) 1997-2001 by Karsten Ballüder (ballueder@gmx.net)
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef MAILFOLDERCC_H
#define MAILFOLDERCC_H

#ifdef __GNUG__
#   pragma interface "MailFolderCC.h"
#endif

#ifndef   USE_PCH
#  include "kbList.h"

#  include "Mcclient.h"
#endif

#include "MailFolderCmn.h"
#include "FolderView.h"

#include <wx/fontenc.h>    // enum wxFontEncoding can't be fwd declared

// fwd decls
class MailFolderCC;
class MMutex;

// ----------------------------------------------------------------------------
// helper classes
// ----------------------------------------------------------------------------

/// structure to hold MailFolder pointer and associated mailstream pointer
class StreamConnection
{
public:
   /// pointer to a MailFolderCC object
   MailFolderCC    *folder;
   /// pointer to the associated MAILSTREAM
   MAILSTREAM   const *stream;
   /// name of the MailFolderCC object
   String name;
   /// for POP3/IMAP/NNTP: login or newsgroup
   String login;

   StreamConnection()
   {
      folder = NULL;
      stream = NULL;
   }

#ifdef DEBUG
   virtual ~StreamConnection();
#endif // DEBUG
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

   static bool CloseFolder(const MFolder *mfolder);
   static int CloseAll();
   //@}

   /** Phyically deletes this folder.
       @return true on success
   */
   static bool DeleteFolder(const MFolder *mfolder);

   static long ClearFolder(const MFolder *folder);

   /// return the directory of the newsspool:
   static String GetNewsSpool(void);

   /// wrapper for FindFolder() version below
   static MailFolderCC *FindFolder(const MFolder* mfolder);

   /// checks whether a folder with that path exists
   static MailFolderCC *FindFolder(const String& path, const String& login);

   /** Checks if it is OK to exit the application now.
        @param which Will either be set to empty or a '\n' delimited
        list of folders which are in critical sections.
   */
   static bool CanExit(String *which);

   virtual bool IsOpened(void) const { return m_MailStream != NULL; }

   /** return a symbolic name for mail folder
       @return the folder's name
   */
   virtual String GetName(void) const { return m_Name; }

   /// return the folder type
   virtual FolderType GetType(void) const { return m_folderType; }

   /// return the folder flags
   virtual int GetFlags(void) const { return m_FolderFlags; }

   /// Return IMAP spec
   virtual String GetImapSpec(void) const { return m_ImapSpec; }

   // capabilities
   virtual bool CanSort() const;
   virtual bool CanThread() const;

   /** Get the profile.
       @return Pointer to the profile.
   */
   virtual Profile *GetProfile(void) { return m_Profile; }

   /// Checks if the folder is in a critical section.
   bool InCritical(void) const { return m_InCritical; }

   // count various kinds of messages
   virtual unsigned long CountNewMessages(void) const;
   virtual unsigned long CountRecentMessages(void) const;
   virtual unsigned long CountUnseenMessages(void) const;
   virtual unsigned long CountDeletedMessages(void) const;

   // uid -> msgno
   virtual MsgnoType GetMsgnoFromUID(UIdType uid) const;

   /** get message header
       @param uid mesage uid
       @return message header information class
   */
   virtual Message *GetMessage(unsigned long uid);

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

   /// override base class SaveMessages() to do server side copy if possible
   virtual bool SaveMessages(const UIdArray *selections, MFolder *folder);

   /** Appends the message to this folder.
       @param msg the message to append
       @param update update mailbox status
       @return true on success
   */
   virtual bool AppendMessage(const Message & msg);

   /** Appends the message to this folder.
       @param msg text of the  message to append
       @param update update mailbox status
       @return true on success
   */
   virtual bool AppendMessage(const String& msg);

   /** Expunge messages.
     */
   virtual void ExpungeMessages(void);

   virtual MsgnoArray *SearchByFlag(MessageStatus flag,
                                    bool set = true,
                                    bool undeletedOnly = true) const;

   /** Search Messages for certain criteria.
       @return UIdArray with UIds of matching messages
   */
   virtual UIdArray *SearchMessages(const class SearchCriterium *crit);

   /** Get a string uniquely identifying the message in this folder, will be
       empty if not supported by this folder type

       @param msgno the number of the message in folder
       @return string uniquely identifying the message in this folder
   */
   virtual String GetMessageUID(unsigned long msgno) const;

   /** Check whether mailbox has changed.
       @return FALSE on error
   */
   bool Ping(void);

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
   bool PingReopen(void);
   /** Like PingReopen() but works on all folders, returns true if all
       folders are fine.
       @param fullPing does a full Ping() instead of a PingReopen()
       only. If doing a full
   */
   static bool PingReopenAll(bool fullPing = FALSE);

   /** Call Ping() on all opened mailboxes. */
   static bool PingAllOpened(void)
      { return PingReopenAll(TRUE) ; }
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
   virtual unsigned long GetMessageCount() const;
   virtual MsgnoType GetHeaderInfo(HeaderInfo **headers,
                                   MsgnoType msgnoFrom, MsgnoType msgnoTo);
   //@}

   virtual char GetFolderDelimiter() const;

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
   static bool IsInitialized() { return ms_CClientInitialisedFlag; }

   /**
      check whether a folder can have subfolders
   */
   static bool HasInferiors(const String &imapSpec,
                            const String &user,
                            const String &pw);

   /** @name Functions used by MessageCC for read progress. */
   //@{
   /**
     Prepare for reading the given amount of data from folder.

     @param total the amount of bytes to be read
    */
   void StartReading(unsigned long total);

   /**
     Called when data is read
    */
   void EndReading();
   //@}

   /**
      For use by class MessageCC only

      @return MAILSTREAM of the folder
   */
   MAILSTREAM *Stream(void) const { return m_MailStream; }

   /** For use by CCNewMailProcessor only
   */
   void OnNewMail();

private:
   /** @name Constructors and such */
   //@{
   /// private constructor, does basic initialisation
   MailFolderCC(int typeAndFlags,
                String const &path,
                Profile *profile,
                String const &server,
                String const &login,
                String const &password);

   /// Common code for constructors
   void Create(int typeAndFlags);

   /// code common to Create() and Close()
   void Init();

   /// destructor closes the folder automatically
   ~MailFolderCC();
   //@}

   /** @name Opening/closing the folders */
   //@{
   /** Try to open the mailstream for this folder.
       @return true on success
   */
   bool Open(void);

   /// half open the folder
   bool HalfOpen(void);

   /// physically create the file (MF_FILE or MF_MH) folder
   void CreateFileFolder();

   /// Close the folder
   virtual void Close(void);
   //@}

   /// called to notify everybody that its listing changed
   virtual void RequestUpdate(void);

   /// called to notify everybody that some messages were expunged
   void RequestUpdateAfterExpunge();

   virtual bool IsAlive(void) const;

   /** @name Message counting */
   //@{
   /// perform the search, return number of messages found
   unsigned long SearchAndCountResults(struct search_program *pgm) const;

   /// helper of SearchAndCountResults() and SearchByFlag()
   MsgnoArray *DoSearch(struct search_program *pgm) const;

   /// called by CountAllMessages() to perform actual counting
   virtual bool DoCountMessages(MailFolderStatus *status) const;
   //@}

   /// Update the timeout values from a profile
   void UpdateTimeoutValues(void);

   /// apply all timeout values
   void ApplyTimeoutValues(void);

   /// Check if this message is a "New Message":
   virtual bool IsNewMessage(const HeaderInfo * hi);

   /// helper of OverviewHeader
   static String ParseAddress(struct mail_address *adr);

   /**
     Called from GetHeaderInfo() to process one header

     @return false to abort overview generation, true to continue.
   */
   bool OverviewHeaderEntry(class OverviewData *overviewData,
                            struct message_cache *elt,
                            struct mail_envelope *env);

   /** We remember the last folder to enter a critical section, helps
       to find crashes.*/
   static String ms_LastCriticalFolder;

   /// Build the sequence string from the array of message uids
   static String BuildSequence(const UIdArray& messages);

   /** Ask the user for login/password if needed and return them. If we already
       have them in the corresponding variables, don't do anything but just
       return true.

       @param name the name of the folder
       @param type the type of the folder
       @param flags the flags of the folder
       @param login a non NULL pointer to the variable containing username
       @param password a non NULL pointer to the variable containing password
       @param didAsk a pointer (may be NULL) set to true if dlg was shown
       @return true if the password is either not needed or was entered
   */
   static bool AskPasswordIfNeeded(const String& name,
                                   FolderType type,
                                   int flags,
                                   String *login,
                                   String *password,
                                   bool *didAsk = NULL);

   /** Try to create folder if it hadn't been created yet, returns false only
       if it needed to be created and the creation failed
   */
   static bool CreateIfNeeded(Profile *profile);

   /// Updates the status of a single message.
   void UpdateMessageStatus(unsigned long seqno);

   /// Apply filters to the new messages
   virtual bool FilterNewMail();

   /// update the folder after appending messages to it
   void UpdateAfterAppend();

   virtual void ReadConfig(MailFolderCmn::MFCmnOptions& config);

   /** @name Notification handlers */
   //@{
   /// called when the number of the messages in the folder changes
   void OnMailExists(struct mail_stream *stream, MsgnoType msgnoMax);

   /// called when the given msgno is expunged from the folder
   void OnMailExpunge(MsgnoType msgno);
   //@}

   // members (mostly) from here on
   // -----------------------------

   /** @name Authentification info */
   //@{
   /// The following is also called by SendMessageCC for ESMTP authentication
   static void SetLoginData(const String &user, const String &pw);

   /// for POP/IMAP boxes, this holds the user id for the callback
   static String MF_user;
   /// for POP/IMAP boxes, this holds the password for the callback
   static String MF_pwd;
   //@}

   /** @name c-client initialization */
   //@{
   /// has c-client library been initialised?
   static bool ms_CClientInitialisedFlag;

   /// initialise c-client library
   static void CClientInit(void);
   //@}

   /** @name Mail folder parameters */
   //@{
   /// Full IMAP spec
   String m_ImapSpec;

   // TODO: m_Name and m_folderType/Flags should be replaced with an MFolder!

   /// The symbolic name of the folder (i.e. the name shown in the tree)
   String m_Name;

   /// Type of this folder
   FolderType m_folderType;

   /// folder flags
   int m_FolderFlags;

   /// the folder name delimiter, (char)-1 if unknown yet
   char m_chDelimiter;

   /// the profile we use for our options
   Profile *m_Profile;
   //@}

   /** @name Mail folder state */
   //@{
   /// mailstream associated with this folder
   MAILSTREAM *m_MailStream;

   /// the number of messages as we know it
   MsgnoType m_nMessages;

   /// last seen UID, all messages above this one are new
   UIdType m_LastUId;
   //@}

   /** @name Temporary operation parameters */
   //@{
   /// the array containing indices of expunged messages or NULL
   wxArrayInt *m_expungedIndices;

   /** If we are searching, this points to an UIdArray where to store
       the entries found.
   */
   UIdArray *m_SearchMessagesFound;

   /**@name only used for ListFolders: */
   //@{
   UserData m_UserData;
   Ticket   m_Ticket;
   class ASMailFolder *m_ASMailFolder;
   //@}

   /// Used by the subscription management.
   class FolderListingCC *m_FolderListing;
   //@}

   /** @name Locking and other semaphores */
   //@{
   /// Is this folder in a critical c-client section?
   bool m_InCritical;

   /// when this mutex is locked, this folder can't be accessed at all
   MMutex *m_Mutex;

   /// Locked while we're applying filter rules
   MMutex *m_InFilterCode;
   //@}

   /** @name functions for mapping mailstreams and objects
       These functions enable the class to map incoming events from
       the  c-client library to the object associated with that event.
       */
   //@{

   /// a pointer to the object to use as default if lookup fails
   static MailFolderCC *ms_StreamListDefaultObj;

   /// mapping MAILSTREAM* to objects of this class and their names
   static StreamConnectionList ms_StreamList;

   /// adds this object to Map
   void AddToMap(MAILSTREAM const *stream) const;

   /// remove this object from Map
   void RemoveFromMap(void) const;

   /// Gets first mailfolder in map or NULL.
   static MailFolderCC *GetFirstMapEntry(StreamConnectionList::iterator &i);

   /// Gets next mailfolder in map or NULL
   static MailFolderCC *GetNextMapEntry(StreamConnectionList::iterator &i);

   /// find the stream in the map
   static MailFolderCC *LookupStream(const MAILSTREAM *stream);

   /// lookup object in map first by stream, then by name
   static MailFolderCC *LookupObject(const MAILSTREAM *stream,
                                     const char *name = NULL);
   //@}

   /** @name c-client parameters */
   //@{
   /// IMAP lookahead value
   int m_LookAhead;
   /// TCP/IP open timeout in seconds.
   int m_TcpOpenTimeout;
   /// TCP/IP read timeout in seconds.
   int m_TcpReadTimeout;
   /// TCP/IP write timeout in seconds.
   int m_TcpWriteTimeout;
   /// TCP/IP close timeout in seconds.
   int m_TcpCloseTimeout;
   /// rsh connection timeout in seconds.
   int m_TcpRshTimeout;
   /// ssh connection timeout in seconds.
   int m_TcpSshTimeout;
   /// the path to rsh
   String m_RshPath;
   /// the path to ssh
   String m_SshPath;
   //@}

#ifdef DEBUG
   /// print a list of all streams
   static void DebugStreams(void);
#endif

   friend bool MailFolderCCInit();
   friend void MailFolderCCCleanup();

   friend class CCDefaultFolder;
   friend class SendMessageCC;

public:
   DEBUG_DEF
   MOBJECT_DEBUG(MailFolderCC)

public:
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
   static void mm_log(const String& str, long errflg, MailFolderCC *mf = NULL);

   /** log a debugging message
       @param str    message string
       */
   static void mm_dlog(const String& str);

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

   /// gets called when messages were deleted
   static void mm_expunged(MAILSTREAM *stream, unsigned long number);

   /// gets called when flags for a message have changed
   static void mm_flags(MAILSTREAM *stream, unsigned long number);
   //@}
};

#endif // MAILFOLDERCC_H

