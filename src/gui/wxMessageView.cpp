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

#include "Mpch.h"

#ifndef USE_PCH
#  include "Mcommon.h"
#  include "Mdefaults.h"
#  include "MApplication.h"
#  include "Profile.h"

#  include "gui/wxIconManager.h"

#  include <wx/stattext.h>
#  include <wx/app.h>
#  include <wx/menu.h>
#  include <wx/panel.h>
#  include <wx/sizer.h>
#endif //USE_PCH

#include <wx/infobar.h>

#include "MessageViewer.h"
#include "MsgCmdProc.h"
#include "FolderView.h"
#include "gui/wxMenuDefs.h"

#include "UIdArray.h"
#include "ASMailFolder.h"

#include "MessageTemplate.h"
#include "Message.h"

#include "mail/MimeDecode.h"

#include "gui/wxMessageView.h"
#include "gui/wxMLog.h"

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

      *value = MIME::DecodeHeader(*value);

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
   virtual String GetSelection() const { return wxEmptyString; }
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
   // set it to NULL initially to avoid sending notification in
   // OnViewerChange() for the initial dummy viewer
   m_FolderView = NULL;

   m_viewerParent = new wxPanel(parent);
   m_infobar = new wxInfoBar(m_viewerParent);

   // Don't delay showing the error messages with unnecessary effects.
   m_infobar->SetShowHideEffects(wxSHOW_EFFECT_NONE, wxSHOW_EFFECT_NONE);

   wxBoxSizer* const sizer = new wxBoxSizer(wxVERTICAL);
   sizer->Add(m_infobar, wxSizerFlags().Expand());
   m_viewerParent->SetSizer(sizer);

   Init(m_viewerParent, profile);

   // now really set it
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
   // First, dismiss any previously shown warnings.
   m_infobar->Dismiss();

   wxMLogTargetSetter logTarget(m_infobar);

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
                              const MessageViewer *viewerNew,
                              const String& nameViewer)
{
   m_infobar->Dismiss();

   if ( m_FolderView )
   {
      m_FolderView->OnMsgViewerChange(viewerNew ? viewerNew->GetWindow()
                                                : NULL);
   }

   if ( viewerOld )
   {
      // Deleting the window will also remove it from the viewer parent sizer,
      // no need to do anything else.
      delete viewerOld->GetWindow();
   }

   // we also need to update the viewers menu to indicate the currently
   // selected one (if we do have a viewer, i.e. are not using the dummy one)
   if ( viewerNew && !nameViewer.empty() )
   {
      int n = m_namesViewers.Index(nameViewer);
      CHECK_RET( n != wxNOT_FOUND, _T("non existing viewer selected?") );

      wxFrame *frame = GetFrame(viewerNew->GetWindow());
      CHECK_RET( frame, _T("no parent frame in wxMessageView") );

      wxMenuBar *mbar = frame->GetMenuBar();
      CHECK_RET( mbar, _T("no menu bar in the frame containing wxMessageView?") );

      mbar->Check(WXMENU_VIEW_VIEWERS_BEGIN + 1 + n, true);
   }

   // finally we must ensure that the new viewer is positioned/sized correctly:
   if ( viewerNew )
   {
      m_viewerParent->GetSizer()->Add(viewerNew->GetWindow(),
                                      wxSizerFlags(1).Expand());
   }

   m_viewerParent->Layout();

   // We may not return to the event loop immediately, as we could be opening a
   // new folder, which takes time, so update the viewer window here explicitly
   // to avoid not laid out yet window showing on the screen, which is ugly.
   m_viewerParent->Update();
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
   WXADD_MENU(frame->GetMenuBar(), MSGVIEW, _("&View"));

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

      // add an accelerator for the viewer
      String desc = descViewers[nViewer];
      desc << _T("\tCtrl-") << nViewer + 1;

      menuView->AppendRadioItem(id, desc);

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
      // remember this filter
      m_namesFilters.Add(name);
      m_statesFilters.Add(enabled);

      // append it to the menu
      menuFlt->AppendCheckItem(id, desc);
      if ( enabled )
         menuFlt->Check(id, true);

      // pass to the next one
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

   ChangeViewer(m_namesViewers[n]);
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

   mbar->Check(WXMENU_VIEW_HEADERS, GetProfileValues().showHeaders);
}

// ----------------------------------------------------------------------------
// wxMessageViewFrame
// ----------------------------------------------------------------------------

wxMessageViewFrame::wxMessageViewFrame(wxWindow *parent,
                                       ASMailFolder *asmf,
                                       UIdType uid)
                  : wxMFrame(_("Mahogany: Message View"), parent)
{
   SetIcon(ICON("MsgViewFrame"));

   m_eventAsync = MEventManager::Register(*this, MEventId_ASFolderResult);

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
      WXMENU_VIEW_HEADERS,
      WXMENU_VIEW_SHOWRAWTEXT,
#ifdef EXPERIMENTAL_show_uid
      WXMENU_VIEW_SHOWUID,
#endif // EXPERIMENTAL_show_uid
      WXMENU_VIEW_SHOWMIME,
   };

   wxMenu *menu = new wxMenu(wxEmptyString, wxMENU_TEAROFF);
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

   CreateToolAndStatusBars();

   // do it after creating the menu as it access the "Toggle headers" item in
   // it
   m_MessageView->SetFolder(asmf);
   m_MessageView->ShowMessage(uid);

   ShowInInitialState();
}

void wxMessageViewFrame::DoCreateToolBar()
{
   CreateMToolbar(this, WXFRAME_MESSAGE);
}

void wxMessageViewFrame::DoCreateStatusBar()
{
   CreateStatusBar(2);
   static const int s_widths[] = { -1, 70 };
   SetStatusWidths(WXSIZEOF(s_widths), s_widths);
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

bool wxMessageViewFrame::OnMEvent(MEventData& event)
{
   // we're only subscribed to ASFolder events
   ASSERT_MSG( event.GetId() == MEventId_ASFolderResult,
                  _T("Got unexpected event in wxMessageViewFrame") );

   if ( !m_MessageView || !m_MessageView->GetMessage() )
   {
      // if we don't have any message, it can't be deleted
      return true;
   }

   // check if it's a delete event for the message we show
   ASMailFolder::Result *res = ((MEventASFolderResultData &)event).GetResult();
   if ( res->GetOperation() == ASMailFolder::Op_DeleteMessages )
   {
      if ( ((ASMailFolder::ResultInt *)res)->GetValue() )
      {
         // is it for our folder?
         if ( res->GetFolder() == m_MessageView->GetFolder() )
         {
            // check if our UID appears among the deleted ones
            const UIdArray& uids = *(res->GetSequence());

            const UIdType uid = m_MessageView->GetMessage()->GetUId();

            const size_t count = uids.Count();
            for ( size_t n = 0; n < count; n++ )
            {
               if ( uids[n] == uid )
               {
                  // it is: close this frame as we don't want to continue
                  // showing a stale message
                  Close();

                  break;
               }
            }
         }
         //else: we're in a strange state...
      }
      //else: it was a delete operation but it failed -- stay opened
   }

   res->DecRef();

   return true;
}

wxMessageViewFrame::~wxMessageViewFrame()
{
   MEventManager::Deregister(m_eventAsync);

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

