///////////////////////////////////////////////////////////////////////////////
// Project:     Mahogany
// File name:   gui/wxMainFrame.cpp - main frame implementation
// Purpose:     implement the GUI for the main program window
// Author:      M-Team
// Modified by:
// Created:     1998
// CVS-ID:      $Id$
// Copyright:   (c) 1998-2006 Mahogany Team
// License:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef   USE_PCH
#  include "Mcommon.h"
#  include "gui/wxMApp.h"
#  include "MHelp.h"
#  include "gui/wxIconManager.h"

#  include <wx/menu.h>
#  include <wx/dirdlg.h>
#endif // USE_PCH

#include "MFolder.h"
#include "ASMailFolder.h"

// these 3 are needed only for SEARCH command processing
#include "MSearch.h"
#include "UIdArray.h"
#include "HeaderInfo.h"

#include "gui/wxMainFrame.h"
#include "gui/wxFolderView.h"
#include "gui/wxFolderTree.h"

#include "gui/wxFiltersDialog.h" // for ConfigureFiltersForFolder
#include "gui/wxIdentityCombo.h" // for IDC_IDENT_COMBO
#include "MFolderDialogs.h"      // for ShowFolderCreateDialog
#include "SpamFilter.h"          // for SpamFilter::Configure()

#include "gui/wxMDialogs.h"
#include "gui/wxMenuDefs.h"

// define this for future, less broken versions of wxWindows to dynamically
// insert/remove the message menu depending on whether we have or not a folder
// view in the frame - with current wxGTK it doesn't work at all, so disabling
#undef HAS_DYNAMIC_MENU_SUPPORT

#ifdef DEBUG
   #include "mail/FolderPool.h"
#endif // DEBUG

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

#ifdef DEBUG

enum
{
   WXMENU_DEBUG_WIZARD = WXMENU_DEBUG_BEGIN + 1,
   WXMENU_DEBUG_SHOW_LICENCE,
   WXMENU_DEBUG_TRACE,
   WXMENU_DEBUG_TOGGLE_LOG,
   WXMENU_DEBUG_CRASH,
#ifdef wxHAS_POWER_EVENTS
   WXMENU_DEBUG_SUSPEND,
   WXMENU_DEBUG_RESUME,
#endif // wxHAS_POWER_EVENTS
   WXMENU_DEBUG_VIEW_OPENED
};

#endif // DEBUG

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_CURRENT_IDENTITY;
extern const MOption MP_FTREE_LEFT;
extern const MOption MP_FVIEW_AUTONEXT_UNREAD_FOLDER;
extern const MOption MP_MAINFOLDER;
extern const MOption MP_OPENFOLDERS;
extern const MOption MP_REOPENLASTFOLDER;
extern const MOption MP_WAIT_NETWORK_AFTER_RESUME;

// ----------------------------------------------------------------------------
// persistent msgboxes we use here
// ----------------------------------------------------------------------------

extern const MPersMsgBox *M_MSGBOX_SEARCH_AGAIN_IF_NO_MATCH;
extern const MPersMsgBox *M_MSGBOX_CONT_UPDATE_AFTER_ERROR;

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// update the status of all folders under the given one in the folder tree,
// returns the number of the folders updated or -1 on error
extern int UpdateFoldersSubtree(const MFolder& folder, wxWindow *parent);

#ifdef OS_WIN

// For now this is only defined under Windows but maybe we can find some way to
// do it elsewhere too later (but this would only be useful if/when wx
// implements resume events generation for the other platforms).
#define CAN_CHECK_NETWORK_STATE

#include <wx/dynlib.h>

// Check if we have network connection at all. This is a bit vague but seems to
// do roughly what we need here, i.e. if this function returns false we have
// really no chance of connecting to the remote servers while it might work if
// it returns true.
static bool IsNetworkAvailable()
{
   static wxDynamicLibrary s_dllWinINet("wininet", wxDL_QUIET);
   if ( !s_dllWinINet.IsLoaded() )
   {
      // If we can't check for it, it's better to assume that it is available
      // immediately than never returning true at all.
      return true;
   }

   typedef BOOL (WINAPI *InternetGetConnectedState_t)(LPDWORD, DWORD);
   static InternetGetConnectedState_t
      wxDL_INIT_FUNC(s_pfn, InternetGetConnectedState, s_dllWinINet);
   if ( !s_pfnInternetGetConnectedState )
   {
      // As above.
      return true;
   }

   DWORD flags;
   return (*s_pfnInternetGetConnectedState)(&flags, 0) == TRUE;
}

#endif // OS_WIN

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// override wxFolderTree OnOpenHere() function to open the folder in this
// frame and OnClose() to close it
// ----------------------------------------------------------------------------

class wxMainFolderTree : public wxFolderTree
{
public:
   wxMainFolderTree(wxWindow *parent, wxMainFrame *frame)
      : wxFolderTree(parent)
   {
      m_frame = frame;
   }

   virtual void OnSelectionChange(MFolder *oldsel, MFolder *newsel)
   {
      if ( newsel )
      {
         // these messages are not very useful but often overwrite others, more
         // useful ones
#if 0
         wxLogStatus(m_frame, _("Selected folder '%s'."),
                     newsel->GetFullName().c_str());
#endif // 0

         m_frame->UpdateFolderMenuUI(newsel);
      }

      wxFolderTree::OnSelectionChange(oldsel, newsel);
   }

   virtual void OnOpenHere(MFolder *folder)
   {
      if ( !folder )
      {
         m_frame->CloseFolder();
      }
      else if ( !m_frame->OpenFolder(folder) )
      {
         // normally the base class version DecRef()s it but as we're going to
         // pass NULL to it, do it ourselves
         folder->DecRef();
         folder = NULL;
      }

      wxFolderTree::OnOpenHere(folder);
   }

