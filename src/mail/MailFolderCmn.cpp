//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   mail/MailFolderCmn.cpp: generic MailFolder methods which don't
//              use cclient (i.e. don't really work with mail folders)
// Purpose:     handling of mail folders with c-client lib
// Author:      Karsten Ballüder
// Modified by:
// Created:     02.04.01 (extracted from mail/MailFolder.cpp)
// CVS-ID:      $Id$
// Copyright:   (C) 1997-2000 by Karsten Ballüder (ballueder@gmx.net)
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

#ifdef __GNUG__
#   pragma implementation "MailFolderCmn.h"
#endif

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include  "Mpch.h"

#ifndef USE_PCH
#  include "Mcommon.h"
#  include "guidef.h"    // only for high-level functions
#  include "strutil.h"
#  include "Profile.h"
#  include "MEvent.h"
#  include "MApplication.h"
#endif

#include "Mdefaults.h"

#include "MFolder.h"
#include "MFilter.h"
#include "modules/Filters.h"

#include "MDialogs.h"         // for MProgressDialog

#include "HeaderInfo.h"

#include "MThread.h"

#include "MFCache.h"

#include "miscutil.h"         // for GetSequenceString()

#include <wx/timer.h>
#include <wx/datetime.h>
#include <wx/file.h>

#include "wx/persctrl.h"      // for wxPFileSelector

#include "MailFolderCmn.h"

#if defined(EXPERIMENTAL_JWZ_THREADING)
#ifndef   MCCLIENT_H
#include "Mcclient.h"         // for hash_table
#endif
#include <wx/regex.h>
#endif

// ----------------------------------------------------------------------------
// trace masks
// ----------------------------------------------------------------------------

// trace new mail processing
#define TRACE_NEWMAIL "newmail"

// trace mail folder ref counting
#define TRACE_MF_REF "mfref"

// trace mask for mail folder closing
#define TRACE_MF_CLOSE "mfclose"

// ----------------------------------------------------------------------------
// mailfolder closing helper classes
// ----------------------------------------------------------------------------

/**
   Idea: before actually closing a mailfolder, we keep it around for a
   some time. If someone reaccesses it, this speeds things up. So
   all we need, is a little helper class to keep a list of mailfolders
   open until a timeout occurs.
*/

// an entry which keeps one "closed" folder (it is really still opened)
class MfCloseEntry
{
public:
   enum
   {
      // special value for MfCloseEntry() ctor delay parameter
      NEVER_EXPIRES = INT_MAX
   };

   MfCloseEntry(MailFolderCmn *mf, int secs);
   ~MfCloseEntry();

   // has this entry expired?
   bool HasExpired() const
   {
      return m_expires && (m_dt > wxDateTime::Now());
   }

   // does this entry match this folder?
   bool Matches(const MailFolder *mf) const
   {
      return m_mf == mf;
   }

#ifdef DEBUG
   String GetName() const { return m_mf->GetName(); }
#endif // DEBUG

private:
   // the folder we're going to close
   MailFolderCmn *m_mf;

   // the time when we will close it
   wxDateTime m_dt;

   // if false, timeout is infinite
   bool m_expires;
};

// a linked list of MfCloseEntries
KBLIST_DEFINE(MfList, MfCloseEntry);

// the timer which periodically really closes the previously "closed" folders
class MfCloser;
class MfCloseTimer : public wxTimer
{
public:
   MfCloseTimer(MfCloser *mfCloser) { m_mfCloser = mfCloser; }

   virtual void Notify(void);

private:
   MfCloser *m_mfCloser;
};

// the class which keeps alive all "closed" folders until next timeout
class MfCloser : public MObject
{
public:
   MfCloser();
   virtual ~MfCloser();

   // add a new "closed" folder to the list
   void Add(MailFolderCmn *mf, int delay);

   // close the folders which timed out
   void OnTimer(void);

   // close all folders (for example because the program terminates)
   void CleanUp(void);

   // get the entry for this folder or NULL if it's not in the list
   MfCloseEntry *GetCloseEntry(MailFolderCmn *mf) const;

   // remove the given entry from list
   void Remove(MfCloseEntry *entry);

   // restart the timer (useful if timer interval changed)
   void RestartTimer();

private:
   // the list of folder entries to close
   MfList m_MfList;

   // the timer we use for periodically checking for expired folders
   MfCloseTimer m_timer;

   // the interval of this timer (in seconds)
   int m_interval;
};

// ----------------------------------------------------------------------------
// MfCmnEventReceiver: event receiver for MailFolderCmn, reacts to options
//                     change event
// ----------------------------------------------------------------------------

class MfCmnEventReceiver : public MEventReceiver
{
public:
   MfCmnEventReceiver(MailFolderCmn *mf);
   virtual ~MfCmnEventReceiver();

   virtual bool OnMEvent(MEventData& event);

private:
   MailFolderCmn *m_Mf;

   void *m_MEventCookie,
        *m_MEventPingCookie;
};

/** a timer class to regularly ping the mailfolder. */
class MailFolderTimer : public wxTimer
{
public:
   /** constructor
       @param mf the mailfolder to query on timeout
   */
   MailFolderTimer(MailFolder *mf)
   {
      m_mf = mf;
   }

   /// get called on timeout and pings the mailfolder
   void Notify(void) { if(! m_mf->IsLocked() ) m_mf->Ping(); }

protected:
   /// the mailfolder to update
   MailFolder *m_mf;
};

// ----------------------------------------------------------------------------
// module global variables
// ----------------------------------------------------------------------------

// the unique MfCloser object
static MfCloser *gs_MailFolderCloser = NULL;

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// MfCloseEntry
// ----------------------------------------------------------------------------

MfCloseEntry::MfCloseEntry(MailFolderCmn *mf, int secs)
{
   wxLogTrace(TRACE_MF_CLOSE,
              "Delaying closing of folder '%s' (%d refs) for %d seconds.",
              mf->GetName().c_str(), mf->GetNRef(), secs);

   m_mf = mf;

   // keep it for now
   m_mf->IncRef();

   m_expires = secs != NEVER_EXPIRES;
   if ( m_expires )
   {
      m_dt = wxDateTime::Now() + wxTimeSpan::Seconds(secs);
   }
}

MfCloseEntry::~MfCloseEntry()
{
   wxLogTrace(TRACE_MF_CLOSE, "Destroying MfCloseEntry(%s) (%d refs left)",
              m_mf->GetName().c_str(), m_mf->GetNRef());

   m_mf->RealDecRef();
}

// ----------------------------------------------------------------------------
// MfCloser
// ----------------------------------------------------------------------------

MfCloser::MfCloser()
        : m_MfList(true),
          m_timer(this)
{
   m_interval = READ_APPCONFIG(MP_FOLDER_CLOSE_DELAY);

   // start it, in fact
   RestartTimer();
}

MfCloser::~MfCloser()
{
   // we don't want to get any more timer events, we're shutting down
   m_timer.Stop();

   // close all folders
   CleanUp();
}

void MfCloser::Add(MailFolderCmn *mf, int delay)
{
#ifdef DEBUG_FOLDER_CLOSE
   // this shouldn't happen as we only add the folder to the list when
   // it is about to be deleted and it can't be deleted if we're holding
   // to it (MfCloseEntry has a lock), so this would be an error in ref
   // counting (extra DecRef()) elsewhere
   ASSERT_MSG( !GetCloseEntry(mf),
               "adding a folder to MfCloser twice??" );
#endif // DEBUG_FOLDER_CLOSE

   CHECK_RET( mf, "NULL MailFolder in MfCloser::Add()");

   wxLogTrace(TRACE_MF_REF, "Adding '%s' to gs_MailFolderCloser",
              mf->GetName().c_str());

   m_MfList.push_back(new MfCloseEntry(mf, delay));

   if ( delay < m_interval )
   {
      // restart the timer using smaller interval - of course, normally we
      // should compute the pgcd of all intervals but let's keep it simple
      m_interval = delay;

      RestartTimer();
   }
}

void MfCloser::OnTimer(void)
{
   MfList::iterator i;
   for( i = m_MfList.begin(); i != m_MfList.end();)
   {
      MfCloseEntry *entry = *i;
      if ( entry->HasExpired() )
      {
         i = m_MfList.erase(i);
      }
      else
      {
         ++i;
      }
   }
}

void MfCloser::CleanUp(void)
{
   MfList::iterator i = m_MfList.begin();
   while (i != m_MfList.end() )
      i = m_MfList.erase(i);
}

void MfCloser::Remove(MfCloseEntry *entry)
{
   CHECK_RET( entry, "NULL entry in MfCloser::Remove" );

#ifdef DEBUG
   wxLogTrace(TRACE_MF_REF, "Removing '%s' from gs_MailFolderCloser",
              entry->GetName().c_str());
#endif // DEBUG

   for ( MfList::iterator i = m_MfList.begin(); i != m_MfList.end(); i++ )
   {
      if ( *i == entry )
      {
         m_MfList.erase(i);

         break;
      }
   }
}

MfCloseEntry *MfCloser::GetCloseEntry(MailFolderCmn *mf) const
{
   for ( MfList::iterator i = m_MfList.begin(); i != m_MfList.end(); i++ )
   {
      MfCloseEntry *entry = *i;
      if ( entry->Matches(mf) )
      {
         return entry;
      }
   }

   return NULL;
}

void MfCloser::RestartTimer()
{
   if ( m_timer.IsRunning() )
      m_timer.Stop();

   // delay is in seconds, we need ms here
   m_timer.Start(m_interval * 1000);
}

void MfCloseTimer::Notify(void)
{
   m_mfCloser->OnTimer();
}

// ----------------------------------------------------------------------------
// MailFolderCmn folder closing
// ----------------------------------------------------------------------------

void MailFolderCmn::Close(void)
{
   if ( m_headers )
   {
      // in case someone else holds to it
      m_headers->OnClose();

      m_headers->DecRef();

      m_headers = NULL;
   }

   if ( m_Timer )
      m_Timer->Stop();
}

bool
MailFolderCmn::DecRef()
{
   // don't keep folders alive artificially if we're going to terminate soon
   // anyhow - or if we didn't start up fully yet and gs_MailFolderCloser
   // hadn't been created
   if ( gs_MailFolderCloser && !mApplication->IsShuttingDown() )
   {
      // if the folder is going to disappear, bump up its ref count
      // artificially to keep it alive for a while
      //
      // we only do it for folders which are really opened, there is no sense
      // in lagging dead connections around
      if ( GetNRef() == 1 && IsOpened() )
      {
         int delay;

         if ( GetFlags() & MF_FLAGS_KEEPOPEN )
         {
            // never close it at all
            delay = MfCloseEntry::NEVER_EXPIRES;
         }
         else
         {
            // don't close it only if the linger delay is set
            delay = READ_CONFIG(GetProfile(), MP_FOLDER_CLOSE_DELAY);
         }

         if ( delay > 0 )
         {
            Checkpoint(); // flush data immediately

            // this calls IncRef() on us so we won't be deleted right now
            gs_MailFolderCloser->Add(this, delay);
         }
      }
   }

   return RealDecRef();
}

void
MailFolderCmn::IncRef()
{
   wxLogTrace(TRACE_MF_REF, "MF(%s)::IncRef(): %u -> %u",
              GetName().c_str(), GetNRef(), GetNRef() + 1);

   MObjectRC::IncRef();

   // if the folder had been kept alive artificially before this IncRef(),
   // remove it from the keep alive list as now it will stay opened without our
   // help (notice that we must do it after closing base class IncRef or the
   // object would have been deleted when DecRef() is called implicitly below)
   if ( GetNRef() == 2 && gs_MailFolderCloser )
   {
      MfCloseEntry *entry = gs_MailFolderCloser->GetCloseEntry(this);
      if ( entry )
      {
         gs_MailFolderCloser->Remove(entry);
      }
   }
}

bool
MailFolderCmn::RealDecRef()
{
   wxLogTrace(TRACE_MF_REF, "MF(%s)::DecRef(): %u -> %u",
              GetName().c_str(), GetNRef(), GetNRef() - 1);

#ifdef DEBUG_FOLDER_CLOSE
   // check that the folder is not in the mail folder closer list any more if
   // we're going to delete it
   if ( GetNRef() == 1 )
   {
      if ( gs_MailFolderCloser && gs_MailFolderCloser->GetCloseEntry(this) )
      {
         // we will crash later when removing it from the list!
         FAIL_MSG("deleting folder still in MfCloser list!");
      }
   }
#endif // DEBUG_FOLDER_CLOSE

   return MObjectRC::DecRef();
}

