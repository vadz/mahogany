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
   // static helper functions (static because they're also used by wxLogWindow
   // which doesn't derive from wxMFrame)
      // read the frame position and size from config (the variables needn't be
      // initialized, they will receive some values in any case), returns FALSE
      // if there is no config object to read settings from
   static bool RestorePosition(const char *name,
                               int *x, int *y, int *w, int *h);
      //  save the given frame's position and size in config file
   static void SavePosition(const char *name, wxFrame *frame);

   /// dummy ctor for DECLARE_DYNAMIC_CLASS
   wxMFrame() : MFrameBase("") { initialised = false; }
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
   wxMenuBar *GetMenuBar(void)  const { return m_MenuBar; }

   /// add a menu to the bar
   void AddEditMenu(void);
   void AddFileMenu(void);
   void AddHelpMenu(void);
   void AddMessageMenu(void);

   // callbacks
   virtual void OnMenuCommand(int id);
#ifdef     USE_WXWINDOWS2
   void OnCommandEvent(wxCommandEvent & event);

   DECLARE_EVENT_TABLE()
#endif   //wxWin2

protected:
   /// menu bar
   wxMenuBar *m_MenuBar;

#ifdef   USE_WXWINDOWS2
   /// toolbar:
   wxMToolBar *m_ToolBar;
#endif

   /// is it initialised?
   bool  initialised;
};

#endif
