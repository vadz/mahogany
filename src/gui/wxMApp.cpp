///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   gui/wxMApp.cpp - GUI-specific part of application logic
// Purpose:     program application, startup, non-portable hooks to implement
//              the functionality needed by MApplication, also log window impl
// Author:      Karsten Ballüder, Vadim Zeitlin
// Modified by:
// Created:     1997
// CVS-ID:      $Id$
// Copyright:   (c) 1997-2002 M-Team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef  USE_PCH
#  include "Mcommon.h"
#  include "gui/wxMApp.h"
#  include "strutil.h"
#  include "kbList.h"
#  include "MHelp.h"
#  include "gui/wxIconManager.h"
#  include "Mdefaults.h"
#  include "Profile.h"

#  include <wx/msgdlg.h>   // for wxMessageBox
#  include <wx/menu.h>
#  include <wx/statusbr.h>
#endif // USE_PCH

#include "wx/persctrl.h" // for wxPMessageBoxEnable
#include <wx/ffile.h>
#include <wx/fs_mem.h>
#include <wx/cmdline.h>
#include <wx/encconv.h>         // for wxEncodingConverter

#ifdef USE_DIALUP
#  include <wx/dialup.h>
#endif // USE_DIALUP

#include <wx/fontmap.h>
#include <wx/tokenzr.h>

#include <wx/snglinst.h>

#include <wx/generic/helpext.h>

#if defined(OS_WIN) || defined(__CYGWIN__)
   #define wxConnection    wxDDEConnection
   #define wxServer        wxDDEServer
   #define wxClient        wxDDEClient

   #include <wx/dde.h>
#else // !Windows
   #define wxConnection    wxTCPConnection
   #define wxServer        wxTCPServer
   #define wxClient        wxTCPClient

   #include <wx/sckipc.h>
#endif // Windows/!Windows

#include "PathFinder.h"

#include "gui/wxMDialogs.h"
#include "gui/wxMainFrame.h"
#include "FolderMonitor.h"
#include "MModule.h"
#include "MThread.h"
#include "Mpers.h"

#include "Composer.h"       // for SaveAll()

#include "MFCache.h"

#include "CmdLineOpts.h"

#ifdef OS_WIN
   #include <winnls.h>

   #include "wx/msw/helpbest.h"

   #ifdef wxHAS_CRASH_REPORT
      #include "wx/msw/crashrpt.h"
   #endif

   #define UNUSED_IF_WIN(arg)
#else
   #define UNUSED_IF_WIN(arg) arg
#endif

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_AFMPATH;
extern const MOption MP_AUTOSAVEDELAY;
extern const MOption MP_AWAY_AUTO_ENTER;
extern const MOption MP_BEACONHOST;
extern const MOption MP_CONFIRMEXIT;
#ifdef USE_DIALUP
extern const MOption MP_DIALUP_SUPPORT;
#endif // USE_DIALUP
extern const MOption MP_HELPBROWSER_KIND;
extern const MOption MP_HELPBROWSER;
extern const MOption MP_HELPBROWSER_ISNS;
extern const MOption MP_HELPDIR;
extern const MOption MP_HELPFRAME_HEIGHT;
extern const MOption MP_HELPFRAME_WIDTH;
extern const MOption MP_HELPFRAME_XPOS;
extern const MOption MP_HELPFRAME_YPOS;
extern const MOption MP_MODULES;
extern const MOption MP_NET_OFF_COMMAND;
extern const MOption MP_NET_ON_COMMAND;
extern const MOption MP_POLLINCOMINGDELAY;
extern const MOption MP_PRINT_BOTTOMMARGIN_X;
extern const MOption MP_PRINT_BOTTOMMARGIN_Y;
extern const MOption MP_PRINT_COLOUR;
extern const MOption MP_PRINT_COMMAND;
extern const MOption MP_PRINT_FILE;
extern const MOption MP_PRINT_MODE;
extern const MOption MP_PRINT_OPTIONS;
extern const MOption MP_PRINT_ORIENTATION;
extern const MOption MP_PRINT_PAPER;
extern const MOption MP_PRINT_TOPMARGIN_X;
extern const MOption MP_PRINT_TOPMARGIN_Y;
extern const MOption MP_SHOWLOG;
extern const MOption MP_SHOWTIPS;
extern const MOption MP_SMTPHOST;
extern const MOption MP_USE_OUTBOX;

// ----------------------------------------------------------------------------
// persistent msgboxes we use here
// ----------------------------------------------------------------------------

extern const MPersMsgBox *M_MSGBOX_CONFIRM_EXIT;
extern const MPersMsgBox *M_MSGBOX_SHOWLOGWINHINT;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// the name of config section where the log window position is saved
#define  LOG_FRAME_SECTION    _T("LogWindow")

// trace mask for timer events
#define TRACE_TIMER _T("timer")

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

/**
  Tries to save all unsaved stuff which the user wouldn't like to lose
 */
static int SaveAll()
{
   int rc = Composer::SaveAll();

   Profile::FlushAll();

   MfStatusCache::Flush();

   return rc;
}

// ----------------------------------------------------------------------------
// classes
// ----------------------------------------------------------------------------

// a small class implementing saving/restoring position for the log frame
class wxMLogWindow : public wxLogWindow
{
public:
   // ctor (creates the frame hidden)
   wxMLogWindow(wxFrame *pParent, const wxChar *szTitle);

   // override base class virtual to implement saving the frame position and
   // to update the MP_SHOWLOG option
   virtual bool OnFrameClose(wxFrame *frame);
   virtual void OnFrameDelete(wxFrame *frame);

   // are we currently shown?
   bool IsShown() const;

private:
   bool m_hasWindow;

   DECLARE_NO_COPY_CLASS(wxMLogWindow)
};

// a wxLogStderr version which closes the file it logs to
class MLogFile : public wxLogStderr
{
public:
   MLogFile(FILE *fp) : wxLogStderr(fp) { }

   virtual ~MLogFile() { fclose(m_fp); }

private:
   DECLARE_NO_COPY_CLASS(MLogFile)
};

// a timer used to periodically autosave profile settings
class AutoSaveTimer : public wxTimer
{
public:
   AutoSaveTimer() { m_started = FALSE; }

   virtual bool Start( int millisecs = -1, bool oneShot = FALSE )
      { m_started = TRUE; return wxTimer::Start(millisecs, oneShot); }

   virtual void Notify()
   {
      wxLogTrace(TRACE_TIMER, _T("Autosaving options and folder status."));

      (void)SaveAll();
   }

   virtual void Stop()
      { if ( m_started ) wxTimer::Stop(); }

public:
    bool m_started;

private:
    DECLARE_NO_COPY_CLASS(AutoSaveTimer)
};

// a timer used to periodically check for new mail
class MailCollectionTimer : public wxTimer
{
public:
   MailCollectionTimer() { }

   virtual void Notify();

private:
   DECLARE_NO_COPY_CLASS(MailCollectionTimer)
};

// a timer used to wake up idle loop to force generation of the idle events
// even if the program is otherwise idle (i.e. no GUI events happen)
class IdleTimer : public wxTimer
{
public:
   IdleTimer() : wxTimer() { Start(100); }

   virtual void Notify() { wxWakeUpIdle(); }

private:
   DECLARE_NO_COPY_CLASS(IdleTimer)
};

// and yet another timer for the away mode
class AwayTimer : public wxTimer
{
public:
   AwayTimer() { }

   virtual void Notify()
   {
      wxLogTrace(TRACE_TIMER, _T("Going away on timer"));

      mApplication->SetAwayMode(TRUE);
   }

private:
   DECLARE_NO_COPY_CLASS(AwayTimer)
};

#if 0 // def OS_WIN

static struct WatchDog
{
   WatchDog()
   {
      HANDLE hThread = ::CreateThread(NULL, 0, &WatchDog::Run, 0, 0, NULL);
      if ( hThread )
      {
         ::CloseHandle(hThread);
      }
   }

   static DWORD WINAPI Run(void *)
   {
      HANDLE hEvent = ::CreateEvent(NULL, FALSE, FALSE, _T("Mahogany_Die"));
      if ( !hEvent )
         return (DWORD)-1;

      if ( ::WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0 )
      {
         // crash
         int *p = 0;
         *p = 17;
      }

      return 0;
   }
} g_watchDog;
#endif // OS_WIN

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

// the mutex we lock to block any background processing temporarily
static MMutex gs_mutexBlockBg;

