/*-*- c++ -*-********************************************************
 * wxMApp class: do all GUI specific  application stuff             *
 *                                                                  *
 * (C) 1997,1998 by Karsten Ballüder (Ballueder@usa.net)            *
 *                                                                  *
 * $Id$                *
 *
 *******************************************************************/

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------
#include "Mpch.h"

#ifndef  USE_PCH
#  include "Mcommon.h"
#  include "guidef.h"
#  include "MFrame.h"
#  include "gui/wxMFrame.h"

#  include "kbList.h"
#  include "PathFinder.h"
#  include "MimeList.h"
#  include "MimeTypes.h"
#  include "Profile.h"
#  include "MApplication.h"
#  include "gui/wxMApp.h"

   extern "C"
   {
#     include <mail.h>
#     include <osdep.h>  // for sysinbox() &c
   }

#  include <wx/log.h>

#  include <wx/config.h>
#endif

#include "Mdefaults.h"
#include "MDialogs.h"

#include "MObject.h"

#include "MHelp.h"

#include "gui/wxMainFrame.h"
#include "gui/wxIconManager.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// the name of config section where the log window position is saved
#define  LOG_FRAME_SECTION    "LogWindow"

// ----------------------------------------------------------------------------
// classes
// ----------------------------------------------------------------------------

// a small class implementing saving/restoring position for the log frame
class wxMLogWindow : public wxLogWindow
{
public:
   // ctor (creates the frame hidden)
   wxMLogWindow(wxFrame *pParent, const char *szTitle);

   // override base class virtual to implement saving the frame position
   virtual void OnFrameDelete(wxFrame *frame);
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxMLogWindow
// ----------------------------------------------------------------------------
wxMLogWindow::wxMLogWindow(wxFrame *pParent, const char *szTitle)
            : wxLogWindow(pParent, szTitle, FALSE)
{
   int x, y, w, h;
   wxMFrame::RestorePosition(LOG_FRAME_SECTION, &x, &y, &w, &h);
   GetFrame()->SetSize(x, y, w, h);
   Show();
}

void wxMLogWindow::OnFrameDelete(wxFrame *frame)
{
   wxMFrame::SavePosition(LOG_FRAME_SECTION, frame);

   wxLogWindow::OnFrameDelete(frame);
}

// ----------------------------------------------------------------------------
// wxMApp
// ----------------------------------------------------------------------------
wxMApp::wxMApp(void)
{
   m_IconManager = NULL;
   m_HelpController = NULL;
}

wxMApp::~wxMApp()
{
   if(m_HelpController)
      delete m_HelpController;
}

void
wxMApp::OnAbnormalTermination()
{
   MAppBase::OnAbnormalTermination();

//FIXME
#if 0
   ::MessageBox(NULL, _("The application is terminating abnormally.\n"
                "Please report the bug to m-developers@makelist.com\n"
                        "Thank you!"), "Fatal application error",
                MB_ICONSTOP);
#endif   
}

// app initilization
bool
wxMApp::OnInit()
{
   m_IconManager = new wxIconManager();
   
   if ( OnStartup() ) {
      // now we can create the log window
      if ( READ_APPCONFIG(MC_SHOWLOG) ) {
         (void)new wxMLogWindow(m_topLevelFrame, _("M Activity Log"));
         
         // we want it to be above the log frame
         m_topLevelFrame->Raise();
      }

      return true;
   }
   else {
      ERRORMESSAGE(("Can't initialize application, terminating."));
      
      return false;
   }
}

MFrame *wxMApp::CreateTopLevelFrame()
{
   m_topLevelFrame = GLOBAL_NEW wxMainFrame();
   m_topLevelFrame->SetTitle(M_TOPLEVELFRAME_TITLE);
   m_topLevelFrame->Show(true);

   return m_topLevelFrame;
}

int wxMApp::OnRun()
{
   return wxApp::OnRun();
}

int wxMApp::OnExit()
{
   MAppBase::OnShutDown();

   delete m_IconManager;

   MObjectRC::CheckLeaks();

   // delete the previously active log target (it's the one we had set before
   // here, but in fact it doesn't even matter: if somebody installed another
   // one, we will delete his log object and his code will delete ours)
   wxLog *log = wxLog::SetActiveTarget(NULL);
   delete log;
 
   // as c-client lib doesn't seem to think that deallocating memory is
   // something good to do, do it at it's place...
#  ifdef OS_WIN
      free(sysinbox());
      free(myhomedir());
      free(myusername());
#  endif

   return 0;
}

void
wxMApp::Help(int id, wxWindow *parent)
{
   // store help key used in search
   static wxString last_key;
   
   if(! m_HelpController)
   {
      m_HelpController = new wxHelpController;
#ifdef OS_UNIX
      ((wxExtHelpController *)m_HelpController)->SetBrowser(
         READ_APPCONFIG(MC_HELPBROWSER),
         READ_APPCONFIG(MC_HELPBROWSER_ISNS));
      // initialise the help system
      m_HelpController->Initialize(GetGlobalDir()+"/doc");
#else // Windows
      m_HelpController->Initialize(GetGlobalDir()+"\\doc\\M.hlp");
#endif // Unix/Windows
   }

   // show help:
   switch(id)
   {
      // look up contents:
   case   MH_CONTENTS:
      m_HelpController->DisplayContents();
      break;
      // implement a search:
   case MH_SEARCH:
      last_key = wxGetTextFromUser(_("Search for?"),
                                   _("Search help for keyword"),
                                   last_key,
                                   parent ? parent : TopLevelFrame());
      if(! last_key.IsEmpty())
         m_HelpController->KeywordSearch(last_key);
      break;
      // all other help ids, just look them up:
   default:
      m_HelpController->DisplaySection(id);
      break;
   }
}


// ----------------------------------------------------------------------------
// global vars
// ----------------------------------------------------------------------------

// this creates the one and only application object
IMPLEMENT_APP(wxMApp);
