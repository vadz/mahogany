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
   GetFrame()->SetSize(x, y, w, h);
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
      m_Locale->AddCatalog("wxstd");
      m_Locale->AddCatalog(M_APPLICATIONNAME);
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

   if ( OnStartup() )
   {
      // now we can create the log window
      if ( READ_APPCONFIG(MP_SHOWLOG) ) {
         (void)new wxMLogWindow(m_topLevelFrame, _("Mahogany: Activity Log"));

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
      long delay = READ_APPCONFIG(MP_AUTOSAVEDELAY)*1000;
      if ( delay > 0 )
         gs_timerAutoSave.Start(delay);

      // start another timer to poll for new mail:
      delay = READ_APPCONFIG(MP_POLLINCOMINGDELAY)*1000;
      if ( delay > 0 )
         gs_timerMailCollection.Start(delay);

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
   gs_timerAutoSave.Stop();
   gs_timerMailCollection.Stop();

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
#  if 0 // def OS_WIN
      free(sysinbox());
      free(myhomedir());
      free(myusername());
#  endif

   return 0;
}

void
wxMApp::Help(int id, wxWindow *parent)
{
   // store help key used in search
   static wxString last_key;

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
      helpfile = GetGlobalDir()+"\\doc\\M.hlp";
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
      if(! m_HelpController->DisplaySection(id))
      {
         wxString str;
         str.Printf(_("No help found for current context (%d)."), id);
         MDialog_Message(str,NULL,_("Sorry"));
      }
      break;
   }
}