// ============================================================================
// implementation
// ============================================================================

// this creates the one and only application object
IMPLEMENT_APP(wxMApp);

// ----------------------------------------------------------------------------
// MailCollectionTimer
// ----------------------------------------------------------------------------

void MailCollectionTimer::Notify()
{
   if ( !mApplication->AllowBgProcessing() )
      return;

   wxLogTrace(TRACE_TIMER, _T("Collection timer expired."));

   mApplication->UpdateOutboxStatus();

   FolderMonitor *collector = mApplication->GetFolderMonitor();
   if ( collector )
      collector->CheckNewMail();
}

// ----------------------------------------------------------------------------
// wxMLogWindow
// ----------------------------------------------------------------------------

wxMLogWindow::wxMLogWindow(wxFrame *pParent, const wxChar *szTitle)
            : wxLogWindow(pParent, szTitle, FALSE)
{
   int x, y, w, h;
   bool i;
   wxMFrame::RestorePosition(LOG_FRAME_SECTION, &x, &y, &w, &h, &i);
   wxFrame *frame = GetFrame();

   frame->SetSize(x, y, w, h);
   frame->SetIcon(ICON(_T("MLogFrame")));
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
   switch ( MDialog_YesNoCancel
            (
               _("Would you like to close the log window only for the rest "
                 "of this session or permanently?\n"
                 "\n"
                 "Note that in either case you may make it appear again by "
                 "changing the corresponding\n"
                 "setting in the 'Miscellaneous' page of the Preferences dialog.\n"
                 "\n"
                 "Choose \"Yes\" to close the log window just for this session,\n"
                 "\"No\" - to permanently close it or \"Cancel\" to not close\n"
                 "the log window at all."),
               NULL,
               MDIALOG_YESNOTITLE,
               M_DLG_YES_DEFAULT,
               M_MSGBOX_SHOWLOGWINHINT
            ) )
   {
      case MDlg_No:
         // disable the log window permanently
         mApplication->GetProfile()->writeEntry(MP_SHOWLOG, 0l);

         // fall through
      case MDlg_Yes:
         break;

      default:
         FAIL_MSG( _T("unexpected MDialog_YesNoCancel return value") );
         // fall through

      case MDlg_Cancel:
         // don't close
         return false;
   }

   return wxLogWindow::OnFrameClose(frame); // TRUE, normally
}

void wxMLogWindow::OnFrameDelete(wxFrame *frame)
{
   wxMFrame::SavePosition(LOG_FRAME_SECTION, frame);

   m_hasWindow = false;

   wxLogWindow::OnFrameDelete(frame);
}

// ----------------------------------------------------------------------------
// wxMApp
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxMApp, wxApp)
   EVT_IDLE                (wxMApp::OnIdle)

#ifdef USE_DIALUP
   EVT_DIALUP_CONNECTED    (wxMApp::OnConnected)
   EVT_DIALUP_DISCONNECTED (wxMApp::OnDisconnected)
#endif // USE_DIALUP

   EVT_QUERY_END_SESSION   (wxMApp::OnQueryEndSession)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// MDebugReport
// ----------------------------------------------------------------------------

#if wxUSE_DEBUGREPORT

#include "wx/debugrpt.h"
#include "wx/file.h"

#include "SendMessage.h"

class MDebugReport : public wxDebugReportCompress
{
public:
   MDebugReport(const char *msg)
   {
      AddAll(msg == NULL ? Context_Exception : Context_Current);

      if ( msg )
      {
         // include message which resulted in program termination
         AddText(_T("error.txt"), msg, _("Last error message"));
      }

      // TODO: include config? this is tricky as it contains sensitive
      //       information such as passwords so we'd have to filter it out
   }

protected:
   virtual bool DoProcess()
   {
      if ( !wxDebugReportCompress::DoProcess() )
         return false;

      Profile *profile = mApplication ? mApplication->GetProfile() : NULL;

      if ( profile )
      {
         wxFile file(GetCompressedFileName());
         if ( !file.IsOpened() )
            return false;

         const wxFileOffset len = file.Length();
         if ( !MDialog_YesNoDialog
               (
                  wxString::Format
                  (
                     _("Would you like to send the crash report to "
                       "Mahogany dev team? The report size is %lukB.\n"
                       "\n"
                       "If you answer \"No\" here you will still be able "
                       "to send the report manually later."),
                     len / 1024
                  )
               ) )
            return false;

         // create a mail message and send it
         SendMessage_obj sm(SendMessage::Create(profile, Prot_SMTP));
         sm->SetSubject(wxString::Format(_T("Mahogany %s crash report"),
                        M_VERSION));
         sm->SetAddresses(_T("mahogany@users.sourceforge.net"));

         wxCharBuffer buf(len);
         if ( !file.Read(buf.data(), len) )
            return false;

         MimeParameterList paramsDisp;
         paramsDisp.push_back(new MimeParameter
                                  (
                                    _T("FILENAME"),
                                    wxString::Format
                                    (
                                       _T("M-%s-debugrpt.zip"),
                                       M_VERSION
                                    )
                                  ));
         sm->AddPart(MimeType::APPLICATION, buf, len,
                     _T("OCTET-STREAM"), _T("ATTACHMENT"),
                     &paramsDisp);
         if ( sm->SendOrQueue(SendMessage::NeverQueue | SendMessage::Silent) )
         {
            // sent successfully
            return true;
         }
      }

      return false;
   }
};

#endif // wxUSE_DEBUGREPORT

// ----------------------------------------------------------------------------
// wxMApp ctor/dtor
// ----------------------------------------------------------------------------

wxMApp::wxMApp(void)
{
   m_IconManager = NULL;
   m_HelpController = NULL;
   m_CanClose = FALSE;
   m_IdleTimer = NULL;

#ifdef USE_DIALUP
   m_OnlineManager = NULL;
   m_DialupSupport = FALSE;
#endif // USE_DIALUP

   m_PrintData = NULL;
   m_PageSetupData = NULL;

   m_logWindow = NULL;
   m_logChain = NULL;

   m_snglInstChecker = NULL;
   m_serverIPC = NULL;

   m_topLevelFrame = NULL;

#ifdef USE_I18N
   m_Locale = NULL;
#endif // USE_I18N

   // this is used for the titles of the message boxess generated by wxWindows
   // internally, set it for consistency with wxMDialogs.cpp
   SetAppName(_T("Mahogany"));
}

wxMApp::~wxMApp()
{
}

void
wxMApp::OnIdle(wxIdleEvent &event)
{
   // now it should be safe to unload any unused DLLs
   UnloadDLLs();

   if ( AllowBgProcessing() )
   {
      MEventManager::DispatchPending();
   }

   event.Skip();
}

bool
wxMApp::AllowBgProcessing() const
{
   return !gs_mutexBlockBg.IsLocked();
}

void
wxMApp::EnterCritical()
{
   gs_mutexBlockBg.Lock();
}

void
wxMApp::LeaveCritical()
{
   gs_mutexBlockBg.Unlock();
}

#ifdef __WXDEBUG__

void
wxMApp::OnAssert(const wxChar *file, int line,
                 const wxChar *cond, const wxChar *msg)
{
   // don't provoke an assert from inside the assert (which would happen if we
   // tried to lock an already locked mutex below)
   if ( gs_mutexBlockBg.IsLocked() )
   {
      wxTrap();

      return;
   }

   // don't allow any calls to c-client while we're in the assert dialog
   // because we might be called from a c-client callback and another call to
   // it will result in a fatal error
   MLocker lock(gs_mutexBlockBg);

   wxApp::OnAssert(file, line, cond, msg);
}

#endif // __WXDEBUG__

bool
wxMApp::Yield(bool onlyIfNeeded)
{
   if ( gs_mutexBlockBg.IsLocked() )
   {
      // background processing already disabled, hopefully it's ok to yield
      // now...
      return wxApp::Yield(onlyIfNeeded);
   }

   // don't allow any calls to c-client from inside wxYield() neither as it is
   // implicitly (!) called by wxProgressDialog which is shown from inside some
   // c-client handlers and so calling c-client now would result in a crash
   MLocker lock(gs_mutexBlockBg);

   return wxApp::Yield(onlyIfNeeded);
}