// ----------------------------------------------------------------------------
// MailFolderCmn creation/destruction
// ----------------------------------------------------------------------------

MailFolderCmn::MailFolderCmn()
{
   m_Timer = new MailFolderTimer(this);
   m_LastNewMsgUId = UID_ILLEGAL;

   m_suspendUpdates = 0;

   m_headers = NULL;

   m_frame = NULL;

   m_MEventReceiver = new MfCmnEventReceiver(this);
}

MailFolderCmn::~MailFolderCmn()
{
   ASSERT_MSG( !m_suspendUpdates,
               "mismatch between Suspend/ResumeUpdates()" );

   delete m_Timer;
   delete m_MEventReceiver;
}

// ----------------------------------------------------------------------------
// MailFolderCmn headers
// ----------------------------------------------------------------------------

HeaderInfoList *MailFolderCmn::GetHeaders(void) const
{
   if ( !m_headers )
   {
      MailFolderCmn *self = wxConstCast(this, MailFolderCmn);
      self->m_headers = HeaderInfoList::Create(self);
   }

   m_headers->IncRef();

   return m_headers;
}

// ----------------------------------------------------------------------------
// MailFolderCmn message saving
// ----------------------------------------------------------------------------

// TODO: the functions below should share at least some common code instead of
//       duplicating it! (VZ)

bool
MailFolderCmn::SaveMessagesToFile(const UIdArray *selections,
                                  const String& fileName0,
                                  MWindow *parent)
{
   String fileName = fileName0;

   if ( fileName.empty() )
   {
      // ask the user
      fileName = wxPFileSelector
                 (
                  "MsgSave",
                  _("Choose file to save message to"),
                  NULL, NULL, NULL,
                  _("All files (*.*)|*.*"),
                  wxSAVE | wxOVERWRITE_PROMPT,
                  parent
                 );

      if ( fileName.IsEmpty() )
      {
         // cancelled by user
         return false;
      }
   }
   else // not empty, use this file name
   {
      fileName = strutil_expandpath(fileName0);
   }

   // truncate the file
   wxFile file;
   if ( !file.Create(fileName, TRUE /* overwrite */) )
   {
      wxLogError(_("Could not truncate the existing file."));
      return false;
   }

   // save the messages
   int n = selections->Count();

   MProgressDialog *pd = NULL;
   int threshold = GetProfile() ?
      READ_CONFIG(GetProfile(), MP_FOLDERPROGRESS_THRESHOLD)
      : MP_FOLDERPROGRESS_THRESHOLD_D;
   if ( threshold > 0 && n > threshold )
   {
      wxString msg;
      msg.Printf(_("Saving %d messages to the file '%s'..."),
                 n, fileName0.empty() ? fileName.c_str() : fileName0.c_str());

      pd = new MProgressDialog(GetName(), msg, 2*n, NULL);
   }

   bool rc = true;
   String tmpstr;
   for ( int i = 0; i < n; i++ )
   {
      Message_obj msg = GetMessage((*selections)[i]);

      if ( msg )
      {
         if ( pd )
            pd->Update( 2*i + 1 );

         // iterate over all parts
         int numParts = msg->CountParts();
         for ( int part = 0; part < numParts; part++ )
         {
            size_t size = msg->GetPartSize(part);
            if ( size == 0 )
            {
               // skip empty parts
               continue;
            }

            int partType = msg->GetPartType(part);
            if ( (partType == Message::MSG_TYPETEXT) ||
                 (partType == Message::MSG_TYPEMESSAGE ))
            {
               const char *cptr = msg->GetPartContent(part);
               if( !cptr )
               {
                  FAIL_MSG( "failed to get the content of a text psrt?" );

                  continue;
               }

               tmpstr = strutil_enforceNativeCRLF(cptr);
               size = tmpstr.length();
               if ( file.Write(tmpstr, size) != size )
               {
                  rc = false;
               }
            }
         }

         if ( pd )
            pd->Update( 2*i + 2);
      }
   }

   delete pd;

   return rc;
}

bool
MailFolderCmn::SaveMessages(const UIdArray *selections,
                            MFolder *folder)
{
   CHECK( folder, false, "SaveMessages() needs a valid folder pointer" );

   if ( !CanCreateMessagesInFolder(folder->GetType()) )
   {
      // normally, this should be checked by GUI code, but if it doesn't,
      // detect it here
      wxLogError(_("Impossible to copy messages in the folder '%s'.\n"
                   "You can't create messages in the folders of this type."),
                 folder->GetFullName().c_str());
      return false;
   }

   int n = selections->Count();
   CHECK( n, true, "SaveMessages(): nothing to save" );

   MailFolder *mf = MailFolder::OpenFolder(folder);
   if ( !mf )
   {
      String msg;
      msg.Printf(_("Cannot save messages to folder '%s'."),
                 folder->GetFullName().c_str());
      ERRORMESSAGE((msg));
      return false;
   }

   if ( mf->IsLocked() )
   {
      FAIL_MSG( "Can't SaveMessages() to locked folder" );

      mf->DecRef();

      return false;
   }

   MProgressDialog *pd = NULL;
   int threshold = mf->GetProfile() ?
      READ_CONFIG(mf->GetProfile(), MP_FOLDERPROGRESS_THRESHOLD)
      : MP_FOLDERPROGRESS_THRESHOLD_D;

   if ( threshold > 0 && n > threshold )
   {
      // open a progress window:
      wxString msg;
      msg.Printf(_("Saving %d messages to the folder '%s'..."),
                 n, folder->GetName().c_str());

      pd = new MProgressDialog(
                               mf->GetName(),   // title
                               msg,             // label message
                               2*n,             // range
                               NULL,            // parent
                               false,           // disable parent only
                               true             // allow aborting
                              );
   }

   if ( n > 1 )
   {
      // minimize the number of updates by only doing it once
      mf->SuspendUpdates();
   }

   bool rc = true;
   for ( int i = 0; i < n; i++ )
   {
      if ( pd && !pd->Update(2*i + 1) )
      {
         // cancelled
         break;
      }

      Message *msg = GetMessage((*selections)[i]);
      if ( msg )
      {
         rc &= mf->AppendMessage(*msg);
         msg->DecRef();

         if ( pd && !pd->Update(2*i + 2) )
         {
            // cancelled
            break;
         }
      }
      else
      {
         FAIL_MSG( "copying inexistent message?" );
      }
   }

   if ( n > 1 )
   {
      // make it notice new messages as we disabled them above
      mf->ResumeUpdates();
   }

   // force the folder status update as the number of messages in it changed
   // (CountAllMessages() will call UpdateStatus() internally which sends out an
   // event notifying the GUI about the update)
   MailFolderStatus status;
   (void)mf->CountAllMessages(&status);

   mf->DecRef();

   delete pd;

   return rc;
}

bool
MailFolderCmn::SaveMessages(const UIdArray *selections,
                            const String& folderName)
{
   MFolder_obj folder(folderName);
   if ( !folder.IsOk() )
   {
      wxLogError(_("Impossible to save messages to not existing folder '%s'."),
                 folderName.c_str());

      return false;
   }

   return SaveMessages(selections, folder);
}

// ----------------------------------------------------------------------------
// MailFolderCmn message replying/forwarding
// ----------------------------------------------------------------------------

void
MailFolderCmn::ReplyMessages(const UIdArray *selections,
                             const MailFolder::Params& params,
                             MWindow *parent)
{
   int n = selections->Count();
   for( int i = 0; i < n; i++ )
   {
      Message *msg = GetMessage((*selections)[i]);
      if ( msg )
      {
         ReplyMessage(msg, params, GetProfile(), parent);
         msg->DecRef();
      }
   }
}


void
MailFolderCmn::ForwardMessages(const UIdArray *selections,
                               const MailFolder::Params& params,
                               MWindow *parent)
{
   int i;
   Message *msg;

   int n = selections->Count();
   for(i = 0; i < n; i++)
   {
      msg = GetMessage((*selections)[i]);
      ForwardMessage(msg, params, GetProfile(), parent);
      msg->DecRef();
   }
}

// ----------------------------------------------------------------------------
// MailFolderCmn sorting
// ----------------------------------------------------------------------------

struct SortParams
{
   // the sort order to use
   long order;

   // replace From addresses with To for messages from oneself?
   bool replaceFromWithTo;

   // if replaceFromWithTo, the pointer to our own addresses
   wxArrayString *ownAddresses;
};

// we can only sort one listing at a time currently as we use global data to
// pass information to the C callback used by qsort() - if this proves to be a
// significant limitation, we should reimplement sorting ourselves
static struct SortData
{
   // the struct must be locked while in use
   MMutex mutex;

   // the listing which we're sorting
   HeaderInfoList *hil;

   // the parameters for sorting
   SortParams sortParams;
} gs_SortData;

// return negative number if a < b, 0 if a == b and positive number if a > b
#define CmpNumeric(a, b) ((a)-(b))

static int CompareStatus(int stat1, int stat2)
{
   // deleted messages are considered to be less important than undeleted ones
   if ( stat1 & MailFolder::MSG_STAT_DELETED )
   {
      if ( ! (stat2 & MailFolder::MSG_STAT_DELETED ) )
         return -1;
   }
   else if ( stat2 & MailFolder::MSG_STAT_DELETED )
   {
      return 1;
   }

   /*
      We use a scoring system:

      recent = +1
      unseen   = +1
      answered = -1

      (by now, messages are either both deleted or neither one is)
   */

   int
      score1 = 0,
      score2 = 0;

   if(stat1 & MailFolder::MSG_STAT_RECENT)
      score1 += 1;
   if( !(stat1 & MailFolder::MSG_STAT_SEEN) )
      score1 += 1;
   if( stat1 & MailFolder::MSG_STAT_ANSWERED )
      score1 -= 1;

   if(stat2 & MailFolder::MSG_STAT_RECENT)
      score2 += 1;
   if( !(stat2 & MailFolder::MSG_STAT_SEEN) )
      score2 += 1;
   if( stat2 & MailFolder::MSG_STAT_ANSWERED )
      score2 -= 1;

   return CmpNumeric(score1, score2);
}

extern "C"
{
   static int ComparisonFunction(const void *p1, const void *p2)
   {
      // check that the caller didn't forget to acquire the global data lock
      ASSERT_MSG( gs_SortData.mutex.IsLocked(), "using unlocked gs_SortData" );

      size_t n1 = *(size_t *)p1,
             n2 = *(size_t *)p2;

      HeaderInfo *i1 = gs_SortData.hil->GetItemByIndex(n1),
                 *i2 = gs_SortData.hil->GetItemByIndex(n2);

      // copy it as we're going to modify it while processing
      long sortOrder = gs_SortData.sortParams.order;

      int result = 0;
      while ( result == 0 && sortOrder != 0 )
      {
         long criterium = sortOrder & 0xF;
         sortOrder >>= 4;

         // we rely on MessageSortOrder values being what they are: as _REV
         // version immediately follows the normal order constant, we should
         // reverse the comparison result for odd values of MSO_XXX
         int reverse = criterium % 2;
         switch ( criterium - reverse )
         {
            case MSO_NONE:
               break;

            case MSO_DATE:
               result = CmpNumeric(i1->GetDate(), i2->GetDate());
               break;

            case MSO_SUBJECT:
               {
                  String
                     subj1 = strutil_removeReplyPrefix(i1->GetSubject()),
                     subj2 = strutil_removeReplyPrefix(i2->GetSubject());

                  result = Stricmp(subj1, subj2);
               }
               break;

            case MSO_SENDER:
               {
                  // use "To" if needed
                  String value1, value2;
                  (void)HeaderInfo::GetFromOrTo
                                    (
                                       i1,
                                       gs_SortData.sortParams.replaceFromWithTo,
                                       *gs_SortData.sortParams.ownAddresses,
                                       &value1
                                    );

                  (void)HeaderInfo::GetFromOrTo
                                    (
                                       i2,
                                       gs_SortData.sortParams.replaceFromWithTo,
                                       *gs_SortData.sortParams.ownAddresses,
                                       &value2
                                    );

                  result = Stricmp(value1, value2);
               }
               break;

            case MSO_STATUS:
               result = CompareStatus(i1->GetStatus(), i2->GetStatus());
               break;

            case MSO_SIZE:
               result = CmpNumeric(i1->GetSize(), i2->GetSize());
               break;

            case MSO_SCORE:
               // we don't store score any more in HeaderInfo
#ifdef USE_HEADER_SCORE
               result = CmpNumeric(i1->GetScore(), i2->GetScore());
#endif // USE_HEADER_SCORE
               break;

            default:
               FAIL_MSG("unknown sorting criterium");
         }

         if ( reverse )
         {
            if ( criterium == MSO_NONE )
            {
               // special case: result is 0 for MSO_NONE but -1 (reversed order)
               // for MSO_NONE_REV
               result = -1;
            }
            else // for all other cases just revert the result value
            {
               result = -result;
            }
         }
      }

      return result;
   }
}

