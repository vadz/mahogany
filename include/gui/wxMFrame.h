/*-*- c++ -*-********************************************************
 * wxMFrame.h: a basic window class                                 *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$              *
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
   wxMFrame(const String &iname, wxWindow *parent = NULL);
   /// Creates an object
   void Create(const String &iname, wxWindow *parent = NULL);
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
   /// return menu bar
   wxMenuBar *GetMenuBar(void)      { return menuBar; }

   /// add a menu to the bar
   void AddMenu(wxMenu *menu, String const & title);
   void AddFileMenu(void);
   void AddHelpMenu(void);
   void AddMessageMenu(void);
   /// handle menu events
   void OnMenuCommand(int id);
#ifdef     USE_WXWINDOWS2
   /// wxWin2 event system
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
