/*-*- c++ -*-********************************************************
 * wxMFrame.h: a basic window class                                 *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.2  1998/03/26 23:05:38  VZ
 * Necessary changes to make it compile under Windows (VC++ only)
 * Header reorganization to be able to use precompiled headers
 *
 * Revision 1.1  1998/03/14 12:21:15  karsten
 * first try at a complete archive
 *
 *******************************************************************/
#ifndef	WXMFRAME_H
#define WXMFRAME_H

#ifdef __GNUG__
#pragma interface "wxMFrame.h"
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
   void	SetName(String const &name);
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
   /// to be called on closing of window
   ON_CLOSE_TYPE OnClose(void);

   /// make it visible or invisible
#if     USE_WXWINDOWS2
   bool Show(bool visible = true) { return wxFrame::Show(visible); }
#else
   void	Show(bool visible = true) { wxFrame::Show(visible); }
#endif
   /// used to set the title of the window class
   void	SetTitle(String const & name);
   /// get name of this frame:
   virtual String const & GetFrameName(void) const { return name; }
   /// handle basic menu callbacks
   void OnMenuCommand(int id);

#if     USE_WXWINDOWS2

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
   //@}

   DECLARE_EVENT_TABLE()
#endif

private:
   ///  try to save the window position and size in config file
   void	SavePosition(void);
};

#endif
