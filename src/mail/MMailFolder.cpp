/*-*- c++ -*-********************************************************
 * MMailFolder class: Mahogany's own mailfolder formats             *
 *                                                                  *
 * (C) 2000 by Karsten Ballüder (ballueder@gmx.net)                 *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/

/**
   @package Mailfolder access
   @author  Karsten Ballüder
*/

#ifdef __GNUG__
#   pragma implementation "MMailFolder.h"
#endif

#include "Mpch.h"

#ifndef   USE_PCH
#   include  "kbList.h"
#   include  "MObject.h"
#   include  "strutil.h"
#endif
#include "MMailFolder.h"


#define MMAILFOLDER_MISSING() \
   ASSERT_MSG(0, "Missing MMailFolder functionality");


/** This class is responsible for retrieving and writing a mail
    message and for retrieving and writing the message cache.
    Two implementations will exist in the end, one having cache and
    all messages in a single mailbox file, the other one using a MH
    style directory with a separate cache file.
*/
class IOHandler : public MObject
{
public:
   virtual Message * GetMessage(UIdType uid) = 0;
   virtual bool      StoreMessage(UIdType uid) = 0;

   virtual ~IOHandler()
      { }
};

/** IOHandler implementation using a directory of message files with a 
    separate index file. */
class IOHandlerMH : public IOHandler
{
public:
   virtual Message * GetMessage(UIdType uid);
   virtual bool      StoreMessage(UIdType uid);
};

Message *
IOHandlerMH::GetMessage(UIdType uid)
{
   MOcheck();
   MMAILFOLDER_MISSING();
   return NULL;
}

bool
IOHandlerMH::StoreMessage(UIdType uid)
{
   MOcheck();
   MMAILFOLDER_MISSING();
   return FALSE;
}



/* FIXME: */
MMailFolder::MMailFolder()
   : MMailFolderBase(NULL)
{
}


/* static */
bool
MMailFolder::CreateFolder(const String &name,
                              FolderType type = MF_FILE,
                              int flags = MF_FLAGS_DEFAULT,
                              const String &path = "",
                              const String &comment = ""){ MMAILFOLDER_MISSING(); }

/** Checks if it is OK to exit the application now.
    @param which Will either be set to empty or a '\n' delimited
    list of folders which are in critical sections.
*/
/* static */
bool
MMailFolder::CanExit(String *which)
{
   MMAILFOLDER_MISSING();
   return TRUE;
}

/** Utiltiy function to get a textual representation of a message
    status.
    @param message status
    @param mf optional pointer to folder to treat e.g. NNTP separately
    @return string representation
    */
/* static */
String
MMailFolder::ConvertMessageStatusToString(int status,
                                              MailFolder *mf = NULL){ MMAILFOLDER_MISSING(); }
/** Forward one message.
    @param message message to forward
    @param profile environment
    @param parent window for dialog
*/
/* static */
void
MMailFolder::ForwardMessage(class Message *msg,
                                ProfileBase *profile = NULL,
                                MWindow *parent = NULL){ MMAILFOLDER_MISSING(); }
/** Reply to one message.
    @param message message to reply to
    @param flags 0, or REPLY_FOLLOWUP
    @param profile environment
    @param parent window for dialog
    */
/* static */
void
MMailFolder::ReplyMessage(class Message *msg,
                              int flags = 0,
                              ProfileBase *profile = NULL,
                              MWindow *parent = NULL){ MMAILFOLDER_MISSING(); }
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
/* static */
bool
MMailFolder::Subscribe(const String &host,
                           FolderType protocol,
                           const String &mailboxname,
                           bool subscribe = true){ MMAILFOLDER_MISSING(); }

/** Get a listing of all mailboxes.

    DO NOT USE THIS FUNCTION, BUT ASMailFolder::ListFolders instead!!!

    @param asmf the ASMailFolder initiating the request
    @param pattern a wildcard matching the folders to list
    @param subscribed_only if true, only the subscribed ones
    @param reference implementation dependend reference
*/
void
MMailFolder::ListFolders(class ASMailFolder *asmf,
                             const String &pattern = "*",
                             bool subscribed_only = false,
                             const String &reference = "",
                             UserData ud = 0,
                             Ticket ticket = ILLEGAL_TICKET){ MMAILFOLDER_MISSING(); }
//@}
//@}

/** Get name of mailbox.
    @return the symbolic name of the mailbox
*/
String
MMailFolder::GetName(void) const{ MMAILFOLDER_MISSING(); }

/** Sets the symbolic name.
 */
