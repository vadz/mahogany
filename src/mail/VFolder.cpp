//////////////////////////////////////////////////////////////////////////////
// Project:     Mahogany - cross platform e-mail GUI client
// File name:   mail/VFolder.cpp: implementation of VirtualFolder class
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

    #include "MFolder.h"
#endif

#include "Sequence.h"
#include "UIdArray.h"

#include "MFStatus.h"

#include "HeaderInfo.h"
#include "HeaderInfoImpl.h" // we need "Impl" for ArrayHeaderInfo declaration

#include "mail/Driver.h"
#include "mail/VFolder.h"
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
   VirtualFolder::Init,
   VirtualFolder::OpenFolder,
   VirtualFolder::CheckStatus,
   VirtualFolder::DeleteFolder,
   VirtualFolder::RenameFolder,
   VirtualFolder::ClearFolder,
   VirtualFolder::GetFullImapSpec,
   VirtualFolder::Cleanup
);

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// VirtualFolder ctor/dtor
// ----------------------------------------------------------------------------

VirtualFolder::VirtualFolder(const MFolder *folder, OpenMode openmode)
             : m_folder((MFolder *)folder), m_openMode(openmode)
{
   m_folder->IncRef();
}

VirtualFolder::~VirtualFolder()
{
   Close();

   ClearMsgs();

   m_folder->DecRef();
}

// ----------------------------------------------------------------------------
// VirtualFolder driver methods
// ----------------------------------------------------------------------------

/* static */
bool VirtualFolder::Init()
{
   return true;
}

/* static */
void VirtualFolder::Cleanup()
{
   // NOOP
}

/* static */
MailFolder *
VirtualFolder::OpenFolder(const MFolder *folder,
                          const String& login,
                          const String& password,
                          OpenMode openmode,
                          wxFrame *frame)
{
   CHECK( folder, NULL, "NULL folder in VirtualFolder::OpenFolder" );

   VirtualFolder *vf = new VirtualFolder(folder, openmode);

   return vf;
}

/* static */
bool VirtualFolder::CheckStatus(const MFolder *folder)
{
   // the status of a virtual folder never changes from outside so this is a
   // NOOP too
   return true;
}

/* static */
bool VirtualFolder::DeleteFolder(const MFolder *folder)
{
   FAIL_MSG( "TODO" );

   return false;
}

/* static */
bool VirtualFolder::RenameFolder(const MFolder *folder, const String& name)
{
   FAIL_MSG( "TODO" );

   return false;
}

/* static */
long VirtualFolder::ClearFolder(const MFolder *folder)
{
   FAIL_MSG( "TODO" );

   return -1;
}

/* static */
String
VirtualFolder::GetFullImapSpec(const MFolder *folder, const String& login)
{
   CHECK( folder, "", "NULL folder in VirtualFolder::GetFullImapSpec" );

   return folder->GetFullName();
}

// ----------------------------------------------------------------------------
// trivial VirtualFolder accessors
// ----------------------------------------------------------------------------

bool VirtualFolder::IsOpened() const
{
    // we're always opened for as long as we exist
    return true;
}

bool VirtualFolder::IsReadOnly() const
{
    return m_openMode == ReadOnly;
}

bool VirtualFolder::CanSetFlag(int flags) const
{
    // all flags can be set even though not all of them make sense for us
    return true;
}

bool VirtualFolder::IsInCriticalSection() const
{
    // we never do anything critical
    return false;
}

ServerInfoEntry *VirtualFolder::CreateServerInfo(const MFolder *folder) const
{
    // we use the trivial server info implementation as we never reuse them and
    // don't use authentification neither
    return new VirtualServerInfo(folder);
}

char VirtualFolder::GetFolderDelimiter() const
{
    // the virtual folder name space is flat
    return '\0';
}

String VirtualFolder::GetName() const
{
   return m_folder->GetFullName();
}

String VirtualFolder::GetImapSpec() const
{
   return GetName();
}

MFolderType VirtualFolder::GetType() const
{
   return m_folder->GetType();
}

int VirtualFolder::GetFlags() const
{
   return m_folder->GetFlags();
}

// the pointer returned by this function should *NOT* be DecRef()'d
Profile *VirtualFolder::GetProfile() const
{
   Profile *profile = m_folder->GetProfile();
   if ( profile )
   {
      // for compatibility reasons we return profile not IncRef()'d so
      // compensate for one done by MFolder::GetProfile()
      profile->DecRef();
   }

   return profile;
}

// ----------------------------------------------------------------------------
// VirtualFolder methods working with m_messages
// ----------------------------------------------------------------------------

VirtualFolder::Msg *VirtualFolder::GetMsgFromMsgno(MsgnoType msgno) const
{
   msgno--;

   CHECK( 0 <= msgno && msgno < GetMsgCount(), NULL,
          "invalid msgno in VirtualFolder" );

   return m_messages[(size_t)msgno];
}

void VirtualFolder::AddMsg(VirtualFolder::Msg *msg)
{
   CHECK_RET( msg, "NULL Msg in VirtualFolder?" );

   m_messages.Add(msg);
}

VirtualFolder::Msg *VirtualFolder::GetFirstMsg(MsgCookie& cookie) const
{
   cookie = 0;

   return GetNextMsg(cookie);
}

VirtualFolder::Msg *VirtualFolder::GetNextMsg(MsgCookie& cookie) const
{
   size_t count = GetMsgCount();
   if ( cookie < count )
   {
      return m_messages[cookie++];
   }
   else
   {
      // shouldn't be > than it!
      ASSERT_MSG( cookie == count, "invalid msg index in VirtualFolder" );

      return NULL;
   }
}