// sorting/threading is going to be done by HeaderInfoList itself now
#ifdef BROKEN_BY_VZ

// ----------------------------------------------------------------------------
// MailFolderCmn threading
// ----------------------------------------------------------------------------

/*
   FIXME:

   1. we should use server side threading when possible
   2. this algorithm is O(N^2) and awfully inefficient in its using of
      Set/GetIndentation() for its own temporary data, we should use JWZ
      algorithm for threading instead, it gives better results and is more
      efficient!
 */


#define MFCMN_INDENT1_MARKER   ((size_t)-1)
#define MFCMN_INDENT2_MARKER   ((size_t)-2)

KBLIST_DEFINE(SizeTList, size_t);


#if defined(EXPERIMENTAL_JWZ_THREADING)

//---------------
// JWZ algorithm
//---------------

/// Used for wxLogTrace calls
#define TRACE_JWZ "jwz"

M_LIST(StringList, String);


// --------------------------------------------------------------------
// Threadable
// --------------------------------------------------------------------

/** This class is used as an interface to threadable object. In our
    case, some HeaderInfo. The result of the JWZ algorithm will be to
    compute the index that the message will have in the display, and
    its indentation.
    */
class Threadable {
private:
   /// The message that this instance abstracts
   HeaderInfo *m_hi;
   /** Sequence number of the messages in the sorted list that is
       the input of the algorithm.
       */
   size_t      m_indexInHilp;
   /// Resulting sequence number of the messages
   size_t      m_threadedIndex;
   /// Resulting indentation of the message
   size_t      m_indent;
   /// Is this a dummy message
   bool        m_dummy;
   /// Next brother of the message
   Threadable *m_next;
   /// First child of the message
   Threadable *m_child;
#if defined(DEBUG)
   String      m_subject;
   String      m_refs;
   String      m_id;
#endif
   String     *m_simplifiedSubject;
   bool        m_isReply;
public:
   Threadable(HeaderInfo *hi, size_t index = 0, bool dummy = false);
   ~Threadable();

   void destroy();

   Threadable *makeDummy() {
      return new Threadable(m_hi, m_indexInHilp, true);
   }
   int isDummy() const { return m_dummy; }
   size_t getIndex() { return m_indexInHilp; }
   size_t getThreadedIndex() { return m_threadedIndex; }
   size_t getIndent() { return m_indent; }
   void setThreadedIndex(size_t i) { m_threadedIndex = i; }
   void setIndent(size_t i) { m_indent = i; }

   Threadable *getNext() const { return m_next; }
   Threadable *getChild() const { return m_child; }
   void setNext(Threadable* x) { m_next = x; }
   void setChild(Threadable* x) { m_child = x; }

   String messageThreadID() const;
   kbStringList *messageThreadReferences() const;
   
#if wxUSE_REGEX
   String getSimplifiedSubject(wxRegEx *replyRemover,
                               const String &replacementString) const;
   bool subjectIsReply(wxRegEx *replyRemover,
                       const String &replacementString) const;
#else // wxUSE_REGEX
   String getSimplifiedSubject(bool removeListPrefix) const;
   bool subjectIsReply(bool removeListPrefix) const;
#endif // wxUSE_REGEX
};

inline
Threadable::Threadable(HeaderInfo *hi, size_t index, bool dummy)
   : m_hi(hi)
   , m_indexInHilp(index)
   , m_threadedIndex(0)
   , m_indent(0)
   , m_dummy(dummy)
   , m_next(0)
   , m_child(0)
   , m_simplifiedSubject(0)
   , m_isReply(false)
{
#if defined(DEBUG)
   m_subject = hi->GetSubject();
   m_refs = hi->GetReferences();
   m_id = hi->GetId();
#endif
}

inline
Threadable::~Threadable()
{
   delete m_simplifiedSubject;
}


void Threadable::destroy() {
   static size_t depth = 0;
   CHECK_RET(depth < 1000, "Deep recursion in Threadable::destroy()");
   if (m_child != 0)
   {
      depth++;
      m_child->destroy();
      depth--;
      CHECK_RET(depth >= 0, "Negative recursion depth in Threadable::destroy()");
   }
   delete m_child;
   m_child = 0;
   if (m_next != 0)
      m_next->destroy();
   delete m_next;
   m_next = 0;
}

String Threadable::messageThreadID() const
{
   String id = m_hi->GetId();
   if (!id.empty())
      return id;
   else
      return String("<emptyId:") << (int)this << ">";
}


// --------------------------------------------------------------------
// Computing the list of references to use
// --------------------------------------------------------------------

kbStringList *Threadable::messageThreadReferences() const
{
   // To build a seemingly correct list of references would be
   // quite easy if the References header could be trusted blindly.
   // But this is real life...
   //
   // So we add together the References and In-Reply-To headers, and
   // then remove duplicate references (keping the last of the two) so that:
   //
   //  - If one header is empty, the other is used.
   //
   //  - We circumvent Eudora's behaviour, where the content of In-Reply-To
   //    is not repeated in References.
   //
   //  - If the content of References is somewhat screwed-up, the last
   //    reference in the list will be the one given in In-Reply-To, which,
   //    when present, seems to be more reliable (no ordering problem).
   //
   // XNOFIXME:
   // One still standing problem is if there are conflicting References
   // headers between messages. In this case, the winner is he first one
   // seen by the algorithm. The problem is that the input order of the
   // messages in the algorithm depends on the sorting options chosen by
   // the user. Thus, the output of the algorithm depends on external
   // factors...
   //
   // As In-Reply-To headers are more reliable, but carry less information
   // (they do not allow to reconstruct threads when some messages are
   // missing), one possible way would be to make two passes: one with
   // only In-Reply-To, and then one with References.

   kbStringList *tmp = new kbStringList;
   String refs = m_hi->GetReferences() + m_hi->GetInReplyTo();
   if (refs.empty())
      return tmp;

   const char *s = refs.c_str();

   enum State {
      notInRef,
      openingSeen,
      atSeen
   } state = notInRef;

   size_t start = 0;
   size_t nbChar = refs.Len();
   size_t i = 0;
   while (i < nbChar)
   {
      char current = s[i];
      switch (state)
      {
      case notInRef:
         if (current == '<')
         {
            start = i;
            state = openingSeen;
         }
         break;
      case openingSeen:
         if (current == '@')
            state = atSeen;
         break;
      case atSeen:
         if (current == '>')
         {
            // We found a reference
            String *ref = new String(refs.Mid(start, i+1-start));
            // In case of duplicated reference we keep the last one:
            // It is important that In-Reply-To is the last reference
            // in the list.
            kbStringList::iterator i;
            for (i = tmp->begin(); i != tmp->end(); i++)
            {
               String *previous = *i;
               if (*previous == *ref)
               {
                  tmp->erase(i);
                  break;
               }
            }
            tmp->push_back(ref);
            state = notInRef;
         }
      }
      i++;
   }
   return tmp;
}


// Removes the list prefix [ListName] at the beginning
// of the line (and the spaces around the result)
//
static
String RemoveListPrefix(const String &subject)
{
   String s = subject;
   size_t length = subject.Len();
   if (s.Len() == 0)
      return s;
   if (s[0] != '[')
      return s;
   s = s.AfterFirst(']');
   s = s.Trim(false);
   return s;
}


// XNOFIXME: put this code in strutil.cpp when JWZ
// becomes the default algo.
//
// Removes all occurences of Re: Re[n]: Re(n):
// without considering the case, and remove unneeded
// spaces (start, end, or multiple consecutive spaces).
// Reply prefixes are removed only at the start of the
// string or just after a list prefix (i.e. [ListName]).
// 
#if !wxUSE_REGEX
String
strutil_removeAllReplyPrefixes(const String &isubject,
                               bool &replyPrefixSeen)
{
   enum State {
      notInPrefix,
      inListPrefix,
      rSeen,
      eSeen,
      parenSeen,
      bracketSeen,
      closingSeen,
      colonSeen,
      becomeGarbage,
      garbage
   } state = notInPrefix;

   String subject = isubject;
   size_t length = subject.Len();
   size_t startIndex = 0;
   char *s = new char[length+1];
   size_t nextPos = 0;
   bool listPrefixAlreadySeen = false;
   replyPrefixSeen = false;
   for (size_t i = 0; subject[i] != '\0'; i++)
   {
      if (state == garbage)
         startIndex = i;

      char c = subject[i];
      switch (state)
      {
      case notInPrefix:
         if (c == 'r' || c == 'R')
         {
            state = rSeen;
            startIndex = i;
         } else if (c == ' ')
            startIndex = i+1;
         else if (!listPrefixAlreadySeen && c == '[')
         {
            s[nextPos++] = c;
            state = inListPrefix;
            listPrefixAlreadySeen = true;
         }
         else
            state = becomeGarbage;
         break;
      case inListPrefix:
         s[nextPos++] = c;
         if (c == ']')
         {
            state = notInPrefix;
            // notInPrefix will skip the next space, but we want it
            s[nextPos++] = ' ';
         }
         break;
      case rSeen:
         if (c == 'e' || c == 'E')
            state = eSeen;
         else
            state = becomeGarbage;
         break;
      case eSeen:
         if (c == ':')
            state = colonSeen;
         else if (c == '[')
            state = bracketSeen;
         else if (c == '(')
            state = parenSeen;
         else
            state = becomeGarbage;
         break;
      case parenSeen:
      case bracketSeen:
         if (c == (state == parenSeen ? ')' : ']'))
            state = closingSeen;
         else if (!isdigit(c))
            state = becomeGarbage;
         break;
      case closingSeen:
         if (c == ':')
            state = colonSeen;
         else
            state = becomeGarbage;
         break;
      case colonSeen:
         if (c == ' ')
         {
            replyPrefixSeen = true;
            state = notInPrefix;
            startIndex = i+1;
         }
         else
            state = becomeGarbage;
         break;
      case garbage:
         for (size_t j = startIndex; j <= i; ++j)
            s[nextPos++] = subject[j];
         if (c == ' ') {
            //state = notInPrefix;
            //startIndex = i+1;
         }
         break;
      }

      if (state == becomeGarbage)
      {
         for (size_t j = startIndex; j <= i; ++j)
            s[nextPos++] = subject[j];
         state = garbage;
      }
   }

   s[nextPos] = '\0';
   if (nextPos > 0 && s[nextPos-1] == ' ')
      s[nextPos-1] = '\0';
   String result(s);
   delete [] s;
   return result;
}
#endif // WX_HAVE_REGEX

#if wxUSE_REGEX
String Threadable::getSimplifiedSubject(wxRegEx *replyRemover,
                                        const String &replacementString) const
#else
String Threadable::getSimplifiedSubject(bool removeListPrefix) const
#endif
{
   // First, we remove all reply prefixes occuring near the beginning
   // of the subject (by near, we mean that a list prefix is ignored,
   // and we remove all reply prefixes at the beginning of the resulting
   // string):
   //   Re: Test              ->  Test
   //   Re: [M-Dev] Test      ->  [M-Dev] Test
   //   [M-Dev] Re: Test      ->  [M-Dev] Test
   //   [M-Dev] Re: Re: Test  ->  [M-Dev] Test
   //   Re: [M-Dev] Re: Test  ->  [M-Dev] Test
   //
   // And then, if asked for, we remove the list prefix.
   // This is handy for case where the two messages
   //   Test
   // and
   //   Re: [m-developpers] Test
   // must be threaded together.
   //
   // BTW, we save the results so that they do not have to
   // be computed again
   //
   Threadable *that = (Threadable *)this;    // Remove constness
   if (that->m_simplifiedSubject != 0)
      return *that->m_simplifiedSubject;
   that->m_simplifiedSubject = new String;
#if wxUSE_REGEX
   *(that->m_simplifiedSubject) = m_hi->GetSubject();
   if (!replyRemover)
   {
      that->m_isReply = false;
      return *that->m_simplifiedSubject;
   }
   size_t len = that->m_simplifiedSubject->Len();
   if (replyRemover->Replace(that->m_simplifiedSubject, replacementString, 1))
      that->m_isReply = (that->m_simplifiedSubject->Len() != len);
   else
      that->m_isReply = 0;
#else // wxUSE_REGEX
   *that->m_simplifiedSubject = 
      strutil_removeAllReplyPrefixes(m_hi->GetSubject(),
                                     that->m_isReply);
   if (removeListPrefix)
      *that->m_simplifiedSubject = RemoveListPrefix(*that->m_simplifiedSubject);
#endif // wxUSE_REGEX
   return *that->m_simplifiedSubject;
}


