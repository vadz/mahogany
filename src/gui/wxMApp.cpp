/*-*- c++ -*-********************************************************
 * wxMApp class: do all GUI specific  application stuff             *
 *                                                                  *
 * (C) 1997,1998 by Karsten Ballüder (Ballueder@usa.net)            *
 *                                                                  *
 * $Id$                *
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

#  include "kbList.h"
#  include "PathFinder.h"
#  include "MimeList.h"
#  include "MimeTypes.h"
#  include "Profile.h"
#  include "MApplication.h"
#  include "gui/wxMApp.h"

   extern "C"
   {
#     include <mail.h>
#     include <osdep.h>  // for sysinbox() &c
   }

#  include <wx/log.h>
#  include <wx/config.h>
#  include <wx/intl.h>
#  include <wx/generic/dcpsg.h>
#endif

#include "Mdefaults.h"
#include "MDialogs.h"

#include "MObject.h"

#include "MHelp.h"

#include "gui/wxMainFrame.h"
#include "gui/wxIconManager.h"
#include "MailCollector.h"

#ifdef OS_WIN
   #include <winnls.h>
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
      { wxLogTrace("Collection timer expired."); mApplication->GetMailCollector()->Collect(); }

    virtual void Stop()
      { if ( m_started ) wxTimer::Stop(); }

public:
    bool m_started;
};

// ----------------------------------------------------------------------------
// global vars
// ----------------------------------------------------------------------------

// a (unique) autosave timer instance
static AutoSaveTimer gs_timerAutoSave;
// a (unique) timer for polling for new mail
static MailCollectionTimer gs_timerMailCollection;

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
   frame->SetIcon(ICON("MFrame"));

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
   CloseSplash();

   if ( m_hasWindow && level == wxLOG_User )
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

// ----------------------------------------------------------------------------
// wxMApp
// ----------------------------------------------------------------------------


BEGIN_EVENT_TABLE(wxMApp, wxApp)
   EVT_IDLE              (wxMApp::OnIdle)
END_EVENT_TABLE()

wxMApp::wxMApp(void)
{
   m_IconManager = NULL;
   m_HelpController = NULL;
   m_CanClose = FALSE;
}

wxMApp::~wxMApp()
{
   if(m_HelpController)
      delete m_HelpController;
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
   // If we get called the second time, we just reuse our old value,
   // but only if it was TRUE.
   if(m_CanClose)
      return m_CanClose;

   // ask the user unless disabled
   if ( ! MDialog_YesNoDialog(_("Do you really want to exit Mahogany?"), m_topLevelFrame,
                            MDIALOG_YESNOTITLE, false,
                            MP_CONFIRMEXIT) )
      return false;

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
}

// app initilization
bool
wxMApp::OnInit()
{
   // Set up locale first, so everything is in the right language.
   bool hasLocale = false;

#ifdef OS_UNIX
   const char *locale = getenv("LANG");
   hasLocale = locale && (strcmp(locale, "C") != 0);
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
      #error "don't know where to find message catalogs on this platform"
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

#if wxUSE_LIBPNG
   wxImage::AddHandler( new wxPNGHandler );
#endif

#if wxUSE_LIBJPEG
   wxImage::AddHandler( new wxJPEGHandler );
#endif

   m_IconManager = new wxIconManager();

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

      // restore our preferred printer settings
    m_PrintData = new wxPrintData;
    m_PageSetupData = new wxPageSetupDialogData;
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

      // start another timer to poll for new mail:
      StartTimer(Timer_PollIncoming);

      // restore the normal behaviour (see the comments above)
      SetExitOnFrameDelete(TRUE);

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
   delete m_PrintData;
   delete m_PageSetupData;

   MAppBase::OnShutDown();

   delete m_IconManager;
   delete m_Locale;

   // FIXME this is not the best place to do it, but at least we're safe
   //       because we now that by now it's unused any more
   ProfileBase::DeleteGlobalConfig();

   MObjectRC::CheckLeaks();

   // delete the previously active log target (it's the one we had set before
   // here, but in fact it doesn't even matter: if somebody installed another
   // one, we will delete his log object and his code will delete ours)
   wxLog *log = wxLog::SetActiveTarget(NULL);
   delete log;

   // as c-client lib doesn't seem to think that deallocating memory is
   // something good to do, do it at it's place...
#ifdef OS_WIN
   free(mail_parameters((MAILSTREAM *)NULL, GET_HOMEDIR, NULL));
   free(mail_parameters((MAILSTREAM *)NULL, GET_NEWSRC, NULL));
#endif // OS_WIN

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
      ((wxExtHelpController *)m_HelpController)->SetBrowser(
         READ_APPCONFIG(MP_HELPBROWSER),
         READ_APPCONFIG(MP_HELPBROWSER_ISNS));
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

         return (delay == 0) || gs_timerAutoSave.Start(delay);

      case Timer_PollIncoming:
         delay = READ_APPCONFIG(MP_POLLINCOMINGDELAY)*1000;

         return (delay == 0) || gs_timerMailCollection.Start(delay);

      case Timer_PingFolder:
         // TODO!!!
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
         gs_timerAutoSave.Stop();
         break;

      case Timer_PollIncoming:
         gs_timerMailCollection.Stop();
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
