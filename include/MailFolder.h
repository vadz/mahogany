//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   MailFolder.h: MailFolder class declaration
// Purpose:     MailFolder is the ABC defining the interface to mail folders
// Author:      Karsten Ballüder
// Modified by: Vadim Zeitlin at 24.01.01: complete rewrite of update logic
// Created:     1997
// CVS-ID:      $Id$
// Copyright:   (C) 1998-2000 by Karsten Ballüder (karsten@phy.hw.ac.uk)
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef MAILFOLDER_H
#define MAILFOLDER_H

#ifdef __GNUG__
#   pragma interface "MailFolder.h"
#endif

#include "MObject.h"
#include "FolderType.h"
#include "kbList.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

/// supported formats for the local file mailboxes (hence MH not counted)
enum FileMailboxFormat
{
   /// the default format, must be 0!
   FileMbox_MBX,
   /// traditional Unix one
   FileMbox_MBOX,
   /// SCO default format
   FileMbox_MMDF,
   /// MM-compatible fast format
   FileMbox_TNX,
   /// end of enum marker
   FileMbox_Max
};

/// how to show the message size in the viewer?
enum MessageSizeShow
{
   /// choose lines/bytes/kbytes/mbytes automatically
   MessageSize_Automatic,
   /// always show bytes/kbytes/mbytes
   MessageSize_AutoBytes,
   /// always show size in bytes
   MessageSize_Bytes,
   /// always show size in Kb
   MessageSize_KBytes,
   /// always show size in Mb
   MessageSize_MBytes,
   /// end of enum marker
   MessageSize_Max
};

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
typedef UIdArray MsgnoArray;

// this is unused so far
#if 0

/// UIdArray which maintains its items always sorted
WX_DEFINE_SORTED_ARRAY(UIdType, UIdArraySortedBase);

/// the function used for comparing UIDs
inline int CMPFUNC_CONV UIdCompareFunction(UIdType uid1, UIdType uid2)
{
   // an invalid UID is less than all the others
   return uid1 == UID_ILLEGAL ? -1
                              : uid2 == UID_ILLEGAL ? 1
                                                    : (long)(uid1 - uid2);
}

class UIdArraySorted : public UIdArraySortedBase
{
public:
   UIdArraySorted() : UIdArraySortedBase(UIdCompareFunction) { }
};

#endif // 0

// forward declarations
class ArrayHeaderInfo;
class FolderView;
class HeaderInfo;
class Profile;
class MFolder;
class MFrame;
class MWindow;
class Message;
class MessageView;
class Sequence;

struct SortParams;
struct ThreadData;
struct ThreadParams;

// ----------------------------------------------------------------------------
// MLogCircle
// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------
// MailFolderStatus
// ----------------------------------------------------------------------------

/**
  MailFolderStatus contains the "interesting" and often changing information
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
      unread =
      flagged =
      searched = 0;
   }

   bool IsValid() const { return total != UID_ILLEGAL; }

   // do we have any "interesting" messages at all?
   bool HasSomething() const
   {
      return newmsgs || recent || unread || flagged || searched;
   }

   bool operator==(const MailFolderStatus& status)
   {
      return memcmp(this, &status, sizeof(MailFolderStatus)) == 0;
   }

   // the total number of messages and the number of new, recent, unread,
   // important (== flagged) and searched (== results of search) messages in
   // this folder
   //
   // note that unread is the total number of unread messages, i.e. it
   // includes some which are just unseen and the others which are new (i.e.
   // unseen and recent)
   unsigned long total,
                 newmsgs,
                 recent,
                 unread,
                 flagged,
                 searched;
};

// ----------------------------------------------------------------------------
// MailFolder
// ----------------------------------------------------------------------------

/**
   MailFolder base class, represents anything containig mails.

   This class defines the interface for all MailFolder classes. It can
   represent anything which contains messages, e.g. folder files, POP3
   or IMAP connections or even newsgroups.
*/
class MailFolder : public MObjectRC
{
public:
   /** @name Constants and Types */
   //@{
   /** What is the status of a given message in the folder?
       Recent messages are those that we never saw in a listing
       before. Once we open a folder, the messages will no longer be
       recent when we close it. Seen are only those that we really
       looked at. */
   enum MessageStatus
   {
      /// message is new (recent and unseen) (NB: this not a real bit flag!)
      MSG_STAT_NEW = 0,
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

   /**
     Different ways to reply to a message

     NB: don't change the numeric values of the enum elements, they are
         stored in the profile and also because wxOptionsDlg relies on
         them being what they are!
    */
   enum ReplyKind
   {
      /// reply to the sender of this message only
      REPLY_SENDER,

