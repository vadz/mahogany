/*-*- c++ -*-********************************************************
 * wxMApp class: do all GUI specific  application stuff             *
 *                                                                  *
 * (C) 1997,1998 by Karsten Ballüder (Ballueder@usa.net)            *
 *                                                                  *
 * $Id$                *
 *
 * $Log$
 * Revision 1.6  1998/06/05 16:56:24  VZ
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
 * Revision 1.5  1998/05/18 17:48:42  KB
 * more list<>->kbList changes, fixes for wxXt, improved makefiles
 *
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

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxMApp
// ----------------------------------------------------------------------------
wxMApp::wxMApp(void)
{
}

wxMApp::~wxMApp()
{
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
   topLevelFrame = GLOBAL_NEW wxMainFrame();
   topLevelFrame->SetTitle(M_TOPLEVELFRAME_TITLE);
   topLevelFrame->Show(true);

#  ifdef  USE_WXWINDOWS2
      SetTopWindow(topLevelFrame);
#  endif

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
      int main(int argc, char *argv[]) { return wxEntry(argc, argv); }
#  endif
#else   // wxWin1
   wzMApp mApplication;
#endif  // wxWin ver
