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
#   include   "MailFolder.h"

// includes for c-client library
extern "C"
{
#      include   <mail.h>
}
#endif

struct __docxxfix;

// fwd decl needed to define StreamListType before MailFolderCC
// (can't be defined inside the class - VC++ 5.0 can't compile it)
class MailFolderCC;

/// structure to hold MailFolder pointer and associated mailstream pointer
struct StreamConnection
{
   /// pointer to a MailFolderCC object
   MailFolderCC    *folder;
   /// reference counter
   int refcount;
   /// pointer to the associated MAILSTREAM
   MAILSTREAM   const *stream;
   /// name of the MailFolderCC object
   String name;
};

KBLIST_DEFINE(StreamConnectionList, StreamConnection);
KBLIST_DEFINE(FolderViewList, FolderViewBase);

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
   */
   static MailFolderCC* OpenFolder(String const &name);
   
   /** closes a mail folder in a save way.
          */
   void Close(void);

   /// default destructor
   ~MailFolderCC();

   /// assume object to only be initialised when stream is ok
   bool IsInitialised(void) const { return okFlag; }

   //@}

   /// enable/disable debugging:
   void   DoDebug(bool flag = true) { debugFlag = flag; }

   CB_DECLARE_CLASS(MailFolderCC, MailFolder);

protected:
   /** open an existing mailbox:
       @param   filename the name of the "file" to open
       */
   bool   Open(String const & filename);

   /** creates an object representing a folder
       @param name name of the folder (relative name!)
   */
   MailFolderCC(String const & name);

   /** creation of the object
       @param name name of the folder
   */
   void   Create(String const & name);
      
public:

   /** Is mailbox ok to use? Did the last operation succeed?
       @return true if everything is fine
   */
   bool   IsOk(void) const { return okFlag; }
   
   /** Register a FolderViewBase derived class to be notified when
       folder contents change.
       @param    view the FolderView to register
       @param   reg if false, unregister it
   */
   void RegisterView(FolderViewBase *view, bool reg = true);

   /** return a symbolic name for mail folder
       @return the folder's name
   */
   String const & GetName(void) const;

   /** get number of messages
       @return number of messages
   */
   long      CountMessages(void) const;

   /** get message header
       @param msgno sequence nubmer of message
       @return message header information class
   */
   class Message *GetMessage(unsigned long msgno);

   /** Get status of message.
       @param    msgno   sequence no of message
       @param size if not NULL, size in bytes gets stored here
       @param day to store day (1..31)
       @param month to store month (1..12)
       @param year to store year (19xx)
       @return flags of message
   */
   int   GetMessageStatus(
      unsigned int msgno,
      unsigned long *size = NULL,
      unsigned int *day = NULL,
      unsigned int *month = NULL,
      unsigned int *year = NULL);

   /** Set a message flag
       @param index the sequence number
       @param flag flag to be set, e.g. MSG_STAT_DELETED
       @param set if true, set the flag, if false, clear it
   */
   void SetMessageFlag(unsigned long index, int flag, bool set = true);

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
   
   /** Updates the associated FolderViews */
   void UpdateViews(void);
   
   /// which type is this mailfolder?
   enum FolderType { MF_INBOX = 0, MF_FILE = 1, MF_POP = 2, MF_IMAP =
                     3, MF_NNTP = 4 };
   
private:
   /// for POP/IMAP boxes, this holds the user id for the callback
   static String MF_user;
   /// for POP/IMAP boxes, this holds the password for the callback
   static String MF_pwd;
   
   /// a list of FolderViews to be notified when this folder changes
   FolderViewList   viewList;
   
   /// which type is this mailfolder?
   FolderType   folderType;
   ///   mailstream associated with this folder
   MAILSTREAM   *mailstream;

   /// is the MAILSTREAM ok or was there an error?
   bool      okFlag;
   
   /// number of messages in mailbox
   long      numOfMessages;

   /// name of mailbox
   String   symbolicName;

   /// filename of mailbox or name describing IMAP/POP3 connection
   String   realName;

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
   static MailFolderCC *LookupObject(MAILSTREAM const *stream);
   //@}
   /** for use by class MessageCC
       @return MAILSTREAM of the folder
   */
   inline MAILSTREAM   *Stream(void) const{  return mailstream; }
   friend class MessageCC;
protected:
   void SetType(FolderType type) { folderType = type; }
   FolderType GetType(void) { return folderType; }
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
   static void mm_notify(MAILSTREAM *stream, char *str, long
                         errflg);
   
   /** this mailbox name matches a listing request
       @param stream mailstream
       @param delim   character that separates hierarchies
       @param name    mailbox name
       @param attrib   mailbox attributes
       */
   static void mm_list(MAILSTREAM *stream, char delim, char *name,
                       long attrib);

   /** matches a subscribed mailbox listing request
       @param stream   mailstream
       @param delim   character that separates hierarchies
       @param name   mailbox name
       @param attrib   mailbox attributes
       */
   static void mm_lsub(MAILSTREAM *stream, char delim, char *name,
                       long attrib);
   /** status of mailbox has changed
       @param stream   mailstream
       @param mailbox    mailbox name for this status
       @param status   structure with new mailbox status
       */
   static void mm_status(MAILSTREAM *stream, char *mailbox, MAILSTATUS *status);

   /** log a message
       @param str   message string
       @param errflg   error level
       */
   static void mm_log(const char *str, long errflg);

   /** log a debugging message
       @param str    message string
       */
   static void mm_dlog(const char *str);

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

   void AppendMessage(const char *msg);
   //@}
   
public:
   DEBUG_DEF
};

/* FIXME: are these actully used ???? */

class MailFolderPopCC : public MailFolderCC
{
private:
   String   popHost;
   String   popLogin;
   String   popPassword;
   
protected:
   MailFolderPopCC(String const & name);
   void Create(String const &name);
   bool Open(void);
   ~MailFolderPopCC() {};
};

class MailFolderIMAPCC : public MailFolder
{
protected:
   MailFolderIMAPCC();
   ~MailFolderIMAPCC() {};
};

class MailFolderFileCC : public MailFolder
{
protected:
   MailFolderFileCC();
   ~MailFolderFileCC() {};
};

class MailFolderINBOXCC : public MailFolder
{
protected:
   MailFolderINBOXCC();
   ~MailFolderINBOXCC() {};
};

#endif
