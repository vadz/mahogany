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

wxMLog::wxMLog()
{
   m_activeInfoBar = NULL;
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

   int flags = 0;
   switch ( level )
   {
      case wxLOG_FatalError:
      case wxLOG_Error:
         flags |= wxICON_ERROR;
         break;

      case wxLOG_Warning:
         flags |= wxICON_WARNING;
         break;

      case wxLOG_Message:
      case wxLOG_Info:
         flags |= wxICON_INFORMATION;
         break;

      case wxLOG_Status:
      case wxLOG_Debug:
      case wxLOG_Trace:
      case wxLOG_Progress:
      case wxLOG_User:
      case wxLOG_Max:
      default:
         // These messages are not shown in the infobar at all.
         return;
   }

   m_activeInfoBar->ShowMessage(msg, flags);
}
