///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   gui/wxMessageView.cpp - message viewer window
// Purpose:     implementation of msg viewer using wxWindows
// Author:      Karsten Ballüder (Ballueder@gmx.net)
// Modified by:
// Created:     1998
// CVS-ID:      $Id$
// Copyright:   (c) 1998-2001 M Team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#ifdef __GNUG__
#   pragma implementation "wxMessageView.h"
#endif

#include "Mpch.h"

#ifndef USE_PCH
#  include "Mcommon.h"
#  include "strutil.h"
#  include "guidef.h"

#  include "PathFinder.h"
#  include "Profile.h"

#  include "MFrame.h"
#  include "MLogFrame.h"

#  include "MApplication.h"
#  include "gui/wxMApp.h"
#  include "gui/wxIconManager.h"
#endif //USE_PCH

#include "Mdefaults.h"
#include "MHelp.h"
#include "Message.h"
#include "FolderView.h"
#include "ASMailFolder.h"
#include "MFolder.h"
#include "MDialogs.h"

#include "MessageView.h"
#include "MessageViewer.h"

#include "MessageTemplate.h"
#include "Composer.h"

#include "gui/wxIconManager.h"
#include "gui/wxMessageView.h"
#include "gui/wxFolderView.h"
#include "gui/wxllist.h"
#include "gui/wxlwindow.h"
#include "gui/wxlparser.h"
#include "gui/wxOptionsDlg.h"

#include <wx/dynarray.h>
#include <wx/file.h>
#include <wx/mimetype.h>
#include <wx/fontmap.h>
#include <wx/clipbrd.h>
#include <wx/splitter.h>

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// the var expander for message view frame title used by ShowMessage()
//
// TODO: the code should be reused with VarExpander in wxComposeView.cpp!
class MsgVarExpander : public MessageTemplateVarExpander
{
public:
   MsgVarExpander(Message *msg) { m_msg = msg; SafeIncRef(m_msg); }
   virtual ~MsgVarExpander() { SafeDecRef(m_msg); }

   virtual bool Expand(const String& category,
                       const String& Name,
                       const wxArrayString& arguments,
                       String *value) const
   {
      if ( !m_msg )
         return false;

      // we only understand fields in the unnamed/default category
      if ( !category.empty() )
         return false;

      String name = Name.Lower();
      if ( name == "from" )
         *value = m_msg->From();
      else if ( name == "subject" )
         *value = m_msg->Subject();
      else if ( name == "date" )
         *value = m_msg->Date();
      else
         return false;

      *value = MailFolder::DecodeHeader(*value);

      return true;
   }

private:
    Message *m_msg;
};

// ----------------------------------------------------------------------------
// popup menu classes
// ----------------------------------------------------------------------------

// the popup menu invoked by clicking on an attachment in the mail message
class MimePopup : public wxMenu
{
public:
   MimePopup(wxMessageView *parent, int partno)
   {
      // init member vars
      m_PartNo = partno;
      m_MessageView = parent;

      // create the menu items
      Append(WXMENU_MIME_INFO, _("&Info"));
      AppendSeparator();
      Append(WXMENU_MIME_OPEN, _("&Open"));
      Append(WXMENU_MIME_OPEN_WITH, _("Open &with..."));
      Append(WXMENU_MIME_SAVE, _("&Save..."));
      Append(WXMENU_MIME_VIEW_AS_TEXT, _("&View as text"));
   }

   // callbacks
   void OnCommandEvent(wxCommandEvent &event);

private:
   wxMessageView *m_MessageView;
   int m_PartNo;

   DECLARE_EVENT_TABLE()
};

// the popup menu used with URLs
class UrlPopup : public wxMenu
{
public:
   UrlPopup(wxMessageView *parent, const String& url)
      : m_url(url)
   {
      m_MessageView = parent;

      SetTitle(url.BeforeFirst(':').Upper() + _(" url"));
      Append(WXMENU_URL_OPEN, _("&Open"));
      Append(WXMENU_URL_OPEN_NEW, _("Open in &new window"));
      Append(WXMENU_URL_COPY, _("&Copy to clipboard"));
   }

