/*-*- c++ -*-********************************************************
 * MailFolderCC class: handling of mail folders through C-Client lib*
 *                                                                  *
 * (C) 1997,1998 by Karsten Ballüder (karsten@phy.hw.ac.uk)         *
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
extern "C"
{
#     include <stdio.h>
#     include <mail.h>
#     include <osdep.h>
#     include <rfc822.h>
#     include <smtp.h>
#     include <nntp.h>
#     include <misc.h>
}
#endif

#include  "MailFolderCmn.h"
#include  "FolderView.h"
#include  "MFolder.h"

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

   /** opens a mail folder in a save way.
       @param name name of the folder
       @param profile profile to use
   */
   static MailFolderCC* OpenFolder(String const &name, ProfileBase *profile);

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

   */
   static MailFolderCC * OpenFolder(int typeAndFlags,
                                    String const &path,
                                    ProfileBase *profile,
                                    String const &server,
                                    String const &login,
                                    String const &password,
                                    String const &symname);

   //@}

   /// enable/disable debugging:
   void   DoDebug(bool flag = true) { debugFlag = flag; }

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

   /// Checks if the folder is in a critical section.
   inline bool InCritical(void) const { return m_InCritical; }

   /** Sets the symbolic name.
    */
   virtual void SetName(const String &name) { m_Name = name; };
   /** Sets the update interval in seconds and restarts the timer.
       @param secs seconds delay or 0 to disable
   */
   virtual void SetUpdateInterval(int secs);

      /// Get update interval in seconds
   virtual int GetUpdateInterval(void) const { return m_UpdateInterval; }

   /** Get number of messages which have a message status of value
       when combined with the mask. When mask = 0, return total
       message count.
       @param mask is a (combination of) MessageStatus value(s) or 0 to test against
       @param value the value of MessageStatus AND mask to be counted
       @return number of messages
   */
   virtual unsigned long CountMessages(int mask = 0, int value = 0) const;

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
   virtual bool SetFlag(const INTARRAY *sequence,
                        int flag,
                        bool set = true);

   /** Set flags on a messages. Possible flag values are MSG_STAT_xxx
       @param uid mesage uid
       @param flag flag to be set, e.g. "\\Deleted"
       @param set if true, set the flag, if false, clear it
       @return always true UNSUPPORTED!
   */
  virtual bool SetMessageFlag(unsigned long uid,
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

   /** Delete a message.
       @param uid mesage uid
       @return always true UNSUPPORTED!
   */
   bool DeleteMessage(unsigned long uid)
   {
     SetMessageFlag(uid,MSG_STAT_DELETED);
     return true;
   }

   /** UnDelete a message.
       @param uid mesage uid
       @return always true UNSUPPORTED!
   */
   bool UnDeleteMessage(unsigned long uid)
      { SetMessageFlag(uid,MSG_STAT_DELETED, false); return true; }

   /** Expunge messages.
     */
   void ExpungeMessages(void);

   /** Check whether mailbox has changed. */
   void Ping(void);

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

   /** Toggle sending of new mail events.
       @param send if true, send them
       @param update if true, update internal message count
   */
   virtual void EnableNewMailEvents(bool send = true, bool update = true)
      {
         m_GenerateNewMailEvents = send;
         m_UpdateMsgCount = update;
      }

   /** Query whether foldre is sending new mail events.
       @return if true, folder sends them
   */
   virtual bool SendsNewMailEvents(void) const
      { return m_GenerateNewMailEvents; }

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
       @param host the server host, or empty for local newsspool
       @param protocol MF_IMAP or MF_NNTP or MF_NEWS
       @param pattern a wildcard matching the folders to list
       @param subscribed_only if true, only the subscribed ones
       @param reference implementation dependend reference
    */
   static FolderListing *
   ListFolders(const String &host,
               FolderType protocol,
               const String &pattern = "*",
               bool subscribed_only = false,
               const String &reference = "");
   //@}
   /**@name Access control */
   //@{
   /** Locks a mailfolder for exclusive access. In multi-threaded mode 
       it really locks it and always returns true. In single-threaded
       mode it returns false if we cannot get a lock.
       @return TRUE if we have the lock
   */
   bool Lock(void) const;
   /** Releases the lock on the mailfolder. */
   void UnLock(void) const;
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

   /** Little helper function to convert iso8859 encoded header lines into
       8 bit. This is a quick fix until wxWindows supports unicode.
       Also used by MessageCC.
       @param in the string with some embedded iso8859 encoding
       @return the full 8bit decoded string
   */
   static String qprint(const String &in);

private:
   /// private constructor, does basic initialisation
   MailFolderCC(int typeAndFlags,
                String const &path,
                ProfileBase *profile,
                String const &server,
                String const &login,
                String const &password);

   /** Try to open the mailstream for this folder.
       @return true on success
   */
   bool   Open(void);

   /// for POP/IMAP boxes, this holds the user id for the callback
   static String MF_user;
   /// for POP/IMAP boxes, this holds the password for the callback
   static String MF_pwd;

   /// which type is this mailfolder?
   FolderType   m_folderType;

   ///   mailstream associated with this folder
   MAILSTREAM   *m_MailStream;

   /// a timer to update information
   class MailFolderTimer *m_Timer;
   /// Update interval for checking folder content
   int m_UpdateInterval;

   /// PingReopen() protection against recursion
   bool m_PingReopenSemaphore;
   /// BuildListing() protection against recursion
   bool m_BuildListingSemaphore;
   /// Do we need to update folder listing?
   bool m_UpdateNeeded;
   /// Request update
   void RequestUpdate(void);
   /// Do we need an update?
   bool UpdateNeeded(void) const { return m_UpdateNeeded; }
   /// number of messages in mailbox
   unsigned long m_NumOfMessages;
   /// last number of messages
   unsigned long m_OldNumOfMessages;
   /// set to true before we get the very first folder info
   bool m_FirstListing;
   /** Do we want to generate new mail events?
       Used to supporess new mail events when first opening the folder
       and when copying to it. */
   bool m_GenerateNewMailEvents;
   /** Do we want to update the message count? */
   bool m_UpdateMsgCount;
   /// Path to mailbox
   String   m_MailboxPath;

   /// The symbolic name of the folder
   String m_Name;
   /// do we want c-client's debug messages?
   bool   debugFlag;

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

   /// To display progress while reading message headers:
   class MProgressDialog *m_ProgressDialog;
   // return the folder type
   FolderType GetType(void) const { return m_folderType; }
protected:
   /// Update the timeout values from a profile
   void UpdateTimeoutValues(void);
   void SetType(FolderType type) { m_folderType = type; }

   /// Gets a complete folder listing from the stream.
   void BuildListing(void);
   /* Handles the mm_overview_header callback on a per folder basis. */
   void OverviewHeaderEntry (unsigned long uid, OVERVIEW *ov);
   /// closes the mailstream
   void Close(void);

   /// A Mutex to control access to this folder.
#ifdef USE_THREADS
   class MMutex *m_Mutex;
#else
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
      Event(MAILSTREAM *stream, EventType type)
         { m_stream = stream; m_type = type; }
      /// The type.
      EventType   m_type;
      /// The stream it relates to.
      MAILSTREAM *m_stream;
      /// The data structure, no more than three members needed
      EventArgument m_args[3];
   };
   KBLIST_DEFINE(EventQueue, Event);
   /**    Add an event to the queue, called from (C) mm_ callback
          routines.
          @param pointer to a new MailFolderCC::Event structure, will be freed by event handling mechanism.
   */
   static void QueueEvent(Event *evptr)
      { ms_EventQueue.push_back(evptr); }

protected:
   /// The list of events to be processed.
   static EventQueue ms_EventQueue;
   /** The index of the next entry in list to fill. Only used for
       BuildListing()/OverviewHeader() interaction. */
   unsigned long      m_BuildNextEntry;
   /// The maximum number of messages to retrive or 0
   unsigned long m_RetrievalLimit;
   //@}
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

   /* Handles the mm_overview_header callback on a per folder basis. */
   static void OverviewHeader (MAILSTREAM *stream, unsigned long uid, OVERVIEW *ov);

//@}
private:
   /// destructor
   ~MailFolderCC();

   /// The current listing of the folder
   class HeaderInfoListCC *m_Listing;
   /// Is this folder in a critical c-client section?
   bool m_InCritical;
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
public:
   DEBUG_DEF
   MOBJECT_DEBUG(MailFolderCC)
};

#endif