void VirtualFolder::DeleteMsg(MsgCookie& cookie)
{
   CHECK_RET( cookie < GetMsgCount(), "invalid UID in VirtualFolder" );

   delete m_messages[cookie];

   // the indices are now shifted, so modify the cookie
   m_messages.RemoveAt(cookie--);
}

void VirtualFolder::ClearMsgs()
{
   WX_CLEAR_ARRAY(m_messages);
}

// ----------------------------------------------------------------------------
// VirtualFolder access control
// ----------------------------------------------------------------------------

bool VirtualFolder::Lock() const
{
   return true;
}

void VirtualFolder::UnLock() const
{
}

bool VirtualFolder::IsLocked() const
{
   return false;
}

// ----------------------------------------------------------------------------
// VirtualFolder operations on headers
// ----------------------------------------------------------------------------

MsgnoType
VirtualFolder::GetHeaderInfo(ArrayHeaderInfo& headers, const Sequence& seq)
{
   size_t count = 0;

   size_t cookie;
   for ( UIdType n = seq.GetFirst(cookie);
         n != UID_ILLEGAL;
         n = seq.GetNext(n, cookie), count )
   {
      const Msg *msg = GetMsgFromMsgno(n);
      if ( !msg )
         break;

      Sequence subseq;
      subseq.Add(msg->mf->GetMsgnoFromUID(msg->uid));

      // we rely on the fact that headers array contains pointers to HeaderInfo
      // objects so we can fill them by passing the same pointer to
      // GetHeaderInfo()
      HeaderInfo * const hi = headers[count++];

      ArrayHeaderInfo subhdrs;
      subhdrs.Add(hi);

      if ( msg->mf->GetHeaderInfo(subhdrs, subseq) )
      {
         // override some fields

         // UID must refer to this folder, not the other one (notice that count
         // is already incremented, as it should be -- UIDs == msgnos start
         // from 1, not 0)
         hi->m_UId = count;

         // and we maintain our own, independent flags
         hi->m_Status = msg->flags;
      }
      //else: what to do on failure? (FIXME)
   }

   return count;
}


unsigned long VirtualFolder::GetMessageCount() const
{
   return GetMsgCount();
}

unsigned long VirtualFolder::CountNewMessages() const
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

unsigned long VirtualFolder::CountRecentMessages() const
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

unsigned long VirtualFolder::CountUnseenMessages() const
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

unsigned long VirtualFolder::CountDeletedMessages() const
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

bool VirtualFolder::DoCountMessages(MailFolderStatus *status) const
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

MsgnoType VirtualFolder::GetMsgnoFromUID(UIdType uid) const
{
   // UIDs are the same as msgnos for us for now...
   return uid;
}

// ----------------------------------------------------------------------------
// other VirtualFolder operations
// ----------------------------------------------------------------------------

bool VirtualFolder::Ping()
{
   // we never get any new mail
   return true;
}

void VirtualFolder::Checkpoint()
{
   // NOOP
}

Message *VirtualFolder::GetMessage(unsigned long uid)
{
   const Msg *msg = GetMsgFromUID(uid);

   return msg ? msg->mf->GetMessage(msg->uid) : NULL;
}

bool
VirtualFolder::SetMessageFlag(unsigned long uid,
                              int flag,
                              bool set)
{
   Msg *msg = GetMsgFromUID(uid);

   if ( !msg )
      return false;

   if ( set )
      msg->flags |= flag;
   else
      msg->flags &= ~flag;

   return true;
}

bool
VirtualFolder::SetSequenceFlag(SequenceKind kind,
                               const Sequence& seq,
                               int flag,
                               bool set)
{
   bool rc = true;

   // we ignore sequence kind here because msgnos == UIDs for us
   size_t cookie;
   for ( UIdType n = seq.GetFirst(cookie);
         n != UID_ILLEGAL;
         n = seq.GetNext(n, cookie) )
   {
      if ( !SetMessageFlag(n, flag, set) )
         rc = false;
   }

   return rc;
}

bool VirtualFolder::AppendMessage(const Message& msg)
{
   MailFolder *mf = msg.GetFolder();
   if ( !mf )
      return false;

   HeaderInfoList_obj hil = mf->GetHeaders();
   if ( !hil )
      return false;

   UIdType uid = msg.GetUId();
   const HeaderInfo *hi = hil->GetEntryUId(uid);
   if ( !hi )
      return false;

   AddMsg(new Msg(mf, uid, hi->GetStatus()));

   return true;
}

bool VirtualFolder::AppendMessage(const String& msg)
{
   FAIL_MSG( "AppendMessage(string) can't be used with virtual folder" );

   return false;
}

void VirtualFolder::ExpungeMessages()
{
   MsgCookie cookie;
   for ( Msg *msg = GetFirstMsg(cookie); msg; msg = GetNextMsg(cookie) )
   {
      if ( msg->flags & MSG_STAT_DELETED )
      {
         DeleteMsg(cookie);
      }
   }
}

MsgnoArray *
VirtualFolder::SearchByFlag(MessageStatus flag,
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
            // we don't care whether we're searching for UIDs or msgnos as they
            // are one and the same for us
            results->Add(n);
         }
      }
   }

   return results;
}

void
VirtualFolder::ListFolders(class ASMailFolder *asmf,
                           const String &pattern,
                           bool subscribed_only,
                           const String &reference,
                           UserData ud,
                           Ticket ticket)
{
   // we never have any subfolders
}

