/*-*- c++ -*-********************************************************
 * wxMApp class: do all GUI specific  application stuff             *
 *                                                                  *
 * (C) 1997-2000 by Karsten Ballüder (karsten@phy.hw.ac.uk)         *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef  USE_PCH
#  include "Mcommon.h"
#  include "guidef.h"
#  include "MFrame.h"
#  include "gui/wxMFrame.h"
#  include "strutil.h"
#  include "kbList.h"
#  include "PathFinder.h"
#  include "Profile.h"
#  include "MApplication.h"
#  include "gui/wxMApp.h"

#  include "Mcclient.h"

#  include <wx/log.h>
#  include <wx/config.h>
#  include <wx/generic/dcpsg.h>
#  include <wx/thread.h>
#endif

#include <wx/msgdlg.h>   // for wxMessageBox
#include "wx/persctrl.h" // for wxPMessageBoxEnable(d)
#include <wx/ffile.h>
#include <wx/menu.h>
#include <wx/statusbr.h>
#include <wx/fs_mem.h>

#include <wx/dialup.h>

#include "MObject.h"

#include "Mdefaults.h"
#include "MDialogs.h"

#include "MHelp.h"

#include "gui/wxMainFrame.h"
#include "gui/wxIconManager.h"
#include "MailCollector.h"
#include "MModule.h"
#include "Mpers.h"

#include "MFCache.h"

#ifdef OS_WIN
#   include <winnls.h>

#   include "wx/html/helpctrl.h"
#endif

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// the name of config section where the log window position is saved
#define  LOG_FRAME_SECTION    "LogWindow"

// trace mask for timer events
#define TRACE_TIMER "timer"

// ----------------------------------------------------------------------------
// classes
// ----------------------------------------------------------------------------

// a small class implementing saving/restoring position for the log frame
class wxMLogWindow : public wxLogWindow
{
public:
   // ctor (creates the frame hidden)
   wxMLogWindow(wxFrame *pParent, const char *szTitle);

   // override base class virtual to implement saving the frame position and
   // to update the MP_SHOWLOG option
   virtual bool OnFrameClose(wxFrame *frame);
   virtual void OnFrameDelete(wxFrame *frame);

   // are we currently shown?
   bool IsShown() const;

private:
   bool m_hasWindow;
};

#ifdef wxHAS_LOG_CHAIN

// a wxLogStderr version which closes the file it logs to
class MLogFile : public wxLogStderr
{
public:
   MLogFile(FILE *fp) : wxLogStderr(fp) { }

   virtual ~MLogFile() { fclose(m_fp); }
};

#endif // wxHAS_LOG_CHAIN

// a timer used to periodically autosave profile settings
class AutoSaveTimer : public wxTimer
{
public:
   AutoSaveTimer() { m_started = FALSE; }

   virtual bool Start( int millisecs = -1, bool oneShot = FALSE )
      { m_started = TRUE; return wxTimer::Start(millisecs, oneShot); }

   virtual void Notify()
   {
      wxLogTrace(TRACE_TIMER, "Autosaving options and folder status.");

      Profile::FlushAll();

      MfStatusCache::Flush();
   }

    virtual void Stop()
      { if ( m_started ) wxTimer::Stop(); }

public:
    bool m_started;
};

// a timer used to periodically check for new mail
class MailCollectionTimer : public wxTimer
{
public:
   MailCollectionTimer() { m_started = FALSE; }

   virtual bool Start( int millisecs = -1, bool oneShot = FALSE )
      { m_started = TRUE; return wxTimer::Start(millisecs, oneShot); }

   virtual void Notify()
      {
         wxLogTrace(TRACE_TIMER, "Collection timer expired.");
         mApplication->UpdateOutboxStatus();
         MailCollector *collector = mApplication->GetMailCollector();
         if ( collector )
            collector->Collect();
      }

    virtual void Stop()
      { if ( m_started ) wxTimer::Stop(); }

public:
    bool m_started;
};

// a timer used to wake up idle loop to force generation of the idle events
// even if the program is otherwise idle (i.e. no GUI events happen)
class IdleTimer : public wxTimer
{
public:
   IdleTimer() : wxTimer() { Start(100); }

   virtual void Notify() { wxWakeUpIdle(); }
};

// and yet another timer for the away mode
class AwayTimer : public wxTimer
{
public:
   virtual void Notify()
   {
      wxLogTrace(TRACE_TIMER, "Going awya on timer");

      mApplication->SetAwayMode(TRUE);
   }
};

// ----------------------------------------------------------------------------
// global vars
// ----------------------------------------------------------------------------

// a (unique) autosave timer instance
static AutoSaveTimer *gs_timerAutoSave = NULL;

// a (unique) timer for polling for new mail
static MailCollectionTimer *gs_timerMailCollection = NULL;

// the timer for auto going into away mode: may be NULL
static AwayTimer *gs_timerAway = NULL;

struct MModuleEntry
{
   MModuleEntry(MModule *m) { m_Module = m; }
   MModule *m_Module;
};

KBLIST_DEFINE(ModulesList, MModuleEntry);

// a list of modules loaded at startup:
static ModulesList gs_GlobalModulesList;

// ============================================================================
// implementation
// ============================================================================

// this creates the one and only application object
IMPLEMENT_APP(wxMApp);

// ----------------------------------------------------------------------------
// wxMLogWindow
// ----------------------------------------------------------------------------

wxMLogWindow::wxMLogWindow(wxFrame *pParent, const char *szTitle)
            : wxLogWindow(pParent, szTitle, FALSE)
{
   int x, y, w, h;
   bool i;
   wxMFrame::RestorePosition(LOG_FRAME_SECTION, &x, &y, &w, &h, &i);
   wxFrame *frame = GetFrame();

   frame->SetSize(x, y, w, h);
   frame->SetIcon(ICON("MLogFrame"));
   m_hasWindow = true;

   // normally we want to iconize the frame before showing it to avoid
   // flicker, but it doesn't seem to work under GTK
#ifdef __WXGTK__
   Show();
   frame->Iconize(i);
#else // !GTK
   frame->Iconize(i);
   Show();
#endif // GTK/!GTK
}

bool wxMLogWindow::IsShown() const
{
   if ( !m_hasWindow )
      return false;

   return GetFrame()->IsShown();
}

bool wxMLogWindow::OnFrameClose(wxFrame *frame)
{
   // disable the log window
   mApplication->GetProfile()->writeEntry(MP_SHOWLOG, 0l);

   MDialog_Message(_("You have closed the log window and it will not be "
                     "opened automatically again any more.\n"
                     "To make it appear again, you should change the "
                     "corresponding setting in the\n"
                     "'Miscellaneous' page of the Preferences dialog."),
                   NULL,
                   MDIALOG_MSGTITLE,
                   GetPersMsgBoxName(M_MSGBOX_SHOWLOGWINHINT));

   return wxLogWindow::OnFrameClose(frame); // TRUE, normally
}

void wxMLogWindow::OnFrameDelete(wxFrame *frame)
{
   wxMFrame::SavePosition(LOG_FRAME_SECTION, frame);

   m_hasWindow = false;

   wxLogWindow::OnFrameDelete(frame);
}

#if 0
// ----------------------------------------------------------------------------
// wxMStatusBar
// ----------------------------------------------------------------------------
class wxMStatusBar : public wxStatusBar
{
public:
   wxMStatusBar(wxWindow *parent) : wxStatusBar(parent, -1)
      {
         m_show = false;
         m_OnlineIcon = new wxIcon;
         m_OfflineIcon = new wxIcon;
         *m_Onli+neIcon = ICON( "online");
         *m_OfflineIcon = ICON( "offline");
      }

   ~wxMStatusBar()
      {
         delete m_OnlineIcon;
         delete m_OfflineIcon;
      }
   void UpdateOnlineStatus(bool show, bool isOnline)
      {
         m_show = show; m_isOnline = isOnline;
//         Refresh();
      }
   virtual void DrawField(wxDC &dc, int i)
      {
         wxStatusBar::DrawField(dc, i);
         if(m_show)
         {
            int field = mApplication->GetStatusField(MAppBase::SF_ONLINE);
            if(i == field)
            {
               wxRect r;
               GetFieldRect(i, r);
               dc.DrawIcon(m_isOnline ?
                           *m_OnlineIcon
                           : *m_OfflineIcon,
                           r.x, r.y+2); // +2: small vertical offset
            }
         }
      }
private:
   wxIcon *m_OnlineIcon;
   wxIcon *m_OfflineIcon;
   /// show online status?
   bool m_show;
   /// are we online?
   bool m_isOnline;
};
#endif

// ----------------------------------------------------------------------------
// wxMApp
// ----------------------------------------------------------------------------


BEGIN_EVENT_TABLE(wxMApp, wxApp)
   EVT_IDLE                (wxMApp::OnIdle)
   EVT_DIALUP_CONNECTED    (wxMApp::OnConnected)
   EVT_DIALUP_DISCONNECTED (wxMApp::OnDisconnected)
END_EVENT_TABLE()

void
wxMApp::OnConnected(wxDialUpEvent &event)
{
   if(! m_DialupSupport)
      return;
   m_IsOnline = TRUE;
   UpdateOnlineDisplay();
   MDialog_Message(_("Dial-Up network connection established."),
                   m_topLevelFrame,
                   _("Information"),
                   "DialUpConnectedMsg");
}

void
wxMApp::OnDisconnected(wxDialUpEvent &event)
{
   if(! m_DialupSupport)
      return;
   m_IsOnline = FALSE;
   UpdateOnlineDisplay();
   MDialog_Message(_("Dial-Up network shut down."),
                   m_topLevelFrame,
                   _("Information"),
                   "DialUpDisconnectedMsg");
}

wxMApp::wxMApp(void)
{
   m_IconManager = NULL;
   m_HelpController = NULL;
   m_CanClose = FALSE;
   m_IdleTimer = NULL;
   m_OnlineManager = NULL;
   m_DialupSupport = FALSE;

   m_logWindow = NULL;
   m_logChain = NULL;
}

wxMApp::~wxMApp()
{
#ifdef wxHAS_LOG_CHAIN
   delete m_logChain;
#endif // wxHAS_LOG_CHAIN
}

void
wxMApp::OnIdle(wxIdleEvent &event)
{
   MEventManager::DispatchPending();
   event.Skip();
}

void
wxMApp::OnAbnormalTermination()
{
   MAppBase::OnAbnormalTermination();

   static const char *msg =
      gettext_noop("The application is terminating abnormally.\n"
                   "\n"
                   "Please report the bug via our bugtracker at\n"
                   "http://mahogany.sourceforge.net/bugz\n"
                   "Be sure to mention your OS version and what\n"
                   "exactly you were doing before this message\n"
                   "box appeared.\n"
                   "\n"
                   "Thank you!");
   static const char *title = gettext_noop("Fatal application error");

   // using a plain message box is safer in this situation, but under Unix we
   // have no such choice
#ifdef __WXMSW__
   ::MessageBox(NULL, _(msg), _(title), MB_ICONSTOP);
#else // !MSW
   wxMessageBox(_(msg), _(title), wxICON_STOP | wxOK);
#endif // MSW/!MSW
}

// can we close now?
bool
wxMApp::CanClose() const
{
   // If we get called the second time, we just reuse our old value, but only
   // if it was TRUE (if we couldn't close before, may be we can now)
   if ( m_CanClose )
      return m_CanClose;

   // verify that the user didn't accidentally remembered "No" as the answer to
   // the next message box - the problem with this is that in this case there
   // is no way to exit M at all!
   if ( !wxPMessageBoxEnabled(MP_CONFIRMEXIT) )
   {
      if ( !MDialog_YesNoDialog("", NULL, "", false, MP_CONFIRMEXIT) )
      {
         wxLogDebug("Exit confirmation msg box has been disabled on [No], "
                    "reenabling it.");

         wxPMessageBoxEnable(MP_CONFIRMEXIT, true);
      }
      //else: it was on [Yes], ok
   }

   // ask the user for confirmation
   if ( !MDialog_YesNoDialog(_("Do you really want to exit Mahogany?"),
                              m_topLevelFrame,
                              MDIALOG_YESNOTITLE,
                              false,
                              MP_CONFIRMEXIT) )
   {
      return false;
   }

   wxWindowList::Node *node = wxTopLevelWindows.GetFirst();
   while ( node )
   {
      wxWindow *win = node->GetData();
      node = node->GetNext();

      // We do not ask the toplevel frame as this would ask us again,
      // leading to some recursion.
      if ( win->IsKindOf(CLASSINFO(wxMFrame)) && win != m_topLevelFrame)
      {
         wxMFrame *frame = (wxMFrame *)win;

         if ( !IsOkToClose(frame) )
         {
            if ( !frame->CanClose() )
               return false;
         }
         //else: had been asked before
      }
      //else: what to do here? if there is something other than a frame
      //      opened, it might be a (non modal) dialog or may be the log
      //      frame?
   }


   // We assume that we can always close the toplevel frame if we make
   // it until here. Attemps to close the toplevel frame will lead to
   // this function being evaluated anyway. The frame itself does not
   // do any tests.
   ((MAppBase *)this)->AddToFramesOkToClose(m_topLevelFrame);

   ((wxMApp *)this)->m_CanClose = MAppBase::CanClose();
   return m_CanClose;
}

// do close the app by closing all frames
void
wxMApp::DoExit()
{
   // shut down MEvent handling
   if(m_IdleTimer)
   {
      m_IdleTimer->Stop();
      delete m_IdleTimer;
      m_IdleTimer = NULL; // paranoid
   }

   // flush the logs while we can
   wxLog::FlushActive();

   // before deleting the frames, make sure that all dialogs which could be
   // still hanging around don't do it any more
   wxTheApp->DeletePendingObjects();

   // close all windows
   wxWindowList::Node *node = wxTopLevelWindows.GetFirst();
   while ( node )
   {
      wxWindow *win = node->GetData();
      node = node->GetNext();

      // avoid closing the log window because it is so smart that it will not
      // reopen itself the next time we're run if we do it
      if ( (!m_logWindow || (win != m_logWindow->GetFrame()))
               && win->IsKindOf(CLASSINFO(wxFrame)) )
      {
         wxMFrame *frame = (wxMFrame *)win;

         // force closing the frame
         frame->Close(TRUE);
      }
   }
}

// app initilization
bool
wxMApp::OnInit()
{
#ifndef DEBUG
   wxHandleFatalExceptions();
#endif

#if wxCHECK_VERSION(2, 3, 2)
   if ( !wxApp::OnInit() )
      return false;
#endif // wxWin 2.3.2+

#ifdef OS_WIN
   // stupidly enough wxWin resets the default timestamp under Windows :-(
   wxLog::SetTimestamp(_T("%X"));
#endif // OS_WIN

   m_topLevelFrame = NULL;

#ifdef USE_I18N
   // Set up locale first, so everything is in the right language.
   bool hasLocale = false;

#ifdef OS_UNIX
   const char *locale = getenv("LANG");
   hasLocale =
      locale &&
      (strcmp(locale, "C") != 0) &&
      (strcmp(locale, "en") != 0) &&
      (strncmp(locale,"en_", 3) != 0) &&
      (strcmp(locale, "us") != 0);
#elif defined(OS_WIN)
   // this variable is not usually set under Windows, but give the user a
   // chance to override our locale detection logic in case it doesn't work
   // (which is well possible because it's totally untested)
   wxString locale = getenv("LANG");
   if ( locale.Length() > 0 )
   {
      // setting LANG to "C" disables all this
      hasLocale = (locale[0] != 'C') || (locale[1] != '\0');
   }
   else
   {
      // try to detect locale ourselves
      LCID lcid = GetThreadLocale(); // or GetUserDefaultLCID()?
      WORD idLang = PRIMARYLANGID(LANGIDFROMLCID(lcid));
      hasLocale = idLang != LANG_ENGLISH;
      if ( hasLocale )
      {
         static char s_countryName[256];
         int rc = GetLocaleInfo(lcid, LOCALE_SABBREVCTRYNAME,
                                s_countryName, WXSIZEOF(s_countryName));
         if ( !rc )
         {
            // this string is intentionally not translated
            wxLogSysError("Can not get current locale setting.");

            hasLocale = FALSE;
         }
         else
         {
            // TODO what if the buffer was too small?
            locale = s_countryName;
         }
      }
   }
#else // Mac?
#   error "don't know how to get the current locale on this platform."
#endif // OS

#ifdef __LINUX__
   /* This is a hack for SuSE Linux which sets LANG="german" instead
      of LANG="de". Once again, they do their own thing...
   */
   if( hasLocale && (wxStricmp(locale, "german") == 0) )
      locale = "de";