   virtual void OnView(MFolder *folder)
   {
      CHECK_RET( folder, _T("can't view a NULL folder") );

      if ( !m_frame->OpenFolder(folder, true /* RO */) )
      {
         folder->DecRef();
         folder = NULL;
      }

      wxFolderTree::OnView(folder);
   }

   virtual bool OnClose(MFolder *folder)
   {
      m_frame->CloseFolder(folder);

      return wxFolderTree::OnClose(folder);
   }

private:
   wxMainFrame *m_frame;
};

// ----------------------------------------------------------------------------
// wxMainFolderView: the folder view which keeps our m_folderName in sync
// ----------------------------------------------------------------------------

class wxMainFolderView : public wxFolderView
{
public:
   wxMainFolderView(wxWindow *parent, wxMainFrame *mainFrame)
      : wxFolderView(parent)
   {
      m_mainFrame = mainFrame;
   }

   virtual bool MoveToNextUnread(bool /* takeNextIfNoUnread */ = true)
   {
      if ( wxFolderView::MoveToNextUnread(false /* don't take next */) )
      {
         // we moved to the next message in this folder, ok
         return true;
      }

      if ( !READ_CONFIG(m_Profile, MP_FVIEW_AUTONEXT_UNREAD_FOLDER) )
      {
         // this feature is disabled
         return false;
      }

      // no more unread messages here, go to the next unread folder
      MFolder_obj folder(m_mainFrame->GetFolderTree()->FindNextUnreadFolder());
      if ( !folder )
      {
         // no unread folders neither, what can we do?
         return false;
      }

      m_mainFrame->OpenFolder(folder);

      return true;
   }

   virtual void SetFolder(MailFolder *mf)
   {
      if ( !mf )
         m_mainFrame->ClearFolderName();

      wxFolderView::SetFolder(mf);
   }

   virtual void OnAppExit()
   {
      // don't do anything here: the base class version saves this folder name
      // in MP_OPENFOLDERS config entry but the main frame does it for us
   }

   virtual Profile *GetFolderProfile() const
   {
      Profile *profile = GetProfile();
      if ( !profile )
      {
         profile = mApplication->GetProfile();
      }

      profile->IncRef();

      return profile;
   }

private:
   wxMainFrame *m_mainFrame;
};

// ============================================================================
// async search helper classes
// ============================================================================

// ----------------------------------------------------------------------------
// AsyncSearchData: data for one search operation
// ----------------------------------------------------------------------------

class AsyncSearchData
{
public:
   // default ctor
   AsyncSearchData()
   {
      m_mfVirt = NULL;

      m_folderVirt = NULL;

      m_nMatchingMessages =
      m_nMatchingFolders = 0;

      m_allScheduled = false;
   }

   ~AsyncSearchData()
   {
      if ( m_mfVirt )
      {
         m_mfVirt->DecRef();
         m_folderVirt->DecRef();
      }
   }

   // add a record for another folder being searched
   //
   // NB: we take ownership of the mf pointer and will DecRef() it
   bool AddSearchFolder(MailFolder *mf, Ticket t)
   {
      CHECK( mf && t != ILLEGAL_TICKET, false,
             _T("invalid params in AsyncSearchData::AddSearchFolder") );

      m_listSingleSearch.push_back(new SingleSearchData(t, mf));

      return true;
   }

   // tell us that AddSearchFolder() won't be called any more, must be called
   // before the search results are shown!
   void FinalizeSearch() { m_allScheduled = true; }

   // process the search result if it concerns this search, in which case true
   // is returned (even if there were errors), otherwise return false to
   // indicate that we don't have anything to do with this result
   bool HandleSearchResult(const ASMailFolder::Result& result)
   {
      const Ticket t = result.GetTicket();
      for ( SingleSearchDataList::iterator i = m_listSingleSearch.begin();
            i != m_listSingleSearch.end();
            ++i )
      {
         if ( i->GetTicket() == t )
         {
            if ( ((const ASMailFolder::ResultInt&)result).GetValue() )
            {
               const UIdArray *uidsMatching = result.GetSequence();
               if ( !uidsMatching )
               {
                  FAIL_MSG( _T("searched ok but no search results??") );
               }
               else // have some messages to show
               {
                  // create the virtual folder to show the results if not done
                  // yet
                  if ( GetResultsVFolder() )
                  {
                     const MailFolder *mf = i->GetMailFolder();

                     // and append all matching messages to the results folder
                     HeaderInfoList_obj hil(mf->GetHeaders());
                     if ( hil )
                     {
                        size_t nMatches = 0;

                        size_t count = uidsMatching->GetCount();
                        for ( size_t n = 0; n < count; n++ )
                        {
                           Message_obj msg(mf->GetMessage((*uidsMatching)[n]));
                           if ( msg )
                           {
                              m_mfVirt->AppendMessage(*msg.Get());

                              nMatches++;
                           }
                        }

                        if ( nMatches )
                        {
                           m_nMatchingMessages += nMatches;
                           m_nMatchingFolders++;
                        }
                     }
                  }
               }
            }
            //else: nothing found at all in this folder, nothing to do

            // we don't care about this one any more
            m_listSingleSearch.erase(i);

            // it was our result
            return true;
         }
      }

      // not the result of this search at all
      return false;
   }

   // if we're still waiting for the completion of [another] search, return
   // false, otherwise return true
   bool IsSearchCompleted() const
      { return m_allScheduled && m_listSingleSearch.empty(); }

   // show the search results to the user
   void ShowSearchResults(wxFrame *frame)
   {
      ASSERT_MSG( IsSearchCompleted(), _T("shouldn't be called yet!") );

      if ( m_folderVirt && m_nMatchingMessages )
      {
         OpenFolderViewFrame(m_folderVirt, frame);

         wxLogStatus(frame, _("Found %lu messages in %lu folders."),
                     (unsigned long)m_nMatchingMessages,
                     (unsigned long)m_nMatchingFolders);

      }
      else
      {
         if ( MDialog_YesNoDialog
              (
               _("No matching messages found.\n"
               "\n"
               "Would you like to search again?"),
               frame,
               MDIALOG_YESNOTITLE,
               M_DLG_YES_DEFAULT,
               M_MSGBOX_SEARCH_AGAIN_IF_NO_MATCH
              ) )
         {
            wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED,
                                 WXMENU_FOLDER_SEARCH);
            wxPostEvent(frame, event);
         }
      }
   }

