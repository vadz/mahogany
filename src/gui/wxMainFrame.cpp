/*-*- c++ -*-********************************************************
 * wxMainFrame: the main window class                               *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
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

#include <wx/persctrl.h>

#include "MFolder.h"

#include "gui/wxMainFrame.h"
#include "gui/wxMApp.h"
#include "gui/wxIconManager.h"
#include "gui/wxFolderView.h"
#include "gui/wxFolderTree.h"

#include "MHelp.h"
#include "MDialogs.h"

// define this for future, less broken versions of wxWindows to dynamically
// insert/remove the message menu depending on whether we have or not a folder
// view in the frame - with current wxGTK it doesn't work at all, so disabling
#undef HAS_DYNAMIC_MENU_SUPPORT

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// override wxFolderTree OnOpenHere() function to open the folder in this
// frame
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
      }

      wxFolderTree::OnSelectionChange(oldsel, newsel);
   }

   virtual void OnOpenHere(MFolder *folder) { m_frame->OpenFolder(folder); }

private:
   wxMainFrame *m_frame;
};

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxMainFrame, wxMFrame)
  EVT_MENU(-1,    wxMainFrame::OnCommandEvent)
  EVT_TOOL(-1,    wxMainFrame::OnCommandEvent)
  EVT_CLOSE(wxMainFrame::OnCloseWindow)
END_EVENT_TABLE()

// ============================================================================
// implementation
// ============================================================================

wxMainFrame::wxMainFrame(const String &iname, wxFrame *parent)
   : wxMFrame(iname,parent)
{
   SetIcon(ICON("MainFrame"));

   AddFileMenu();
   AddEditMenu();

   CreateStatusBar();

   int x,y;
   GetClientSize(&x, &y);

   m_FolderView = NULL;
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

   // open the last folder in the main frame by default
   String foldername = READ_APPCONFIG(MP_MAINFOLDER);
   if ( !foldername.IsEmpty() && foldername[0u] != '/' )
      foldername.Prepend('/');
   bool hasFolderView = false;
   if ( !foldername.IsEmpty() )
   {
      MFolder *folder = MFolder::Get(foldername);
      if ( folder )
      {
         // make sure it doesn't go away after OpenFolder()
         folder->IncRef();
         hasFolderView = OpenFolder(folder);
         if ( hasFolderView )
            m_FolderTree->SelectFolder(folder);
         folder->DecRef();
      }
   }

   if ( !hasFolderView )
      m_splitter->Initialize(m_FolderTree->GetWindow());
   //else: already split in OpenFolder()

#ifndef HAS_DYNAMIC_MENU_SUPPORT
   AddMessageMenu();
#endif

   // finish constructing the menu and toolbar
   AddHelpMenu();
   SetMenuBar(m_MenuBar);

   m_ToolBar = CreateToolBar();
   AddToolbarButtons(m_ToolBar, WXFRAME_MAIN);

   m_splitter->SetMinimumPaneSize(0);
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

bool
wxMainFrame::OpenFolder(MFolder *folder)
{
#ifdef HAS_DYNAMIC_MENU_SUPPORT
   static bool s_hasMsgMenu = false;
#endif

   // don't do anything if there is nothing to change
   if ( folder && m_folderName == folder->GetFullName() )
   {
      folder->DecRef();

      return true;
   }
   else if ( folder )
      m_folderName = folder->GetFullName();
   else if ( m_folderName.IsEmpty() )
      return true;
   else
      m_folderName.Empty();

   wxWindow *winOldFolderView;
   if ( m_FolderView )
   {
      winOldFolderView = m_FolderView->GetWindow();

      delete m_FolderView;
      m_FolderView = NULL;
   }
   else
   {
      winOldFolderView = NULL;
   }

   bool hasFolder = true;
   if ( folder )
   {
      m_FolderView = wxFolderView::Create(m_folderName, m_splitter);

      if ( m_FolderView && m_FolderView->IsOk() )
      {
         if ( winOldFolderView )
         {
            m_splitter->ReplaceWindow(winOldFolderView,
                                      m_FolderView->GetWindow());
         }
         else
         {
            m_splitter->SplitVertically(m_FolderTree->GetWindow(),
                                        m_FolderView->GetWindow());
         }

#ifdef HAS_DYNAMIC_MENU_SUPPORT
         // only add the msg menu once
         if ( !s_hasMsgMenu )
         {
            AddMessageMenu();
            s_hasMsgMenu = true;
         }
#endif // HAS_DYNAMIC_MENU_SUPPORT
      }
      else
      {
         hasFolder = false;

         delete m_FolderView;
      }

      folder->DecRef();
   }
   else
   {
      if ( m_splitter->IsSplit() )
         m_splitter->Unsplit();
   }

   if ( !hasFolder )
   {
#ifdef HAS_DYNAMIC_MENU_SUPPORT
      // TODO remove the message menu - wxMenuBar::Delete() not implemented
      //      currently in wxGTK and is somewhat broken in wxMSW
#endif // HAS_DYNAMIC_MENU_SUPPORT

      // no folder view
      if ( m_splitter->IsSplit() )
         m_splitter->Unsplit();
   }

   return hasFolder;
}

void
wxMainFrame::OnCloseWindow(wxCloseEvent&)
{
   // ask the user unless disabled
   if ( MDialog_YesNoDialog(_("Do you really want to exit M?"), this,
                            MDIALOG_YESNOTITLE, false,
                            MP_CONFIRMEXIT) ) {
      delete m_FolderTree;
      delete m_FolderView;

      mApplication->OnMainFrameClose();

      Destroy();
   }
}

void
wxMainFrame::OnCommandEvent(wxCommandEvent &event)
{
   int id = event.GetId();

   if(m_FolderView &&
      (WXMENU_CONTAINS(MSG, id) || WXMENU_CONTAINS(LAYOUT, id)))
      m_FolderView->OnCommandEvent(event);
   else if(id == WXMENU_HELP_CONTEXT)
      mApplication->Help(MH_MAIN_FRAME,this);
   else
      wxMFrame::OnMenuCommand(id);
}

wxMainFrame::~wxMainFrame()
{
   // save the last opened folder
   mApplication->GetProfile()->writeEntry(MP_MAINFOLDER, m_folderName);
}