static void
FatalMsgBox(const String& msg)
{
   static const wxChar *title = gettext_noop("Fatal application error");

   // using a plain message box is safer in this situation, but under Unix we
   // have no such choice
#ifdef __WXMSW__
   ::MessageBox(NULL, msg, wxGetTranslation(title), MB_ICONSTOP);
#else // !MSW
   wxMessageBox(msg, wxGetTranslation(title), wxICON_STOP | wxOK);
#endif // MSW/!MSW
}

void
wxMApp::OnAbnormalTermination(const char *msgOrig)
{
   static bool s_crashed = false;

   if ( s_crashed )
      return;
   s_crashed = true;

   // no background processing in this function
   MLocker lock(gs_mutexBlockBg);


   MAppBase::OnAbnormalTermination();

   // try to save the unsaved messages and options
   int nSaved = SaveAll();

   String msg(_("The application is terminating abnormally,\n"
                "sorry for the inconvenience.\n\n"));

   String msgComposer;
   switch ( nSaved )
   {
      case -1:
         msgComposer = _("Not all composer windows could be saved.");
         break;

      case 0:
         // leave empty
         break;

      default:
         msgComposer.Printf
                     (
                        _("%d unsaved messages were saved,\n"
                          "they will be restored on the next program startup."),
                        nSaved
                     );
   }

   if ( !msgComposer.empty() )
   {
      msg << msgComposer << _T('\n');
   }

#if wxUSE_DEBUGREPORT
   MDebugReport report(msgOrig);
   if ( wxDebugReportPreviewStd().Show(report) )
   {
      if ( report.Process() )
      {
         msg += _("Debug report was successfully sent to Mahogany dev-team.\n"
                  "\n"
                  "Thank you for your help!");
      }
      else // failed to send debug report
      {
         const wxString& fname = report.GetCompressedFileName();
         if ( wxFile::Exists(fname) )
            msg << _("Debug report wasn't sent but was generated in "
                     "the file\n")
                << fname << _T("\n\n")
                << _("Please attach this file to the the bug if you open\n"
                     "one in Mahogany bug tracker.\n"
                     "\n"
                     "Thank you in advance!");
         else
            msg += _("Debug report couldn't be generated.");
      }
   }
   else // user cancelled the report
   {
      msg += _("Debug report generation has been cancelled.");
   }
#else // !wxUSE_DEBUGREPORT
   msg += _("Please report the bug via our bugtracker at\n"
            "\n"
            "     http://mahogany.sourceforge.net/bugz\n"
            "\n"
            "Be sure to mention your OS and Mahogany version\n"
            "and what exactly you were doing before this message\n"
            "box appeared as well as any other useful details!.\n"
            "\n"
            "Thank you!");
#endif // wxUSE_DEBUGREPORT/!wxUSE_DEBUGREPORT

   FatalMsgBox(msg);
}

// can we close now?
bool
wxMApp::CanClose() const
{
   // If we get called the second time, we just reuse our old value, but only
   // if it was TRUE (if we couldn't close before, may be we can now)
   if ( m_CanClose )
      return true;

   // verify that the user didn't accidentally remembered "No" as the answer to
   // the next message box - the problem with this is that in this case there
   // is no way to exit M at all!
   String path = GetPersMsgBoxName(M_MSGBOX_CONFIRM_EXIT);
   if ( wxPMessageBoxIsDisabled(path) )
   {
      if ( !MDialog_YesNoDialog(_T(""), NULL, _T(""),
                                M_DLG_NO_DEFAULT, M_MSGBOX_CONFIRM_EXIT) )
      {
         wxLogDebug(_T("Exit confirmation msg box has been disabled on [No], reenabling it."));

         wxPMessageBoxEnable(path);
      }
      //else: it was on [Yes], ok
   }

   // ask the user for confirmation
   bool okToClose = MDialog_YesNoDialog
                    (
                        _("Do you really want to exit Mahogany?"),
                        m_topLevelFrame,
                        MDIALOG_YESNOTITLE,
                        M_DLG_YES_DEFAULT | M_DLG_NOT_ON_NO,
                        M_MSGBOX_CONFIRM_EXIT
                    );

   if ( okToClose )
   {
      // now check for anything else which may require the user intervention
      okToClose = MAppBase::CanClose();
   }

   if ( okToClose )
   {
      wxWindowList::Node *node = wxTopLevelWindows.GetFirst();
      while ( node && okToClose )
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
               okToClose = frame->CanClose();
            }
            //else: had been asked before
         }
         //else: what to do here? if there is something other than a frame
         //      opened, it might be a (non modal) dialog or may be the log
         //      frame?
      }
   }

   if ( !okToClose )
   {
      // when we will try to close the next time, we shouldn't assume that
      // these frames still don't mind being closed - may be the user will
      // modify the compose view contents or something else changes
      ((MAppBase *)this)->ResetFramesOkToClose();  // const_cast
   }

   return okToClose;
}

void wxMApp::OnQueryEndSession(wxCloseEvent& event)
{
   m_CanClose = false;

   if ( !CanClose() )
      event.Veto();
}

// do close the app by closing all frames
void
wxMApp::OnClose()
{
   // shut down MEvent handling
   if ( m_IdleTimer )
   {
      delete m_IdleTimer;
      m_IdleTimer = NULL;
   }

   // flush the logs while we can
   wxLog::FlushActive();

#if 0
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
#endif // 0
}

