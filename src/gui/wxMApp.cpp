/*-*- c++ -*-********************************************************
 * wxMApp class: do all GUI specific  application stuff             *
 *                                                                  *
 * (C) 1997-1999 by Karsten Ballüder (karsten@phy.hw.ac.uk)         *
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
#  include "MimeList.h"
#  include "MimeTypes.h"
#  include "Profile.h"
#  include "MApplication.h"
#  include "gui/wxMApp.h"

#  include "Mcclient.h"

#  include <wx/log.h>
#  include <wx/config.h>
#  include <wx/intl.h>
#  include <wx/generic/dcpsg.h>
#  include <wx/thread.h>
#endif

#include <wx/msgdlg.h>   // for wxMessageBox
#include <wx/cmndata.h>  // for wxPageSetupData
#include <wx/persctrl.h> // for wxPMessageBoxEnable(d)
#include <wx/menu.h>
#include <wx/statusbr.h>

#include "wx/dialup.h"

#include "Mdefaults.h"
#include "MDialogs.h"

#include "MObject.h"

#include "MHelp.h"

#include "gui/wxMainFrame.h"
#include "gui/wxIconManager.h"
#include "MailCollector.h"
#include "MModule.h"

#ifdef OS_WIN
#   include <winnls.h>
#endif

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// the name of config section where the log window position is saved
#define  LOG_FRAME_SECTION    "LogWindow"

// ----------------------------------------------------------------------------
// classes
// ----------------------------------------------------------------------------

// a small class implementing saving/restoring position for the log frame
class wxMLogWindow : public wxLogWindow
{
public:
   // ctor (creates the frame hidden)
   wxMLogWindow(wxFrame *pParent, const char *szTitle);

   // override base class virtual to implement saving the frame position
   virtual void OnFrameDelete(wxFrame *frame);

protected:
   // we handle verbose messages in a special way
   virtual void DoLog(wxLogLevel level, const wxChar *szString, time_t t);

private:
   bool m_hasWindow;
};

// a timer used to periodically autosave profile settings
class AutoSaveTimer : public wxTimer
{
public:
   AutoSaveTimer() { m_started = FALSE; }

   virtual bool Start( int millisecs = -1, bool oneShot = FALSE )
      { m_started = TRUE; return wxTimer::Start(millisecs, oneShot); }

   virtual void Notify()
      { wxLogTrace("Autosaving everything."); ProfileBase::FlushAll(); }

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
         wxLogTrace("Collection timer expired.");
         mApplication->UpdateOutboxStatus();
         mApplication->GetMailCollector()->Collect();
      }

    virtual void Stop()
      { if ( m_started ) wxTimer::Stop(); }

public:
    bool m_started;
};

// a timer used to do idle processing
class IdleTimer : public wxTimer
{
public:
   IdleTimer() : wxTimer() { Start(); }
   virtual bool Start() { return wxTimer::Start(100); }

   virtual void Notify()
      { MEventManager::DispatchPending(); }
};


// ----------------------------------------------------------------------------
// global vars
// ----------------------------------------------------------------------------

// a (unique) autosave timer instance
static AutoSaveTimer *gs_timerAutoSave = NULL;
// a (unique) timer for polling for new mail
static MailCollectionTimer *gs_timerMailCollection = NULL;


struct MModuleEntry
{
   MModuleEntry(MModule *m) { m_Module = m; }
   MModule *m_Module;
};

KBLIST_DEFINE(ModulesList, MModuleEntry);
// a list of modules loaded at startup:
static ModulesList gs_GlobalModulesList;

// this creates the one and only application object
IMPLEMENT_APP(wxMApp);

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxMLogWindow
// ----------------------------------------------------------------------------
wxMLogWindow::wxMLogWindow(wxFrame *pParent, const char *szTitle)
            : wxLogWindow(pParent, szTitle, FALSE)
{
   int x, y, w, h;
   wxMFrame::RestorePosition(LOG_FRAME_SECTION, &x, &y, &w, &h);
   wxFrame *frame = GetFrame();

   frame->SetSize(x, y, w, h);
   frame->SetIcon(ICON("MLogFrame"));

   m_hasWindow = true;
   Show();
}

void wxMLogWindow::OnFrameDelete(wxFrame *frame)
{
   wxMFrame::SavePosition(LOG_FRAME_SECTION, frame);

   m_hasWindow = false;

   wxLogWindow::OnFrameDelete(frame);
}

void wxMLogWindow::DoLog(wxLogLevel level, const wxChar *szString, time_t t)
{
   // close the splash screen as the very first thing because otherwise it
   // would float on top of our message boxes
   //CloseSplash(); -- not any more, we have our SplashKillerLog installed

   if ( m_hasWindow ) //&& level == wxLOG_User )
   {
      // this will call wxLogWindow::DoLogString()
      wxLog::DoLog(level, szString, t);
   }
   else
   {
      // leave the base class handle it
      wxLogWindow::DoLog(level, szString, t);
   }
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
         *m_OnlineIcon = ICON( "online");
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
}

wxMApp::~wxMApp()
{
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

#ifdef __WXMSW__
   ::MessageBox(NULL, _("The application is terminating abnormally.\n"
                        "Please report the bug to m-developers@makelist.com\n"
                        "Thank you!"),
                "Fatal application error",
                MB_ICONSTOP);
#endif // MSW only for now
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
   wxWindowList::Node *node = wxTopLevelWindows.GetFirst();
   while ( node )
   {
      wxWindow *win = node->GetData();
      node = node->GetNext();

      if ( win->IsKindOf(CLASSINFO(wxMFrame)) )
      {
         wxMFrame *frame = (wxMFrame *)win;

         // force closing the frame
         frame->Close(TRUE);
      }
   }

#if wxUSE_HELP && wxUSE_HTML
   /// Close the help frame if it is open:
   wxFrame *hf = NULL;
   if(m_HelpController
      && m_HelpController->IsKindOf(CLASSINFO(wxHelpControllerHtml)))
      hf = ((wxHelpControllerHtml *)m_HelpController)->GetFrameParameters();
   if(hf) hf->Close(TRUE);
#endif // wxHTML-based help

   // shut down MEvent handling

   if(m_IdleTimer)
   {
      m_IdleTimer->Stop();
      delete m_IdleTimer;
      m_IdleTimer = NULL; // paranoid
   }
}

// app initilization
bool
wxMApp::OnInit()
{
   m_topLevelFrame = NULL;
   // Set up locale first, so everything is in the right language.
   bool hasLocale = false;

#ifdef OS_UNIX
   const char *locale = getenv("LANG");
   hasLocale = locale &&
               (strcmp(locale, "C") != 0) &&
               (strcmp(locale, "en") != 0) &&
               (strcmp(locale, "us") != 0);
#elif defined(OS_WIN)
   // this variable is not usually set under Windows, but give the user a
   // chance to override our locale detection logic in case it doesn't work
   // (which is well possible because it's totally untested)
   const char *locale = getenv("LANG");
   if ( locale )
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

   // if we fail to load the msg catalog, remember it in this variable because
   // we can't remember this in the profile yet - it is not yet created
   bool failedToLoadMsgs = false;

   if ( hasLocale )
   {
      m_Locale = new wxLocale(locale, locale, NULL, false);
#ifdef OS_UNIX
      String localePath;
      localePath << M_BASEDIR << "/locale";
#elif defined(OS_WIN)
      InitGlobalDir();
      String localePath;
      localePath << m_globalDir << "/locale";
#else
#   error "don't know where to find message catalogs on this platform"
#endif // OS
      m_Locale->AddCatalogLookupPathPrefix(localePath);

      // TODO should catch the error messages and save them for later
      wxLogNull noLog;

      bool ok = m_Locale->AddCatalog("wxstd");
      ok |= m_Locale->AddCatalog(M_APPLICATIONNAME);

      if ( !ok )
      {
         // better use English messages if msg catalog was not found
         delete m_Locale;
         m_Locale = NULL;

         failedToLoadMsgs = true;
      }
   }
   else
      m_Locale = NULL;

   wxInitAllImageHandlers();

   m_IconManager = new wxIconManager();

   m_OnlineManager = wxDialUpManager::Create();
   m_PrintData = new wxPrintData;
   m_PageSetupData = new wxPageSetupDialogData;

   // this is necessary to avoid that the app closes automatically when we're
   // run for the first time and show a modal dialog before opening the main
   // frame - if we don't do it, when the dialog (which is the last app window
   // at this moment) disappears, the app will close.
   SetExitOnFrameDelete(FALSE);

   if ( OnStartup() )
   {
      // only now we can use profiles
      if ( failedToLoadMsgs )
      {
         // if we can't load the msg catalog for the given locale once, don't
         // try it again next time - the flag is stored in the profile in this
         // key
         wxString configPath;
         configPath << "UseLocale_" << locale;

         ProfileBase *profile = GetProfile();
         if ( profile->readEntry(configPath, 1l) != 0l )
         {
            CloseSplash();

            // the strings here are intentionally not translated
            wxString msg;
            msg.Printf("Impossible to load message catalog(s) for the "
                       "locale '%s', do you want to retry next time?",
                       locale);
            if ( wxMessageBox(msg, "Error",
                              wxICON_STOP | wxYES_NO) != wxYES )
            {
               profile->writeEntry(configPath, 0l);
            }
         }
         //else: the user had previously answered "no" to the question above,
         //      don't complain any more about missing catalogs
      }

      // now we can create the log window
      if ( READ_APPCONFIG(MP_SHOWLOG) ) {
         (void)new wxMLogWindow(m_topLevelFrame, _("Mahogany : Activity Log"));

         // we want it to be above the log frame
         m_topLevelFrame->Raise();
      }

      // at this moment we're fully initialized, i.e. profile and log
      // subsystems are working and the main window is created

#if 0
      // If we are not in the default location:
      if(GetGlobalDir() != wxString(M_BASEDIR)+"/Mahogany") // broken?
      {
         if( locale && (strcmp(locale, "C") != 0) )
         {
            String localePath;
            localePath << GetGlobalDir() << "/locale";
            m_Locale->AddCatalogLookupPathPrefix(localePath);
            m_Locale->AddCatalog(M_APPLICATIONNAME);
         }
      }
#endif

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


      // create timers
      gs_timerAutoSave = new AutoSaveTimer;
      gs_timerMailCollection = new MailCollectionTimer;

      // start a timer to autosave the profile entries
      StartTimer(Timer_Autosave);

      // start another timer to poll for new mail:
      StartTimer(Timer_PollIncoming);

      // another timer to do MEvent handling:
      m_IdleTimer = new IdleTimer;

      // restore the normal behaviour (see the comments above)
      SetExitOnFrameDelete(TRUE);

      // reflect settings in menu and statusbar:
      UpdateOnlineDisplay();

      // enable/disable modules button:
      ((wxMainFrame *)m_topLevelFrame)->UpdateToolBar();
      return true;
   }
   else
   {
      ERRORMESSAGE(("Can't initialize application, terminating."));
      return false;
   }
}

MFrame *wxMApp::CreateTopLevelFrame()
{
   m_topLevelFrame = GLOBAL_NEW wxMainFrame();
   m_topLevelFrame->Show(true);
   SetTopWindow(m_topLevelFrame);
   return m_topLevelFrame;
}

int wxMApp::OnRun()
{
   return wxApp::OnRun();
}

int wxMApp::OnExit()
{
   // disable timers for autosave and mail collection, won't need them any more
   StopTimer(Timer_Autosave);
   StopTimer(Timer_PollIncoming);

   // delete timers
   delete gs_timerAutoSave;
   delete gs_timerMailCollection;

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
   delete m_Locale;
   delete m_OnlineManager;
   // FIXME this is not the best place to do it, but at least we're safe
   //       because we now that by now it's unused any more
   ProfileBase::DeleteGlobalConfig();

   MObjectRC::CheckLeaks();
   MObject::CheckLeaks();

   // delete the previously active log target (it's the one we had set before
   // here, but in fact it doesn't even matter: if somebody installed another
   // one, we will delete his log object and his code will delete ours)
   wxLog *log = wxLog::SetActiveTarget(NULL);
   delete log;
   return 0;
}

void
wxMApp::Help(int id, wxWindow *parent)
{
   // store help key used in search
   static wxString last_key;

   // first thing: close splash if it's still there
   CloseSplash();

   if(! m_HelpController)
   {
      bool ok;
      m_HelpController = new wxHelpController;
      wxString helpfile;
#ifdef OS_UNIX
#if ! wxUSE_HTML
      ((wxExtHelpController *)m_HelpController)->SetBrowser(
         READ_APPCONFIG(MP_HELPBROWSER),
         READ_APPCONFIG(MP_HELPBROWSER_ISNS));
#endif
      helpfile = GetGlobalDir()+"/doc";
#else // Windows
      helpfile = GetGlobalDir()+"\\doc\\Mahogany.hlp";
#endif // Unix/Windows
      // initialise the help system
      ok = m_HelpController->Initialize(helpfile);
      if(! ok)
      {
         wxString msg = _("Cannot initialise help system.\n");
         msg << _("Help file '") << helpfile << _("' not found.");
         wxLogError(msg);
         return ;
      }
      wxSize size = wxSize(
         READ_APPCONFIG(MP_HELPFRAME_WIDTH),
         READ_APPCONFIG(MP_HELPFRAME_HEIGHT));
      wxPoint pos = wxPoint(
         READ_APPCONFIG(MP_HELPFRAME_XPOS),
         READ_APPCONFIG(MP_HELPFRAME_YPOS));
      m_HelpController->SetFrameParameters("Mahogany : %s", size, pos);
   }
   switch(id)
   {
      // look up contents:
   case   MH_CONTENTS:
      m_HelpController->DisplayContents();
      break;
      // implement a search:
   case MH_SEARCH:
      if( MInputBox(&last_key,
                     _("Search for?"),
                     _("Search help for keyword"),
                    parent ? parent : TopLevelFrame())
                    && ! last_key.IsEmpty())
         m_HelpController->KeywordSearch(last_key);
      break;
      // all other help ids, just look them up:
   default:
#ifdef OS_WIN
      // context-sensitive help doesn't work currently, try to at least do
      // something in response to the commands from the program "Help menu"
      if ( id == MH_CONTENTS )
         m_HelpController->DisplayContents();
      else
#endif // OS_WIN

      if(! m_HelpController->DisplaySection(id))
      {
         wxString str;
         str.Printf(_("No help found for current context (%d)."), id);
         MDialog_Message(str,NULL,_("Sorry"));
      }
      break;
   }
}

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
         INFOMESSAGE(("Successfully loaded module:\nName: %s\nDescr: %s\nVersion: %s\n",
                   module->GetName(),
                   module->GetDescription(),
                   module->GetVersion()));

#endif
      }
   }
}

void
wxMApp::UnloadModules(void)
{
   ModulesList::iterator i;
   for(i = gs_GlobalModulesList.begin();
       i != gs_GlobalModulesList.end();
       i++)
      (**i).m_Module->DecRef();
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
         delay = READ_APPCONFIG(MP_AUTOSAVEDELAY)*1000;

         return (delay == 0) || gs_timerAutoSave->Start(delay);

      case Timer_PollIncoming:
         delay = READ_APPCONFIG(MP_POLLINCOMINGDELAY)*1000;

         return (delay == 0) || gs_timerMailCollection->Start(delay);

      case Timer_PingFolder:
         // TODO - just sending an event "restart timer" which all folders
         //        would be subscribed too should be enough
         return true;

      default:
         wxFAIL_MSG("attempt to start an unknown timer");
         return false;
   }
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

      default:
         wxFAIL_MSG("attempt to stop an unknown timer");
         return false;
   }

   return true;
}

#ifndef USE_THREADS

void
wxMApp::ThrEnterLeave(bool /* enter */, SectionId /*what*/, bool /* testing */)
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
   static int statusIconWidth = -1;
      
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
   if(m_UseOutbox)
      widths[GetStatusField(SF_OUTBOX)] = 100;
   //FIXME: wxGTK crashes after calling this repeatedly sbar->SetFieldsCount(n, widths);
}

