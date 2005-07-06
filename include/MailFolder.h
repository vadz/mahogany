//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   MailFolder.h: MailFolder class declaration
// Purpose:     MailFolder is the ABC defining the interface to mail folders
// Author:      Karsten Ballüder
// Modified by: Vadim Zeitlin at 24.01.01: complete rewrite of update logic
// Created:     1997
// CVS-ID:      $Id$
// Copyright:   (C) 1997-2002 Mahogany Team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _MAILFOLDER_H
#define _MAILFOLDER_H

#ifdef __GNUG__
#   pragma interface "MailFolder.h"
#endif

#ifndef USE_PCH
#  include "FolderType.h"
#endif // USE_PCH

#include "MObject.h"

#include "FolderType.h"         // for MFolderType
#include <wx/fontenc.h>         // for wxFontEncoding

// forward declarations
class ArrayHeaderInfo;
class Composer;
class FolderView;
class HeaderInfo;
class HeaderInfoList;
class Message;
class MessageView;
class MFolder;
class MLogCircle;
class Profile;
class Sequence;
class ServerInfoEntry;
class UIdArray;

struct MailFolderStatus;
struct SearchCriterium;
struct SortParams;
struct ThreadData;
struct ThreadParams;

class WXDLLEXPORT wxFrame;
class WXDLLEXPORT wxWindow;

#ifndef MsgnoArray
   #define MsgnoArray UIdArray
