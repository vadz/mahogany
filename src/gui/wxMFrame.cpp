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

#   include <wx/confbase.h>
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

   wxConfigBase *pConf = mApplication->GetProfile()->GetConfig();
   if ( pConf != NULL )
   {
      pConf->SetPath(String(M_FRAMES_CONFIG_SECTION)+name);
      *x = pConf->Read(MP_XPOS, MP_XPOS_D);
      *y = pConf->Read(MP_YPOS,MP_YPOS_D);
      *w = pConf->Read(MP_WIDTH, MP_WIDTH_D);
      *h = pConf->Read(MP_HEIGHT, MP_HEIGHT_D);

      // assume that if one entry existed, then the other existed too
      return pConf->HasEntry(MP_XPOS);
   }
   else
   {
      wxLogDebug("Can't restore position/size of window '%s'.", name);
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
   WXADD_MENU(m_MenuBar, EDIT, _("&Edit"));
}

void
wxMFrame::AddMessageEditMenu(void)
{
   WXADD_MENU(m_MenuBar, MSG_EDIT, _("&Edit"));
}

void
wxMFrame::AddHelpMenu(void)
{
   WXADD_MENU(m_MenuBar, HELP, _("&Help"));
}

void
wxMFrame::AddMessageMenu(void)
{
   WXADD_MENU(m_MenuBar, MSG, _("&Message"));
}

wxMFrame::~wxMFrame()
{
   SavePosition(MFrameBase::GetName(), this);
}

void
wxMFrame::SetTitle(String const &title)
{
   wxString t = "Mahogany : " + title;
   // the following (char *) is required to avoid a warning
   wxFrame::SetTitle((char *)t.c_str());
}

void
wxMFrame::SavePosition(const char *name, wxWindow *frame)
{
   int x,y;

   wxConfigBase *pConf = mApplication->GetProfile()->GetConfig();
   pConf->SetPath(String(M_FRAMES_CONFIG_SECTION)+name);
   if ( pConf != NULL )
   {
      frame->GetPosition(&x,&y);
      pConf->Write(MP_XPOS, (long)x);
      pConf->Write(MP_YPOS, (long)y);

      frame->GetSize(&x,&y);
      pConf->Write(MP_WIDTH, (long)x);
      pConf->Write(MP_HEIGHT, (long)y);
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
         wxComposeView *composeView = wxComposeView::CreateNewMessage(this);
         composeView->Show();
      }
      break;
   case WXMENU_FILE_POST:
      {
         wxComposeView *composeView = wxComposeView::CreateNewArticle(this);
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
      filename = MDialog_FileRequester(_("Please select a Python script to run."),
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
      if ( CanClose() )
      {
         // this frame has been already asked whether it wants to exit, so
         // don't ask it again
         mApplication->AddToFramesOkToClose(this);

         mApplication->Exit();
      }
      break;

   case WXMENU_EDIT_ADB:
      ShowAdbFrame(this);
      break;

   case WXMENU_EDIT_PREF:
      ShowOptionsDialog(this);
      break;

   case WXMENU_EDIT_RESTORE_PREF:
      (void)ShowRestoreDefaultsDialog(mApplication->GetProfile(), this);
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
      wxFrame::CreateToolBar(wxTB_DOCKABLE|wxTB_FLAT|wxTB_HORIZONTAL,-1,
                             _("Mahogany Toolbar"));
   tb->SetMargins(2,2);
   return tb;
}

void
wxMFrame::OnCloseWindow(wxCloseEvent& event)
{
   if ( event.CanVeto() && !mApplication->IsOkToClose(this) && !CanClose() )
   {
      event.Veto();
   }
   else
   {
      Destroy();
   }
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
#endif
   wxPrintDialogData printDialogData(* ((wxMApp *)mApplication)->GetPrintData());
   wxPrintDialog printerDialog(this, & printDialogData);
   
   printerDialog.GetPrintDialogData().SetSetupDialog(TRUE);
   printerDialog.ShowModal();
   
   (*((wxMApp *)mApplication)->GetPrintData())
      = printerDialog.GetPrintDialogData().GetPrintData();
}

void wxMFrame::OnPrintSetupPS()
{
   wxGetApp().SetPrintMode(wxPRINT_POSTSCRIPT);
   
   wxPrintDialogData printDialogData(* ((wxMApp *)mApplication)->GetPrintData());
   wxPrintDialog printerDialog(this, & printDialogData);
   
   printerDialog.GetPrintDialogData().SetSetupDialog(TRUE);
   printerDialog.ShowModal();
   
   (*((wxMApp *)mApplication)->GetPrintData())
      = printerDialog.GetPrintDialogData().GetPrintData();
}


void wxMFrame::OnPageSetup()
{
   *((wxMApp *)mApplication)->GetPageSetupData()
      = *((wxMApp *)mApplication)->GetPrintData();

      wxPageSetupDialog pageSetupDialog(this,
                                        ((wxMApp *)mApplication)->GetPageSetupData()
                                        );
    pageSetupDialog.ShowModal();
    
    *((wxMApp *)mApplication)->GetPrintData()
       = pageSetupDialog.GetPageSetupData().GetPrintData();
    *((wxMApp *)mApplication)->GetPageSetupData()
       = pageSetupDialog.GetPageSetupData();
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
      //data.SetOrientation(orientation);

      wxGenericPageSetupDialog pageSetupDialog(this, & data);
      pageSetupDialog.ShowModal();

      //FIXME orientation = pageSetupDialog.GetPageSetupData().GetOrientation();
}
#endif
