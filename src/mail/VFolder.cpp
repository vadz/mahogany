//////////////////////////////////////////////////////////////////////////////
// Project:     Mahogany - cross platform e-mail GUI client
// File name:   mail/VFolder.cpp: implementation of MailFolderVirt class
// Purpose:     virtual folder provides MailFolder interface to the message
//              physically living in other, different folders
// Author:      Vadim Zeitlin
// Modified by:
// Created:     16.07.02
// CVS-ID:      $Id$
// Copyright:   (c) 2002 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#ifdef __GNUG__
   #pragma implementation "VFolder.h"
#endif

#include  "Mpch.h"

#ifndef USE_PCH
    #include "Mcommon.h"
#endif // USE_PCH

#include "Sequence.h"
#include "UIdArray.h"

#include "MFStatus.h"

#include "HeaderInfoImpl.h" // we need "Impl" for ArrayHeaderInfo declaration

#include "mail/Driver.h"
#include "mail/VFolder.h"
#include "mail/VMessage.h"
#include "mail/ServerInfo.h"

// ----------------------------------------------------------------------------
// VirtualServerInfo: trivial implementation of ServerInfoEntry
// ----------------------------------------------------------------------------

class VirtualServerInfo : public ServerInfoEntry
{
public:
   VirtualServerInfo(const MFolder *folder) : ServerInfoEntry(folder) { }
};

// ----------------------------------------------------------------------------
// the virtual folder driver
// ----------------------------------------------------------------------------

static MFDriver gs_driverVirt
(
   "virtual",
   MailFolderVirt::Init,
   MailFolderVirt::OpenFolder,
   MailFolderVirt::CheckStatus,
   MailFolderVirt::DeleteFolder,
   MailFolderVirt::RenameFolder,
   MailFolderVirt::ClearFolder,
   MailFolderVirt::GetFullImapSpec,
   MailFolderVirt::Cleanup
);

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// MailFolderVirt ctor/dtor
// ----------------------------------------------------------------------------

MailFolderVirt::MailFolderVirt(const MFolder *folder, OpenMode openmode)
              : m_folder((MFolder *)folder), m_openMode(openmode)
{
   m_folder->IncRef();

   // no messages so far
   m_uidLast = 0;
}

MailFolderVirt::~MailFolderVirt()
{
   Close();

   ClearMsgs();

   m_folder->DecRef();
}

// ----------------------------------------------------------------------------
// MailFolderVirt driver methods
// ----------------------------------------------------------------------------

/* static */
bool MailFolderVirt::Init()
{
   return true;
}

/* static */
void MailFolderVirt::Cleanup()
{
   // NOOP
}

/* static */
MailFolder *
MailFolderVirt::OpenFolder(const MFolder *folder,
                           const String& /* login */,
                           const String& /* password */,
                           OpenMode openmode,
                           wxFrame * /* frame */)
{
   CHECK( folder, NULL, _T("NULL folder in MailFolderVirt::OpenFolder") );

   MailFolderVirt *vf = new MailFolderVirt(folder, openmode);

   return vf;
}

/* static */
bool MailFolderVirt::CheckStatus(const MFolder * /* folder */)
{
   // the status of a virtual folder never changes from outside so this is a
   // NOOP too
   return true;
}

/* static */
bool MailFolderVirt::DeleteFolder(const MFolder * /* folder */)
{
   FAIL_MSG( _T("TODO") );

   return false;
}

/* static */
bool MailFolderVirt::RenameFolder(const MFolder * /* folder */,
                                  const String& /* name */)
{
   FAIL_MSG( _T("TODO") );

   return false;
}

/* static */
long MailFolderVirt::ClearFolder(const MFolder * /* folder */)
{
   FAIL_MSG( _T("TODO") );

   return -1;
}

/* static */
String
MailFolderVirt::GetFullImapSpec(const MFolder *folder,
                                const String& /* login */)
{
   CHECK( folder, wxEmptyString, _T("NULL folder in MailFolderVirt::GetFullImapSpec") );

   String spec;
   spec << folder->GetFullName() << ';' << folder->GetPath();

   return spec;
}

// ----------------------------------------------------------------------------
// trivial MailFolderVirt accessors
// ----------------------------------------------------------------------------

bool MailFolderVirt::IsOpened() const
{
    // we're always opened for as long as we exist
    return true;
}

bool MailFolderVirt::IsReadOnly() const
{
    return m_openMode == ReadOnly;
}

bool MailFolderVirt::CanSetFlag(int /* flags */) const
{
    // all flags can be set even though not all of them make sense for us
    return true;
}