   // callbacks
   void OnCommandEvent(wxCommandEvent &event);

private:
   wxMessageView *m_MessageView;
   String m_url;

   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(MimePopup, wxMenu)
   EVT_MENU(-1, MimePopup::OnCommandEvent)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(UrlPopup, wxMenu)
   EVT_MENU(-1, UrlPopup::OnCommandEvent)
END_EVENT_TABLE()

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// MimePopup
// ----------------------------------------------------------------------------

void
MimePopup::OnCommandEvent(wxCommandEvent &event)
{
   switch ( event.GetId() )
   {
      case WXMENU_MIME_INFO:
         m_MessageView->MimeInfo(m_PartNo);
         break;

      case WXMENU_MIME_OPEN:
         m_MessageView->MimeHandle(m_PartNo);
         break;

      case WXMENU_MIME_OPEN_WITH:
         m_MessageView->MimeOpenWith(m_PartNo);
         break;

      case WXMENU_MIME_VIEW_AS_TEXT:
         m_MessageView->MimeViewText(m_PartNo);
         break;

      case WXMENU_MIME_SAVE:
         m_MessageView->MimeSave(m_PartNo);
         break;
   }
}

// ----------------------------------------------------------------------------
// UrlPopup
// ----------------------------------------------------------------------------

void
UrlPopup::OnCommandEvent(wxCommandEvent &event)
{
   switch ( event.GetId() )
   {
      case WXMENU_URL_OPEN:
         m_MessageView->OpenURL(m_url, FALSE);
         break;

      case WXMENU_URL_OPEN_NEW:
         m_MessageView->OpenURL(m_url, TRUE);
         break;

      case WXMENU_URL_COPY:
         {
            wxClipboardLocker lockClip;
            if ( !lockClip )
            {
               wxLogError(_("Failed to lock clipboard, URL not copied."));
            }
            else
            {
               wxTheClipboard->UsePrimarySelection();
               wxTheClipboard->SetData(new wxTextDataObject(m_url));
            }
         }
         break;

      default:
         FAIL_MSG( "unexpected URL popup menu command" );
         break;
   }
}

// ----------------------------------------------------------------------------
// wxMessageView
// ----------------------------------------------------------------------------

wxMessageView::wxMessageView(wxWindow *parent)
             : MessageView(parent)
{
}

wxMessageView::~wxMessageView()
{
}

// ----------------------------------------------------------------------------
// popup menus
// ----------------------------------------------------------------------------

void wxMessageView::PopupURLMenu(const String& url, const wxPoint& pt)
{
   UrlPopup menu(this, url);

   GetWindow()->PopupMenu(&menu, pt);
}

void wxMessageView::PopupMIMEMenu(size_t nPart, const wxPoint& pt)
{
   MimePopup mimePopup(this, nPart);

   GetWindow()->PopupMenu(&mimePopup, pt);
}

// ----------------------------------------------------------------------------
// event handling
// ----------------------------------------------------------------------------

void
wxMessageView::DoShowMessage(Message *mailMessage)
{
   MessageView::DoShowMessage(mailMessage);

   wxFrame *frame = GetParentFrame();
   if ( wxDynamicCast(frame, wxMessageViewFrame) )
   {
      wxString fmt = READ_CONFIG(GetProfile(), MP_MVIEW_TITLE_FMT);
      MsgVarExpander expander(mailMessage);
      frame->SetTitle(ParseMessageTemplate(fmt, expander));
   }
   //else: we're in the main frame, don't do this
}

// ----------------------------------------------------------------------------
// helpers for viewer changing
// ----------------------------------------------------------------------------

void
wxMessageView::OnViewerChange(const MessageViewer *viewerOld,
                              const MessageViewer *viewerNew)
{
   if ( !viewerOld )
   {
      return;
   }

   wxWindow *winOld = viewerOld->GetWindow();
   wxSplitterWindow *splitter = wxDynamicCast(winOld->GetParent(), wxSplitterWindow);

   if ( splitter )
      splitter->ReplaceWindow(winOld, viewerNew->GetWindow());

   delete winOld;
}

// ----------------------------------------------------------------------------
// wxMessageViewFrame
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxMessageViewFrame, wxMFrame)

wxMessageViewFrame::wxMessageViewFrame(wxWindow *parent)
                  : wxMFrame(_("Mahogany: Message View"), parent)
{
   m_MessageView = new wxMessageView(this);

   AddFileMenu();
   AddEditMenu();
   AddMessageMenu();
   AddLanguageMenu();

   // add a toolbar to the frame
   AddToolbarButtons(CreateToolBar(), WXFRAME_MESSAGE);

   // a status bar
   CreateStatusBar(2);
   static const int s_widths[] = { -1, 70 };
   SetStatusWidths(WXSIZEOF(s_widths), s_widths);

   Show(true);
}

void
wxMessageViewFrame::OnMenuCommand(int id)
{
   if( !m_MessageView->DoMenuCommand(id) )
   {
      wxMFrame::OnMenuCommand(id);
   }
}

extern MessageView *ShowMessageViewFrame(wxWindow *parent)
{
   wxMessageViewFrame *frame = new wxMessageViewFrame(parent);

   return frame->GetMessageView();
}