      /// reply to all recipients of this message
      REPLY_ALL,

      /// reply to the mailing list this message was sent to
      REPLY_LIST,

      /// default reply: may go to the sender, all or list
      REPLY
   };

   /// flags for OpenFolder()
   enum OpenMode
   {
      Normal,
      ReadOnly,
      HalfOpen
   };

   /**
     The structure containing the parameters for Forward/ReplyMessage(s)
     methods.
    */
   struct Params
   {
      Params(ReplyKind rk = REPLY) { Init(rk); }
      Params(const String& t, ReplyKind rk = REPLY) : templ(t) { Init(rk); }

      void Init(ReplyKind rk) { replyKind = rk; msgview = NULL; }

      /// see ReplyKind enum above
      ReplyKind replyKind;

      /// the template to use for the new message, use default if empty
      String templ;

      /// msg viewer from which the reply/forward command originated
      MessageView *msgview;
   };
   //@}

   /** @name Static functions, implemented in MailFolder.cpp */
   //@{

   /** @name Opening folders */
   //@{
   /**
     Open the specified mail folder. May create the folder if it doesn't exist
     yet.

     @param mfolder the MFolder object identifying the folder to use
     @param openmode the mode for opening the folder, read/write by default
     @return the opened MailFolder object or NULL if opening failed
    */
   static MailFolder * OpenFolder(const MFolder *mfolder,
                                  OpenMode openmode = Normal);

   /**
     Half open the folder using paremeters from MFolder object. This is a
     simple wrapper around OpenFolder()
    */
   static MailFolder * HalfOpenFolder(const MFolder *mfolder)
      { return OpenFolder(mfolder, HalfOpen); }

   /**
      Closes the folder: this is always safe to call, even if this folder is
      not opened at all. OTOH, if it is opened, this function will always
      close it (i.e. break any connection to server).

      @param folder identifies the folder to be closed
      @return true if the folder was closed, false if it wasn't opened
    */
   static bool CloseFolder(const MFolder *mfolder);

   /**
      Closes all currently opened folders

      @return the number of folders closed, -1 on error
    */
   static int CloseAll();

   /**
     Call Ping() on all opened mailboxes.
    */
   static bool PingAllOpened(void);
   //@}

   /** Phyically deletes this folder.
       @return true on success
   */
   static bool DeleteFolder(const MFolder *mfolder);

   /**
     Rename the mailbox corresponding to this folder.

     @param mfolder the folder to rename
     @param name the new name for the mailbox
     @return true on success  
   */
   static bool Rename(const MFolder *mfolder, const String& name);

   /** Clear the folder, i.e. delete all messages in it

       @return the number of message deleted or -1 on error
   */
   static long ClearFolder(const MFolder *folder);

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

   /** Utility function to get a textual representation of a message
       status.

       @param message flags
       @return string representation
   */
   static String ConvertMessageStatusToString(int status);

   /**
     Returns the string containing the size as it should be shown to the user.

     @param sizeBytes the size of the message in bytes
     @param sizeLines the size of message in lines (only if text)
     @param show how should we show the size?
     @return string containing the text for display
    */
   static String SizeToString(unsigned long sizeBytes,
                              unsigned long sizeLines = 0,
                              MessageSizeShow show = MessageSize_Automatic,
                              bool verbose = false);

   /** Forward one message.
       @param message message to forward
       @param params is the Params struct to use
       @param profile environment
       @param parent window for dialog
   */
   static void ForwardMessage(Message *msg,
                              const Params& params,
                              Profile *profile = NULL,
                              MWindow *parent = NULL);

