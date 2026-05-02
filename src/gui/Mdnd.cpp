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

#include "Mpch.h"

#if wxUSE_DRAG_AND_DROP

#ifndef USE_PCH
#  include "Mcommon.h"

#  include "guidef.h"
#endif // USE_PCH

#include "MFolder.h"

#include "MsgCmdProc.h"
#include "gui/wxMenuDefs.h"

#include "UIdArray.h"

#include "Mdnd.h"

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// MMessagesDataObject
// ----------------------------------------------------------------------------

MMessagesDataObject::MMessagesDataObject(MsgCmdProc *msgProc,
                                         MailFolder *folder,
                                         const UIdArray& messages)
                   : wxCustomDataObject(MMESSAGE_FORMAT)
{
   // we store the MsgCmdProc pointer first followed by the number of
   // messages - and then all messages after it
   size_t len = sizeof(MMessagesDataObject::Data) +
                  messages.GetCount()*sizeof(UIdType);
   void *buf = new char[len];
   Data *data = (Data *)buf;
   data->msgProc = msgProc;
   data->number = messages.GetCount();
   data->folder = folder;
   
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

MMessagesDataObject::~MMessagesDataObject()
{
}

// ----------------------------------------------------------------------------
// MMessagesDropTargetBase
// ----------------------------------------------------------------------------

MMessagesDropTargetBase::MMessagesDropTargetBase(wxWindow *win)
                       : wxDropTarget(new MMessagesDataObject)
{
   m_frame = ::GetFrame(win);
   win->SetDropTarget(this);
}

wxDragResult
MMessagesDropTargetBase::OnEnter(wxCoord x, wxCoord y, wxDragResult def)
{
   if ( GetFrame() )
   {
      wxLogStatus(GetFrame(), _("You can drop mail messages here."));
   }

   return OnDragOver(x, y, def);
}

void MMessagesDropTargetBase::OnLeave()
{
   if ( GetFrame() )
   {
      GetFrame()->SetStatusText(wxEmptyString);
   }
}

wxDragResult
MMessagesDropTargetBase::OnData(wxCoord x, wxCoord y, wxDragResult def)
{
   if ( !GetData() )
   {
      wxLogDebug(_T("Failed to get drag and drop data"));

      return wxDragNone;
   }

   return OnMsgDrop(x, y, (MMessagesDataObject *)GetDataObject(), def);
}

// ----------------------------------------------------------------------------
// MMessagesDropTarget
// ----------------------------------------------------------------------------

MMessagesDropTarget::MMessagesDropTarget(MMessagesDropWhere *where,
                                         wxWindow *win)
                   : MMessagesDropTargetBase(win)
{
   m_where = where;
}

wxDragResult MMessagesDropTarget::OnMsgDrop(wxCoord x, wxCoord y,
                                            MMessagesDataObject *data,
                                            wxDragResult def)
{
   MFolder_obj folder(m_where->GetFolder(x, y));
   if ( !folder )
   {
      wxLogStatus(GetFrame(),
                  _("No folder under cursor, messages not dropped."));

      return wxDragNone;
   }

   if ( !CanCreateMessagesInFolder(folder->GetType()) )
   {
      wxLogStatus(GetFrame(), _("Can't drop messages to this folder."));

      return wxDragNone;
   }

   UIdArray messages = data->GetMessages();

   MsgCmdProc *msgCmdProc = data->GetMsgCmdProc();

   // TODO: check here if the folder can be written to?

   // check that we are not copying to the same folder
   if ( msgCmdProc->GetFolderName() == folder->GetFullName() )
   {
      wxLogStatus(GetFrame(), _("Can't drop messages to the same folder."));

      return wxDragNone;
   }

   msgCmdProc->ProcessCommand(WXMENU_MSG_DROP_TO_FOLDER, messages, folder);

   // it's ok even if m_frame is NULL
   wxLogStatus(GetFrame(), _("%zu message(s) dropped."), messages.GetCount());
   m_where->Refresh();

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

#endif // wxUSE_DRAG_AND_DROP