bool MailFolderVirt::IsInCriticalSection() const
{
    // we never do anything critical
    return false;
}

ServerInfoEntry *MailFolderVirt::CreateServerInfo(const MFolder *folder) const
{
    // we use the trivial server info implementation as we never reuse them and
    // don't use authentication neither
    return new VirtualServerInfo(folder);
}

char MailFolderVirt::GetFolderDelimiter() const
{
    // the virtual folder name space is flat
    return '\0';
}

String MailFolderVirt::GetName() const
{
   return m_folder->GetFullName();
}

MFolderType MailFolderVirt::GetType() const
{
   return m_folder->GetType();
}

int MailFolderVirt::GetFlags() const
{
   return m_folder->GetFlags();
}

// the pointer returned by this function should *NOT* be DecRef()'d
Profile *MailFolderVirt::GetProfile() const
{
   Profile *profile = m_folder->GetProfile();
   if ( profile )
   {
      // for compatibility reasons we return profile not IncRef()'d so
      // compensate for IncRef() done by MFolder::GetProfile()
      profile->DecRef();
   }

   return profile;
}

// ----------------------------------------------------------------------------
// MailFolderVirt methods working with m_messages
// ----------------------------------------------------------------------------

MailFolderVirt::Msg *MailFolderVirt::GetMsgFromMsgno(MsgnoType msgno) const
{
   msgno--;

   CHECK( 0 <= msgno && msgno < GetMsgCount(), NULL,
          _T("invalid msgno in MailFolderVirt") );

   return m_messages[(size_t)msgno];
}

MailFolderVirt::Msg *MailFolderVirt::GetMsgFromUID(UIdType uid) const
{
   MsgCookie cookie;
   for ( Msg *msg = GetFirstMsg(cookie); msg; msg = GetNextMsg(cookie) )
   {
      if ( msg->uidVirt == uid )
      {
         return msg;
      }
   }

   FAIL_MSG( _T("no message with such UID in the virtual folder") );

   return NULL;
}

void MailFolderVirt::AddMsg(MailFolderVirt::Msg *msg)
{
   CHECK_RET( msg, _T("NULL Msg in MailFolderVirt?") );

   m_messages.Add(msg);
}

MailFolderVirt::Msg *MailFolderVirt::GetFirstMsg(MsgCookie& cookie) const
{
   cookie = 0;

   return GetNextMsg(cookie);
}

MailFolderVirt::Msg *MailFolderVirt::GetNextMsg(MsgCookie& cookie) const
{
   size_t count = GetMsgCount();
   if ( cookie < count )
   {
      return m_messages[cookie++];
   }
   else
   {
      // shouldn't be > than it!
      ASSERT_MSG( cookie == count, _T("invalid msg index in MailFolderVirt") );

      return NULL;
   }
}

void MailFolderVirt::DeleteMsg(MsgCookie& cookie)
{
   CHECK_RET( cookie < GetMsgCount(), _T("invalid UID in MailFolderVirt") );

   CHECK_RET( m_headers, _T("no HeaderInfoList in MailFolderVirt::DeleteMsg()?") );

   // collect the information about the expunged messages to notify the GUI
   // about them later
   if ( !m_expungeData )
   {
      m_expungeData = new ExpungeData;
   }

   // the cookie already contains the index of the next item so normally we
   // should decrement it but it's already ok as msgno, so use it first and
   // decrement later to make it the correct index
   m_expungeData->msgnos.Add(cookie--);
   m_expungeData->positions.Add(m_headers->GetPosFromIdx(cookie));

   // also let the headers object know that this header doesn't exist any more
   m_headers->OnRemove(cookie);

   // finally, really delete the message
   delete m_messages[cookie];

   m_messages.RemoveAt(cookie);
}

void MailFolderVirt::ClearMsgs()
{
   WX_CLEAR_ARRAY(m_messages);
}

// ----------------------------------------------------------------------------
// MailFolderVirt access control
// ----------------------------------------------------------------------------

bool MailFolderVirt::Lock() const
{
   return true;
}

void MailFolderVirt::UnLock() const
{
}

bool MailFolderVirt::IsLocked() const
{
   return false;
}

// ----------------------------------------------------------------------------
// MailFolderVirt operations on headers
// ----------------------------------------------------------------------------