#endif // __LINUX__

   // if we fail to load the msg catalog, remember it in this variable because
   // we can't remember this in the profile yet - it is not yet created
   bool failedToLoadMsgs = false,
        failedToSetLocale = false;

   if ( hasLocale )
   {
      // TODO should catch the error messages and save them for later
      wxLogNull noLog;

      m_Locale = new wxLocale(locale, locale, NULL);
      if ( !m_Locale->IsOk() )
      {
         delete m_Locale;
         m_Locale = NULL;

         failedToSetLocale = true;
      }
      else
      {
#ifdef OS_UNIX
         String localePath;
         localePath << M_BASEDIR << "/locale";
#elif defined(OS_WIN)
         // we can't use InitGlobalDir() here as the profile is not created yet,
         // but I don't have time to fix it right now - to do later!!
         #error "this code is broken and will crash"  // FIXME

         InitGlobalDir();
         String localePath;
         localePath << m_globalDir << "/locale";
#else
#   error "don't know where to find message catalogs on this platform"
#endif // OS
         m_Locale->AddCatalogLookupPathPrefix(localePath);

         if ( !m_Locale->AddCatalog(M_APPLICATIONNAME) )
         {
            // better use English messages if msg catalog was not found
            delete m_Locale;
            m_Locale = NULL;

            failedToLoadMsgs = true;
         }
      }
   }
   else
   {
      m_Locale = NULL;
   }
