//////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   src/gui/wxMSplash.cpp
// Purpose:     splash screen/about dialog implementation
// Author:      Vadim Zeitlin
// Created:     2006-06-04 (extracted from wxMDialogs.cpp)
// CVS-ID:      $Id$
// Copyright:   (c) 1998-2006 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
#  include "Mcommon.h"
#  include "Mdefaults.h"
#  include "MApplication.h"
#  include "gui/wxMFrame.h"
#  include "gui/wxIconManager.h"

#  include <wx/timer.h>
#endif

#include "Mversion.h"

#include <wx/statbmp.h>
#include <wx/fs_mem.h>
#include <wx/html/htmlwin.h>

// from wxMDialogs.cpp
extern void ReallyCloseTopLevelWindow(wxFrame *frame);

// ----------------------------------------------------------------------------
// the images names
// ----------------------------------------------------------------------------

#if defined(OS_WIN)
#  define Mahogany      "Mahogany"
#  define PythonPowered "PythonPowered"
#else   //real XPMs
#  include "../src/icons/wxlogo.xpm"
#  ifdef USE_SSL
#     include "../src/icons/ssllogo.xpm"
#  endif
#  ifdef USE_PYTHON
#     include "../src/icons/PythonPowered.xpm"
#  endif
#endif  //Win/Unix

// under Windows, we might not have PNG support compiled in, but we always have
// BMPs, so fall back to them
#if defined(__WINDOWS__) && !wxUSE_PNG
   #define MEMORY_FS_FILE_EXT ".bmp"
   #define MEMORY_FS_FILE_FMT wxBITMAP_TYPE_BMP
#else // either not Windows or we do have PNG support
   #define MEMORY_FS_FILE_EXT ".png"
   #define MEMORY_FS_FILE_FMT wxBITMAP_TYPE_PNG
#endif

// ----------------------------------------------------------------------------
// globals
// ----------------------------------------------------------------------------

wxFrame *g_pSplashScreen = NULL;

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_SPLASHDELAY;

// ----------------------------------------------------------------------------
// local classes
// ----------------------------------------------------------------------------

class AboutWindow;

// timer which calls our DoClose() when it expires
class SplashCloseTimer : public wxTimer
{
public:
   SplashCloseTimer(AboutWindow *window)
   {
      m_window = window;
      Start(READ_APPCONFIG(MP_SPLASHDELAY)*1000, true /* single shot */);
   }

   virtual void Notify();

private:
   AboutWindow *m_window;

   DECLARE_NO_COPY_CLASS(SplashCloseTimer)
};

// ----------------------------------------------------------------------------
// AboutWindow implementation
// ----------------------------------------------------------------------------

// the main difference is that it goes away as soon as you click it
// or after some time (if not disabled in the ctor).
//
// It is also unique and must be removed before showing any message boxes
// (because it has on top attribute) with CloseSplash() function.
class AboutWindow : public wxWindow
{
public:
  // show the bitmap and the about text in the window
  AboutWindow(wxFrame *parent, wxBitmap bmp, bool bCloseOnTimeout = true);

  /// stop the timer
  void StopTimer(void)
  {
     if ( m_pTimer )
     {
        delete m_pTimer;
        m_pTimer = NULL;
     }
  }

  // close the about frame
  void DoClose()
  {
     StopTimer();

     wxFrame *parent = wxDynamicCast(GetParent(), wxFrame);
     CHECK_RET( parent, _T("should have the splash frame as parent!") );

     ReallyCloseTopLevelWindow(parent);
  }

private:
   // mouse and key event handlers close the window
   void OnClick(wxMouseEvent&) { DoClose(); }
   void OnChar(wxKeyEvent& event)
   {
      switch( event.GetKeyCode() )
      {
         case WXK_UP:
         case WXK_DOWN:
         case WXK_PAGEUP:
         case WXK_PAGEDOWN:
            // these keys are used for scrolling the window contents
            event.Skip();
            break;

         default:
            DoClose();
      }
   }

   // handle key presses/mouse clicks in the given window
   void ConnectMouseAndKeyEvents(wxWindow *win);


   SplashCloseTimer *m_pTimer;

   DECLARE_NO_COPY_CLASS(AboutWindow)
};

