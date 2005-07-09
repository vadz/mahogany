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

#ifdef __GNUG__
#   pragma interface "wxMFrame.h"
#endif

#ifndef  USE_PCH
#   include  "MFrame.h"
#   include  "guidef.h"
#endif

#include <wx/frame.h>

// enable workaround broken maximize?
#if defined(__WXMSW__) && !wxCHECK_VERSION(2, 3, 0)
   #define USE_WORKAROUND_FOR_MAXIMIZE
#endif // wxMSW < 2.3.0

class Profile;

/**
  * A wxWindows Frame class
  */

class wxMFrame : public wxFrame, public MFrameBase
{
public:
   // static helper functions (static because they're also used by wxLogWindow
   // which doesn't derive from wxMFrame)
      // read the frame position and size from config (the variables needn't be
      // initialized, they will receive some values in any case), returns FALSE
      // if there is no config object to read settings from
   static bool RestorePosition(const wxChar *name,
                               int *x, int *y, int *w, int *h,
                               bool *iconised = NULL, bool *maximised = NULL);

      //  save the given frame's position and size in config file
   static void SavePosition(const wxChar *name, wxFrame *frame);
   static void SavePosition(const wxChar *name, wxWindow *frame);

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

   // if we use this hack, Show() is implemented below
#ifndef USE_WORKAROUND_FOR_MAXIMIZE
   /// make it visible or invisible
   bool Show(bool visible = true) { return wxFrame::Show(visible); }
#endif // USE_WORKAROUND_FOR_MAXIMIZE

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

   /// add a menu to the bar
   virtual void AddFileMenu(void);
   virtual void AddHelpMenu(void);
   void AddEditMenu(void);
   void AddMessageMenu(void);
   void AddLanguageMenu(void);

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
   static void SavePositionInternal(const wxChar *name, wxWindow *frame, bool isFrame);

   /// is it initialised?
   bool m_initialised;

#ifdef USE_PYTHON
   /// update the state (enabled/disabled) of the "Run Python script" menu item
   void UpdateRunPyScriptMenu();

   /// the object which calls our UpdateRunPyScriptMenu()
   friend class PythonOptionChangeHandler;
   PythonOptionChangeHandler *m_pyOptHandler;
#endif // USE_PYTHON

   DECLARE_EVENT_TABLE()
   DECLARE_DYNAMIC_CLASS_NO_COPY(wxMFrame)
};

#endif
