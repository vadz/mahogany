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
#  include "Mdefaults.h"
#  include "Profile.h"

#  include <wx/stattext.h>
#  include <wx/app.h>
#  include <wx/menu.h>
#endif //USE_PCH

#include "MessageViewer.h"
#include "MsgCmdProc.h"
#include "FolderView.h"
#include "gui/wxMenuDefs.h"

#include "UIdArray.h"
#include "ASMailFolder.h"

#include "MessageTemplate.h"
#include "Message.h"

#include "gui/wxMessageView.h"

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_MSGVIEW_VIEWER;
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
                       const wxArrayString& /* arguments */,
                       String *value) const
   {
      if ( !m_msg )
         return false;

      // we only understand fields in the unnamed/default category
      if ( !category.empty() )
         return false;

      String name = Name.Lower();
      if ( name == _T("from") )
         *value = m_msg->From();
      else if ( name == _T("subject") )
         *value = m_msg->Subject();
      else if ( name == _T("date") )
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
// DummyViewer: a trivial implementation of MessageViewer which doesn't do
//              anything but which we use to avoid crashing if no viewers are
//              found
//
//              this is also the one we use when we have no folder set
// ----------------------------------------------------------------------------

class DummyViewer : public MessageViewer
{
public:
   // creation
   DummyViewer() { }

   virtual void Create(MessageView * /* msgView */, wxWindow *parent)
   {
      m_window = new wxStaticText(parent, -1, _("\n\nNo message"),
                                  wxDefaultPosition, wxDefaultSize,
                                  wxALIGN_CENTER);
   }

   // operations
   virtual void Clear() { }
   virtual void Update() { }
   virtual void UpdateOptions() { }
   virtual wxWindow *GetWindow() const { return m_window; }

   virtual bool Find(const String& /* text */) { return false; }
   virtual bool FindAgain() { return false; }
   virtual void SelectAll() { }
   virtual String GetSelection() const { return _T(""); }
   virtual void Copy() { }
   virtual bool Print() { return false; }
   virtual void PrintPreview() { }

   // header showing
   virtual void StartHeaders() { }
   virtual void ShowRawHeaders(const String& /* header */) { }
   virtual void ShowHeaderName(const String& /* name */) { }
   virtual void ShowHeaderValue(const String& /* value */,
                                wxFontEncoding /* encoding */) { }
   virtual void ShowHeaderURL(const String& /* text */,
                              const String& /* url */) { }
   virtual void EndHeader() { }
   virtual void ShowXFace(const wxBitmap& /* bitmap */) { }
   virtual void EndHeaders() { }

   // body showing
   virtual void StartBody() { }
   virtual void StartPart() { }
   virtual void InsertAttachment(const wxBitmap& /* icon */,
                                 ClickableInfo * /* ci */) { }
   virtual void InsertClickable(const wxBitmap& /* icon */,
                                ClickableInfo * /* ci */,
                                const wxColour& /* col */) { }
   virtual void InsertImage(const wxImage& /* image */,
                            ClickableInfo * /* ci */) { }
   virtual void InsertRawContents(const String& /* data */) { }
   virtual void InsertText(const String& /* text */,
                           const MTextStyle& /* style */) { }
   virtual void InsertURL(const String& /* text */,
                          const String& /* url */) { }
   virtual void EndText() { }
   virtual void EndPart() { }
   virtual void EndBody() { }

   // scrolling
   virtual bool LineDown() { return false; }
   virtual bool LineUp() { return false; }
   virtual bool PageDown() { return false; }
   virtual bool PageUp() { return false; }

   // capabilities querying
   virtual bool CanInlineImages() const { return false; }
   virtual bool CanProcess(const String& /* mimetype */) const { return false; }

private:
   wxWindow *m_window;
};

// ============================================================================
// wxMessageView implementation
// ============================================================================

// ----------------------------------------------------------------------------
// ctor/dtor
// ----------------------------------------------------------------------------

wxMessageView::wxMessageView(wxWindow *parent,
                             FolderView *folderView,
                             Profile *profile)
{
   Init(parent, profile);

   m_FolderView = folderView;
}

wxMessageView::~wxMessageView()
{
}

/* static */
MessageView *MessageView::Create(wxWindow *parent, FolderView *folderView)
{
   return new wxMessageView(parent, folderView);
}

/* static */
MessageView *
MessageView::CreateStandalone(wxWindow *parent, Profile *profile)
{
   return new wxMessageView(parent, NULL, profile);
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
// message viewer functions
// ----------------------------------------------------------------------------

void
wxMessageView::OnViewerChange(const MessageViewer *viewerOld,
                              const MessageViewer *viewerNew)
{
   if ( m_FolderView )
   {
      m_FolderView->OnMsgViewerChange(viewerNew ? viewerNew->GetWindow()
                                                : NULL);
   }

   if ( viewerOld )
   {
      delete viewerOld->GetWindow();
   }

   // we also need to update the viewers menu to indicate the currently
   // selected one
   if ( viewerNew )
   {
      int n = m_namesViewers.Index(GetProfileValues().msgViewer);
      CHECK_RET( n != wxNOT_FOUND, _T("non existing viewer selected?") );

      wxFrame *frame = GetFrame(viewerNew->GetWindow());
      CHECK_RET( frame, _T("no parent frame in wxMessageView") );

      wxMenuBar *mbar = frame->GetMenuBar();
      CHECK_RET( mbar, _T("no menu bar in the frame containing wxMessageView?") );

      mbar->Check(WXMENU_VIEW_VIEWERS_BEGIN + 1 + n, true);

      // finally we must ensure that the new viewer is positioned/sized
      // correctly: folder view does it itself in OnMsgViewerChange() but for
      // standalone message view frame we have to do it ourselves
      if ( !m_FolderView )
      {
         frame->SendSizeEvent();
      }
   }
}

MessageViewer *
wxMessageView::CreateDefaultViewer() const
{
   return new DummyViewer();
}

// ----------------------------------------------------------------------------
// "View" menu support functions
// ----------------------------------------------------------------------------

void
wxMessageView::CreateViewMenu()
{
   wxFrame *frame = GetParentFrame();
   CHECK_RET( frame, _T("no parent frame in wxMessageView") );

   // create the top level menu
   WXADD_MENU(frame->GetMenuBar(), VIEW, _("&View"));

   // initialize viewers submenu
   // --------------------------

   wxMenu *menuView = FindSubmenu(frame, WXMENU_VIEW_VIEWERS_SUBMENU_BEGIN + 1);
   CHECK_RET( menuView, _T("View|Viewers submenu not found?") );

   Profile * const profile = GetProfile();
   CHECK_RET( profile, _T("no Profile in wxMessageView?") );

   const wxString nameCurViewer = READ_CONFIG(profile, MP_MSGVIEW_VIEWER);
   wxArrayString descViewers;
   size_t countViewers = GetAllAvailableViewers(&m_namesViewers, &descViewers);

   for ( size_t nViewer = 0; nViewer < countViewers; nViewer++ )
   {
      const int id = WXMENU_VIEW_VIEWERS_BEGIN + 1 + nViewer;

      menuView->AppendRadioItem(id, descViewers[nViewer]);

      if ( m_namesViewers[nViewer] == nameCurViewer )
      {
         // indicate the current viewer
         menuView->Check(id, true);
      }
   }


   // initialize view filters submenu
   // -------------------------------

   // find the filters submenu in the top level menu
   wxMenu *menuFlt = FindSubmenu(frame, WXMENU_VIEW_FILTERS_SUBMENU_BEGIN + 1);
   CHECK_RET( menuFlt, _T("View|Filters submenu not found?") );


   // add all the filters to it

   // initialize the filters chain first
   MessageView::CreateViewMenu();

   // and then iterate over all the filters adding them
   int id = WXMENU_VIEW_FILTERS_BEGIN + 1;
   String name,
          desc;
   void *cookie;
   bool enabled;
   bool cont = GetFirstViewFilter(&name, &desc, &enabled, &cookie);
   while ( cont )
   {
      m_namesFilters.Add(name);
      m_statesFilters.Add(enabled);

      menuFlt->AppendCheckItem(id, desc);
      if ( enabled )
         menuFlt->Check(id, true);

      id++;

      cont = GetNextViewFilter(&name, &desc, &enabled, &cookie);
   }

   // should we disable/remove filters menu if there are no filters?
}

void
wxMessageView::OnToggleViewFilter(int id)
{
   const int n = id - WXMENU_VIEW_FILTERS_BEGIN - 1;

   CHECK_RET( n >= 0 && (size_t)n < m_namesFilters.GetCount(),
              _T("invalid view filter toggled?") );

   // this one must not be DecRef()'d
   Profile * const profile = GetProfile();
   CHECK_RET( profile, _T("no Profile in wxMessageView?") );

   m_statesFilters[n] = !m_statesFilters[n];
   profile->writeEntry(m_namesFilters[n], m_statesFilters[n]);

   MEventManager::Send(new MEventOptionsChangeData
                           (
                            profile,
                            MEventOptionsChangeData::Ok
                           ));
}

void
wxMessageView::OnSelectViewer(int id)
{
   const int n = id - WXMENU_VIEW_VIEWERS_BEGIN - 1;

   CHECK_RET( n >= 0 && (size_t)n < m_namesViewers.GetCount(),
              _T("invalid viewer selected from the menu?") );

   // this one must not be DecRef()'d
   Profile * const profile = GetProfile();
   CHECK_RET( profile, _T("no Profile in wxMessageView?") );

   profile->writeEntry(MP_MSGVIEW_VIEWER, m_namesViewers[n]);

   MEventManager::Send(new MEventOptionsChangeData
                           (
                            profile,
                            MEventOptionsChangeData::Ok
                           ));
}

// ----------------------------------------------------------------------------
// other menu stuff
// ----------------------------------------------------------------------------

void
wxMessageView::OnShowHeadersChange()
{
   wxFrame *frame = GetParentFrame();
   CHECK_RET( frame, _T("message view without parent frame?") );

   wxMenuBar *mbar = frame->GetMenuBar();
   CHECK_RET( mbar, _T("message view frame without menu bar?") );

   mbar->Check(WXMENU_MSG_TOGGLEHEADERS, GetProfileValues().showHeaders);
}

// ----------------------------------------------------------------------------
// wxMessageViewFrame
// ----------------------------------------------------------------------------

wxMessageViewFrame::wxMessageViewFrame(wxWindow *parent,
                                       ASMailFolder *asmf,
                                       UIdType uid)
                  : wxMFrame(_("Mahogany: Message View"), parent)
{
   Profile_obj profile(Profile::CreateTemp(asmf->GetProfile()));
   m_MessageView = MessageView::CreateStandalone(this, profile);

   m_msgCmdProc = MsgCmdProc::Create(m_MessageView);
   m_msgCmdProc->SetFolder(asmf);
   m_msgCmdProc->SetFrame(this);

   AddFileMenu();
   AddEditMenu();

   // don't use AddMessageMenu() as some commands in this menu don't make
   // sense for standalone message preview
   static const int menuCommands[] =
   {
      WXMENU_MSG_EDIT,
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
         WXMENU_MSG_REPLY_SENDER,
         WXMENU_MSG_REPLY_SENDER_WITH_TEMPLATE,
         WXMENU_MSG_REPLY_ALL,
         WXMENU_MSG_REPLY_ALL_WITH_TEMPLATE,
         WXMENU_MSG_REPLY_LIST,
         WXMENU_MSG_REPLY_LIST_WITH_TEMPLATE,
         WXMENU_MSG_FORWARD,
         WXMENU_MSG_FORWARD_WITH_TEMPLATE,
         WXMENU_MSG_FOLLOWUP_TO_NEWSGROUP,
      WXMENU_MSG_SEND_SUBMENU_END,
      WXMENU_MSG_BOUNCE,
      WXMENU_MSG_SEP2,
      WXMENU_MSG_SAVE_TO_FILE,
      WXMENU_MSG_SAVE_TO_FOLDER,
      WXMENU_MSG_MOVE_TO_FOLDER,
      WXMENU_MSG_DELETE,
      WXMENU_MSG_UNDELETE,
      WXMENU_MSG_SEP3,
      WXMENU_MSG_SAVEADDRESSES,
      WXMENU_MSG_TOGGLEHEADERS,
      WXMENU_MSG_SHOWRAWTEXT,
#ifdef EXPERIMENTAL_show_uid
      WXMENU_MSG_SHOWUID,
#endif // EXPERIMENTAL_show_uid
      WXMENU_MSG_SHOWMIME,
   };

   wxMenu *menu = new wxMenu(_T(""), wxMENU_TEAROFF);
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

   m_MessageView->CreateViewMenu();
   AddLanguageMenu();

   // add a toolbar to the frame
   CreateMToolbar(this, WXFRAME_MESSAGE);

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

// ----------------------------------------------------------------------------
// global functions implemented here
// ----------------------------------------------------------------------------

extern MessageView *ShowMessageViewFrame(wxWindow *parent,
                                         ASMailFolder *asmf,
                                         UIdType uid)
{
   CHECK( asmf, NULL, _T("NULL folder in ShowMessageViewFrame()?") );

   wxMessageViewFrame *frame = new wxMessageViewFrame(parent, asmf, uid);

   return frame->GetMessageView();
}

