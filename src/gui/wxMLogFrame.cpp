/*-*- c++ -*-********************************************************
 * wxMLogFrame.h : class for window printing log output             *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$           *
 ********************************************************************
 * $Log$
 * Revision 1.5  1998/06/05 16:56:28  VZ
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
 * Revision 1.4  1998/05/18 17:48:45  KB
 * more list<>->kbList changes, fixes for wxXt, improved makefiles
 *
 * Revision 1.3  1998/04/30 19:12:35  VZ
 * (minor) changes needed to make it compile with wxGTK
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
#  pragma implementation "wxMLogFrame.h"
#endif

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------
#include "Mpch.h"

#ifndef  USE_PCH
#  include "Mcommon.h"
#  include "MFrame.h"
#  include "MLogFrame.h"

#  include "kbList.h"
#  include "PathFinder.h"
#  include "MimeList.h"
#  include "MimeTypes.h"
#  include "Profile.h"
#endif // USE_PCH

#include "MApplication.h"
#include "gui/wxMApp.h"

#include "gui/wxMLogFrame.h"

#ifdef USE_WXWINDOWS2
#  include <wx/log.h>
#endif // wxWin2

// ----------------------------------------------------------------------------
// (local) classes
// ----------------------------------------------------------------------------
#ifdef USE_WXWINDOWS2
class MLog : public wxLogGui
{
public:
   // ctor takes the text control where the output is forked
   MLog(wxTextCtrl *pTextCtrl) { m_pTextCtrl = pTextCtrl; }

protected:
   virtual void DoLog(wxLogLevel level, const char *szString);
   virtual void DoLogString(const char *szString);

private:
   wxTextCtrl *m_pTextCtrl;
};
#endif // wxWin2

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------
#ifdef USE_WXWINDOWS2
   BEGIN_EVENT_TABLE(wxMLogFrame, wxMFrame)
      // wxMLogFrame menu events
      EVT_MENU(WXMENU_FILE_CLOSE, wxMLogFrame::OnLogClose)
   END_EVENT_TABLE() 
#endif // wxWin2

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// MLog
// ----------------------------------------------------------------------------
#ifdef USE_WXWINDOWS2

void MLog::DoLog(wxLogLevel level, const char *str)
{
   // first let wxLogGui show it
   wxLogGui::DoLog(level, str);

   // and this will format it nicely and call our DoLogString()
   wxLog::DoLog(level, str);
}

void MLog::DoLogString(const char *str)
{
   // put the text into our window and ensure that the line can be seen
   m_pTextCtrl->WriteText(str);
   m_pTextCtrl->WriteText("\n");
   
   // @@ no GetNumberOfLines() in wxTextCtrl in wxGTK
#  ifndef  USE_WXGTK
   m_pTextCtrl->ShowPosition(m_pTextCtrl->GetNumberOfLines());
#  endif //USE_WXGTK
}

#endif // wxWin2

// ----------------------------------------------------------------------------
// wxMLogFrame
// ----------------------------------------------------------------------------
wxMLogFrame::wxMLogFrame()
           : wxMFrame("LogWindow")
{
   SetTitle(_("M activity log"));

   #if  USE_WXWINDOWS2
      tw = GLOBAL_NEW wxTextCtrl(this, -1, "", wxDefaultPosition, 
                                 wxDefaultSize, wxTE_MULTILINE);
   #else  // wxWin 1
      tw = GLOBAL_NEW wxTextWindow(this);
   #endif // wxWin ver

   AddMenuBar();
   AddFileMenu();
   AddHelpMenu();

   // associate the menu bar with the frame
   SetMenuBar(menuBar);

   // redirect the log output here
#  ifdef USE_WXWINDOWS2
      m_pLogSink = new MLog(tw);
      m_pOldLogSink = wxLog::SetActiveTarget(m_pLogSink);
#  endif // wxWin2

   wxFrame::Show(TRUE);
}

wxMLogFrame::~wxMLogFrame()
{
   wxLog::SetActiveTarget(m_pOldLogSink);
}

void
wxMLogFrame::Clear()
{
   tw->Clear();
}

bool
wxMLogFrame::Save(const char *filename)
{
   // wxMLogFrame::Save TODO

   return false;
}

void
wxMLogFrame::OnMenuCommand(int id)
{
   switch(id)
   {
   case WXMENU_FILE_CLOSE:
      OnClose();
      break;

   default:
      wxMFrame::OnMenuCommand(id);
   }
}

ON_CLOSE_TYPE
wxMLogFrame::OnClose()
{
   // just hide the frame
   Show(FALSE);

   // don't delete us
   return FALSE;
}

// this method is useless in wxWin2 where you should use the standard
// log functions instead
#ifndef  USE_WXWINDOWS2
void
wxMLogFrame::Write(const char *str)
{
   tw->WriteText(WXCPTR(str));
   tw->ShowPosition(tw->GetNumberOfLines());
}
#endif // wxWin1