#endif // USE_I18N

   wxInitAllImageHandlers();
   wxFileSystem::AddHandler(new wxMemoryFSHandler);

   m_IconManager = new wxIconManager();

   m_PrintData = new wxPrintData;
   m_PageSetupData = new wxPageSetupDialogData;

   // this is necessary to avoid that the app closes automatically when we're
   // run for the first time and show a modal dialog before opening the main
   // frame - if we don't do it, when the dialog (which is the last app window
   // at this moment) disappears, the app will close.
   SetExitOnFrameDelete(FALSE);

   // create timers -- are accessed by OnStartup()
   gs_timerAutoSave = new AutoSaveTimer;
   gs_timerMailCollection = new MailCollectionTimer;

   if ( OnStartup() )
   {
#ifdef USE_I18N
      // only now we can use profiles
      if ( failedToSetLocale || failedToLoadMsgs )
      {
         // if we can't load the msg catalog for the given locale once, don't
         // try it again next time - the flag is stored in the profile in this
         // key
         wxString configPath;
         configPath << "UseLocale_" << locale;

         Profile *profile = GetProfile();
         if ( profile->readEntry(configPath, 1l) != 0l )
         {
            CloseSplash();

            // the strings here are intentionally not translated
            wxString msg;
            if ( failedToSetLocale )
            {
               msg.Printf("Locale '%s' couldn't be set, do you want to "
                          "retry setting it the next time?",
                          locale);
            }
            else // failedToLoadMsgs
            {
               msg.Printf("Impossible to load message catalog(s) for the "
                          "locale '%s', do you want to retry next time?",
                          locale);
            }
            if ( wxMessageBox(msg, "Error",
                              wxICON_STOP | wxYES_NO) != wxYES )
            {
               profile->writeEntry(configPath, 0l);
            }
         }
         //else: the user had previously answered "no" to the question above,
         //      don't complain any more about missing catalogs
      }
#endif // USE_I18N

      // we want the main window to be above the log frame
      if ( IsLogShown() )
      {
         m_topLevelFrame->Raise();
      }

      // at this moment we're fully initialized, i.e. profile and log
      // subsystems are working and the main window is created

      // restore our favourite preferred printer settings
#if defined(__WXGTK__) || defined(__WXMOTIF__)
      (*m_PrintData) = * wxThePrintSetupData;
#endif

#if wxUSE_POSTSCRIPT
      GetPrintData()->SetPrinterCommand(READ_APPCONFIG(MP_PRINT_COMMAND));
      GetPrintData()->SetPrinterOptions(READ_APPCONFIG(MP_PRINT_OPTIONS));
      GetPrintData()->SetOrientation(READ_APPCONFIG(MP_PRINT_ORIENTATION));
      GetPrintData()->SetPrintMode((wxPrintMode)READ_APPCONFIG(MP_PRINT_MODE));
      GetPrintData()->SetPaperId((wxPaperSize)READ_APPCONFIG(MP_PRINT_PAPER));
      GetPrintData()->SetFilename(READ_APPCONFIG(MP_PRINT_FILE));
      GetPrintData()->SetColour(READ_APPCONFIG(MP_PRINT_COLOUR));
      GetPageSetupData()->SetMarginTopLeft(wxPoint(
         READ_APPCONFIG(MP_PRINT_TOPMARGIN_X),
         READ_APPCONFIG(MP_PRINT_TOPMARGIN_X)));
      GetPageSetupData()->SetMarginBottomRight(wxPoint(
         READ_APPCONFIG(MP_PRINT_BOTTOMMARGIN_X),
         READ_APPCONFIG(MP_PRINT_BOTTOMMARGIN_X)));
#endif // wxUSE_POSTSCRIPT

      // start a timer to autosave the profile entries
      StartTimer(Timer_Autosave);

      // the timer to poll for new mail will be started when/if MailCollector
      // is created
      StartTimer(Timer_PollIncoming);

      // start away timer if necessary
      StartTimer(Timer_Away);

      // another timer to do MEvent handling:
      m_IdleTimer = new IdleTimer;

      // restore the normal behaviour (see the comments above)
      SetExitOnFrameDelete(TRUE);

      // reflect settings in menu and statusbar:
      UpdateOnlineDisplay();

      // make sure this is displayed correctly:
      UpdateOutboxStatus();

      // show tip dialog unless disabled
      if ( READ_APPCONFIG(MP_SHOWTIPS) )
      {
         MDialog_ShowTip(GetTopWindow());
      }

      return true;
   }
   else
   {
      if ( GetLastError() != M_ERROR_CANCEL )
      {
         ERRORMESSAGE(("Can't initialize application, terminating."));
      }
      //else: this would be superfluous

      return false;
   }
}

