/*-*- c++ -*-********************************************************
 * wxMainFrame.h: a basic window class                              *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$       *
 ********************************************************************
 * $Log$
 * Revision 1.4  1998/06/14 12:24:08  KB
 * started to move wxFolderView to be a panel, Python improvements
 *
 * Revision 1.3  1998/06/05 16:57:02  VZ
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
 * Revision 1.2  1998/05/18 17:48:26  KB
 * more list<>->kbList changes, fixes for wxXt, improved makefiles
 *
 * Revision 1.1  1998/03/14 12:21:15  karsten
 * first try at a complete archive
 *
 *******************************************************************/
#ifndef  WXMAINFRAME_H
#define  WXMAINFRAME_H

#include <MMainFrame.h>
#include <gui/wxMenuDefs.h>
#include <gui/wxMFrame.h>

#ifdef __GNUG__
#pragma interface "wxMainFrame.h"
#endif

class wxMainTreeCtrl : public wxTreeCtrl
{
public:
   wxMainTreeCtrl(wxWindow *parent, const wxWindowID id, const wxPoint& pos,
                const wxSize& size, long style):
      wxTreeCtrl(parent, id, pos, size, style)
      {
      }
/*
  void OnBeginDrag(wxTreeEvent& event);
  void OnBeginRDrag(wxTreeEvent& event);
  void OnBeginLabelEdit(wxTreeEvent& event);
  void OnEndLabelEdit(wxTreeEvent& event);
  void OnDeleteItem(wxTreeEvent& event);
  void OnGetInfo(wxTreeEvent& event);
  void OnSetInfo(wxTreeEvent& event);
  void OnItemExpanded(wxTreeEvent& event);
  void OnItemExpanding(wxTreeEvent& event);
  void OnSelChanged(wxTreeEvent& event);
  void OnSelChanging(wxTreeEvent& event);
  void OnKeyDown(wxTreeEvent& event);
*/
   DECLARE_EVENT_TABLE()
};


class wxMainFrame : public wxMFrame
{
public:
   /// Constructor
   wxMainFrame(const String &iname = String("wxMainFrame"),
               wxFrame *parent = NULL);
   
   void OnMenuCommand(int id);
#ifdef   USE_WXWINDOWS2
   void OnAbout(wxCommandEvent &) { OnMenuCommand(WXMENU_HELP_ABOUT); }
   DECLARE_EVENT_TABLE()
#endif
private:
#ifdef   USE_WXWINDOWS2
   wxSplitterWindow *splitter;
   wxMainTreeCtrl *treeCtrl;
#endif
};

#endif
