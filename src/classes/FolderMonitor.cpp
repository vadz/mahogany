///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   FolderMonitor.h - class which pings folders periodically
// Purpose:     declares FolderMonitor class
// Author:      Vadim Zeitlin
// Modified by:
// Created:     13.11.01 (based on MailCollector.cpp)
// CVS-ID:      $Id$
// Copyright:   (c) 1999-2000 Karsten Ballüder (Ballueder@gmx.net)
//              (c) 2001      Vadim Zeitlin    <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef   USE_PCH
#  include "Mcommon.h"
#  include "Mdefaults.h"
#  include "MEvent.h"
#  include "MApplication.h"

#  ifdef USE_DIALUP
#     include "MApplication.h"  // for IsOnline()
#  endif // USE_DIALUP
#endif   // USE_PCH

#include "MThread.h"
#include "lists.h"
#include "MFolder.h"
#include "MailFolder.h"

#include "FolderMonitor.h"

#include "gui/wxMDialogs.h"      // MDialog_YesNoDialog

class MOption;

// ----------------------------------------------------------------------------
// options we use
// ----------------------------------------------------------------------------

extern const MOption MP_COLLECTATSTARTUP;
extern const MOption MP_POLL_OPENED_ONLY;
extern const MOption MP_POLLINCOMINGDELAY;

// ----------------------------------------------------------------------------
// persistent msgboxes we use here
// ----------------------------------------------------------------------------

extern const MPersMsgBox *M_MSGBOX_SUSPENDAUTOCOLLECT;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// the number of times we try to open an unaccessible folder before giving up
static const int MC_MAX_FAIL = 5;

// trace mask
#define TRACE_MONITOR _T("monitor")

// folder state
enum FolderState
{
   // can be checked
   Folder_Ok,

#ifdef USE_DIALUP
   // can't be checked now but might be checked later
   Folder_TempUnavailable,
#endif // USE_DIALUP

   // can't be checked now nor later
   Folder_Unaccessible
};

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

/// create the progress info object to show while we're doing the check
static MProgressInfo *CreateProgressInfo()
{
   // intentionally make the label long to avoid clipping it when it is changed
   // later
   return new MProgressInfo(NULL,
                            _("Checking for new mail, please wait..."),
                            _("Checking for new mail"));
}

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// FolderMonitorFolderEntry holds the folder it corresponds to and the moment
// when it should be pinged the next time as well as the number of times we
// have already failed to ping it - when it becomes too high, we stop trying to
// do it
class FolderMonitorFolderEntry
{
public:
   // ctor/dtor
   FolderMonitorFolderEntry(MFolder *folder)
   {
      m_folder = folder;
      if ( m_folder )
      {
         m_folder->IncRef();
      }
      else
      {
         FAIL_MSG( _T("NULL folder in FolderMonitorFolderEntry?") );
      }

      m_failcount = 0;
      m_state = Folder_Ok;
      m_timeNext = time(NULL);
   }

   ~FolderMonitorFolderEntry()
   {
      if ( m_folder )
         m_folder->DecRef();
   }

   // accessors
   MFolder *GetFolder() const { return m_folder; } // NOT IncRef()'d!
   String GetName() const { return m_folder->GetFullName(); }

   // state
   void SetState(FolderState state) { m_state = state; }
   FolderState GetState() const { return m_state; }

   // fail count
   bool IncreaseFailCount() { return ++m_failcount >= MC_MAX_FAIL; }
   void ResetFailCount() { m_failcount = 0; }

   long GetPollInterval() const
   {
      Profile_obj profile(m_folder->GetProfile());

      return READ_CONFIG(profile, MP_POLLINCOMINGDELAY);
   }

   // next check time
   void UpdateCheckTime()
   {
      m_timeNext = time(NULL) + (time_t)GetPollInterval();

      wxLogTrace(TRACE_MONITOR, _T("Next check for %s scheduled for %s"),
                 m_folder->GetFullName().c_str(),
                 ctime(&m_timeNext));
   }

   time_t GetCheckTime() const { return m_timeNext; }

private:
   // the folder we monitor
   MFolder *m_folder;

   // its current state: Ok/Temp Unavailable/Inaccessible
   FolderState m_state;

   // the time of the next check
   time_t m_timeNext;

