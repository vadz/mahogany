/*-*- c++ -*-********************************************************
 * wxMFrame: a basic window class                                   *
 *                                                                  *
 * (C) 1997, 1998 by Karsten Ballüder (Ballueder@usa.net)           *
 *                                                                  *
 * $Id$             *
 *******************************************************************/

#ifdef __GNUG__
#   pragma implementation "wxMFrame.h"
#endif

#include "Mpch.h"
#include "Mcommon.h"

#ifndef  USE_PCH
#  include "guidef.h"
#  include "strutil.h"

#  include "MFrame.h"

#  include "kbList.h"

#  include "PathFinder.h"
#  include "MimeList.h"
#  include "MimeTypes.h"
#  include "Profile.h"
#  include "Mdefaults.h"
#  include "MApplication.h"
#  include "gui/wxMApp.h"
#endif

#include "Message.h"
#include "FolderView.h"
#include "MailFolder.h"
#include "MailFolderCC.h"

#include "MDialogs.h"

#include "gui/wxIconManager.h"

#include "adb/AdbFrame.h"

#include "gui/wxOptionsDlg.h"

// test:
#include   "SendMessageCC.h"
#include   "MailFolderCC.h"

#include   "gui/wxMFrame.h"
#include   "gui/wxComposeView.h"
#include   "gui/wxFolderView.h"

IMPLEMENT_DYNAMIC_CLASS(wxMFrame, wxFrame)

#ifdef  USE_WXWINDOWS2
   BEGIN_EVENT_TABLE(wxMFrame, wxFrame)
      EVT_MENU(-1,    wxMFrame::OnCommandEvent)
      EVT_TOOL(-1,    wxMFrame::OnCommandEvent)
   END_EVENT_TABLE()
#endif

bool wxMFrame::RestorePosition(const char *name,
                               int *x, int *y, int *w, int *h)
{
   wxCHECK( x && y && w && h, FALSE ); // no NULL pointers please

   ProfileBase *pConf = mApplication->GetProfile();
   if ( pConf != NULL )
   {
      ProfilePathChanger ppc(pConf, M_FRAMES_CONFIG_SECTION);
      pConf->SetPath(name);

      *x = READ_APPCONFIG(MC_XPOS);
      *y = READ_APPCONFIG(MC_YPOS);
      *w = READ_APPCONFIG(MC_WIDTH);
      *h = READ_APPCONFIG(MC_HEIGHT);

      return TRUE;
   }
   else {
      wxLogDebug("Can't restore frame '%s' position.", name);
      *x = MC_XPOS_D;
      *y = MC_YPOS_D;
      *w = MC_WIDTH_D;
      *h = MC_HEIGHT_D;

      return FALSE;
   }
}

wxMFrame::wxMFrame(const String &iname, wxWindow *parent)
        : MFrameBase(iname)
{
   initialised = false;
   Create(iname,parent);
}

void
wxMFrame::Create(const String &iname, wxWindow *parent)
{
   wxCHECK_RET( !initialised, "wxMFrame created twice" );

   SetName(Str(iname));

   const char *name = MFrameBase::GetName();
   int xpos, ypos, width, height;
   RestorePosition(name, &xpos, &ypos, &width, &height);

   // use name as default title
   wxFrame::CreateFrame(parent, name, xpos, ypos, width, height);
   //Show(true);

   SetIcon(ICON("MFrame"));

   initialised = true;
   m_MenuBar = new wxMenuBar;
   m_ToolBar = NULL;
}

void
wxMFrame::AddFileMenu(void)
{
   wxMenu *fileMenu = new wxMenu;
   AppendToMenu(fileMenu, WXMENU_FILE_BEGIN + 1, WXMENU_FILE_CLOSE - 1);

#ifdef USE_WXWINDOWS2
   wxWindow *parent = GetParent();
#endif

   if ( parent != NULL )
      AppendToMenu(fileMenu, WXMENU_FILE_CLOSE);
   AppendToMenu(fileMenu, WXMENU_FILE_CLOSE + 1, WXMENU_FILE_END);

   m_MenuBar->Append(fileMenu, _("&File"));
}

void
wxMFrame::AddEditMenu(void)
{
   WXADD_MENU(m_MenuBar, EDIT, "&Edit");
}

void
wxMFrame::AddHelpMenu(void)
{
   WXADD_MENU(m_MenuBar, HELP, "&Help");
}

void
wxMFrame::AddMessageMenu(void)
{
   WXADD_MENU(m_MenuBar, MSG, "&Message");
}

wxMFrame::~wxMFrame()
{
   SavePosition(MFrameBase::GetName(), this);
}

ON_CLOSE_TYPE wxMFrame::OnClose(void)
{
   // @@@ check that we have no unsaved data!
   return TRUE;
}

void
wxMFrame::SetTitle(String const &title)
{
   // the following (char *) is required to avoid a warning
   wxFrame::SetTitle((char *)title.c_str());
}

void
wxMFrame::SavePosition(const char *name, wxFrame *frame)
{
   int x,y;

   ProfileBase *pConf = mApplication->GetProfile();
   if ( pConf != NULL ) {
      ProfilePathChanger ppc(pConf, M_FRAMES_CONFIG_SECTION);
      pConf->SetPath(name);

      frame->GetPosition(&x,&y);
      pConf->writeEntry(MC_XPOS, x);
      pConf->writeEntry(MC_YPOS, y);

      frame->GetSize(&x,&y);
      pConf->writeEntry(MC_WIDTH, x);
      pConf->writeEntry(MC_HEIGHT, y);
   }
}

void
wxMFrame::OnMenuCommand(int id)
{
   switch(id)
   {
   case WXMENU_FILE_OPEN:
   {
      wxString name;
      if ( MInputBox(&name, _("Folder Open"), _("Name of the folder?"),
                     this, "OpenFolderName", "INBOX") ) {
         (void)new wxFolderViewFrame(Str(name), this);
      }
      break;
   }
   case WXMENU_FILE_CLOSE:
   {
      Close();
      break;
   }
   case WXMENU_FILE_CREATE:
   {
      MDialog_FolderCreate(this);
      break;
   }
   case WXMENU_FILE_COMPOSE:
   {
      (new wxComposeView("ComposeView", this))->Show();
      break;
   }

   case WXMENU_FILE_EXIT:
      mApplication->Exit();
      break;

   case WXMENU_EDIT_ADB:
      ShowAdbFrame(this);
      break;

   case WXMENU_EDIT_PREF:
      ShowOptionsDialog(this);
      break;

   case WXMENU_EDIT_SAVE_PREF:
      MDialog_Message(_("Not implemented yet."),this,_("Sorry"));
      break;
      
   case WXMENU_HELP_ABOUT:
      MDialog_AboutDialog(this, false /* don't timeout */);
      break;
      
   case WXMENU_HELP_HELP:
      MDialog_Message(_(ABOUTMESSAGE),this,_("About M"));
      break;
   }
}

#ifdef   USE_WXWINDOWS2
void
wxMFrame::OnCommandEvent(wxCommandEvent &event)
{
   OnMenuCommand(event.GetId());
}
#endif