// app initilization
bool
wxMApp::OnInit()
{
   // we want our OnAbnormalTermination() be called if we crash
   wxHandleFatalExceptions();

   // create a semaphore indicating that we're running - or check if another
   // copy already is
   m_snglInstChecker = new wxSingleInstanceChecker;

   // note that we can't create the lock file in ~/.M because the directory
   // might not have been created yet...
   //
   // it could also be a bad idea as HOME is often NFS-mounted and creating
   // locks over NFS doesn't work well/at all
   //
   // NB: see comments in GetIPCSocket() about using wxGetUserId()
   if ( !m_snglInstChecker->Create
                            (
                              wxString::Format(_T(".mahogany-%s.lock"),
                                               wxGetUserId().c_str()),
                              _T("/tmp")
                            ) )
   {
      // this message is in English because translations were not loaded yet as
      // the locale hadn't been set
      wxLogWarning(_T("Mahogany will not be able to detect if any other "
                   "program instances are running."));
   }

#if wxCHECK_VERSION(2, 3, 2)
   // parse our command line options inside OnInit()
   if ( !wxApp::OnInit() )
   {
      DoCleanup();

      return false;
   }
#endif // wxWin 2.3.2+

#ifdef OS_WIN
   // stupidly enough wxWin resets the default timestamp under Windows :-(
   wxLog::SetTimestamp(_T("%X"));
#endif // OS_WIN

#ifdef USE_I18N
   // Set up locale first, so everything is in the right language.
   wxString locale = m_cmdLineOptions->lang;

   // if we fail to load the msg catalog, remember it in this variable because
   // we can't remember this in the profile yet - it is not yet created
   bool failedToLoadMsgs = false,
        failedToSetLocale = false;

   // user may explicitly disable i18n by giving --lang=none switch
   if ( locale != _T("none") )
   {
      // TODO should catch the error messages and save them for later
      wxLogNull noLog;

      if ( locale.empty() )
      {
         // use the default system language
         int lang = wxLocale::GetSystemLanguage();
         switch ( lang )
         {
            case wxLANGUAGE_ENGLISH:
            case wxLANGUAGE_ENGLISH_UK:
            case wxLANGUAGE_ENGLISH_US:
            case wxLANGUAGE_ENGLISH_AUSTRALIA:
            case wxLANGUAGE_ENGLISH_BELIZE:
            case wxLANGUAGE_ENGLISH_BOTSWANA:
            case wxLANGUAGE_ENGLISH_CANADA:
            case wxLANGUAGE_ENGLISH_CARIBBEAN:
            case wxLANGUAGE_ENGLISH_DENMARK:
            case wxLANGUAGE_ENGLISH_EIRE:
            case wxLANGUAGE_ENGLISH_JAMAICA:
            case wxLANGUAGE_ENGLISH_NEW_ZEALAND:
            case wxLANGUAGE_ENGLISH_PHILIPPINES:
            case wxLANGUAGE_ENGLISH_SOUTH_AFRICA:
            case wxLANGUAGE_ENGLISH_TRINIDAD:
            case wxLANGUAGE_ENGLISH_ZIMBABWE:
               // nothing to do -- we don't have to translate the messages from
               // English to English
               break;

            case wxLANGUAGE_UNKNOWN:
               // ignore silently for now, what can we do, anyhow?
               break;

            default:
               const wxLanguageInfo *info = wxLocale::GetLanguageInfo(lang);
               if ( info )
                  locale = info->CanonicalName;
               else
                  locale.Printf(_("unknown language (%d)"), lang);

               m_Locale = new wxLocale(lang);
         }
      }
      else // the user specified a locale
      {
#if wxCHECK_VERSION(2, 5, 0)
         const wxLanguageInfo *info = wxLocale::FindLanguageInfo(locale);
         if ( info )
         {
            m_Locale = new wxLocale(info->Language);
         }
         else // use the same string for locale and the message catalog
              // locations -- this only really works under Unix but we
              // can't do much better without FindLanguageInfo()
#endif // 2.5.0+
         {
            m_Locale = new wxLocale(locale, locale);
         }
      }

      if ( m_Locale )
      {
         if ( !m_Locale->IsOk() )
         {
            delete m_Locale;
            m_Locale = NULL;

            failedToSetLocale = true;
         }
         else // set locale successfully
         {
            // now load the message catalogs
#ifdef OS_UNIX
            String localePath;

            // if we're installed in a non-standard prefix, look for our
            // locale files in correct place
            if ( wxStrcmp(M_PREFIX, _T("/usr")) &&
                     wxStrcmp(M_PREFIX, _T("/usr/local")) )
            {
               localePath << M_PREFIX << _T("/share/locale");
            }
#elif defined(OS_WIN)
            // the program directory is not initialized yet so we can't do much
            // more than looking in the current directory...
            wxString strPath;
            ::GetModuleFileName(NULL, wxStringBuffer(strPath, MAX_PATH), MAX_PATH);

            // get just the path
            wxString strDir;
            wxSplitPath(strPath, &strDir, NULL, NULL);

            // a special hack: when running M from the build dir, we really
            // want to look in the top level directory
            if ( strDir.Right(5) == _T("Debug") )
               strDir.Truncate(strDir.length() - 5);
            else if ( strDir.Right(7) == _T("Release") )
               strDir.Truncate(strDir.length() - 7);

            String localePath = strDir;
            if ( !wxEndsWithPathSeparator(localePath) )
               localePath += _T('\\');

            localePath += "locale";
#else
            #error "don't know where to find message catalogs on this platform"
#endif // OS

            if ( !localePath.empty() )
            {
               m_Locale->AddCatalogLookupPathPrefix(localePath);
            }

            if ( !m_Locale->AddCatalog(M_APPLICATIONNAME) )
            {
               // better use English messages if msg catalog was not found
               delete m_Locale;
               m_Locale = NULL;

               failedToLoadMsgs = true;
            }
         }
      }
   }
#endif // USE_I18N

   wxInitAllImageHandlers();
   wxFileSystem::AddHandler(new wxMemoryFSHandler);
//   wxFileSystem::AddHandler(new wxZipFSHandler); // for zipped docs

   // this is necessary to avoid that the app closes automatically when we're
   // run for the first time and show a modal dialog before opening the main
   // frame - if we don't do it, when the dialog (which is the last app window
   // at this moment) disappears, the app will close.
   SetExitOnFrameDelete(FALSE);

   // create timers -- are accessed by OnStartup()
   gs_timerAutoSave = new AutoSaveTimer;
   gs_timerMailCollection = new MailCollectionTimer;

   if ( !OnStartup() )
   {
      if ( GetLastError() != M_ERROR_CANCEL )
      {
         ERRORMESSAGE((_T("Can't initialize application, terminating.")));
      }
      //else: this would be superfluous

      DoCleanup();

      return false;
   }

#ifdef USE_I18N
   // only now we can use profiles
   if ( failedToSetLocale || failedToLoadMsgs )
   {
      // if we can't load the msg catalog for the given locale once, don't
      // try it again next time - the flag is stored in the profile in this
      // key
      wxString configPath;
      configPath << _T("UseLocale_") << locale;

      Profile *profile = GetProfile();
      if ( profile->readEntry(configPath, 1l) != 0l )
      {
         CloseSplash();

         // the strings here are intentionally not translated
         wxString msg;
         if ( failedToSetLocale )
         {
            msg.Printf(_T("Locale '%s' couldn't be set, do you want to "
                       "retry setting it the next time?"),
                       locale.c_str());
         }
         else // failedToLoadMsgs
         {
            msg.Printf(_T("Impossible to load message catalog(s) for the "
                       "locale '%s', do you want to retry next time?"),
                       locale.c_str());
         }

         if ( wxMessageBox(msg, _T("Error"),
                           wxICON_STOP | wxYES_NO) != wxYES )
         {
            profile->writeEntry(configPath, 0l);
         }
      }
      //else: the user had previously answered "no" to the question above,
      //      don't complain any more about missing catalogs
   }
#endif // USE_I18N

   // at this moment we're fully initialized, i.e. profile and log
   // subsystems are working and the main window is created

   // start a timer to autosave the profile entries
   StartTimer(Timer_Autosave);

   // the timer to poll for new mail will be started when/if FolderMonitor
   // is created
   //StartTimer(Timer_PollIncoming);

   // start away timer if necessary
   StartTimer(Timer_Away);

   // another timer to do MEvent handling:
   m_IdleTimer = new IdleTimer;

   // restore the normal behaviour (see the comments above)
   SetExitOnFrameDelete(TRUE);

#ifdef USE_DIALUP
   // reflect settings in menu and statusbar:
   UpdateOnlineDisplay();
#endif // USE_DIALUP

   // make sure this is displayed correctly:
   UpdateOutboxStatus();

   // show tip dialog unless disabled
   if ( READ_APPCONFIG(MP_SHOWTIPS) )
   {
      MDialog_ShowTip(GetTopWindow());
   }

   return true;
}

wxMFrame *wxMApp::CreateTopLevelFrame()
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

void wxMApp::DoCleanup()
{
   // we don't really run any more
   if ( m_snglInstChecker )
   {
      delete m_snglInstChecker;
      m_snglInstChecker = NULL;
   }

   // no more IPC for us neither
   if ( m_serverIPC )
   {
      delete m_serverIPC;
      m_serverIPC = NULL;
   }

   // if one timer was created, then all of them were
   if ( gs_timerAutoSave )
   {
      // delete timers
      delete gs_timerAutoSave;
      delete gs_timerMailCollection;
      delete gs_timerAway;

      gs_timerAutoSave = NULL;
      gs_timerMailCollection = NULL;
      gs_timerAway = NULL;
   }

   CleanUpPrintData();

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

   UnloadModules();
   MAppBase::OnShutDown();

   MModule_Cleanup();

   if ( m_IconManager )
   {
      delete m_IconManager;
      m_IconManager = NULL;
   }

#ifdef USE_I18N
   if ( m_Locale )
   {
      delete m_Locale;
      m_Locale = NULL;
   }
#endif // USE_I18N

#ifdef USE_DIALUP
   if ( m_OnlineManager )
   {
      delete m_OnlineManager;
      m_OnlineManager = NULL;
   }
#endif // USE_DIALUP

   // save all data now as we may still report errors -- later will be too late
   Profile::FlushAll();

   // and delete config as we won't be using it any longer
   Profile::DeleteGlobalConfig();

}

int wxMApp::OnExit()
{
   DoCleanup();

   // this does cleanup which must be done whether OnInit() succeeded or failed
   return wxApp::OnExit();
}

// ----------------------------------------------------------------------------
// wxMApp printing
// ----------------------------------------------------------------------------