void
wxMApp::UpdateOutboxStatus(void) const
{
   ASSERT(m_topLevelFrame);

   UIdType nNNTP, nSMTP;
   bool enable = CheckOutbox(&nSMTP, &nNNTP);

   // only enable menu item if outbox is used and contains messages:
   ASSERT(m_topLevelFrame->GetMenuBar());
   m_topLevelFrame->GetMenuBar()->Enable(
      (int)WXMENU_FILE_SEND_OUTBOX,enable && m_UseOutbox);

   if(! m_UseOutbox)
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


void
wxMApp::SetupOnlineManager(void)
{
   ASSERT(m_OnlineManager);
   m_DialupSupport = READ_APPCONFIG(MP_DIALUP_SUPPORT) != 0;

   if(m_DialupSupport)
   {
      if(! m_OnlineManager->EnableAutoCheckOnlineStatus(60))
      {
         wxLogError(_("Cannot activate auto-check for dial-up network status."));
      }

      String beaconhost = READ_APPCONFIG(MP_BEACONHOST);
      strutil_delwhitespace(beaconhost);
      // If no host configured, use smtp host:
      if(beaconhost.length() > 0)
         m_OnlineManager->SetWellKnownHost(beaconhost);
      else
      {
         beaconhost = READ_APPCONFIG(MP_SMTPHOST);
         m_OnlineManager->SetWellKnownHost(beaconhost, 25);
      }
      m_OnlineManager->SetConnectCommand(
         READ_APPCONFIG(MP_NET_ON_COMMAND),
         READ_APPCONFIG(MP_NET_OFF_COMMAND));
      m_IsOnline = m_OnlineManager->IsOnline();
   }
   else
   {
      m_OnlineManager->DisableAutoCheckOnlineStatus();
      m_IsOnline = TRUE;
   }
   UpdateOnlineDisplay();
}

bool
wxMApp::IsOnline(void) const
{
   if(! m_DialupSupport)
      return TRUE; // no dialup--> always connected
   else
      // make sure we always have the very latest value:
      return ((wxMApp*)this)->m_IsOnline = m_OnlineManager->IsOnline();
}

void
wxMApp::GoOnline(void) const
{
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
