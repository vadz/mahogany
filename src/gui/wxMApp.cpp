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
#  include  "Mcommon.h"
#  include  "guidef.h"
#  include  "MFrame.h"
#  include  "gui/wxMFrame.h"
#  include  "MLogFrame.h"

#  include  "kbList.h"
#  include  "PathFinder.h"
#  include  "MimeList.h"
#  include  "MimeTypes.h"
#  include  "Profile.h"
#endif

#include "Mdefaults.h"

#include "MApplication.h"
#include "gui/wxMApp.h"
#include "gui/wxMainFrame.h"
#include "gui/wxMLogFrame.h"
#include "gui/wxIconManager.h"

// ============================================================================
// implementation
// ============================================================================

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
   if ( OnStartup() ) {
#     ifdef  USE_WXWINDOWS2
         return true;
#     else
         return topLevelFrame;
#     endif
   }
   else {
      ERRORMESSAGE(("Can't initialize application, terminating."));

      return 0; // either false or (wxFrame *)NULL
   }
}

MFrame *wxMApp::CreateTopLevelFrame()
{
   m_IconManager = new wxIconManager();

   topLevelFrame = GLOBAL_NEW wxMainFrame();
   topLevelFrame->SetTitle(M_TOPLEVELFRAME_TITLE);
   topLevelFrame->Show(true);

#ifdef  USE_WXWINDOWS2
   SetTopWindow(topLevelFrame);
#endif
      
   return topLevelFrame;
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
