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

// VZ: please don't change the order of headers, "Adb.h" must be the first one
//     or it doesn't compile under VC++ (don't yet know why @@@)
#include "Adb.h"

#include "Message.h"
#include "FolderView.h"
#include "MailFolder.h"
#include "MailFolderCC.h"

#include "MDialogs.h"

#include "gui/wxIconManager.h"

// test:
#include   "SendMessageCC.h"
#include   "MailFolderCC.h"

#include   "gui/wxMFrame.h"
#include   "gui/wxComposeView.h"
#include   "gui/wxFolderView.h"
#include   "gui/wxAdbEdit.h"

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

   FileConfig *pConf = Profile::GetAppConfig();
   if ( pConf != NULL )
   {
      String oldPath = Str(pConf->GET_PATH());
      pConf->SET_PATH(M_FRAMES_CONFIG_SECTION);
      pConf->CHANGE_PATH(name);

      *x = READ_APPCONFIG(MC_XPOS);
      *y = READ_APPCONFIG(MC_YPOS);
      *w = READ_APPCONFIG(MC_WIDTH);
      *h = READ_APPCONFIG(MC_HEIGHT);

      pConf->SET_PATH(oldPath.c_str());

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

   FileConfig *pConf = Profile::GetAppConfig();
   if ( pConf != NULL ) {
      String tmp = Str(pConf->GET_PATH());
      pConf->SET_PATH(M_FRAMES_CONFIG_SECTION);
      pConf->CHANGE_PATH(name);

      frame->GetPosition(&x,&y);
      pConf->WRITE_ENTRY(MC_XPOS, (long int) x);
      pConf->WRITE_ENTRY(MC_YPOS, (long int) y);

      frame->GetSize(&x,&y);
      pConf->WRITE_ENTRY(MC_WIDTH, (long int) x);
      pConf->WRITE_ENTRY(MC_HEIGHT, (long int) y);

      pConf->SET_PATH(tmp.c_str());
   }
}

void
wxMFrame::OnMenuCommand(int id)
{
   switch(id)
   {
   case WXMENU_FILE_OPEN:
   {
#ifdef USE_WXWINDOWS2
      wxString
#else
         char *
#endif
         name = wxGetTextFromUser(_("Name of the folder?"),
                                  _("Folder Open"),
                                  "INBOX",
                                  this);
      VAR(name);
      if ( !strutil_isempty(name) )
         (void) new wxFolderViewFrame(Str(name),this);
      break;
   }
   case WXMENU_FILE_CLOSE:
   {
      //if(OnClose())
      //   Destroy();
      Close();
      break;
   }
   case WXMENU_FILE_COMPOSE:
   {
      (new wxComposeView("ComposeView", this))->Show();
      break;
   }
   case WXMENU_FILE_EXIT:
      mApplication.Exit();
      break;
   case WXMENU_EDIT_ADB:
      (void) new wxAdbEditFrame(this);
      break;
   case WXMENU_EDIT_PREF:
   case WXMENU_EDIT_SAVE_PREF:
      MDialog_Message(_("Not implemented yet."),this,_("Sorry"));
      break;
   case WXMENU_HELP_ABOUT:
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

