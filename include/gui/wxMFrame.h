/*-*- c++ -*-********************************************************
 * wxMFrame.h: a basic window class                                 *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$               *
 *
 * $Log$
 * Revision 1.11  1998/06/14 12:24:07  KB
 * started to move wxFolderView to be a panel, Python improvements
 *
 * Revision 1.10  1998/06/05 16:57:00  VZ
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
 * Revision 1.9  1998/05/14 09:48:40  KB
 * added IsEmpty() to strutil, minor changes
 *
 * Revision 1.8  1998/05/11 20:18:44  VZ
 * Testing CVS from Windows
 *
 * Revision 1.7  1998/05/02 15:21:33  KB
 * Fixed the #if/#ifndef etc mess - previous sources were not compilable.
 *
 *
 *******************************************************************/
#ifndef  WXMFRAME_H
#define WXMFRAME_H

#ifdef __GNUG__
#   pragma interface "wxMFrame.h"
#endif

#ifndef  USE_PCH
#   include  "MFrame.h"
#   include  "guidef.h"
#   include  "kbList.h"
#endif

/**
  * A wxWindows Frame class
  */

class wxMFrame : public wxFrame, public MFrameBase
{
   DECLARE_DYNAMIC_CLASS(wxMFrame)
public:
   /// dummy ctor for DECLARE_DYNAMIC_CLASS
   wxMFrame() : MFrameBase("") { FAIL; }
   /// Constructor
   wxMFrame(const String &iname, wxFrame *parent = NULL);
   /// Creates an object
   void Create(const String &iname, wxFrame *parent = NULL);
   /// Destructor
   ~wxMFrame();

   /// return true if initialised
   bool  IsInitialised(void) const { return initialised; }

   /// make it visible or invisible
#ifdef     USE_WXWINDOWS2
   bool Show(bool visible = true) { return wxFrame::Show(visible); }
#else
   void Show(bool visible = true) { wxFrame::Show(visible); }
#endif

   /// to be called on closing of window
   ON_CLOSE_TYPE OnClose(void);
   /// used to set the title of the window class
   void  SetTitle(String const & name);
   /// handle basic menu callbacks
   void OnMenuCommand(int id);
   /// return menu bar
   wxMenuBar *GetMenuBar(void)      { return menuBar; }

   /// add a menu to the bar
   void AddMenu(wxMenu *menu, String const & title);
   void AddFileMenu(void);
   void AddHelpMenu(void);
   void AddMessageMenu(void);
#ifdef     USE_WXWINDOWS2

   //@{ Menu callbacks
    /// 
   void OnOpen(wxCommandEvent&)     { OnMenuCommand(WXMENU_FILE_OPEN); }
    /// 
   void OnAdbEdit(wxCommandEvent&)  { OnMenuCommand(WXMENU_FILE_ADBEDIT); }
    /// 
   void OnClose(wxCommandEvent&)    { OnMenuCommand(WXMENU_FILE_CLOSE); }
    /// 
   void OnCompose(wxCommandEvent&)  { OnMenuCommand(WXMENU_FILE_COMPOSE); }
    /// 
   void OnExit(wxCommandEvent&)     { OnMenuCommand(WXMENU_FILE_EXIT); }
    /// 
   void OnAbout(wxCommandEvent&)    { OnMenuCommand(WXMENU_HELP_ABOUT); }
    ///
   void OnMenuClose(wxCommandEvent&) { OnMenuCommand(WXMENU_FILE_CLOSE); }
   //@}

   void OnCommandEvent(wxCommandEvent & event);
   DECLARE_EVENT_TABLE()
#endif


protected:
   /// the menuBar:
   wxMenuBar   *menuBar;
   /// the File menu:
   wxMenu   *fileMenu;
   /// the Help menu:
   wxMenu   *helpMenu;

private:
   /// is it initialised?
   bool  initialised;
   ///  try to save the window position and size in config file
   void  SavePosition(void);
};

#endif