AboutWindow::AboutWindow(wxFrame *parent, wxBitmap bmp, bool bCloseOnTimeout)
           : wxWindow(parent, -1, wxDefaultPosition,
                      wxSize(bmp.GetWidth(), 2*bmp.GetHeight()))
{
   const wxSize sizeBmp(bmp.GetWidth(), bmp.GetHeight());
   wxStaticBitmap *top = new wxStaticBitmap(this, wxID_ANY, bmp,
                                            wxPoint(0, 0), sizeBmp);
   wxHtmlWindow *bottom = new wxHtmlWindow(this, wxID_ANY,
                                           wxPoint(0, sizeBmp.y + 1), sizeBmp,
                                           wxHW_DEFAULT_STYLE | wxHW_NO_SELECTION);

   ConnectMouseAndKeyEvents(top);
   ConnectMouseAndKeyEvents(bottom);

   // prepare the images used in HTML
   wxMemoryFSHandler::AddFile(_T("wxlogo") MEMORY_FS_FILE_EXT, wxBITMAP(wxlogo), MEMORY_FS_FILE_FMT);

#ifdef USE_SSL
   wxMemoryFSHandler::AddFile(_T("ssllogo") MEMORY_FS_FILE_EXT, wxBITMAP(ssllogo), MEMORY_FS_FILE_FMT);
#endif // USE_SSL

#ifdef USE_PYTHON
   wxMemoryFSHandler::AddFile(_T("pythonpowered") MEMORY_FS_FILE_EXT, wxBITMAP(PythonPowered), MEMORY_FS_FILE_FMT);
#endif // USE_PYTHON

#define HTML_IMAGE(name) \
   "<center><img src=\"memory:" #name MEMORY_FS_FILE_EXT "\"></center><br>"

#define HTML_WARNING "<font color=#ff0000><b>WARNING: </b></font>"

   // build the page text from several strings to be able to translate them
   // ---------------------------------------------------------------------

#ifdef USE_I18N
   // make special provision for the translators name
   static const wxChar *TRANSLATOR_NAME =
      gettext_noop("Translate this into your name (for About dialog)");
   wxString translator = wxGetTranslation(TRANSLATOR_NAME);
   if ( translator == TRANSLATOR_NAME )
   {
      // not translated, don't show
      translator.clear();
   }
   else
   {
      wxString tmp;
      tmp << _T(" (") << _("translations by ") << translator << _T(')');
      translator = tmp;
   }
#endif // USE_I18N

   wxString pageHtmlText;

   pageHtmlText << _T("<body text=#000000 bgcolor=#ffffff>"
                   "<meta HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html;charset=windows-1257\">"
                   "<font face=\"Times New Roman,times\">"

                   "<h4>") << _("Mahogany information") << _T("</h4>")
                << _("Version ") << M_VERSION_STRING
                << _("  built with ") << wxVERSION_STRING << _T("<br>")
#ifdef DEBUG
                   HTML_WARNING << _("This is a debug build") << _T("<br>")
#else
                << _("Release build ")
#endif
#if wxUSE_UNICODE
                << _("Unicode build ")
#endif
                << _("(compiled at ") << __DATE__ ", " __TIME__ ")<br>"

#if defined(USE_SSL) || defined(USE_THREADS) || defined(USE_PYTHON)
                   _T("<h4>") << _("Extra features:") << _T("</h4>")
#ifdef USE_SSL
                << _("SSL support") << _T("<br>")
#endif
#ifdef USE_THREADS
                << _("Threads") << _T("<br>")
#endif
#ifdef USE_PYTHON
                << _("Embedded Python interpreter") << _T("<br>")
#endif
#ifdef USE_DIALUP
                << _("Dial-up support") << _T("<br>")
#endif
#ifdef USE_I18N
                << _("Internationalization") << translator << _T("<br>")
#endif
#endif // USE_XXX

#ifdef EXPERIMENTAL
                << HTML_WARNING << _("Includes experimental code")
                                << " (" EXPERIMENTAL ")"
#endif

                << "<p>"
                   _T("<h4>") << _("List of contributors:") << _T("</h4>")
                   "<p>"
                   "Karsten Ball&uuml;der, Vadim Zeitlin, Greg Noel,<br> "
                   "Nerijus Bali&#363;nas, Xavier Nodet, Vaclav Slavik,<br>"
                   "Daniel Seifert, Michele Ravani, Michael A Chase,<br>"
                   "Robert Vazan " << _("and many others") << "<br>"
                   "<br>"
                   _T("<i>") << _("The Mahogany team") << _T("</i><br>")
                   "<font size=2>"
                   "(<tt>mahogany-developers@lists.sourceforge.net</tt>)"
                   "</font>"
                   "<hr>"
                   HTML_IMAGE(wxlogo)
                << _("Mahogany is built on the cross-platform C++ framework "
                     "wxWidgets (http://www.wxwidgets.org/).")
                << _T("<p>")
                << _("This product includes software developed and copyright "
                     "by the University of Washington written by Mark Crispin.")
                <<
#ifdef USE_SSL
                   _T("<p>")
                   HTML_IMAGE(ssllogo)
                << _("This product includes software developed by the OpenSSL Project "
                     "for use in the OpenSSL Toolkit. (http://www.openssl.org/).<br>"
                     "This product includes cryptographic software written by Eric Young (eay@cryptsoft.com)<br>"
                     "This product includes software written by Tim Hudson (tjh@cryptsoft.com)<br>")
                <<
#endif // USE_SSL

#ifdef USE_PYTHON
                   _T("<p>")
                   HTML_IMAGE(pythonpowered)
                << _("This program contains an embedded Python interpreter.")
                <<
#endif // USE_PYTHON
                   _T("<hr>")
                << _("Special thanks to Daniel Lord for hardware donations.")
                << _T("<p>")
                << _("The Mahogany Team would also like to acknowledge "
                     "the support of ")
                << _("Anthemion Software, "
                   "Heriot-Watt University, "
                   "SourceForge.net, "
                   "SourceGear.com, "
                   "GDev.net, "
                   "Simon Shapiro, "
                   "VA Linux, "
                   "and SuSE GmbH.")
                  ;

#undef HTML_IMAGE
#undef HTML_WARNING

   bottom->SetPage(pageHtmlText);

   wxMemoryFSHandler::RemoveFile(_T("wxlogo") MEMORY_FS_FILE_EXT);
#ifdef USE_SSL
   wxMemoryFSHandler::RemoveFile(_T("ssllogo") MEMORY_FS_FILE_EXT);
#endif // USE_SSL
#ifdef USE_PYTHON
   wxMemoryFSHandler::RemoveFile(_T("pythonpowered") MEMORY_FS_FILE_EXT);
#endif

   bottom->SetFocus();

   // start a timer which will close us (if not disabled)
   m_pTimer = bCloseOnTimeout ? new SplashCloseTimer(this) : NULL;
}