#endif

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

      /// follow-up to newsgroup
      FOLLOWUP_TO_NEWSGROUP,

      /// default reply: may go to the sender, all or list
      REPLY
   };

   /// flags for OpenFolder()
   enum OpenMode
   {
      /// open the folder normally
      Normal,

      /// open the folder in readonly mode
      ReadOnly,

      /// only connect to the server, don't really open it
      HalfOpen
   };

   /// flags for SetSequenceFlag()
   enum SequenceKind
   {
      /// the sequence contains the UIDs
      SEQ_UID,

      /// the sequence contains the msgnos
      SEQ_MSGNO
   };

   /// flags for SearchByFlag()
   enum
   {
      /// return the msgnos, not UIDs of the messages (default)
      SEARCH_MSGNO = 0,

      /// search messages with the given flag set (default)
      SEARCH_SET = 0,

      /// search messages without the given flag
      SEARCH_UNSET = 1,

      /// search among undeleted messages only
      SEARCH_UNDELETED = 2,

      /// return the UIDs, not msgnos, of the found messages
      SEARCH_UID = 4
   };

   /// flags for DeleteOrTrashMessages
   enum
   {
      /// use the trash setting of the folder (default)
      DELETE_ALLOW_TRASH = 0,

      /**
         don't move the messages to trash, even if the folder is configured to
         use it, always delete them
       */
      DELETE_NO_TRASH = 1
   };

   /// flags for DeleteMessages
   enum
   {
      /// don't expunge after deleting the messages, this is the default
      DELETE_NO_EXPUNGE = 0,

      /// expunge the messages after deleting them
      DELETE_EXPUNGE = 1
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

      /**
        msg viewer from which the reply/forward command originated: this is
        used to get the text to be quoted, if it is NULL no quoting is to be
        done
      */
      const MessageView *msgview;
   };

   //@}

   /** @name Opening folders */
   //@{
   /**
     Open the specified mail folder. May create the folder if it doesn't exist
     yet.

     @param mfolder the MFolder object identifying the folder to use
     @param openmode the mode for opening the folder, read/write by default
     @param frame the interactive frame for the folder or NULL
     @return the opened MailFolder object or NULL if opening failed
    */
   static MailFolder * OpenFolder(const MFolder *mfolder,
                                  OpenMode openmode = Normal,
                                  wxFrame *frame = NULL);

   /**
     Half open the folder using paremeters from MFolder object. This is a
     simple wrapper around OpenFolder()
    */
   static MailFolder * HalfOpenFolder(const MFolder *mfolder,
                                      wxFrame *frame = NULL)
      { return OpenFolder(mfolder, HalfOpen, frame); }

   /**
      Closes the folder: this is always safe to call, even if this folder is
      not opened at all. OTOH, if it is opened, this function will always
      close it (i.e. break any connection to server).

      @param folder identifies the folder to be closed
      @param mayLinger if true (default), the network connect may be kept
      @return true if the folder was closed, false if it wasn't opened
    */
   static bool CloseFolder(const MFolder *mfolder, bool mayLinger = true);

   /**
     Check the folder status without opening it (if possible).

     @param frame if not NULL, some feedback is given
     @return true if ok, false if an error occured
    */
   static bool CheckFolder(const MFolder *mfolder, wxFrame *frame = NULL);

   //@}

   /** @name Operations on all folders at once */
   //@{

   /**
     Return the currently opened mailfolder for the given MFolder or NULL if it
     is not opened.

     @return the opened mailfolder or NULL
    */
   static MailFolder *GetOpenedFolderFor(const MFolder *folder);

   /**
      Closes all currently opened folders

      @return the number of folders closed, -1 on error
    */
   static int CloseAll();

   /**
     Call Ping() on all opened mailboxes.

     @param frame if not NULL, some feedback is given
     @return true if ok, false if an error occured
    */
   static bool PingAllOpened(wxFrame *frame = NULL);

   //@}

   /**
     @name Miscellaneous static methods
    */
   //@{

   /** Phyically deletes this folder and all of the messages in it.
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

   /** Checks if it is OK to exit the application now.
       @param which Will either be set to empty or a '\n' delimited
       list of folders which are in critical sections.
   */
   static bool CanExit(String *which);

   /**
     Create a new file containing the message data in MBOX format.

     If the file already exists, message is appended to it.

     @param filename the name of the file to create or append to
     @param contents the pointer to message contents, non NULL
     @param len its length
     @return true if everything went ok, false on error
    */
   static bool SaveMessageAsMBOX(const String& filename,
                                 const void *contents,
                                 size_t len);

   /**
      Same as above but from a String.

      Do not create a String just to call this function if you have a C
      pointer, call the overload above directly as this avoids an unnecessary
      copy of potentially big amounts of data!
    */
   static bool SaveMessageAsMBOX(const String& filename, const String& s)
   {
      return SaveMessageAsMBOX(filename, s.c_str(), s.length());
   }

   //@}

   /**
     @name Replying/forwarding to the messages
    */
   //@{

   /** Forward one message.
       @param message message to forward
       @param params is the Params struct to use
       @param profile environment
       @param parent window for dialog
       @param composer the composer to use, will create if NULL
       @return the composer
   */
   static Composer *ForwardMessage(Message *msg,
                                   const Params& params,
                                   Profile *profile = NULL,
                                   wxWindow *parent = NULL,
                                   Composer *composer = NULL);

   /** Reply to one message.
       @param message message to reply to
       @param params is the Params struct to use
       @param profile environment
       @param parent window for dialog
       @param composer the composer to use, will create if NULL
       @return the composer
   */
   static Composer *ReplyMessage(Message *msg,
                                 const Params& params,
                                 Profile *profile = NULL,
                                 wxWindow *parent = NULL,
                                 Composer *composer = NULL);

   //@}

   /**
     @name Mailbox management
    */
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
                         MFolderType protocol,
                         const String &mailboxname,
                         bool subscribe = true);

   /** Get a listing of all mailboxes.

       DO NOT USE THIS FUNCTION, BUT ASMailFolder::ListFolders instead!

       This function is not reentrant and not MT-safe, it needs to be fixed to
       be both.

       @param asmf the ASMailFolder initiating the request
       @param pattern a wildcard matching the folders to list
       @param subscribed_only if true, only the subscribed ones
       @param reference implementation dependend reference
    */
   virtual void ListFolders(class ASMailFolder *asmf,
                            const String &pattern = _T("*"),
                            bool subscribed_only = false,
                            const String &reference = _T(""),
                            UserData ud = 0,
                            Ticket ticket = ILLEGAL_TICKET) = 0;

   //@}


   /** @name Accessors */
   //@{

   /// is the folder opened
   virtual bool IsOpened(void) const = 0;

   /**
      Even a folder opened without ReadOnly flag could be opened in read only
      mode if this is the maximum we can manage (examples: public IMAP servers,
      file formats using read only files &c).

      In this case, this method may be used to check if we can write to the
      folder. Note that it's invalid to use it unless IsOpened() is true

      @return true if the folder is opened as read only, false if read/write
    */
   virtual bool IsReadOnly(void) const = 0;

   /**
      An IMAP folder opened in read only mode might still allow setting the
      flags so this should be used instead of IsReadOnly() to test whether we
      can do it.

      @param flags the combination of flags we'd like to set or clear
      @return true if this flag can be set, false otherwise
    */
   virtual bool CanSetFlag(int flags) const = 0;

   /**
     Get the name of the mail folder: this is the user visible name which
     appears in the folder tree (for the folders which live there), not the
     path of the mailbox.

     @return the full symbolic name of the mailbox
    */
   virtual String GetName(void) const = 0;

   /// Return the folder's type.
   virtual MFolderType GetType(void) const = 0;

   /// return the folder flags
   virtual int GetFlags(void) const = 0;

   /** Get the profile used by this folder WITHOUT IncRef()ing IT!
       @return pointer to the profile (DO NOT DecRef() IT)
   */
   virtual Profile *GetProfile(void) const = 0;

   /// Checks if the folder is in a critical section.
   virtual bool IsInCriticalSection(void) const = 0;

   /// return class name
   const wxChar *GetClassName(void) const
      { return _T("MailFolder"); }

   /**
     Create the server info entry for the given folder -- this is a backdoor
     used by static ServerInfoEntry::GetOrCreate() to behave polymorphically
     and shouldn't be used by anything else.
    */
   virtual ServerInfoEntry *CreateServerInfo(const MFolder *folder) const = 0;

   /**
      Returns the delimiter used to separate the components of the folder
      name

      @return character depending on the folder type and server
    */
   virtual char GetFolderDelimiter() const = 0;

   /**
     This function allows to get the folder hierarchy delimiter without
     necessarily creating a MailFolder object just for this.

     @param folder the MFolder describing the folder we'd like to get info for
     @return the delimiter character or NUL
    */
   static char GetFolderDelimiter(const MFolder *folder);

   /**
      Get the logical mailbox name (as opposed to the physical one).

      Usually these 2 names are the same and they're only ever different for
      dual use mailboxes when the underlying driver doesn't support them. Dual
      use mailboxes are those which contain both messages and other mailboxes
      and are very convenient but not always directly supported (notorious
      example is MB(O)X format which represents each mailbox as a file and as a
      file can't be both a regular file and a subdirectory, it can contain
      either only messages or only other mailboxes but not both).

      If the dual use mailboxes are not supported directly, we emulate them
      using the following simple schema: for a dual use mailbox "foo" we use 2
      physical mailboxes: "foo" containing the subfolders and "foo.messages"
      containing the messages. This functions returns the logical name "foo"
      given either "foo" (i.e. normal situation) or "foo.messages".

      @param name the physical name of the mailbox
      @return the logical name of the mailbox with the given name
    */
   static String GetLogicalMailboxName(const String& name);

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
   /** Check whether mailbox has changed and also keeps it alive.
       @return FALSE on error, TRUE otherwise
   */
   virtual bool Ping(void) = 0;

   /** Perform a checkpoint on the folder. What this does depends on the server
       but, quoting from RFC 2060: "A checkpoint MAY take a non-instantaneous
       amount of real time to complete."
    */
   virtual void Checkpoint(void) = 0;

   /** get the message with unique id uid
       @param uid message uid
       @return message handler
   */
   virtual Message *GetMessage(unsigned long uid) const = 0;

   /** Delete a message, really delete, not move to trash. UNSUPPORTED!
       @param uid the message uid
       @return true on success
   */
   virtual bool DeleteMessage(unsigned long uid) = 0;

   /** UnDelete a message.
       @param uid the message uid @return true on success
   */
   virtual bool UnDeleteMessage(unsigned long uid) = 0;

   /**
     Set or clear the given flag for one message. Possible flag values are
     MSG_STAT_xxx.

     @param uid the message uid
     @param flag flag to be set, e.g. "\\Deleted"
     @param set if true, set the flag, if false, clear it
     @return true on success
   */
   virtual bool SetMessageFlag(unsigned long uid,
                               int flag,
                               bool set = true) = 0;

   /** Set flags on a sequence of messages. Possible flag values are
       MSG_STAT_xxx. Note that it is more efficient to call this function
       rather SetMessageFlag() for each sequence element so it should be used
       whenever possible.

       @param kind if SEQ_UID, sequence contains UIDs, otherwise -- msgnos
       @param sequence the sequence of uids or msgnos
       @param flag flag to be set, e.g. "\\Deleted"
       @param set if true, set the flag, if false, clear it
       @return true on success
   */
   virtual bool SetSequenceFlag(SequenceKind kind,
                                const Sequence& sequence,
                                int flag,
                                bool set = true) = 0;

   /// for compatibility: SetSequenceFlag() working only with UIDs
   bool SetSequenceFlag(const Sequence& sequence, int flag, bool set = true)
      { return SetSequenceFlag(SEQ_UID, sequence, flag, set); }

   /**
     Set or clear the given flag for all messages in the folder.
    */
   bool SetFlagForAll(int flag, bool set = true);

   /**
     Simple wrapper around SetSequenceFlag().

     Avoid using this one, it is mainly here for backwards compatibility.
   */
   bool SetFlag(const UIdArray *sequence, int flag, bool set = true);

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

   /**
     Find messages with given flags. It returns either msgnos (default) or
     UIDs if flags has SEARCH_UID bit set. If the parameter last is not 0
     (default value), then the search is restricted to the messages after this
     one. It is interpreted as either msgno or UID depending on the presence of
     SEARCH_UID flag.

     @param status one of MSG_STAT_xxx constants
     @param flags the combination of SEARCH_XXX bits
     @param last only return the matches after this UID/msgno
     @return array with msgnos or UIDs of messages found (to be freed by caller)
   */
   virtual MsgnoArray *SearchByFlag(MessageStatus flag,
                                    int flags = MailFolder::SEARCH_SET |
                                                MailFolder::SEARCH_UNDELETED,
                                    MsgnoType last = 0) const = 0;

   /**
     Search messages for certain criteria.

     @param crit the search criterium
     @param flags the search flags, only SEARCH_UID and SEARCH_MSGNO allowed
     @return UIdArray with UIds of matching messages, caller must free it
   */
   virtual UIdArray *SearchMessages(const SearchCriterium *crit,
                                    int flags = SEARCH_UID) = 0;

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
                                   wxWindow *parent = NULL) = 0;

   /** Mark messages as deleted or move them to trash.

       @param messages pointer to an array holding the message UIDs, !NULL
       @param flags combination of DELETE_XXX values
       @return true on success
   */
   virtual bool DeleteOrTrashMessages(const UIdArray *messages,
                                      int flags = DELETE_ALLOW_TRASH) = 0;

   /** Mark messages as deleted.
       @param messages pointer to an array holding the message numbers
       @param flags combination of DELETE_XXX values
       @return true on success
   */
   virtual bool DeleteMessages(const UIdArray *messages,
                               int flags = DELETE_NO_EXPUNGE) = 0;

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
                              wxWindow *parent = NULL) = 0;

   /** Forward selected messages.
       @param messages pointer to an array holding the message numbers
       @param params is the Params struct to use
       @param parent window for dialog
   */
   virtual void ForwardMessages(const UIdArray *messages,
                                const Params& params,
                                wxWindow *parent = NULL) = 0;

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

   /** @name New mail processing */
   //@{

   /**
     Process the new mail in this or another folder (if folderDst is not NULL):
     this may involve collecting it (== copying or moving to another folder),
     filtering it or just reporting it depending on the folder options. It
     removes all messages deleted as results of its actions from uidsNew array.

     If uidsNew is NULL, it means that we detected new mail in the folderDst
     but we don't know which messages are new - but we still know at least how
     many of them there are.

     @param uidsNew the array containing UIDs of the new messages
     @param folderDst if not NULL, folder where the new messages really are
     @param countNew the number of new messages if uidsNew == NULL
     @return true if ok, false on error
   */
   virtual bool ProcessNewMail(UIdArray& uidsNew,
                               const MFolder *folderDst = NULL) = 0;

   /** Apply any filter rules to the folder.
       Applies the rule to all messages listed in msgs.
       @return -1 if no filter module exists, return code otherwise
   */
   virtual int ApplyFilterRules(const UIdArray& msgs) = 0;

   //@}

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

      @param folder the folder to build spec for
      @param login for the folder, if empty folder->GetLogin() is used
      @return the full IMAP spec for the given folder
   */
   static String GetImapSpec(const MFolder *folder, const String& login = _T(""));

   /** Extracts the folder name from the folder specification string used by
       cclient (i.e. {nntp/xxx}news.answers => news.answers and also #mh/Foo
       => Foo)

       @param specification the full cclient folder specification
       @param folderType the (supposed) type of the folder
       @param name the variable where the folder name will be returned
       @return TRUE if folder name could be successfully extracted
    */
   static bool SpecToFolderName(const String& specification,
                                MFolderType folderType,
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

   /** @name mail subsystem initialization and shutdown */
   //@{

   /// Initialize the underlying mail library, return false on error
   static bool Init();

   /// Clean up for program exit.
   static void CleanUp();

   //@}

   /** @name Interactivity control */
   //@{
   /**
      Folder operations work differently if the interactive frame is set:
      they will put more messages into status bar and possibly ask questions to
      the user while in non interactive mode this will never be done. This
      method associates such frame with the folder with given name.

      @param frame the associated frame (may be NULL to disable interactivity)
      @return the old interactive frame for this folder
   */
   virtual wxFrame *SetInteractiveFrame(wxFrame *frame) = 0;

   /**
      Get the frame to use for interactive messages. May return NULL.
   */
   virtual wxFrame *GetInteractiveFrame() const = 0;

   /// Returns the shared log circle object used for error analysis
   static MLogCircle& GetLogCircle(void);
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
   /**
      Close the folder.

      If mayLinger parameter is true (default), we can keep the network
      connection so that it could be reused later. If it is false, the
      connection should be closed as well.
    */
   virtual void Close(bool mayLinger = true) = 0;

   /**
     Check if the network connectivity is up if the given folder requires it

     @param folder the folder we're going to access
     @param frame the parent window for the dialog boxes
     @return true if network is up or not required, false otherwise
    */
   static bool CheckNetwork(const MFolder *folder, wxFrame *frame);

   /** @name Authentification info */
   //@{

   /**
     Try to find login/password for the given folder looking among the
     logins/passwords previosuly entered by user if they are not stored in the
     folder itself. If it doesn't find it there neither, asks the user for the
     password (and sets didAsk to true then)

     @param mfolder the folder we need auth info for
     @param login the variable containing username
     @param password the variable containing password
     @param didAsk a pointer (may be NULL) set to true if password was entered
     @return true if the password is either not needed or was entered
    */
   static bool GetAuthInfoForFolder(const MFolder *mfolder,
                                    String& login,
                                    String& password,
                                    bool *didAsk = NULL);

   /**
     Propose to the user to save the login and password temporarily (i.e. in
     memory only for the duration of this session) or permanently (in config).

     @param mf the opened folder with correct GetLogin()/GetPassword() values
     @param folder for which the login and/or password should be saved
     @param login the login name to save
     @param password the password to save
   */
   static void ProposeSavePassword(MailFolder *mf,
                                   MFolder *folder,
                                   const String& login,
                                   const String& password);

   //@}


   /// the folder for which we had set default interactive frame
   static String ms_interactiveFolder;

   /// the frame to which interactive messages for ms_interactiveFolder go
   static wxFrame *ms_interactiveFrame;

private:
   /// this is the only class which can call our GetHeaderInfo()
   friend class HeaderInfoList;
};

class SuspendFolderUpdates
{
public:
   SuspendFolderUpdates(MailFolder *mf)
      : m_mf(mf)
      { m_mf->SuspendUpdates(); }
   ~SuspendFolderUpdates()
      { m_mf->ResumeUpdates(); }
private:
   MailFolder *m_mf;
};

// MailFolder_obj is a smart reference to MailFolder
DECLARE_AUTOPTR_WITH_CONVERSION(MailFolder);

#endif // _MAILFOLDER_H