void wxMApp::CleanUpPrintData()
{
#if wxUSE_PRINTING_ARCHITECTURE
   // save our preferred printer settings
   if ( m_PrintData )
   {
#if wxUSE_POSTSCRIPT
      m_profile->writeEntry(MP_PRINT_COMMAND, m_PrintData->GetPrinterCommand());
      m_profile->writeEntry(MP_PRINT_OPTIONS, m_PrintData->GetPrinterOptions());
      m_profile->writeEntry(MP_PRINT_ORIENTATION, m_PrintData->GetOrientation());
      m_profile->writeEntry(MP_PRINT_MODE, m_PrintData->GetPrintMode());
      m_profile->writeEntry(MP_PRINT_PAPER, m_PrintData->GetPaperId());
      m_profile->writeEntry(MP_PRINT_FILE, m_PrintData->GetFilename());
      m_profile->writeEntry(MP_PRINT_COLOUR, m_PrintData->GetColour());
#endif // wxUSE_POSTSCRIPT

      delete m_PrintData;
      m_PrintData = NULL;
   }

   if ( m_PageSetupData )
   {
#if wxUSE_POSTSCRIPT
      m_profile->writeEntry(MP_PRINT_TOPMARGIN_X,
                            m_PageSetupData->GetMarginTopLeft().x);
      m_profile->writeEntry(MP_PRINT_TOPMARGIN_Y,
                            m_PageSetupData->GetMarginTopLeft().y);
      m_profile->writeEntry(MP_PRINT_BOTTOMMARGIN_X,
                            m_PageSetupData->GetMarginBottomRight().x);
      m_profile->writeEntry(MP_PRINT_BOTTOMMARGIN_Y,
                            m_PageSetupData->GetMarginBottomRight().y);
#endif // wxUSE_POSTSCRIPT

      delete m_PageSetupData;
      m_PageSetupData = NULL;
   }
#endif // wxUSE_PRINTING_ARCHITECTURE
}

const wxPrintData *wxMApp::GetPrintData()
{
#if wxUSE_PRINTING_ARCHITECTURE
   if ( !m_PrintData )
   {
#ifdef OS_WIN
      wxGetApp().SetPrintMode(wxPRINT_WINDOWS);
#else // Unix
      wxGetApp().SetPrintMode(wxPRINT_POSTSCRIPT);

      // set AFM path
      PathFinder pf(mApplication->GetDataDir() + _T("/afm"), false);
      pf.AddPaths(READ_APPCONFIG_TEXT(MP_AFMPATH), false);
      pf.AddPaths(mApplication->GetLocalDir(), true);

      bool found;
      String afmpath = pf.FindDirFile(_T("Cour.afm"), &found);
      if(found)
      {
        //wxThePrintSetupData->SetAFMPath(afmpath);
      }
#endif // Win/Unix

      m_PrintData = new wxPrintData;

#ifndef OS_WIN
      if ( found )
      {
         m_PrintData->SetFontMetricPath(afmpath);
      }
#endif // !OS_WIN

#if wxUSE_POSTSCRIPT
      //*m_PrintData = *wxThePrintSetupData;

      m_PrintData->SetPrinterCommand(READ_APPCONFIG(MP_PRINT_COMMAND));
      m_PrintData->SetPrinterOptions(READ_APPCONFIG(MP_PRINT_OPTIONS));
      m_PrintData->SetOrientation(READ_APPCONFIG(MP_PRINT_ORIENTATION));
      m_PrintData->SetPrintMode((wxPrintMode)(long)READ_APPCONFIG(MP_PRINT_MODE));
      m_PrintData->SetPaperId((wxPaperSize)(long)READ_APPCONFIG(MP_PRINT_PAPER));
      m_PrintData->SetFilename(READ_APPCONFIG(MP_PRINT_FILE));
      m_PrintData->SetColour(READ_APPCONFIG(MP_PRINT_COLOUR));
#endif // wxUSE_POSTSCRIPT
   }
#endif // wxUSE_PRINTING_ARCHITECTURE

   return m_PrintData;
}

void wxMApp::SetPrintData(const wxPrintData& printData)
{
#if wxUSE_PRINTING_ARCHITECTURE
   CHECK_RET( m_PrintData, _T("must have called GetPrintData() before!") );

   *m_PrintData = printData;
#endif // wxUSE_PRINTING_ARCHITECTURE
}

wxPageSetupDialogData *wxMApp::GetPageSetupData()
{
#if wxUSE_PRINTING_ARCHITECTURE
   if ( !m_PageSetupData )
   {
      m_PageSetupData = new wxPageSetupDialogData(*GetPrintData());

#if wxUSE_POSTSCRIPT
      m_PageSetupData->SetMarginTopLeft(wxPoint(
         READ_APPCONFIG(MP_PRINT_TOPMARGIN_X),
         READ_APPCONFIG(MP_PRINT_TOPMARGIN_X)));
      m_PageSetupData->SetMarginBottomRight(wxPoint(
         READ_APPCONFIG(MP_PRINT_BOTTOMMARGIN_X),
         READ_APPCONFIG(MP_PRINT_BOTTOMMARGIN_X)));
#endif // wxUSE_POSTSCRIPT
   }
#endif // wxUSE_PRINTING_ARCHITECTURE

   return m_PageSetupData;
}

void wxMApp::SetPageSetupData(const wxPageSetupDialogData& data)
{
#if wxUSE_PRINTING_ARCHITECTURE
   CHECK_RET( m_PrintData, _T("must have called GetPageSetupData() before!") );

   *m_PageSetupData = data;
#endif // wxUSE_PRINTING_ARCHITECTURE
}

// ----------------------------------------------------------------------------
// Help subsystem
// ----------------------------------------------------------------------------

/* static */
wxString wxMApp::BuildHelpInitString(const wxString& dir)
{
   wxString helpfile = dir;

#ifdef OS_WIN
   helpfile += _T("\\Manual.chm");
#else // !Windows
   if ( !READ_APPCONFIG(MP_HELPBROWSER_KIND) )
   {
#if wxUSE_LIBMSPACK
      helpfile += _T("/Manual.chm");
#else
      helpfile += _T("/Manual.hhp");
#endif // wxUSE_LIBMSPACK
   }
#endif // Windows/!Windows
   return helpfile;
}

/* static */
wxString wxMApp::GetHelpDir()
{
   wxString helpdir = mApplication->GetDataDir();
   if ( !helpdir.empty() )
      helpdir += wxFILE_SEP_PATH;

#ifdef OS_WIN
   helpdir += _T("help");
#else // !Windows
   helpdir += _T("doc");
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

#ifdef OS_UNIX
   bool helpBrowserIsExternal = READ_APPCONFIG(MP_HELPBROWSER_KIND);
#endif // OS_UNIX
   while ( !m_HelpController )
   {
      // we hardcode the help controller class we use instead of using the
      // default wxHelpController everywhere as we don't have docs in all
      // possible formats, just HTML and CHM
#ifdef OS_UNIX
      if ( !helpBrowserIsExternal )
      {
         m_HelpController = new wxHtmlHelpController;
      }
      else
      {
         m_HelpController = new wxExtHelpController;
      }
#else // Windows
      m_HelpController = new wxBestHelpController;
#endif // Unix/Windows

      // try to initialise the help system
      if ( !m_HelpController->Initialize(BuildHelpInitString(helpdir)) )
      {
         // failed
         delete m_HelpController;
         m_HelpController = NULL;

         // ask the user if we want to look elsewhere?
         wxString msg;
         msg.Printf(_("Cannot initialise help system:\n"
                      "help files not found in the directory '%s'.\n"
                      "\n"
                      "Would you like to specify another help files "
                      "location (otherwise help will be unavailable)?"),
                    helpdir.c_str());

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

#ifdef OS_UNIX
   if ( helpBrowserIsExternal )
   {
      ((wxExtHelpController *)m_HelpController)->SetBrowser(
         READ_APPCONFIG(MP_HELPBROWSER),
         READ_APPCONFIG(MP_HELPBROWSER_ISNS));
   }
#endif // OS_UNIX

   wxSize size = wxSize(READ_APPCONFIG(MP_HELPFRAME_WIDTH),
                        READ_APPCONFIG(MP_HELPFRAME_HEIGHT));
   wxPoint pos = wxPoint(READ_APPCONFIG(MP_HELPFRAME_XPOS),
                         READ_APPCONFIG(MP_HELPFRAME_YPOS));

   m_HelpController->SetFrameParameters(_T("Mahogany : %s"), size, pos);

   // remember the dir where we found the files
   if ( helpdir != READ_APPCONFIG(MP_HELPDIR) )
   {
      mApplication->GetProfile()->writeEntry(MP_HELPDIR, helpdir);
   }

   // we do have m_HelpController now
   return true;
}

void
wxMApp::Help(int UNUSED_IF_WIN(id), wxWindow * UNUSED_IF_WIN(parent))
{
   // first thing: close splash if it's still there
   CloseSplash();

   if ( !m_HelpController && !InitHelp() )
   {
      // help controller couldn't be initialized, so no help available
      return;
   }

   // by now we should have either created it or returned
   CHECK_RET( m_HelpController, _T("no help controller, but should have one!") );

#ifdef OS_WIN
   // under Windows only help contents can be currently shown
   m_HelpController->DisplayContents();
#else // !OS_WIN
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
                 && !s_lastSearchKey.empty() )
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
#endif // OS_WIN/!OS_WIN
}

// ----------------------------------------------------------------------------
// Loadable modules
// ----------------------------------------------------------------------------

void
wxMApp::LoadModules(void)
{
   RefCounter<MModuleListing>
      listing(MModule::ListAvailableModules(STARTUP_INTERFACE));

   if ( !listing )
      return;

   const size_t count = listing->Count();
   for ( size_t n = 0; n < count; n++ )
   {
      const String& name = (*listing)[n].GetName();
      MModule *module = MModule::LoadModule(name);

      if ( !module )
      {
         ERRORMESSAGE((_("Cannot load module '%s'."), name.c_str()));
      }
      else
      {
         // remember it so we can decref it before exiting
         gs_GlobalModulesList.push_back(new MModuleEntry(module));
#ifdef DEBUG
         LOGMESSAGE((M_LOG_WINONLY,
                   _T("Successfully loaded module:\nName: %s\nDescr: %s\nVersion: %s\n"),
                   module->GetName(),
                   module->GetDescription(),
                   module->GetVersion()));

#endif // DEBUG
      }
   }
}

void
wxMApp::InitModules(void)
{
   for ( ModulesList::iterator i = gs_GlobalModulesList.begin();
         i != gs_GlobalModulesList.end();
         ++i )
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
   for ( ModulesList::iterator i = gs_GlobalModulesList.begin();
         i != gs_GlobalModulesList.end();
         ++i )
   {
      (**i).m_Module->DecRef();
   }
}

// ----------------------------------------------------------------------------
// timer stuff
// ----------------------------------------------------------------------------

// just a wrapper for FolderMonitor::GetMinCheckTimeout()
static long GetPollIncomingTimerDelay(void)
{
   FolderMonitor *collector = mApplication->GetFolderMonitor();

   return collector ? collector->GetMinCheckTimeout() : 0;
}

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
         delay = GetPollIncomingTimerDelay();
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
         wxFAIL_MSG(_T("attempt to start an unknown timer"));
   }

   // timer not started
   return false;
}

