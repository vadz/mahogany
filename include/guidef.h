/*-*- c++ -*-********************************************************
 * guidef.h define the GUI implementation                           *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.6  1998/06/14 12:24:02  KB
 * started to move wxFolderView to be a panel, Python improvements
 *
 * Revision 1.5  1998/06/05 16:56:50  VZ
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
 * Revision 1.4  1998/05/11 20:29:37  VZ
 * compiles under Windows again + option USE_WXCONFIG added
 *
 * Revision 1.3  1998/04/22 19:54:48  KB
 * Fixed _lots_ of problems introduced by Vadim's efforts to introduce
 * precompiled headers. Compiles and runs again under Linux/wxXt. Header
 * organisation is not optimal yet and needs further
 * cleanup. Reintroduced some older fixes which apparently got lost
 * before.
 *
 * Revision 1.2  1998/03/26 23:05:37  VZ
 * Necessary changes to make it compile under Windows (VC++ only)
 * Header reorganization to be able to use precompiled headers
 *
 * Revision 1.1  1998/03/14 12:21:13  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifndef GUIDEF_H
#define GUIDEF_H

#if   USE_WXWINDOWS

///   define the class to use for implementing MFrame objects
#   define   MFrame      wxMFrame
#   define   FolderView   wxFolderView
#   define   MainFrame   wxMainFrame
#   define   MDialogs   wxMDialogs


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
#   include   <wx/wx.h>
#   include   <wx/splitter.h>
#   include   <wx/treectrl.h>
#   include   "gui/wxMenuDefs.h"
/// how much space to leave in frame around other items
#   define   WXFRAME_WIDTH_DELTA   16
/// how much space to leave in frame around other items
#   define   WXFRAME_HEIGHT_DELTA   64

#endif

#endif
