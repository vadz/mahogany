/*-*- c++ -*-********************************************************
 * wxMAppl class: all GUI specific application stuff                *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                 *
 *******************************************************************/
#ifndef WXMAPP_H
#define WXMAPP_H

#ifndef USE_PCH
#  define   Uses_wxApp
#  include  <wx/wx.h>
#  include  <MApplication.h>
#endif  //USE_PCH

// fwd decl
class wxLog;

// ----------------------------------------------------------------------------
// wxMApp
// ----------------------------------------------------------------------------

/**
  * A wxWindows implementation of MApplication class.
  */

class wxMApp : public wxApp, public MAppBase
{
public:
   /// Constructor
   wxMApp();

   /// create the main application window
   virtual MFrame *CreateTopLevelFrame();

   // wxWin callback
#ifdef  USE_WXWINDOWS2
   virtual bool OnInit();
#else   // wxWin1
   virtual wxFrame *OnInit(void);
#endif  // wxWin ver

   /// Destructor
   ~wxMApp();
};

// ----------------------------------------------------------------------------
// global application object
// ----------------------------------------------------------------------------
#ifdef  USE_WXWINDOWS2
   // created dynamically by wxWindows
   DECLARE_APP(wxMApp);

#  define mApplication (wxGetApp())
#else
   // global variable
   extern MApplication mApplication;
#endif

#endif
