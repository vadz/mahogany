///////////////////////////////////////////////////////////////////////////////
// Project:     Mahogany
// File name:   gui/wxMainFrame.cpp - main frame implementation
// Purpose:     implement the GUI for the main program window
// Author:      M-Team
// Modified by:
// Created:     1998
// CVS-ID:      $Id$
// Copyright:   (c) 1998-2001 Mahogany Team
// License:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#ifdef __GNUG__
#   pragma   implementation "wxMainFrame.h"
#endif

#include "Mpch.h"

#ifndef   USE_PCH
#  include "Mcommon.h"
#  include "guidef.h"
#  include "MFrame.h"
#  include "Mdefaults.h"
#  include "strutil.h"

#  include <wx/menu.h>
#  include <wx/dirdlg.h>
#  include <wx/statusbr.h>
#  include <wx/toolbar.h>
#endif

#include "wx/persctrl.h"
#include <wx/menuitem.h>

#include "MFolder.h"

// these 3 are needed only for SEARCH command processing
#include "MSearch.h"
#include "UIdArray.h"
#include "HeaderInfo.h"

#include "gui/wxMainFrame.h"
#include "gui/wxMApp.h"
#include "gui/wxIconManager.h"
#include "gui/wxFolderView.h"
#include "gui/wxFolderTree.h"

#include "gui/wxFiltersDialog.h" // for ConfigureFiltersForFolder
#include "gui/wxIdentityCombo.h" // for IDC_IDENT_COMBO
#include "MFolderDialogs.h"      // for ShowFolderCreateDialog

#include "MHelp.h"
#include "MDialogs.h"

#include "MModule.h"

// define this for future, less broken versions of wxWindows to dynamically
// insert/remove the message menu depending on whether we have or not a folder
// view in the frame - with current wxGTK it doesn't work at all, so disabling
#undef HAS_DYNAMIC_MENU_SUPPORT

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

#ifdef DEBUG

enum
{
   WXMENU_DEBUG_WIZARD = WXMENU_DEBUG_BEGIN + 1,
   WXMENU_DEBUG_TOGGLE_LOG
};

#endif // DEBUG

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_CURRENT_IDENTITY;
extern const MOption MP_FVIEW_AUTONEXT_UNREAD_FOLDER;
extern const MOption MP_MAINFOLDER;
extern const MOption MP_OPENFOLDERS;
extern const MOption MP_REOPENLASTFOLDER;

// ----------------------------------------------------------------------------
// persistent msgboxes we use here
// ----------------------------------------------------------------------------