#if wxUSE_REGEX
bool Threadable::subjectIsReply(wxRegEx *replyRemover, 
                                const String &replacementString) const
#else
bool Threadable::subjectIsReply(bool x) const
#endif
{
   Threadable *that = (Threadable *)this;    // Remove constness
   if (that->m_simplifiedSubject == 0)
      getSimplifiedSubject(replyRemover, replacementString);
   return that->m_isReply;
}



// --------------------------------------------------------------------
// ThreadContainer
// --------------------------------------------------------------------

#if defined(DEBUG)
// Count of the number of instances alive
// Should be 0 at the end of the threading
static int NumberOfThreadContainers = 0;
#endif



class ThreadContainer {
private:
   Threadable      *m_threadable;
   ThreadContainer *m_parent;
   ThreadContainer *m_child;
   ThreadContainer *m_next;

public:
   ThreadContainer(Threadable *th = 0);
   ~ThreadContainer();

   Threadable *getThreadable() const { return m_threadable; }
   ThreadContainer *getParent() const { return m_parent; }
   ThreadContainer *getChild() const { return m_child; }
   ThreadContainer *getNext() const { return m_next; }
   void setThreadable(Threadable *x) { m_threadable = x; }
   void setParent(ThreadContainer *x) { m_parent = x; }
   void setChild(ThreadContainer *x) { m_child = x; }
   void setNext(ThreadContainer *x) { m_next = x; }

   /** Returns the index to be used to compare instances
       of ThreadContainer to determine their ordering
       */
   size_t getIndex() const;
   /** Adds a ThreadContainer to the children of the caller,
       taking their index into account.
       */
   void addAsChild(ThreadContainer *c);

   /** Returns true if target is a children (maybe deep in the
       tree) of the caller.
       */
   bool findChild(ThreadContainer *target, bool withNexts = false) const;

   /// Recursively reverses the order of the the caller's children
   void reverseChildren();

   /** Computes and saves (in the Threadable they store) the indentation
       and the numbering of the messages
       */
   void flush(size_t &threadedIndex, size_t indent, bool indentIfDummyNode);

   /// Recursively destroys a tree
   void destroy();
};


inline
ThreadContainer::ThreadContainer(Threadable *th)
   : m_threadable(th)
   , m_parent(0)
   , m_child(0)
   , m_next(0)
{
#if defined(DEBUG)
   NumberOfThreadContainers++;
#endif
}


inline
ThreadContainer::~ThreadContainer()
{
   //if (m_threadable != NULL && m_threadable->isDummy())
   //   delete m_threadable;
#if defined(DEBUG)
   NumberOfThreadContainers--;
#endif
}


size_t ThreadContainer::getIndex() const
{
   const size_t foolish = 1000000000;
   static size_t depth = 0;
   CHECK(depth < 1000, foolish,
         "Deep recursion in ThreadContainer::getSmallestIndex()");
   Threadable *th = getThreadable();
   if (th != 0)
      return th->getIndex();
   else
   {
      if (getChild() != 0)
      {
         depth++;
         size_t index = getChild()->getIndex();
         depth--;
         CHECK(depth >= 0, foolish,
               "Negative recursion depth in ThreadContainer::getSmallestIndex()");
         return index;
      }
      else
         return foolish;
   }
}


void ThreadContainer::addAsChild(ThreadContainer *c)
{
   // Takes into account the indices so that the new kid
   // is inserted in the correct position
   CHECK_RET(c->getThreadable(), "No threadable in ThreadContainer::addAsChild()");
   size_t cIndex = c->getThreadable()->getIndex();
   ThreadContainer *prev = 0;
   ThreadContainer *current = getChild();
   while (current != 0)
   {
      if (current->getIndex() > cIndex)
         break;
      prev = current;
      current = current->getNext();
   }
   c->setParent(this);
   c->setNext(current);
   if (prev != 0)
      prev->setNext(c);
   else
      setChild(c);
}


bool ThreadContainer::findChild(ThreadContainer *target, bool withNexts) const
{
   static size_t depth = 0;
   CHECK(depth < 1000, true, "Deep recursion in ThreadContainer::findChild()");
   if (m_child == target)
      return true;
   if (withNexts && m_next == target)
      return true;
   if (m_child != 0)
   {
      depth++;
      bool found = m_child->findChild(target, true);
      depth--;
      CHECK(depth >= 0, true, "Negative recursion depth in ThreadContainer::findChild()");
      if (found)
         return true;
   }
   if (withNexts && m_next != 0)
   {
      bool found = m_next->findChild(target, true);
      if (found)
         return true;
   }
   return false;
}


void ThreadContainer::reverseChildren()
{
   static size_t depth = 0;
   CHECK_RET(depth < 1000, "Deep recursion in ThreadContainer::reverseChildren()");

   if (m_child != 0)
   {
      ThreadContainer *kid, *prev, *rest;
      for (prev = 0, kid = m_child, rest = kid->getNext();
           kid != 0;
           prev = kid, kid = rest, rest = (rest == 0 ? 0 : rest->getNext()))
         kid->setNext(prev);
      m_child = prev;

      depth++;
      for (kid = m_child; kid != 0; kid = kid->getNext())
         kid->reverseChildren();
      depth--;
      CHECK_RET(depth >= 0, "Negative recursion depth in ThreadContainer::reverseChildren()");
   }
}


void ThreadContainer::flush(size_t &threadedIndex, size_t indent, bool indentIfDummyNode)
{
   static size_t depth = 0;
   CHECK_RET(depth < 1000, "Deep recursion in ThreadContainer::flush()");
   CHECK_RET(m_threadable != 0, "No threadable in ThreadContainer::flush()");
   m_threadable->setChild(m_child == 0 ? 0 : m_child->getThreadable());
   if (!m_threadable->isDummy())
   {
      m_threadable->setThreadedIndex(threadedIndex++);
      m_threadable->setIndent(indent);
   }
   if (m_child != 0)
   {
      depth++;
      if (indentIfDummyNode)
         m_child->flush(threadedIndex, indent+1, indentIfDummyNode);
      else
         m_child->flush(threadedIndex,
                        indent+(m_threadable->isDummy() ? 0 : 1),
                        indentIfDummyNode);
      depth--;
      CHECK_RET(depth >= 0, "Negative recursion depth in ThreadContainer::flush()");
   }
   if (m_threadable != 0)
      m_threadable->setNext(m_next == 0 ? 0 : m_next->getThreadable());
   if (m_next != 0)
      m_next->flush(threadedIndex, indent, indentIfDummyNode);
}


void ThreadContainer::destroy()
{
   static size_t depth = 0;
   CHECK_RET(depth < 1000, "Deep recursion in ThreadContainer::destroy()");
   if (m_child != 0)
   {
      depth++;
      m_child->destroy();
      depth--;
      CHECK_RET(depth >= 0, "Negative recursion depth in ThreadContainer::destroy()");
   }
   delete m_child;
   m_child = 0;
   if (m_next != 0)
      m_next->destroy();
   delete m_next;
   m_next = 0;
}




// --------------------------------------------------------------------
// Threader : a JWZ threader
// --------------------------------------------------------------------

class Threader {
private:
   ThreadContainer *m_root;
   HASHTAB         *m_idTable;
   int              m_bogusIdCount;

   // Options
   bool m_gatherSubjects;
   wxRegEx *m_replyRemover;
   String m_replacementString;
   bool m_breakThreadsOnSubjectChange;
   bool m_indentIfDummyNode;
   bool m_removeListPrefixGathering;
   bool m_removeListPrefixBreaking;

public:
   Threader()
      : m_root(0)
      , m_idTable(0)
      , m_bogusIdCount(0)
      , m_gatherSubjects(true)
      , m_replyRemover(0)
      , m_replacementString()
      , m_breakThreadsOnSubjectChange(true)
      , m_indentIfDummyNode(false)
      , m_removeListPrefixGathering(true)
      , m_removeListPrefixBreaking(true)
   {}
   
   ~Threader()
   {
      delete m_replyRemover;
   }
   
   /** Does all the job. Input is a list of messages, output
       is a dummy root message, with all the tree under.

       If shouldGatherSubjects is true, all messages that
       have the same non-empty subject will be considered
       to be part of the same thread.

       If indentIfDummyNode is true, messages under a dummy
       node will be indented.
       */
   Threadable *thread(Threadable *threadableRoot);

   void setGatherSubjects(bool x) { m_gatherSubjects = x; }
   void setSimplifyingRegex(const String &x) 
   { 
      if (!m_replyRemover)
         m_replyRemover = new wxRegEx(x);
      else
         m_replyRemover->Compile(x);
   }
   void setReplacementString(const String &x) { m_replacementString = x; }
   void setBreakThreadsOnSubjectChange(bool x) { m_breakThreadsOnSubjectChange = x; }
   void setIndentIfDummyNodet(bool x) { m_indentIfDummyNode = x; }
   void setRemoveListPrefixGathering(bool x) { m_removeListPrefixGathering = x; }
   void setRemoveListPrefixBreaking(bool x) { m_removeListPrefixBreaking = x; }
   
private:
   void buildContainer(Threadable *threadable);
   void findRootSet();
   void pruneEmptyContainers(ThreadContainer *parent,
                             bool fromBreakThreads);
   size_t collectSubjects(HASHTAB*, ThreadContainer *, bool recursive);
   void gatherSubjects();
   void breakThreads(ThreadContainer*);

   // An API on top of the hash-table
   HASHTAB *create(size_t) const;
   ThreadContainer* lookUp(HASHTAB*, const String&) const;
   void add(HASHTAB*, const String&, ThreadContainer*) const;
   void destroy(HASHTAB **) const;
};





static bool SeemsPrime(size_t n)
{
   if (n % 2 == 0)
      return false;
   if (n % 3 == 0)
      return false;
   if (n % 5 == 0)
      return false;
   if (n % 7 == 0)
      return false;
   if (n % 11 == 0)
      return false;
   if (n % 13 == 0)
      return false;
   if (n % 17 == 0)
      return false;
   if (n % 19 == 0)
      return false;
   return true;
}


static size_t FindPrime(size_t n)
{
   if (n % 2 == 0)
      n++;
   while (!SeemsPrime(n))
      n += 2;
   return n;
}


HASHTAB *Threader::create(size_t size) const
{
   // Find a correct size for an hash-table
   size_t aPrime = FindPrime(size);
   // Create it
   return hash_create(aPrime);
}


void Threader::add(HASHTAB *hTable, const String &str, ThreadContainer *container) const
{
   // As I do not want to ensure that the String instance given
   // as key will last at least as long as the table, I copy its
   // value.
   char *s = new char[str.Len()+1];
   strcpy(s, str.c_str());
   hash_add(hTable, s, container, 0);
}


ThreadContainer *Threader::lookUp(HASHTAB *hTable, const String &s) const
{
   void** data = hash_lookup(hTable, (char*) s.c_str());
   if (data != 0)
      return (ThreadContainer*)data[0];
   else
      return 0;
}


void Threader::destroy(HASHTAB ** hTable) const
{
   // Delete all the strings that have been used as key
   HASHENT *ent;
   size_t i;
   for (i = 0; i < (*hTable)->size; i++)
      for (ent = (*hTable)->table[i]; ent != 0; ent = ent->next)
         delete [] ent->name;
      // and destroy the hash-table
      hash_destroy(hTable);
}


