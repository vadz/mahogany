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
#   define   FolderView    wxFolderView
#   define   MainFrame     wxMainFrame
#   define   MDialogs      wxMDialogs


#   define   Uses_wxFrame
#   define   Uses_wxMenu
#   define   Uses_wxString
#   define   Uses_wxCanvas
#   define   Uses_wxTextWindow
#   define   Uses_wxFont
#   define   Uses_wxMenu
#   define   Uses_wxMenuBar
#   define   Uses_wxListBox
#   define   Uses_wxPostScriptDC
#   define   Uses_wxPrintSetup
#   define   Uses_wxTextWindow
#   define   Uses_wxTimer
#   define   Uses_wxMessage
#   define   Uses_wxButton
#   define   Uses_wxDialogBox
#   define   Uses_wxDialog
#   include  <wx/wx.h>
#   include  <wx/splitter.h>
#   include  <wx/treectrl.h>
#   include  <wx/toolbar.h>
#   include  "gui/wxMenuDefs.h"
/// how much space to leave in frame around other items
#   define   WXFRAME_WIDTH_DELTA   16
/// how much space to leave in frame around other items
#   define   WXFRAME_HEIGHT_DELTA   64

// minimal space to leave between dialog/panel items: tune them if you want,
// but use these constants everywhere instead of raw numbers to make the look
// of all dialogs consistent
#define LAYOUT_X_MARGIN       5
#define LAYOUT_Y_MARGIN       5

#else
#  error "Implemented only for wxWindows."
#endif // USE_WXWINDOWS

#endif // GUIDEF_H