MsgnoType
MailFolderVirt::GetHeaderInfo(ArrayHeaderInfo& headers, const Sequence& seq)
{
   size_t count = 0;

   size_t cookie;
   for ( UIdType n = seq.GetFirst(cookie);
         n != UID_ILLEGAL;
         n = seq.GetNext(n, cookie), count++ )
   {
      const Msg *msg = GetMsgFromMsgno(n);
      if ( !msg )
         break;

      HeaderInfoList_obj hil(msg->mf->GetHeaders());

      HeaderInfo * const hiDst = headers[n - 1]; // -1 to make index from msgno
      if ( !hiDst )
      {
         FAIL_MSG( _T("NULL pointer in array passed to GetHeaderInfo()?") );
         break;
      }

      const HeaderInfo * const hiSrc = hil->GetEntryUId(msg->uidPhys);
      if ( !hiSrc )
      {
         FAIL_MSG( _T("failed to get header info in virtual folder") );
         break;
      }

      *hiDst = *hiSrc;

      // override some fields:

      // UID must refer to this folder, not the other one
      hiDst->m_UId = msg->uidVirt;

      // and we maintain our own, independent flags
      hiDst->m_Status = msg->flags;
   }

   return count;
}


unsigned long MailFolderVirt::GetMessageCount() const
{
   return GetMsgCount();
}

unsigned long MailFolderVirt::CountNewMessages() const
{
   unsigned long count = 0;

   MsgCookie cookie;
   for ( Msg *msg = GetFirstMsg(cookie); msg; msg = GetNextMsg(cookie) )
   {
      if ( !(msg->flags & MSG_STAT_SEEN) && (msg->flags & MSG_STAT_RECENT) )
         count++;
   }

   return count;
}

unsigned long MailFolderVirt::CountRecentMessages() const
{
   unsigned long count = 0;

   MsgCookie cookie;
   for ( Msg *msg = GetFirstMsg(cookie); msg; msg = GetNextMsg(cookie) )
   {
      if ( msg->flags & MSG_STAT_RECENT )
         count++;
   }

   return count;
}

unsigned long MailFolderVirt::CountUnseenMessages() const
{
   unsigned long count = 0;

   MsgCookie cookie;
   for ( Msg *msg = GetFirstMsg(cookie); msg; msg = GetNextMsg(cookie) )
   {
      if ( !(msg->flags & MSG_STAT_SEEN) )
         count++;
   }

   return count;
}

unsigned long MailFolderVirt::CountDeletedMessages() const
{
   unsigned long count = 0;

   MsgCookie cookie;
   for ( Msg *msg = GetFirstMsg(cookie); msg; msg = GetNextMsg(cookie) )
   {
      if ( msg->flags & MSG_STAT_DELETED )
         count++;
   }

   return count;
}

bool MailFolderVirt::DoCountMessages(MailFolderStatus *status) const
{
   status->Init();
   status->total = GetMsgCount();

   MsgCookie cookie;
   for ( Msg *msg = GetFirstMsg(cookie); msg; msg = GetNextMsg(cookie) )
   {
      bool isRecent = (msg->flags & MSG_STAT_RECENT) != 0;

      if ( !(msg->flags & MSG_STAT_SEEN) )
      {
         if ( isRecent )
            status->newmsgs++;

         status->unread++;
      }
      else if ( isRecent )
      {
         status->recent++;
      }

      if ( msg->flags & MSG_STAT_FLAGGED )
         status->flagged++;

      if ( msg->flags & MSG_STAT_SEARCHED )
         status->searched++;
   }

   return true;
}

MsgnoType MailFolderVirt::GetMsgnoFromUID(UIdType uid) const
{
   MsgnoType n = 1;
   MsgCookie cookie;
   for ( Msg *msg = GetFirstMsg(cookie); msg; msg = GetNextMsg(cookie), n++ )
   {
      if ( msg->uidVirt == uid )
      {
         return n;
      }
   }

   return MSGNO_ILLEGAL;
}

// ----------------------------------------------------------------------------
// other MailFolderVirt operations
// ----------------------------------------------------------------------------

bool MailFolderVirt::Ping()
{
   // we never get any new mail
   return true;
}

void MailFolderVirt::Checkpoint()
{
   // NOOP
}

Message *MailFolderVirt::GetMessage(unsigned long uid) const
{
   Msg *msg = GetMsgFromUID(uid);
   if ( !msg )
      return NULL;

   Message *message = msg->mf->GetMessage(msg->uidPhys);
   if ( !message )
      return NULL;

   return MessageVirt::Create((MailFolderVirt *)this, uid, &msg->flags, message);
}

bool
MailFolderVirt::SetMessageFlag(unsigned long uid, int flag, bool set)
{
   if ( !DoSetMessageFlag(SEQ_UID, uid, flag, set) )
      return false;

   SendMsgStatusChangeEvent();

   return true;
}