void
MMailFolder::SetName(const String &name){ MMAILFOLDER_MISSING(); }

/** Get number of messages which have a message status of value
    when combined with the mask. When mask = 0, return total
    message count.
    @param mask is a (combination of) MessageStatus value(s) or 0 to test against
    @param value the value of MessageStatus AND mask to be counted
    @return number of messages
*/
unsigned long
MMailFolder::CountMessages(int mask = 0, int value = 0) const{ MMAILFOLDER_MISSING(); }

/// Count number of recent messages.
unsigned long
MMailFolder::CountRecentMessages(void) const{ MMAILFOLDER_MISSING(); }

/** Count number of new messages but only if a listing is
    available, returns UID_ILLEGAL otherwise.
*/
unsigned long
MMailFolder::CountNewMessagesQuick(void) const{ MMAILFOLDER_MISSING(); }

/** Check whether mailbox has changed. */
void
MMailFolder::Ping(void){ MMAILFOLDER_MISSING(); }

/** get the message with unique id uid
    @param uid message uid
    @return message handler
*/
Message *
MMailFolder::GetMessage(unsigned long uid){ MMAILFOLDER_MISSING(); }

/** Set flags on a sequence of messages. Possible flag values are MSG_STAT_xxx
    @param sequence the IMAP sequence of uids
    @param flag flag to be set, e.g. "\\Deleted"
    @param set if true, set the flag, if false, clear it
    @return true on success
*/
bool
MMailFolder::SetFlag(const UIdArray *sequence,
                         int flag,
                         bool set = true){ MMAILFOLDER_MISSING(); }

/** Set flags on a sequence of messages. Possible flag values are MSG_STAT_xxx
    @param sequence the IMAP sequence of uids
    @param flag flag to be set, e.g. "\\Deleted"
    @param set if true, set the flag, if false, clear it
    @return true on success
*/
bool
MMailFolder::SetSequenceFlag(const String &sequence,
                                 int flag,
                                 bool set = true){ MMAILFOLDER_MISSING(); }
/** Appends the message to this folder.
    @param msg the message to append
    @return true on success
*/
bool
MMailFolder::AppendMessage(const Message &msg){ MMAILFOLDER_MISSING(); }

/** Appends the message to this folder.
    @param msg text of the  message to append
    @return true on success
*/
bool
MMailFolder::AppendMessage(const String &msg){ MMAILFOLDER_MISSING(); }

/** Expunge messages.
 */
void
MMailFolder::ExpungeMessages(void){ MMAILFOLDER_MISSING(); }

/** Search Messages for certain criteria.
    @return UIdArray with UIds of matching messages, caller must
    free it
*/
UIdArray *
MMailFolder::SearchMessages(const class SearchCriterium *crit){ MMAILFOLDER_MISSING(); }

/**@name Access control */
//@{
/** Locks a mailfolder for exclusive access. In multi-threaded mode
    it really locks it and always returns true. In single-threaded
    mode it returns false if we cannot get a lock.
    @return TRUE if we have obtained the lock
*/
bool
MMailFolder::Lock(void) const{ MMAILFOLDER_MISSING(); }

/** Releases the lock on the mailfolder. */
void
MMailFolder::UnLock(void) const{ MMAILFOLDER_MISSING(); }

/// Is folder locked?
bool
MMailFolder::IsLocked(void) const{ MMAILFOLDER_MISSING(); }
//@}

/**@name Functions to get an overview of messages in the folder. */
//@{
/** Returns a listing of the folder. Must be DecRef'd by caller. */
class HeaderInfoList *
MMailFolder::GetHeaders(void) const{ MMAILFOLDER_MISSING(); }
//@}
/// Return the folder's type.
FolderType
MMailFolder::GetType(void) const{ MMAILFOLDER_MISSING(); }
/// return the folder flags
int
MMailFolder::GetFlags(void) const{ MMAILFOLDER_MISSING(); }

/** Sets a maximum number of messages to retrieve from server.
    @param nmax maximum number of messages to retrieve, 0 for no limit
*/
void
MMailFolder::SetRetrievalLimit(unsigned long nmax){ MMAILFOLDER_MISSING(); }

/// Request update
void
MMailFolder::RequestUpdate(void){ MMAILFOLDER_MISSING(); }


/// Update the folder status, number of messages, etc
void
MMailFolder::UpdateStatus(void)
{
   MMAILFOLDER_MISSING(); 
}

/** Check if this message is a "New Message" for generating new
    mail event. */
bool
MMailFolder::IsNewMessage(const HeaderInfo * hi)
{
   MMAILFOLDER_MISSING(); 
}
