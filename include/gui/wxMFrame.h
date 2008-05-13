///////////////////////////////////////////////////////////////////////////////
// Project:     Mahogany
// File name:   gui/wxMFrame.h - base frame class
// Purpose:     GUI functionality common to all Mahogany frames
// Author:      M-Team
// Modified by:
// Created:     1997
// CVS-ID:      $Id$
// Copyright:   (c) 1997-2001 Mahogany Team
// License:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef  WXMFRAME_H
#define WXMFRAME_H

#ifndef  USE_PCH
#   include  "MFrame.h"
#   include  "guidef.h"
#endif

#include <wx/frame.h>

class Profile;

#ifdef USE_PYTHON
   class PythonOptionChangeHandler;
#endif

/**
  A wxWindows frame class.
 */
class wxMFrame : public wxFrame,
                 public MFrameBase
{
public:
   // static helper functions (static because they're also used by wxLogWindow
   // which doesn't derive from wxMFrame)
      // read the frame position and size from config (the variables needn't be
      // initialized, they will receive some values in any case), returns FALSE
      // if there is no config object to read settings from
   static bool RestorePosition(const char *name,
                               int *x, int *y, int *w, int *h,
                               bool *iconised = NULL, bool *maximised = NULL);

      //  save the given frame's position and size in config file
   static void SavePosition(const char *name, wxFrame *frame);
   static void SavePosition(const char *name, wxWindow *frame);

   /// dummy ctor for DECLARE_DYNAMIC_CLASS
   wxMFrame() : MFrameBase(M_EMPTYSTRING) { FAIL_MSG(_T("unreachable")); }
   /// Constructor
   wxMFrame(const String &iname, wxWindow *parent = NULL);
   /// Creates an object
   void Create(const String &iname, wxWindow *parent = NULL);
   /// Destructor
   ~wxMFrame();

   /// to enforce common style
   wxToolBar *CreateToolBar(void);
   /// return true if initialised
   bool  IsInitialised(void) const { return m_initialised; }

   /// make it visible or invisible
   bool Show(bool visible = true) { return wxFrame::Show(visible); }

   /// used to set the title of the window class
   void  SetTitle(String const & name);

   /**
      This virtual method returns a pointer to the profile of the mailfolder
      being displayed, for those wxMFrames which have a folder displayed or the
      global application profile for the other ones. Used to make the compose
      view inherit the current folder's settings.

      @return profile pointer, the caller must DecRef() it
   */
   virtual Profile *GetFolderProfile(void) const;

   /// Passes a menu id to modules for reacting to it.
   virtual bool ProcessModulesMenu(int id);

   /**
      Methods for adding standard menus to the frame menu bar.

      All frames should use AddFileMenu() and AddHelpMenu() and either
      AddMessageMenu() if they show a message or AddViewMenu() otherwise.
    */
   //@{
   virtual void AddFileMenu();
   virtual void AddHelpMenu();
   void AddEditMenu();
   void AddViewMenu();
   void AddMessageMenu();
   void AddLanguageMenu();
   //@}

   /// wxMFrame handles all print setup
   void OnPrintSetup();
   void OnPageSetup();
#ifdef USE_PS_PRINTING
   void OnPrintSetupPS();
   void OnPageSetupPS();
#endif // USE_PS_PRINTING

   // callbacks
   virtual void OnMenuCommand(int id);
   void OnCommandEvent(wxCommandEvent & event);
   void OnCloseWindow(wxCloseEvent& event);

protected:
   /// Flags for SaveState()
   enum
   {
      /// Geometry is always saved
      Save_Geometry = 0,

      /// Minimized/maximized/full-screen state of a frame
      Save_State = 1,

      /// Status of the tool/status bars
      Save_View = 2
   };

   /**
      Save state of this window in config.

      Window geometry is always saved, for the frames we can also save their
      minimized/maximized state as well as whether their tool/status bar should
      be shown.
    */
   static void SaveState(const char *name, wxWindow *frame, int flags);

   /**
      Create the tool and status bars if they are configured to be shown.

      This method calls the pure virtual DoCreateToolBar() and
      DoCreateStatusBar() if necessary.
    */
   void CreateToolAndStatusBars();

   /**
      Convenient function for derived classes: show the frame either normally
      or full-screen, depending on the last saved state.
    */
   void ShowInInitialState();

   /// is it initialised?
   bool m_initialised;

private:
   // implement these to create tool/status bars for this frame: they're called
   // by wxMFrame itself when the user decides to show a previously non-existent call them from
   // bar and also from the derived classes ctor via CreateToolAndStatusBars()
   virtual void DoCreateToolBar() = 0;
   virtual void DoCreateStatusBar() = 0;

   // get the config object with its path set to the frames option
   //
   // may return NULL during the initial program execution
   wxConfigBase *GetFrameOptionsConfig() const
   {
      return GetFrameOptionsConfig(MFrameBase::GetName());
   }

   // same as above except takes the frame name as parameter so it can be used
   // from the static methods
   static wxConfigBase *GetFrameOptionsConfig(const char *name);


#ifdef USE_PYTHON
   /// update the state (enabled/disabled) of the "Run Python script" menu item
   void UpdateRunPyScriptMenu();

   /// the object which calls our UpdateRunPyScriptMenu()
   friend class PythonOptionChangeHandler;
   PythonOptionChangeHandler *m_pyOptHandler;
#endif // USE_PYTHON

   DECLARE_EVENT_TABLE()
   DECLARE_ABSTRACT_CLASS(wxMFrame)
   DECLARE_NO_COPY_CLASS(wxMFrame)
};

#endif
