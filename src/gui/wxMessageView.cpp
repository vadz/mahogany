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
#include "ASMailFolder.h"
#include "MFolder.h"
#include "MDialogs.h"

#include "MessageView.h"
#include "MessageViewer.h"
#include "MsgCmdProc.h"

#include "MessageTemplate.h"
#include "Composer.h"

#include "gui/wxIconManager.h"
#include "gui/wxMessageView.h"
#include "gui/wxllist.h"
#include "gui/wxlwindow.h"
#include "gui/wxlparser.h"
#include "gui/wxOptionsDlg.h"
#include "gui/wxDialogLayout.h"

#include <wx/dynarray.h>
#include <wx/file.h>
#include <wx/mimetype.h>
#include <wx/fontmap.h>
#include <wx/clipbrd.h>
#include <wx/splitter.h>
#include <wx/imaglist.h>
#include <wx/treectrl.h>

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_MVIEW_TITLE_FMT;

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
   MimePopup(wxMessageView *parent, const MimePart *mimepart)
   {
      // init member vars
      m_mimepart = mimepart;
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

   const MimePart *m_mimepart;

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
// dialog classes
// ----------------------------------------------------------------------------

class wxMIMETreeCtrl : public wxTreeCtrl
{
public:
   wxMIMETreeCtrl(wxWindow *parent) : wxTreeCtrl(parent, -1)
   {
      InitImageLists();
   }

private:
   void InitImageLists();
};

class wxMIMETreeData : public wxTreeItemData
{
public:
   wxMIMETreeData(const MimePart *mimepart) { m_mimepart = mimepart; }

   const MimePart *GetMimePart() const { return m_mimepart; }

private:
   const MimePart *m_mimepart;
};

class wxMIMETreeDialog : public wxManuallyLaidOutDialog
{
public:
   wxMIMETreeDialog(wxMessageView *msgView, const MimePart *partRoot);

protected:
   // event handlers
   void OnTreeItemRightClick(wxTreeEvent& event)
   {
      wxMIMETreeData *data =
         (wxMIMETreeData *)m_treectrl->GetItemData(event.GetItem());

      m_msgView->PopupMIMEMenu(m_treectrl, data->GetMimePart(), event.GetPoint());
   }

private:
   // fill the tree
   void AddToTree(wxTreeItemId id, const MimePart *mimepart);

   // total number of parts
   size_t m_countParts;

   // GUI controls
   wxStaticBox *m_box;
   wxTreeCtrl *m_treectrl;

   // the parent message view
   wxMessageView *m_msgView;

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

BEGIN_EVENT_TABLE(wxMIMETreeDialog, wxManuallyLaidOutDialog)
   EVT_TREE_ITEM_RIGHT_CLICK(-1, wxMIMETreeDialog::OnTreeItemRightClick)
END_EVENT_TABLE()

// ============================================================================
// implementation of misc private classes
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
// wxMIMETreeCtrl
// ----------------------------------------------------------------------------

void wxMIMETreeCtrl::InitImageLists()
{
   wxIconManager *iconManager = mApplication->GetIconManager();

   wxImageList *imaglist = new wxImageList(32, 32);

   for ( int i = MimeType::TEXT; i <= MimeType::OTHER; i++ )
   {
      // this is a big ugly: we need to get just the 
      MimeType mt((MimeType::Primary)i, "");
      wxIcon icon = iconManager->GetIconFromMimeType(mt.GetType());
      imaglist->Add(icon);
   }

   AssignImageList(imaglist);
}

// ----------------------------------------------------------------------------
// wxMIMETreeDialog
// ----------------------------------------------------------------------------

wxMIMETreeDialog::wxMIMETreeDialog(wxMessageView *msgView,
                                   const MimePart *partRoot)
                : wxManuallyLaidOutDialog(msgView->GetParentFrame(),
                                          _("MIME structure of the message"),
                                          "MimeTreeDialog")
{
   // init members
   m_msgView = msgView;
   m_countParts = 0;
   m_box = NULL;
   m_treectrl = NULL;

   // create controls
   wxLayoutConstraints *c;
   m_box = CreateStdButtonsAndBox(""); // label will be set later

   m_treectrl = new wxMIMETreeCtrl(this);
   c = new wxLayoutConstraints;
   c->top.SameAs(m_box, wxTop, 4*LAYOUT_Y_MARGIN);
   c->left.SameAs(m_box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(m_box, wxRight, 2*LAYOUT_X_MARGIN);
   c->bottom.SameAs(m_box, wxBottom, 2*LAYOUT_Y_MARGIN);
   m_treectrl->SetConstraints(c);

   // initialize them
   AddToTree(wxTreeItemId(), partRoot);

   m_treectrl->Expand(m_treectrl->GetRootItem());

   m_box->SetLabel(wxString::Format(_("%u MIME parts"), m_countParts));

   SetDefaultSize(4*wBtn, 10*hBtn);
}

void
wxMIMETreeDialog::AddToTree(wxTreeItemId idParent, const MimePart *mimepart)
{
   int image = mimepart->GetType().GetPrimary();
   wxTreeItemId id  = m_treectrl->AppendItem(idParent,
                                             MessageView::GetLabelFor(mimepart),
                                             image, image,
                                             new wxMIMETreeData(mimepart));

   m_countParts++;

   const MimePart *partChild = mimepart->GetNested();
   while ( partChild )
   {
      AddToTree(id, partChild);

      partChild = partChild->GetNext();
   }
}

// ============================================================================
// wxMessageView implementation
// ============================================================================

// ----------------------------------------------------------------------------
// ctor/dtor
// ----------------------------------------------------------------------------

wxMessageView::wxMessageView(wxWindow *parent)
             : MessageView(parent)
{
}

wxMessageView::~wxMessageView()
{
}

/* static */
MessageView *MessageView::Create(wxWindow *parent)
{
   return new wxMessageView(parent);
}

// ----------------------------------------------------------------------------
// popup menus
// ----------------------------------------------------------------------------

void wxMessageView::PopupURLMenu(wxWindow *window,
                                 const String& url,
                                 const wxPoint& pt)
{
   UrlPopup menu(this, url);

   window->PopupMenu(&menu, pt);
}

void wxMessageView::PopupMIMEMenu(wxWindow *window,
                                  const MimePart *part,
                                  const wxPoint& pt)
{
   MimePopup mimePopup(this, part);

   window->PopupMenu(&mimePopup, pt);
}

// ----------------------------------------------------------------------------
// MIME structure dialog
// ----------------------------------------------------------------------------

void wxMessageView::ShowMIMEDialog(const MimePart *part)
{
   wxMIMETreeDialog dialog(this, part);
   dialog.ShowModal();
}

// ----------------------------------------------------------------------------
// event handling
// ----------------------------------------------------------------------------

void
wxMessageView::DoShowMessage(Message *mailMessage)
{
   MessageView::DoShowMessage(mailMessage);

   wxFrame *frame = GetParentFrame();
   if ( frame != wxTheApp->GetTopWindow() )
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

wxMessageViewFrame::wxMessageViewFrame(wxWindow *parent,
                                       ASMailFolder *asmf,
                                       UIdType uid)
                  : wxMFrame(_("Mahogany: Message View"), parent)
{
   m_MessageView = MessageView::Create(this);

   m_msgCmdProc = MsgCmdProc::Create(m_MessageView);
   m_msgCmdProc->SetFolder(asmf);
   m_msgCmdProc->SetFrame(this);

   AddFileMenu();
   AddEditMenu();

   // don't use AddMessageMenu() as some commands in this menu don't make
   // sense for standalone message preview
   static const int menuCommands[] =
   {
      WXMENU_MSG_PRINT,
      WXMENU_MSG_PRINT_PREVIEW,
#ifdef USE_PS_PRINTING
      // extra postscript printing
      WXMENU_MSG_PRINT_PS,
      WXMENU_MSG_PRINT_PREVIEW_PS,
#endif
      WXMENU_MSG_SEP1,
      WXMENU_MSG_SEND_SUBMENU_BEGIN,
         WXMENU_MSG_REPLY,
         WXMENU_MSG_REPLY_WITH_TEMPLATE,
         WXMENU_MSG_FOLLOWUP,
         WXMENU_MSG_FOLLOWUP_WITH_TEMPLATE,
         WXMENU_MSG_FORWARD,
         WXMENU_MSG_FORWARD_WITH_TEMPLATE,
      WXMENU_MSG_SEND_SUBMENU_END,
      WXMENU_MSG_SEP2,
      WXMENU_MSG_SAVE_TO_FILE,
      WXMENU_MSG_SAVE_TO_FOLDER,
      WXMENU_MSG_MOVE_TO_FOLDER,
      WXMENU_MSG_DELETE,
      WXMENU_MSG_UNDELETE,
      WXMENU_MSG_SEP3,
      WXMENU_MSG_ADVANCED_SUBMENU_BEGIN,
         WXMENU_MSG_SAVEADDRESSES,
         WXMENU_MSG_TOGGLEHEADERS,
         WXMENU_MSG_SHOWRAWTEXT,
#ifdef EXPERIMENTAL_show_uid
         WXMENU_MSG_SHOWUID,
#endif // EXPERIMENTAL_show_uid
         WXMENU_MSG_SHOWMIME,
      WXMENU_MSG_ADVANCED_SUBMENU_END
   };

   wxMenu *menu = new wxMenu("", wxMENU_TEAROFF);
   for ( size_t n = 0; n < WXSIZEOF(menuCommands); n++ )
   {
      int cmd = menuCommands[n];
      AppendToMenu(menu, cmd);

      // AppendToMenu() might have advanced cmd to the end of the submenu, so
      // advance n accordingly
      while ( n < WXSIZEOF(menuCommands) && menuCommands[n] < cmd )
      {
         n++;
      }
   }

   GetMenuBar()->Append(menu, _("Me&ssage"));

   AddLanguageMenu();

   // add a toolbar to the frame
   AddToolbarButtons(CreateToolBar(), WXFRAME_MESSAGE);

   // a status bar
   CreateStatusBar(2);
   static const int s_widths[] = { -1, 70 };
   SetStatusWidths(WXSIZEOF(s_widths), s_widths);

   // do it after creating the menu as it access the "Toggle headers" item in
   // it
   m_MessageView->SetFolder(asmf);
   m_MessageView->ShowMessage(uid);

   Show(true);
}

void
wxMessageViewFrame::OnMenuCommand(int id)
{
   UIdArray messages;
   messages.Add(m_MessageView->GetUId());

   if ( !m_msgCmdProc->ProcessCommand(id, messages) )
   {
      wxMFrame::OnMenuCommand(id);
   }
}

wxMessageViewFrame::~wxMessageViewFrame()
{
   delete m_msgCmdProc;
   delete m_MessageView;
}

extern MessageView *ShowMessageViewFrame(wxWindow *parent,
                                         ASMailFolder *asmf,
                                         UIdType uid)
{
   wxMessageViewFrame *frame = new wxMessageViewFrame(parent, asmf, uid);

   return frame->GetMessageView();
}

