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
#endif // USE_PCH

#include "Mdefaults.h"

#include "FolderView.h"

#include "gui/wxMainFrame.h"

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
            MEventId_MsgStatus,         &m_regCookieMsgStatus,
            MEventId_ASFolderResult,    &m_regCookieASFolderResult,
            MEventId_AppExit,           &m_regCookieAppExit,
            MEventId_Null
          ) )
    {
        FAIL_MSG( "Failed to register folder view with event manager" );
    }
}

void FolderView::DeregisterEvents(void)
{
    MEventManager::DeregisterAll(&m_regCookieTreeChange,
                                 &m_regCookieFolderUpdate,
                                 &m_regCookieFolderExpunge,
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
        OnMsgStatusEvent((MEventMsgStatusData&)ev );
    else if ( ev.GetId() == MEventId_FolderExpunge )
        OnFolderExpungeEvent((MEventFolderExpungeData&)ev );
    else if ( ev.GetId() == MEventId_FolderUpdate )
        OnFolderUpdateEvent((MEventFolderUpdateData&)ev );
    else if ( ev.GetId() == MEventId_ASFolderResult )
        OnASFolderResultEvent((MEventASFolderResultData &) ev );
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
      // don't remember the folder opened in the main frame - the main frame
      // does it itself
      if ( m_folderName != ((wxMainFrame *)mApplication->TopLevelFrame())
                             ->GetFolderName() )
      {
         String foldersToReopen = READ_APPCONFIG(MP_OPENFOLDERS);
         if ( !foldersToReopen.empty() )
            foldersToReopen += ';';
         foldersToReopen += m_folderName;

         mApplication->GetProfile()->writeEntry(MP_OPENFOLDERS, foldersToReopen);
      }
   }
}

// ----------------------------------------------------------------------------
// misc
// ----------------------------------------------------------------------------

/* static */
String FolderView::SizeToString(unsigned long sizeBytes,
                                unsigned long sizeLines,
                                MessageSizeShow show)
{
   String s;

   switch ( show )
   {
      case MessageSize_Max: // to silence gcc warning
         FAIL_MSG( "unexpected message size format" );
         // fall through

      case MessageSize_Automatic:
         if ( sizeLines > 0 )
         {
            s.Printf(_("%lu lines"),  sizeLines);
            break;
         }
         // fall through

      case MessageSize_AutoBytes:
         if ( sizeBytes == 0 )
         {
            s = _("empty");
         }
         else if ( sizeBytes < 10*1024 )
         {
            s = SizeToString(sizeBytes, 0, MessageSize_Bytes);
         }
         else if ( sizeBytes < 10*1024*1024 )
         {
            s = SizeToString(sizeBytes, 0, MessageSize_KBytes);
         }
         else // Mb
         {
            s = SizeToString(sizeBytes, 0, MessageSize_MBytes);
         }
         break;

      case MessageSize_Bytes:
         s.Printf("%lu", sizeBytes);
         break;

      case MessageSize_KBytes:
         s.Printf(_("%luKb"), sizeBytes / 1024);
         break;

      case MessageSize_MBytes:
         s.Printf(_("%luMb"), sizeBytes / (1024*1024));
         break;
   }

   return s;
}