Threadable *Threader::thread(Threadable *threadableRoot)
{
   if (threadableRoot == 0)
      return 0;

#if defined(DEBUG)
   // In case we leaked in a previous run of the algo, reset the count
   NumberOfThreadContainers = 0;
#endif

   // Count the number of messages to thread, and verify
   // the input list
   Threadable *th = threadableRoot;
   size_t thCount = 0;
   for (; th != 0; th = th->getNext())
   {
      VERIFY(th->getChild() == 0, "Bad input list in Threader::thread()");
      thCount++;
   }
   // Make an hash-table big enough to hold all the instances of
   // ThreadContainer that will be created during the run.
   // Note that some dummy containers will be created also, so
   // don't be shy...
   m_idTable = create(thCount*2);

   // Scan the list, and build the corresponding ThreadContainers
   // This step will:
   //  - Build a ThreadContainer for the Threadable given as argument
   //  - Scan the references of the message, and build on the fly some
   //    ThreadContainer for them, if the corresponding messages have not
   //    already been processed
   //  - Store all those containers in the hash-table
   //  - Build their parent/child relations
   for (th = threadableRoot; th != 0; th = th->getNext())
   {
      VERIFY(th->getIndex() >= 0, "Negative index in Threader::thread()");
      VERIFY(th->getIndex() < thCount, "Too big in Threader::thread()");
      // As things are now, dummy messages won't get past the algorithm
      // and won't be displayed. Thus we should not get them back when
      // re-threading this folder.
      // When this changes, we will have to take care to skip them.
      VERIFY(!th->isDummy(), "Dummy message in Threader::thread()");
      buildContainer(th);
   }

   // Create a dummy root ThreadContainer (stored in m_root) and
   // scan the content of the hash-table to make all parent-less
   // containers (i.e. first message in a thread) some children of
   // this root.
   findRootSet();

   // We are finished with this hash-table
   destroy(&m_idTable);

   // Remove all the useless containers (e.g. those that are not
   // in the root-set, have no message, but have a child)
   pruneEmptyContainers(m_root, false);

   // If asked to, gather all the messages that have the same
   // non-empty subjet in the same thread.
   if (m_gatherSubjects)
      gatherSubjects();

   ASSERT(m_root->getNext() == 0); // root node has a next ?!

   // Build dummy messages for the nodes that have no message.
   // Those can only appear in the root set.
   ThreadContainer *thr;
   for (thr = m_root->getChild(); thr != 0; thr = thr->getNext())
      if (thr->getThreadable() == 0)
         thr->setThreadable(thr->getChild()->getThreadable()->makeDummy());

   if (m_breakThreadsOnSubjectChange)
      breakThreads(m_root);

   // Prepare the result to be returned: the root of the
   // *Threadable* tree that we will build by flushing the
   // ThreadContainer tree structure.
   Threadable *result = (m_root->getChild() == 0
      ? 0
      : m_root->getChild()->getThreadable());

   // Compute the index of each message (the line it will be
   // displayed to when threaded) and its indentation.
   size_t threadedIndex = 0;
   if (m_root->getChild() != 0)
      m_root->getChild()->flush(threadedIndex, 0, m_indentIfDummyNode);

   // Destroy the ThreadContainer structure.
   m_root->destroy();
   delete m_root;
   ASSERT(NumberOfThreadContainers == 0); // Some ThreadContainers leak

   return result;
}



void Threader::buildContainer(Threadable *th)
{
   String id = th->messageThreadID();
   ASSERT(!id.IsEmpty());
   ThreadContainer *container = lookUp(m_idTable, id);

   if (container != 0)
   {
      if (container->getThreadable() != 0)
      {
         id = String("<bogusId:") << m_bogusIdCount++ << ">";
         container = 0;
      } else {
         container->setThreadable(th);
      }
   }

   if (container == 0)
   {
      container = new ThreadContainer(th);
      add(m_idTable, id, container);
   }

   ThreadContainer *parentRefCont = 0;
   kbStringList *refs = th->messageThreadReferences();
   kbStringList::iterator i;
   for (i = refs->begin(); i != refs->end(); i++)
   {
      String *ref = *i;
      ThreadContainer *refCont = lookUp(m_idTable, *ref);
      if (refCont == 0)
      {
         refCont = new ThreadContainer();
         add(m_idTable, *ref, refCont);
      }

      if ((parentRefCont != 0) &&
          (refCont->getParent() == 0) &&
          (parentRefCont != refCont) &&
          !parentRefCont->findChild(refCont))
      {
         if (!refCont->findChild(parentRefCont))
         {
            refCont->setParent(parentRefCont);
            refCont->setNext(parentRefCont->getChild());
            parentRefCont->setChild(refCont);
         } else
         {
            // refCont should be a child of parentRefCont (because
            // refCont is seens in the references after parentRefCont
            // but refCont is the parent of parentRefCont.
            // This must be because some References fields contradict
            // each other about the order. Let's keep the first
            // encountered.
            int i = 0;
         }
      }
      parentRefCont = refCont;
   }
   if ((parentRefCont != 0) &&
       ((parentRefCont == container) ||
        container->findChild(parentRefCont)))
      parentRefCont = 0;

   if (container->getParent() != 0) {
      ThreadContainer *rest, *prev;
      for (prev = 0, rest = container->getParent()->getChild();
           rest != 0;
           prev = rest, rest = rest->getNext())
         if (rest == container)
            break;
         ASSERT(rest != 0); // Did not find a container in parent
         if (prev == 0)
            container->getParent()->setChild(container->getNext());
         else
            prev->setNext(container->getNext());
         container->setNext(0);
         container->setParent(0);
   }

   if (parentRefCont != 0)
   {
      ASSERT(!container->findChild(parentRefCont));
      container->setParent(parentRefCont);
      container->setNext(parentRefCont->getChild());
      parentRefCont->setChild(container);
   }
   delete refs;
   //wxLogTrace(TRACE_JWZ, "Leaving Threader::buildContainer(Threadable 0x%lx)", th);
}


void Threader::findRootSet()
{
   wxLogTrace(TRACE_JWZ, "Entering Threader::findRootSet()");
   m_root = new ThreadContainer();
   HASHENT *ent;
   size_t i;
   for (i = 0; i < m_idTable->size; i++)
      for (ent = m_idTable->table[i]; ent != 0; ent = ent->next) {
         ThreadContainer *container = (ThreadContainer*)ent->data[0];
         if (container->getParent() == 0)
         {
            // This one will be in the root set
            ASSERT(container->getNext() == 0);
            // Find the correct position to insert it, so that the
            // order given to us is respected
            ThreadContainer *c = m_root->getChild();
            if (c == 0)
            {
               // This is the first one found
               container->setNext(m_root->getChild());
               m_root->setChild(container);
            } else {
               size_t index = container->getIndex();
               ThreadContainer *prev = 0;
               while ((c != 0) &&
                  (c->getIndex() < index))
               {
                  prev = c;
                  c = c->getNext();
               }
               ASSERT(container->getNext() == 0);
               container->setNext(c);
               if (prev == 0)
               {
                  ASSERT(m_root->getChild() == c);
                  m_root->setChild(container);
               } else
               {
                  ASSERT(prev->getNext() == c);
                  prev->setNext(container);
               }
            }
         }
      }
}


void Threader::pruneEmptyContainers(ThreadContainer *parent,
                                    bool fromBreakThreads)
{
   ThreadContainer *c, *prev, *next;
   for (prev = 0, c = parent->getChild(), next = c->getNext();
   c != 0;
   prev = c, c = next, next = (c == 0 ? 0 : c->getNext()))
   {
      if ((c->getThreadable() == 0 ||
           c->getThreadable()->isDummy()) &&
          (c->getChild() == 0))
      {
         if (prev == 0)
            parent->setChild(c->getNext());
         else
            prev->setNext(c->getNext());
         if (fromBreakThreads)
            delete c->getThreadable();
         delete c;
         c = prev;

      } else if ((c->getThreadable() == 0 ||
                  c->getThreadable()->isDummy()) &&
                 (c->getChild() != 0) &&
                 ((c->getParent() != 0) ||
                  (c->getChild()->getNext() == 0)))
      {
         ThreadContainer *kids = c->getChild();
         if (prev == 0)
            parent->setChild(kids);
         else
            prev->setNext(kids);

         ThreadContainer *tail;
         for (tail = kids; tail->getNext() != 0; tail = tail->getNext())
            tail->setParent(c->getParent());

         tail->setParent(c->getParent());
         tail->setNext(c->getNext());

         next = kids;
         
         if (fromBreakThreads)
            delete c->getThreadable();

         delete c;
         c = prev;
         
      } else if (c->getChild() != 0 && !fromBreakThreads)
         
         pruneEmptyContainers(c, false);
   }
}


// If one message in the root set has the same subject than another
// message (not necessarily in the root-set), merge them.
// This is so that messages which don't have Reference headers at all
// still get threaded (to the extent possible, at least.)
//
void Threader::gatherSubjects()
{
   size_t count = 0;
   ThreadContainer *c = m_root->getChild();
   for (; c != 0; c = c->getNext())
      count++;

   // Make the hash-table large enough. Let's consider
   // that there are not too many (not more than one per
   // thread) subject changes.
   HASHTAB *subjectTable = create(count*2);

   // Collect the subjects in all the tree
   count = collectSubjects(subjectTable, m_root, true);

   if (count == 0)            // If the table is empty, we're done.
   {
      destroy(&subjectTable);
      return;
   }

   // The sujectTable is now populated with one entry for each subject.
   // Now iterate over the root set, and gather together the difference.
   //
   ThreadContainer *prev, *rest;
   for (prev = 0, c = m_root->getChild(), rest = c->getNext();
        c != 0;
        prev = c, c = rest, rest = (rest == 0 ? 0 : rest->getNext()))
   {
      Threadable *th = c->getThreadable();
      if (th == 0)   // might be a dummy -- see above
      {
          th = c->getChild()->getThreadable();
          ASSERT(th != NULL);
      }
      
#if wxUSE_REGEX
      String subject = th->getSimplifiedSubject(m_replyRemover,
                                                m_replacementString);
#else
      String subject = th->getSimplifiedSubject(m_removeListPrefixGathering);
#endif
      
      // Don't thread together all subjectless messages; let them dangle.
      if (subject.empty())
         continue;

      ThreadContainer *old = lookUp(subjectTable, subject);
      if (old == c)    // oops, that's us
         continue;

      // Ok, so now we have found another container in the root set with
      // the same subject. There are a few possibilities:
      //
      // - If both are dummies, append one's children to the other, and remove
      //   the now-empty container.
      //
      // - If one container is a dummy and the other is not, make the non-dummy
      //   one be a child of the dummy, and a sibling of the other "real"
      //   messages with the same subject (the dummy's children.)
      //
      // - If that container is a non-dummy, and that message's subject does
      //   not begin with "Re:", but *this* message's subject does, then make
      //   this be a child of the other.
      //
      // - If that container is a non-dummy, and that message's subject begins
      //   with "Re:", but *this* message's subject does *not*, then make that
      //   be a child of this one -- they were misordered. (This happens
      //   somewhat implicitely, since if there are two messages, one with "Re:"
      //   and one without, the one without will be in the hash-table,
      //   regardless of the order in which they were seen.)
      //
      // - If that container is in the root-set, make a new dummy container and
      //   make both messages be a child of it.  This catches the both-are-replies
      //   and neither-are-replies cases, and makes them be siblings instead of
      //   asserting a hierarchical relationship which might not be true.
      //
      // - Otherwise (that container is not in the root set), make this container
      //   be a sibling of that container.
      //

      // Remove the "second" message from the root set.
      if (prev == 0)
         m_root->setChild(c->getNext());
      else
         prev->setNext(c->getNext());
      c->setNext(0);

      if (old->getThreadable() == 0 && c->getThreadable() == 0)
      {
         // They're both dummies; merge them by
         // adding all the children of c to old
         ThreadContainer *kid = c->getChild();
         while (kid != NULL)
         {
            ThreadContainer *next = kid->getNext();
            old->addAsChild(kid);
            kid = next;
         }
         delete c;
      }
      else if (old->getThreadable() == 0 ||               // old is empty, or
               (c->getThreadable() != 0 &&
#if wxUSE_REGEX
               c->getThreadable()->subjectIsReply(m_replyRemover,
                                                  m_replacementString) &&   // c has "Re:"
                !old->getThreadable()->subjectIsReply(m_replyRemover,
                                                      m_replacementString))) // and old does not
#else
               c->getThreadable()->subjectIsReply(m_removeListPrefixGathering) &&   // c has "Re:"
                !old->getThreadable()->subjectIsReply(m_removeListPrefixGathering))) // and old does not