   /**
     Failcount is 0 initially, when it reaches MC_MAX_FAIL a warning is
     printed. If set to -1, user has been warned about it being inaccessible
     and no more warnings are printed.
    */
   int m_failcount;
};

// declare a list (owning the objects in it) of FolderMonitorFolderEntries
M_LIST_OWN(FolderMonitorFolderList, FolderMonitorFolderEntry);

// ----------------------------------------------------------------------------
// FolderMonitorTraversal used by FolderMonitor to find all incoming folders
// ----------------------------------------------------------------------------

class FolderMonitorTraversal : public MFolderTraversal
{
public:
   FolderMonitorTraversal(FolderMonitorFolderList& list)
      : MFolderTraversal(*(m_folder = MFolder::Get(wxEmptyString))), m_list(list)
      { }
   ~FolderMonitorTraversal()
      { m_folder->DecRef(); }

   virtual bool OnVisitFolder(const wxString& folderName)
      {
         MFolder_obj folder(folderName);

         CHECK( folder, true, _T("traversed a non-existing folder?") );

         if ( folder->GetFlags() & MF_FLAGS_MONITOR )
         {
            wxLogTrace(TRACE_MONITOR, _T("Found folder to monitor: %s"),
                       folderName.c_str());

            m_list.push_back(new FolderMonitorFolderEntry(folder));
         }

         return true;
      }

private:
   MFolder *m_folder;

   MMutex m_inNewMailCheck;

   FolderMonitorFolderList& m_list;

   DECLARE_NO_COPY_CLASS(FolderMonitorTraversal)
};

// ----------------------------------------------------------------------------
// FolderMonitorImpl: the implementation of FolderMonitor ABC
// ----------------------------------------------------------------------------

class FolderMonitorImpl : public FolderMonitor,
                          public MEventReceiver
{
public:
   // ctor/dtor
   FolderMonitorImpl();
   virtual ~FolderMonitorImpl();

   // implement the FolderMonitor pure virtuals
   virtual bool CheckNewMail(int flags);
   virtual bool AddOrRemoveFolder(MFolder *folder, bool add);
   virtual long GetMinCheckTimeout(void) const;

   // react to folder deletion by removing it from the list of folders to poll
   virtual bool OnMEvent(MEventData& event);

protected:
   /// check for new mail in this one folder only
   bool CheckOneFolder(FolderMonitorFolderEntry *i,
                       MProgressInfo *progInfo);

   /// return true if we already have this folder in the incoming list
   bool IsBeingMonitored(const MFolder *folder) const;

   /// initialize m_list with the incoming folders
   void BuildList(void);

   /// remove the folder from the list of the incoming ones
   bool RemoveFolder(const String& name);

private:
   /// a list of folders we monitor
   FolderMonitorFolderList m_list;

   /// the event manager cookies
   void *m_regFolderDelete,
        *m_regOptionsChange;

   /// the mutex locked while we're checking for new mail
   MMutex m_inNewMailCheck;
};

// ============================================================================
// FolderMonitorImpl implementation
// ============================================================================

// ----------------------------------------------------------------------------
// FolderMonitor
// ----------------------------------------------------------------------------

FolderMonitor::~FolderMonitor()
{
}

// ----------------------------------------------------------------------------
// FolderMonitorImpl creation
// ----------------------------------------------------------------------------

/* static */
FolderMonitor *
FolderMonitor::Create(void)
{
   static FolderMonitor *s_folderMonitor = NULL;

   CHECK( !s_folderMonitor, NULL, _T("FolderMonitor::Create() called twice!") );

   // do create it
   s_folderMonitor = new FolderMonitorImpl;

   return s_folderMonitor;
}

FolderMonitorImpl::FolderMonitorImpl()
{
   m_regFolderDelete = MEventManager::Register(*this, MEventId_FolderTreeChange);
   m_regOptionsChange = MEventManager::Register(*this, MEventId_OptionsChange);

   BuildList();

   MProgressInfo *progInfo = NULL;

   for ( FolderMonitorFolderList::iterator i = m_list.begin();
         i != m_list.end();
         ++i )
   {
      if ( !progInfo )
         progInfo = CreateProgressInfo();

      Profile_obj profile(i->GetFolder()->GetProfile());
      if ( READ_CONFIG_BOOL(profile, MP_COLLECTATSTARTUP) )
      {
         (void)CheckOneFolder(*i, progInfo);
      }
   }

   delete progInfo;
}

