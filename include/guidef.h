/*-*- c++ -*-********************************************************
 * guidef.h define the GUI implementation                           *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/

#ifndef GUIDEF_H
#define GUIDEF_H

#ifdef  USE_WXWINDOWS

/// define the class to use for implementing MFrame objects
#   define   MFrame        wxMFrame
#   define   MWindow       wxWindow
#   define   MainFrame     wxMainFrame
#   define   MDialogs      wxMDialogs

#include   <wx/defs.h>
#include   <wx/frame.h>
#include   <wx/button.h>
#include   <wx/filefn.h>
#include   <wx/filedlg.h>
#include   <wx/textctrl.h> //wxBrowsebutton
#   include  "gui/wxMenuDefs.h"
/// how much space to leave in frame around other items
#   define   WXFRAME_WIDTH_DELTA   16
/// how much space to leave in frame around other items
#   define   WXFRAME_HEIGHT_DELTA   64

#else
#  error "Implemented only for wxWindows."
#endif // USE_WXWINDOWS

// minimal space to leave between dialog/panel items: tune them if you want,
// but use these constants everywhere instead of raw numbers to make the look
// of all dialogs consistent
#define LAYOUT_X_MARGIN       5
#define LAYOUT_Y_MARGIN       5

// these coefficients are used to calculate the size of the controls in
// character height units (which we retrieve with wxGetCharHeight).
// @@@ the coeffecients are purely empirical...

inline long AdjustCharHeight(long h)
{
#  ifdef OS_WIN
      return h - 3;
#  else  // !Win
      return h;
#  endif // OS_WIN
}

// calculate the "optimal" text height for given label height
#define TEXT_HEIGHT_FROM_LABEL(h)      (23*(h)/13)

// to get nice looking buttons width should be proportional to the height with
// this coefficient (it is not my personal taste, but rather the coefficient
// is the same as for the standard buttons under Windows)
#define BUTTON_WIDTH_FROM_HEIGHT(w)    (75*(w)/23)

// this functions go up the window inheritance chain until it finds a window of
// the given class or a window without parent (in which case the function will
// return NULL). This function is mainly for the use with the macro below.
extern wxWindow *GetParentOfClass(wxWindow *win, wxClassInfo *classinfo);

// get the pointer to the parent window of given type (may be NULL), very
// useful to find, for example, the parent frame for any control in the frame.
#define GET_PARENT_OF_CLASS(win, classname)  \
   ((classname *)(GetParentOfClass(win, CLASSINFO(classname))))

// get the pointer to the frame we belong to (returns NULL if none)
inline wxFrame *GetFrame(wxWindow *win)
{
   return  GET_PARENT_OF_CLASS(win, wxFrame);
}

#endif // GUIDEF_H