#endif
      {
         // Make this message be a child of the other.
         old->addAsChild(c);
      }
      else
      {
         // old may be not in the root-set, as we searched for subjects
         // in all the tree. If this is the case, we must not create a dummy
         // node, but rather insert c as a sibling of old.

         if (old->getParent() == NULL)
         {
            // Make the old and new message be children of a new dummy container.
            // We do this by creating a new container object for old->msg and
            // transforming the old container into a dummy (by merely emptying it
            // so that the hash-table still points to the one that is at depth 0
            // instead of depth 1.

            ThreadContainer *newC = new ThreadContainer(old->getThreadable());
            newC->setChild(old->getChild());
            ThreadContainer *tail = newC->getChild();
            for (; tail != 0; tail = tail->getNext())
               tail->setParent(newC);

            old->setThreadable(0);
            old->setChild(0);
            c->setParent(old);
            newC->setParent(old);

            // Determine the one, between c and newC, that must be displayed
            // first, by considering their index. Reorder them so that c is the one
            // with smallest index (to be displayed first)
            if (c->getThreadable()->getIndex() >
               newC->getThreadable()->getIndex())
            {
               ThreadContainer *tmp = c;
               c = newC;
               newC = tmp;
            }

            // Make old point to its kids
            old->setChild(c);
            c->setNext(newC);
         }
         else
            old->getParent()->addAsChild(c);
      }

      // We've done a merge, so keep the same 'prev' next time around.
      c = prev;
   }

   destroy(&subjectTable);
}


size_t Threader::collectSubjects(HASHTAB *subjectTable,
                                 ThreadContainer *parent,
                                 bool recursive)
{
   static size_t depth = 0;
   VERIFY(depth < 1000, "Deep recursion in Threader::collectSubjects()");
   size_t count = 0;
   ThreadContainer *c;
   for (c = parent->getChild(); c != 0; c = c->getNext())
   {
      Threadable *cTh = c->getThreadable();
      Threadable *th = cTh;

      // If there is no threadable, this is a dummy node in the root set.
      // Only root set members may be dummies, and they always have at least
      // two kids. Take the first kid as representative of the subject.
      if (th == 0)
      {
         ASSERT(c->getParent() == 0);          // In root-set
         th = c->getChild()->getThreadable();
      }
      ASSERT(th != 0);
      
#if wxUSE_REGEX
      String subject = th->getSimplifiedSubject(m_replyRemover,
                                                m_replacementString);
#else
      String subject = th->getSimplifiedSubject(m_removeListPrefixGathering);
#endif
      if (subject.empty())
         continue;

      ThreadContainer *old = lookUp(subjectTable, subject);

      // Add this container to the table if:
      //  - There is no container in the table with this subject, or
      //  - This one is a dummy container and the old one is not: the dummy
      //    one is more interesting as a root, so put it in the table instead
      //  - The container in the table has a "Re:" version of this subject,
      //    and this container has a non-"Re:" version of this subject.
      //    The non-"Re:" version is the more interesting of the two.
      //
      if (old == 0 ||
          (cTh == 0 && old->getThreadable() != 0) ||
          (old->getThreadable() != 0 &&  
#if wxUSE_REGEX
           old->getThreadable()->subjectIsReply(m_replyRemover,
                                                m_replacementString) &&
#else
           old->getThreadable()->subjectIsReply(m_removeListPrefixGathering) &&
#endif
           cTh != 0 && 
#if wxUSE_REGEX
           !cTh->subjectIsReply(m_replyRemover, m_replacementString)))
#else
           !cTh->subjectIsReply(m_removeListPrefixGathering)))
#endif
      {
         // The hash-table we use does not offer to replace (nor remove)
         // an entry. But adding a new entry with the same key will 'hide'
         // the old one. This is just not as efficient if there are collisions
         // but we do not care for now.
         add(subjectTable, subject, c);
         count++;
      }

      if (recursive)
      {
         depth++;
         count += collectSubjects(subjectTable, c, true);
         depth--;
         VERIFY(depth >= 0, "Negative recursion depth in Threader::collectSubjects()");
      }
   }

   // Return number of subjects found
   return count;
}


void Threader::breakThreads(ThreadContainer* c)
{
   static size_t depth = 0;
   CHECK_RET(depth < 1000, "Deep recursion in Threader::breakThreads()");

   // Dummies should have been built
   CHECK_RET((c == m_root) || (c->getThreadable() != NULL),
             "No threadable in Threader::breakThreads()");

   // If there is no parent, there is no thread to break.
   if (c->getParent() != NULL)
   {
      // Dummies should have been built
      CHECK_RET(c->getParent()->getThreadable() != NULL,
                "No parent's threadable in Threader::breakThreads()");

      ThreadContainer *parent = c->getParent();

#if wxUSE_REGEX
      if (c->getThreadable()->getSimplifiedSubject(m_replyRemover,
                                                   m_replacementString) !=
          parent->getThreadable()->getSimplifiedSubject(m_replyRemover,
                                                        m_replacementString))
#else
      if (c->getThreadable()->getSimplifiedSubject(m_removeListPrefixGathering) !=
          parent->getThreadable()->getSimplifiedSubject(m_removeListPrefixGathering))
#endif
      {
         // Subject changed. Break the thread
         if (parent->getChild() == c)
            parent->setChild(c->getNext());
         else
         {
            ThreadContainer *prev = parent->getChild();
            CHECK_RET(parent->findChild(c),
                      "Not child of its parent in Threader::breakThreads()");
            while (prev->getNext() != c)
               prev = prev->getNext();
            prev->setNext(c->getNext());
         }
         m_root->addAsChild(c);
         c->setParent(0);

         // If the parent was a dummy node and c has other siblings,
         // we should remove the parent.
         if (parent->getThreadable()->isDummy())
         {
            ThreadContainer *grandParent = parent->getParent();
            if (grandParent == NULL)
               grandParent = m_root;
            pruneEmptyContainers(grandParent, true);
         }
      }
   }

   ThreadContainer *kid = c->getChild();
   while (kid != NULL)
   {
      // We save the next to check, as kid may disappear of this thread
      ThreadContainer *next = kid->getNext();

      depth++;
      breakThreads(kid);
      depth--;
      VERIFY(depth >= 0, "Negative recursion depth in Threader::breakThreads()");

      kid = next;
   }
}

// Build the input struture for the threader: a list
// of all the messages to thread.
static Threadable *BuildThreadableList(HeaderInfoList *hilp)
{
   wxLogTrace(TRACE_JWZ, "Entering BuildThreadableList");
   ASSERT(hilp);
   HeaderInfoList_obj hil(hilp);
   size_t count = hil->Count();
   size_t i;
   Threadable *root = 0;
   for (i = 0; i < count; ++i) {
      HeaderInfo *hi = hil[i];
      Threadable *item = new Threadable(hi, i);
      item->setNext(root);
      root = item;
   }
   wxLogTrace(TRACE_JWZ, "Leaving BuildThreadableList");
   return root;
}


// Scan the Threadable tree and fill the arrays needed to
// correctly display the threads
//
static void FlushThreadable(Threadable *t,
                            size_t indices[],
                            size_t indents[]
#if defined(DEBUG)
                            , bool seen[]
#endif
                            )
{
   if (!t->isDummy())
   {
      size_t threadedIndex = t->getThreadedIndex();
      size_t index = t->getIndex();
      indices[threadedIndex] = index;
      indents[threadedIndex] = t->getIndent();
#if defined(DEBUG)
      seen[index] = true;
#endif
   }
   if (t->getChild())
#if defined(DEBUG)
      FlushThreadable(t->getChild(), indices, indents, seen);
#else
      FlushThreadable(t->getChild(), indices, indents);
#endif
   if (t->getNext())
#if defined(DEBUG)
      FlushThreadable(t->getNext(), indices, indents, seen);
#else
   FlushThreadable(t->getNext(), indices, indents);
#endif
}


static void ComputeThreadedIndicesWithJWZ(MailFolder *mf,
                                          HeaderInfoList *hilp,
                                          size_t indices[],
                                          size_t indents[])
{
   wxLogTrace(TRACE_JWZ, "Entering BuildDependantsWithJWZ");
   Threadable *threadableRoot = BuildThreadableList(hilp);
   size_t count = 0;
   Threadable *th = threadableRoot;
   for (; th != 0; th = th->getNext())
    count++;
   Threader *threader = new Threader;

   bool gatherSubjects = false;
   String simplifyingRegex;
   String replacementString;
   bool breakThreadsOnSubjectChange = false;
   bool indentIfDummyNode = true;
   bool removeListPrefixGathering = false;
   bool removeListPrefixBreaking = false;
   Profile *profile = mf->GetProfile();
   if (profile != NULL)
   {
      gatherSubjects = (READ_CONFIG(profile, MP_MSGS_GATHER_SUBJECTS) == 1);
      breakThreadsOnSubjectChange = (READ_CONFIG(profile, MP_MSGS_BREAK_THREAD) == 1);
#if wxUSE_REGEX
      simplifyingRegex = READ_CONFIG(profile, MP_MSGS_SIMPLIFYING_REGEX);
      replacementString = READ_CONFIG(profile, MP_MSGS_REPLACEMENT_STRING);
#else
      removeListPrefixGathering = (READ_CONFIG(profile, MP_MSGS_REMOVE_LIST_PREFIX_GATHERING) == 1);
      removeListPrefixBreaking = (READ_CONFIG(profile, MP_MSGS_REMOVE_LIST_PREFIX_BREAKING) == 1);
#endif
      indentIfDummyNode = (READ_CONFIG(profile, MP_MSGS_INDENT_IF_DUMMY) == 1);
   }

   threader->setGatherSubjects(gatherSubjects);
   threader->setSimplifyingRegex(simplifyingRegex);
   threader->setReplacementString(replacementString);
   threader->setBreakThreadsOnSubjectChange(breakThreadsOnSubjectChange);
   threader->setIndentIfDummyNodet(indentIfDummyNode);
   threader->setRemoveListPrefixGathering(removeListPrefixGathering);
   threader->setRemoveListPrefixBreaking(removeListPrefixBreaking);
   
   threadableRoot = threader->thread(threadableRoot);

   if (threadableRoot != 0)
   {
#if defined(DEBUG)
      bool *seen = new bool[count];
      for (size_t i = 0; i < count; ++i)
         seen[i] = false;
      FlushThreadable(threadableRoot, indices, indents, seen);
      for (i = 0; i < count; ++i)
         ASSERT(seen[i]);
      for (i = 0; i < count; ++i)
         if (indices[i] > count)
         {
            indices[i] = 0;
            indents[i] = 0;
         }
      delete [] seen;

#else
      FlushThreadable(threadableRoot, indices, indents);
#endif
      threadableRoot->destroy();
   }
   else
   {
      // It may be an error...
      for (size_t i = 0; i < count; ++i)
      {
         indices[i] = i;
         indents[i] = 0;
      }
   }

   delete threadableRoot;
   delete threader;
   wxLogTrace(TRACE_JWZ, "Leaving BuildDependantsWithJWZ");
}

#endif //EXPERIMENTAL_JWZ_THREADING

static void AddDependents(size_t &idx,
                          int level,
                          int i,
                          size_t indices[],
                          size_t indents[],
                          HeaderInfoList *hilp,
                          SizeTList dependents[])
{
   SizeTList::iterator it;

   for(it = dependents[i].begin(); it != dependents[i].end(); it++)
   {
      size_t idxToAdd = **it;
      // a non-zero index means, this one has been added already
      if((*hilp)[idxToAdd]->GetIndentation() != MFCMN_INDENT1_MARKER)
      {
         ASSERT(idx < (*hilp).Count());
         indices[idx] = idxToAdd;
         indents[idx] = level;
         (*hilp)[idxToAdd]->SetIndentation(MFCMN_INDENT1_MARKER);
         idx++;
         // after adding an element, we need to check if we have to add
         // any of its own dependents:
         AddDependents(idx, level+1, idxToAdd, indices, indents, hilp, dependents);
      }
   }
}