MFrame *wxMApp::CreateTopLevelFrame()
{
   m_topLevelFrame = new wxMainFrame();
   m_topLevelFrame->Show(true);
   SetTopWindow(m_topLevelFrame);
   return m_topLevelFrame;
}

int wxMApp::OnRun()
{
#ifdef PROFILE_STARTUP
   return 0;
#else
   return wxApp::OnRun();
#endif
}

int wxMApp::OnExit()
{
   // disable timers for autosave and mail collection, won't need them any more
   StopTimer(Timer_Autosave);
   StopTimer(Timer_PollIncoming);

   // delete timers
   delete gs_timerAutoSave;
   delete gs_timerMailCollection;
   delete gs_timerAway;

   gs_timerAutoSave = NULL;
   gs_timerMailCollection = NULL;

#if wxUSE_POSTSCRIPT
   // save our preferred printer settings
   m_profile->writeEntry(MP_PRINT_COMMAND, GetPrintData()->GetPrinterCommand());
   m_profile->writeEntry(MP_PRINT_OPTIONS, GetPrintData()->GetPrinterOptions());
   m_profile->writeEntry(MP_PRINT_ORIENTATION, GetPrintData()->GetOrientation());
   m_profile->writeEntry(MP_PRINT_MODE, GetPrintData()->GetPrintMode());
   m_profile->writeEntry(MP_PRINT_PAPER, GetPrintData()->GetPaperId());
   m_profile->writeEntry(MP_PRINT_FILE, GetPrintData()->GetFilename());
   m_profile->writeEntry(MP_PRINT_COLOUR, GetPrintData()->GetColour());
   m_profile->writeEntry(MP_PRINT_TOPMARGIN_X, GetPageSetupData()->GetMarginTopLeft().x);
   m_profile->writeEntry(MP_PRINT_TOPMARGIN_Y, GetPageSetupData()->GetMarginTopLeft().y);
   m_profile->writeEntry(MP_PRINT_BOTTOMMARGIN_X, GetPageSetupData()->GetMarginBottomRight().x);
   m_profile->writeEntry(MP_PRINT_BOTTOMMARGIN_Y, GetPageSetupData()->GetMarginBottomRight().y);
#endif // wxUSE_POSTSCRIPT

   if(m_HelpController)
   {
      wxSize size;
      wxPoint pos;
      m_HelpController->GetFrameParameters(&size, &pos);
      m_profile->writeEntry(MP_HELPFRAME_WIDTH, size.x),
      m_profile->writeEntry(MP_HELPFRAME_HEIGHT, size.y);
      m_profile->writeEntry(MP_HELPFRAME_XPOS, pos.x);
      m_profile->writeEntry(MP_HELPFRAME_YPOS, pos.y);
      delete m_HelpController;
   }

   delete m_PrintData;
   delete m_PageSetupData;

   UnloadModules();
   MAppBase::OnShutDown();

   MModule_Cleanup();
   delete m_IconManager;

#ifdef USE_I18N
   delete m_Locale;
#endif // USE_I18N

   delete m_OnlineManager;

   // FIXME this is not the best place to do it, but at least we're safe
   //       because we now that by now it's unused any more
   Profile::DeleteGlobalConfig();

   MObjectRC::CheckLeaks();
   MObject::CheckLeaks();

   // delete the previously active log target (it's the one we had set before
   // here, but in fact it doesn't even matter: if somebody installed another
   // one, we will delete his log object and his code will delete ours)
   wxLog *log = wxLog::SetActiveTarget(NULL);
   delete log;
   return 0;
}

