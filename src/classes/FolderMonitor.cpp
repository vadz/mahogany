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

#ifdef __GNUG__
   #pragma implementation "FolderMonitor.h"
#endif

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef   USE_PCH
   #include "Mcommon.h"
   #include "lists.h"
#endif   // USE_PCH

#include "MFolder.h"

#include "FolderMonitor.h"

#include "MThread.h"

#include "MApplication.h"  // for IsOnline()

#include "MDialogs.h"      // MDialog_YesNoDialog
#include "Mpers.h"

// ----------------------------------------------------------------------------
// options we use
// ----------------------------------------------------------------------------

extern const MOption MP_COLLECTATSTARTUP;
extern const MOption MP_POLLINCOMINGDELAY;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// the number of times we try to open an unaccessible folder before giving up
static const int MC_MAX_FAIL = 5;

// trace mask
#define TRACE_MONITOR "monitor"

// folder state
enum FolderState
{
   // can be checked
   Folder_Ok,

   // can't be checked now but might be checked later
   Folder_TempUnavailable,

   // can't be checked now nor later
   Folder_Unaccessible
};

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
         FAIL_MSG( "NULL folder in FolderMonitorFolderEntry?" );
      }

      m_failcount = 0;
      m_state = Folder_Ok;
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

   // next check time
   void UpdateCheckTime()
   {
      Profile_obj profile(m_folder->GetProfile());

      m_timeNext = time(NULL) + READ_CONFIG(profile, MP_POLLINCOMINGDELAY);

      wxLogTrace(TRACE_MONITOR, "Next check for %s scheduled for %s",
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
      : MFolderTraversal(*(m_folder = MFolder::Get(""))), m_list(list)
      { }
   ~FolderMonitorTraversal()
      { m_folder->DecRef(); }

   virtual bool OnVisitFolder(const wxString& folderName)
      {
         MFolder_obj folder(folderName);

         CHECK( folder, true, "traversed a non-existing folder?" );

         if ( folder->GetFlags() & MF_FLAGS_MONITOR )
         {
            wxLogTrace(TRACE_MONITOR, "Found folder to monitor: %s",
                       folderName.c_str());

            m_list.push_back(new FolderMonitorFolderEntry(folder));
         }

         return true;
      }

private:
   MFolder *m_folder;

   MMutex m_inNewMailCheck;

   FolderMonitorFolderList& m_list;
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

   // react to folder deletion by removing it from the list of folders to poll
   virtual bool OnMEvent(MEventData& event);

protected:
   /// check for new mail in this one folder only
   bool CheckOneFolder(FolderMonitorFolderEntry *i, bool interactive);

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

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// translate bool interactive flag to MailFolder open mode
static inline MailFolder::OpenMode GetMode(bool interactive)
{
   return interactive ? MailFolder::Normal : MailFolder::Silent;
}

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

   CHECK( !s_folderMonitor, NULL, "FolderMonitor::Create() called twice!" );

   // do create it
   s_folderMonitor = new FolderMonitorImpl;

   return s_folderMonitor;
}

FolderMonitorImpl::FolderMonitorImpl()
{
   m_regFolderDelete = MEventManager::Register(*this, MEventId_FolderTreeChange);
   m_regOptionsChange = MEventManager::Register(*this, MEventId_OptionsChange);

   BuildList();

   for ( FolderMonitorFolderList::iterator i = m_list.begin();
         i != m_list.end();
         ++i )
   {
      Profile_obj profile(i->GetFolder()->GetProfile());
      if ( READ_CONFIG_BOOL(profile, MP_COLLECTATSTARTUP) )
      {
         (void)CheckOneFolder(i.operator->(), true /* interactive check */);
      }
   }
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

bool
FolderMonitorImpl::AddOrRemoveFolder(MFolder *folder, bool monitor)
{
   CHECK( folder, false, "FolderMonitor::AddOrRemoveFolder(): NULL folder" );

#ifdef DEBUG
   if ( IsBeingMonitored(folder) == monitor )
   {
      FAIL_MSG( "FolderMonitor::AddOrRemoveFolder(): nothing to do!" );

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
         FAIL_MSG( "logic error in FolderMonitor::AddOrRemoveFolder()" );

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
   CHECK( !m_inNewMailCheck.IsLocked(), false, "can't remove it now" );

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

   time_t timeCur = time(NULL);

   MLocker lockNewMailCheck(m_inNewMailCheck);

   bool rc = true;

   // check all opened folders if requested
   if ( flags & Opened )
   {
      if ( !MailFolder::PingAllOpened(GetMode((flags & Interactive) != 0)) )
         rc = false;
   }

   // check all incoming folders
   for ( FolderMonitorFolderList::iterator i = m_list.begin();
         i != m_list.end();
         ++i )
   {
      // force the check now if it was done by the user, even if the timeout
      // hasn't expired yet
      if ( (flags & Interactive) || (i->GetCheckTime() < timeCur) )
      {
         if ( !CheckOneFolder(i.operator->(), (flags & Interactive) != 0) )
            rc = false;
      }
      //else: don't check this folder yet
   }

   return rc;
}

bool
FolderMonitorImpl::CheckOneFolder(FolderMonitorFolderEntry *i, bool interactive)
{
   const MFolder *folder = i->GetFolder();

   switch ( i->GetState() )
   {
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

      case Folder_Unaccessible:
         // don't even try any more
         return true;

      default:
         FAIL_MSG( "unknown folder state" );
         // fall through

      case Folder_Ok:
         // nothing to do
         ;
   }

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

   // update the time of the next check
   i->UpdateCheckTime();

   wxLogTrace(TRACE_MONITOR, "Checking for new mail in '%s'.",
              i->GetName().c_str());

   if ( interactive )
   {
      // TODO: show to the user that we're doing something
      ;
   }

   // don't show the dialogs in non-interactive mode
   if ( !MailFolder::CheckFolder(folder, GetMode(interactive)) )
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
               TRUE,
               GetPersMsgBoxName(M_MSGBOX_SUSPENDAUTOCOLLECT)
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

