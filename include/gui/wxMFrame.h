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
   static void SavePosition(const char *name, wxWindow *frame);

   /// dummy ctor for DECLARE_DYNAMIC_CLASS
   wxMFrame() : MFrameBase(M_EMPTYSTRING) { initialised = false; }
   /// Constructor
   wxMFrame(const String &iname, wxWindow *parent = NULL);
   /// Creates an object
   void Create(const String &iname, wxWindow *parent = NULL);
   /// Destructor
   ~wxMFrame();

   /// to enforce common style
   wxToolBar *CreateToolBar(void);
   /// return true if initialised
   bool  IsInitialised(void) const { return initialised; }

   /// make it visible or invisible
   bool Show(bool visible = true) { return wxFrame::Show(visible); }

   /// used to set the title of the window class
   void  SetTitle(String const & name);

   /** This virtual method returns either NULL or an incref'd
       pointer to the profile of the mailfolder being displayed, for
       those wxMFrames which have a folder displayed. Used to make the
       compose view inherit the current folder's settings.
   */
   virtual class Profile *GetFolderProfile(void)
      { return NULL; }

   /// Passes a menu id to modules for reacting to it.
   virtual bool ProcessModulesMenu(int id);

   /// add a menu to the bar
   virtual void AddFileMenu(void);
   virtual void AddHelpMenu(void);
   void AddEditMenu(void);
   void AddMessageMenu(void);
   void AddLanguageMenu(void);

   /// wxMFrame handles all print setup
   void OnPrintSetup();
   void OnPrintSetupPS();
   void OnPageSetup();
//   void OnPageSetupPS();

   // callbacks
   virtual void OnMenuCommand(int id);
   void OnCommandEvent(wxCommandEvent & event);
   void OnCloseWindow(wxCloseEvent& event);

   DECLARE_EVENT_TABLE()

protected:
   /// is it initialised?
   bool  initialised;
};

#endif
