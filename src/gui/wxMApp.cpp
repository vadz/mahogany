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
#  include   <wx/generic/dcpsg.h>
#endif

#include "Mdefaults.h"
#include "MDialogs.h"

#include "MObject.h"

#include "MHelp.h"

#include "gui/wxMainFrame.h"
#include "gui/wxIconManager.h"

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

// ----------------------------------------------------------------------------
// global vars
// ----------------------------------------------------------------------------

// a (unique) autosave timer instance
static AutoSaveTimer gs_timerAutoSave;

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
   Show();
}

void wxMLogWindow::OnFrameDelete(wxFrame *frame)
{
   wxMFrame::SavePosition(LOG_FRAME_SECTION, frame);

   wxLogWindow::OnFrameDelete(frame);
}

// ----------------------------------------------------------------------------
// wxMApp
// ----------------------------------------------------------------------------

wxMApp::wxMApp(void)
{
   m_IconManager = NULL;
   m_HelpController = NULL;
}

wxMApp::~wxMApp()
{
   if(m_HelpController)
      delete m_HelpController;
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
    wxNode *node = wxTopLevelWindows.First();
    while ( node )
    {
        wxWindow *win = (wxWindow *)node->Data();
        node = node->Next();

        if ( win->IsKindOf(CLASSINFO(wxMFrame)) )
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

    return MAppBase::CanClose();
}

// app initilization
bool
wxMApp::OnInit()
{
   // Set up locale first, so everything is in the right language.
   const char * locale = getenv("LANG");
   if( locale && (strcmp(locale, "C") != 0) )
   {
      m_Locale = new wxLocale(locale, locale, "");
      String localePath ;
      localePath << M_BASEDIR << "/locale";
      m_Locale->AddCatalogLookupPathPrefix(localePath);
      m_Locale->AddCatalog(M_APPLICATIONNAME);
   }
   else
      m_Locale = NULL;

#ifdef wxUSE_LIBPNG
   wxImage::AddHandler( new wxPNGHandler );
#endif
   
#ifdef wxUSE_LIBJPEG
// we don't use it yet, broken   wxImage::AddHandler( new wxJPEGHandler );
#endif

   m_IconManager = new wxIconManager();

   if ( OnStartup() )
   {
      // now we can create the log window
      if ( READ_APPCONFIG(MP_SHOWLOG) ) {
         (void)new wxMLogWindow(m_topLevelFrame, _("M Activity Log"));

         // we want it to be above the log frame
         m_topLevelFrame->Raise();
      }

      // at this moment we're fully initialized, i.e. profile and log
      // subsystems are working and the main window is created

      // restore our preferred printer settings
#if wxUSE_POSTSCRIPT
      wxThePrintSetupData->SetPrinterCommand(READ_APPCONFIG(MP_PRINT_COMMAND));
      wxThePrintSetupData->SetPrinterOptions(READ_APPCONFIG(MP_PRINT_OPTIONS));
      wxThePrintSetupData->SetPrinterOrientation(READ_APPCONFIG(MP_PRINT_ORIENTATION));
      wxThePrintSetupData->SetPrinterMode(READ_APPCONFIG(MP_PRINT_MODE));
      wxThePrintSetupData->SetPaperName(READ_APPCONFIG(MP_PRINT_PAPER));
      wxThePrintSetupData->SetPrinterFile(READ_APPCONFIG(MP_PRINT_FILE));
      wxThePrintSetupData->SetColour(READ_APPCONFIG(MP_PRINT_COLOUR));
#endif // wxUSE_POSTSCRIPT
      
      // start a timer to autosave the profile entries
      long delay = READ_APPCONFIG(MP_AUTOSAVEDELAY);
      if ( delay > 0 )
         gs_timerAutoSave.Start(delay);

      return true;
   }
   else {
      ERRORMESSAGE(("Can't initialize application, terminating."));

      return false;
   }
}

MFrame *wxMApp::CreateTopLevelFrame()
{
   m_topLevelFrame = GLOBAL_NEW wxMainFrame();
   m_topLevelFrame->SetTitle(M_TOPLEVELFRAME_TITLE);
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
   // disable autosave, won't need it any more
   gs_timerAutoSave.Stop();

#if wxUSE_POSTSCRIPT
   // save our preferred printer settings
   m_profile->writeEntry(MP_PRINT_COMMAND, wxThePrintSetupData->GetPrinterCommand());
   m_profile->writeEntry(MP_PRINT_OPTIONS, wxThePrintSetupData->GetPrinterOptions());
   m_profile->writeEntry(MP_PRINT_ORIENTATION, wxThePrintSetupData->GetPrinterOrientation());
   m_profile->writeEntry(MP_PRINT_MODE, wxThePrintSetupData->GetPrinterMode());
   m_profile->writeEntry(MP_PRINT_PAPER, wxThePrintSetupData->GetPaperName());
   m_profile->writeEntry(MP_PRINT_FILE, wxThePrintSetupData->GetPrinterFile());
   m_profile->writeEntry(MP_PRINT_COLOUR, wxThePrintSetupData->GetColour());
#endif // wxUSE_POSTSCRIPT

   MAppBase::OnShutDown();

   delete m_IconManager;
   delete m_Locale;

   MObjectRC::CheckLeaks();

   // delete the previously active log target (it's the one we had set before
   // here, but in fact it doesn't even matter: if somebody installed another
   // one, we will delete his log object and his code will delete ours)
   wxLog *log = wxLog::SetActiveTarget(NULL);
   delete log;

   // as c-client lib doesn't seem to think that deallocating memory is
   // something good to do, do it at it's place...
#  ifdef OS_WIN
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
      last_key = wxGetTextFromUser(_("Search for?"),
                                   _("Search help for keyword"),
                                   last_key,
                                   parent ? parent : TopLevelFrame());
      if(! last_key.IsEmpty())
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
