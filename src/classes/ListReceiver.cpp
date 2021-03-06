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

#include  "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"
#endif // USE_PCH

#include "ASMailFolder.h"

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

bool
ListEventReceiver::List(ASMailFolder *asmf,
                        const String& pattern,
                        const String& root)
{
   CHECK( asmf, false, _T("NULL ASMailFolder in ListEventReceiver::ListAll") );

   return asmf->ListFolders(pattern, false, root, this) != ILLEGAL_TICKET;
}

bool ListEventReceiver::OnMEvent(MEventData& event)
{
   // we're only subscribed to the ASFolder events
   CHECK( event.GetId() == MEventId_ASFolderResult, false,
          _T("unexpected event type") );

   MEventASFolderResultData &data = (MEventASFolderResultData &)event;

   ASFolderExistsResult_obj result((ASFolderExistsResult *)data.GetResult());

   // is this message really for us?
   if ( result->GetUserData() != this )
   {
      // no: continue with other event handlers
      return true;
   }

   if ( result->GetOperation() != ASMailFolder::Op_ListFolders )
   {
      FAIL_MSG( _T("unexpected operation notification") );

      // eat the event - it was for us but we didn't process it...
      return false;
   }

   // is it the special event which signals that there will be no more of
   // folders?
   if ( result->NoMore() )
   {
      // end of enumeration
      OnNoMoreFolders();
   }
   else // notification about a folder
   {
      const wxChar delim = result->GetDelimiter();
      String name = result->GetName();

      // we don't want the leading slash, if any
      if ( *name.c_str() == delim )
         name.erase(0, 1);

      OnListFolder(name, delim, result->GetAttributes());
   }

   // we don't want anyone else to receive this message - it was for us only
   return false;
}