// ----------------------------------------------------------------------------
// Help subsystem
// ----------------------------------------------------------------------------

/* static */
wxString wxMApp::BuildHelpInitString(const wxString& dir)
{
   wxString helpfile = dir;

#ifdef OS_WIN
   helpfile += "\\Manual.hhp";
#endif // Windows

   return helpfile;
}

/* static */
wxString wxMApp::GetHelpDir()
{
   wxString helpdir = mApplication->GetGlobalDir();
   if ( !helpdir.empty() )
      helpdir += wxFILE_SEP_PATH;

#ifdef OS_WIN
   helpdir += "\\help";
#else // !Windows
   helpdir += "/doc";
#endif // Windows/!Windows

   return helpdir;
}

bool wxMApp::InitHelp()
{
   wxString helpdir = READ_APPCONFIG(MP_HELPDIR);
   if ( helpdir.empty() )
   {
      helpdir = GetHelpDir();
   }

   while ( !m_HelpController )
   {
      // we hardcode the help controller class we use instead of using the
      // default wxHelpController everywhere as we don't have docs in all
      // possible formats, just HTML and CHM
#ifdef OS_UNIX
      m_HelpController = new wxHelpController;
#else // Windows
      m_HelpController = new wxHtmlHelpController;
#endif // Unix/Windows

      // try to initialise the help system
      if ( !m_HelpController->Initialize(BuildHelpInitString(helpdir)) )
      {
         // failed
         delete m_HelpController;
         m_HelpController = NULL;

         // ask the user if we want to look elsewhere?
         wxString msg;
         msg << _("Cannot initialise help system:\n")
             << _("Help files not found in the directory '") << helpdir << "'\n"
             << "\n"
             << _("Would you like to specify another help files location?");

         if ( !MDialog_YesNoDialog(msg, NULL, _("Mahogany Help")) )
         {
            // can't do anything more
            return false;
         }

         String dir = MDialog_DirRequester
                      (
                       _("Please choose the directory containing "
                         "the Mahogany help files."),
                       helpdir
                      );

         if ( dir.empty() )
         {
            // dialog cancelled
            return false;
         }

         // try again with another location
         helpdir = dir;
      }
   }

   // set help viewer options

#if defined(OS_UNIX) && !wxUSE_HTML
   ((wxExtHelpController *)m_HelpController)->SetBrowser(
      READ_APPCONFIG(MP_HELPBROWSER),
      READ_APPCONFIG(MP_HELPBROWSER_ISNS));
#endif // using wxExtHelpController

   wxSize size = wxSize(READ_APPCONFIG(MP_HELPFRAME_WIDTH),
                        READ_APPCONFIG(MP_HELPFRAME_HEIGHT));
   wxPoint pos = wxPoint(READ_APPCONFIG(MP_HELPFRAME_XPOS),
                         READ_APPCONFIG(MP_HELPFRAME_YPOS));

   m_HelpController->SetFrameParameters("Mahogany : %s", size, pos);

   // remember the dir where we found the files
   if ( helpdir != GetHelpDir() )
   {
      mApplication->GetProfile()->writeEntry(MP_HELPDIR, helpdir);
   }

   // we do have m_HelpController now
   return true;
}

