/*-*- c++ -*-********************************************************
 * wxMainFrame: the main window class                               *
 *                                                                  *
 * (C) 1998-1999 by Karsten Ballüder (Ballueder@usa.net)            *
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
#include <wx/menuitem.h>

#include "MFolder.h"

#include "gui/wxMainFrame.h"
#include "gui/wxMApp.h"
#include "gui/wxIconManager.h"
#include "gui/wxFolderView.h"
#include "gui/wxFolderTree.h"
#include "MFolderDialogs.h"      // for ShowFolderPropertiesDialog
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


// A popup menu for entering and configuring plugin modules
class wxModulePopup : public wxMenu
{
public:
   static wxModulePopup * Create(void);
   // callbacks
   void OnCommandEvent(wxCommandEvent &event);
   ~wxModulePopup()
      { SafeDecRef(m_Listing); }

private:
   wxModulePopup();
   MModuleListing *m_Listing;
   size_t m_CountModules, m_CountMain, m_CountConfig;
   DECLARE_EVENT_TABLE()
};


/* static */
wxModulePopup *
wxModulePopup::Create(void)
{
   wxModulePopup * popup = new wxModulePopup();
   if(popup->m_CountMain + popup->m_CountConfig == 0) // nothing to do
   {
      delete popup;
      return NULL;
   }
   return popup;
}


wxModulePopup::wxModulePopup()
   : wxMenu(_("Plugin Modules"))
{
   m_CountMain = 0;
   m_CountConfig = 0;

   m_Listing = MModule::ListLoadedModules();
   m_CountModules = m_Listing->Count();
   for(size_t i = 0; i < m_CountModules; i++)
   {
      MModule *mod = (*m_Listing)[i].GetModule();
      if(mod->Entry(MMOD_FUNC_GETFLAGS) & MMOD_FLAG_HASMAIN)
         m_CountMain++;
      if(mod->Entry(MMOD_FUNC_GETFLAGS) & MMOD_FLAG_HASCONFIG)
         m_CountConfig++;
      mod->DecRef();
   }

   if(m_CountMain+m_CountConfig == 0)
   {
      /// dummy menu:
      Append(WXMENU_POPUP_MODULES_OFFS,_("No modules to run or configure!"));
      Enable(WXMENU_POPUP_MODULES_OFFS, FALSE);
   }
   else
   {
      if(m_CountMain)
      {
         for(size_t i = 0; i < m_Listing->Count(); i++)
         {
            wxString entry;
            MModule *mod = (*m_Listing)[i].GetModule();
            if(mod->Entry(MMOD_FUNC_GETFLAGS) & MMOD_FLAG_HASMAIN)
            {
               entry = (*m_Listing)[i].GetName();
               entry << _(" module...");
               Append(WXMENU_POPUP_MODULES_OFFS+i, entry);
            }
            mod->DecRef();
         }
      }
      if(m_CountMain && m_CountConfig)
         AppendSeparator();
      if(m_CountConfig)
      {
         for(size_t i = 0; i < m_Listing->Count(); i++)
         {
            wxString entry;
            MModule *mod = (*m_Listing)[i].GetModule();
            if(mod->Entry(MMOD_FUNC_GETFLAGS) & MMOD_FLAG_HASCONFIG)
            {
               entry.Printf(_("Configure %s module..."),
                            (*m_Listing)[i].GetName().c_str());
               Append(WXMENU_POPUP_MODULES_OFFS+i+m_CountModules, entry);
            }
            mod->DecRef();
         }
      }
   }
}

