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
#  include "Mcclient.h"
#endif // USE_PCH

#include "MThread.h"

#include "MailFolderCmn.h"

#include <wx/fontenc.h>    // for wxFontEncoding

// fwd decls
class ASMailFolder;
class MailFolderCC;

// ----------------------------------------------------------------------------
// helper classes
// ----------------------------------------------------------------------------

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
   static MailFolder *OpenFolder(const MFolder *mfolder,
                                 const String& login,
                                 const String& password,
                                 OpenMode openmode,
                                 wxFrame *frame);

   //@}

   /** Phyically deletes this folder.
       @return true on success
   */
   static bool DeleteFolder(const MFolder *mfolder);

   static bool Rename(const MFolder *mfolder, const String& name);

   static long ClearFolder(const MFolder *folder);

   static bool CheckStatus(const MFolder *folder);

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

   virtual bool IsReadOnly(void) const;
   virtual bool CanSetFlag(int flags) const;

   /** return the full folder name
       @return the folder's name
   */
   virtual String GetName(void) const;

   /// return the folder type
   virtual MFolderType GetType(void) const;

   /// return the folder flags
   virtual int GetFlags(void) const;

   /// Return IMAP spec
   virtual String GetImapSpec(void) const { return m_ImapSpec; }

   /// return full IMAP spec including the login name
   static String GetFullImapSpec(const MFolder *folder, const String& login);

   /** Get the profile.
       @return Pointer to the profile.
   */
   virtual Profile *GetProfile(void) const { return m_Profile; }

   /// Checks if the folder is in a critical section.
   virtual bool IsInCriticalSection(void) const { return m_InCritical; }

   virtual ServerInfoEntry *CreateServerInfo(const MFolder *folder) const;

   /// @name Folder operations
   //@{

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
   virtual Message *GetMessage(unsigned long uid) const;

   virtual bool SetMessageFlag(unsigned long uid,
                               int flag,
                               bool set = true);

   /** Set flags on a sequence of messages. Possible flag values are MSG_STAT_xxx
       @param sequence the IMAP sequence
       @param flag flag to be set, e.g. "\\Deleted"
       @param set if true, set the flag, if false, clear it
       @return always true UNSUPPORTED!
   */
   virtual bool SetSequenceFlag(SequenceKind kind,
                                const Sequence& sequence,
                                int flag,
                                bool set = true);

   /// override base class SaveMessages() to do server side copy if possible
   virtual bool SaveMessages(const UIdArray *selections, MFolder *folder);

   virtual bool AppendMessage(const Message & msg);

   virtual bool AppendMessage(const String& msg);
   virtual void ExpungeMessages(void);


   virtual MsgnoArray *SearchByFlag(MessageStatus flag,
                                    int flags = SEARCH_SET |
                                                SEARCH_UNDELETED,
                                    MsgnoType last = 0) const;

   virtual UIdArray *SearchMessages(const SearchCriterium *crit, int flags);


   virtual bool ThreadMessages(const ThreadParams& thrParams,
                               ThreadData *thrData);

   virtual bool SortMessages(MsgnoType *msgnos, const SortParams& sortParams);


   /** Check whether mailbox has changed.
       @return FALSE on error
   */
   virtual bool Ping(void);

   /** Perform a checkpoint on the folder. */
   virtual void Checkpoint(void);

   //@}

   /** Get a listing of all mailboxes.

       DO NOT USE THIS FUNCTION, BUT ASMailFolder::ListFolders instead!!!

       @param asmf the ASMailFolder initiating the request
       @param pattern a wildcard matching the folders to list
       @param subscribed_only if true, only the subscribed ones
       @param reference implementation dependend reference
    */
   void ListFolders(class ASMailFolder *asmf,
                    const String &pattern = _T("*"),
                    bool subscribed_only = false,
                    const String &reference = _T(""),
                    UserData ud = 0,
                    Ticket ticket = ILLEGAL_TICKET);

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

   virtual MsgnoType GetHeaderInfo(ArrayHeaderInfo& headers,
                                   const Sequence& seq);
   //@}

   virtual char GetFolderDelimiter() const;

   /// return TRUE if CClient lib had been initialized
   static bool IsInitialized() { return ms_CClientInitialisedFlag; }

   // unused for now
#if 0
   /**
      check whether a folder can have subfolders
   */
   static bool HasInferiors(const String &imapSpec,
                            const String &user,
                            const String &pw);
#endif // 0

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

   /**
     Methods for use by CCEventReflector only: see the comment in
     MailFolderCC.cpp for more details.
   */
   //@{

   /// called to process new mail in the folder (filter it, ...)
   void OnNewMail();

   /// called to generate MsgStatus event for many messages at once
   void OnMsgStatusChanged();

   //@}

