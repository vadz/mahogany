//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   mail/MailFolderCmn.cpp: generic MailFolder methods which don't
//              use cclient (i.e. don't really work with mail folders)
// Purpose:     functions common to all MailFolder implementations, in
//              particular handling of folder closing (including keep alive
//              logic) and the new mail processing (filtering, collecting, ...)
// Author:      Karsten Ballüder
// Modified by:
// Created:     02.04.01 (extracted from mail/MailFolder.cpp)
// CVS-ID:      $Id$
// Copyright:   (C) 1997-2002 Mahogany Team
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

#ifdef USE_PYTHON
#    include  "MPython.h"        // Python fix for Pyobject / presult
#    include  "PythonHelp.h"     // Python fix for PythonCallback
#    include  "Mcallbacks.h"     // Python fix for MCB_* declares
#endif //USE_PYTHON

#ifndef USE_PCH
#  include "Mcommon.h"
#  include "guidef.h"
#  include "strutil.h"
#  include "MApplication.h"
#  include "Mdefaults.h"

#  include <wx/filedlg.h>
#  include <wx/timer.h>                   // for wxTimer
#endif // USE_PCH

#include <wx/mimetype.h>

#include "Sequence.h"
#include "UIdArray.h"

#include "MSearch.h"
#include "Message.h"

#include "MFilter.h"
#include "modules/Filters.h"

#include "HeaderInfo.h"

#include "MThread.h"
#include "MFCache.h"
#include "MFStatus.h"
#include "MFolder.h"
#include "Address.h"

#include "Composer.h"

#include "MailFolderCmn.h"
#include "MFPrivate.h"
#include "mail/FolderPool.h"
#include "gui/wxMDialogs.h"
#include "wx/persctrl.h"

#include <wx/datetime.h>
#include <wx/file.h>

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_FOLDERPROGRESS_THRESHOLD;
extern const MOption MP_FOLDER_CLOSE_DELAY;
extern const MOption MP_MOVE_NEWMAIL;
extern const MOption MP_MSGS_RESORT_ON_CHANGE;
extern const MOption MP_NEWMAILCOMMAND;
extern const MOption MP_NEWMAIL_FOLDER;
extern const MOption MP_NEWMAIL_PLAY_SOUND;
extern const MOption MP_NEWMAIL_SOUND_FILE;
#if defined(OS_UNIX) || defined(__CYGWIN__)
extern const MOption MP_NEWMAIL_SOUND_PROGRAM;
#endif // OS_UNIX
extern const MOption MP_SAFE_FILTERS;
extern const MOption MP_SHOW_NEWMAILINFO;
extern const MOption MP_SHOW_NEWMAILMSG;
extern const MOption MP_TRASH_FOLDER;
extern const MOption MP_UPDATEINTERVAL;
extern const MOption MP_USE_NEWMAILCOMMAND;
extern const MOption MP_USE_TRASH_FOLDER;

// ----------------------------------------------------------------------------
// trace masks
// ----------------------------------------------------------------------------

// trace mail folder ref counting
#define TRACE_MF_REF _T("mfref")

// trace mask for mail folder closing
#define TRACE_MF_CLOSE _T("mfclose")

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

static long GetProgressThreshold(Profile *profile)
{
   if ( !profile )
      return GetNumericDefault(MP_FOLDERPROGRESS_THRESHOLD);

   return READ_CONFIG(profile, MP_FOLDERPROGRESS_THRESHOLD);
}

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
#if 0
      wxLogTrace(TRACE_MF_CLOSE,
                 _T("Checking if '%s' expired: exp time: %s, now: %s"),
                 m_mf->GetName().c_str(),
                 m_dt.FormatTime().c_str(),
                 wxDateTime::Now().FormatTime().c_str());
#endif // 0

      return m_expires && (m_dt <= wxDateTime::Now());
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
M_LIST_OWN(MfList, MfCloseEntry);

// the timer which periodically really closes the previously "closed" folders
class MfCloser;
class MfCloseTimer : public wxTimer
{
public:
   MfCloseTimer(MfCloser *mfCloser) { m_mfCloser = mfCloser; }

   virtual void Notify(void);

private:
   MfCloser *m_mfCloser;

   DECLARE_NO_COPY_CLASS(MfCloseTimer)
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

   // is this folder in this list?
   bool HasFolder(MailFolderCmn *mf) const { return GetCloseEntry(mf) != NULL; }

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

   DECLARE_NO_COPY_CLASS(MfCloser)
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

// ----------------------------------------------------------------------------
// MailFolderTimer: a timer class to regularly ping the mailfolder
// ----------------------------------------------------------------------------

class MailFolderTimer : public wxTimer
{
public:
   /** constructor
       @param mf the mailfolder to query on timeout
   */
   MailFolderTimer(MailFolderCmn *mf)
   {
      m_mf = mf;
   }

   /// get called on timeout and pings the mailfolder
   void Notify(void);

protected:
   /// the mailfolder to update
   MailFolderCmn *m_mf;

