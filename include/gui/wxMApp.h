///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   gui/wxMApp.h - GUI-specific part of application logic
// Purpose:     declares wxMApp class implementing MApplication ABC
// Author:      Karsten Ballüder, Vadim Zeitlin
// Modified by:
// Created:     1997
// CVS-ID:      $Id$
// Copyright:   (c) 1997-2002 M-Team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _WXMAPP_H_
#define _WXMAPP_H_

#ifndef USE_PCH
#  include "MApplication.h"

#ifdef OS_WIN // cygwin and mingw
#  undef Yield // otherwise we get include/wx/app.h:182: arguments given to macro `Yield'
#endif
#  include <wx/app.h>
// it includes windows.h which defines SendMessage under Windows
#ifdef OS_WIN // cygwin and mingw
#  undef SendMessage
#endif

#  include <wx/help.h>
#  include <wx/icon.h>
#endif  //USE_PCH

// fwd decl
#ifdef USE_DIALUP
   class WXDLLEXPORT wxDialUpEvent;
   class WXDLLEXPORT wxDialUpManager;
#endif // USE_DIALUP
class WXDLLEXPORT wxLocale;
class WXDLLEXPORT wxLog;
class WXDLLEXPORT wxLogChain;
class WXDLLEXPORT wxHelpControllerBase;
class WXDLLEXPORT wxPrintData;
class WXDLLEXPORT wxServerBase;
class WXDLLEXPORT wxSingleInstanceChecker;
class WXDLLEXPORT wxTimer;

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
   virtual wxMFrame *CreateTopLevelFrame();
   virtual void OnFatalException() { OnAbnormalTermination(); }
   virtual void OnAbnormalTermination(const char *msg = NULL);

   virtual bool StartTimer(Timer timer);
   virtual bool StopTimer(Timer timer);

   virtual bool IsLogShown() const;
   virtual void ShowLog(bool doShow = TRUE);
   virtual void SetLogFile(const String& filename);

   // program termination helpers
   virtual bool CanClose() const;
   virtual void OnClose();

   // wxWin calls these functions to start/run/stop the application
   virtual bool OnInit();
   virtual int  OnRun();
   virtual int  OnExit();

   // methods used for the command line parsing
   virtual void OnInitCmdLine(wxCmdLineParser& parser);
   virtual bool OnCmdLineParsed(wxCmdLineParser& parser);

   // this function is virtual only in 2.3.0
#if wxCHECK_VERSION(2, 3, 0)
   // override top level window detection: never return splash frame from here
   // as it is transient and so is not suitable for use as a parent for the
   // dialogs (it can disappear before the dialog is closed)
   virtual wxWindow *GetTopWindow() const;
#endif // 2.3.0

   // override wxWindows default icons
   virtual wxIcon GetStdIcon(int which) const;

   /**
     OnIdle() handler to process Mahogany-specific MEvents which are
     asynchronous.
   */
   void OnIdle(wxIdleEvent& event);

   /**
     Handles the session termination confirmation event by asking all top level
     windows if they can close.
    */
   void OnQueryEndSession(wxCloseEvent& event);

   /** Gets help for a specific topic.
       @param id help id from MHelp.h
   */
   virtual void Help(int id, wxWindow *parent = NULL);

   /// return a pointer to the IconManager:
   wxIconManager *GetIconManager(void) const;

   /// Destructor
   ~wxMApp();

   /// get a pointer to the print data
   virtual const wxPrintData *GetPrintData();

   /// store the print data (after the user modified it)
   virtual void SetPrintData(const wxPrintData& printData);

   /// get the page setup data
   virtual wxPageSetupDialogData *GetPageSetupData();

   /// change it
   virtual void SetPageSetupData(const wxPageSetupDialogData& data);

   /** @name Thread control */
   //@{
   virtual void ThrEnter(SectionId what) { ThrEnterLeave(TRUE, what, FALSE); }
   virtual void ThrLeave(SectionId what, bool testing) { ThrEnterLeave(FALSE, what, testing); }
   //@}

   wxHelpControllerBase *GetHelpController(void) const
      { return m_HelpController; }

