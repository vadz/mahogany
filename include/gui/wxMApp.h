/*-*- c++ -*-********************************************************
 * wxMAppl class: all GUI specific application stuff                *
 *                                                                  *
 * (C) 1997-1999 by Karsten Ballüder (Balluedergmx.net)             *
 *                                                                  *
 * $Id$
 *******************************************************************/
#ifndef WXMAPP_H
#define WXMAPP_H

#ifndef USE_PCH
#  include  <wx/app.h>
#  include  <wx/help.h>
#  include  <wx/icon.h>
#  include  "MApplication.h"

#  include <wx/cmndata.h>  // for wxPageSetupData, can't fwd declare it
#endif  //USE_PCH

#  include  <wx/dialup.h>

// fwd decl
class WXDLLEXPORT wxLog;
class WXDLLEXPORT wxHelpController;
class WXDLLEXPORT wxPrintData;
class wxIconManager;

/// Define this to have an online status icon in the statusbar
#ifdef DEBUG
#   define USE_STATUSBARICON
#endif


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

   virtual bool StartTimer(Timer timer);
   virtual bool StopTimer(Timer timer);

   virtual bool IsLogShown() const;
   virtual void ShowLog(bool doShow = TRUE);

   // override this to return true only if all frames can close
   virtual bool CanClose() const;

   // wxWin calls these functions to start/run/stop the application
   virtual bool OnInit();
   virtual int  OnRun();
   virtual int  OnExit();

   // this function is virtual only in 2.3.0
#if wxCHECK_VERSION(2, 3, 0)
   // override top level window detection: never return splash frame from here
   // as it is transient and so is not suitable for use as a parent for the
   // dialogs (it can disappear before the dialog is closed)
   virtual wxWindow *GetTopWindow() const;
#endif // 2.3.0

   // override wxWindows default icons
   virtual wxIcon GetStdIcon(int which) const;

   // OnIdle() handler to process Mahogany-specific MEvents which are
   // asynchronous.
   void OnIdle(wxIdleEvent &event);

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

   /** @name Thread control */
   //@{
   virtual void ThrEnter(SectionId what) { ThrEnterLeave(TRUE, what, FALSE); }
   virtual void ThrLeave(SectionId what, bool testing) { ThrEnterLeave(FALSE, what, testing); }
   //@}

   wxHelpController *GetHelpController(void)
      { return m_HelpController; }

   virtual bool IsOnline(void) const;
   virtual void GoOnline(void) const;
   virtual void GoOffline(void) const;

   void OnConnected(wxDialUpEvent &event);
   void OnDisconnected(wxDialUpEvent &event);
   /// updates display of outbox status
   virtual void UpdateOutboxStatus(class MailFolder *mf = NULL) const;

   /// Report a fatal error:
   virtual void FatalError(const char *message);
protected:
   /// makes sure the status bar has enough fields
   virtual void UpdateStatusBar(int nfields, bool isminimum = FALSE) const;
   /// sets up the class handling dial up networking
   virtual void SetupOnlineManager(void);
   /** Common code for ThrEnter and ThrLeave, if enter==TRUE, enter,
       otherwise leave.
   */
   void ThrEnterLeave(bool enter, SectionId what, bool testing);
   /// Load modules at startup
   virtual void LoadModules(void);
   /// Unload modules loaded at startup
   virtual void UnloadModules(void);

   /// update display of online connection status
   void UpdateOnlineDisplay();
private:
   // implement base class pure virtual
   virtual void DoExit();

   /// an iconmanager instance
   wxIconManager *m_IconManager;
   /// a help controller instance
   wxHelpController *m_HelpController;
   /// a locale for translation
   class wxLocale *m_Locale;
   /// data for printing
   wxPrintData *m_PrintData;
   /// page setup for printing
   wxPageSetupData *m_PageSetupData;
   /// to recylce the last CanClose() result
   bool m_CanClose;
   /// timer used to call OnIdle for MEvent handling
   class wxTimer *m_IdleTimer;
   /// online manager
   class wxDialUpManager *m_OnlineManager;
   /// are we currently online?
   bool m_IsOnline;

   /// the log window (may be NULL)
   class wxMLogWindow *m_logWindow;

   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// global application object
// ----------------------------------------------------------------------------

// created dynamically by wxWindows
DECLARE_APP(wxMApp);

#endif