private:
   // returns, creating if necessary, the virtual folder in which we show the
   // search results
   //
   // NB: the returned pointer should not be DecRef()'d by caller
   MailFolder *GetResultsVFolder()
   {
      if ( !m_mfVirt )
      {
         // FIXME: we should use the profile of the common parent of the
         //        folders being searched instead of the global settings here
         //        or, at the very least, the profile of the only folder being
         //        searched if we search in only one folder
         m_folderVirt = MFolder::CreateTemp
                        (
                         _("Search results"),
                         MF_VIRTUAL,
                         mApplication->GetProfile()
                        );

         if ( m_folderVirt )
         {
            // FIXME: a hack to prevent the same search results folder from
            //        being reused all the time
            static unsigned int s_countSearch = 0;
            m_folderVirt->SetPath(String::Format(_T("(%u)"), ++s_countSearch));

            m_mfVirt = MailFolder::OpenFolder(m_folderVirt);
            if ( !m_mfVirt )
            {
               m_folderVirt->DecRef();
               m_folderVirt = NULL;
            }
         }
      }

      if ( !m_mfVirt )
      {
         ERRORMESSAGE((_("Failed to create the search results folder")));
      }

      return m_mfVirt;
   }

   // SingleSearchData: struct containing info about search in one folder
   class SingleSearchData
   {
   public:
      SingleSearchData(Ticket ticket, MailFolder *mf)
      {
         m_ticket = ticket;
         m_mf = mf;
      }

      ~SingleSearchData() { m_mf->DecRef(); }

      /// get the associated async operation ticket
      Ticket GetTicket() const { return m_ticket; }

      /// get the the mail folder we're searching in (NOT IncRef()'d)
      MailFolder *GetMailFolder() const { return m_mf; }

   private:
      /// the associated async ticket
      Ticket m_ticket;

      /// the folder we're searching in
      MailFolder *m_mf;

      /// the UIDs found so far
      UIdArray m_uidsFound;
   };

   // the list containing the individual search records for all folders we're
   // searching in
   M_LIST_OWN(SingleSearchDataList, SingleSearchData) m_listSingleSearch;

   // the virtual folder we show the search results in and the associated
   // MFolder object for it
   MailFolder *m_mfVirt;
   MFolder *m_folderVirt;

   // the number of messages found so far
   size_t m_nMatchingMessages;

   // the number of folders containing the matching messages
   size_t m_nMatchingFolders;

   // have all folders we search been already added to us using
   // AddSearchFolder()?
   bool m_allScheduled;
};

// ----------------------------------------------------------------------------
// GlobalSearchData: contains data for all search operations in progress
// ----------------------------------------------------------------------------

class GlobalSearchData
{
public:
   // ctor
   GlobalSearchData(wxFrame *frame) { m_frame = frame; }

   // create a record for a new search operation
   AsyncSearchData *StartNewSearch()
   {
      AsyncSearchData *ssd = new AsyncSearchData;
      m_listAsyncSearch.push_back(ssd);
      return ssd;
   }

   // process the result of the async search operation
   void HandleSearchResult(const ASMailFolder::Result& result)
   {
      // leave the real handling to the search this result concerns
      for ( AsyncSearchDataList::iterator i = m_listAsyncSearch.begin();
            i != m_listAsyncSearch.end();
            ++i )
      {
         if ( i->HandleSearchResult(result) )
         {
            // was it the last search result for this search
            if ( i->IsSearchCompleted() )
            {
               // yes, show the results ...
               i->ShowSearchResults(m_frame);

               // ... and delete the stale stale search record
               m_listAsyncSearch.erase(i);
            }

            return;
         }
      }

      FAIL_MSG( _T("got search result for a search we hadn't ever started?") );
   }

private:
   M_LIST_OWN(AsyncSearchDataList, AsyncSearchData) m_listAsyncSearch;

   wxFrame *m_frame;
};

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxMainFrame, wxMFrame)
   EVT_MENU(-1,    wxMainFrame::OnCommandEvent)
   EVT_TOOL(-1,    wxMainFrame::OnCommandEvent)

   EVT_IDLE(wxMainFrame::OnIdle)

   EVT_UPDATE_UI(WXMENU_VIEW_SHOWMIME, wxMainFrame::OnUpdateUIEnableIfHasFolder)
   EVT_UPDATE_UI(WXMENU_VIEW_SHOWRAWTEXT, wxMainFrame::OnUpdateUIEnableIfHasFolder)

   EVT_UPDATE_UI(WXMENU_VIEW_HEADERS, wxMainFrame::OnUpdateUIEnableIfHasPreview)
   EVT_UPDATE_UI(WXMENU_EDIT_FIND, wxMainFrame::OnUpdateUIEnableIfHasPreview)
   EVT_UPDATE_UI(WXMENU_EDIT_FINDAGAIN, wxMainFrame::OnUpdateUIEnableIfHasPreview)

   EVT_CHOICE(IDC_IDENT_COMBO, wxMainFrame::OnIdentChange)

#ifdef wxHAS_POWER_EVENTS
   EVT_POWER_SUSPENDED(wxMainFrame::OnPowerSuspended)
   EVT_POWER_RESUME(wxMainFrame::OnPowerResume)
#endif // wxHAS_POWER_EVENTS
END_EVENT_TABLE()

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// construction
// ----------------------------------------------------------------------------