void
wxMApp::Help(int id, wxWindow *parent)
{
   // first thing: close splash if it's still there
   CloseSplash();

   if ( !m_HelpController && !InitHelp() )
   {
      // help controller couldn't be initialized, so no help available
      return;
   }

   // by now we should have either created it or returned
   CHECK_RET( m_HelpController, "no help controller, but should have one!" );

   switch(id)
   {
         // look up contents:
      case MH_CONTENTS:
         m_HelpController->DisplayContents();
         break;

         // implement a search:
      case MH_SEARCH:
         {
            // store help key used in search
            static wxString s_lastSearchKey;

            if ( MInputBox(&s_lastSearchKey,
                           _("Search for?"),
                           _("Search help for keyword"),
                           parent ? parent : TopLevelFrame())
                 && !s_lastSearchKey.IsEmpty() )
            {
               m_HelpController->KeywordSearch(s_lastSearchKey);
            }
         }
         break;

         // all other help ids, just look them up:
      default:
         if ( !m_HelpController->DisplaySection(id) )
         {
            wxLogWarning(_("No help found for current context (%d)."), id);
         }
   }
}

// ----------------------------------------------------------------------------
// Loadable modules
// ----------------------------------------------------------------------------

void
wxMApp::LoadModules(void)
{
   char *modulestring;
   kbStringList modules;
   kbStringList::iterator i;

   modulestring = strutil_strdup(READ_APPCONFIG(MP_MODULES));
   strutil_tokenise(modulestring, ":", modules);
   delete [] modulestring;

   MModule *module;
   for(i = modules.begin(); i != modules.end(); i++)
   {
      module = MModule::LoadModule(**i);
      if(module == NULL)
         ERRORMESSAGE((_("Cannot load module '%s'."),
                       (**i).c_str()));
      else
      {
         // remember it so we can decref it before exiting
         gs_GlobalModulesList.push_back(new MModuleEntry(module));
#ifdef DEBUG
         LOGMESSAGE((M_LOG_WINONLY,
                   "Successfully loaded module:\nName: %s\nDescr: %s\nVersion: %s\n",
                   module->GetName(),
                   module->GetDescription(),
                   module->GetVersion()));

#endif
      }
   }
}

void
wxMApp::InitModules(void)
{
   for ( ModulesList::iterator i = gs_GlobalModulesList.begin();
         i != gs_GlobalModulesList.end();
         i++ )
   {
      MModule *module = (**i).m_Module;
      if ( module->Entry(MMOD_FUNC_INIT) != 0 )
      {
         // TODO: propose to disable this module?
         wxLogWarning(_("Module '%s' failed to initialize."),
                      module->GetName());
      }
   }
}

void
wxMApp::UnloadModules(void)
{
   for (ModulesList::iterator j = gs_GlobalModulesList.begin();
       j != gs_GlobalModulesList.end();)
   {
      ModulesList::iterator i = j;
      ++j;
      (**i).m_Module->DecRef();
   }
}

// ----------------------------------------------------------------------------
// timer stuff
// ----------------------------------------------------------------------------

bool wxMApp::StartTimer(Timer timer)
{
   long delay;

   switch ( timer )
   {
      case Timer_Autosave:
         delay = READ_APPCONFIG(MP_AUTOSAVEDELAY);
         if ( delay )
         {
            return gs_timerAutoSave->Start(1000*delay);
         }
         break;

      case Timer_PollIncoming:
         delay = READ_APPCONFIG(MP_POLLINCOMINGDELAY);
         if ( delay )
         {
            return gs_timerMailCollection->Start(1000*delay);
         }
         break;

      case Timer_PingFolder:
         // TODO - just sending an event "restart timer" which all folders
         //        would be subscribed too should be enough
         return true;

      case Timer_Away:
         delay = READ_APPCONFIG(MP_AWAY_AUTO_ENTER);
         if ( delay )
         {
            // this timer is not created by default
            if ( !gs_timerAway )
               gs_timerAway = new AwayTimer;

            // this delay is in minutes
            return gs_timerAway->Start(1000*60*delay);
         }
         break;

      case Timer_Max:
      default:
         wxFAIL_MSG("attempt to start an unknown timer");
   }

   // timer not started
   return false;
}

bool wxMApp::StopTimer(Timer timer)
{
   switch ( timer )
   {
      case Timer_Autosave:
         gs_timerAutoSave->Stop();
         break;

      case Timer_PollIncoming:
         gs_timerMailCollection->Stop();
         break;

      case Timer_PingFolder:
         // TODO!!!
         break;

      case Timer_Away:
         if ( gs_timerAway )
            gs_timerAway->Stop();
         break;

      case Timer_Max:
      default:
         wxFAIL_MSG("attempt to stop an unknown timer");
         return false;
   }

   return true;
}

#ifndef USE_THREADS

void
wxMApp::ThrEnterLeave(bool /* enter */, SectionId /*what*/ , bool /*testing */)
{
   // nothing to do
}

#else

// this mutex must be acquired before any call to a critical c-client function
static wxMutex gs_CclientMutex;
// this mutex must be acquired before any call to MEvent code
static wxMutex gs_MEventMutex;

void
wxMApp::ThrEnterLeave(bool enter, SectionId what, bool
#ifdef DEBUG
                      testing
#endif
   )
{
   wxMutex *which = NULL;
   switch(what)
   {
   case GUI:
      /* case EVENTS: -- avoid multiple case value warning (EVENTS == GUI) */
      if(enter)
         wxMutexGuiEnter();
      else
         wxMutexGuiLeave();
      return;
      break;
   case MEVENT:
      which = &gs_MEventMutex;
      break;
   case CCLIENT:
      which = &gs_CclientMutex;
      break;
   default:
      FAIL_MSG("unsupported mutex id");
   }
   ASSERT(which);
   if(enter)
   {
      ASSERT(which->IsLocked() == FALSE);
      which->Lock();
   }
   else
   {
      ASSERT(which->IsLocked() || testing);
      if(which->IsLocked())
         which->Unlock();
   }
}