   DECLARE_NO_COPY_CLASS(MailFolderTimer)
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
// MailFolderTimer
// ----------------------------------------------------------------------------

void MailFolderTimer::Notify(void)
{
   if ( mApplication->AllowBgProcessing() && !m_mf->IsLocked() )
   {
      // don't ping the mailbox which we maintain artificially alive, otherwise
      // it can never close at all as pinging it may remove and add it back to
      // MfCloser thus resetting its expiration timeout
      //
      // but do ping it if it shouldn't close - as otherwise it indeed will!
      if ( (m_mf->GetFlags() & MF_FLAGS_KEEPOPEN) ||
               !gs_MailFolderCloser || !gs_MailFolderCloser->HasFolder(m_mf) )
      {
         // don't show any dialogs when doing background checks
         NonInteractiveLock noInter(m_mf, false /* !interactive */);

         m_mf->Ping();

         // restart the timer after Ping() has completed: this ensures that
         // we're not going to ping the folder too often if accessing it took a
         // lot of time (i.e. if Ping() takes longer than our timer interval,
         // we'd keep pinging all the time and never do anything else)
         Start();
      }
   }
}

// ----------------------------------------------------------------------------
// MfCloseEntry
// ----------------------------------------------------------------------------

MfCloseEntry::MfCloseEntry(MailFolderCmn *mf, int secs)
{
   wxLogTrace(TRACE_MF_CLOSE,
              _T("Delaying closing of '%s' (%lu refs) for %d seconds."),
              mf->GetName().c_str(),
              (unsigned long)mf->GetNRef(),
              secs == NEVER_EXPIRES ? -1 : secs);

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
   wxLogTrace(TRACE_MF_CLOSE, _T("Destroying MfCloseEntry(%s) (%lu refs left)"),
              m_mf->GetName().c_str(), (unsigned long)m_mf->GetNRef());

   m_mf->RealDecRef();
}

// ----------------------------------------------------------------------------
// MfCloser
// ----------------------------------------------------------------------------

MfCloser::MfCloser()
        : m_timer(this)
{
   // default timer interval, will be updated in Add() if needed
   m_interval = READ_APPCONFIG(MP_FOLDER_CLOSE_DELAY);

   // don't start the timer for now, we don't have any folders to close yet
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
   ASSERT_MSG( !HasFolder(mf), _T("adding a folder to MfCloser twice??") );
#endif // DEBUG_FOLDER_CLOSE

   CHECK_RET( mf, _T("NULL MailFolder in MfCloser::Add()"));

   CHECK_RET( delay > 0, _T("folder close timeout must be positive") );

   wxLogTrace(TRACE_MF_REF, _T("Adding '%s' to gs_MailFolderCloser"),
              mf->GetName().c_str());

   m_MfList.push_back(new MfCloseEntry(mf, delay));

   if ( delay < m_interval )
   {
      // restart the timer using smaller interval - of course, normally we
      // should compute the gcd of all intervals but let's keep it simple
      m_interval = delay;

      RestartTimer();
   }
}

void MfCloser::OnTimer(void)
{
   for ( MfList::iterator i = m_MfList.begin(); i != m_MfList.end();)
   {
      if ( i->HasExpired() )
      {
#ifdef DEBUG
         wxLogTrace(TRACE_MF_CLOSE, _T("Going to remove '%s' from m_MfList"),
                    i->GetName().c_str());
#endif // DEBUG

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
   m_MfList.clear();
}

void MfCloser::Remove(MfCloseEntry *entry)
{
   CHECK_RET( entry, _T("NULL entry in MfCloser::Remove") );

#ifdef DEBUG
   wxLogTrace(TRACE_MF_REF, _T("Removing '%s' from gs_MailFolderCloser"),
              entry->GetName().c_str());
#endif // DEBUG

   for ( MfList::iterator i = m_MfList.begin(); i != m_MfList.end(); i++ )
   {
      if ( i.operator->() == entry )
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
      if ( i->Matches(mf) )
      {
         return *i;
      }
   }

   return NULL;
}

void MfCloser::RestartTimer()
{
   if ( m_timer.IsRunning() )
      m_timer.Stop();

   if ( m_interval )
   {
      // delay is in seconds, we need ms here
      m_timer.Start(m_interval * 1000);
   }
}

void MfCloseTimer::Notify(void)
{
   if ( mApplication->AllowBgProcessing() )
      m_mfCloser->OnTimer();
}

// ----------------------------------------------------------------------------
// MailFolderCmn folder closing
// ----------------------------------------------------------------------------

void MailFolderCmn::Close(bool /* mayLinger */)
{
   // this folder shouldn't be reused by MailFolder::OpenFolder() any more as
   // it is being closed anyhow, so prevent this from happening by removing it
   // from the pool of available folders
   MFPool::Remove(this);

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
   // anyhow -- or if we didn't start up fully yet and gs_MailFolderCloser
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

         // we shouldn't keep connection to the POP folders whatever the
         // options for it are (the new versions of Mahogany don't allow the
         // user to set them in this way anyhow but the settings could come
         // from an older version) as POP3 protocol doesn't really support this
         if ( GetType() != MF_POP )
         {
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
               // don't do it any more as IMAP CHECK command is *slow* and we
               // are calling it constantly when the folder is removed
               // from/added back to gs_MailFolderCloser
               //
               // Checkpoint(); // flush data immediately

               // this calls IncRef() on us so we won't be deleted right now
               gs_MailFolderCloser->Add(this, delay);
            }
         }
      }
   }