#ifdef USE_DIALUP
   virtual bool IsOnline(void) const;
   virtual void GoOnline(void) const;
   virtual void GoOffline(void) const;

   void OnConnected(wxDialUpEvent &event);
   void OnDisconnected(wxDialUpEvent &event);
#endif // USE_DIALUP

   virtual bool AllowBgProcessing() const;
   virtual void EnterCritical();
   virtual void LeaveCritical();

   /// updates display of outbox status
   virtual void UpdateOutboxStatus(class MailFolder *mf = NULL) const;

   virtual void SetAwayMode(bool isAway = true);

   // multiple program instances handling
   virtual bool IsAnotherRunning() const;
   virtual bool CallAnother();
   virtual bool SetupRemoteCallServer();

   bool OnRemoteRequest(const wxChar *request);

   /// Report a fatal error:
   virtual void FatalError(const wxChar *message);

#ifdef __WXDEBUG__
   virtual void OnAssert(const wxChar *file, int line,
                         const wxChar *cond, const wxChar *msg);
#endif // __WXDEBUG__

   virtual bool Yield(bool onlyIfNeeded = FALSE);

protected:
   virtual void RecreateStatusBar();

#ifdef USE_DIALUP
   /// sets up the class handling dial up networking
   virtual void SetupOnlineManager(void);

   /// update display of online connection status
   void UpdateOnlineDisplay();
#endif // USE_DIALUP

   /** Common code for ThrEnter and ThrLeave, if enter==TRUE, enter,
       otherwise leave.
   */
   void ThrEnterLeave(bool enter, SectionId what, bool testing);
   /// Load modules at startup
   virtual void LoadModules(void);
   /// Init modules - called as soon as the program is fully initialized
   virtual void InitModules(void);
   /// Unload modules loaded at startup
   virtual void UnloadModules(void);

private:
   /// common part of OnExit() and dtor, i.e. cleanup which is always done
   void DoCleanup();

   /**
     @name Help subsystem
   */
   //@{

   /// initialize the help controller, return true only if ok
   bool InitHelp();

   /// construct the helpfile name from the dir name
   static wxString BuildHelpInitString(const wxString& dir);

   /// get the default help directory
   static wxString GetHelpDir();

   /// an iconmanager instance
   wxIconManager *m_IconManager;

   /// a help controller instance
   wxHelpControllerBase *m_HelpController;

   //@}

#ifdef USE_I18N
   /// a locale for translation
   wxLocale *m_Locale;
#endif // USE_I18N

   /**
     @name Printing
   */
   //@{

   /// save the printing parameters
   void CleanUpPrintData();

   /// data for printing
   wxPrintData *m_PrintData;
   /// page setup for printing
   wxPageSetupDialogData *m_PageSetupData;

   //@}

   /// to recycle the last CanClose() result
   bool m_CanClose;

   /// timer used to call OnIdle for MEvent handling
   wxTimer *m_IdleTimer;

#ifdef USE_DIALUP
   /// online manager
   wxDialUpManager *m_OnlineManager;

   /// are we currently online?
   bool m_IsOnline;
#endif // USE_DIALUP

   /** @name Log data */
   //@{

   /// the log window (may be NULL)
   class wxMLogWindow *m_logWindow;

   /// the chaining log target for file logging (may be NULL)
   wxLogChain *m_logChain;

   //@}

   /// @name IPC data
   //@{

   /// the wxIPC server
   wxServerBase *m_serverIPC;

   /// object used to check if another program instance is running
   wxSingleInstanceChecker *m_snglInstChecker;

   //@}

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxMApp)
};

// ----------------------------------------------------------------------------
// global application object
// ----------------------------------------------------------------------------

// created dynamically by wxWindows
DECLARE_APP(wxMApp);

#endif // _WXMAPP_H_