void AboutWindow::ConnectMouseAndKeyEvents(wxWindow *win)
{
   win->Connect(wxEVT_CHAR,
                wxKeyEventHandler(AboutWindow::OnChar), NULL, this);
   win->Connect(wxEVT_LEFT_UP,
                wxMouseEventHandler(AboutWindow::OnClick), NULL, this);
   win->Connect(wxEVT_RIGHT_UP,
                wxMouseEventHandler(AboutWindow::OnClick), NULL, this);
   win->Connect(wxEVT_MIDDLE_UP,
                wxMouseEventHandler(AboutWindow::OnClick), NULL, this);
}

// ----------------------------------------------------------------------------
// SplashCloseTimer
// ----------------------------------------------------------------------------

void
SplashCloseTimer::Notify()
{
   m_window->DoClose();
}

// ----------------------------------------------------------------------------
// wxAboutFrame: the top level about dialog window itself
// ----------------------------------------------------------------------------

class wxAboutFrame : public wxFrame
{
public:
   wxAboutFrame(bool bCloseOnTimeout);
   virtual ~wxAboutFrame();

   void OnClose(wxCloseEvent& event)
   {
      m_Window->StopTimer();

      event.Skip();
   }

private:
   AboutWindow *m_Window;

   wxLog *m_logSplash;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxAboutFrame)
};

BEGIN_EVENT_TABLE(wxAboutFrame, wxFrame)
   EVT_CLOSE(wxAboutFrame::OnClose)
END_EVENT_TABLE()

wxAboutFrame::wxAboutFrame(bool bCloseOnTimeout)
            : wxFrame(NULL, -1, _("Welcome"),
                      wxDefaultPosition, wxDefaultSize,
                      /* no border styles at all */ wxSTAY_ON_TOP )
{
   wxCHECK_RET( g_pSplashScreen == NULL, _T("one splash is more than enough") );

   g_pSplashScreen = (wxMFrame *)this;

   m_Window = new AboutWindow
                  (
                     this,
                     mApplication->GetIconManager()->GetBitmap(_T("Msplash")),
                     bCloseOnTimeout
                  );

   SetClientSize(m_Window->GetSize());

   Centre(wxCENTRE_ON_SCREEN | wxBOTH);
   Show(true);
}

wxAboutFrame::~wxAboutFrame()
{
   g_pSplashScreen = NULL;
}

// ----------------------------------------------------------------------------
// global functions
// ----------------------------------------------------------------------------

extern void CloseSplash()
{
   if ( g_pSplashScreen )
   {
      // do close the splash
      ReallyCloseTopLevelWindow(g_pSplashScreen);
   }
}

void
MDialog_AboutDialog(const wxWindow * /* parent */, bool bCloseOnTimeout)
{
   if ( !g_pSplashScreen )
      (void)new wxAboutFrame(bCloseOnTimeout);

   wxYield();
}

