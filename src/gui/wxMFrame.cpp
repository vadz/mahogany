/*-*- c++ -*-********************************************************
 * wxMFrame: a basic window class                                   *
 *                                                                  *
 * (C) 1997, 1998 by Karsten Ballüder (Ballueder@usa.net)           *
 *                                                                  *
 * $Id$
 *******************************************************************/

#ifdef __GNUG__
#   pragma implementation "wxMFrame.h"
#endif

#include "Mpch.h"
#include "Mcommon.h"

#ifndef  USE_PCH
#   include "guidef.h"
#   include "strutil.h"
#   include "MFrame.h"
#   include "kbList.h"
#   include "PathFinder.h"
#   include "MimeList.h"
#   include "MimeTypes.h"
#   include "Profile.h"
#   include "Mdefaults.h"
#   include "MApplication.h"
#   include "gui/wxMApp.h"
#   include "MailFolder.h"
#   include "Message.h"

#   include "wx/confbase.h"
#endif

#ifdef USE_PYTHON
#  include <Python.h>
#  include "PythonHelp.h"
#endif // Python

#include "MHelp.h"
#include "MFolder.h"

#include "FolderView.h"
#include "MDialogs.h"

#include "MFolderDialogs.h"

#include "gui/wxIconManager.h"
#include "gui/wxOptionsDlg.h"
#include "adb/AdbFrame.h"
#include "gui/wxMFrame.h"
#include "gui/wxComposeView.h"
#include "gui/wxFolderView.h"

IMPLEMENT_DYNAMIC_CLASS(wxMFrame, wxFrame)

BEGIN_EVENT_TABLE(wxMFrame, wxFrame)
   EVT_MENU(-1,    wxMFrame::OnCommandEvent)
   EVT_TOOL(-1,    wxMFrame::OnCommandEvent)
   EVT_CLOSE(wxMFrame::OnCloseWindow)
END_EVENT_TABLE()

#ifdef OS_WIN
    // ok, wxPrintDialog is the right one
#else
    typedef wxGenericPrintDialog wxPrintDialog;
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

      *x = READ_APPCONFIG(MP_XPOS);
      *y = READ_APPCONFIG(MP_YPOS);
      *w = READ_APPCONFIG(MP_WIDTH);
      *h = READ_APPCONFIG(MP_HEIGHT);

      return TRUE;
   }
   else {
      wxLogDebug("Can't restore frame '%s' position.", name);
      *x = MP_XPOS_D;
      *y = MP_YPOS_D;
      *w = MP_WIDTH_D;
      *h = MP_HEIGHT_D;

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
   // +2 because WXMENU_FILE_CLOSE has a separator with it
   AppendToMenu(fileMenu, WXMENU_FILE_CLOSE + 2, WXMENU_FILE_END);

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

void
wxMFrame::SetTitle(String const &title)
{
   // the following (char *) is required to avoid a warning
   wxFrame::SetTitle((char *)title.c_str());
}

void
wxMFrame::SavePosition(const char *name, wxWindow *frame)
{
   int x,y;

   ProfileBase *pConf = mApplication->GetProfile();
   if ( pConf != NULL ) {
      ProfilePathChanger ppc(pConf, M_FRAMES_CONFIG_SECTION);
      pConf->SetPath(name);

      frame->GetPosition(&x,&y);
      pConf->writeEntry(MP_XPOS, x);
      pConf->writeEntry(MP_YPOS, y);

      frame->GetSize(&x,&y);
      pConf->writeEntry(MP_WIDTH, x);
      pConf->writeEntry(MP_HEIGHT, y);
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
                        this, "OpenFolderName", "INBOX") )
         {
            (void) wxFolderViewFrame::Create(name, this);
         }
      }
      break;

   case WXMENU_FILE_OPENANY:
      MDialog_FolderOpen(this);
      break;

   case WXMENU_FILE_CLOSE:
      Close();
      break;

   case WXMENU_FILE_CREATE:
      {
         MFolder *folder = ShowFolderCreateDialog(this);
         if ( folder )
            folder->DecRef();
      }
      break;

   case WXMENU_FILE_COMPOSE:
      {
         wxComposeView *composeView = new wxComposeView("ComposeView", this);
         composeView->Show();
      }
      break;

#ifdef USE_PYTHON
   case WXMENU_FILE_SCRIPT:
   {
      String
         path,
         filename;

      path << mApplication->GetGlobalDir() << DIR_SEPARATOR << "scripts";
      filename = MDialog_FileRequester("Please select a Python script to run.",
                                       this,
                                       path, filename,
                                       "py", "*.py?",
                                       false,
                                       NULL /* profile */);
      if(! strutil_isempty(filename))
      {
         FILE *file = fopen(filename,"rb");
         if(file)
         {
            PyH_RunScript(file,filename);
            fclose(file);
         }
         break;
      }
      //else: cancelled by user
   }
