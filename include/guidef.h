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

#include <wx/defs.h>
#include <wx/frame.h>
#include <wx/button.h>
#include <wx/filefn.h>
#include <wx/filedlg.h>
#include <wx/textctrl.h> // wxBrowsebutton

#include "gui/wxMenuDefs.h"

/// how much space to leave in frame around other items
#define   WXFRAME_WIDTH_DELTA   16
/// how much space to leave in frame around other items
#define   WXFRAME_HEIGHT_DELTA   64

// minimal space to leave between dialog/panel items: tune them if you want,
// but use these constants everywhere instead of raw numbers to make the look
// of all dialogs consistent
#define LAYOUT_X_MARGIN       5
#define LAYOUT_Y_MARGIN       5

// this function is obsolete
inline long AdjustCharHeight(long h)
{
   return h;
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
extern wxWindow *GetParentOfClass(const wxWindow *win, wxClassInfo *classinfo);

// get the pointer to the parent window of given type (may be NULL), very
// useful to find, for example, the parent frame for any control in the frame.
#define GET_PARENT_OF_CLASS(win, classname)  \
   ((classname *)(GetParentOfClass(win, CLASSINFO(classname))))

// get the pointer to the frame we belong to (returns NULL if none)
inline wxFrame *GetFrame(const wxWindow *win)
{
   return  GET_PARENT_OF_CLASS(win, wxFrame);
}

/**
  Check if the given font encoding is available on this system. If not, try to
  find a replacement encoding - if this succeeds, the text is translated into
  this encoding and the encoding parameter is modified in place.

  Note that this function is implemented in wxMApp.cpp.

  @param encoding the encoding to check, may be modified
  @param text the text we want to show in this encoding, may be translated
  @return true if this or equivalent encoding is available, false otherwise
 */
extern bool EnsureAvailableTextEncoding(wxFontEncoding *encoding,
                                        wxString *text = NULL,
                                        bool mayAskUser = false);

// Prevent MEvent dispatch inside wxYield
extern int g_busyCursorYield;

extern void MBeginBusyCursor();
extern void MEndBusyCursor();

class MBusyCursor
{
public:
   MBusyCursor() { MBeginBusyCursor(); }
   ~MBusyCursor() { MEndBusyCursor(); }
};

#endif // GUIDEF_H