extern const MPersMsgBox *M_MSGBOX_SEARCH_AGAIN_IF_NO_MATCH;

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// update the status of all folders under the given one in the folder tree,
// returns the number of the folders updated or -1 on error
static int UpdateFoldersSubtree(const MFolder& folder, wxWindow *parent);

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
      // attention, base version of OnOpenHere() will DecRef() the folder, so
      // compensate for it
      SafeIncRef(folder);
      wxFolderTree::OnOpenHere(folder);

      m_frame->OpenFolder(folder);
   }

   virtual void OnView(MFolder *folder)
   {
      SafeIncRef(folder);
      wxFolderTree::OnView(folder);

      m_frame->OpenFolder(folder, true /* RO */);
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
      MFolder *folder = m_mainFrame->GetFolderTree()->FindNextUnreadFolder();
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
                     HeaderInfoList_obj hil = mf->GetHeaders();
                     if ( hil )
                     {
                        size_t nMatches = 0;

                        size_t count = uidsMatching->GetCount();
                        for ( size_t n = 0; n < count; n++ )
                        {
                           Message_obj msg = mf->GetMessage((*uidsMatching)[n]);
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
            m_folderVirt->SetPath(String::Format("(%u)", ++s_countSearch));

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

   EVT_UPDATE_UI(WXMENU_EDIT_FIND, wxMainFrame::OnUpdateUIEnableIfHasPreview)
   EVT_UPDATE_UI(WXMENU_EDIT_FINDAGAIN, wxMainFrame::OnUpdateUIEnableIfHasPreview)

   EVT_CHOICE(IDC_IDENT_COMBO, wxMainFrame::OnIdentChange)
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

   // set frame icon/title, create status bar
   SetIcon(ICON("MainFrame"));
   SetTitle(_("Copyright (C) 1997-2003 The Mahogany Developers Team"));

   CreateStatusBar();

   // create the child controls
   m_splitter = new wxPSplitterWindow("MainSplitter", this, -1,
                                      wxDefaultPosition, wxDefaultSize,
                                      0);

   wxSize sizeFrame = GetClientSize();
   m_splitter->SetSize(sizeFrame);

   // insert treectrl in one of the splitter panes
   m_FolderTree = new wxMainFolderTree(m_splitter, this);
   m_FolderView = new wxMainFolderView(m_splitter, this);
   m_splitter->SplitVertically(m_FolderTree->GetWindow(),
                               m_FolderView->GetWindow(),
                               sizeFrame.x/3);

   m_splitter->SetMinimumPaneSize(10);

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
   menuDebug->AppendCheckItem(WXMENU_DEBUG_TOGGLE_LOG,
                              _T("Toggle &debug logging\tCtrl-Alt-D"));
   GetMenuBar()->Append(menuDebug, "&Debug");
#endif // debug

   AddHelpMenu();

   CreateMToolbar(this, WXFRAME_MAIN);

   // disable the operations which don't make sense for viewer
   wxMenuBar *menuBar = GetMenuBar();
   menuBar->Enable(WXMENU_EDIT_CUT, FALSE);
   menuBar->Enable(WXMENU_EDIT_PASTE, FALSE);

   m_ModulesMenu = NULL;

   // update the menu to match the initial selection
   MFolder_obj folder = m_FolderTree->GetSelection();
   if ( folder )
   {
      UpdateFolderMenuUI(folder);
   }

   m_FolderTree->GetWindow()->SetFocus();
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
   if ( folder && folder->GetFullName() == m_folderName )
   {
      m_FolderView->SetFolder(NULL);

      //m_folderName.clear(); -- now done in our ClearFolderName()
   }
   //else: otherwise, we don't have it opened
}

void
wxMainFrame::OpenFolder(MFolder *pFolder, bool readonly)
{
#ifdef HAS_DYNAMIC_MENU_SUPPORT
   static bool s_hasMsgMenu = false;
#endif

   MFolder_obj folder(pFolder);

   // don't do anything if there is nothing to change
   if ( folder.IsOk() && (m_folderName == folder->GetFullName()) )
   {
      wxLogStatus(this, _("The folder '%s' is already opened."),
                  m_folderName.c_str());

      return;
   }
   else if ( folder )
      m_folderName = folder->GetFullName();
   else if ( m_folderName.empty() )
      return;
   else
      m_folderName.Empty();

   if ( folder )
   {
      // we want save the full folder name in m_folderName
      ASSERT( folder->GetFullName() == m_folderName );

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
      else // select the folder on screen as well
      {
         m_FolderTree->SelectFolder(folder);
      }
  }
#ifdef HAS_DYNAMIC_MENU_SUPPORT
   // only add the msg menu once
   if ( !s_hasMsgMenu )
   {
      AddMessageMenu();
      s_hasMsgMenu = true;
   }
#endif // HAS_DYNAMIC_MENU_SUPPORT

#ifdef HAS_DYNAMIC_MENU_SUPPORT
      // TODO remove the message menu - wxMenuBar::Delete() not implemented
      //      currently in wxGTK and is somewhat broken in wxMSW
#endif // HAS_DYNAMIC_MENU_SUPPORT
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

   wxMenuBar *mbar = GetMenuBar();
   if ( !mbar )
      return;

   // although we don't have a preview yet, the menus are created enabled by
   // default, so setting this to true initially ensures that the setting is in
   // sync with the real menu state
   static bool s_hasPreview = true;
   static bool s_hasFolder = true;

   bool hasFolder = m_FolderView && m_FolderView->GetFolder();
   if ( hasFolder != s_hasFolder )
   {
      EnableMMenu(MMenu_Message, this, hasFolder);
      EnableMMenu(MMenu_View, this, hasFolder);

      // also update the toolbar buttons
      static const int buttonsToDisable[] =
      {
         WXTBAR_MSG_NEXT_UNREAD,
         WXTBAR_MSG_OPEN,
         WXTBAR_MSG_REPLY,
         WXTBAR_MSG_FORWARD,
         WXTBAR_MSG_PRINT,
         WXTBAR_MSG_DELETE,
      };

      wxToolBar *tbar = GetToolBar();
      for ( size_t n = 0; n < WXSIZEOF(buttonsToDisable); n++ )
      {
         EnableToolbarButton(tbar, buttonsToDisable[n], hasFolder);
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

   if ( WXMENU_CONTAINS(FOLDER, id) )
   {
      switch ( id )
      {
         case WXMENU_FOLDER_OPEN:
         case WXMENU_FOLDER_OPEN_RO:
            {
               MFolder_obj folder = MDialog_FolderChoose
                                    (
                                       this, // parent window
                                       NULL, // parent folder
                                       true  // open
                                    );
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
               MFolder_obj parent = m_FolderTree->GetSelection();

               wxWindow *winTop = ((wxMApp *)mApplication)->GetTopWindow();
               bool wantsDialog;
               MFolder *newfolder = RunCreateFolderWizard(&wantsDialog,
                                                          parent,
                                                          winTop);
               if ( wantsDialog )
               {
                  // users wants to use the dialog directly instead of the
                  // wizard
                  newfolder = ShowFolderCreateDialog(winTop,
                                                     FolderCreatePage_Default,
                                                     parent);
               }

               SafeDecRef(newfolder);
            }
            break;

         case WXMENU_FOLDER_SEARCH:
            DoFolderSearch();
            break;

         case WXMENU_FOLDER_FILTERS:
            {
               MFolder_obj folder = m_FolderTree->GetSelection();
               if ( folder )
                  ConfigureFiltersForFolder(folder, this);
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
                     folder = MFolder::Get("");
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
               MFolder_obj folder = m_FolderTree->GetSelection();
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
                  folder = MFolder::Get("");

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
            ( WXMENU_CONTAINS(MSG, id) ||
              WXMENU_CONTAINS(LAYOUT, id) ||
              ( WXMENU_CONTAINS(LANG, id) && (id != WXMENU_LANG_SET_DEFAULT) ) ||
             id == WXMENU_EDIT_COPY ||
             id == WXMENU_EDIT_FIND ||
             id == WXMENU_EDIT_FINDAGAIN ||
             WXMENU_CONTAINS(VIEW, id) ||
             WXMENU_CONTAINS(VIEW_FILTERS, id)) )
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

         case WXMENU_DEBUG_TOGGLE_LOG:
            extern bool g_debugMailForceOn;

            g_debugMailForceOn = event.IsChecked();
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

void wxMainFrame::DoFolderSearch()
{
   SearchCriterium crit;

   Profile_obj profile(GetFolderProfile());
   MFolder_obj folder = m_FolderTree->GetSelection();
   if ( ConfigureSearchMessages(&crit, profile, folder, this) )
   {
      AsyncSearchData *searchData = NULL;

      const wxArrayString& folderNames = crit.m_Folders;
      size_t count = folderNames.GetCount();
      for ( size_t n = 0; n < count; n++ )
      {
         const String& name = folderNames[n];

         MFolder_obj folder = name;
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
      m_ModulesMenu = new wxMenu(_T(""), wxMENU_TEAROFF);

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
wxMainFrame::AddModulesMenu(const char *name,
                            const char *help,
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
wxMainFrame::AddModulesMenu(const char *name,
                            const char *help,
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
   MFolder_obj folder = GetFolderTree()->GetSelection();
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
         // stop on error as chances are that the other ones could follow
         return false;
      }

      m_progInfo.SetValue(++m_nCount);

      return true;
   }

   size_t GetCountTraversed() const { return m_nCount; }

private:
   // the progress indicator
   MProgressInfo m_progInfo;

   // the count of the folders traversed so far
   size_t m_nCount;
};

static int UpdateFoldersSubtree(const MFolder& folder, wxWindow *parent)
{
   UpdateFolderVisitor visitor(folder, parent);

   (void)visitor.Traverse();

   return visitor.GetCountTraversed();
}

