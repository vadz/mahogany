///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   gui/MDnd.cpp - dnd support
// Purpose:     implements MMessagesDataObject and MMessagesDropTarget classes
// Author:      Vadim Zeitlin
// Modified by:
// Created:     26.04.00
// CVS-ID:      $Id$
// Copyright:   (c) 2000 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#ifdef __GNUG__
   #pragma implementation "Mdnd.h"
#endif

#include "Mpch.h"

#ifndef USE_PCH
#  include "Mcommon.h"

#  include "MailFolder.h"

#  include "guidef.h"
#endif // USE_PCH

#include "MFolder.h"

#include "gui/wxFolderView.h"

#include "Mdnd.h"

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// MMessagesDataObject
// ----------------------------------------------------------------------------

MMessagesDataObject::MMessagesDataObject(wxFolderView *view,
                                         const UIdArray& messages)
                   : wxCustomDataObject(MMESSAGE_FORMAT)
{
   // we store the wxFolderView pointer first followed bythe number of
   // messages - and then all messages after it
   size_t len = sizeof(MMessagesDataObject::Data) +
                  messages.GetCount()*sizeof(UIdType);
   void *buf = new char[len];
   Data *data = (Data *)buf;
   data->view = view;
   data->number = messages.GetCount();

   UIdType *p = GetUIDs(data);
   for ( size_t n = 0; n < data->number; n++ )
   {
      p[n] = messages[n];
   }

   TakeData(len, data);
}

UIdArray MMessagesDataObject::GetMessages() const
{
   UIdArray messages;
   size_t count = GetMessageCount();
   messages.Alloc(count);

   for ( size_t n = 0; n < count; n++ )
   {
      messages.Add(GetMessageUId(n));
   }

   return messages;
}

// ----------------------------------------------------------------------------
// MMessagesDropTarget
// ----------------------------------------------------------------------------

MMessagesDropTarget::MMessagesDropTarget(MMessagesDropWhere *where, wxWindow *win)
                   : wxDropTarget(new MMessagesDataObject)
{
   m_where = where;
   m_frame = GetFrame(win);
   win->SetDropTarget(this);
}

wxDragResult MMessagesDropTarget::OnEnter(wxCoord x, wxCoord y, wxDragResult def)
{
   if ( m_frame )
   {
      wxLogStatus(m_frame, _("You can drop mail messages here."));
   }

   return OnDragOver(x, y, def);
}

void MMessagesDropTarget::OnLeave()
{
   if ( m_frame )
   {
      wxLogStatus(m_frame, "");
   }
}

wxDragResult MMessagesDropTarget::OnData(wxCoord x, wxCoord y, wxDragResult def)
{
   if ( !GetData() )
   {
      wxLogDebug("Failed to get drag and drop data");

      return wxDragNone;
   }

   MFolder *folder = m_where->GetFolder(x, y);
   if ( folder )
   {
      MMessagesDataObject *data = (MMessagesDataObject *)GetDataObject();
      UIdArray messages = data->GetMessages();

      // TODO: check here if the folder can be written to?

      data->GetFolderView()->DropMessagesToFolder(messages, folder);

      // it's ok even if m_frame is NULL
      wxLogStatus(m_frame, _("%u messages dropped."), messages.GetCount());
      m_where->Refresh();

      folder->DecRef();
   }

   return def;
}

MMessagesDropTarget::~MMessagesDropTarget()
{
   delete m_where;
}

// ----------------------------------------------------------------------------
// MMessagesDropWhere
// ----------------------------------------------------------------------------

void MMessagesDropWhere::Refresh()
{
}

MMessagesDropWhere::~MMessagesDropWhere()
{
}
