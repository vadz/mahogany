// -*- c++ -*-/////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   FolderView.cpp - implementation of FolderView base methods
// Author:      Vadim Zeitlin
// Modified by:
// Created:     08.01.01
// CVS-ID:      $Id$
// Copyright:   (c) 2001 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef  USE_PCH
#   include "Mcommon.h"
#   include "MApplication.h"
#   include "Profile.h"
#   include "Mdefaults.h"
#endif // USE_PCH

#include "FolderView.h"

#include "gui/wxMainFrame.h"

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_OPENFOLDERS;
extern const MOption MP_REOPENLASTFOLDER;

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// FolderView creation
// ----------------------------------------------------------------------------

FolderView::FolderView()
{
    if ( !MEventManager::RegisterAll
          (
            this,
            MEventId_FolderTreeChange,  &m_regCookieTreeChange,
            MEventId_FolderUpdate,      &m_regCookieFolderUpdate,
            MEventId_FolderExpunge,     &m_regCookieFolderExpunge,
            MEventId_FolderClosed,      &m_regCookieFolderClosed,
            MEventId_MsgStatus,         &m_regCookieMsgStatus,
            MEventId_ASFolderResult,    &m_regCookieASFolderResult,
            MEventId_AppExit,           &m_regCookieAppExit,
            MEventId_Null
          ) )
    {
        FAIL_MSG( _T("Failed to register folder view with event manager") );
    }
}

void FolderView::DeregisterEvents(void)
{
    MEventManager::DeregisterAll(&m_regCookieTreeChange,
                                 &m_regCookieFolderUpdate,
                                 &m_regCookieFolderExpunge,
                                 &m_regCookieFolderClosed,
                                 &m_regCookieMsgStatus,
                                 &m_regCookieASFolderResult,
                                 &m_regCookieAppExit,
                                 NULL);
}

FolderView::~FolderView()
{
}

// ----------------------------------------------------------------------------
// event dispatching
// ----------------------------------------------------------------------------

bool FolderView::OnMEvent(MEventData& ev)
{
    if ( ev.GetId() == MEventId_MsgStatus )
        OnMsgStatusEvent((MEventMsgStatusData&)ev);
    else if ( ev.GetId() == MEventId_FolderExpunge )
        OnFolderExpungeEvent((MEventFolderExpungeData&)ev);
    else if ( ev.GetId() == MEventId_FolderUpdate )
        OnFolderUpdateEvent((MEventFolderUpdateData&)ev);
    else if ( ev.GetId() == MEventId_ASFolderResult )
        OnASFolderResultEvent((MEventASFolderResultData &)ev);
    else if ( ev.GetId() == MEventId_FolderClosed )
        OnFolderClosedEvent((MEventFolderClosedData &)ev);
    else if ( ev.GetId() == MEventId_FolderTreeChange )
    {
        MEventFolderTreeChangeData& event = (MEventFolderTreeChangeData &)ev;
        if ( event.GetChangeKind() == MEventFolderTreeChangeData::Delete )
            OnFolderDeleteEvent(event.GetFolderFullName());
    }
    else if ( ev.GetId() == MEventId_AppExit )
        OnAppExit();

    return true; // continue evaluating this event
}

void FolderView::OnAppExit()
{
   // do we want to reopen this folder the next time automatically?
   if ( READ_APPCONFIG(MP_REOPENLASTFOLDER) )
   {
      String foldersToReopen = READ_APPCONFIG(MP_OPENFOLDERS);
      if ( !foldersToReopen.empty() )
         foldersToReopen += ';';
      foldersToReopen += m_folderName;

      mApplication->GetProfile()->writeEntry(MP_OPENFOLDERS, foldersToReopen);
   }
}

// ----------------------------------------------------------------------------
// FolderView misc
// ----------------------------------------------------------------------------

Profile *FolderView::GetFolderProfile() const
{
   Profile *profile = GetProfile();
   if ( !profile )
   {
      profile = mApplication->GetProfile();

      CHECK( profile, NULL, _T("no global profile?") );
   }

   profile->IncRef();

   return profile;
}