void
wxModulePopup::OnCommandEvent(wxCommandEvent &event)
{
   if(event.GetId() < 0) // wxGTK menu title (how stupid!)
      return;

   size_t id = (size_t) event.GetId();

   id = id-WXMENU_POPUP_MODULES_OFFS;
   if(id < m_CountModules)
   {
      MModule *mod = (*m_Listing)[id].GetModule();
      mod->Entry(MMOD_FUNC_MAIN);
      mod->DecRef();
      return;
   }
   else
   {
      id -= m_CountModules;
      MModule *mod = (*m_Listing)[id].GetModule();
      mod->Entry(MMOD_FUNC_CONFIG);
      mod->DecRef();
      return;
   }
   ASSERT(0 /* unreachable code */);
}

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

   virtual void OnOpenHere(MFolder *folder)
   {
      // attention, base version of OnOpenHere() will DecRef() the folder, so
      // compensate for it
      SafeIncRef(folder);
      wxFolderTree::OnOpenHere(folder);
      m_frame->OpenFolder(folder);
   }

private:
   wxMainFrame *m_frame;
};

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxMainFrame, wxMFrame)
  EVT_MENU(-1,    wxMainFrame::OnCommandEvent)
  EVT_TOOL(-1,    wxMainFrame::OnCommandEvent)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxModulePopup, wxMenu)
  EVT_MENU(-1,    wxModulePopup::OnCommandEvent)
END_EVENT_TABLE()

// ============================================================================
// implementation
// ============================================================================