bool wxMApp::StopTimer(Timer timer)
{
   switch ( timer )
   {
      case Timer_Autosave:
         if ( gs_timerAutoSave )
            gs_timerAutoSave->Stop();
         break;

      case Timer_PollIncoming:
         if ( gs_timerMailCollection )
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
         wxFAIL_MSG(_T("attempt to stop an unknown timer"));
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
      FAIL_MSG(_T("unsupported mutex id"));
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
wxMApp::GetStdIcon(int UNUSED_IF_WIN(which)) const
{
#ifndef OS_WIN
   // this function may be (and is) called from the persistent controls code
   // which uses the global config object for its own needs, so avoid changing
   // its path here - or rather restore it on funciton exit
   class ConfigPathRestorer
   {
   public:
      ConfigPathRestorer()
         {
            m_profile = mApplication ? mApplication->GetProfile() : NULL;
            if ( m_profile )
               m_path = m_profile->GetConfig()->GetPath();
         }

      ~ConfigPathRestorer()
         {
            if ( m_profile )
               m_profile->GetConfig()->SetPath(m_path);
         }

   private:
      wxString m_path;
      Profile *m_profile;
   } storeConfigPath;

   // Set our icons for the dialogs.
   switch(which)
   {
   case wxICON_HAND:
      return ICON(_T("msg_error")); break;
   case wxICON_EXCLAMATION:
      return ICON(_T("msg_warning")); break;
   case wxICON_QUESTION:
      return ICON(_T("msg_question")); break;
   case wxICON_INFORMATION:
      return ICON(_T("msg_info")); break;
   }
#endif // !OS_WIN

   return wxNullIcon;
}

void
wxMApp::UpdateOutboxStatus(MailFolder *mf) const
{
   if(! m_topLevelFrame) // called when flushing events at program end?
      return;

   UIdType nNNTP, nSMTP;
   bool enable = CheckOutbox(&nSMTP, &nNNTP, mf);

   // only enable menu item if outbox is used and contains messages:
   wxMenuBar *mbar = m_topLevelFrame->GetMenuBar();
   CHECK_RET( mbar, _T("no menubar in the main frame?") );

   bool useOutbox = READ_APPCONFIG_BOOL(MP_USE_OUTBOX);
   mbar->Enable(WXMENU_FILE_SEND_OUTBOX, enable && useOutbox);

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

   wxStatusBar *sbar = m_topLevelFrame->GetStatusBar();
   CHECK_RET( sbar, _T("no status bar in the main frame?") );

   // const_cast needed
   sbar->SetStatusText(msg, ((wxMApp *)this)->GetStatusField(SF_OUTBOX));
}

wxIconManager *wxMApp::GetIconManager(void) const
{
    if ( !m_IconManager )
    {
        // const_cast
        ((wxMApp *)this)->m_IconManager = new wxIconManager;
    }

    return m_IconManager;
}

// ----------------------------------------------------------------------------
// dial up support
// ----------------------------------------------------------------------------

#ifdef USE_DIALUP

void
wxMApp::OnConnected(wxDialUpEvent&)
{
   if(! m_DialupSupport)
      return;
   m_IsOnline = TRUE;
   UpdateOnlineDisplay();
   MDialog_Message(_("Dial-Up network connection established."),
                   m_topLevelFrame,
                   _("Information"),
                   _T("DialUpConnectedMsg"));
}

void
wxMApp::OnDisconnected(wxDialUpEvent&)
{
   if(! m_DialupSupport)
      return;
   m_IsOnline = FALSE;
   UpdateOnlineDisplay();
   MDialog_Message(_("Dial-Up network shut down."),
                   m_topLevelFrame,
                   _("Information"),
                   _T("DialUpDisconnectedMsg"));
}

void
wxMApp::SetupOnlineManager(void)
{
   // only create dial up manager if needed
   m_DialupSupport = READ_APPCONFIG_BOOL(MP_DIALUP_SUPPORT);

   if ( m_DialupSupport )
   {
      if ( !m_OnlineManager )
      {
         m_OnlineManager = wxDialUpManager::Create();
      }

      // under Windows it autostarts itself
#ifndef OS_WIN
      if(! m_OnlineManager->EnableAutoCheckOnlineStatus(60))
      {
         wxLogError(_("Cannot activate auto-check for dial-up network status."));
      }
#endif // !OS_WIN

      String beaconhost = READ_APPCONFIG(MP_BEACONHOST);
      strutil_delwhitespace(beaconhost);
      // If no host configured, use smtp host:
      if(beaconhost.length() > 0)
      {
         m_OnlineManager->SetWellKnownHost(beaconhost);
      }
      else // beacon host configured
      {
         beaconhost = READ_APPCONFIG_TEXT(MP_SMTPHOST);
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
      m_OnlineManager = NULL; // Cleanup will try to delete it.

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
   CHECK_RET( m_OnlineManager, _T("can't go online") );

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
   CHECK_RET( m_OnlineManager, _T("can't go offline") );

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
wxMApp::UpdateOnlineDisplay(void)
{
   // Can be called during  application startup, in this case do
   // nothing:
   if(! m_topLevelFrame)
      return;

   wxStatusBar *sbar = m_topLevelFrame->GetStatusBar();
   wxMenuBar *mbar = m_topLevelFrame->GetMenuBar();
   CHECK_RET( sbar && mbar, _T("no status/menu bar in the main frame?") );

   if(! m_DialupSupport)
   {
      mbar->Enable(WXMENU_FILE_NET_ON, FALSE);
      mbar->Enable(WXMENU_FILE_NET_OFF, FALSE);
   }
   else // we do have dialup support
   {
      bool online = IsOnline();
      mbar->Enable(WXMENU_FILE_NET_OFF, online);
      mbar->Enable(WXMENU_FILE_NET_ON, !online);

      sbar->SetStatusText(online ? _("Online"):_("Offline"),
                          GetStatusField(SF_ONLINE));
   }
}

#endif // USE_DIALUP

// ----------------------------------------------------------------------------
// miscellaneous
// ----------------------------------------------------------------------------

void
wxMApp::FatalError(const wxChar *message)
{
   OnAbnormalTermination(message);

   abort();
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

void wxMApp::RecreateStatusBar()
{
   // this array is indexed by StatusFields elements
   //
   // SF_STANDARD is the largest and takes thrice as much space as SF_FOLDER
   // while SF_ONLINE and SF_OUTBOX have fixed width
   static const int s_statusWidths[] =
   {
      -3,               // main pane
      -1,               // folder view status
#ifdef USE_DIALUP
      70,               // online/offline
#endif // USE_DIALUP
      150,              // outbox status
   };

   wxCOMPILE_TIME_ASSERT2( WXSIZEOF(s_statusWidths) == SF_MAXIMUM,
                           StatusWidthsNotInSync, MAppStatus );

   if ( !m_topLevelFrame )
   {
      // this may happen when we're shutting down, so just ignore it quietly in
      // this case -- but complain loudly otherwise
      ASSERT_MSG( IsShuttingDown(),
                     _T("no top level frame to recreate status bar for") );

      return;
   }

   wxStatusBar *sbar = m_topLevelFrame->GetStatusBar();
   CHECK_RET( sbar, _T("no status bar to recreate?") );

   int widths[SF_MAXIMUM];
   size_t n;
   for ( n = 0; n < WXSIZEOF(m_statusPanes); n++ )
   {
      StatusFields field = m_statusPanes[n];
      if ( field == SF_ILLEGAL )
      {
         // no more fields, SF_ILLEGAL is the termination marker
         break;
      }

      widths[n] = s_statusWidths[field];
   }

   sbar->SetFieldsCount(n, widths);
}

// ----------------------------------------------------------------------------
// log window support (see also wxMLogWindow)
// ----------------------------------------------------------------------------

void wxMApp::SetLogFile(const String& filename)
{
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
      wxFFile file(filename, _T("w"));
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

// ----------------------------------------------------------------------------
// command line parsing
// ----------------------------------------------------------------------------

// the names of cmd line options
#define OPTION_BCC         _T("bcc")
#define OPTION_BODY        _T("body")
#define OPTION_CC          _T("cc")
#define OPTION_USERDIR     _T("userdir")
#define OPTION_CONFIG      _T("config")
#define OPTION_IMPORT      _T("import")
#define OPTION_DEBUGMAIL   _T("debug")
#define OPTION_FOLDER      _T("folder")
#define OPTION_LANG        _T("lang")
#define OPTION_NEWSGROUP   _T("newsgroup")
#define OPTION_NOPYTHON    _T("nopython")
#define OPTION_SAFE        _T("safe")
#define OPTION_SUBJECT     _T("subject")

void wxMApp::OnInitCmdLine(wxCmdLineParser& parser)
{
   wxApp::OnInitCmdLine(parser);

   // NB: add new options in alphabetical order
   static const wxCmdLineEntryDesc cmdLineDesc[] =
   {
      // -b or --bcc to specify the BCC headers
      {
         wxCMD_LINE_OPTION,
         _T("b"),
         OPTION_BCC,
         gettext_noop("specify the blind carbon-copy (BCC) recipients"),
      },

      // --body to specify the message body
      {
         wxCMD_LINE_OPTION,
         NULL,
         OPTION_BODY,
         gettext_noop("specify the body of the message"),
      },

      // -c or --cc to specify the CC headers
      {
         wxCMD_LINE_OPTION,
         _T("c"),
         OPTION_CC,
         gettext_noop("specify the carbon-copy (CC) recipients"),
      },

      // --userdir=dir to specify an alternative home directory to use
      {
         wxCMD_LINE_OPTION,
         NULL,
         OPTION_USERDIR,
         gettext_noop("specify a non default directory for user files"),
      },

      // --config=file to specify an alternative config file to use
      {
         wxCMD_LINE_OPTION,
         NULL,
         OPTION_CONFIG,
         gettext_noop("specify the alternative configuration file to use"),
      },

      // --debug to force mail debugging on
      {
         wxCMD_LINE_SWITCH,
         NULL,
         OPTION_DEBUGMAIL,
         gettext_noop("enable mail subsystem debug message logging"),
      },

      // -f or --folder to specify the folder to open in the main frame
      {
         wxCMD_LINE_OPTION,
         _T("f"),
         OPTION_FOLDER,
         gettext_noop("specify the folder to open in the main window"),
      },

      // --import=spec to get the settings from file or remove server
      {
         wxCMD_LINE_OPTION,
         NULL,
         OPTION_IMPORT,
         gettext_noop("file or remote server containing Mahogany options"),
      },

      // --lang option to specify the language to use
      {
         wxCMD_LINE_OPTION,
         NULL,
         OPTION_LANG,
         gettext_noop("the language to use for the program messages"),
      },

      // --nopython to disable loading Python interpreter
      // (note that this option exists even if Python is not compiled in, in
      // this case it simply does nothing)
      {
         wxCMD_LINE_SWITCH,
         NULL,
         OPTION_NOPYTHON,
         gettext_noop("don't load Python interpreter even if configured to do so"),
      },

      // --newsgroup to specify the newsgroup to post the message to
      {
         wxCMD_LINE_OPTION,
         NULL,
         OPTION_NEWSGROUP,
         gettext_noop("the newsgroup to post the message to"),
      },

      // --safe option to prevent crashes at startup
      {
         wxCMD_LINE_SWITCH,
         NULL,
         OPTION_SAFE,
         gettext_noop("don't perform any operations on startup"),
      },

      // -s or --subject to specify the subject
      {
         wxCMD_LINE_OPTION,
         _T("s"),
         OPTION_SUBJECT,
         gettext_noop("specify the subject"),
      },

      // <address> to write to
      {
         wxCMD_LINE_PARAM,
         NULL,
         NULL,
         _T("address(es) to start composing the message to"),
         wxCMD_LINE_VAL_STRING,
         wxCMD_LINE_PARAM_OPTIONAL | wxCMD_LINE_PARAM_MULTIPLE
      },

      // terminator
      { wxCMD_LINE_NONE }
   };

   parser.SetDesc(cmdLineDesc);
}

bool wxMApp::OnCmdLineParsed(wxCmdLineParser& parser)
{
   if ( !wxApp::OnCmdLineParsed(parser) )
      return false;

   // first get the options values
   bool startComposer = parser.Found(OPTION_BCC, &m_cmdLineOptions->composer.bcc);
   startComposer |= parser.Found(OPTION_BODY, &m_cmdLineOptions->composer.body);
   startComposer |= parser.Found(OPTION_CC, &m_cmdLineOptions->composer.cc);
   startComposer |= parser.Found(OPTION_SUBJECT, &m_cmdLineOptions->composer.subject);

   bool hasRcpt = parser.Found(OPTION_NEWSGROUP, &m_cmdLineOptions->composer.newsgroups);
   size_t count = parser.GetParamCount();
   if ( count > 0 )
   {
      hasRcpt = true;

      String to;
      for ( size_t n = 0; n < count; n++ )
      {
         if ( !to.empty() )
            to += _T(", ");

         to += parser.GetParam(n);
      }

      m_cmdLineOptions->composer.to = to;
   }

   m_cmdLineOptions->safe = parser.Found(OPTION_SAFE);
   m_cmdLineOptions->debugMail = parser.Found(OPTION_DEBUGMAIL);

#ifdef USE_PYTHON
   m_cmdLineOptions->noPython = parser.Found(OPTION_NOPYTHON);
#endif // USE_PYTHON

   if ( startComposer )
   {
      if ( m_cmdLineOptions->safe )
      {
         wxLogWarning(_("Composer options are ignored in safe mode."));
      }
      else
      {
         // if we have any of the message composing-related options, we must
         // have at least one rcpt as well
         if ( !hasRcpt )
         {
            wxLogError(_("A message recipient or a newsgroup must be specified "
                         "if any of the composer-related options are used."));
            return false;
         }
      }
   }

   // check for non default local dir and config file
   //
   // consider that config file is supposed to be in the user dir under Unix.
   // under Windows we don't use config file at all unless it was specified
#ifndef OS_WIN
   const bool hasDir =
#endif // OS_WIN
   parser.Found(OPTION_USERDIR, &m_cmdLineOptions->userDir);
   if ( !parser.Found(OPTION_CONFIG, &m_cmdLineOptions->configFile) )
   {
#ifndef OS_WIN
      if ( hasDir )
      {
         m_cmdLineOptions->configFile = m_cmdLineOptions->userDir +
                                          wxFILE_SEP_PATH + _T("config");
      }
#endif // !OS_WIN
   }

   m_cmdLineOptions->useFolder = parser.Found(OPTION_FOLDER,
                                              &m_cmdLineOptions->folder);

   (void)parser.Found(OPTION_LANG, &m_cmdLineOptions->lang);

   (void)parser.Found(OPTION_IMPORT, &m_cmdLineOptions->configImport);

   return true;
}

// ----------------------------------------------------------------------------
// global functions implemented here
// ----------------------------------------------------------------------------

extern bool EnsureAvailableTextEncoding(wxFontEncoding *enc,
                                        wxString *text,
                                        bool mayAskUser)
{
   CHECK( enc, false, _T("CheckEncodingAvailability: NULL encoding") );

   if ( !wxFontMapper::Get()->IsEncodingAvailable(*enc) )
   {
      // try to find another encoding
      wxFontEncoding encAlt;
      if ( wxFontMapper::Get()->GetAltForEncoding(*enc, &encAlt, _T(""), mayAskUser) )
      {
         // translate the text (if any) to the equivalent encoding
         if ( text )
         {
            wxEncodingConverter conv;
            if ( !conv.Init(*enc, encAlt) )
            {
               // failed to convert the text
               return false;
            }

            *text = conv.Convert(*text);
         }
         //else: just return the encoding

         *enc = encAlt;
      }
      else //no equivalent encoding
      {
         return false;
      }
   }

   // we have either the requested encoding or an equivalent one
   return true;
}

// ============================================================================
// IPC and multiple program instances handling
// ============================================================================

// ----------------------------------------------------------------------------
// IPC constants
// ----------------------------------------------------------------------------

#define IPC_TOPIC  _T("MRemote")

// ----------------------------------------------------------------------------
// IPC functions
// ----------------------------------------------------------------------------

// return the name of the IPC socket we use -- this is only really used under
// Unix
static String GetIPCSocket()
{
   static String s_socketName;
   if ( s_socketName.empty() )
   {
      // we are using wxGetUserId() to allow multiple users to run multiple
      // copies of Mahogany, each communicating via its own socket and the name
      // seems to be better than UID as, although it seems unlikely, we might
      // have the same user logged in from 2 different machines sharing the
      // same /tmp directory
      s_socketName.Printf(_T("/tmp/.mahogany-%s.ipc"), wxGetUserId().c_str());
   }

   return s_socketName;
}

// ----------------------------------------------------------------------------
// IPC classes
// ----------------------------------------------------------------------------

class MAppIPCConnection : public wxConnection
{
public:
   MAppIPCConnection()
      : wxConnection(m_buffer, WXSIZEOF(m_buffer))
   {
   }

   virtual bool OnExecute(const wxString& WXUNUSED(topic),
                          wxChar *data,
                          int WXUNUSED(size),
                          wxIPCFormat WXUNUSED(format))
   {
      return wxGetApp().OnRemoteRequest(data);
   }

private:
   wxChar m_buffer[4096];

   DECLARE_NO_COPY_CLASS(MAppIPCConnection)
};

class MAppIPCServer : public wxServer
{
public:
   virtual wxConnectionBase *OnAcceptConnection(const wxString& topic)
   {
      if ( topic != IPC_TOPIC )
         return NULL;

      return new MAppIPCConnection;
   }
};

// ----------------------------------------------------------------------------
// wxMApp IPC methods
// ----------------------------------------------------------------------------

bool wxMApp::OnRemoteRequest(const wxChar *request)
{
   CmdLineOptions cmdLineOpts;
   if ( !cmdLineOpts.FromString(request) )
   {
      wxLogError(_("Ignoring unrecognized remote request '%s'."),
                 request);
      return false;
   }

   if ( !ProcessSendCmdLineOptions(cmdLineOpts) )
   {
      // no composer windows were opened, bring the main frame to top to ensure
      // that we have focus
      if ( m_topLevelFrame )
      {
         m_topLevelFrame->Raise();
      }
   }

   return true;
}

bool wxMApp::IsAnotherRunning() const
{
   // it's too early or too late to call us, normally we shouldn't try to do it
   // then, i.e. this indicates an error in the caller logic
   CHECK( m_snglInstChecker, false, _T("IsAnotherRunning() shouldn't be called") );

   return m_snglInstChecker->IsAnotherRunning();
}

bool wxMApp::SetupRemoteCallServer()
{
   ASSERT_MSG( !m_serverIPC, _T("SetupRemoteCallServer() called twice?") );

   m_serverIPC = new MAppIPCServer;
   if ( !m_serverIPC->Create(GetIPCSocket()) )
   {
      delete m_serverIPC;
      m_serverIPC = NULL;

      return false;
   }

   return true;
}

bool wxMApp::CallAnother()
{
   CHECK( IsAnotherRunning(), false,
          _T("can't call another copy when it doesn't run!") );

   CHECK( m_cmdLineOptions, false,
          _T("we need to parse the options before doing the remote call") );

   wxClient client;
   wxConnectionBase *conn = client.MakeConnection(_T(""), GetIPCSocket(), IPC_TOPIC);
   if ( !conn )
   {
      // failed to connect to server at all
      return false;
   }

   // pass m_cmdLineOptions to the other process
   bool ok = conn->Execute(m_cmdLineOptions->ToString());

   delete conn;

   return ok;
}

// ----------------------------------------------------------------------------
// CmdLineOptions
// ----------------------------------------------------------------------------

// the char to use as separator: don't use NUL as I'm not sure that wxIPC
// code handlers it correctly
#define CMD_LINE_OPTS_SEP _T('\1')

// the version of the CmdLineOptions::ToString() format
#define CMD_LINE_OPTS_VERSION 1.2

// the string versions of TRUE and FALSE
#define CMD_LINE_OPTS_TRUE _T('1')
#define CMD_LINE_OPTS_FALSE _T('0')

// the positions of the individual members in the string produced by ToString()
enum
{
   CmdLineOptions_Version,
   CmdLineOptions_UserDir,
   CmdLineOptions_Config,
   CmdLineOptions_Bcc,
   CmdLineOptions_Body,
   CmdLineOptions_Cc,
   CmdLineOptions_Newsgroups,
   CmdLineOptions_Subject,
   CmdLineOptions_To,
   CmdLineOptions_DebugMail,
   CmdLineOptions_Max
};

String CmdLineOptions::ToString() const
{
   // only serialize the "interesting" options
   String s;
   s << s.Format(_T("%f"), CMD_LINE_OPTS_VERSION) << CMD_LINE_OPTS_SEP
     << userDir << CMD_LINE_OPTS_SEP
     << configFile << CMD_LINE_OPTS_SEP
     << composer.bcc << CMD_LINE_OPTS_SEP
     << composer.body << CMD_LINE_OPTS_SEP
     << composer.cc << CMD_LINE_OPTS_SEP
     << composer.newsgroups << CMD_LINE_OPTS_SEP
     << composer.subject << CMD_LINE_OPTS_SEP
     << composer.to << CMD_LINE_OPTS_SEP
     << (debugMail ? CMD_LINE_OPTS_TRUE : CMD_LINE_OPTS_FALSE);

   return s;
}

bool CmdLineOptions::FromString(const String& s)
{
   wxArrayString
      tokens = wxStringTokenize(s, CMD_LINE_OPTS_SEP, wxTOKEN_RET_EMPTY_ALL);

   if ( tokens.GetCount() != CmdLineOptions_Max )
      return false;

   if ( wxAtof(tokens[CmdLineOptions_Version]) != CMD_LINE_OPTS_VERSION )
      return false;

   userDir = tokens[CmdLineOptions_UserDir];
   configFile = tokens[CmdLineOptions_Config];
   composer.bcc = tokens[CmdLineOptions_Bcc];
   composer.body = tokens[CmdLineOptions_Body];
   composer.cc = tokens[CmdLineOptions_Cc];
   composer.newsgroups = tokens[CmdLineOptions_Newsgroups];
   composer.subject = tokens[CmdLineOptions_Subject];
   composer.to = tokens[CmdLineOptions_To];
   debugMail = tokens[CmdLineOptions_DebugMail] == CMD_LINE_OPTS_TRUE;

   return true;
}

int g_busyCursorYield;

extern void MBeginBusyCursor()
{
   ++g_busyCursorYield;
   wxBeginBusyCursor();
   --g_busyCursorYield;
}

extern void MEndBusyCursor()
{
   ++g_busyCursorYield;
   wxEndBusyCursor();
   --g_busyCursorYield;
}
