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
#endif // USE_PCH

#include "Mdefaults.h"

#include "MFolder.h"
#include "MFilter.h"
#include "modules/Filters.h"

#include "MDialogs.h"         // for MProgressDialog

#include "HeaderInfo.h"
#include "Address.h"

#include "MThread.h"

#include "MFCache.h"

#include <wx/timer.h>
#include <wx/datetime.h>
#include <wx/file.h>

#include "wx/persctrl.h"      // for wxPFileSelector

#include "MailFolderCmn.h"

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_FOLDERPROGRESS_THRESHOLD;
extern const MOption MP_FOLDER_CLOSE_DELAY;
extern const MOption MP_MSGS_RESORT_ON_CHANGE;
extern const MOption MP_NEWMAIL_FOLDER;
extern const MOption MP_SAFE_FILTERS;
extern const MOption MP_TRASH_FOLDER;
extern const MOption MP_UPDATEINTERVAL;
extern const MOption MP_USE_TRASH_FOLDER;

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

      m_headers->SetSortOrder(m_Config.m_SortParams);
      m_headers->SetThreadParameters(m_Config.m_ThrParams);
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

      if ( fileName.empty() )
      {
         // cancelled by user: don't return false from here as the caller would
         // think there was an error otherwise and, worse, as this function is
         // called indirectly (by ASMailFolder::SaveMessagesToFile()), the
         // caller can't even check mApplication->GetLastError() to check if it
         // was set to M_ERROR_CANCEL
         return true;
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
   int threshold = GetProfile()
      ? READ_CONFIG(GetProfile(), MP_FOLDERPROGRESS_THRESHOLD)
      : GetNumericDefault(MP_FOLDERPROGRESS_THRESHOLD);
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
   int threshold = mf->GetProfile()
      ? READ_CONFIG(mf->GetProfile(), MP_FOLDERPROGRESS_THRESHOLD)
      : GetNumericDefault(MP_FOLDERPROGRESS_THRESHOLD);

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
   static int SortComparisonFunction(const void *p1, const void *p2)
   {
      // check that the caller didn't forget to acquire the global data lock
      ASSERT_MSG( gs_SortData.mutex.IsLocked(), "using unlocked gs_SortData" );

      // convert msgnos to indices
      MsgnoType n1 = *(MsgnoType *)p1 - 1,
                n2 = *(MsgnoType *)p2 - 1;

      HeaderInfo *hi1 = gs_SortData.hil->GetItemByIndex(n1),
                 *hi2 = gs_SortData.hil->GetItemByIndex(n2);

      int result = 0;

      // if one header is invalid (presumable because it wasn't retrieved from
      // server at all because the user aborted it), assume it is always less
      // than the other
      if ( !hi1->IsValid() )
      {
         // even if the second one is invalid, we may still return 1, it's not
         // like it changes anything
         result = -1;
      }
      else if ( !hi2->IsValid() )
      {
         // as hi1 is valid, it is greater
         result = 1;
      }

      // copy it as we're going to modify it while processing
      long sortOrder = gs_SortData.sortParams.sortOrder;
      while ( !result && sortOrder != 0 )
      {
         long criterium = GetSortCrit(sortOrder);
         sortOrder = GetSortNextCriterium(sortOrder);

         // we rely on MessageSortOrder values being what they are: as _REV
         // version immediately follows the normal order constant, we should
         // reverse the comparison result for odd values of MSO_XXX
         int reverse = criterium % 2;
         switch ( criterium - reverse )
         {
            case MSO_NONE:
               break;

            case MSO_DATE:
               result = CmpNumeric(hi1->GetDate(), hi2->GetDate());
               break;

            case MSO_SUBJECT:
               {
                  String
                     subj1 = Address::NormalizeSubject(hi1->GetSubject()),
                     subj2 = Address::NormalizeSubject(hi2->GetSubject());

                  result = Stricmp(subj1, subj2);
               }
               break;

            case MSO_SENDER:
               {
                  // use "To" if needed
                  String value1, value2;
                  (void)HeaderInfo::GetFromOrTo
                                    (
                                       hi1,
                                       gs_SortData.sortParams.detectOwnAddresses,
                                       gs_SortData.sortParams.ownAddresses,
                                       &value1
                                    );

                  (void)HeaderInfo::GetFromOrTo
                                    (
                                       hi2,
                                       gs_SortData.sortParams.detectOwnAddresses,
                                       gs_SortData.sortParams.ownAddresses,
                                       &value2
                                    );

                  result = Stricmp(value1, value2);
               }
               break;

            case MSO_STATUS:
               result = CompareStatus(hi1->GetStatus(), hi2->GetStatus());
               break;

            case MSO_SIZE:
               result = CmpNumeric(hi1->GetSize(), hi2->GetSize());
               break;

            case MSO_SCORE:
               // we don't store score any more in HeaderInfo
#ifdef USE_HEADER_SCORE
               result = CmpNumeric(hi1->GetScore(), hi2->GetScore());
#else
               FAIL_MSG("unimplemented");
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

bool
MailFolderCmn::SortMessages(MsgnoType *msgnos, const SortParams& sortParams)
{
   HeaderInfoList_obj hil = GetHeaders();

   CHECK( hil, false, "no listing to sort" );

   MsgnoType count = hil->Count();

   // HeaderInfoList::Sort() is supposed to check for these cases
   ASSERT_MSG( GetSortCritDirect(sortParams.sortOrder) != MSO_NONE,
               "nothing to do in Sort() - shouldn't be called" );

   ASSERT_MSG( count > 1,
               "nothing to sort in Sort() - shouldn't be called" );

   MFrame *frame = GetInteractiveFrame();
   if ( frame )
   {
      wxLogStatus(frame, _("Sorting %u messages..."), count);
   }

   // we need all headers, prefetch them
   hil->CacheMsgnos(1, count);

   MLocker lock(gs_SortData.mutex);
   gs_SortData.sortParams = sortParams;
   gs_SortData.hil = hil.operator->();

   // start with unsorted listing
   for ( MsgnoType n = 0; n < count; n++ )
   {
      // +1 because we want the msgnos, not indices
      msgnos[n] = n + 1;
   }

   // now sort it
   qsort(msgnos, count, sizeof(MsgnoType), SortComparisonFunction);

   // don't leave dangling pointers around
   gs_SortData.hil = NULL;

   if ( frame )
   {
      wxLogStatus(frame, _("Sorting %u messages... done."), count);
   }

   return true;
}

// ----------------------------------------------------------------------------
// MailFolderCmn threading
// ----------------------------------------------------------------------------

bool MailFolderCmn::ThreadMessages(MsgnoType *msgnos,
                                   size_t *indents,
                                   const ThreadParams& thrParams)
{
   HeaderInfoList_obj hil = GetHeaders();

   CHECK( hil, false, "no listing to sort" );

   MsgnoType count = hil->Count();

   // HeaderInfoList::Thread() is supposed to check for these cases
   ASSERT_MSG( thrParams.useThreading,
               "nothing to do in ThreadMessages() - shouldn't be called" );

   ASSERT_MSG( count > 1,
               "nothing to sort in ThreadMessages() - shouldn't be called" );

   MFrame *frame = GetInteractiveFrame();
   if ( frame )
   {
      wxLogStatus(frame, _("Threading %u messages..."), count);
   }

   // we need all headers, prefetch them
   hil->CacheMsgnos(1, count);

   // reset indentation first
   memset(indents, 0, count*sizeof(size_t));

   // do thread!
   JWZThreadMessages(thrParams, hil.operator->(), msgnos, indents);

   // finishing touches...
   for ( size_t i = 0; i < count; i++ )
   {
      // convert to msgnos from indices
      //
      // TODO: compute directly msgnos, not indices in JWZThreadMessages
      msgnos[i]++;
   }

   if ( frame )
   {
      wxLogStatus(frame, _("Threading %u messages... done."), count);
   }

   return true;
}

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
// MFCmnOptions
// ----------------------------------------------------------------------------

MailFolderCmn::MFCmnOptions::MFCmnOptions()
{
   m_ReSortOnChange = false;

   m_UpdateInterval = 0;
}

bool MailFolderCmn::MFCmnOptions::operator!=(const MFCmnOptions& other) const
{
   return m_SortParams != other.m_SortParams ||
          m_ThrParams != other.m_ThrParams ||
          m_ReSortOnChange != other.m_ReSortOnChange ||
          m_UpdateInterval != other.m_UpdateInterval;
}

// ----------------------------------------------------------------------------
// MailFolderCmn options handling
// ----------------------------------------------------------------------------

void
MailFolderCmn::OnOptionsChange(MEventOptionsChangeData::ChangeKind kind)
{
   MFCmnOptions config;
   ReadConfig(config);
   if ( config != m_Config )
   {
      bool listingChanged = false;

      // we want to avoid rebuilding the listing unnecessary (it is an
      // expensive operation) so we only do it if a setting really affecting
      // it changed
      if ( m_headers )
      {
         // do we need to resort messages?
         if ( m_headers->SetSortOrder(config.m_SortParams) )
         {
            listingChanged = true;
         }

         // rethread?
         if ( m_headers->SetThreadParameters(config.m_ThrParams) )
         {
            listingChanged = true;
         }
      }

      m_Config = config;

      DoUpdate();

      if ( listingChanged )
      {
         // listing has been resorted/rethreaded
         RequestUpdate();
      }
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

   config.m_SortParams.Read(profile);
   config.m_ThrParams.Read(profile);
   config.m_ReSortOnChange = READ_CONFIG_BOOL(profile, MP_MSGS_RESORT_ON_CHANGE);
   config.m_UpdateInterval = READ_CONFIG(profile, MP_UPDATEINTERVAL);
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