wxMainFrame::wxMainFrame(const String &iname, wxFrame *parent)
           : wxMFrame(iname,parent)
{
   // init members
   m_searchData = NULL;
   m_FolderTree = NULL;
   m_FolderView = NULL;

   // set frame icon/title, create status bar
   SetIcon(ICON(_T("MainFrame")));
   SetTitle(_("Copyright (C) 1997-2007 The Mahogany Developers Team"));

   // create the child controls
   m_splitter = new wxPSplitterWindow(_T("MainSplitter"), this, -1,
                                      wxDefaultPosition, wxDefaultSize,
                                      0);

   // we have to do this to prevent the splitter from clipping the sash
   // position to its current (== initial, small) size
   const wxSize sizeFrame = GetClientSize();
   m_splitter->SetSize(sizeFrame);

   // insert treectrl in one of the splitter panes
   m_FolderTree = new wxMainFolderTree(m_splitter, this);
   m_FolderView = new wxMainFolderView(m_splitter, this);

   wxWindow *winLeft,
            *winRight;
   if ( READ_APPCONFIG_BOOL(MP_FTREE_LEFT) )
   {
      winLeft = m_FolderTree->GetWindow();
      winRight = m_FolderView->GetWindow();
   }
   else // folder tre on the right
   {
      winLeft = m_FolderView->GetWindow();
      winRight = m_FolderTree->GetWindow();
   }

   // construct the menu and toolbar
   AddFileMenu();
   AddFolderMenu();
   AddEditMenu();

#ifndef HAS_DYNAMIC_MENU_SUPPORT
   AddMessageMenu();
   m_FolderView->CreateViewMenu();
   AddLanguageMenu();
#endif

#ifdef DEBUG
   wxMenu *menuDebug = new wxMenu;
   menuDebug->Append(WXMENU_DEBUG_WIZARD, _T("Run install &wizard..."));
   menuDebug->Append(WXMENU_DEBUG_SHOW_LICENCE, _T("Show &licence dialog..."));
   menuDebug->Append(WXMENU_DEBUG_TRACE, "Add/remove &trace mask...");
   menuDebug->AppendCheckItem(WXMENU_DEBUG_TOGGLE_LOG,
                              _T("Toggle &debug logging\tCtrl-Alt-D"));
   menuDebug->Append(WXMENU_DEBUG_CRASH, _T("Provoke a c&rash"));
#ifdef wxHAS_POWER_EVENTS
   menuDebug->AppendSeparator();
   menuDebug->Append(WXMENU_DEBUG_SUSPEND, "Simulate &suspend");
   menuDebug->Append(WXMENU_DEBUG_RESUME, "Simulate &resume");
#endif // wxHAS_POWER_EVENTS
   menuDebug->AppendSeparator();
   menuDebug->Append(WXMENU_DEBUG_VIEW_OPENED, "View &opened folders");

   GetMenuBar()->Append(menuDebug, _T("&Debug"));
#endif // debug

   AddHelpMenu();

   CreateToolAndStatusBars();

   // disable the operations which don't make sense for viewer
   wxMenuBar *menuBar = GetMenuBar();
   menuBar->Enable(WXMENU_EDIT_CUT, FALSE);
   menuBar->Enable(WXMENU_EDIT_PASTE, FALSE);
   menuBar->Enable(WXMENU_EDIT_PASTE_QUOTED, FALSE);

   m_ModulesMenu = NULL;

   // update the menu to match the initial selection
   MFolder_obj folder(m_FolderTree->GetSelection());
   if ( folder )
   {
      UpdateFolderMenuUI(folder);
   }

   // split the window after creating the menus and toolbars as they change
   // client frame size and so result in sash repositioning which we avoid by
   // doing this in the end
   m_splitter->SetMinimumPaneSize(50);
   m_splitter->SplitVertically(winLeft, winRight, sizeFrame.x/3);

   m_FolderTree->GetWindow()->SetFocus();

   ShowInInitialState();
}

void wxMainFrame::DoCreateToolBar()
{
   CreateMToolbar(this, WXFRAME_MAIN);
}

void wxMainFrame::DoCreateStatusBar()
{
   CreateStatusBar();
}

void wxMainFrame::AddFolderMenu(void)
{
   WXADD_MENU(GetMenuBar(), FOLDER, _("&Folder"));
}

void
wxMainFrame::UpdateFolderMenuUI(MFolder *sel)
{
   m_FolderTree->UpdateMenu(GetMenuBar()->GetMenu(MMenu_Folder), sel);
}

void
wxMainFrame::CloseFolder(MFolder *folder)
{
   if ( !folder || folder->GetFullName() == m_folderName )
   {
      m_FolderView->SetFolder(NULL);

      //m_folderName.clear(); -- now done in our ClearFolderName()
   }
   //else: otherwise, we don't have it opened
}

bool
wxMainFrame::OpenFolder(MFolder *pFolder, bool readonly)
{
   CHECK( pFolder, false, "can't open NULL folder" );

   pFolder->IncRef(); // to compensate for DecRef() by MFolder_obj
   MFolder_obj folder(pFolder);

   // don't do anything if there is nothing to change
   const String folderName = folder->GetFullName();
   if ( m_folderName == folderName )
   {
      wxLogStatus(this, _("The folder '%s' is already opened."),
                  m_folderName.c_str());

      return true;
   }

   // save the full folder name in m_folderName for use from elsewhere
   m_folderName = folderName;


   // and do try to open the folder
   if ( !m_FolderView->OpenFolder(folder, readonly) )
   {
      if ( mApplication->GetLastError() == M_ERROR_CANCEL )
      {
         // don't set the unaccessible flag - may be it's ok
         wxLogStatus(this, _("Opening folder '%s' cancelled."),
                     m_folderName.c_str());
      }

      m_folderName.clear();
   }
   else // folder opened successfully
   {
      // Associate it with this frame to e.g. let it use our status bar for any
      // messages.
      MailFolder_obj(m_FolderView->GetMailFolder())->SetInteractiveFrame(this);

      // Select the folder in the tree as well.
      m_FolderTree->SelectFolder(folder);
   }

#ifdef HAS_DYNAMIC_MENU_SUPPORT
   // only add the msg menu once
   static bool s_hasMsgMenu = false;
   if ( !s_hasMsgMenu )
   {
      AddMessageMenu();
      s_hasMsgMenu = true;
   }
#endif // HAS_DYNAMIC_MENU_SUPPORT

   return !m_folderName.empty();
}