bool
MailFolderVirt::DoSetMessageFlag(SequenceKind kind,
                                 unsigned long uid,
                                 int flag,
                                 bool set)
{
   Msg *msg = kind == SEQ_MSGNO ? GetMsgFromMsgno(uid) : GetMsgFromUID(uid);

   if ( !msg )
      return false;

   int flagsOld = msg->flags;
   int flagsNew = set ? (flagsOld | flag) : (flagsOld & ~flag);
   if ( flagsOld == flagsNew )
   {
      // nothing changed
      return true;
   }

   const MsgnoType msgno = kind == SEQ_MSGNO ? uid : GetMsgnoFromUID(uid);
   CHECK( msgno != MSGNO_ILLEGAL, false, _T("SetMessageFlag: invalid UID") );

   HeaderInfoList_obj headers(GetHeaders());
   CHECK( headers, false, _T("SetMessageFlag: couldn't get headers") );

   HeaderInfo *hi = headers->GetItemByMsgno(msgno);
   CHECK( hi, false, _T("SetMessageFlag: no header info for the given msgno?") );

   // remember the old and new status of the changed messages
   if ( !m_statusChangeData )
   {
      m_statusChangeData = new StatusChangeData;
   }

   m_statusChangeData->msgnos.Add(msgno);
   m_statusChangeData->statusOld.Add(flagsOld);
   m_statusChangeData->statusNew.Add(flagsNew);

   hi->m_Status =
   msg->flags = flagsNew;

   return true;
}

bool
MailFolderVirt::SetSequenceFlag(SequenceKind kind,
                                const Sequence& seq,
                                int flag,
                                bool set)
{
   bool rc = true;

   size_t cookie;
   for ( UIdType n = seq.GetFirst(cookie);
         n != UID_ILLEGAL;
         n = seq.GetNext(n, cookie) )
   {
      if ( !DoSetMessageFlag(kind, n, flag, set) )
         rc = false;
   }

   SendMsgStatusChangeEvent();

   return rc;
}

bool MailFolderVirt::AppendMessage(const Message& msg)
{
   MailFolder *mf = msg.GetFolder();
   if ( !mf )
      return false;

   HeaderInfoList_obj hil(mf->GetHeaders());
   if ( !hil )
      return false;

   UIdType uidPhys = msg.GetUId();
   const HeaderInfo *hi = hil->GetEntryUId(uidPhys);
   if ( !hi )
      return false;

   // use the new UID for the new message
   AddMsg(new Msg(mf, uidPhys, ++m_uidLast, hi->GetStatus()));

   return true;
}

bool MailFolderVirt::AppendMessage(const String& /* msg */)
{
   FAIL_MSG( _T("AppendMessage(string) can't be used with virtual folder") );

   return false;
}

void MailFolderVirt::ExpungeMessages()
{
   MsgCookie cookie;
   for ( Msg *msg = GetFirstMsg(cookie); msg; msg = GetNextMsg(cookie) )
   {
      if ( msg->flags & MSG_STAT_DELETED )
      {
         DeleteMsg(cookie);
      }
   }

   if ( m_expungeData )
   {
      RequestUpdateAfterExpunge();
   }
}

MsgnoArray *
MailFolderVirt::SearchByFlag(MessageStatus flag,
                             int flags,
                             MsgnoType last) const
{
   int shouldBeSet = flags & SEARCH_SET;

   MsgnoArray *results = new MsgnoArray;

   MsgnoType n = 0;
   MsgCookie cookie;
   for ( Msg *msg = GetFirstMsg(cookie); msg; msg = GetNextMsg(cookie), n++ )
   {
      // should we skip all messages until "last"?
      //
      // NB: if last == 0 we should ignore it which is just what this test does
      if ( n < last )
         continue;

      // check if the flags match
      if ( (msg->flags & flag) == shouldBeSet )
      {
         // they do, now check that this message is not deleted -- or that we
         // don't care whether it is or not
         if ( !(msg->flags & MSG_STAT_DELETED) || !(flags & SEARCH_UNDELETED) )
         {
            results->Add(flags & SEARCH_UID ? msg->uidVirt : n);
         }
      }
   }

   return results;
}

void
MailFolderVirt::ListFolders(class ASMailFolder * /* asmf */,
                            const String& /* pattern */,
                            bool /* subscribed_only */,
                            const String& /* reference */,
                            UserData /* ud */,
                            Ticket /* ticket */)
{
   // we never have any subfolders
}

