/*-*- c++ -*-********************************************************
 * wxMFrame.h: a basic window class                                 *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$               *
 *
 * $Log$
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
#ifndef	WXMFRAME_H
#define WXMFRAME_H

#ifdef __GNUG__
#pragma interface "wxMFrame.h"
#endif

#ifndef	USE_PCH
#	include	"MFrame.h"
#	include	"guidef.h"
#endif

/**
  * A wxWindows Frame class
  */

class wxMFrame : public wxFrame, public MFrameBase
{
   DECLARE_DYNAMIC_CLASS(wxMFrame)

private:
   /// each frame has a unique name used to identify it
   String	name;
   /// used to set the name of the window class
   virtual void	SetName(String const &name);
   /// is it initialised?
   bool 	initialised;
protected:
   /// the menuBar:
   wxMenuBar	*menuBar;
   /// the File menu:
   wxMenu	*fileMenu;
   /// the Help menu:
   wxMenu	*helpMenu;
   void AddMenuBar(void);
   void AddFileMenu(void);
   void AddHelpMenu(void);
public:
   /// Constructor
   wxMFrame(const String &iname,
	    wxFrame *parent = NULL);
   /// Constructor
   wxMFrame() { initialised = false; }
   /// Creates an object
   void Create(const String &iname, wxFrame *parent = NULL);
   /// Destructor
   ~wxMFrame();

   /// return true if initialised
   bool	IsInitialised(void) const { return initialised; }

   /// make it visible or invisible
#ifdef     USE_WXWINDOWS2
   bool Show(bool visible = true) { return wxFrame::Show(visible); }
#else
   void Show(bool visible = true) { wxFrame::Show(visible); }
#endif

   /// to be called on closing of window
   ON_CLOSE_TYPE OnClose(void);
   /// used to set the title of the window class
   void	SetTitle(String const & name);
   /// get name of this frame:
   virtual String const & GetFrameName(void) const { return name; }
   /// handle basic menu callbacks
   void OnMenuCommand(int id);

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

   DECLARE_EVENT_TABLE()
#endif

private:
   ///  try to save the window position and size in config file
   void	SavePosition(void);
};

#endif
