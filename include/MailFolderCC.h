/*-*- c++ -*-********************************************************
 * MailFolderCC class: handling of mail folders through C-Client lib*
 *                                                                  *
 * (C) 1997,1998 by Karsten Ballüder (Ballueder@usa.net)            *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/

#ifndef MAILFOLDERCC_H
#define MAILFOLDERCC_H

#ifdef __GNUG__
#   pragma interface "MailFolderCC.h"
#endif

#ifndef   USE_PCH
#   include  "kbList.h"

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

#include  "MailFolder.h"
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
class MailFolderCC : public MailFolder
{
//   DECLARE_CLASS(MailFolderCC)
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
                                    String const &password);

   /// assume object to only be initialised when stream is ok
   bool IsInitialised(void) const { return okFlag; }

   //@}

   /// enable/disable debugging:
   void   DoDebug(bool flag = true) { debugFlag = flag; }

   // checks whether a folder with that path exists
   static MailFolderCC *FindFolder(String const &path,
                                   String const &login);

public:

   /** Is mailbox ok to use? Did the last operation succeed?
       @return true if everything is fine
   */
   bool   IsOk(void) const { return okFlag; }

   /** Register a FolderView derived class to be notified when
       folder contents change.
       @param    view the FolderView to register
       @param   reg if false, unregister it
   */
   void RegisterView(FolderView *view, bool reg = true);

   /** return a symbolic name for mail folder
       @return the folder's name
   */
   String GetName(void) const;

   /** get number of messages
       @return number of messages
   */
   long      CountMessages(void) const;

   /** get message header
       @param msgno sequence nubmer of message
       @return message header information class
   */
   class Message *GetMessage(unsigned long msgno);

   /** Set flags on a sequence of messages. Possible flag values are MSG_STAT_xxx
       @param sequence the IMAP sequence
       @param flag flag to be set, e.g. "\\Deleted"
       @param set if true, set the flag, if false, clear it
   */
   virtual void SetSequenceFlag(String const &sequence,
                                int flag,
                                bool set = true);

  /** Set flags on a messages. Possible flag values are MSG_STAT_xxx
       @param sequence number of the message
       @param flag flag to be set, e.g. "\\Deleted"
       @param set if true, set the flag, if false, clear it
   */
  void SetMessageFlag(unsigned long msgno,
                      int flag,
                      bool set = true);
   
   /** Appends the message to this folder.
       @param msg the message to append
   */
   virtual void AppendMessage(Message const &msg);

   /** Appends the message to this folder.
       @param msg text of the  message to append
   */
   virtual void AppendMessage(String const &msg);

   /** Delete a message.
       @param index the sequence number
   */
   void DeleteMessage(unsigned long index) { SetMessageFlag(index, MSG_STAT_DELETED); }

   /** UnDelete a message.
       @param index the sequence number
   */
   void UnDeleteMessage(unsigned long index) { SetMessageFlag(index,MSG_STAT_DELETED, false); }

   /** Expunge messages.
     */
   void ExpungeMessages(void);

   /** Check whether mailbox has changed. */
   void Ping(void);

   /** Tells the folder that n messages are now in it.
       Called from mm_exists callback.
       
       @param n the number of messages in the folder
   */
   void UpdateCount(long n);
   
   /** Updates the associated FolderViews */
   void UpdateViews(void);

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

   /// a list of FolderViews to be notified when this folder changes
   FolderViewList   m_viewList;

   /// which type is this mailfolder?
   FolderType   m_folderType;

   ///   mailstream associated with this folder
   MAILSTREAM   *m_MailStream;

   /// is the MAILSTREAM ok or was there an error?
   bool      okFlag;

   /// number of messages in mailbox
   long      numOfMessages;

   /// Path to mailbox
   String   m_MailboxPath;

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
   void   AddToMap(MAILSTREAM const *stream);

   /// remove this object from Map
   void   RemoveFromMap(MAILSTREAM const *stream);

   /** set the default object in Map
       @param setit if false, erase default object
   */

   void SetDefaultObj(bool setit = true);

   /// lookup object in Map
   static MailFolderCC *LookupObject(MAILSTREAM const *stream,
                                     const char *name = NULL);
   //@}
   /** for use by class MessageCC
       @return MAILSTREAM of the folder
   */
   inline MAILSTREAM   *Stream(void) const{  return m_MailStream; }
   friend class MessageCC;

protected:
   void SetType(FolderType type) { m_folderType = type; }
   FolderType GetType(void) const { return m_folderType; }

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
      LSub, Status, Log, DLog
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
   /// Add an event to the queue, called from (C) mm_ callback routines.
   static void QueueEvent(Event *evptr)
          { ms_EventQueue.push_back(evptr); }
protected:
   /// The list of events to be processed.
   static EventQueue ms_EventQueue;
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

   /// this message has been expunged, subsequent numbers changed
   static void mm_expunged(MAILSTREAM *stream, unsigned long number);

   /// flags have changed for a message
   static void mm_flags(MAILSTREAM *stream, unsigned long number);

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
       */
   static void mm_log(String str, long errflg);

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

   //@}
private:
   /// destructor
   ~MailFolderCC();

public:
   DEBUG_DEF
};

#endif
