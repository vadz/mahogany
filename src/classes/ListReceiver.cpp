//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   classes/ListReceiver.cpp: implements ListReceiver
// Purpose:     makes using ASMailFolder::ListFolders() less painful
// Author:      Vadim Zeitlin
// Modified by:
// Created:     08.10.02
// CVS-ID:      $Id$
// Copyright:   (C) 2002 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#ifdef __GNUG__
   #pragma implementation "MFPool.h"
#endif

#include  "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"
#endif

#include "ListReceiver.h"

// needed to be able to use DECLARE_AUTOREF() macro
typedef ASMailFolder::ResultFolderExists ASFolderExistsResult;
DECLARE_AUTOPTR(ASFolderExistsResult);

// ============================================================================
// implementation
// ============================================================================

ListEventReceiver::ListEventReceiver()
{
   m_regCookie = MEventManager::Register(*this, MEventId_ASFolderResult);
   ASSERT_MSG( m_regCookie, _T("can't register with event manager"));
}

ListEventReceiver::~ListEventReceiver()
{
   MEventManager::Deregister(m_regCookie);
}

bool ListEventReceiver::OnMEvent(MEventData& event)
{
   // we're only subscribed to the ASFolder events
   CHECK( event.GetId() == MEventId_ASFolderResult, FALSE,
          _T("unexpected event type") );

   MEventASFolderResultData &data = (MEventASFolderResultData &)event;

   ASFolderExistsResult_obj result((ASFolderExistsResult *)data.GetResult());

   // is this message really for us?
   if ( result->GetUserData() != this )
   {
      // no: continue with other event handlers
      return TRUE;
   }

   if ( result->GetOperation() != ASMailFolder::Op_ListFolders )
   {
      FAIL_MSG( _T("unexpected operation notification") );

      // eat the event - it was for us but we didn't process it...
      return FALSE;
   }

   // is it the special event which signals that there will be no more of
   // folders?
   wxString path = result->GetName();
   if ( path.empty() )
      OnNoMoreFolders();
   else
      OnListFolder(path, result->GetDelimiter(), result->GetAttributes());

   // we don't want anyone else to receive this message - it was for us only
   return FALSE;
}
