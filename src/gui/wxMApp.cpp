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
#  include "Mdefaults.h"
#  include "MApplication.h"
#  include "gui/wxMApp.h"

#  include <wx/log.h>
#endif

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
   // ctor (it creates the frame hidden)
   wxMLogWindow(const char *szTitle) : wxLogWindow(szTitle, FALSE) { }

   // override base class virtual to implement saving the frame position
   virtual void OnFrameDelete(wxFrame *frame);
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxMLogWindow
// ----------------------------------------------------------------------------
void wxMLogWindow::OnFrameDelete(wxFrame *frame)
{
   wxMFrame::SavePosition(LOG_FRAME_SECTION, frame);

//FIXME for old wxWindows   wxLogWindow::OnFrameDelete(frame);
}

// ----------------------------------------------------------------------------
// wxMApp
// ----------------------------------------------------------------------------
wxMApp::wxMApp(void)
{
   m_IconManager = NULL;
}

wxMApp::~wxMApp()
{
   if(m_IconManager) delete m_IconManager;
}

// app initilization
#ifdef  USE_WXWINDOWS2
bool
#else //wxWin1
wxFrame *
#endif
wxMApp::OnInit()
{
   // first of all, create the log
   wxLogWindow *log = new wxMLogWindow(_("M Activity Log"));

   m_IconManager = new wxIconManager();

   if ( OnStartup() ) {
      // now that we have config, restore the log frame position and show it
      int x, y, w, h;
      wxMFrame::RestorePosition(LOG_FRAME_SECTION, &x, &y, &w, &h);
      log->GetFrame()->SetSize(x, y, w, h);
      log->Show();

      // we want it to be above the log frame
      topLevelFrame->Raise();

#     ifdef  USE_WXWINDOWS2
         return true;
#     else
         return m_topLevelFrame;
#     endif
   }
   else {
      ERRORMESSAGE(("Can't initialize application, terminating."));

      return 0; // either false or (wxFrame *)NULL
   }
}

MFrame *wxMApp::CreateTopLevelFrame()
{
   m_topLevelFrame = GLOBAL_NEW wxMainFrame();
   m_topLevelFrame->SetTitle(M_TOPLEVELFRAME_TITLE);
   m_topLevelFrame->Show(true);

   return m_topLevelFrame;
}

int wxMApp::OnExit()
{
   delete wxLog::SetActiveTarget(NULL);

   return 0;
}

// ----------------------------------------------------------------------------
// global vars
// ----------------------------------------------------------------------------

// this creates the one and only application object
#ifdef  USE_WXWINDOWS2
   IMPLEMENT_APP(wxMApp);
   
#  ifdef USE_WXGTK
      // @@@ I don't understand why isn't it in gtk/app.cpp in wxGTK
      extern int wxEntry(int argc, char *argv[]);
      //int main(int argc, char *argv[]) { return wxEntry(argc, argv); }
#  endif
#else   // wxWin1
   wxMApp mApplication;
#endif  // wxWin ver