bool
wxMainFrame::CanClose() const
{
   // just in case it's still opened - may get in the way of our dialogs
   CloseSplash();

   // closing the main frame will close the app so ask the other frames
   // whether it's ok to close them
   if ( !mApplication->CanClose() )
   {
      // not confirmed by user
      return false;
   }

   // remember the last opened folder name
   if ( READ_APPCONFIG(MP_REOPENLASTFOLDER) )
   {
      // save the last opened folder if we're going to reopen it
      mApplication->GetProfile()->writeEntry(MP_MAINFOLDER, m_folderName);
   }

   // make sure folder is closed before we close the window
   m_FolderView->SetFolder(NULL);

   // tell all the others that we're going away
   mApplication->OnClose();

   return true;
}

// ----------------------------------------------------------------------------
// callbacks
// ----------------------------------------------------------------------------

// we use OnIdle() and not OnUpdateUI() as a menu in the menubar doesn't have
// an id, unfortunately - at least in the current wxWin version
void wxMainFrame::OnIdle(wxIdleEvent &event)
{
   // we don't want to block further event propagation in any case
   event.Skip();

   // we may be called from ctor before m_FolderView is created under wxGTK
   // (because wxTreeCtrl ctor implicitly calls wxYield() which results in an
   // idle dispatch), ignore calls that early
   if ( !m_FolderView )
      return;

   // although we don't have a preview yet, the menus are created enabled by
   // default, so setting this to true initially ensures that the setting is in
   // sync with the real menu state
   static bool s_hasPreview = true;
   static bool s_hasFolder = true;

   bool hasFolder = m_FolderView->GetFolder() != NULL;
   if ( hasFolder != s_hasFolder )
   {
      EnableMMenu(MMenu_Message, this, hasFolder);

      // also update the toolbar buttons
      static const int buttonsToDisable[] =
      {
         WXTBAR_MSG_NEXT_UNREAD,
         WXTBAR_MSG_OPEN,
         WXTBAR_MSG_REPLY,
         WXTBAR_MSG_REPLYALL,
         WXTBAR_MSG_FORWARD,
         WXTBAR_MSG_PRINT,
         WXTBAR_MSG_DELETE,
      };

      wxToolBar * const tbar = GetToolBar();
      if ( tbar )
      {
         for ( size_t n = 0; n < WXSIZEOF(buttonsToDisable); n++ )
         {
            EnableToolbarButton(tbar, buttonsToDisable[n], hasFolder);
         }
      }

      s_hasFolder = hasFolder;
   }

   bool hasPreview = hasFolder && m_FolderView->HasPreview();
   if ( hasPreview != s_hasPreview )
   {
      EnableMMenu(MMenu_Language, this, hasPreview);

      // only change the internal status now, if we succeeded in
      // enabling/disabling the menu, otherwise it would get out of sync
      s_hasPreview = hasPreview;
   }
}

void wxMainFrame::OnUpdateUIEnableIfHasFolder(wxUpdateUIEvent& event)
{
   event.Enable( m_FolderView && m_FolderView->HasFolder() );
}

void wxMainFrame::OnUpdateUIEnableIfHasPreview(wxUpdateUIEvent& event)
{
   event.Enable( m_FolderView && m_FolderView->HasPreview() );
}

void
wxMainFrame::OnIdentChange(wxCommandEvent &event)
{
   Profile *profile = mApplication->GetProfile();
   if ( event.GetInt() == 0 )
   {
      // restore the default identity
      profile->DeleteEntry(MP_CURRENT_IDENTITY);
   }
   else
   {
      profile->writeEntry(MP_CURRENT_IDENTITY, event.GetString());
   }

   // TODO: update everything on identity change
}

