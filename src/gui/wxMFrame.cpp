/*-*- c++ -*-********************************************************
 * wxMFrame: a basic window class                                   *
 *                                                                  *
 * (C) 1997, 1998 by Karsten Ballüder (Ballueder@usa.net)           *
 *                                                                  *
 * $Id$              *
 ********************************************************************
 * $Log$
 * Revision 1.11  1998/06/14 12:24:21  KB
 * started to move wxFolderView to be a panel, Python improvements
 *
 * Revision 1.10  1998/06/09 13:42:28  VZ
 *
 * corrected the line which checks that wxGetTextFromUser() returns something
 * (i.e. wasn't cancelled)
 *
 * Revision 1.9  1998/06/05 16:56:26  VZ
 *
 * many changes among which:
 *  1) AppBase class is now the same to MApplication as FrameBase to wxMFrame,
 *     i.e. there is wxMApp inheriting from AppBse and wxApp
 *  2) wxMLogFrame changed (but will probably change again because I wrote a
 *     generic wxLogFrame for wxWin2 - we can as well use it instead)
 *  3) Profile stuff simplified (but still seems to work :-), at least with
 *     wxConfig), no more AppProfile separate class.
 *  4) wxTab "#ifdef USE_WXWINDOWS2"'d out in wxAdbEdit.cc because not only
 *     it doesn't work with wxWin2, but also breaks wxClassInfo::Initialize
 *     Classes
 *  5) wxFTCanvas tweaked and now _almost_ works (but not quite)
 *  6) constraints in wxComposeView changed to work under both wxGTK and
 *     wxMSW (but there is an annoying warning about unsatisfied constraints
 *     coming from I don't know where)
 *  7) some more wxWin2 specific things corrected to avoid (some) crashes.
 *  8) many other minor changes I completely forgot about.
 *
 * Revision 1.8  1998/05/24 14:48:16  KB
 * lots of progress on Python, but cannot call functions yet
 * kbList fixes again?
 *
 * Revision 1.7  1998/05/18 17:48:44  KB
 * more list<>->kbList changes, fixes for wxXt, improved makefiles
 *
 * Revision 1.6  1998/05/12 12:19:03  VZ
 *
 * fixes to Windows fixes. Compiles under wxGTK if you #define USE_APPCONF.
 *
 * Revision 1.5  1998/05/11 20:57:33  VZ
 * compiles again under Windows + new compile option USE_WXCONFIG
 *
 * Revision 1.4  1998/04/30 19:12:35  VZ
 * (minor) changes needed to make it compile with wxGTK
 *
 * Revision 1.3  1998/04/26 16:27:17  KB
 * changed configure for wxGTK support (not compiling cleanly yet)
 * did some work on scripting (rudimentary)
 *
 * Revision 1.2  1998/03/26 23:05:41  VZ
 * Necessary changes to make it compile under Windows (VC++ only)
 * Header reorganization to be able to use precompiled headers
 *
 * Revision 1.1  1998/03/14 12:21:22  karsten
 * first try at a complete archive
 *
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
      EVT_MENU(WXMENU_FILE_OPEN,    wxMFrame::OnOpen)
      EVT_MENU(WXMENU_FILE_ADBEDIT, wxMFrame::OnAdbEdit)
      EVT_MENU(WXMENU_FILE_CLOSE,   wxMFrame::OnMenuClose)
      EVT_MENU(WXMENU_FILE_COMPOSE, wxMFrame::OnCompose)
      EVT_MENU(WXMENU_FILE_EXIT,    wxMFrame::OnExit)
      EVT_MENU(WXMENU_HELP_ABOUT,   wxMFrame::OnAbout)
   END_EVENT_TABLE()
#endif

wxMFrame::wxMFrame(const String &iname, wxFrame *parent)
        : MFrameBase(iname)
{
   initialised = false;
   Create(iname,parent);
}

void
wxMFrame::Create(const String &iname, wxFrame *parent)
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
   SetMenuBar(menuBar);
}

void
wxMFrame::AddFileMenu(void)
{
   fileMenu = new wxMenu;

   fileMenu->Append(WXMENU_FILE_COMPOSE,(char *)_("&Compose"));
//   fileMenu->Append(WXMENU_FILE_TEST,(char *)_("&Test"));
   fileMenu->Append(WXMENU_FILE_OPEN,(char *)_("&Open"));

#ifdef USE_WXWINDOWS2
   wxWindow *parent = GetParent();
#endif

   if(parent != NULL)
      fileMenu->Append(WXMENU_FILE_CLOSE,(char *)_("&Close"));
   fileMenu->Append(WXMENU_FILE_ADBEDIT, (char *)_("edit &Database"));
   fileMenu->AppendSeparator();
   fileMenu->Append(WXMENU_FILE_EXIT,(char *)_("&Exit"));

   menuBar->Append(fileMenu, (char *)_("&File"));
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
wxMFrame::OnCommandEvent(wxCommandEvent &event)
{
   OnMenuCommand(event.GetId());
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
   case WXMENU_FILE_ADBEDIT:
      (void) new wxAdbEditFrame(this);
      break;
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
   case WXMENU_HELP_ABOUT:
      MDialog_Message(_(ABOUTMESSAGE),this,_("About M"));
      break;
   }
}