static void ThreadMessages(MailFolder *mf, HeaderInfoList *hilp)
{
   CHECK_RET( hilp, "no listing to thread" );

   HeaderInfoList_obj hil(hilp);

   size_t count = hil->Count();

   if ( count <= 1 )
   {
      // nothing to be done: one message is never threaded
      return;
   }

   MFrame *frame = mf->GetInteractiveFrame();
   if ( frame )
   {
      wxLogStatus(frame, _("Threading %u messages..."), count);
   }

   // reset indentation first
   size_t i;
   for ( i = 0; i < count; i++ )
   {
      hil[i]->SetIndentation(0);
   }

   // We need a list of dependent messages for each entry.
   SizeTList *dependents = new SizeTList[count];

#if defined(EXPERIMENTAL_JWZ_THREADING)
   size_t *indices = new size_t[count];
   size_t *indents = new size_t[count];
   hilp->IncRef();
   ComputeThreadedIndicesWithJWZ(mf, hilp, indices, indents);

   hilp->AddTranslationTable(indices);

   for ( i = 0; i < hilp->Count(); i++ )
   {
      hil[i]->SetIndentation(indents[i]);
   }

#else
   for ( i = 0; i < count; i++ )
   {
      String id = hil[i]->GetId();

      if ( !id.empty() ) // no Id lines in Outbox!
      {
         for ( size_t j = 0; j < count; j++ )
         {
            String references = hil[j]->GetReferences();
            String inReplyTo = hil[j]->GetInReplyTo();
            if ( (references.Find(id) != wxNOT_FOUND) ||
                 (inReplyTo.Find(id) != wxNOT_FOUND))
            {
               dependents[i].push_back(new size_t(j));
               hil[j]->SetIndentation(MFCMN_INDENT2_MARKER);
            }
         }
      }
   }

   /* Now we have all the dependents set up properly and can re-build
      the list. We now build an array of indices for the new list.
   */

   size_t *indices = new size_t[count];
   size_t *indents = new size_t[count];
   memset(indents, 0, count*sizeof(size_t));

   size_t idx = 0; // where to store next entry
   for (i = 0; i < count; i++ )
   {
      // we mark used indices with a non-0 indentation, so only the top level
      // messages still have 0 indentation
      if ( hil[i]->GetIndentation() == 0 )
      {
         ASSERT(idx < (*hilp).Count());

         indices[idx++] = i;
         hil[i]->SetIndentation(MFCMN_INDENT1_MARKER);
         AddDependents(idx, 1, i, indices, indents, hilp, dependents);
      }
   }

   // we should have found all the messages
   ASSERT_MSG( idx == hilp->Count(), "logic error in threading code" );

   // we use AddTranslationTable() instead of SetTranslationTable() as this one
   // should be combined with the existing table from sorting
   hilp->AddTranslationTable(indices);

   for ( i = 0; i < hilp->Count(); i++ )
   {
      hil[i]->SetIndentation(indents[i]);
   }
#endif

   delete [] indices;
   delete [] indents;
   delete [] dependents;

   if ( frame )
   {
      wxLogStatus(frame, _("Threading %lu messages... done."), count);
   }
}

static void SortListing(MailFolder *mf,
                        HeaderInfoList *hil,
                        const SortParams& sortParams)
{
   CHECK_RET( hil, "no listing to sort" );

   // don't sort the listing if we don't have any sort criterium (so sorting
   // "by arrival order" will be much faster!)
   if ( sortParams.order != 0 )
   {
      size_t count = hil->Count();
      if ( count > 1 )
      {
         MFrame *frame = mf->GetInteractiveFrame();
         if ( frame )
         {
            wxLogStatus(frame, _("Sorting %u messages..."), count);
         }

         MLocker lock(gs_SortData.mutex);
         gs_SortData.sortParams = sortParams;
         gs_SortData.hil = hil;

         // start with unsorted listing
         size_t *transTable = new size_t[count];
         for ( size_t n = 0; n < count; n++ )
         {
            transTable[n] = n;
         }

         // now sort it
         qsort(transTable, count, sizeof(size_t), ComparisonFunction);

         // and tell the listing to use it (it will delete it)
         hil->SetTranslationTable(transTable);

         // just in case
         gs_SortData.hil = NULL;

         if ( frame )
         {
            wxLogStatus(frame, _("Sorting %u messages... done."), count);
         }
      }
      //else: avoid sorting empty listing or listing of 1 element
   }
   //else: configured not to do any sorting

   hil->DecRef();
}

void
MailFolderCmn::ProcessHeaderListing(HeaderInfoList *hilp)
{
   CHECK_RET( hilp, "no listing to process" );

   hilp->IncRef();

   SortParams sortParams;
   sortParams.order = m_Config.m_ListingSortOrder;
   sortParams.replaceFromWithTo = m_Config.m_replaceFromWithTo;
   if ( sortParams.replaceFromWithTo )
      sortParams.ownAddresses = &m_Config.m_ownAddresses;
   else
      sortParams.ownAddresses = NULL;

   SortListing(this, hilp, sortParams);

   if ( m_Config.m_UseThreading )
   {
      hilp->IncRef();
      ThreadMessages(this, hilp);
   }
   else // reset indentation as it may have been set by threading code before
   {
      size_t count = hilp->Count();
      for ( size_t i = 0; i < count; i++ )
      {
         (*hilp)[i]->SetIndentation(0);
      }
   }
}

#endif // BROKEN_BY_VZ

// ----------------------------------------------------------------------------
// MailFolderCmn new mail check
// ----------------------------------------------------------------------------

/*
  This function is called by GetHeaders() immediately after building a
  new folder listing. It checks for new mails and, if required, sends
  out new mail events.
*/
void
MailFolderCmn::CheckForNewMail(HeaderInfoList *hilp)
{
   CHECK_RET( hilp, "no listing in CheckForNewMail" );

   UIdType n = hilp->Count();
   if ( !n )
      return;

   UIdType *messageIDs = new UIdType[n];

   wxLogTrace(TRACE_NEWMAIL,
              "CheckForNewMail(): folder: %s highest seen uid: %lu.",
              GetName().c_str(), (unsigned long) m_LastNewMsgUId);

   // new messages are supposed to have UIDs greater than the last new message
   // seen, but not all messages with greater UID are new, so we have to first
   // find all messages with such UIDs and then check if they're really new

   // when we check for the new mail the first time, m_LastNewMsgUId is still
   // invalid and so we must reset it before comparing with it
   bool firstTime = m_LastNewMsgUId == UID_ILLEGAL;
   if ( firstTime )
      m_LastNewMsgUId = 0;

   // Find the new messages:
   UIdType nextIdx = 0;
   UIdType highestId = m_LastNewMsgUId;
   for ( UIdType i = 0; i < n; i++ )
   {
      HeaderInfo *hi = hilp->GetItemByIndex(i);
      UIdType uid = hi->GetUId();
      if ( uid > m_LastNewMsgUId )
      {
         if ( IsNewMessage(hi) )
         {
            messageIDs[nextIdx++] = uid;
         }

         if ( uid > highestId )
            highestId = uid;
      }
   }

   ASSERT_MSG( nextIdx <= n, "more new messages than total?" );

   if ( nextIdx != 0)
   {
      MEventManager::Send(new MEventNewMailData(this, nextIdx, messageIDs));
   }
   //else: no new messages found

   m_LastNewMsgUId = highestId;

   wxLogTrace(TRACE_NEWMAIL,
              "CheckForNewMail() after test: folder: %s highest seen uid: %lu.",
              GetName().c_str(), (unsigned long) highestId);


   delete [] messageIDs;
}

// ----------------------------------------------------------------------------
// MailFolderCmn options handling
// ----------------------------------------------------------------------------

void
MailFolderCmn::OnOptionsChange(MEventOptionsChangeData::ChangeKind kind)
{
   /*
      We want to avoid rebuilding the listing unnecessary (it is an expensive
      operation) so we only do it if something really changed for us.
   */

   MFCmnOptions config;
   ReadConfig(config);
   if ( config != m_Config )
   {
      m_Config = config;

      DoUpdate();

      // listing has been resorted/rethreaded
      RequestUpdate();
   }
}

void
MailFolderCmn::UpdateConfig(void)
{
   ReadConfig(m_Config);

   DoUpdate();
}

void
MailFolderCmn::DoUpdate()
{
   int interval = m_Config.m_UpdateInterval * 1000;
   if ( interval != m_Timer->GetInterval() )
   {
      m_Timer->Stop();

      if ( interval > 0 ) // interval of zero == disable ping timer
         m_Timer->Start(interval);
   }
}

void
MailFolderCmn::ReadConfig(MailFolderCmn::MFCmnOptions& config)
{
   Profile *profile = GetProfile();
   config.m_ListingSortOrder = READ_CONFIG(profile, MP_MSGS_SORTBY);
   config.m_ReSortOnChange = READ_CONFIG(profile, MP_MSGS_RESORT_ON_CHANGE) != 0;
   config.m_UpdateInterval = READ_CONFIG(profile, MP_UPDATEINTERVAL);
   config.m_UseThreading = READ_CONFIG(profile, MP_MSGS_USE_THREADING) != 0;
#if defined(EXPERIMENTAL_JWZ_THREADING)
   config.m_GatherSubjects = READ_CONFIG(profile, MP_MSGS_GATHER_SUBJECTS) != 0;
   config.m_BreakThread = READ_CONFIG(profile, MP_MSGS_BREAK_THREAD) != 0;
#if wxUSE_REGEX
   config.m_SimplifyingRegex = READ_CONFIG(profile, MP_MSGS_SIMPLIFYING_REGEX);
   config.m_ReplacementString = READ_CONFIG(profile, MP_MSGS_REPLACEMENT_STRING);
#else
   config.m_RemoveListPrefixGathering = READ_CONFIG(profile, MP_MSGS_REMOVE_LIST_PREFIX_GATHERING) != 0;
   config.m_RemoveListPrefixBreaking = READ_CONFIG(profile, MP_MSGS_REMOVE_LIST_PREFIX_BREAKING) != 0;
#endif
   config.m_IndentIfDummyNode = READ_CONFIG(profile, MP_MSGS_INDENT_IF_DUMMY) != 0;
#endif // EXPERIMENTAL_JWZ_THREADING
   config.m_replaceFromWithTo = READ_CONFIG(profile, MP_FVIEW_FROM_REPLACE) != 0;
   if ( config.m_replaceFromWithTo )
   {
      String returnAddrs = READ_CONFIG(profile, MP_FROM_REPLACE_ADDRESSES);
      if ( returnAddrs == MP_FROM_REPLACE_ADDRESSES_D )
      {
         // the default for this option is just the return address
         returnAddrs = READ_CONFIG(profile, MP_FROM_ADDRESS);
      }

      config.m_ownAddresses = strutil_restore_array(':', returnAddrs);
   }
}

// ----------------------------------------------------------------------------
// MailFolderCmn message deletion
// ----------------------------------------------------------------------------

bool
MailFolderCmn::UnDeleteMessages(const UIdArray *selections)
{
   int n = selections->Count();
   int i;
   bool rc = true;
   for(i = 0; i < n; i++)
      rc &= UnDeleteMessage((*selections)[i]);
   return rc;
}


/** Delete a message.
    @param uid mesage uid
    @return true if ok
    */
bool
MailFolderCmn::DeleteMessage(unsigned long uid)
{
   UIdArray a;
   a.Add(uid);

   // delete without expunging
   return DeleteMessages(&a);
}


bool
MailFolderCmn::DeleteOrTrashMessages(const UIdArray *selections)
{
   CHECK( CanDeleteMessagesInFolder(GetType()), FALSE,
          "can't delete messages in this folder" );

   // we can either "really delete" the messages (in fact, just mark them as
   // deleted) or move them to trash which implies deleting and expunging them
   bool reallyDelete = !READ_CONFIG(GetProfile(), MP_USE_TRASH_FOLDER);

   // If we are the trash folder, we *really* delete
   wxString trashFolderName = READ_CONFIG(GetProfile(), MP_TRASH_FOLDER);
   if ( !reallyDelete && GetName() == trashFolderName )
      reallyDelete = true;

   bool rc;
   if ( reallyDelete )
   {
      // delete without expunging
      rc = DeleteMessages(selections, FALSE);
   }
   else // move to trash
   {
      rc = SaveMessages(selections, trashFolderName);
      if ( rc )
      {
         // delete and expunge
         rc = DeleteMessages(selections, TRUE);
      }
   }

   return rc;
}

bool
MailFolderCmn::DeleteMessages(const UIdArray *selections, bool expunge)
{
   String seq = GetSequenceString(selections);
   if ( seq.empty() )
   {
      // nothing to do
      return true;
   }

   bool rc = SetSequenceFlag(seq, MailFolder::MSG_STAT_DELETED);
   if ( rc && expunge )
      ExpungeMessages();

   return rc;
}

// ----------------------------------------------------------------------------
// MailFolderCmn filtering
// ----------------------------------------------------------------------------