void
wxMainFrame::OnCommandEvent(wxCommandEvent &event)
{
   const int id = event.GetId();

   if ( WXMENU_CONTAINS(FOLDER, id) && !WXSUBMENU_CONTAINS(MSG_SELECT, id) )
   {
      switch ( id )
      {
         case WXMENU_FOLDER_OPEN:
         case WXMENU_FOLDER_OPEN_RO:
            {
               MFolder_obj folder(MDialog_FolderChoose
                                  (
                                      this, // parent window
                                      NULL, // parent folder
                                      true  // open
                                  ));
               if ( folder )
               {
                  OpenFolderViewFrame
                  (
                     folder,
                     this,
                     id == WXMENU_FOLDER_OPEN_RO ? MailFolder::ReadOnly
                                                 : MailFolder::Normal
                  );
               }
            }
            break;

         case WXMENU_FOLDER_CREATE:
            {
               MFolder_obj parent(m_FolderTree->GetSelection());

               SafeDecRef(AskUserToCreateFolder(this, parent));
            }
            break;

         case WXMENU_FOLDER_SEARCH:
            DoFolderSearch();
            break;

         case WXMENU_FOLDER_FILTERS:
            {
               MFolder_obj folder(m_FolderTree->GetSelection());
               if ( folder )
                  ConfigureFiltersForFolder(folder, this);
            }
            break;

         case WXMENU_FOLDER_WHENCE:
            {
               MFolder_obj folder(m_FolderTree->GetSelection());
               if ( folder )
                  FindFiltersForFolder(folder, this);
            }
            break;

         case WXMENU_FOLDER_SPAM_CONFIG:
            {
               MFolder_obj folder(m_FolderTree->GetSelection());
               if ( folder )
               {
                  Profile_obj profile(folder->GetProfile());
                  SpamFilter::Configure(profile, this);
               }
            }
            break;

         case WXMENU_FOLDER_IMPORTTREE:
            // create all MBOX folders under the specified dir
            {
               // TODO: it should be more user friendly (i.e. either let the
               //       user choose the folder too or take the dir from folder
               //       or both...)

               // get the directory to use
               wxDirDialog dlgDir(this,
                                  _("Choose directory containing MBOX folders"),
                                  mApplication->GetLocalDir());
               if ( dlgDir.ShowModal() == wxID_OK )
               {
                  // get the parent folder
                  MFolder *folder = m_FolderTree->GetSelection();
                  if ( !folder )
                  {
                     // use root folder if no selection
                     folder = MFolder::Get(wxEmptyString);
                  }

                  // do create them
                  size_t count = CreateMboxSubtree(folder, dlgDir.GetPath());
                  if ( count )
                  {
                     wxLogStatus(this, _("Created %u folders under '%s'."),
                                 count, folder->GetPath().c_str());
                  }

                  folder->DecRef();
               }
               //else: dir dlg cancelled by user
            }
            break;

         case WXMENU_FOLDER_CLOSEALL:
            {
               int nClosed = MailFolder::CloseAll();
               if ( nClosed < 0 )
               {
                  wxLogError(_("Failed to close all opened folders."));
               }
               else
               {
                  wxLogStatus(this, _("Closed %d folders."), nClosed);
               }
            }
            break;

         case WXMENU_FOLDER_UPDATE:
            {
               MFolder_obj folder(m_FolderTree->GetSelection());
               if ( folder )
               {
                  if ( !MailFolder::CheckFolder(folder) )
                  {
                     wxLogError(_("Failed to update the status of "
                                  "the folder '%s'."),
                                folder->GetFullName().c_str());
                  }
                  else
                  {
                     wxLogStatus(this, _("Updated status of the folder '%s'"),
                                 folder->GetFullName().c_str());
                  }
               }
            }
            break;

         case WXMENU_FOLDER_UPDATEALL:
            {
               MFolder *folder = m_FolderTree->GetSelection();
               if ( !folder )
                  folder = MFolder::Get(wxEmptyString);

               int nUpdated = UpdateFoldersSubtree(*folder, this);
               if ( nUpdated < 0 )
               {
                  wxLogError(_("Failed to update the status"));
               }
               else
               {
                  wxLogStatus(this, _("Updated status of %d folders."),
                              nUpdated);
               }

               folder->DecRef();
            }
            break;

         default:
            // all others are processed by the folder tree
            m_FolderTree->ProcessMenuCommand(id);
      }
   }
   else if ( m_FolderView &&
            ( WXSUBMENU_CONTAINS(MSG_SELECT, id) ||
              WXMENU_CONTAINS(MSG, id) ||
              WXMENU_CONTAINS(LAYOUT, id) ||
              ( WXMENU_CONTAINS(LANG, id) && (id != WXMENU_LANG_SET_DEFAULT) ) ||
              // in edit menu there are some commands which should be forwarded
              // to the folder view but the others should not so it is a bit
              // tricky...
              id == WXMENU_EDIT_COPY ||
              id == WXMENU_EDIT_FIND ||
              id == WXMENU_EDIT_FINDAGAIN ||
              id == WXMENU_EDIT_SELECT_ALL ||
              // view menu is handled by the base class but we extend it with
              // extra commands in message view menu which we must handle
              // ourselves
              (WXMENU_CONTAINS(MSGVIEW, id) &&
               !WXMENU_CONTAINS(VIEW, id)) ||
              WXMENU_CONTAINS(VIEW_FILTERS, id) ||
              WXMENU_CONTAINS(VIEW_VIEWERS, id) ) )
   {
      m_FolderView->OnCommandEvent(event);
   }
   else if (id == WXMENU_HELP_CONTEXT)
   {
      mApplication->Help(MH_MAIN_FRAME,this);
   }
#ifdef DEBUG
   else if ( WXMENU_CONTAINS(DEBUG, id) )
   {
      switch ( id )
      {
         case WXMENU_DEBUG_WIZARD:
            extern bool RunInstallWizard(bool justTest);

            wxLogStatus(_T("Running installation wizard..."));

            if ( !RunInstallWizard(true) )
            {
               wxLogWarning(_T("Wizard failed!"));
            }
            break;

         case WXMENU_DEBUG_SHOW_LICENCE:
            ShowLicenseDialog(this);
            break;

         case WXMENU_DEBUG_TRACE:
            {
               wxString mask;
               if ( MInputBox
                    (
                     &mask,
                     "Modify tracing mask",
                     "Enter trace mask to enable it or enter it preceded "
                     "by \"-\" to disable it if currently enabled:",
                     this,
                     "DebugTraceMask"
                    ) && !mask.empty() )
               {
                  if ( mask[0] == '-' )
                  {
                     mask.erase(0, 1);
                     wxLogTrace(mask, "Tracing disabled for \"%s\"", mask);
                     wxLog::RemoveTraceMask(mask);
                  }
                  else
                  {
                     wxLog::AddTraceMask(mask);
                     wxLogTrace(mask, "Tracing enabled for \"%s\"", mask);
                  }
               }
            }
            break;

         case WXMENU_DEBUG_TOGGLE_LOG:
            extern bool g_debugMailForceOn;

            g_debugMailForceOn = event.IsChecked();
            break;

         case WXMENU_DEBUG_CRASH:
            *(char *)17 = '!';
            break;

#ifdef wxHAS_POWER_EVENTS
         case WXMENU_DEBUG_SUSPEND:
            {
               wxPowerEvent dummyEvent;
               OnPowerSuspended(dummyEvent);
            }
            break;

         case WXMENU_DEBUG_RESUME:
            {
               wxPowerEvent dummyEvent;
               OnPowerResume(dummyEvent);
            }
            break;
#endif // wxHAS_POWER_EVENTS

         case WXMENU_DEBUG_VIEW_OPENED:
            {
               wxArrayString folderNames;

               MFPool::Cookie cookie;
               MFolder *folder = NULL;
               for ( MailFolder *mf = MFPool::GetFirst(cookie, NULL, &folder);
                     mf;
                     mf = MFPool::GetNext(cookie, NULL, &folder) )
               {
                  folderNames.push_back(folder->GetFullName());

                  folder->DecRef();
                  mf->DecRef();
               }

               if ( folderNames.empty() )
               {
                  wxLogMessage("No folders are currently opened.");
               }
               else
               {
                  const unsigned count = folderNames.size();
                  wxString msg;
                  msg.Printf("The following %u folders are opened:", count);
                  for ( unsigned n = 0; n < count; n++ )
                  {
                     msg << "\n    " << folderNames[n];
                  }

                  wxLogMessage("%s", msg);
               }
            }
            break;

         default:
            FAIL_MSG( _T("unknown debug menu command?") );
      }
   }
#endif // DEBUG
   else
   {
      wxMFrame::OnMenuCommand(id);
   }
}

