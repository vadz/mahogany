///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   gui/wxMLog.cpp
// Purpose:     wxMLog implementation
// Author:      Vadim Zeitlin
// Created:     2017-09-30
// Copyright:   (c) 2017 Mahogany team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
#  include "Mcommon.h"
#endif // !USE_PCH

#include "gui/wxMLog.h"

#include <wx/infobar.h>

// ============================================================================
// wxMLog implementation
// ============================================================================

wxMLog* wxMLog::ms_MLog = NULL;

void
wxMLog::Activate()
{
   ASSERT( !ms_MLog );
   ms_MLog = new wxMLog;

   delete wxLog::SetActiveTarget(ms_MLog);
}

void
wxMLog::SetInfoBarToUse(wxInfoBarBase* infobar)
{
   // Last message was shown in a different infobar, so forget about it.
   m_lastMsg.clear();
   m_lastShownLevel = wxLOG_Max;

   m_activeInfoBar = infobar;
}

static
int GetFlagsForLogLevel(wxLogLevel level)
{
   switch ( level )
   {
      case wxLOG_FatalError:
      case wxLOG_Error:
         return wxICON_ERROR;

      case wxLOG_Warning:
         return wxICON_WARNING;

      case wxLOG_Message:
      case wxLOG_Info:
         return wxICON_INFORMATION;

      case wxLOG_Status:
      case wxLOG_Debug:
      case wxLOG_Trace:
      case wxLOG_Progress:
      case wxLOG_User:
      case wxLOG_Max:
      default:
         return 0;
   }
}

void
wxMLog::DoLogRecord(wxLogLevel level,
                    const wxString& msg,
                    const wxLogRecordInfo& info)
{
   if ( !m_activeInfoBar )
   {
      wxLogGui::DoLogRecord(level, msg, info);
      return;
   }

   int flags = GetFlagsForLogLevel(level);
   if ( !flags )
   {
      // These messages are too low priority to be shown in the infobar.
      return;
   }

   wxString msgToShow;

   if ( m_lastMsg.empty() )
   {
      m_lastMsg = msg;
      msgToShow = msg;
   }
   else // We already have another message shown
   {
      // Check if we need to override it with the more recent one: normally we
      // do it, but avoid overriding an error with an informational message.
      if ( level <= m_lastShownLevel )
      {
         m_lastMsg = msg;
         m_lastShownLevel = level;
      }
      else // The new message has strictly lower severity
      {
         // Don't show it and don't update the last values, but do use the same
         // flags as the last time instead of the new ones.
         flags = GetFlagsForLogLevel(m_lastShownLevel);
      }

      msgToShow = m_lastMsg;
      msgToShow += " (and other messages, see log)";
   }

   m_activeInfoBar->ShowMessage(msgToShow, flags);
}