/// apply the filters to the selected messages
int
MailFolderCmn::ApplyFilterRules(UIdArray msgs)
{
   // Has the folder got any filter rules set? Concat all its filters together
   String filterString;

   MFolder_obj folder(GetName());
   wxArrayString filters = folder->GetFilters();
   size_t count = filters.GetCount();

   for ( size_t n = 0; n < count; n++ )
   {
      MFilter_obj filter(filters[n]);
      MFilterDesc fd = filter->GetDesc();
      filterString += fd.GetRule();
   }

   int rc = 0;
   if ( !filterString.empty() )
   {
      // Obtain pointer to the filtering module:
      MModule_Filters *filterModule = MModule_Filters::GetModule();

      if ( filterModule )
      {
         FilterRule *filterRule = filterModule->GetFilter(filterString);
         if ( filterRule )
         {
            wxBusyCursor busyCursor;

            // don't ignore deleted messages here (hence "false")
            rc = filterRule->Apply(this, msgs, false);

            filterRule->DecRef();
         }
         else
         {
            wxLogWarning(_("Error in filter code for folder '%s', "
                           "filters not applied"),
                         filterString.c_str());

            rc = FilterRule::Error;
         }

         filterModule->DecRef();
      }
      else // no filter module
      {
         wxLogWarning(_("Filter module couldn't be loaded, "
                        "filters not applied"));

         rc = FilterRule::Error;
      }
   }
   else // no filters to apply
   {
      wxLogVerbose(_("No filters configured for this folder."));
   }

   return rc;
}

DECLARE_AUTOPTR(MModule_Filters);
DECLARE_AUTOPTR(FilterRule);

// Checks for new mail and filters if necessary.
bool
MailFolderCmn::FilterNewMail()
{
   // Maybe we are lucky and have nothing to do?
   if ( CountRecentMessages() == 0 )
   {
      // true as no error
      return true;
   }

   // Obtain pointer to the filtering module:
   MModule_Filters_obj filterModule = MModule_Filters::GetModule();
   if ( !filterModule )
   {
      wxLogVerbose("Filter module couldn't be loaded.");

      return false;
   }

   // build a single program from all filter rules:
   String filterString;
   MFolder_obj folder(GetName());
   wxArrayString filters;
   if ( folder )
      filters = folder->GetFilters();
   size_t countFilters = filters.GetCount();
   for ( size_t nFilter = 0; nFilter < countFilters; nFilter++ )
   {
      MFilter_obj filter(filters[nFilter]);
      MFilterDesc fd = filter->GetDesc();
      filterString += fd.GetRule();
   }

   // don't do anything if no filters
   if ( filterString.empty() )
      return true;

   // or if there is a syntax error in them
   FilterRule_obj filterRule = filterModule->GetFilter(filterString);
   if ( !filterRule )
      return false;

   HeaderInfoList_obj hil = GetHeaders();
   CHECK( hil, false, "FilterNewMail: no headers" );

   // Build an array of NEW message UIds to apply the filters to:
   MsgnoArray *messages = SearchByFlag(MSG_STAT_NEW);
   if ( !messages )
   {
      // no new messages
      return true;
   }

   size_t count = messages->GetCount();
   if ( !count )
   {
      delete messages;

      return true;
   }

   // convert msgnos we got from SearchByFlag() into UIDs
   //
   // notice that it isn't really important that we force retrieving all these
   // headers now (instead of getting UIDs directly from server) as they will
   // be needed for Apply() below soon anyhow

   // it is very common to have a range of new messages, so cache all of them
   // at once - if it's worth it
   MsgnoType msgnoFirstNew = messages->Item(0),
             msgnoLastNew = messages->Item(count - 1);

   if ( msgnoFirstNew < msgnoLastNew )
   {
      hil->CacheMsgnos(msgnoFirstNew, msgnoLastNew);
   }

   for ( size_t n = 0; n < count; n++ )
   {
      HeaderInfo *hi = hil->GetItemByMsgno(messages->Item(n));
      if ( !hi )
      {
         FAIL_MSG( "failed to get header for a new message" );

         messages->Item(n) = UID_ILLEGAL;
      }
      else
      {
         messages->Item(n) = hi->GetUId();
      }
   }

   // do apply the filters finally
   int rc = filterRule->Apply(this, *messages, true /* ignore deleted */);

   // avoid doing anything harsh (like expunging the messages) if an
   // error occurs
   if ( !(rc & FilterRule::Error) )
   {
      // some of the messages might have been deleted by the filters
      // and, moreover, the filter code could have called
      // ExpungeMessages() explicitly, so we may have to expunge some
      // messages from the folder

      // calling ExpungeMessages() from filter code is unconditional
      // and should always be honoured, so check for it first
      bool expunge = (rc & FilterRule::Expunged) != 0;
      if ( !expunge )
      {
         if ( rc & FilterRule::Deleted )
         {
            // expunging here is dangerous because we can expunge the
            // messages which had been deleted by the user manually
            // before the filters were applied, so check a special
            // option which may be set to prevent us from doing this
            expunge = !READ_APPCONFIG(MP_SAFE_FILTERS);
         }
      }

      if ( expunge )
      {
         // so be it
         ExpungeMessages();
      }
   }

   delete messages;

   return true;
}

bool MailFolderCmn::CollectNewMail()
{
   if ( !(GetFlags() & MF_FLAGS_INCOMING) )
   {
      // we don't collect messages from this folder
      return true;
   }

   // where to we move the mails?
   String newMailFolder = READ_CONFIG(GetProfile(), MP_NEWMAIL_FOLDER);
   if ( newMailFolder == GetName() )
   {
      ERRORMESSAGE((_("Cannot collect mail from folder '%s' into itself, "
                      "please modify the properties for this folder.\n"
                      "\n"
                      "Disabling automatic mail collection for now."),
                   GetName().c_str()));

      // reset MF_FLAGS_INCOMING flag to avoid the error the next time
      MFolder_obj folder(GetProfile());
      if ( folder )
      {
         folder->ResetFlags(MF_FLAGS_INCOMING);
      }

      return false;
   }

   // do we have any messages at all?
   unsigned long count = GetMessageCount();
   if ( count > 0 )
   {
      HeaderInfoList_obj hil = GetHeaders();
      CHECK( hil, false, "CollectNewMail: no headers" );

      // get all undeleted messages
      UIdArray messages;
      for ( unsigned long idx = 0; idx < count; idx++ )
      {
         // check that the message isn't deleted - avoids getting 2 (or
         // more!) copies of the same "new" message (first normal,
         // subsequent deleted) if, for whatever reason, we failed to
         // expunge the messages the last time we collected mail from here
         HeaderInfo *hi = hil->GetItemByIndex(idx);
         if ( !(hi->GetStatus() & MSG_STAT_DELETED) )
         {
            messages.Add(hi->GetUId());
         }
      }

      // it is possible that all new messages had MSG_STAT_DELETED flag
      // set and so we have nothing to copy
      if ( !messages.IsEmpty() )
      {
         if ( SaveMessages(&messages, newMailFolder) )
         {
            // delete and expunge
            DeleteMessages(&messages, true);
         }
         else // don't delete them if we failed to move
         {
            ERRORMESSAGE((_("Cannot move newly arrived messages.")));

            return false;
         }
      }
   }

   return true;
}

// ----------------------------------------------------------------------------
// eliminating duplicate messages code
// ----------------------------------------------------------------------------

// VZ: disabling this stuff - it is a workaround for the bug which doesn't exist
//     any more
#if 0
struct Dup_MsgInfo
{
   Dup_MsgInfo(const String &s, UIdType id, unsigned long size)
      { m_Id = s; m_UId = id; m_Size = size; }
   String m_Id;
   UIdType m_UId;
   unsigned long m_Size;
};

KBLIST_DEFINE(Dup_MsgInfoList, Dup_MsgInfo);

UIdType
MailFolderCmn::DeleteDuplicates()
{
   HeaderInfoList *hil = GetHeaders();
   if(! hil)
      return 0;

   Dup_MsgInfoList mlist;

   UIdArray toDelete;

   for(size_t idx = 0; idx < hil->Count(); idx++)
   {
      String   id = (*hil)[idx]->GetId();
      UIdType uid = (*hil)[idx]->GetUId();
      size_t size = (*hil)[idx]->GetSize();
      bool found = FALSE;
      for(Dup_MsgInfoList::iterator i = mlist.begin();
          i != mlist.end();
          i++)
         if( (**i).m_Id == id )
         {
            /// if new message is larger, keep it instead
            if( (**i).m_Size < size )
            {
               toDelete.Add((**i).m_UId);
               (**i).m_UId = uid;
               (**i).m_Size = size;
               found = FALSE;
            }
            else
               found = TRUE;
            break;
         }
      if(found)
         toDelete.Add(uid);
      else
      {
         Dup_MsgInfo *mi = new Dup_MsgInfo(id, uid, size);
         mlist.push_back(mi);
      }
   }
   hil->DecRef();

   if(toDelete.Count() == 0)
      return 0; // nothing to do

   if(DeleteMessages(&toDelete,FALSE))
      return toDelete.Count();
   // else - uncommented or compiler thinks there's return without value
   return UID_ILLEGAL; // an error happened
}
#endif // 0

// ----------------------------------------------------------------------------
// MailFolderCmn message counting
// ----------------------------------------------------------------------------

bool MailFolderCmn::CountAllMessages(MailFolderStatus *status) const
{
   CHECK( status, false, "CountAllMessages: NULL pointer" );

   String name = GetName();
   MfStatusCache *mfStatusCache = MfStatusCache::Get();
   if ( !mfStatusCache->GetStatus(name, status) )
   {
      if ( !DoCountMessages(status) )
      {
         // failed to get the message count, don't update status cache with
         // bogus values
         return false;
      }

      mfStatusCache->UpdateStatus(name, *status);
   }

   return status->HasSomething();
}

// ----------------------------------------------------------------------------
// MailFolderCmn interactive frame functions
// ----------------------------------------------------------------------------

MFrame *MailFolderCmn::SetInteractiveFrame(MFrame *frame)
{
   MFrame *frameOld = m_frame;
   m_frame = frame;
   return frameOld;
}

MFrame *MailFolderCmn::GetInteractiveFrame() const
{
   if ( !mApplication->IsInAwayMode() )
   {
      if ( m_frame )
         return m_frame;

      if ( GetName() == ms_interactiveFolder )
         return ms_interactiveFrame;
   }
   //else: no interactivity in away mode at all

   return NULL;
}

// ----------------------------------------------------------------------------
// MfCmnEventReceiver
// ----------------------------------------------------------------------------

MfCmnEventReceiver::MfCmnEventReceiver(MailFolderCmn *mf)
{
   m_Mf = mf;

   m_MEventCookie = MEventManager::Register(*this, MEventId_OptionsChange);
   ASSERT_MSG( m_MEventCookie, "can't reg folder with event manager");

   m_MEventPingCookie = MEventManager::Register(*this, MEventId_Ping);
   ASSERT_MSG( m_MEventPingCookie, "can't reg folder with event manager");
}

MfCmnEventReceiver::~MfCmnEventReceiver()
{
   MEventManager::Deregister(m_MEventCookie);
   MEventManager::Deregister(m_MEventPingCookie);
}

bool MfCmnEventReceiver::OnMEvent(MEventData& event)
{
   if ( event.GetId() == MEventId_Ping )
   {
      m_Mf->Ping();
   }
   else // options change
   {
      // do update - but only if this event is for us
      MEventOptionsChangeData& data = (MEventOptionsChangeData&)event;
      if ( data.GetProfile()->IsAncestor(m_Mf->GetProfile()) )
         m_Mf->OnOptionsChange(data.GetChangeKind());

      // also update close timer in case timeout changed
      if ( gs_MailFolderCloser )
      {
         gs_MailFolderCloser->RestartTimer();
      }
   }

   return true;
}

// ----------------------------------------------------------------------------
// global functions
// ----------------------------------------------------------------------------

extern bool MailFolderCmnInit()
{
   if ( !gs_MailFolderCloser )
   {
      gs_MailFolderCloser = new MfCloser;
   }

   return true;
}

extern void MailFolderCmnCleanup()
{
   if ( gs_MailFolderCloser )
   {
      // any MailFolderCmn::DecRef() shouldn't add folders to
      // gs_MailFolderCloser from now on, so NULL it immediately
      MfCloser *mfCloser = gs_MailFolderCloser;
      gs_MailFolderCloser = NULL;

      delete mfCloser;
   }
}