// ----------------------------------------------------------------------------
// power events
// ----------------------------------------------------------------------------

#ifdef wxHAS_POWER_EVENTS

void wxMainFrame::OnPowerSuspended(wxPowerEvent& WXUNUSED(event))
{
   ASSERT_MSG( m_foldersToResume.empty(), "didn't resume from last suspend?" );

   MFPool::Cookie cookie;
   for ( MailFolder *mf = MFPool::GetFirst(cookie); mf; mf = MFPool::GetNext(cookie) )
   {
      MailFolder_obj mfObj(mf);

      // We only need to reopen the folders that the user is working with and
      // those that need to be constantly monitored.
      if ( mf->GetInteractiveFrame() || (mf->GetFlags() & MF_FLAGS_MONITOR))
      {
         if ( mf->Suspend() )
         {
            // Pass ownership to the list of folders to resume.
            m_foldersToResume.push_back(mfObj.Detach());
         }
         //else: this (probably local) folder will survive resume.
      }
      else
      {
         // All the others can be simply closed.
         mf->Close(false /* don't linger */);
      }
   }

   if ( !m_foldersToResume.empty() )
   {
      wxLogStatus(_("Closed %lu folders which will be reopened on resume."),
                  (unsigned long)m_foldersToResume.size());
   }

   // save all options just in case
   Profile::FlushAll();
}

void wxMainFrame::OnPowerResume(wxPowerEvent& WXUNUSED(event))
{
   if ( m_foldersToResume.empty() )
      return;

   // copy to a temporary variable to avoid problems in case we get several
   // resume messages (currently happens under Windows sometimes)
   MailFolders foldersToResume;
   foldersToResume.swap(m_foldersToResume);

   wxLogStatus(_("Reopening %lu folders on system resume"),
               (unsigned long)foldersToResume.size());

#ifdef CAN_CHECK_NETWORK_STATE
   bool checkedNetwork = false;
#endif // CAN_CHECK_NETWORK_STATE
   for ( MailFolders::iterator i = foldersToResume.begin();
         i != foldersToResume.end();
         ++i )
   {
      MailFolder* const mf = *i;

#ifdef CAN_CHECK_NETWORK_STATE
      if ( !checkedNetwork && mf->NeedsNetwork() )
      {
         // We can get a resume event before the system got reconnected so stay
         // here for a few seconds to give it a chance to do it as otherwise
         // reopening the remote folders after resume would always fail.
         const long resumeTimeout = READ_APPCONFIG(MP_WAIT_NETWORK_AFTER_RESUME);
         for ( int n = 0; n < resumeTimeout/2; n++ )
         {
            if ( IsNetworkAvailable() )
            {
               wxLogDebug("Network is available.");
               break;
            }

            wxLogDebug("Network is still not available, sleeping.");
            wxMilliSleep(500);
         }

         checkedNetwork = true;
      }
#endif // CAN_CHECK_NETWORK_STATE

      if ( !mf->Resume() )
      {
         ERRORMESSAGE((_("Failed to reopen folder \"%s\" after resuming from sleep."),
                      mf->GetName()));
      }
      else
      {
         // Notify everybody about the folder reopening. This is an abuse of
         // this event as there are not necessarily any real updates but it's
         // the simplest way to make all folder listings in all the existing
         // folder views to refresh.
         MEventManager::Send(new MEventFolderUpdateData(mf));
      }

      mf->DecRef();
   }
}

#endif // wxHAS_POWER_EVENTS

void wxMainFrame::DoFolderSearch()
{
   SearchCriterium crit;

   Profile_obj profile(GetFolderProfile());
   MFolder_obj folder(m_FolderTree->GetSelection());
   if ( ConfigureSearchMessages(&crit, profile, folder, this) )
   {
      AsyncSearchData *searchData = NULL;

      const wxArrayString& folderNames = crit.m_Folders;
      size_t count = folderNames.GetCount();
      for ( size_t n = 0; n < count; n++ )
      {
         const String& name = folderNames[n];

         MFolder_obj folder(name);
         if ( !folder )
         {
            wxLogError(_("Can't search for messages in a "
                         "non existent folder '%s'."),
                       name.c_str());
            continue;
         }

         if ( folder->CanOpen() )
         {
            ASMailFolder *asmf = ASMailFolder::OpenFolder(folder);
            if ( !asmf )
            {
               wxLogError(_("Can't search for messages in the "
                            "folder '%s'."),
                          name.c_str());

               continue;
            }

            // opened ok, search in it
            Ticket t = asmf->SearchMessages(&crit, this);
            if ( t != ILLEGAL_TICKET )
            {
               // create a new single search data on the fly
               if ( !searchData )
               {
                  // create the search data on demand as well if necessary
                  InitSearchData();

                  searchData = m_searchData->StartNewSearch();
               }

               searchData->AddSearchFolder(asmf->GetMailFolder(), t);
            }

            asmf->DecRef();
         }
         //else: silently skip this one
      }

      if ( searchData )
      {
         // no more calls to AddSearchFolder() scheduled
         searchData->FinalizeSearch();
      }
   }
   //else: cancelled by user
}