FolderMonitorImpl::~FolderMonitorImpl(void)
{
   MEventManager::DeregisterAll(&m_regFolderDelete, &m_regOptionsChange, NULL);
}

// ----------------------------------------------------------------------------
// FolderMonitorImpl incoming folders management
// ----------------------------------------------------------------------------

void
FolderMonitorImpl::BuildList(void)
{
   m_list.clear();

   FolderMonitorTraversal t(m_list);
   if ( !t.Traverse(true) )
   {
      wxLogWarning(_("Cannot build list of incoming mail folders."));
   }
}

bool
FolderMonitorImpl::IsBeingMonitored(const MFolder *folder) const
{
   for ( FolderMonitorFolderList::iterator i = m_list.begin();
         i != m_list.end();
         ++i )
   {
      if ( i->GetFolder() == folder )
         return true;
   }

   return false;
}

long
FolderMonitorImpl::GetMinCheckTimeout(void) const
{
   long min_delay = READ_APPCONFIG(MP_POLLINCOMINGDELAY);
   
   for ( FolderMonitorFolderList::iterator i = m_list.begin();
         i != m_list.end();
         ++i )
   {
      long delay = i->GetPollInterval();
      if ((delay > 0) && (delay < min_delay))
         min_delay = delay;
   }

   return min_delay;
}

bool
FolderMonitorImpl::AddOrRemoveFolder(MFolder *folder, bool monitor)
{
   CHECK( folder, false, _T("FolderMonitor::AddOrRemoveFolder(): NULL folder") );

#ifdef DEBUG
   if ( IsBeingMonitored(folder) == monitor )
   {
      FAIL_MSG( _T("FolderMonitor::AddOrRemoveFolder(): nothing to do!") );

      folder->DecRef();

      return false;
   }
#endif // DEBUG

   if ( monitor )
   {
      m_list.push_back(new FolderMonitorFolderEntry(folder));
      folder->AddFlags(MF_FLAGS_MONITOR);
   }
   else
   {
      if ( !RemoveFolder(folder->GetFullName()) )
      {
         // this is impossible in debug mode because of the check above
         FAIL_MSG( _T("logic error in FolderMonitor::AddOrRemoveFolder()") );

         return false;
      }

      folder->ResetFlags(MF_FLAGS_MONITOR);
   }

   return true;
}

bool
FolderMonitorImpl::RemoveFolder(const String& name)
{
   // this could lead to a crash in CheckNewMail()
   CHECK( !m_inNewMailCheck.IsLocked(), false, _T("can't remove it now") );

   for ( FolderMonitorFolderList::iterator i = m_list.begin();
         i != m_list.end();
         ++i )
   {
      if ( i->GetFolder()->GetFullName() == name )
      {
         m_list.erase(i);

         return true;
      }
   }

   return false;
}

// ----------------------------------------------------------------------------
// FolderMonitorImpl worker function
// ----------------------------------------------------------------------------

bool
FolderMonitorImpl::CheckNewMail(int flags)
{
   if ( m_inNewMailCheck.IsLocked() )
      return true;

   MLocker lockNewMailCheck(m_inNewMailCheck);

   bool rc = true;

   // show what we're doing in interactive mode
   MProgressInfo *progInfo;
   if ( flags & Interactive )
   {
      progInfo = CreateProgressInfo();
   }
   else // not interactive
   {
      progInfo = NULL;
   }

   // check all opened folders if requested
   if ( flags & Opened )
   {
      if ( progInfo )
         progInfo->SetLabel(_("Checking all opened folders..."));

      if ( !MailFolder::PingAllOpened(progInfo ? progInfo->GetFrame() : NULL) )
         rc = false;
   }

   // check all incoming folders
   time_t timeCur = time(NULL);
   FolderMonitorFolderList::iterator i;
   for ( i = m_list.begin(); i != m_list.end(); ++i )
   {
      // force the check now if it was done by the user, even if the timeout
      // hasn't expired yet
      if ( (flags & Interactive) || (i->GetCheckTime() <= timeCur) )
      {
         if ( !CheckOneFolder(*i, progInfo) )
            rc = false;
         // update the time of the next check only now, i.e. after CheckFolder() call
         // as if it takes time longer than the check interval we might keep checking
         // it all the time without doing anything else
         i->UpdateCheckTime();
      }
      //else: don't check this folder yet
   }

   delete progInfo;

   return rc;
}