private:
   /** @name Constructors and such */
   //@{
   /// private constructor, does basic initialisation
   MailFolderCC(const MFolder *mfolder, wxFrame *frame);

   /// Common code for constructors
   void Create(MFolderType type, int flags);

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
   bool Open(OpenMode openmode = Normal);

   /// physically create the file (MF_FILE or MF_MH) folder
   void CreateFileFolder();

   /// check (and delete if requested) for the lock on this file folder
   bool CheckForFileLock();

   /// Close the folder
   virtual void Close(bool mayLinger = true);

   //@}

   /** @name Message counting */
   //@{
   /// perform the search, return number of messages found
   unsigned long SearchAndCountResults(struct search_program *pgm) const;

   /**
     common part of the various Search() functions: calls c-client
     mail_search()

     @param pgm the search program
     @param flags either SEARCH_UID or SEARCH_MSGNO
     @return array containing either UIDs or msgnos of the found messages
   */
   MsgnoArray *DoSearch(struct search_program *pgm,
                        int flags = SEARCH_MSGNO) const;

   /// called by CountAllMessages() to perform actual counting
   virtual bool DoCountMessages(MailFolderStatus *status) const;
   //@}

   /// Update the timeout values from a profile
   void UpdateTimeoutValues(void);

   /// apply all timeout values
   void ApplyTimeoutValues(void);

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

   /**
      Try to create folder if it hadn't been created yet, returns true if the
      folder could be created and opened successfully or false if the folder
      had to be created but its creation failed.

      @param folder to create/open
      @param pStream will be filled with a valid stream if not NULL and
                     if we indeed created the folder (and will be NULL if we
                     didn't do anything)
      @return false if the folder couldn't be created
   */
   static bool CreateIfNeeded(const MFolder *folder,
                              MAILSTREAM **pStream = NULL);

   /// just do mail_ping() on the opened folder, return TRUE if ok
   bool PingOpenedFolder();

   /**
     Called when the folder was closed not in result of calling our Close():
     this will reset m_MailStream to NIL, call Close() to clean up and notyf
     the outside world about our closure
   */
   void ForceClose();

   /// Updates the folder status after some messages were expunged
   void UpdateMsgFlagsOnExpunge(MsgnoType msgnoExpunged);

   /// Updates the status of a single message.
   void UpdateMessageStatus(MsgnoType msgno, int statusNew);

   /// update the folder after appending messages to it
   void UpdateAfterAppend();

   virtual void ReadConfig(MailFolderCmn::MFCmnOptions& config);

   /// fill mailstatus with the results of c-client mail_status() call
   static bool DoCheckStatus(const MFolder *folder,
                             struct mbx_status *mailstatus);

   /**
      @name Notification handlers

      These handles are called from inside c-client and so no other calls to
      c-client should be made from them. Normally they should just quickly
      update the internal state and maybe post an event to ourselves which will
      be processed (thanks to CCEventReflector) by OnXXX() methods.
    */
   //@{

   /// called when the number of the messages in the folder changes
   void HandleMailExists(struct mail_stream *stream, MsgnoType msgnoMax);

   /// called when the given msgno is expunged from the folder
   void HandleMailExpunge(MsgnoType msgno);

   /// called when the flags of the given message change
   void HandleMsgFlags(MsgnoType msgno);

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

   /// do we have login/password?
   bool HasLogin() const { return !m_login.empty() && !m_password.empty(); }

   /// do we need to authentificate?
   bool NeedsAuthInfo() const;

   /// set login data (possibly asking the user about it) if needed, return
   /// false if we don't have login/password and so can't continue
   static bool SetLoginDataIfNeeded(const MFolder *mfolder,
                                    String *login = NULL);

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

   /// the object containing all our parameters
   MFolder *m_mfolder;

   /// Full folder spec for c-client
   String m_ImapSpec;

   /// the folder name delimiter, (char)-1 if unknown yet
   char m_chDelimiter;

   /// the profile we use for our options
   Profile *m_Profile;

   /**
     Note that login/password may be different from m_mfolder data (i.e. what
     GetLogin()/Password() return) if the user had chosen to not store them in
     the config but enters them interactively.
    */

   /// username to use for opening this folder
   String m_login;

   /// password to use for opening this folder
   String m_password;

   //@}

   /** @name Mail folder state */
   //@{

   /// mailstream associated with this folder
   MAILSTREAM *m_MailStream;

   /// the number of messages as we know it
   MsgnoType m_nMessages;

   /// last seen UID
   UIdType m_uidLast;

   /// last seen UID of a new message
   UIdType m_uidLastNew;

   /// UID validity (in IMAP/c-client sense) for this folder
   UIdType m_uidValidity;

   //@}

   /** @name Temporary operation parameters */
   //@{

   /**
      If we are searching, this points to an UIdArray where to store
      the entries found.
   */
   UIdArray *m_SearchMessagesFound;

   /**
      Data used by ListFolders().
    */

   // TODO: put all this into a single struct
   UserData m_UserData;
   Ticket   m_Ticket;
   ASMailFolder *m_ASMailFolder;

   //@}

   /** @name Locking and other semaphores */
   //@{

   /// Is this folder in a critical c-client section?
   bool m_InCritical;

   /// when this mutex is locked, this folder can't be accessed at all
   MMutex m_mutexExclLock;

   /// locked while we're processing the new mail which just arrived
   MMutex m_mutexNewMail;

   //@}

   /** @name functions for mapping mailstreams and objects
       These functions enable the class to map incoming events from
       the  c-client library to the object associated with that event.
       */
   //@{

   /// a pointer to the object to use as default if lookup fails
   static MailFolderCC *ms_StreamListDefaultObj;

   /// lookup object in map by its stream
   static MailFolderCC *LookupObject(const MAILSTREAM *stream);

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

