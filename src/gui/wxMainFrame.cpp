/*-*- c++ -*-********************************************************
 * wxMainFrame: the main window class                               *
 *                                                                  *
 * (C) 1998-2000 by Karsten Ballüder (Ballueder@gmx.net)            *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#ifdef __GNUG__
#   pragma   implementation "wxMainFrame.h"
#endif

#include   "Mpch.h"

#ifndef   USE_PCH
#   include   "Mcommon.h"
#   include   "guidef.h"
#   include   "MFrame.h"
#   include   "Mdefaults.h"
#   include   "strutil.h"
#endif

#include "wx/persctrl.h"
#include <wx/menuitem.h>

#include "MFolder.h"

#include "gui/wxMainFrame.h"
#include "gui/wxMApp.h"
#include "gui/wxIconManager.h"
#include "gui/wxFolderView.h"
#include "gui/wxFolderTree.h"

#include "gui/wxFiltersDialog.h" // for ConfigureFiltersForFolder
#include "gui/wxIdentityCombo.h" // for IDC_IDENT_COMBO
#include "MFolderDialogs.h"      // for ShowFolderCreateDialog

#include "miscutil.h"            // for UpdateTitleAndStatusBars

#include "MHelp.h"
#include "MDialogs.h"

#include "MModule.h"

// define this for future, less broken versions of wxWindows to dynamically
// insert/remove the message menu depending on whether we have or not a folder
// view in the frame - with current wxGTK it doesn't work at all, so disabling
#undef HAS_DYNAMIC_MENU_SUPPORT

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
         wxLogStatus(m_frame, _("Selected folder '%s'."),
                     newsel->GetFullName().c_str());

         UpdateMenu(m_frame->GetMenuBar()->GetMenu(MMenu_Folder), newsel);
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

   virtual bool OnClose(MFolder *folder)
   {
      if ( !wxFolderTree::OnClose(folder) )
         return false;

      m_frame->CloseFolder(folder);

      return true;
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

   virtual void SetFolder(MailFolder *mf, bool recreateFolderCtrl = TRUE)
   {
      if ( !mf )
         m_mainFrame->ClearFolderName();

      wxFolderView::SetFolder(mf, recreateFolderCtrl);
   }

private:
   wxMainFrame *m_mainFrame;
};

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxMainFrame, wxMFrame)
   EVT_MENU(-1,    wxMainFrame::OnCommandEvent)
   EVT_TOOL(-1,    wxMainFrame::OnCommandEvent)

   EVT_IDLE(wxMainFrame::OnIdle)

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
   SetIcon(ICON("MainFrame"));
   SetTitle(M_TOPLEVELFRAME_TITLE);

   static int widths[3] = { -1, 70, 100 }; // FIXME: temporary for debugging
   CreateStatusBar(3, wxST_SIZEGRIP, 12345); // 3 fields, id 12345 fo
   GetStatusBar()->SetFieldsCount(3, widths);

   // although we don't have a preview yet, the language menu is created
   // enabled by default, so setting this to true initially ensures that the
   // setting is in sync with the real menu state
   m_hasPreview = true;

   // construct the menu and toolbar
   AddFileMenu();
   AddFolderMenu();
   AddEditMenu();

#ifndef HAS_DYNAMIC_MENU_SUPPORT
   AddMessageMenu();
   AddLanguageMenu();
#endif

   AddHelpMenu();
   AddToolbarButtons(CreateToolBar(), WXFRAME_MAIN);

   // disable the operations which don't make sense for viewer
   wxMenuBar *menuBar = GetMenuBar();
   menuBar->Enable(WXMENU_EDIT_CUT, FALSE);
   menuBar->Enable(WXMENU_EDIT_PASTE, FALSE);

   m_ModulesMenu = NULL;

   int x,y;
   GetClientSize(&x, &y);

   m_splitter = new wxPSplitterWindow
                    (
                     "MainSplitter",
                     this,
                     -1,
                     wxPoint(1,31),       // FIXME what is this "31"?
                     wxSize(x-1,y-31),
                     wxSP_3D | wxSP_BORDER
                    );

   // insert treectrl in one of the splitter panes
   m_FolderTree = new wxMainFolderTree(m_splitter, this);
   m_FolderView = new wxMainFolderView(m_splitter, this);
   m_splitter->SplitVertically(m_FolderTree->GetWindow(),
                               m_FolderView->GetWindow(),
                               x/3);

   m_splitter->SetMinimumPaneSize(10);
   m_splitter->SetFocus();

   //FIXME: ugly workaround for wxGTK toolbar/resize bug
#ifdef __WXGTK__
   {
     int x,y;
     GetSize(&x,&y);
     SetSize(x,y+1);
     SetSize(x,y);
   }
#endif // GTK
}

void wxMainFrame::AddFolderMenu(void)
{
   WXADD_MENU(GetMenuBar(), FOLDER, _("&Folder"));
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
wxMainFrame::OpenFolder(MFolder *pFolder)
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
   else if ( m_folderName.IsEmpty() )
      return;
   else
      m_folderName.Empty();

   if ( folder )
   {
      // we want save the full folder name in m_folderName
      ASSERT( folder->GetFullName() == m_folderName );

      MailFolder *mailFolder = m_FolderView->OpenFolder(folder->GetFullName());
      if ( !mailFolder )
      {
         if ( mApplication->GetLastError() == M_ERROR_CANCEL )
         {
            // don't set the unaccessible flag - may be it's ok
            wxLogStatus(this, _("Opening folder '%s' cancelled."),
                        m_folderName.c_str());
         }

         m_folderName.clear();
      }

      m_FolderTree->SelectFolder(folder);
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
   // closing the main frame will close the app so ask the other frames
   // whether it's ok to close them
   bool rc = mApplication->CanClose();
   // make sure folder is closed before we close the window
   if(rc == TRUE)
      m_FolderView->SetFolder(NULL);
   return rc;
}

// ----------------------------------------------------------------------------
// callbacks
// ----------------------------------------------------------------------------

// we use OnIdle() and not OnUpdateUI() as a menu in the menubar doesn't have
// an id, unfortunately - at least in the current wxWin version
void wxMainFrame::OnIdle(wxIdleEvent &event)
{
   event.Skip();

   bool hasPreview = m_FolderView && m_FolderView->HasPreview();
   if ( hasPreview != m_hasPreview )
   {
      wxMenuBar *mbar = GetMenuBar();
      if ( !mbar )
         return;

      int idLangMenu = mbar->FindMenu(_("&Language"));
      if ( idLangMenu == wxNOT_FOUND )
         return;

      mbar->EnableTop(idLangMenu, hasPreview);

      // only change the internal status now, if we succeeded in
      // enabling/disabling the menu, otherwise it would get out of sync
      m_hasPreview = hasPreview;
   }
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
   int id = event.GetId();

   if ( WXMENU_CONTAINS(FOLDER, id) )
   {
      switch ( id )
      {
         case WXMENU_FOLDER_OPEN:
            MDialog_FolderOpen(this);
            break;

         case WXMENU_FOLDER_CREATE:
            {
               wxWindow *winTop = ((wxMApp *)mApplication)->GetTopWindow();
               bool wantsDialog;
               MFolder *newfolder = RunCreateFolderWizard(&wantsDialog, NULL, winTop);
               if ( wantsDialog )
               {
                  // users wants to use the dialog directly instead of the
                  // wizard
                  newfolder = ShowFolderCreateDialog(winTop,
                                                     FolderCreatePage_Default,
                                                     NULL);
               }
               SafeDecRef(newfolder);
            }
            break;

         case WXMENU_FOLDER_FILTERS:
            {
               MFolder_obj folder(m_FolderTree->GetSelection());
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

         default:
            // all others are processed by the folder tree
            m_FolderTree->ProcessMenuCommand(id);
      }
   }
   else if ( m_FolderView &&
            (WXMENU_CONTAINS(MSG, id) ||
             WXMENU_CONTAINS(LAYOUT, id) ||
             (WXMENU_CONTAINS(LANG, id) && (id != WXMENU_LANG_SET_DEFAULT)) ||
             id == WXMENU_FILE_COMPOSE ||
             id == WXMENU_FILE_POST ||
             id == WXMENU_EDIT_COPY ) )
   {
      m_FolderView->OnCommandEvent(event);
   }
   else if(id == WXMENU_HELP_CONTEXT)
   {
      mApplication->Help(MH_MAIN_FRAME,this);
   }
   else
   {
      wxMFrame::OnMenuCommand(id);
   }
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

   if ( READ_APPCONFIG(MP_REOPENLASTFOLDER) )
   {
      // save the last opened folder if we're going to reopen it
      mApplication->GetProfile()->writeEntry(MP_MAINFOLDER, m_folderName);
   }
}

void
wxMainFrame::MakeModulesMenu(void)
{
   wxMenuBar *menuBar = GetMenuBar();
   if(! m_ModulesMenu)
   {
      // create the modules menu:
      m_ModulesMenu = new wxMenu("", wxMENU_TEAROFF);
      int pos = menuBar->GetMenuCount();
      wxASSERT(pos  > 1);
      // pos is the Help menu, so insert at pos-1:
      menuBar->Insert(pos-1, m_ModulesMenu, _("&Plugins"));
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
wxMainFrame::GetFolderProfile(void)
{
   return m_FolderView ? m_FolderView->GetProfile() : NULL;
}