wxMainFrame::wxMainFrame(const String &iname, wxFrame *parent)
   : wxMFrame(iname,parent)
{
   m_ModulePopup = NULL;
   SetIcon(ICON("MainFrame"));
   SetTitle(M_TOPLEVELFRAME_TITLE);

   static int widths[3] = { -1, 70, 100 }; // FIXME: temporary for debugging    
   CreateStatusBar(3, wxST_SIZEGRIP, 12345); // 3 fields, id 12345 fo           
   GetStatusBar()->SetFieldsCount(3, widths);                                   
   
   AddFileMenu();
   AddEditMenu();

   // disable the operations which don't make sense for viewer
   wxMenuBar *menuBar = GetMenuBar();
   menuBar->Enable(WXMENU_EDIT_CUT, FALSE);
   menuBar->Enable(WXMENU_EDIT_PASTE, FALSE);

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
   m_FolderView = wxFolderView::Create(m_splitter);
   m_splitter->SplitVertically(m_FolderTree->GetWindow(),
                               m_FolderView->GetWindow(),
                               x/3);

#ifndef HAS_DYNAMIC_MENU_SUPPORT
   AddMessageMenu();
#endif

   // open the last folder in the main frame by default
   String foldername = READ_APPCONFIG(MP_MAINFOLDER);
   if ( !foldername.IsEmpty() )
   {
      MFolder *folder = MFolder::Get(foldername);
      if ( folder )
      {
         // make sure it doesn't go away after OpenFolder()
         folder->IncRef();
         OpenFolder(folder);
         m_FolderTree->SelectFolder(folder);
         folder->DecRef();
      }
   }

   // finish constructing the menu and toolbar
   AddHelpMenu();
   AddToolbarButtons(CreateToolBar(), WXFRAME_MAIN);

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


void
wxMainFrame::UpdateToolBar(void)
{
   if(m_ModulePopup) delete m_ModulePopup;
   m_ModulePopup = wxModulePopup::Create();
   if(! m_ModulePopup)
   {
      GetToolBar()->EnableTool(WXMENU_MODULES, FALSE);
      delete m_ModulePopup;
      m_ModulePopup = NULL;
   }
}

void
wxMainFrame::OpenFolder(MFolder *pFolder)
{
#ifdef HAS_DYNAMIC_MENU_SUPPORT
   static bool s_hasMsgMenu = false;
#endif

   MFolder_obj folder(pFolder);

   // don't do anything if there is nothing to change
   if ( folder && m_folderName == folder->GetFullName() )
   {
      // ... unless opening the folder previously failed, in which case we'll
      // try to reopen it (may be the user changed some of its settings)
      bool reopen = FALSE;

      int flags = folder->GetFlags();
      if ( flags & MF_FLAGS_MODIFIED )
      {
         // user changed some settings, try opening the folder again without
         // asking all sorts of dumb questions (like below)
         reopen = TRUE;
      }
      else if ( flags & MF_FLAGS_UNACCESSIBLE )
      {
         if ( MDialog_YesNoDialog(_("This folder couldn't be opened last time, "
                                    "do you still want to try to open it (it "
                                    "will probably fail again)?"),
                                  this,
                                  MDIALOG_YESNOTITLE,
                                  FALSE,
                                  "OpenUnaccessibleFolder") )
         {
            if ( MDialog_YesNoDialog(_("Would you like to change folder "
                                       "settings before trying to open it?"),
                                     this,
                                     MDIALOG_YESNOTITLE,
                                     FALSE,
                                     "ChangeUnaccessibleFolderSettings") )
            {
               // invoke the folder properties dialog
               (void)ShowFolderPropertiesDialog(folder, this);
            }

            reopen = TRUE;
         }
      }

      if ( !reopen )
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
      if ( mailFolder )
      {
         mailFolder->IncRef();
         // reset the unaccessible and modified flags this folder might have
         // had
         folder->ResetFlags(MF_FLAGS_MODIFIED | MF_FLAGS_UNACCESSIBLE);

         String statusMsg;
         statusMsg.Printf(_("Opened folder '%s'"), m_folderName.c_str());

         UpdateTitleAndStatusBars(m_folderName, statusMsg, this, mailFolder);
         mailFolder->DecRef();
      }
      else if ( mApplication->GetLastError() == M_ERROR_CANCEL )
      {
         // don't set the unaccessible flag - may be it's ok
         wxLogStatus(this, _("Opening folder '%s' cancelled."),
                     m_folderName.c_str());
      }
      else
      {
         // it's not modified any more...
         folder->ResetFlags(MF_FLAGS_MODIFIED);

         // ... and it is unacessible because we couldn't open it
         folder->AddFlags(MF_FLAGS_UNACCESSIBLE);
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
   // closing the main frame will close the app so ask the other frames
   // whether it's ok to close them
   bool rc = mApplication->CanClose();
   // make sure folder is closed before we close the window
   if(rc == TRUE)
      m_FolderView->SetFolder(NULL);
   return rc;
}

void
wxMainFrame::OnCommandEvent(wxCommandEvent &event)
{
   int id = event.GetId();


   if(id == WXMENU_MODULES)
   {
      if(m_ModulePopup)
         delete m_ModulePopup;
      m_ModulePopup = wxModulePopup::Create();
      if(m_ModulePopup)
      {
         int xpos, ypos;
         int fxpos=0, fypos=0;
         wxGetMousePosition(&xpos,&ypos);
         ClientToScreen(&fxpos,&fypos); // get client area offset
         PopupMenu(m_ModulePopup, xpos-fxpos, ypos-fypos);
      }
   }
   else if(m_FolderView &&
      (WXMENU_CONTAINS(MSG, id) || WXMENU_CONTAINS(LAYOUT, id)
       || id == WXMENU_FILE_COMPOSE || id == WXMENU_FILE_POST
       || id == WXMENU_EDIT_COPY ))
      m_FolderView->OnCommandEvent(event);
   else if(id == WXMENU_HELP_CONTEXT)
      mApplication->Help(MH_MAIN_FRAME,this);
   else if(id == WXMENU_EDIT_FILTERS)
   {
      ASMailFolder *amf = m_FolderView->GetFolder();
      if(amf)
         (void) ConfigureFilterRules(amf->GetProfile(), this);
      else
      {
         ERRORMESSAGE((_("You must select a folder first.")));
      }
   }
   else
      wxMFrame::OnMenuCommand(id);
}

wxMainFrame::~wxMainFrame()
{
   // tell the app there is no main frame any more and do it as the very first
   // thing because the next calls may result in message boxes being displayed
   // and they shouldn't have this frame as a parent because it can go away at
   // any moment
   mApplication->OnMainFrameClose();

   if(m_ModulePopup) delete m_ModulePopup;
   delete m_FolderView;
   delete m_FolderTree;
   // save the last opened folder
   mApplication->GetProfile()->writeEntry(MP_MAINFOLDER, m_folderName);
}