#endif

// ----------------------------------------------------------------------------
// Customize wxApp behaviour
// ----------------------------------------------------------------------------

#if wxCHECK_VERSION(2, 3, 0)

// never return splash frame from here
wxWindow *wxMApp::GetTopWindow() const
{
   wxWindow *win = wxApp::GetTopWindow();
   if ( win == g_pSplashScreen )
      win = NULL;

   return win;
}

#endif // 2.3.0

// return our icons
wxIcon
wxMApp::GetStdIcon(int which) const
{
   // this function may be (and is) called from the persistent controls code
   // which uses the global config object for its own needs, so avoid changing
   // its path here - or rather restore it on funciton exit
   class ConfigPathRestorer
   {
   public:
      ConfigPathRestorer()
         {
            if(mApplication->GetProfile())
               m_path = mApplication->GetProfile()->GetConfig()->GetPath();
         }

      ~ConfigPathRestorer()
         {
            if(mApplication->GetProfile())
               mApplication->GetProfile()->GetConfig()->SetPath(m_path);
         }

   private:
      wxString m_path;
   } storeConfigPath;

   // Set our icons for the dialogs.

   // This might happen during program shutdown, when the profile has
   // already been deleted or at startup before it is set up.
   if(! GetProfile() || GetGlobalDir().Length() == 0)
      return wxApp::GetStdIcon(which);

   // this ugly "#ifdefs" are needed to silent warning about "switch without
   // any case" warning under Windows
#ifndef OS_WIN
   switch(which)
   {
   case wxICON_HAND:
      return ICON("msg_error"); break;
   case wxICON_EXCLAMATION:
      return ICON("msg_warning"); break;
   case wxICON_QUESTION:
      return ICON("msg_question"); break;
   case wxICON_INFORMATION:
      return ICON("msg_info"); break;
   default:
      return wxApp::GetStdIcon(which);
   }
#else
   return wxApp::GetStdIcon(which);
#endif
}

void
wxMApp::UpdateOnlineDisplay(void)
{
   // Can be called during  application startup, in this case do
   // nothing:
   if(! m_topLevelFrame)
      return;
   wxStatusBar *sbar = m_topLevelFrame->GetStatusBar();
   wxMenuBar *mbar = m_topLevelFrame->GetMenuBar();
   ASSERT(sbar);
   ASSERT(mbar);

   if(! m_DialupSupport)
   {
      mbar->Enable((int)WXMENU_FILE_NET_ON, FALSE);
      mbar->Enable((int)WXMENU_FILE_NET_OFF, FALSE);
   }
   else
   {
      bool online = IsOnline();
      if(online)
      {
         mbar->Enable((int)WXMENU_FILE_NET_OFF, TRUE);
         mbar->Enable((int)WXMENU_FILE_NET_ON, FALSE);
//    m_topLevelFrame->GetToolBar()->EnableItem(WXMENU_FILE_NET_OFF, m_DialupSupport);
      }
      else
      {
         mbar->Enable((int)WXMENU_FILE_NET_ON, TRUE);
         mbar->Enable((int)WXMENU_FILE_NET_OFF, FALSE);
//    m_topLevelFrame->GetToolBar()->EnableItem(WXMENU_FILE_NET_ON, m_DialupSupport);
      }
      int field = GetStatusField(SF_ONLINE);
      UpdateStatusBar(field+1, TRUE);
      sbar->SetStatusText(online ? _("Online"):_("Offline"), field);
   }
}

void
wxMApp::UpdateStatusBar(int nfields, bool isminimum) const
{
   ASSERT(nfields <= SF_MAXIMUM);
   ASSERT(nfields >= 0);
   ASSERT(m_topLevelFrame);
   ASSERT(m_topLevelFrame->GetStatusBar());
   int n = nfields;

   // ugly, but effective:
   //static int statusIconWidth = -1;

   wxStatusBar *sbar = m_topLevelFrame->GetStatusBar();
   if(isminimum && sbar->GetFieldsCount() > nfields)
      n = sbar->GetFieldsCount();

   int widths[SF_MAXIMUM];
   widths[0] = 100; //flexible
   widths[1] = 10; // small empty field

#if 0
   if(m_DialupSupport)
   {
      if(statusIconWidth == -1)
      {
         const wxIcon & icon = ICON("online");
         // make field 1.5 times as wide as icon and add border:
         statusIconWidth = (3*icon.GetWidth())/2 + 2*sbar->GetBorderX();
      }
      widths[GetStatusField(SF_ONLINE)] = statusIconWidth;
   }
#endif
   if(m_DialupSupport)
      widths[GetStatusField(SF_ONLINE)] = 70;
   if(READ_APPCONFIG(MP_USE_OUTBOX))
      widths[GetStatusField(SF_OUTBOX)] = 100;
   //FIXME: wxGTK crashes after calling this repeatedly sbar->SetFieldsCount(n, widths);
}

void
wxMApp::UpdateOutboxStatus(MailFolder *mf) const
{
   if(! m_topLevelFrame) // called when flushing events at program end?
      return;

   UIdType nNNTP, nSMTP;
   bool enable = CheckOutbox(&nSMTP, &nNNTP, mf);

   // only enable menu item if outbox is used and contains messages:
   ASSERT(m_topLevelFrame->GetMenuBar());

   bool useOutbox = READ_APPCONFIG(MP_USE_OUTBOX) != 0;
   m_topLevelFrame->GetMenuBar()->Enable(
      (int)WXMENU_FILE_SEND_OUTBOX,enable && useOutbox);

   if ( !useOutbox )
         return;

   // update status bar
   String msg;
   if(nNNTP == 0 && nSMTP == 0)
      msg = _("Outbox empty");
   else
      msg.Printf(_("Outbox %lu, %lu"),
                 (unsigned long) nSMTP,
                 (unsigned long) nNNTP);

   int field = GetStatusField(SF_OUTBOX);
   UpdateStatusBar(field+1, TRUE);
   ASSERT(m_topLevelFrame->GetStatusBar());
   m_topLevelFrame->GetStatusBar()->SetStatusText(msg, field);
}

