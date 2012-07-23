///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   gui/ClickAtt.cpp implementations of ClickableAttachment
// Purpose:     ClickableAttachment is a ClickableInfo which corresponds to an
//              attachment (i.e. MIME part) of the message
// Author:      Vadim Zeitlin
// Modified by:
// Created:     05.12.02 (extracted from MessageView.cpp and wxMessageView.cpp)
// CVS-ID:      $Id$
// Copyright:   (c) 2001-2002 Mahogany Team
// Licence:     Mahogany license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"
   #include "strutil.h"

   #include <wx/menu.h>
#endif //USE_PCH

#include "MimePart.h"
#include "MessageView.h"
#include "gui/wxMenuDefs.h"

#include "ClickAtt.h"

// ----------------------------------------------------------------------------
// MimePopup: the popup menu invoked by clicking on an attachment
// ----------------------------------------------------------------------------

class MimePopup : public wxMenu
{
public:
   MimePopup(MessageView *parent, const MimePart *mimepart);

   // callbacks
   void OnCommandEvent(wxCommandEvent &event);

private:
   MessageView * const m_MessageView;

   const MimePart * const m_mimepart;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(MimePopup)
};

// ============================================================================
// MimePopup implementation
// ============================================================================

BEGIN_EVENT_TABLE(MimePopup, wxMenu)
   EVT_MENU(-1, MimePopup::OnCommandEvent)
END_EVENT_TABLE()

MimePopup::MimePopup(MessageView *parent, const MimePart *mimepart)
         : m_MessageView(parent),
           m_mimepart(mimepart)
{
   // create the menu items
   Append(WXMENU_MIME_INFO, _("&Info"));
   AppendSeparator();
   Append(WXMENU_MIME_OPEN, _("&Open"));
   Append(WXMENU_MIME_OPEN_WITH, _("Open &with..."));
   Append(WXMENU_MIME_SAVE, _("&Save..."));
   Append(WXMENU_MIME_VIEW_AS_TEXT, _("&View as text"));
}

void
MimePopup::OnCommandEvent(wxCommandEvent &event)
{
   switch ( event.GetId() )
   {
      case WXMENU_MIME_INFO:
         m_MessageView->MimeInfo(m_mimepart);
         break;

      case WXMENU_MIME_OPEN:
         m_MessageView->MimeHandle(m_mimepart);
         break;

      case WXMENU_MIME_OPEN_WITH:
         m_MessageView->MimeOpenWith(m_mimepart);
         break;

      case WXMENU_MIME_VIEW_AS_TEXT:
         m_MessageView->MimeViewText(m_mimepart);
         break;

      case WXMENU_MIME_SAVE:
         m_MessageView->MimeSave(m_mimepart);
         break;
   }
}

// ============================================================================
// ClickableAttachment implementation
// ============================================================================

// ----------------------------------------------------------------------------
// ClickableAttachment ctor
// ----------------------------------------------------------------------------

ClickableAttachment::ClickableAttachment(MessageView *msgView,
                                         const MimePart *mimepart)
                   : ClickableInfo(msgView),
                     m_mimepart(mimepart)
{
   ASSERT_MSG( mimepart, _T("NULL mimepart in ClickableAttachment ctor") );
}

// ----------------------------------------------------------------------------
// ClickableAttachment accessors
// ----------------------------------------------------------------------------

/* static */
String ClickableAttachment::GetLabelFor(const MimePart *mimepart)
{
   wxString label = mimepart->GetFilename();
   if ( !label.empty() )
      label << _T(" : ");

   MimeType type = mimepart->GetType();
   label << type.GetFull();

   // multipart always have size of 0, don't show
   if ( type.GetPrimary() != MimeType::MULTIPART )
   {
      label << _T(", ");

      size_t lines;
      if ( type.IsText() && (lines = mimepart->GetNumberOfLines()) != 0 )
      {
         label << strutil_ultoa(lines) << _(" lines");
      }
      else
      {
         label << strutil_ultoa(mimepart->GetSize()) << _(" bytes");
      }
   }

#ifdef DEBUG
   // show the part spec in debug build
   label << _T(" (") << mimepart->GetPartSpec() << _T(')');
#endif // DEBUG

   return label;
}

String ClickableAttachment::GetLabel() const
{
   return GetLabelFor(m_mimepart);
}

// ----------------------------------------------------------------------------
// ClickableAttachment click handlers
// ----------------------------------------------------------------------------

void ClickableAttachment::OnLeftClick() const
{
   // open the attachment: this is dangerous and should be made configurable
   // and probably disabled by default, see #670
   m_msgView->MimeHandle(m_mimepart);
}

void ClickableAttachment::OnRightClick(const wxPoint& pt) const
{
   ShowPopupMenu(m_msgView->GetWindow(), pt);
}

void
ClickableAttachment::ShowPopupMenu(wxWindow *window, const wxPoint& pt) const
{
   MimePopup menu(m_msgView, m_mimepart);

   window->PopupMenu(&menu, pt);
}

