/*-*- c++ -*-********************************************************
 * wxMainFrame.h: a basic window class                              *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$            *
 *******************************************************************/
#ifndef  WXMAINFRAME_H
#define  WXMAINFRAME_H

#include "MMainFrame.h"
#include "gui/wxMenuDefs.h"
#include "gui/wxMFrame.h"

#ifdef __GNUG__
#   pragma interface "wxMainFrame.h"
#endif

class wxMainTreeCtrl : public wxTreeCtrl
{
public:
   wxMainTreeCtrl(wxWindow *parent, const wxWindowID id, const wxPoint& pos,
                const wxSize& size, long style):
      wxTreeCtrl(parent, id, pos, size, style)
      {
      }
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