   return RealDecRef();
}

void
MailFolderCmn::IncRef()
{
   wxLogTrace(TRACE_MF_REF, _T("MF(%s)::IncRef(): %lu -> %lu"),
              GetName().c_str(),
              (unsigned long)GetNRef(),
              (unsigned long)GetNRef() + 1);

   MObjectRC::IncRef();
}

bool
MailFolderCmn::RealDecRef()
{
   wxLogTrace(TRACE_MF_REF, _T("MF(%s)::DecRef(): %lu -> %lu"),
              GetName().c_str(),
              (unsigned long)GetNRef(),
              (unsigned long)GetNRef() - 1);

#ifdef DEBUG_FOLDER_CLOSE
   // check that the folder is not in the mail folder closer list any more if
   // we're going to delete it
   if ( GetNRef() == 1 )
   {
      if ( gs_MailFolderCloser && gs_MailFolderCloser->HasFolder(this) )
      {
         // we will crash later when removing it from the list!
         FAIL_MSG(_T("deleting folder still in MfCloser list!"));
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

   m_suspendUpdates = 0;

   m_headers = NULL;

   m_frame = NULL;

   m_statusChangeData = NULL;
   m_expungeData = NULL;

   m_msgnoLastNotified = MSGNO_ILLEGAL;

   m_MEventReceiver = new MfCmnEventReceiver(this);
}

MailFolderCmn::~MailFolderCmn()
{
   ASSERT_MSG( !m_suspendUpdates,
               _T("mismatch between Suspend/ResumeUpdates()") );

   ASSERT_MSG( !MFPool::Remove(this), _T("folder shouldn't be left in the pool!") );

   // this must have been cleared by SendMsgStatusChangeEvent() call earlier
   if ( m_statusChangeData )
   {
      FAIL_MSG( _T("m_statusChangeData unexpectedly != NULL") );

      delete m_statusChangeData;
   }

   // normally this one should be deleted as well
   if ( m_expungeData )
   {
      FAIL_MSG( _T("m_expungeData unexpectedly != NULL") );

      delete m_expungeData;
   }


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

void MailFolderCmn::CacheLastMessages(MsgnoType count)
{
   if ( count > 1 )
   {
      HeaderInfoList_obj hil(GetHeaders());

      CHECK_RET( hil, _T("Failed to cache messages because there are no headers") );

      MsgnoType total = GetMessageCount();

      hil->CacheMsgnos(total - count + 1, total);
   }
}

// ----------------------------------------------------------------------------
// MailFolderCmn message saving
// ----------------------------------------------------------------------------

// TODO: the functions below should share at least some common code instead of
//       duplicating it! (VZ)

bool
MailFolderCmn::SaveMessagesToFile(const UIdArray *selections,
                                  const String& fileName0,
                                  wxWindow *parent)
{
   String fileName = fileName0;

   if ( fileName.empty() )
   {
      // ask the user
      fileName = wxPFileSelector
                 (
                  _T("MsgSave"),
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

   scoped_ptr<MProgressDialog> pd;
   long threshold = GetProgressThreshold(GetProfile());

   if ( threshold > 0 && n > threshold )
   {
      wxString msg;
      msg.Printf(_("Saving %d messages to the file '%s'..."),
                 n, fileName0.empty() ? fileName.c_str() : fileName0.c_str());

      pd.reset(new MProgressDialog(GetName(), msg, 2*n));
   }

   bool rc = true;
   String tmpstr;
   for ( int i = 0; i < n; i++ )
   {
      Message_obj msg(GetMessage((*selections)[i]));

      if ( msg )
      {
         if ( pd && !pd->Update( 2*i + 1 ) )
            break;

         if ( !msg->WriteToString(tmpstr) )
         {
            wxLogError(_("Failed to get the text of the message to save."));
            rc = false;
         }
         else
         {
            rc &= file.Write(tmpstr);
         }

         if ( pd && !pd->Update( 2*i + 2) )
            break;
      }
   }

   return rc;
}

bool
MailFolderCmn::SaveMessages(const UIdArray *selections,
                            MFolder *folder)
{
   CHECK( folder, false, _T("SaveMessages() needs a valid folder pointer") );

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
   CHECK( n, true, _T("SaveMessages(): nothing to save") );

   MailFolder_obj mf(MailFolder::OpenFolder(folder, Normal, m_frame));
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
      FAIL_MSG( _T("Can't SaveMessages() to locked folder") );
      return false;
   }

   scoped_ptr<MProgressDialog> pd;
   long threshold = GetProgressThreshold(mf->GetProfile());

   if ( threshold > 0 && n > threshold )
   {
      // open a progress window:
      wxString msg;
      msg.Printf(_("Saving %d messages to the folder '%s'..."),
                 n, folder->GetName().c_str());

      pd.reset(new MProgressDialog
                   (
                     mf->GetName(),   // title
                     msg,             // label message
                     2*n              // range
                   ));
   }

   // minimize the number of updates by only doing it once
   SuspendFolderUpdates suspend(mf);

   bool rc = true;
   for ( int i = 0; i < n; i++ )
   {
      if ( pd && !pd->Update(2*i + 1) )
      {
         // cancelled
         return false;
      }

      Message *msg = GetMessage((*selections)[i]);
      if ( msg )
      {
         rc &= mf->AppendMessage(*msg);
         msg->DecRef();

         if ( pd && !pd->Update(2*i + 2) )
         {
            // cancelled
            return false;
         }
      }
      else
      {
         FAIL_MSG( _T("copying inexistent message?") );
      }
   }

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
                             wxWindow *parent)
{
   Composer *composer = NULL;

   int n = selections->Count();
   for( int i = 0; i < n; i++ )
   {
      Message *msg = GetMessage((*selections)[i]);
      if ( msg )
      {
         composer = ReplyMessage(msg, params, GetProfile(), parent, composer);
         msg->DecRef();
      }
   }

   if ( composer )
      composer->Launch();
}


void
MailFolderCmn::ForwardMessages(const UIdArray *selections,
                               const MailFolder::Params& params,
                               wxWindow *parent)
{
   Composer *composer = NULL;

   int n = selections->Count();
   for ( int i = 0; i < n; i++ )
   {
      Message *msg = GetMessage((*selections)[i]);
      if ( msg )
      {
         composer = ForwardMessage(msg, params, GetProfile(), parent, composer);
         msg->DecRef();
      }
   }

   if ( composer )
      composer->Launch();
}

// ----------------------------------------------------------------------------
// MailFolderCmn searching
// ----------------------------------------------------------------------------

UIdArray *MailFolderCmn::SearchMessages(const SearchCriterium *crit, int flags)
{
   HeaderInfoList_obj hil(GetHeaders());
   CHECK( hil, NULL, _T("no listing in SearchMessages") );

   // the search results
   UIdArray *results = new UIdArray;

   // how many did we find?
   unsigned long countFound = 0;

   MProgressDialog *progDlg = NULL;

   MsgnoType nMessages = GetMessageCount();

   // show the progress dialog if the search is going to take a long time
   if ( nMessages > (unsigned long)READ_CONFIG(GetProfile(),
                                               MP_FOLDERPROGRESS_THRESHOLD) )
   {
      String msg;
      msg.Printf(_("Searching in %lu messages..."), nMessages);

      progDlg = new MProgressDialog(GetName(), msg, nMessages);
   }

   // check all messages
   bool cont = true;
   String what;
   for ( size_t idx = 0; idx < nMessages && cont; idx++ )
   {
      HeaderInfo *hi = hil->GetItemByIndex(idx);

      if ( !hi )
      {
         FAIL_MSG( _T("SearchMessages: can't get header info") );

         continue;
      }

      if ( crit->m_What == SearchCriterium::SC_SUBJECT )
      {
         what = hi->GetSubject();
      }
      else if ( crit->m_What == SearchCriterium::SC_FROM )
      {
         what = hi->GetFrom();
      }
      else if ( crit->m_What == SearchCriterium::SC_TO )
      {
         what = hi->GetTo();
      }
      else
      {
         Message_obj msg(GetMessage(hi->GetUId()));
         if ( !msg )
         {
            FAIL_MSG( _T("SearchMessages: can't get message") );

            continue;
         }

         switch ( crit->m_What )
         {
            case SearchCriterium::SC_FULL:
            case SearchCriterium::SC_BODY:
               // FIXME: wrong for body as it checks the whole message
               //        including header
               what = msg->FetchText();
               break;

            case SearchCriterium::SC_HEADER:
               what = msg->GetHeader();
               break;

            case SearchCriterium::SC_CC:
               msg->GetHeaderLine(_T("CC"), what);
               break;

            default:
               FAIL_MSG(_T("Unknown search criterium!"));
         }
      }

      bool found = wxStrstr(what, crit->m_Key) != NULL;
      if ( found != crit->m_Invert )
      {
         // really found, remember its UID or msgno depending on the flags
         results->Add(flags & SEARCH_UID ? hi->GetUId() : idx + 1);
      }

      // update the progress dialog and check for abort
      if ( progDlg )
      {
         String msg;
         msg.Printf(_("Searching in %lu messages..."), nMessages);

         unsigned long cnt = results->Count();
         if ( cnt != countFound )
         {
            String msg2;
            msg2.Printf(_(" - %lu matches found."), cnt);

            cont = progDlg->Update(idx, msg + msg2);

            countFound = cnt;
         }
         else
         {
            cont = progDlg->Update(idx);
         }
      }
   }

   delete progDlg;

   return results;
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

static int ComputeStatusScore(int status)
{
   /*
      The idea is to make the messages appear in the order of new, important,
      recent or unread, other, answered so the scores are assigned in a way to
      make new appear in front of the important, but the important before the
      ones which are just recent or unread (remember that new == recent &&
      unread)
   */
   enum
   {
      SCORE_RECENT = 2,
      SCORE_UNREAD = 3,
      SCORE_IMPORTANT = 4,
      SCORE_ANSWERED = -1
   };

   int score = 0;

   if ( status & MailFolder::MSG_STAT_RECENT )
      score += SCORE_RECENT;

   if ( !(status & MailFolder::MSG_STAT_SEEN) )
      score += SCORE_UNREAD;

   if ( status & MailFolder::MSG_STAT_FLAGGED )
      score += SCORE_IMPORTANT;

   if ( status & MailFolder::MSG_STAT_ANSWERED )
      score += SCORE_ANSWERED;

   return score;
}

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

   // now, messages are either both deleted or neither one is -- compare
   // according to the other status bits
   return CmpNumeric(ComputeStatusScore(stat1), ComputeStatusScore(stat2));
}

extern "C"
{
   static int SortComparisonFunction(const void *p1, const void *p2)
   {
      // check that the caller didn't forget to acquire the global data lock
      ASSERT_MSG( gs_SortData.mutex.IsLocked(), _T("using unlocked gs_SortData") );

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

                  result = wxStricmp(subj1, subj2);
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

                  result = wxStricmp(value1, value2);
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
               FAIL_MSG(_T("unimplemented"));
#endif // USE_HEADER_SCORE
               break;

            default:
               FAIL_MSG(_T("unknown sorting criterium"));
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
   HeaderInfoList_obj hil(GetHeaders());

   CHECK( hil, false, _T("no listing to sort") );

   MsgnoType count = hil->Count();

   // HeaderInfoList::Sort() is supposed to check for these cases
   ASSERT_MSG( GetSortCritDirect(sortParams.sortOrder) != MSO_NONE,
               _T("nothing to do in Sort() - shouldn't be called") );

   ASSERT_MSG( count > 1,
               _T("nothing to sort in Sort() - shouldn't be called") );

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

   return true;
}

// ----------------------------------------------------------------------------
// MailFolderCmn threading
// ----------------------------------------------------------------------------

bool MailFolderCmn::ThreadMessages(const ThreadParams& thrParams,
                                   ThreadData *thrData)
{
   HeaderInfoList_obj hil(GetHeaders());

   CHECK( hil, false, _T("no listing to sort") );

   MsgnoType count = hil->Count();

   // HeaderInfoList::Thread() is supposed to check for these cases
   ASSERT_MSG( thrParams.useThreading,
               _T("nothing to do in ThreadMessages() - shouldn't be called") );

   ASSERT_MSG( count > 1,
               _T("nothing to sort in ThreadMessages() - shouldn't be called") );

   // we need all headers, prefetch them
   hil->CacheMsgnos(1, count);

   // do thread!
   JWZThreadMessages(thrParams, hil.operator->(), thrData);

   return true;
}

// ----------------------------------------------------------------------------
// MailFolderCmn misc
// ----------------------------------------------------------------------------

void
MailFolderCmn::ResumeUpdates()
{
   if ( !--m_suspendUpdates )
   {
      RequestUpdate();
      
      // make the folder notice the new messages
      // This step was skipped in MailFolderCC::UpdateAfterAppend
      Ping();
   }
}

void
MailFolderCmn::RequestUpdate()
{
   if ( IsUpdateSuspended() )
      return;

   wxLogTrace(TRACE_MF_EVENTS, _T("Sending FolderUpdate event for folder '%s'"),
              GetName().c_str());

   // remember that the GUI is going to know about that many messages
   m_msgnoLastNotified = GetMessageCount();

   // tell all interested that the folder changed
   MEventManager::Send(new MEventFolderUpdateData(this));
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
MailFolderCmn::OnOptionsChange(MEventOptionsChangeData::ChangeKind /* kind */)
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
      {
         m_Timer->Start(interval, TRUE /* one shot */);
      }
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


bool
MailFolderCmn::DeleteMessage(unsigned long uid)
{
   return SetMessageFlag(uid, MSG_STAT_DELETED, true);
}

bool
MailFolderCmn::UnDeleteMessage(unsigned long uid)
{
   return SetMessageFlag(uid, MSG_STAT_DELETED, false);
}

bool
MailFolderCmn::DeleteOrTrashMessages(const UIdArray *selections,
                                     int flags)
{
   CHECK( CanDeleteMessagesInFolder(GetType()), FALSE,
          _T("can't delete messages in this folder") );

   // we can either delete the messages by moving them to the trash folder and
   // expunging them from this one or by marking them deleted and leaving them
   // in place
   bool useTrash = READ_CONFIG_BOOL(GetProfile(), MP_USE_TRASH_FOLDER);

   wxString trashFolderName;
   if ( useTrash )
   {
      // however if we are the trash folder, we can't use it
      trashFolderName = READ_CONFIG_TEXT(GetProfile(), MP_TRASH_FOLDER);
      if ( GetName() == trashFolderName )
         useTrash = false;
   }

   bool rc;
   if ( useTrash )
   {
      // skip moving the messages to trash if the caller disabled it: in this
      // case we keep the same apparent behaviour as usual (i.e. unlike in the
      // !useTrash case, no messages marked as deleted are left in the folder)
      // but we don't copy the messages unnecessarily -- this is especially
      // useful when moving the messages, as when we delete them after having
      // successfully copied them elsewhere we really don't need to keep yet
      // another copy in the trash
      rc = flags & DELETE_NO_TRASH ? true
                                   : SaveMessages(selections, trashFolderName);
      if ( rc )
      {
         // delete and expunge
         //
         // TODO: unset the 'DELETED' flag for the currently deleted messages
         //       before calling DeleteMessages(selections) and set it back
         //       afterwards: see bug 653 at
         //
         //       http://mahogany.sourceforge.net/cgi-bin/show_bug.cgi?id=653
         rc = DeleteMessages(selections, TRUE /* expunge */);
      }

   }
   else // delete in place
   {
      // delete without expunging
      rc = DeleteMessages(selections, FALSE /* don't expunge */);
   }

   return rc;
}

bool
MailFolderCmn::DeleteMessages(const UIdArray *selections, int flags)
{
   CHECK( selections, false,
          _T("NULL selections in MailFolderCmn::DeleteMessages") );

   Sequence seq;
   seq.AddArray(*selections);

   bool rc = SetSequenceFlag(SEQ_UID, seq, MailFolder::MSG_STAT_DELETED);
   if ( rc && (flags & DELETE_EXPUNGE) )
      ExpungeMessages();

   return rc;
}

// ----------------------------------------------------------------------------
// MailFolderCmn filtering
// ----------------------------------------------------------------------------

/// apply the filters to the selected messages
int
MailFolderCmn::ApplyFilterRules(const UIdArray& msgsOrig)
{
   MFolder_obj folder(GetName());
   CHECK( folder, FilterRule::Error, _T("ApplyFilterRules: no MFolder?") );

   int rc = 0;

   FilterRule *filterRule = GetFilterForFolder(folder);
   if ( filterRule )
   {
      MBusyCursor busyCursor;

      // make copy as Apply() may modify it
      UIdArray msgs = msgsOrig;
      rc = filterRule->Apply(this, msgs);

      filterRule->DecRef();
   }
   else // no filters to apply
   {
      // do give the message because this function is never called
      // automatically but only when the user selects the menu command manually
      // and so it would be surprizing if nothing happened and no message were
      // given
      wxLogMessage(_("No filters configured for this folder."));
   }

   return rc;
}

// apply filters to the new mail, remove the UIDs deleted by the filters from
// uidsNew array
bool
MailFolderCmn::FilterNewMail(FilterRule *filterRule, UIdArray& uidsNew)
{
   CHECK( filterRule, false, _T("FilterNewMail: NULL filter") );

   wxLogTrace(TRACE_MF_NEWMAIL, _T("MF(%s)::FilterNewMail(%lu msgs)"),
              GetName().c_str(), (unsigned long)uidsNew.GetCount());

   // Shut up some messages down in OverviewData::UpdateProgress
   FilterNewMailContext context(RefCounter<FilterRule>::convert(filterRule));

   // we're almost surely going to look at all new messages, so pre-cache them
   // all at once
   CacheLastMessages(uidsNew.GetCount());

   // apply the filters finally
   int rc = filterRule->Apply(this, uidsNew);

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
         ExpungeMessages();
      }
   }

   // some messages could have been deleted by filters
   wxLogTrace(TRACE_MF_NEWMAIL, _T("MF(%s)::FilterNewMail(): %lu msgs left"),
              GetName().c_str(), (unsigned long)uidsNew.GetCount());

   return true;
}

// ----------------------------------------------------------------------------
// MailFolderCmn new mail processing
// ----------------------------------------------------------------------------

/*
   DoProcessNewMail() is very confusing as it is really a merge of three
   different functions: one which processes the new mail in the folder itself,
   the other which processes new mail in another folder which were copied there
   from this one and yet another which processes new mail in another folder but
   when we don't know what the new messages are. I realize that it is very
   unclear but it was the only way I found to share the code for these two
   tasks (or, to be precise, the others were even worse). The only thing I did
   was to make it private and provide 2 wrappers for it as public
   ProcessNewMail() methods.

   So there are three cases:

   1. we have new mail in this folder, in this case mf is the opened folder for
      folder and mfNew is the same as mf and we do the usual sequence of events
      (filter/collect/report) without any problems as this folder is already
      opened and so we can do everything we want.

   2. we have copied some new mail from this folder to another one for which we
      don't have an opened MailFolder (this condition is important as it
      explains why do we go through all these troubles). In this case, mf is
      NULL and the folder where the new mail is is only identified by folder
      parameter but mfNew is the folder where the new mail had originally been.

      We do the same steps as above (filter/collect/report) except that if we
      notice that we do need to filter or collect messages we just open folder
      and return - opening it is enough to ensure that its own ProcessNewMail()
      is called in its 1st version. But if we don't need to filter not collect
      we ReportNewMail() without ever opening the folder - which is a big gain
      (imagine that we filter 10 messages we just got and they go to 5
      different folders: we really don't need to open all 5 of them just to
      report new mail in them, we can do it immediately)

   3. we have new mail in some not opened folder and it wasn't copied there by
      us - this is like (2) except that mfNew is also NULL and uidsNew is NULL
      as well but countNew is provided instead

      The only difference between this case and (2) is that we can't even give
      detailed new mail notification (i.e. showing the subjects and senders of
      the new messages) but we just tersely say that "N new messages" were
      received.
 */

/* static */
bool
MailFolderCmn::DoProcessNewMail(const MFolder *folder,
                                MailFolder *mf,
                                UIdArray *uidsNew,
                                MsgnoType countNew,
                                MailFolder *mfNew)
{
   // if we don't have uidsNew, the folder can't be opened
   if ( !uidsNew )
   {
      CHECK( !mf, false, _T("ProcessNewMail: bad parameters") );

      // and we are also supposed to have at least the number of new messages
      ASSERT_MSG( countNew, _T("ProcessNewMail: no new mail at all?") );
   }

   Profile_obj profile(folder->GetProfile());

   // first filter the messages
   // -------------------------

   FilterRule *filterRule = GetFilterForFolder(folder);
   if ( filterRule )
   {
      bool ok;

      if ( !mf )
      {
         // we need to open the folder where the new mail is
         mf = OpenFolder(folder);
         if ( !mf )
         {
            ok = false;
         }

         mf->DecRef();

         // important for test below
         mf = NULL;

         ok = true;
      }
      else // new mail in this folder, filter it
      {
         ok = ((MailFolderCmn *)mf)->FilterNewMail(filterRule, *uidsNew);
      }

      filterRule->DecRef();

      // return if an error occured or if we had opened the folder - in this
      // case we have nothing to do here any more as this folder processed (or
      // is going to process) the new mail in it itself
      if ( !ok || !mf )
         return ok;

      if ( uidsNew->IsEmpty() )
      {
         // all new mail was deleted by the filters, nothing more to do
         return true;
      }
   }

   // next copy/move all new mail to the central new mail folder if needed
   // --------------------------------------------------------------------

   // do we collect messages from this folder at all?
   if ( folder->GetFlags() & MF_FLAGS_INCOMING )
   {
      // where do we collect the mail?
      String newMailFolder = READ_CONFIG(profile, MP_NEWMAIL_FOLDER);
      if ( newMailFolder == folder->GetFullName() )
      {
         ERRORMESSAGE((_("Cannot collect mail from folder '%s' into itself, "
                         "please modify the properties for this folder.\n"
                         "\n"
                         "Disabling automatic mail collection for now."),
                      newMailFolder.c_str()));

         ((MFolder *)folder)->ResetFlags(MF_FLAGS_INCOMING); // const_cast

         return false;
      }

      if ( !mf )
      {
         // we need to open the folder where the new mail is
         mf = OpenFolder(folder);
         if ( !mf )
         {
            return false;
         }

         mf->DecRef();

         return true;
      }
      //else: new mail in this folder, collect it

      if ( !((MailFolderCmn *)mf)->CollectNewMail(*uidsNew, newMailFolder) )
      {
         return false;
      }

      if ( uidsNew->IsEmpty() )
      {
         // we moved everything elsewhere, nothing left
         return true;
      }
   }

   // finally, notify the user about it
   // ---------------------------------

   // we don't notify about the new mail in the folder the user has just opened
   // himself: he'd see it there anyhow
   if ( !mf || !mf->GetInteractiveFrame() )
   {
      ReportNewMail(folder, uidsNew, countNew, mfNew);
   }

   return true;
}

bool MailFolderCmn::ProcessNewMail(UIdArray& uidsNew,
                                   const MFolder *folderDst)
{
   wxLogTrace(TRACE_MF_NEWMAIL, _T("MF(%s)::ProcessNewMail(%lu msgs) for %s"),
              GetName().c_str(),
              (unsigned long)uidsNew.GetCount(),
              folderDst ? folderDst->GetFullName().c_str() : _T("ourselves"));

   // use the settings for the folder where the new mail is!
   MFolder *folderWithNewMail;
   if ( folderDst )
   {
      // use the folder where new mail is (need to const_cast it to IncRef())
      folderWithNewMail = (MFolder *)folderDst;

      // to compensate for DecRef() done by MFolder_obj
      folderWithNewMail->IncRef();
   }
   else // use this folder
   {
      folderWithNewMail = MFolder::Get(GetName());

      // this may happen when opening a folder not from tree: for example, we
      // create a temp folder when viewing the attachments in a message and
      // then this happens as the (only) message in this temp folder is always
      // new
      //
      // the right thing to do is, of course, to ignore it completely as we
      // know that we're never going to monitor in the background a folder not
      // in the tree anyhow (there is no way to configure this neither at the
      // GUI level nor in FolderMonitor API)
      if ( !folderWithNewMail )
         return true;
   }

   MFolder_obj folder(folderWithNewMail);

   return DoProcessNewMail
          (
            folderWithNewMail,
            folderDst ? NULL : this,   // folder where new mail is
            &uidsNew,
            0,                         // count of new messages is unused
            this                       // folder contains UIDs from uidsNew
          );
}

bool
MailFolderCmn::CollectNewMail(UIdArray& uidsNew, const String& newMailFolder)
{
   // should we do this here or, knowing that we'll download all message bodies
   // and not just the headers anyhow, it would be superfluous?
   //CacheLastMessages(uidsNew.GetCount());

   bool move = READ_CONFIG_BOOL(GetProfile(), MP_MOVE_NEWMAIL);

   wxLogTrace(TRACE_MF_NEWMAIL, _T("MF(%s)::CollectNewMail(%lu msgs) (%s)"),
              GetName().c_str(),
              (unsigned long)uidsNew.GetCount(),
              move ? "moving" : "copying");

   if ( !SaveMessages(&uidsNew, newMailFolder) )
   {
      // don't delete them if we failed to save them
      ERRORMESSAGE((_("Cannot %s new mail from folder '%s' to '%s'."),
                    move ? _("move") : _("copy"),
                    GetName().c_str(),
                    newMailFolder.c_str()));

      return false;
   }

   if ( move )
   {
      // delete and expunge
      DeleteMessages(&uidsNew, true);

      // no new mail left here
      uidsNew.Clear();
   }

   return true;
}

/*
   The parameters have the following meaning:

   folder is the folder where the new mail has arrived, never NULL
   uidsNew are the UIDs of the new messages in the folder mf (may be NULL)
   countNew is the number of new messages if (and only if) uidsNew == NULL
   mf is the opened folder where new mail is or NULL (then uidsNew is too)
 */

/* static */
void
MailFolderCmn::ReportNewMail(const MFolder *folder,
                             const UIdArray *uidsNew,
                             MsgnoType countNew,
                             MailFolder *mf)
{
   CHECK_RET( folder, _T("ReportNewMail: NULL folder") );

   // first of all, do nothing at all in the away mode
   if ( mApplication->IsInAwayMode() )
   {
      // avoid any interaction with the user
      return;
   }

   // the count is only given if the array itself is not
   if ( uidsNew )
      countNew = uidsNew->GetCount();

   wxLogTrace(TRACE_MF_NEWMAIL, _T("MF(%s)::ReportNewMail(%u msgs) (folder is %s)"),
              folder->GetFullName().c_str(),
              (unsigned int)countNew,
              mf ? "opened" : "closed");

   // step 1: execute external command if it's configured
   Profile_obj profile(folder->GetProfile());
   if ( READ_CONFIG(profile, MP_USE_NEWMAILCOMMAND) )
   {
      String command = READ_CONFIG(profile, MP_NEWMAILCOMMAND);
      if ( !command.empty() )
      {
         wxLogTrace(TRACE_MF_NEWMAIL, _T("MF(%s)::ReportNewMail(): running '%s'"),
                    folder->GetFullName().c_str(), command.c_str());

         if ( !wxExecute(command, false /* async */) )
         {
            // TODO ask whether the user wants to disable it
            wxLogError(_("Command '%s' (to execute on new mail reception)"
                         " failed."), command.c_str());
         }
      }
   }

   // step 2: play a sound
   if ( READ_CONFIG(profile, MP_NEWMAIL_PLAY_SOUND) )
   {
      String sound = READ_CONFIG(profile, MP_NEWMAIL_SOUND_FILE);

#if defined(OS_WIN) && !defined(__CYGWIN__) && !defined(__MINGW32__) // FIXME
      DWORD flags = SND_ASYNC;

      if ( sound.empty() )
      {
         // use the system default sound
         sound = "MailBeep";
         flags |= SND_ALIAS;
      }
      else // file configured
      {
         flags |= SND_FILENAME;
      }

      if ( !::PlaySound(sound, NULL, flags) )
#elif defined(OS_UNIX) || defined(__CYGWIN__)
      String soundCmd = READ_CONFIG(profile, MP_NEWMAIL_SOUND_PROGRAM);

      // we have a handy function in wxFileType which will replace
      // '%s' with the file name or add the file name at the end if
      // there is no '%s'
      wxFileType::MessageParameters params(sound, _T(""));
      String command = wxFileType::ExpandCommand(soundCmd, params);

      wxLogTrace(TRACE_MF_NEWMAIL, _T("MF(%s)::ReportNewMail(): playing '%s'"),
                 folder->GetFullName().c_str(), command.c_str());

      if ( !wxExecute(command, false /* async */) )
#elif defined(__MINGW32__)
#else // other platform
   #error "don't know how to play sounds on this platform"
#endif
      {
         // TODO ask whether the user wants to disable it
         wxLogWarning(_("Failed to play new mail sound."));
      }
   }

#ifdef USE_PYTHON
   // step 3: folder specific Python callback
   if ( !PythonCallback(MCB_FOLDER_NEWMAIL, 0,
                        (MFolder *)folder, "MFolder", profile) ) // const_cast

      // step 4: global python callback
      if ( !PythonCallback(MCB_MAPPLICATION_NEWMAIL, 0,
                           mApplication, "MApplication",
                           mApplication->GetProfile()) )
#endif //USE_PYTHON
      {
         // step 5: show notification
         if ( READ_CONFIG(profile, MP_SHOW_NEWMAILMSG) )
         {
            String message;
            message.Printf(_("You have received %lu new messages "
                             "in the folder '%s'"),
                           countNew, folder->GetFullName().c_str());

            // we give the detailed new mail information when there are few new
            // mail messages, otherwise we just give a brief message with their
            // number
            long detailsThreshold;

            // we can't show the detailed new mail information if the folder is
            // not opened and it seems wasteful to open it just for this
            //
            // OTOH, the user can already configure the folder to never show
            // the details of new mail if he wants it to behave like this so
            // maybe we should still open it here? (TODO)
            if ( !uidsNew || !mf )
            {
               detailsThreshold = 0;
            }
            else // we have the new messages
            {
               // the threshold may be set to -1 to always give the detailed
               // message
               detailsThreshold = READ_CONFIG(profile, MP_SHOW_NEWMAILINFO);
            }

            if ( detailsThreshold == -1 ||
                 countNew < (unsigned long)detailsThreshold )
            {
               message += ':';

               for( unsigned long i = 0; i < countNew; i++)
               {
                  Message *msg = mf->GetMessage(uidsNew->Item(i));
                  if ( msg )
                  {
                     String from = msg->From();
                     if ( from.empty() )
                     {
                        from = _("unknown sender");
                     }

                     String subject = msg->Subject();
                     if ( subject.empty() )
                     {
                        subject = _("without subject");
                     }
                     else
                     {
                        String s;
                        s << _(" about '") << subject << '\'';
                        subject = s;
                     }

                     message << '\n'
                             << _("\tFrom: ") << from << subject;

                     msg->DecRef();
                  }
                  else // no message?
                  {
                     // this may happen if another session deleted it
                     wxLogDebug(_T("New message %lu disappeared from folder '%s'"),
                                uidsNew->Item(i),
                                folder->GetFullName().c_str());
                  }
               }
            }
            else // too many new messages
            {
               // don't give the details
               message += '.';
            }

            LOGMESSAGE((M_LOG_WINONLY, message));
         }
      }
   //else: new mail reported by the Python code
}

// ----------------------------------------------------------------------------
// MailFolderCmn message counting
// ----------------------------------------------------------------------------

bool MailFolderCmn::CountAllMessages(MailFolderStatus *status) const
{
   CHECK( status, false, _T("CountAllMessages: NULL pointer") );

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
// MailFolderCmn message status
// ----------------------------------------------------------------------------

// msg status info
//
// NB: name MessageStatus is already taken by MailFolder
struct MsgStatus
{
   bool recent;
   bool unread;
   bool newmsgs;
   bool flagged;
   bool searched;
};

// is this message recent/new/unread/...?
static MsgStatus AnalyzeStatus(int stat)
{
   MsgStatus status;

   // deal with recent and new (== recent and !seen)
   status.recent = (stat & MailFolder::MSG_STAT_RECENT) != 0;
   status.unread = !(stat & MailFolder::MSG_STAT_SEEN);
   status.newmsgs = status.recent && status.unread;

   // and also count flagged and searched messages
   status.flagged = (stat & MailFolder::MSG_STAT_FLAGGED) != 0;
   status.searched = (stat & MailFolder::MSG_STAT_SEARCHED ) != 0;

   return status;
}

void
MailFolderCmn::SendMsgStatusChangeEvent()
{
   if ( !m_statusChangeData )
   {
      // nothing to do
      return;
   }

   // first update the cached status
   MailFolderStatus status;
   MfStatusCache *mfStatusCache = MfStatusCache::Get();
   if ( mfStatusCache->GetStatus(GetName(), &status) )
   {
      size_t count = m_statusChangeData->msgnos.GetCount();
      for ( size_t n = 0; n < count; )
      {
         int statusOld = m_statusChangeData->statusOld[n];
         bool wasDeleted = (statusOld & MSG_STAT_DELETED) != 0;

         int statusNew;
         bool isDeleted;

         if ( m_statusChangeData->msgnos[n] == MSGNO_ILLEGAL )
         {
            // this means that this message has been expunged
            isDeleted = true;

            // unused anyhow if isDeleted == true
            statusNew = 0;

            // delete it from the list which GUI will see -- it doesn't need to
            // know about it as it doesn't need to update the status of non
            // existing message any more
            m_statusChangeData->Remove(n);

            count--;
         }
         else // message still exists
         {
            statusNew = m_statusChangeData->statusNew[n];
            isDeleted = (statusNew & MSG_STAT_DELETED) != 0;

            // pass to the next one
            n++;
         }

         MsgStatus msgStatusOld = AnalyzeStatus(statusOld),
                   msgStatusNew = AnalyzeStatus(statusNew);

         // we consider that a message has some flag only if it is not deleted
         // (which is discussable at least for flagged and searched flags
         // although, OTOH, why flag a deleted message?)

         #define UPDATE_NUM_OF(what)   \
            if ( (!isDeleted && msgStatusNew.what) && \
                 (wasDeleted || !msgStatusOld.what) ) \
            { \
               wxLogTrace(M_TRACE_MFSTATUS, _T("%s: " #what "++ (now %lu)"), \
                          GetName().c_str(), status.what + 1); \
               status.what++; \
            } \
            else if ( (!wasDeleted && msgStatusOld.what) && \
                      (isDeleted || !msgStatusNew.what) ) \
               if ( status.what > 0 ) \
               { \
                  wxLogTrace(M_TRACE_MFSTATUS, _T("%s: " #what "-- (now %lu)"), \
                             GetName().c_str(), status.what - 1); \
                  status.what--; \
               } \
               else \
                  FAIL_MSG( _T("error in msg status change logic") )

         UPDATE_NUM_OF(recent);
         UPDATE_NUM_OF(unread);
         UPDATE_NUM_OF(newmsgs);
         UPDATE_NUM_OF(flagged);
         UPDATE_NUM_OF(searched);

         #undef UPDATE_NUM_OF
      }

      mfStatusCache->UpdateStatus(GetName(), status);
   }
   else // no cached status
   {
      // we still have to weed out the expunged messages, just as we do above
      size_t count = m_statusChangeData->msgnos.GetCount();
      for ( size_t n = 0; n < count; )
      {
         if ( m_statusChangeData->msgnos[n] == MSGNO_ILLEGAL )
         {
            m_statusChangeData->Remove(n);

            count--;
         }
         else // message still exists
         {
            n++;
         }
      }
   }

   // next notify everyone else about the status change
   wxLogTrace(TRACE_MF_EVENTS,
              _T("Sending MsgStatus event for %lu msgs in folder '%s'"),
              (unsigned long)m_statusChangeData->msgnos.GetCount(),
              GetName().c_str());

   MEventManager::Send(new MEventMsgStatusData(this, m_statusChangeData));

   // MEventMsgStatusData will delete them
   m_statusChangeData = NULL;
}

// ----------------------------------------------------------------------------
// MailFolderCmn expunge data
// ----------------------------------------------------------------------------

void MailFolderCmn::DiscardExpungeData()
{
   if ( m_expungeData )
   {
      delete m_expungeData;

      m_expungeData = NULL;
   }
}

void MailFolderCmn::RequestUpdateAfterExpunge()
{
   CHECK_RET( m_expungeData, _T("shouldn't be called if we didn't expunge") );

   // FIXME: we ignore IsUpdateSuspended() here, should we? and if not,
   //        what to do?

   // we can update the status faster here as we have decremented the
   // number of recent/unseen/... when the messages were deleted (before
   // being expunged), so we just have to update the total now
   //
   // NB: although this has all chances to break down with manual expunge
   //     or even with automatic one due to race condition (if the
   //     message status changed from outside...) - but this is so much
   //     faster and the problem is not really fatal that I still prefer
   //     to do it like this
   MailFolderStatus status;
   MfStatusCache *mfStatusCache = MfStatusCache::Get();
   if ( mfStatusCache->GetStatus(GetName(), &status) )
   {
      // caller is supposed to check for this!
      CHECK_RET( IsOpened(), _T("RequestUpdateAfterExpunge(): closed folder?") );

      status.total = GetMessageCount();

      mfStatusCache->UpdateStatus(GetName(), status);
   }

   // tell GUI to update
   wxLogTrace(TRACE_MF_EVENTS, _T("Sending FolderExpunged event for folder '%s'"),
              GetName().c_str());

   MEventManager::Send(new MEventFolderExpungeData(this, m_expungeData));

   // GUI is going to know about less messages now
   if ( m_msgnoLastNotified )
   {
      const size_t countExp = m_expungeData->msgnos.GetCount();

      ASSERT_MSG( m_msgnoLastNotified >= countExp,
                     _T("we expunged more messages that we had?") );

      m_msgnoLastNotified -= countExp;
   }

   // MEventFolderExpungeData() will delete the data
   m_expungeData = NULL;
}

// ----------------------------------------------------------------------------
// MailFolderCmn interactive frame functions
// ----------------------------------------------------------------------------

wxFrame *MailFolderCmn::SetInteractiveFrame(wxFrame *frame)
{
   wxFrame *frameOld = m_frame;
   m_frame = frame;
   return frameOld;
}

wxFrame *MailFolderCmn::GetInteractiveFrame() const
{
   // no interactivity at all in away mode
   return mApplication->IsInAwayMode() ? NULL : m_frame;
}

// ----------------------------------------------------------------------------
// MfCmnEventReceiver
// ----------------------------------------------------------------------------

MfCmnEventReceiver::MfCmnEventReceiver(MailFolderCmn *mf)
{
   m_Mf = mf;

   m_MEventCookie = MEventManager::Register(*this, MEventId_OptionsChange);
   ASSERT_MSG( m_MEventCookie, _T("can't reg folder with event manager"));

   m_MEventPingCookie = MEventManager::Register(*this, MEventId_Ping);
   ASSERT_MSG( m_MEventPingCookie, _T("can't reg folder with event manager"));
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

bool MailFolderCmnInit()
{
   if ( !gs_MailFolderCloser )
   {
      gs_MailFolderCloser = new MfCloser;
   }

   return true;
}

void MailFolderCmnCleanup()
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

static MFSubSystem gs_subsysCmn(MailFolderCmnInit, MailFolderCmnCleanup);

FilterNewMailContext *FilterNewMailContext::m_instance;
