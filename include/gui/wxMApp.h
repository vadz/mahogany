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
#  include  <wx/help.h>
#  include  "MApplication.h"
#endif  //USE_PCH

// fwd decl
class wxLog;
class wxIconManager;
class wxHelpController;

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

   /// implement base class virtuals
   virtual MFrame *CreateTopLevelFrame();
   virtual void OnFatalException() { OnAbnormalTermination(); }
   virtual void OnAbnormalTermination();

   // override this to return true only if all frames can close
   virtual bool CanClose() const;

   // wxWin calls these functions to start/run/stop the application
   virtual bool OnInit();
   virtual int  OnRun();
   virtual int  OnExit();

   /** Gets help for a specific topic.
       @param id help id from MHelp.h
   */
   virtual void Help(int id, wxWindow *parent = NULL);

   /// return a pointer to the IconManager:
   wxIconManager *GetIconManager(void) const { return m_IconManager; }

   /// Destructor
   ~wxMApp();

   /// get a pointer to the print data
   wxPrintData * GetPrintData(void) { return m_PrintData; }
   /// get the page setup data
   wxPageSetupData * GetPageSetupData(void) { return m_PageSetupData; }

private:
   /// an iconmanager instance
   wxIconManager *m_IconManager;
   /// a help controller instance
   wxHelpController *m_HelpController;
   /// a locale for translation
   class wxLocale *m_Locale;
   /// data for printing
   wxPrintData *m_PrintData;
   // page setup for printing
   wxPageSetupData *m_PageSetupData;
};

// ----------------------------------------------------------------------------
// global application object
// ----------------------------------------------------------------------------

// created dynamically by wxWindows
DECLARE_APP(wxMApp);

#endif
