/*-*- c++ -*-********************************************************
 * wxMFrame: a basic window class                                   *
 *                                                                  *
 * (C) 1997, 1998 by Karsten Ballüder (Ballueder@usa.net)           *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.1  1998/03/14 12:21:22  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifdef __GNUG__
#pragma implementation "wxMFrame.h"
#endif

#include	<MApplication.h>
#include	<Mdefaults.h>

#include	<guidef.h>	
#include	<wxMFrame.h>
#include	<wxComposeView.h>
#include	<wxFolderView.h>

#include	<MFrame.xpm>
#include	<wxAdbEdit.h>
#include	<MDialogs.h>

// test:
#include	<SendMessageCC.h>
#include	<MailFolderCC.h>

IMPLEMENT_DYNAMIC_CLASS(wxMFrame, wxFrame)

wxMFrame::wxMFrame(const String &iname, wxFrame *parent)
{

   initialised = false;
   Create(iname,parent);
}

void
wxMFrame::Create(const String &iname, wxFrame *parent)
{
   int xpos, ypos, width, height;

   if(initialised)
   {
      INTERNALERROR((_("wxMFrame::Create() called on already initialised object.")));
      return; // ERROR!
   }
   
   SetName(iname);

   String tmp = mApplication.getCurrentPath();
   mApplication.setCurrentPath(M_FRAMES_CONFIG_SECTION);
   mApplication.changeCurrentPath(GetFrameName().c_str());
   xpos = mApplication.readEntry(MC_XPOS, MC_XPOS_D);
   ypos = mApplication.readEntry(MC_YPOS, MC_YPOS_D);
   width = mApplication.readEntry(MC_WIDTH, MC_WIDTH_D);
   height = mApplication.readEntry(MC_HEIGHT, MC_HEIGHT_D);
   mApplication.setCurrentPath(tmp.c_str());
   
   // use name as default title
   wxFrame::Create(parent, GetFrameName().c_str(), xpos, ypos, width, height);
   //Show(true);

   SetIcon(new wxIcon(MFrame_xpm));
   
   initialised = true;
}

void
wxMFrame::AddMenuBar(void)
{
   menuBar = new wxMenuBar;
}

void
wxMFrame::AddFileMenu(void)
{
   fileMenu = new wxMenu;

   fileMenu->Append(WXMENU_FILE_COMPOSE,(char *)_("&Compose"));
//   fileMenu->Append(WXMENU_FILE_TEST,(char *)_("&Test"));
   fileMenu->Append(WXMENU_FILE_OPEN,(char *)_("&Open"));
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

wxMFrame::~wxMFrame()
{
   SavePosition();
}

Bool
wxMFrame::OnClose(void)
{
   if(this == mApplication.TopLevelFrame())
   {
      mApplication.Exit();
      return FALSE;
   }
   else	// we can safely close this
      return TRUE;
}

void
wxMFrame::SetName(String const & iname)
{
   name = iname;
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

   String tmp = mApplication.getCurrentPath();
   mApplication.setCurrentPath(M_FRAMES_CONFIG_SECTION);
   mApplication.changeCurrentPath(GetFrameName().c_str());
   
   GetPosition(&x,&y);
   mApplication.writeEntry(MC_XPOS, (long int) x);
   mApplication.writeEntry(MC_YPOS, (long int) y);

   GetSize(&x,&y);
   mApplication.writeEntry(MC_WIDTH, (long int) x);
   mApplication.writeEntry(MC_HEIGHT, (long int) y);

   mApplication.setCurrentPath(tmp.c_str());
}

void
wxMFrame::OnMenuCommand(int id)
{
   switch(id)
   {
   case WXMENU_FILE_OPEN:
   {
      char *name = wxGetTextFromUser(
	 _("Name of the folder?"),
	 _("Folder Open"),
	 "INBOX",this);
      if(name)
      {
	 MailFolder *mf = new MailFolderCC(name);
	 if(mf->IsInitialised())
	    (new wxFolderView(mf, "FolderView", this))->Show();
	 else
	    delete mf;
      }
      break;
   }
   case WXMENU_FILE_ADBEDIT:
      (void) new wxAdbEditFrame(this);
      break;
   case WXMENU_FILE_CLOSE:
      OnClose();
      delete this;
      break;
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