#endif   // USE_PYTHON

   case WXMENU_FILE_EXIT:
      // first decide if it's ok to close this frame
      if ( CanClose() )
         mApplication->Exit();
      break;

   case WXMENU_EDIT_ADB:
      ShowAdbFrame(this);
      break;

   case WXMENU_EDIT_PREF:
      ShowOptionsDialog(this);
      break;

   case WXMENU_EDIT_SAVE_PREF:
      {
         // FIXME any proper way to flush all profiles at once?
         wxConfigBase *config = mApplication->GetProfile()->GetConfig();
         bool ok = config != NULL;
         if ( ok )
            ok = config->Flush();

         if ( ok )
            wxLogStatus(this, _("Program preferences successfully saved."));
         else
            ERRORMESSAGE((_("Couldn't save preferences.")));
      }
      break;

   case WXMENU_HELP_ABOUT:
      MDialog_AboutDialog(this, false /* don't timeout */);
      break;
   case WXMENU_HELP_CONTEXT:
      MDialog_Message(_("Help not implemented for current context, yet."),this,_("Sorry"));
      break;
   case WXMENU_HELP_CONTENTS:
      mApplication->Help(MH_CONTENTS,this);
      break;
   case WXMENU_HELP_RELEASE_NOTES:
      mApplication->Help(MH_RELEASE_NOTES,this);
      break;
   case WXMENU_HELP_FAQ:
      mApplication->Help(MH_FAQ,this);
      break;
   case WXMENU_HELP_SEARCH:
      mApplication->Help(MH_SEARCH,this);
      break;
   case WXMENU_HELP_COPYRIGHT:
      mApplication->Help(MH_COPYRIGHT,this);
      break;
      // printing:
   case WXMENU_FILE_PRINT_SETUP:
      OnPrintSetup();
      break;
//   case WXMENU_FILE_PAGE_SETUP:
//      OnPageSetup();
//      break;
#ifdef USE_PS_PRINTING
   case WXMENU_FILE_PRINT_SETUP_PS:
      OnPrintSetup();
      break;
#endif
//   case WXMENU_FILE_PAGE_SETUP_PS:
//      OnPageSetup();
//      break;
   }
}

wxToolBar *
wxMFrame::CreateToolBar(void)
{
   wxToolBar *tb =
      wxFrame::CreateToolBar(wxTB_DOCKABLE|wxTB_FLAT|wxTB_HORIZONTAL);
   tb->SetMargins(2,2);
   return tb;
}

void
wxMFrame::OnCloseWindow(wxCloseEvent& event)
{
   if ( event.CanVeto() && !CanClose() )
      event.Veto();
   else
      Destroy();
}

void
wxMFrame::OnCommandEvent(wxCommandEvent &event)
{
   OnMenuCommand(event.GetId());
}

void
wxMFrame::OnPrintSetup()
{
#ifdef OS_WIN
      wxGetApp().SetPrintMode(wxPRINT_WINDOWS);
#else
      wxGetApp().SetPrintMode(wxPRINT_POSTSCRIPT);
#endif

      wxPrintData &data = ((wxMApp *)mApplication)->GetPrintData();
      //FIXME: save and restore orientation
      //data.SetOrientation(orientation);

      wxPrintDialog printerDialog(this, & data);
      printerDialog.GetPrintData().SetSetupDialog(TRUE);
      printerDialog.ShowModal();

      //FIXME orientation = printerDialog.GetPrintData().GetOrientation();
}

void wxMFrame::OnPrintSetupPS()
{
      wxGetApp().SetPrintMode(wxPRINT_POSTSCRIPT);

      wxPrintData &data = ((wxMApp *)mApplication)->GetPrintData();
      //FIXMEdata.SetOrientation(orientation);

      wxPrintDialog printerDialog(this, & data);
      printerDialog.GetPrintData().SetSetupDialog(TRUE);
      printerDialog.ShowModal();

      //FIXME orientation = printerDialog.GetPrintData().GetOrientation();
}

#if 0
// unused so far:

void wxMFrame::OnPageSetup()
{
#ifdef OS_WIN
      wxGetApp().SetPrintMode(wxPRINT_WINDOWS);
#else
      wxGetApp().SetPrintMode(wxPRINT_POSTSCRIPT);
#endif
      wxPrintData &data = ((wxMApp *)mApplication)->GetPrintData();
      //FIXME data.SetOrientation(orientation);

#ifdef OS_WIN
      wxPageSetupDialog pageSetupDialog(this, & data);
#else
      wxGenericPageSetupDialog pageSetupDialog(this, & data);
#endif
      pageSetupDialog.ShowModal();

      data = pageSetupDialog.GetPageSetupData();
      //FIXME orientation = data.GetOrientation();
}

void wxMFrame::OnPageSetupPS()
{
      wxGetApp().SetPrintMode(wxPRINT_POSTSCRIPT);

      wxPrintData &data = ((wxMApp *)mApplication)->GetPrintData();
      data.SetOrientation(orientation);

      wxGenericPageSetupDialog pageSetupDialog(this, & data);
      pageSetupDialog.ShowModal();

      //FIXME orientation = pageSetupDialog.GetPageSetupData().GetOrientation();
}
#endif