void wxMainFrame::InitSearchData()
{
   if ( m_searchData )
      return;

   m_searchData = new GlobalSearchData(this);

   m_cookieASMf = MEventManager::Register(*this, MEventId_ASFolderResult);
}

bool wxMainFrame::OnMEvent(MEventData& ev)
{
   if ( ev.GetId() == MEventId_ASFolderResult )
   {
      const MEventASFolderResultData& event = (MEventASFolderResultData &)ev;

      ASMailFolder::Result *result = event.GetResult();
      if ( result )
      {
         if ( result->GetUserData() == this )
         {
            switch ( result->GetOperation() )
            {
               case ASMailFolder::Op_SearchMessages:
                  if ( m_searchData )
                  {
                     m_searchData->HandleSearchResult(*result);
                  }
                  else
                  {
                     FAIL_MSG( _T("search in progress but no search data?") );
                  }
                  break;

               default:
                  FAIL_MSG( _T("unexpected ASMailFolder::Result in wxMainFrame") );
            }
         }

         result->DecRef();
      }
   }

   // continue evaluating this event
   return true;
}

wxMainFrame::~wxMainFrame()
{
   // tell the app there is no main frame any more and do it as the very first
   // thing because the next calls may result in message boxes being displayed
   // and they shouldn't have this frame as a parent because it can go away at
   // any moment
   mApplication->OnMainFrameClose();

   delete m_FolderView;
   delete m_FolderTree;

   if ( m_searchData )
   {
      delete m_searchData;

      MEventManager::Deregister(m_cookieASMf);
   }
}

void
wxMainFrame::MakeModulesMenu(void)
{
   if ( !m_ModulesMenu )
   {
      wxMenuBar *menuBar = GetMenuBar();
      wxCHECK_RET( menuBar, _T("no menu bar in the main frame?") );

      // create the modules menu:
      m_ModulesMenu = new wxMenu(wxEmptyString, wxMENU_TEAROFF);

      int pos = menuBar->GetMenuCount();
      wxASSERT_MSG(pos > 1, _T("no menus in the main frame menubar?") );

      // and insert it just before the Help menu which is the last one
      pos--;

#ifdef DEBUG
      // (and before the debug menu which is the last one before "Help")
      pos--;
#endif // DEBUG

      menuBar->Insert(pos, m_ModulesMenu, _("&Tools"));
   }
}

/// Appends the menu for a module to the menubar
void
wxMainFrame::AddModulesMenu(const wxChar *name,
                            const wxChar *help,
                            class wxMenu *submenu,
                            int id)
{
   wxASSERT(submenu);
   MakeModulesMenu();
   if(id == -1)
      id = NewControlId();
   m_ModulesMenu->Append(id, name, submenu, help);
}


/// Appends the menu entry for a module to the modules menu
void
wxMainFrame::AddModulesMenu(const wxChar *name,
                            const wxChar *help,
                            int id)
{
   MakeModulesMenu();
   m_ModulesMenu->Append(id, name, help);
}

Profile *
wxMainFrame::GetFolderProfile(void) const
{
   // try the profile for the folder in the tree first: like this, the compose
   // command will use the settings of the currently selected folder and not
   // the one currently opened which is usually what you want
   //
   // this also allows to use the settings of a folder which you can't open now
   // (e.g. because you're offline)
   MFolder_obj folder(GetFolderTree()->GetSelection());
   if ( folder )
   {
      return Profile::CreateProfile(folder->GetFullName());
   }

   // no current selection but maybe an opened folder (weird, but who knows)?
   return m_FolderView ? m_FolderView->GetFolderProfile()
                       : wxMFrame::GetFolderProfile();
}

// ----------------------------------------------------------------------------
// UpdateFoldersSubtree() implementation
// ----------------------------------------------------------------------------

class UpdateFolderVisitor : public MFolderTraversal
{
public:
   UpdateFolderVisitor(const MFolder& folder, wxWindow *parent)
      : MFolderTraversal(folder),
        m_progInfo(parent, _("Folders updated:"), _("Updating folder tree"))
   {
      m_winParent = parent;
      m_nCount = 0;
   }

   virtual bool OnVisitFolder(const wxString& folderName)
   {
      MFolder_obj folder(folderName);
      CHECK( folder, false, _T("visiting folder which doesn't exist?") );

      if ( !folder->CanOpen() )
      {
         // skip this one and continue
         return true;
      }

      if ( !MailFolder::CheckFolder(folder) )
      {
         // ask the user whether we should continue with the others as they
         // could be inaccessible as well (if server is offline for example)
         return MDialog_YesNoDialog
                (
                  wxString::Format
                  (
                     _("Checking status of the folder \"%s\" failed, do you\n"
                       "want to continue updating the other folders?"),
                     folderName.c_str()
                  ),
                  m_winParent,
                  MDIALOG_YESNOTITLE,
                  M_DLG_NO_DEFAULT,
                  M_MSGBOX_CONT_UPDATE_AFTER_ERROR
                );
      }

      m_progInfo.SetValue(++m_nCount);

      return true;
   }

   size_t GetCountTraversed() const { return m_nCount; }

private:
   // the parent window for all dialogs &c
   wxWindow *m_winParent;

   // the progress indicator
   MProgressInfo m_progInfo;

   // the count of the folders traversed so far
   size_t m_nCount;
};

int UpdateFoldersSubtree(const MFolder& folder, wxWindow *parent)
{
   UpdateFolderVisitor visitor(folder, parent);

   (void)visitor.Traverse();

   return visitor.GetCountTraversed();
}