// ----------------------------------------------------------------------------
// dial up support
// ----------------------------------------------------------------------------

void
wxMApp::SetupOnlineManager(void)
{
   // only create dial up manager if needed
   m_DialupSupport = READ_APPCONFIG(MP_DIALUP_SUPPORT) != 0;

   if ( m_DialupSupport )
   {
      if ( !m_OnlineManager )
      {
         m_OnlineManager = wxDialUpManager::Create();
      }

      if(! m_OnlineManager->EnableAutoCheckOnlineStatus(60))
      {
         wxLogError(_("Cannot activate auto-check for dial-up network status."));
      }

      String beaconhost = READ_APPCONFIG(MP_BEACONHOST);
      strutil_delwhitespace(beaconhost);
      // If no host configured, use smtp host:
      if(beaconhost.length() > 0)
      {
         m_OnlineManager->SetWellKnownHost(beaconhost);
      }
      else // beacon host configured
      {
         beaconhost = READ_APPCONFIG(MP_SMTPHOST);
         m_OnlineManager->SetWellKnownHost(beaconhost, 25);
      }
#ifdef OS_UNIX
      m_OnlineManager->SetConnectCommand(
         READ_APPCONFIG(MP_NET_ON_COMMAND),
         READ_APPCONFIG(MP_NET_OFF_COMMAND));
#endif // Unix
      m_IsOnline = m_OnlineManager->IsOnline();
   }
   else // no dialup support
   {
      delete m_OnlineManager;

      m_IsOnline = TRUE;
   }

   UpdateOnlineDisplay();
}

bool
wxMApp::IsOnline(void) const
{
   if(! m_DialupSupport)
      return TRUE; // no dialup--> always connected

   // make sure we always have the very latest value:
   ((wxMApp*)this)->m_IsOnline = m_OnlineManager->IsOnline();

   return m_IsOnline;
}

void
wxMApp::GoOnline(void) const
{
   CHECK_RET( m_OnlineManager, "can't go online" );

   if(m_OnlineManager->IsOnline())
   {
      ((wxMApp *)this)->m_IsOnline = TRUE;
      ERRORMESSAGE((_("Dial-up network is already online.")));
      return;
   }

   m_OnlineManager->Dial();
}

void
wxMApp::GoOffline(void) const
{
   CHECK_RET( m_OnlineManager, "can't go offline" );

   if(! m_OnlineManager->IsOnline())
   {
      ((wxMApp *)this)->m_IsOnline = FALSE;
      ERRORMESSAGE((_("Dial-up network is already offline.")));
      return;
   }
   m_OnlineManager->HangUp();
   if( m_OnlineManager->IsOnline())
   {
      ERRORMESSAGE((_("Attempt to shut down network seems to have failed.")));
   }
}

void
wxMApp::FatalError(const char *message)
{
   OnAbnormalTermination();
}

void
wxMApp::SetAwayMode(bool isAway)
{
   MAppBase::SetAwayMode(isAway);

   // update the frame menu
   if ( m_topLevelFrame )
   {
      wxMenuBar *mbar = m_topLevelFrame->GetMenuBar();
      if ( mbar )
      {
         mbar->Check(WXMENU_FILE_AWAY_MODE, isAway);
      }

      wxLogStatus(m_topLevelFrame,
                  isAway ? _("Mahogany is now in unattended mode")
                         : _("Mahogany is not in unattended mode any more"));
   }

   // also change the mode of the log target: in away mode we only show the
   // messages in the log window and don't popup the dialogs
   if ( m_logWindow )
   {
      // show all old messages normally (including the one logged above)
      m_logWindow->Flush();

      m_logWindow->PassMessages(!isAway);
   }
}

// ----------------------------------------------------------------------------
// log window support (see also wxMLogWindow)
// ----------------------------------------------------------------------------

void wxMApp::SetLogFile(const String& filename)
{
#ifdef wxHAS_LOG_CHAIN
   if ( filename.empty() )
   {
      if ( m_logChain )
      {
         // just disable logging to the old file and only pass messages
         // through to the next log target
         m_logChain->SetLog(NULL);
      }
   }
   else // log to file
   {
      wxFFile file(filename, "w");
      if ( !file.IsOpened() )
      {
         wxLogError(_("Failed to open log file for writing."));
      }
      else
      {
         wxLog *log = new MLogFile(file.fp());

         // leave the file opened
         file.Detach();

         if ( m_logChain )
         {
            m_logChain->SetLog(log);
         }
         else
         {
            m_logChain = new wxLogChain(log);
         }

         wxLogVerbose(_("Started logging to the log file '%s'."),
                      filename.c_str());
      }
   }
#endif // wxHAS_LOG_CHAIN
}

bool wxMApp::IsLogShown() const
{
   return m_logWindow && m_logWindow->IsShown();
}

void wxMApp::ShowLog(bool doShow)
{
   if ( !m_logWindow )
   {
      if ( doShow )
      {
         // close the splash first as otherwise we get a subtle bug: the
         // splash screen installs its own (temp) log handler and deletes itin
         // its dtor, however if we install ourselves as the log handler in
         // the meanwhile, _we_ will be deleted - which we don't want to
         // happen
         CloseSplash();

         // before creating the log window, force auto creation of the default
         // GUI logger so that log window would pass messages to it
         (void)wxLog::GetActiveTarget();

         // create the log window
         m_logWindow = new wxMLogWindow(m_topLevelFrame,
                                        _("Mahogany : Activity Log"));
      }
      //else: nothing to do
   }
   else
   {
      // reuse the existing one
      m_logWindow->Show(doShow);
   }
}