   /** Reply to one message.
       @param message message to reply to
       @param params is the Params struct to use
       @param profile environment
       @param parent window for dialog
   */
   static void ReplyMessage(Message *msg,
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

   /** @name Accessors */
   //@{
   /// is the folder opened
   virtual bool IsOpened(void) const = 0;

   /** Get name of mailbox.
       @return the symbolic name of the mailbox
   */
   virtual String GetName(void) const = 0;

   /// Return IMAP spec
   virtual String GetImapSpec(void) const = 0;

   /// Return the folder's type.
   virtual FolderType GetType(void) const = 0;

   /// return the folder flags
   virtual int GetFlags(void) const = 0;

   /** Get the profile used by this folder
       @return Pointer to the profile.
   */
   virtual Profile *GetProfile(void) const = 0;

   /// return class name
   const char *GetClassName(void) const
      { return "MailFolder"; }

   /**
      Returns the delimiter used to separate the components of the folder
      name

      @return character dependind on the folder type and server
    */
   virtual char GetFolderDelimiter() const;
   //@}

   /** Folder capabilities querying */
   //@{
   /// Does the folder need a working network to be accessed?
   virtual bool NeedsNetwork(void) const
   {
      return FolderNeedsNetwork(GetType(), GetFlags());
   }

   //@}

   /** @name Functions working with message headers */
   //@{
   /** Returns the object which is used to retrieve the headers of this
       folder.

       @return pointer to HeaderInfoList which must be DecRef()d by caller
    */
   virtual HeaderInfoList *GetHeaders(void) const = 0;

   /**
      Get the header info for the specified headers. This is for use of
      HeaderInfoList only!

      @param headers the array in which we read the headers
      @param seq sequence containing the msgnos of headers to retrieve
      @return the number of headers retrieved (may be less than requested if
              cancelled or an error occured)
    */
   virtual MsgnoType GetHeaderInfo(ArrayHeaderInfo& headers,
                                   const Sequence& seq) = 0;

   /**
      Get the total number of messages in the folder. This should be a fast
      operation unlike (possibly) CountXXXMessages() below.

      @return number of messages
    */
   virtual unsigned long GetMessageCount() const = 0;

   /// return true if the mailbox is empty
   bool IsEmpty() const { return GetMessageCount() == 0; }

   /// Count number of new messages (== RECENT && !SEEN)
   virtual unsigned long CountNewMessages(void) const = 0;

   /// Count number of recent messages.
   virtual unsigned long CountRecentMessages(void) const = 0;

   /// Count number of unread messages
   virtual unsigned long CountUnseenMessages(void) const = 0;

   /// Count number of deleted messages
   virtual unsigned long CountDeletedMessages(void) const = 0;

   /** Count the number of all interesting messages, i.e. any with non default
       flags (except for delete).

       @param status is the struct to fill with the folder info
       @return true if any interesting messages found, false otherwise
   */
   virtual bool CountAllMessages(MailFolderStatus *status) const = 0;

   /** Get the msgno corresponding to the given UID.

       NB: this is a slow function which may involve a round trip to server

       @return msgno for the given UID or MSGNO_ILLEGAL if not found
   */
   virtual MsgnoType GetMsgnoFromUID(UIdType uid) const = 0;
   //@}

   /** @name Operations on the folder */
   //@{
   /** Check whether mailbox has changed.
       @return FALSE on error, TRUE otherwise
   */
   virtual bool Ping(void) = 0;

   /** Perform a checkpoint on the folder. What this does depends on the server
       but, quoting from RFC 2060: " A checkpoint MAY take a non-instantaneous
       amount of real time to complete."
    */
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

   /** Set flag for all messages
    */
   virtual bool SetFlagForAll(int flag, bool set = true) = 0;

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
       @return true on success
   */
   virtual bool AppendMessage(const Message& msg) = 0;

   /** Appends the message to this folder.
       @param msg text of the  message to append
       @return true on success
   */
   virtual bool AppendMessage(const String& msg) = 0;

   /** Expunge messages.
    */
   virtual void ExpungeMessages(void) = 0;

   /** Find messages with given flags.

       @param flag one of MSG_STAT_xxx constants
       @param set if true, find all messages with this flag, false - without
       @param undeletedOnly if true, don't included deleted messages
       @param return array with msgnos (not UIDs!) of messages found to be
              freed by caller
   */
   virtual MsgnoArray *SearchByFlag(MessageStatus flag,
                                    bool set = true,
                                    bool undeletedOnly = true) const = 0;

   /** Search messages for certain criteria.
       @return UIdArray with UIds of matching messages, caller must
       free it
   */
   virtual UIdArray *SearchMessages(const class SearchCriterium *crit) = 0;

   //@}

   /**@name Some higher level functionality implemented by the
      MailFolder class on top of the other functions.
      These functions are not used by anything else in the MailFolder
      class and can easily be removed if needed.
   */
   //@{
   /** Save messages to a folder identified by MFolder

       @param selections pointer to an array holding the message UIDs
       @param folder is the folder to save to, can't be NULL
       @return true if messages got saved
   */
   virtual bool SaveMessages(const UIdArray *selections, MFolder *folder) = 0;

   /** Save the messages to a folder.
       @param selections the message UIDs
       @param folderName the name of the folder to save to
       @return true on success
   */
   virtual bool SaveMessages(const UIdArray *selections,
                             const String& folderName) = 0;

   /** Save the messages to a folder.
       @param selections the message UIDs
       @param filename the name of the folder to save to, ask user if empty
       @param parent the parent window for the file dialog
       @return true on success
   */
   virtual bool SaveMessagesToFile(const UIdArray *selections,
                                   const String& filename,
                                   MWindow *parent = NULL) = 0;

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

   /** Sort messages: returns the array containing the msgnos of the messages
       in sorted order.

       @param the array where to put the sorted msgnos (must be big enough!)
       @param sortParams indicates how to sort the messages
       @return true on success, false if msgs couldn't be sorted
    */
   virtual bool SortMessages(MsgnoType *msgnos,
                             const SortParams& sortParams) = 0;

   /** Thread messages: fill the tree strucure with the threading
       information.

       @param thrParams indicates how to thread the messages
       @param thrData  filled up with results
       @return true on success, false if msgs couldn't be threaded
    */
   virtual bool ThreadMessages(const ThreadParams& thrParams,
                               ThreadData *thrData) = 0;

   //@}

   /**@name Access control */
   //@{
   /** Locks a mailfolder for exclusive access. In multi-threaded mode
       it really locks it and always returns true. In single-threaded
       mode it returns false if we cannot get a lock.
       @return TRUE if we have obtained the lock
   */
   virtual bool Lock(void) const = 0;

   /** Releases the lock on the mailfolder.
    */
   virtual void UnLock(void) const = 0;

   /// Is folder locked?
   virtual bool IsLocked(void) const = 0;
   //@}

   /** Apply any filter rules to the folder.
       Applies the rule to all messages listed in msgs.
       @return -1 if no filter module exists, return code otherwise
   */
   virtual int ApplyFilterRules(UIdArray msgs) = 0;

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

   /** RFC 2047 compliant message decoding: all encoded words from the header
       are decoded, but only the encoding of the first of them is returned in
       encoding output parameter (if non NULL).

       @param in the RFC 2047 header string
       @param encoding the pointer to the charset of the string (may be NULL)
       @return the full 8bit decoded string
   */
   static String DecodeHeader(const String &in,
                              wxFontEncoding *encoding = NULL);

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
   virtual MFrame *SetInteractiveFrame(MFrame *frame) = 0;

   /**
      Get the frame to use for interactive messages. May return NULL.
   */
   virtual MFrame *GetInteractiveFrame() const = 0;

   /// Returns the shared log circle object used for error analysis
   static MLogCircle& GetLogCircle(void) { return ms_LogCircle; }
   //@}

   /** @name Update control

       The folder view(s) updating can be suspended temporarily which is
       useful for batch operations. Resume must be called the same number
       of times as Suspend()!
    */
   //@{
   /** Request update: sends an event telling everyone that the messages in the
       mail folder have changed, i.e. either we have new messages or some
       messages were deleted.
   */
   virtual void RequestUpdate() = 0;

   /// Suspend folder updates (call ResumeUpdates() soon!)
   virtual void SuspendUpdates() = 0;

   /// Resume updates (updates everybody if suspend count becomes 0)
   virtual void ResumeUpdates() = 0;
   //@}

protected:
   /// Close the folder
   virtual void Close(void) = 0;

   /// the helper class for determining the exact error msg from cclient log
   static MLogCircle ms_LogCircle;

   /// the folder for which we had set default interactive frame
   static String ms_interactiveFolder;

   /// the frame to which interactive messages for ms_interactiveFolder go
   static MFrame *ms_interactiveFrame;

private:
   /// this is the only class which can call our GetHeaderInfo()
   friend class HeaderInfoList;
};

// MailFolder_obj is a smart reference to MailFolder
DECLARE_AUTOPTR_WITH_CONVERSION(MailFolder);

// ----------------------------------------------------------------------------
// MFInteractiveLock: sets the interactive frame to the one provided for the
// life time of this object
// ----------------------------------------------------------------------------

class MFInteractiveLock
{
public:
   // we will DecRef() the mail folder which must be !NULL - but the frame may
   // be NULL, although it's probably not very useful
   MFInteractiveLock(MailFolder *mf, MFrame *frame)
   {
      m_mf = mf;
      if ( m_mf )
      {
         m_frameOld = mf->SetInteractiveFrame(frame);
      }
      else
      {
         FAIL_MSG( "NULL folder in MFInteractiveLock" );
      }
   }

   ~MFInteractiveLock()
   {
      if ( m_mf )
      {
         (void)m_mf->SetInteractiveFrame(m_frameOld);

         m_mf->DecRef();
      }
   }

private:
   MailFolder *m_mf;
   MFrame *m_frameOld;
};

// ----------------------------------------------------------------------------
// Various helper classes
// ----------------------------------------------------------------------------

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
extern String FormatFolderStatusString(const String& format,
                                       const String& folderName,
                                       MailFolderStatus *status,
                                       const MailFolder *mf = NULL);

// get the sequence string for these messages
extern String GetSequenceString(const UIdArray *messages);

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