bool
FolderMonitorImpl::CheckOneFolder(FolderMonitorFolderEntry *i,
                                  MProgressInfo *progInfo)
{
   const MFolder *folder = i->GetFolder();

   switch ( i->GetState() )
   {
#ifdef USE_DIALUP
      case Folder_TempUnavailable:
         // if online status changed, the folder might have become accessible
         // again now
         if ( folder->NeedsNetwork() && mApplication->IsOnline() )
         {
            // try again
            i->SetState(Folder_Ok);
            break;
         }
         //else: fall through
#endif // USE_DIALUP

      case Folder_Unaccessible:
         // don't even try any more
         return true;

      default:
         FAIL_MSG( _T("unknown folder state") );
         // fall through

      case Folder_Ok:
         // nothing to do
         ;
   }

   // the user may want to start checking this folder only after it had been
   // opened for the first time, check for this
   Profile_obj profile(folder->GetProfile());
   if ( READ_CONFIG_BOOL(profile, MP_POLL_OPENED_ONLY) )
   {
      MailFolder *mf = MailFolder::GetOpenedFolderFor(folder);
      if ( !mf )
      {
         wxLogTrace(TRACE_MONITOR, _T("Skipping not opened folder %s"),
                    folder->GetFullName().c_str());
         return true;
      }

      mf->DecRef();
   }

#ifdef USE_DIALUP
   if ( folder->NeedsNetwork() && !mApplication->IsOnline() )
   {
      ERRORMESSAGE((_("Cannot check for new mail in the folder '%s' "
                      "while offline.\n"
                      "If you believe this message to be wrong, please "
                      "set the flag allowing to access the folder\n"
                      "without network in its \"Access\" properties page."),
                    i->GetName().c_str()));

      i->SetState(Folder_TempUnavailable);
   }
#endif // USE_DIALUP

   wxLogTrace(TRACE_MONITOR, _T("Checking for new mail in '%s'."),
              i->GetName().c_str());

   if ( progInfo )
   {
      // show to the user that we're doing something
      progInfo->SetLabel(String::Format(_("Checking folder %s..."),
                                        folder->GetFullName().c_str()));
   }

   // don't show the dialogs in non-interactive mode
   if ( !MailFolder::CheckFolder(folder,
                                 progInfo ? progInfo->GetFrame() : NULL) )
   {
      if ( !i->IncreaseFailCount() )
      {
         wxString msg;
         msg.Printf(_("Checking for new mail in the folder '%s' failed.\n"
                      "Do you want to stop checking it during this session?"),
                    i->GetName().c_str());

         if ( MDialog_YesNoDialog
              (
               msg,
               NULL,
               _("Check for new mail failed"),
               M_DLG_YES_DEFAULT,
               M_MSGBOX_SUSPENDAUTOCOLLECT
              ) )
         {
            i->SetState(Folder_Unaccessible);
         }
         else
         {
            // reset failure count for new tries
            i->ResetFailCount();
         }
      }

      return false;
   }

   // process the new mail events for the folder we just checked: if we don't
   // do it know, it might time out and close while we're checking the other
   // folders
   MEventManager::ForceDispatchPending();

   return true;
}

// ----------------------------------------------------------------------------
// FolderMonitorImpl MEvent processing
// ----------------------------------------------------------------------------

bool
FolderMonitorImpl::OnMEvent(MEventData& event)
{
   if ( event.GetId() == MEventId_FolderTreeChange )
   {
      MEventFolderTreeChangeData& evTree = (MEventFolderTreeChangeData &)event;
      if ( evTree.GetChangeKind() == MEventFolderTreeChangeData::Delete )
      {
         RemoveFolder(evTree.GetFolderFullName());
      }
   }
   else if ( event.GetId() == MEventId_OptionsChange )
   {
      // rebuild it, in fact
      BuildList();
   }

   // continue propagating the event
   return true;
}

