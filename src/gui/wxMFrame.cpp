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
#  include "MLogFrame.h"

#  include "kbList.h"

#  include "PathFinder.h"
#  include "MimeList.h"
#  include "MimeTypes.h"
#  include "Profile.h"
#endif

#include "MApplication.h"
#include "gui/wxMApp.h"

// VZ: please don't change the order of headers, "Adb.h" must be the first one
//     or it doesn't compile under VC++ (don't yet know why @@@)
#include "Adb.h"

#include "Message.h"
#include "FolderView.h"
#include "MailFolder.h"
#include "MailFolderCC.h"

#include "MDialogs.h"

#include "Mdefaults.h"

// test:
#include   "SendMessageCC.h"
#include   "MailFolderCC.h"

#include   "gui/wxMFrame.h"
#include   "gui/wxComposeView.h"
#include   "gui/wxFolderView.h"
#include   "gui/wxAdbEdit.h"

#ifdef    OS_WIN
#  define   MFrame_xpm    "MFrame"
#else
#  include   "../src/icons/MFrame.xpm"
#endif

IMPLEMENT_DYNAMIC_CLASS(wxMFrame, wxFrame)

#ifdef  USE_WXWINDOWS2
   BEGIN_EVENT_TABLE(wxMFrame, wxFrame)
      EVT_MENU(-1,    wxMFrame::OnCommandEvent)
      EVT_TOOL(-1,    wxMFrame::OnCommandEvent)
   END_EVENT_TABLE()


void
wxMFrame::OnSize( wxSizeEvent &WXUNUSED(event) )
   
{
  int x,y;
  GetClientSize( &x, &y );
  if(m_ToolBar)
     m_ToolBar->SetSize( 1, 0, x-2, 30 );
};

#endif

wxMFrame::wxMFrame(const String &iname, wxWindow *parent)
        : MFrameBase(iname)
{
   initialised = false;
   Create(iname,parent);
}

void
wxMFrame::Create(const String &iname, wxWindow *parent)
{
   wxCHECK( !initialised );

   int xpos = MC_XPOS_D,
       ypos = MC_YPOS_D,
       width = MC_WIDTH_D,
       height = MC_HEIGHT_D;

   SetName(iname);

   FileConfig *pConf = Profile::GetAppConfig();
   if ( pConf != NULL )
   {
      String tmp = pConf->GET_PATH();
      pConf->SET_PATH(M_FRAMES_CONFIG_SECTION);
      pConf->CHANGE_PATH(MFrameBase::GetName());

      xpos = READ_APPCONFIG(MC_XPOS);
      ypos = READ_APPCONFIG(MC_YPOS);
      width = READ_APPCONFIG(MC_WIDTH);
      height = READ_APPCONFIG(MC_HEIGHT);

      pConf->SET_PATH(tmp.c_str());
   }
   
   // use name as default title
   wxFrame::CreateFrame(parent, MFrameBase::GetName(), xpos, ypos, width, height);
   //Show(true);

#if   defined(USE_WXWINDOWS2) && defined(USE_WXGTK)
   SetIcon(new wxIcon(MFrame_xpm,-1,-1));
#else
   SetIcon(new wxIcon(MFrame_xpm));
#endif
  
   initialised = true;
   menuBar = new wxMenuBar;
   m_ToolBar = NULL;
   SetMenuBar(menuBar);
}

void
wxMFrame::AddFileMenu(void)
{
   fileMenu = new wxMenu;

   fileMenu->Append(WXMENU_FILE_COMPOSE,(char *)_("&Compose Message"));
//   fileMenu->Append(WXMENU_FILE_TEST,(char *)_("&Test"));
   fileMenu->Append(WXMENU_FILE_OPEN,(char *)_("&Open Folder"));

#ifdef USE_WXWINDOWS2
   wxWindow *parent = GetParent();
#endif

   if(parent != NULL)
      fileMenu->Append(WXMENU_FILE_CLOSE,(char *)_("&Close Window"));
   fileMenu->AppendSeparator();
   fileMenu->Append(WXMENU_FILE_EXIT,(char *)_("&Exit"));

   menuBar->Append(fileMenu, (char *)_("&File"));
}

void
wxMFrame::AddEditMenu(void)
{
   m_EditMenu = new wxMenu;

   m_EditMenu->Append(WXMENU_EDIT_ADB,_("&Database"));
   m_EditMenu->Append(WXMENU_EDIT_PREFERENCES,(char
                                               *)_("&Preferences"));
   m_EditMenu->AppendSeparator();
   m_EditMenu->Append(WXMENU_EDIT_SAVE_PREFERENCES,(char *)_("&Save Preferences"));

   menuBar->Append(m_EditMenu, _("&Edit"));
}

void
wxMFrame::AddHelpMenu(void)
{
   helpMenu = new wxMenu;
   helpMenu->Append(WXMENU_HELP_ABOUT,(char *)_("&About"));

   menuBar->Append(helpMenu, (char *)_("&Help"));
}

void
wxMFrame::AddMessageMenu(void)
{
   wxMenu   *messageMenu;

   messageMenu = GLOBAL_NEW wxMenu;
   messageMenu->Append(WXMENU_MSG_OPEN, (char *)_("&Open"));
   messageMenu->Append(WXMENU_MSG_REPLY, (char *)_("&Reply"));
   messageMenu->Append(WXMENU_MSG_FORWARD, (char *)_("&Forward"));
   messageMenu->Append(WXMENU_MSG_DELETE,(char *)_("&Delete"));
   messageMenu->Append(WXMENU_MSG_SAVE,(char *)_("&Save"));
   messageMenu->Append(WXMENU_MSG_SELECTALL, (char *)_("Select &all"));
   messageMenu->Append(WXMENU_MSG_DESELECTALL, (char *)_("&Deselect all"));
   messageMenu->AppendSeparator();
   messageMenu->Append(WXMENU_MSG_EXPUNGE,(char *)_("Ex&punge"));

   menuBar->Append(messageMenu, _("&Message"));
}

void
wxMFrame::AddMenu(wxMenu *menu, String const &title)
{
   menuBar->Append(menu, _(title));
}

wxMFrame::~wxMFrame()
{
   SavePosition();
}

ON_CLOSE_TYPE wxMFrame::OnClose(void)
{
   if(this == mApplication.TopLevelFrame())
   {
      mApplication.Exit();
      return FALSE;
   }
   else   // we can safely close this
      return TRUE;
}

void
wxMFrame::SetTitle(String const &title)
{
   // the following (char *) is required to avoid a warning
   wxFrame::SetTitle((char *)title.c_str());
}

void
wxMFrame::SavePosition(void)
{
   int x,y;

   FileConfig *pConf = Profile::GetAppConfig();
   if ( pConf != NULL ) {
      String tmp = pConf->GET_PATH();
      pConf->SET_PATH(M_FRAMES_CONFIG_SECTION);
      pConf->CHANGE_PATH(MFrameBase::GetName());
   
      GetPosition(&x,&y);
      pConf->WRITE_ENTRY(MC_XPOS, (long int) x);
      pConf->WRITE_ENTRY(MC_YPOS, (long int) y);

      GetSize(&x,&y);
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
         new wxFolderView(name,this);
      break;
   }
   case WXMENU_FILE_CLOSE:
   {
      if(OnClose())
         delete this;
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
   case WXMENU_EDIT_PREFERENCES:
   case WXMENU_EDIT_SAVE_PREFERENCES:
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

