/*-*- c++ -*-********************************************************
 * wxMDialogs.h : wxWindows version of dialog boxes                 *
 *                                                                  *
 * (C) 1998-2000 by Karsten Ballüder (karsten@phy.hw.ac.uk)         *
 *                                                                  *
 * $Id$
 *******************************************************************/

#ifdef __GNUG__
#   pragma implementation "wxMDialogs.h"
#endif

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
#   include "Mcommon.h"
#   include "Mdefaults.h"
#   include "guidef.h"
#   include "strutil.h"
#   include "MFrame.h"
#   include "MDialogs.h"
#   include "Profile.h"
#   include "MApplication.h"
#   include "MailFolder.h"
#   include "Profile.h"
#   include "MModule.h"
#   include "MHelp.h"
#endif

#include "Mpers.h"

#include "modules/Scoring.h"
#include "XFace.h"

#include "MHelp.h"

#include "gui/wxMApp.h"
#include "gui/wxMIds.h"

#include "MFolder.h"
#include "gui/wxlwindow.h"

#include "gui/wxIconManager.h"

#include <wx/dynarray.h>
#include <wx/window.h>
#include <wx/radiobox.h>
#include <wx/confbase.h>
#include <wx/persctrl.h>
#include <wx/gauge.h>
#include <wx/stattext.h>
#include <wx/statbmp.h>
#include <wx/choice.h>
#include <wx/textdlg.h>
#include <wx/treectrl.h>
#include <wx/utils.h>
#include <wx/help.h>
#include <wx/fs_mem.h> // memory filesystem for startup screen
#include "MFolderDialogs.h"

#include "adb/AdbEntry.h"
#include "adb/AdbBook.h"
#include "adb/AdbManager.h"

#include "gui/wxFolderTree.h"
#include "gui/wxFolderView.h"
#include "gui/wxDialogLayout.h"

#include "sysutil.h"    // for sysutil_compare_filenames

#include <errno.h>


#ifdef    OS_WIN
#   define Mahogany      "Mahogany"
#   define PythonPowered "PythonPowered"
#else   //real XPMs
#   include "../src/icons/Msplash.xpm"
#   ifdef USE_PYTHON
#     include "../src/icons/PythonPowered.xpm"
#   endif
#endif  //Win/Unix

// ----------------------------------------------------------------------------
// global vars and functions
// ----------------------------------------------------------------------------

/// wxAboutFrame: g_pSplashScreen and CloseSplash() further down

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

#ifdef USE_SEMIMODAL

int
wxSMDialog::ShowModal()
{
   m_modalShowing = TRUE;

#if wxUSE_HELP && wxUSE_HTML
   /* Disable all other windows apart from the help frame and this
      one. */

   wxFrame *hf = NULL;
   wxHelpController *hc = ((wxMApp *)mApplication)->GetHelpController();
   if(hc && hc->IsKindOf(CLASSINFO(wxHelpControllerHtml)))
      hf = ((wxHelpControllerHtml *)hc)->GetFrameParameters();
#else
   wxWindow *hf = NULL;
#endif

   wxWindowList::Node *node;
   for ( node = wxTopLevelWindows.GetFirst(); node; node = node->GetNext() )
   {
      if(node->GetData() != hf && node->GetData() != this)
         node->GetData()->Enable(FALSE);
   }

   Show( TRUE );

   while(IsModal())
      wxTheApp->Dispatch();

   wxEnableTopLevelWindows(TRUE);
   return GetReturnCode();
}

void wxSMDialog::EndModal( int retCode )
{
    SetReturnCode( retCode );

    if (!IsModal())
    {
        wxFAIL_MSG( _T("wxSMDialog:EndModal called twice") );
        return;
    }
    m_modalShowing = FALSE;
    Show( FALSE );
}

#endif // USE_SEMIMODAL

// better looking and wxConfig-aware wxTextEntryDialog
class MTextInputDialog : public wxDialog
{
public:
   MTextInputDialog(wxWindow *parent,
                    const wxString& strText,
                    const wxString& strCaption,
                    const wxString& strPrompt,
                    const wxString& strConfigPath,
                    const wxString& strDefault,
                    bool passwordflag);

   // accessors
   const wxString& GetText() const { return m_strText; }

   // base class virtuals implemented
   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

   // if using a textctrl and not a combobox, this will process the
   // ENTER key
   void OnEnter(WXUNUSED(wxEvent &))
      { TransferDataFromWindow(); EndModal(wxID_OK); }
private:
   wxString      m_strText;
   wxPTextEntry *m_text;
   wxTextCtrl   *m_passwd; // used if we ask for a password, NULL otherwise
   DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(MTextInputDialog, wxDialog)
    EVT_TEXT_ENTER(-1, MTextInputDialog::OnEnter)
END_EVENT_TABLE()

// a dialog showing all folders
class MFolderDialog : public wxSMDialog
{
public:
   MFolderDialog(wxWindow *parent, const wxPoint& pos, const wxSize& size);
   ~MFolderDialog() { delete m_tree; }

   // accessors
   MFolder *GetFolder() const { return m_folder; }

   // base class virtuals implemented
   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

   void OnButton(wxCommandEvent &ev);

private:
   wxString     m_FileName;
   MFolder      *m_folder;
   wxFolderTree *m_tree;

   DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(MFolderDialog, wxSMDialog)
    EVT_BUTTON(-1, MFolderDialog::OnButton)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// MTextDialog - a dialog containing a multi-line text control (used to show
//               user some text)
// ----------------------------------------------------------------------------

class MTextDialog : public wxDialog
{
public:
    MTextDialog(wxWindow *parent,
                const wxString& title,
                const wxString& text,
                const wxPoint& position,
                const wxSize& size)
    : wxDialog(parent, -1, title, position, size,
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER
               | wxSYSTEM_MENU| wxMINIMIZE_BOX
               | wxMAXIMIZE_BOX | wxTHICK_FRAME) // make it resizealbe
    {
        m_text = new wxTextCtrl(this, -1, text, wxPoint(0, 0), size,
                                wxTE_MULTILINE | wxTE_READONLY);
    }

    void OnSize(wxSizeEvent& event)
    {
       m_text->SetSize(-1, -1, event.GetSize().GetX(), event.GetSize().GetY());
    }

private:
    wxTextCtrl *m_text;

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(MTextDialog, wxDialog)
    EVT_SIZE(MTextDialog::OnSize)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// NoBusyCursor ensures that the cursor is not "busy" (hourglass) while this
// object is in scope and is supposed to be used when creating a modal dialog
// (see MDialog_ErrorMessage &c for the examples)
// ----------------------------------------------------------------------------

class NoBusyCursor
{
public:
   NoBusyCursor()
   {
      m_countBusy = 0;
      while ( wxIsBusy() )
      {
         m_countBusy++;

         wxEndBusyCursor();
      }
   }

   ~NoBusyCursor()
   {
      while ( m_countBusy-- > 0 )
      {
         wxBeginBusyCursor();
      }
   }

private:
   int m_countBusy;
};

// ----------------------------------------------------------------------------
// functions
// ----------------------------------------------------------------------------

/// returns the argument if it's !NULL of the top-level application frame
static inline wxWindow *GetDialogParent(const wxWindow *parent)
{
  return parent == NULL ? mApplication->TopLevelFrame()
                        : GetFrame(parent);
}

// under Windows we don't use wxCENTRE style which uses the generic message box
// instead of the native one (and thus it doesn't have icons, for example)
static inline long Style(long style)
{
# ifdef OS_WIN
    return style;
# else //OS_WIN
    return style | wxCENTRE;
# endif
}

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// simple wrappers around wxWin functions
// ----------------------------------------------------------------------------

long
MGetNumberFromUser(const wxString& message,
                   const wxString& prompt,
                   const wxString& caption,
                   long value,
                   long min,
                   long max,
                   wxWindow *parent,
                   const wxPoint& pos)
{
   NoBusyCursor no;

   return wxGetNumberFromUser(message, prompt, caption,
                              value, min, max,
                              parent, pos);
}

// ----------------------------------------------------------------------------
// MTextInputDialog dialog and MInputBox (which uses it)
// ----------------------------------------------------------------------------

MTextInputDialog::MTextInputDialog(wxWindow *parent,
                                   const wxString& strText,
                                   const wxString& strCaption,
                                   const wxString& strPrompt,
                                   const wxString& strConfigPath,
                                   const wxString& strDefault,
                                   bool passwordflag)
   : wxDialog(parent, -1, wxString("Mahogany : ") + strCaption,
              wxDefaultPosition,
              wxDefaultSize,
              wxDEFAULT_DIALOG_STYLE | wxDIALOG_MODAL)
{
  // text is the default value normally read from config and it may be
  // overriden by strDefault parameter if it is not empty
  if ( !strDefault )
     m_strText = strText;
  else
     m_strText = strDefault;

  // layout
  long widthLabel, heightLabel;
  wxClientDC dc(this);
  dc.SetFont(wxSystemSettings::GetSystemFont(wxSYS_DEFAULT_GUI_FONT));
  dc.GetTextExtent(strPrompt, &widthLabel, &heightLabel);

  long heightBtn = TEXT_HEIGHT_FROM_LABEL(heightLabel),
       widthBtn = BUTTON_WIDTH_FROM_HEIGHT(heightBtn);
  long widthText = 3*widthBtn,
       heightText = TEXT_HEIGHT_FROM_LABEL(heightLabel);
  if ( widthText < widthLabel ) {
     // too short text zone looks ugly
     widthText = widthLabel;
  }

  long widthDlg = widthLabel + widthText + 3*LAYOUT_X_MARGIN,
       heightDlg = heightText + heightBtn + 3*LAYOUT_Y_MARGIN;

  long x = LAYOUT_X_MARGIN,
       y = LAYOUT_Y_MARGIN,
       dy = (heightText - heightLabel) / 2;

  // label and the text
  (void)new wxStaticText(this, -1, strPrompt, wxPoint(x, y + dy),
                         wxSize(widthLabel, heightLabel));
  if(passwordflag)
  {
     m_passwd = new wxTextCtrl(this, -1, "",
                               wxPoint(x + widthLabel + LAYOUT_X_MARGIN, y),
                               wxSize(widthText, heightText),
                               wxTE_PASSWORD|wxTE_PROCESS_ENTER);
     m_passwd->SetFocus();
  }
  else
  {
     m_text = new wxPTextEntry(strConfigPath, this, -1, "",
                               wxPoint(x + widthLabel + LAYOUT_X_MARGIN, y),
                               wxSize(widthText, heightText));
     m_text->SetFocus();
     m_passwd = NULL; // signal that it's not used
  }
  // buttons
  wxButton *btnOk = new
    wxButton(this, wxID_OK, _("OK"),
             wxPoint(widthDlg - 2*LAYOUT_X_MARGIN - 2*widthBtn,
                     heightDlg - LAYOUT_Y_MARGIN - heightBtn),
             wxSize(widthBtn, heightBtn));
  (void)new wxButton(this, wxID_CANCEL, _("Cancel"),
                     wxPoint(widthDlg - LAYOUT_X_MARGIN - widthBtn,
                             heightDlg - LAYOUT_Y_MARGIN - heightBtn),
                     wxSize(widthBtn, heightBtn));
  btnOk->SetDefault();

  // set position and size
  SetClientSize(widthDlg, heightDlg);
  Centre(wxCENTER_FRAME | wxBOTH);
}

bool MTextInputDialog::TransferDataToWindow()
{
   if(m_passwd == NULL)
   {
      m_text->SetValue(m_strText);
      // select everything so that it's enough to type a single letter to erase
      // the old value (this way it's as unobtrusive as you may get)
      m_text->SetSelection(-1, -1);
   }
   return TRUE;
}

bool MTextInputDialog::TransferDataFromWindow()
{
   wxString strText;
   if(m_passwd)
      strText = m_passwd->GetValue();
   else
      strText = m_text->GetValue();
  if ( strText.IsEmpty() )
  {
    // imitate [Cancel] button
    EndModal(wxID_CANCEL);
    return FALSE;
  }
  else
  {
    m_strText = strText;
    return TRUE;
  }
}

// a wxConfig-aware function which asks user for a string
bool MInputBox(wxString *pstr,
               const wxString& strCaption,
               const wxString& strPrompt,
               const wxWindow *parent,
               const char *szKey,
               const char *def,
               bool passwordflag)
{
  wxString strConfigPath;
  strConfigPath << "/Prompts/" << szKey;

  MTextInputDialog dlg(GetDialogParent(parent), *pstr,
                       strCaption, strPrompt, strConfigPath, def, passwordflag);

  // do not allow attempts to store the password:
  wxASSERT((!passwordflag)||(szKey==NULL && def == NULL));
  if ( dlg.ShowModal() == wxID_OK )
  {
    *pstr = dlg.GetText();

    return TRUE;
  }
  else
  {
    return FALSE;
  }
}

// ----------------------------------------------------------------------------
// other functions
// ----------------------------------------------------------------------------
void
MDialog_ErrorMessage(const char *msg,
                     const MWindow *parent,
                     const char *title,
                     bool /* modal */)
{
   //MGuiLocker lock;
   CloseSplash();
   NoBusyCursor no;
   wxMessageBox(msg, wxString("Mahogany : ") + title,
                Style(wxOK|wxICON_EXCLAMATION),
                 GetDialogParent(parent));
}


/** display system error message:
    @param message the text to display
    @param parent the parent frame
    @param title  title for message box window
    @param modal  true to make messagebox modal
   */
void
MDialog_SystemErrorMessage(const char *message,
               const MWindow *parent,
               const char *title,
               bool modal)
{
   String
      msg;

   msg = String(message) + String(_("\nSystem error: "))
      + String(strerror(errno));

   MDialog_ErrorMessage(msg.c_str(), parent, wxString("Mahogany : ")+title, modal);
}


/** display error message and exit application
       @param message the text to display
       @param title  title for message box window
       @param parent the parent frame
   */
void
MDialog_FatalErrorMessage(const char *message,
              const MWindow *parent,
              const char *title)
{
   String msg = String(message) + _("\nExiting application...");

   MDialog_ErrorMessage(message,parent, wxString("Mahogany : ")+title,true);
   mApplication->Exit();
}


/** display normal message:
       @param message the text to display
       @param parent the parent frame
       @param title  title for message box window
       @param modal  true to make messagebox modal
   */
void
MDialog_Message(const char *message,
                const MWindow *parent,
                const char *title,
                const char *configPath)
{
   wxString caption = "Mahogany : ";
   caption += title;

   //MGuiLocker lock;
   CloseSplash();
   NoBusyCursor no;
   if(configPath)
      wxPMessageBox(configPath, message, caption,
                    Style(wxOK | wxICON_INFORMATION),
                   GetDialogParent(parent));
   else
      wxMessageBox(message, caption,
                   Style(wxOK | wxICON_INFORMATION),
                   GetDialogParent(parent));
}


/** simple Yes/No dialog
       @param message the text to display
       @param parent the parent frame
       @param title  title for message box window
       @param modal  true to make messagebox modal
       @param YesDefault true if Yes button is default, false for No as default
       @return true if Yes was selected
   */
bool
MDialog_YesNoDialog(const char *message,
                    const MWindow *parent,
                    const char *title,
                    bool yesDefault,
                    const char *configPath)
{
   wxString caption = "Mahogany : ";
   caption += title;

   int style = Style(wxYES_NO | wxICON_QUESTION | wxCENTRE);
   if(! yesDefault) style |= wxNO_DEFAULT;

   //MGuiLocker lock;
   CloseSplash();
   NoBusyCursor no;
   return wxPMessageBox(configPath, message, caption,
                        style,
                        GetDialogParent(parent)) == wxYES;
}


/** Filerequester
       @param message the text to display
       @param parent the parent frame
       @param path   default path
       @param filename  default filename
       @param extension default extension
       @param wildcard  pattern matching expression
       @param save   if true, for saving a file
       @return pointer to a temporarily allocated buffer with he filename, or NULL
   */
const char *
MDialog_FileRequester(String const & message,
                      const MWindow *parent,
                      String path,
                      String filename,
                      String extension,
                      String wildcard,
                      bool save,
                      ProfileBase * /* profile */)
{
   //MGuiLocker lock;
   CloseSplash();


   if(parent == NULL)
      parent = mApplication->TopLevelFrame();

   // TODO we save only one file name for all "open file" dialogs and one for
   //      all "save file" dialogs - may be should be more specific (add
   //      configPath parameter to MDialog_FileRequester?)
   return wxPFileSelector(save ? "save" : "load",
                          message, path, filename, extension,
                          wildcard, 0, (wxWindow *)parent);
}

int
MDialog_AdbLookupList(ArrayAdbElements& aEntries,
                      const MWindow *parent)
{
   //MGuiLocker lock;
   CloseSplash();

   wxArrayString aChoices;
   wxString strName, strEMail;

   size_t nEntryCount = aEntries.Count();
   for( size_t nEntry = 0; nEntry < nEntryCount; nEntry++ )
   {
      aChoices.Add(aEntries[nEntry]->GetDescription());
   }

   int
      w = 400,
      h = 400;

   parent = GetDialogParent(parent);
   if(parent)
   {
      parent->GetClientSize(&w,&h);
      w = (w * 8) / 10;
      h = (h * 8) / 10;
   }

   if ( nEntryCount == 0 ) {
     // no matches at all
     return -1;
   }
   else if ( nEntryCount == 1 ) {
     // don't ask user to choose among one entry and itself!
     return 0;
   }
   else {
      return wxGetSingleChoiceIndex
             (
               _("Please choose an entry:"),
               wxString("Mahogany : ")+_("Expansion options"),
               nEntryCount,
               &aChoices[0],
               (wxWindow *)parent,
               -1, -1, // x,y
               TRUE,   //centre
               w, h
             );
   }
}

// simple AboutDialog to be displayed at startup

// timer which calls our DoClose() when it expires
class LogCloseTimer : public wxTimer
{
public:
   LogCloseTimer(class wxAboutWindow *window)
      {
         m_window = window;
         Start(READ_APPCONFIG(MP_SPLASHDELAY)*1000);
      }

   virtual void Notify();

private:
   wxAboutWindow *m_window;
};


// getting log messages when splash screen is shown is extremely annoying,
// because there is no (easy) way to close the msg box hidden by the splash
// screen, so install a temporary log redirector which will close the splash
// screen before showing any messages
class SplashKillerLog : public wxLog
{
public:
   SplashKillerLog() { m_logOld = wxLog::GetActiveTarget(); }
   virtual ~SplashKillerLog() { wxLog::SetActiveTarget(m_logOld); }

   virtual void DoLog(wxLogLevel level, const wxChar *szString, time_t t)
      {
         // all previous ones will show a msg box
         if ( level < wxLOG_Status )
         {
            CloseSplash();
         }

         if ( m_logOld )
         {
            // the cast is bogus, just to be able to call protected DoLog()
            ((SplashKillerLog *)m_logOld)->DoLog(level, szString, t);
         }
      }

private:
   wxLog *m_logOld;
};



#include "wx/html/htmlwin.h"

// the main difference is that it goes away as soon as you click it
// or after some time (if not disabled in the ctor).
//
// It is also unique and must be removed before showing any message boxes
// (because it has on top attribute) with CloseSplash() function.
class wxAboutWindow : public wxWindow
{
public:
  // fills the window with some pretty text
  wxAboutWindow(wxFrame *parent, bool bCloseOnTimeout = true);

  /// stop the timer
  void StopTimer(void)
  {
     if(m_pTimer)
     {
        delete m_pTimer;
        m_pTimer = NULL;
     }
  }

  // close the about frame
  void DoClose()
  {
     if(GetParent()) GetParent()->Close(true);
     StopTimer();
  }

private:
  LogCloseTimer  *m_pTimer;
};

void
LogCloseTimer::Notify()
{
   m_window->DoClose();
}

class wxAboutFrame : public wxFrame
{
public:
   wxAboutFrame(bool bCloseOnTimeout);
   virtual ~wxAboutFrame()
   {
      // remove our temp log redirector
      delete wxLog::GetActiveTarget();

      g_pSplashScreen = NULL;
   }

   void Close(void) { m_Window->StopTimer(); wxWindow::Close(); }

private:
   wxAboutWindow *m_Window;
};

class MyHtmlWindow : public wxHtmlWindow
{
public:
   MyHtmlWindow(wxAboutWindow *aw, wxWindow *parent)
      : wxHtmlWindow(parent, -1)
      {
         m_window = aw;
      }
   // mouse event handler closes the parent window
   void OnClick(wxMouseEvent&) { m_window->DoClose(); }
   void OnChar(wxKeyEvent& ev)
      {
         switch(ev.KeyCode())
         {
         case WXK_UP:
         case WXK_DOWN:
         case WXK_PRIOR:
         case WXK_NEXT:
            ev.Skip();
            break;
         default:
            m_window->DoClose();
         }
      }
private:
   wxAboutWindow *m_window;
   DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(MyHtmlWindow, wxHtmlWindow)
  EVT_LEFT_DOWN(MyHtmlWindow::OnClick)
  EVT_MIDDLE_DOWN(MyHtmlWindow::OnClick)
  EVT_RIGHT_DOWN(MyHtmlWindow::OnClick)
  EVT_CHAR(MyHtmlWindow::OnChar)
END_EVENT_TABLE()

wxAboutWindow::wxAboutWindow(wxFrame *parent, bool bCloseOnTimeout)
             : wxWindow(parent, -1, wxDefaultPosition, parent->GetClientSize())
{


   wxSplitterWindow *sp = new wxSplitterWindow(this, -1,
                                               wxDefaultPosition,
                                               GetSize());
   wxHtmlWindow *top = new MyHtmlWindow(this, sp);
   wxHtmlWindow *bottom = new MyHtmlWindow(this,sp);
   sp->SplitHorizontally(top,bottom,240);

   wxMemoryFSHandler::AddFile("splash.png", wxBITMAP(Mahogany),
                              wxBITMAP_TYPE_PNG);
#ifdef USE_PYTHON
   wxMemoryFSHandler::AddFile("pythonpowered.png", wxBITMAP(PythonPowered),
                              wxBITMAP_TYPE_PNG);
#endif

   top->SetPage("<body text=#000000 bgcolor=#ffffff>"
                "<center><img src=\"memory:splash.png\"><br>"
                "</center>");

   bottom->SetPage("<body text=#000000 bgcolor=#ffffff>"
#ifdef DEBUG
                   "<h3>Debug information:</h3>"
                   "<h4>Features:</h4>"
#ifdef USE_THREADS
                   "Threads "
#endif
#ifdef USE_PYTHON
                   "Python "
#endif
#ifdef USE_SSL
                   "SSL "
#   if defined(NO_IDEA) || defined (NO_DSA) || defined (NO_RSA)
                   "("
#      ifdef NO_IDEA
                   "no-IDEA "
#      endif
#      ifdef NO_RSA
                   "no-RSA "
#      endif
#      ifdef _NO_DSA
                   "no-DSA"
#      endif
                   ")"
#   endif
#endif
#ifdef EXPERIMENTAL
                   "Experimental Code "
#endif

                   "<p>"
#endif
                   "<b>List of contributors:</b><p>"
                   "Karsten Ball&uuml;der (ballueder@gmx.net)<br>"
                   "Vadim Zeitlin (vadim@wxwindows.org)<br>"
                   "Vaclav Slavik (vaclav@wxwindows.org)<br>"
                   "Daniel Seifert (dseifert@gmx.de)<br>"
                   "<i>The wxWindows team</i><br><hr>"
                   "<p>This Product includes software developed and copyright "
                   "by the University of Washington.<br>"
#ifdef USE_SSL
                   "<p>"
                   "This product includes software developed by the OpenSSL Project "
                   "for use in the OpenSSL Toolkit. (http://www.openssl.org/).<br>"
                   "This product includes cryptographic software written by Eric Young (eay@cryptsoft.com)<br>"
                   "This product includes software written by Tim Hudson (tjh@cryptsoft.com)<br>"
#endif
                   "<p>"
                   "The Mahogany Team would like to acknowledge the support of "
                   "Anthemion Software, Heriot-Watt University, Sourceforge.net, SourceGear.com, "
                   "GDev.net, Simon Shapiro, VA Linux and SuSE GmbH."
      );


   wxMemoryFSHandler::RemoveFile("splash.png");
#ifdef USE_PYTHON
   wxMemoryFSHandler::RemoveFile("pythonpowered.png");
#endif

   bottom->SetFocus();
   // start a timer which will close us (if not disabled)
   if ( bCloseOnTimeout ) {
     m_pTimer = new LogCloseTimer(this);
   }
   else {
     // must initialize to NULL because we delete it later unconditionally
     m_pTimer = NULL;
   }
}

wxAboutFrame::wxAboutFrame(bool bCloseOnTimeout)
            : wxFrame(NULL, -1, _("Welcome"),
                      wxDefaultPosition,
                      // this is ugly, but having scrollbars is even uglier
#ifdef __WXMSW__
                      wxSize(400, 400),
#else  // !MSW
                      wxSize(400, 400),
#endif // MSW/!MSW
                      /* no border styles at all */ wxSTAY_ON_TOP )
{
   wxCHECK_RET( g_pSplashScreen == NULL, "one splash is more than enough" );

   wxLog::SetActiveTarget(new SplashKillerLog);

   m_Window = new wxAboutWindow(this, bCloseOnTimeout);
   g_pSplashScreen = (wxMFrame *)this;
   Centre(wxCENTER_FRAME | wxBOTH);
   Show(TRUE);
}





class wxMFrame *g_pSplashScreen = NULL;

extern void CloseSplash()
{
   if ( g_pSplashScreen )
     ((wxAboutFrame *)g_pSplashScreen)->Close();
}



void
MDialog_AboutDialog( const MWindow * /* parent */, bool bCloseOnTimeout)
{
   if(g_pSplashScreen == NULL)
      (void)new wxAboutFrame(bCloseOnTimeout);
   wxYield();
}


void
MDialog_FolderProfile(const MWindow *parent, const String& folderName)
{
   MFolder_obj folder(MFolder::Get(folderName));

   if ( folder )
   {
      ShowFolderPropertiesDialog(folder, (wxWindow *)parent);
   }
}

void
MDialog_FolderOpen(const MWindow *parent)
{
   MFolder *folder = MDialog_FolderChoose(parent);
   if ( folder != NULL )
   {
      // open a view on this folder
      (void)wxFolderViewFrame::Create(folder->GetName(),
                                      mApplication->TopLevelFrame());
      folder->DecRef();
   }
   //else: cancelled
}

// ----------------------------------------------------------------------------
// folder dialog stuff
// ----------------------------------------------------------------------------

MFolderDialog::MFolderDialog(wxWindow *parent,
                             const wxPoint& pos,
                             const wxSize& size)
             : wxSMDialog(parent, -1, _("Choose folder"),
                        pos, size,
                        wxDEFAULT_DIALOG_STYLE |
                        wxDIALOG_MODAL |
                        wxRESIZE_BORDER)
{
   SetAutoLayout(TRUE);
   wxLayoutConstraints *c;

   // create 2 buttons
   // ----------------

   // we want to have the buttons of standard size
   long heightChar = AdjustCharHeight(GetCharHeight());
   long heightBtn = TEXT_HEIGHT_FROM_LABEL(heightChar),
        widthBtn = BUTTON_WIDTH_FROM_HEIGHT(heightBtn);

   // Cancel button
   wxButton *btnCancel = new wxButton(this, wxID_CANCEL, _("Cancel"));

   c = new wxLayoutConstraints;
   c->height.Absolute(heightBtn);
   c->width.Absolute(widthBtn);
   c->right.SameAs(this, wxRight, LAYOUT_X_MARGIN);
   c->bottom.SameAs(this, wxBottom, 2*LAYOUT_Y_MARGIN);
   btnCancel->SetConstraints(c);

   // Ok button
   wxButton *btnOk = new wxButton(this, wxID_OK, _("OK"));

   btnOk->SetDefault();

   c = new wxLayoutConstraints;
   c->height.Absolute(heightBtn);
   c->width.Absolute(widthBtn);
   c->right.SameAs(btnCancel, wxLeft, 2*LAYOUT_X_MARGIN);
   c->bottom.SameAs(this, wxBottom, 2*LAYOUT_Y_MARGIN);

   btnOk->SetConstraints(c);

   // File button
   wxButton *btnFile = new wxButton(this, wxID_OPEN, _("File..."));
   c = new wxLayoutConstraints;
   c->height.Absolute(heightBtn);
   c->width.Absolute(widthBtn);
   c->right.SameAs(btnOk, wxLeft, 2*LAYOUT_X_MARGIN);
   c->bottom.SameAs(this, wxBottom, 2*LAYOUT_Y_MARGIN);
   btnFile->SetConstraints(c);

   // Help button
   wxButton *btnHelp = new wxButton(this, wxID_HELP, _("Help"));
   c = new wxLayoutConstraints;
   c->height.Absolute(heightBtn);
   c->width.Absolute(widthBtn);
   c->left.SameAs(this, wxLeft, 2*LAYOUT_X_MARGIN);
   c->bottom.SameAs(this, wxBottom, 2*LAYOUT_Y_MARGIN);
   btnHelp->SetConstraints(c);

   // create the folder tree control
   // ------------------------------

   m_tree = new wxFolderTree(this);

   c = new wxLayoutConstraints;
   c->top.SameAs(this, wxTop, LAYOUT_Y_MARGIN);
   c->left.SameAs(this, wxLeft, LAYOUT_X_MARGIN);
   c->right.SameAs(this, wxRight, LAYOUT_X_MARGIN);
   c->bottom.SameAs(btnOk, wxTop, 2*LAYOUT_Y_MARGIN);
   m_tree->GetWindow()->SetConstraints(c);

   // position the dialog
   if ( size.GetX() < 3*widthBtn )
   {
      SetClientSize(3*widthBtn, size.GetY());
   }

   Centre(wxCENTER_FRAME | wxBOTH);
}

void
MFolderDialog::OnButton(wxCommandEvent &ev)
{
   switch(ev.GetId())
   {
      case wxID_OPEN:
         m_FileName = wxPFileSelector("FolderDialogFile",
                                      _("Mahogany: Please choose a folder file"),
                                      NULL, NULL, NULL, NULL,
                                      0, this);
         if ( !!m_FileName )
         {
            // folder (file) chosen
            if ( TransferDataFromWindow() )
            {
               EndModal(wxID_OK);
            }
         }
         break;

      case wxID_HELP:
         mApplication->Help(MH_DIALOG_FOLDERDLG);
         break;

      default:
         ev.Skip();
   }
}

bool MFolderDialog::TransferDataToWindow()
{
   // TODO restore last folder from config
   return true;
}

bool MFolderDialog::TransferDataFromWindow()
{
   if ( m_FileName.Length() == 0 )
   {
      m_folder = m_tree->GetSelection();
      if ( m_folder != NULL )
      {
         // TODO save the folder name to config
      }
   }
   else
   {
      // the name of the folder can't contain '/' and such, so take just the
      // name, not the full name as the folder name
      wxString name;
      wxSplitPath(m_FileName, NULL, &name, NULL);

      // verify that the folder with this name doesn't already exist
      m_folder = NULL;
      MFolder *folder;
      while ( (folder = MFolder::Get(name)) != NULL )
      {
         // it does exist - may be it's already the same file?
         if ( folder->GetType() == MF_FILE )
         {
            Profile_obj profile(name);
            wxString filename = READ_CONFIG(profile, MP_FOLDER_PATH);
            if ( sysutil_compare_filenames(filename, m_FileName) )
            {
               m_folder = folder;

               break;
            }
         }

         folder->DecRef();

         if (
              !MInputBox
               (
                &name,
                _("Mahogany: folder selection"),
                wxString::Format(
                _("Sorry, the folder '%s' already exists "
                  "and corresponds to another file, please "
                  "choose a different name for the folder "
                  "which will correspond to the file '%s'."),
                name.c_str(), m_FileName.c_str()
                ),
                this,
                "FileFolderName"
               )
            )
         {
            // can't continue
            return FALSE;
         }
      }

      if ( !m_folder )
      {
         m_folder = MFolder::CreateTemp(name, MF_FILE, 0, m_FileName,
                                        "", "", ""); // no server/login/pwd
      }
      //else: it already existed before with the same filename
   }

   return true;
}

MFolder *
MDialog_FolderChoose(const MWindow *parent)
{
   // the config path where we store the position of the dialog
   static const char *folderDialogPos = "FolderSelDlg";

   int x, y, w, h;
   wxMFrame::RestorePosition(folderDialogPos, &x, &y, &w, &h);

   MFolderDialog dlg((wxWindow *)parent, wxPoint(x, y), wxSize(w, h));

   bool selected = dlg.ShowModal() == wxID_OK;

   wxMFrame::SavePosition(folderDialogPos, &dlg);

   return selected ? dlg.GetFolder() : NULL;
}


void MDialog_ShowText(MWindow *parent,
                      const char *title,
                      const char *text,
                      const char *configPath)
{
   int x = -1, y = -1, w = -1, h = -1;
   if ( configPath )
      wxMFrame::RestorePosition(configPath, &x, &y, &w, &h);

   MTextDialog dlg(parent, title, text,
                   wxPoint(x, y), wxSize(w, h));
   (void)dlg.ShowModal();

   if ( configPath )
      wxMFrame::SavePosition(configPath, &dlg);
}



#include "gui/wxDialogLayout.h"

#define NUM_SORTLEVELS 5

/* These must not be more than 16, as they are stored in a 4-bit
   value! They must be in sync with the enum in MailFolderCmn.h.
*/
static wxString sortCriteria[] =
{
   gettext_noop("None"),
   gettext_noop("Date"),
   gettext_noop("Date (reverse)"),
   gettext_noop("Subject"),
   gettext_noop("Subject (reverse)"),
   gettext_noop("Author"),
   gettext_noop("Author (reverse)"),
   gettext_noop("Status"),
   gettext_noop("Status (reverse)"),
   gettext_noop("Score"),
   gettext_noop("Score (reverse)"),
};

// defining it like this makes it much more difficult to forget to update it
static const size_t NUM_CRITERIA  = WXSIZEOF(sortCriteria);

#define NUM_LABELS 2
static wxString labels[NUM_LABELS] =
{
   gettext_noop("First, sort by"),
   gettext_noop("then, sort by")
};

class wxMessageSortingDialog : public wxOptionsPageSubdialog
{
public:
   wxMessageSortingDialog(ProfileBase *profile, wxWindow *parent);

   // reset the selected options to their default values
   virtual bool TransferDataFromWindow();
   virtual bool TransferDataToWindow();
   bool WasChanged(void) { return m_SortOrder != m_OldSortOrder;};

protected:
   wxChoice    *m_Choices[NUM_CRITERIA];
   wxCheckBox  *m_UseThreading;
   wxCheckBox  *m_ReSortOnChange;
   long         m_OldSortOrder;
   long         m_SortOrder;
};

wxMessageSortingDialog::wxMessageSortingDialog(ProfileBase *profile,
                                               wxWindow *parent)
                      : wxOptionsPageSubdialog(profile,parent,
                                               _("Message sorting"),
                                               "MessageSortingDialog")
{
   wxStaticBox *box = CreateStdButtonsAndBox(_("Sort messages by"), FALSE,
                                             MH_DIALOG_SORTING);

   wxClientDC dc(this);
   dc.SetFont(wxSystemSettings::GetSystemFont(wxSYS_DEFAULT_GUI_FONT));
   long width, widthMax = 0;

   // see the comment near NUM_CRITERIA definition
   ASSERT_MSG( NUM_LABELS < 16, "too many search criteria" );

   size_t n;
   for ( n = 0; n < NUM_LABELS; n++ )
   {
      dc.GetTextExtent(labels[n], &width, NULL);
      if ( width > widthMax ) widthMax = width;
   }

   wxLayoutConstraints *c;
   for( n = 0; n < NUM_SORTLEVELS; n++)
   {
      wxStaticText *txt = new wxStaticText(this, -1,
                                           n < NUM_LABELS
                                           ? _(labels[n])
                                           : _(labels[NUM_LABELS-1]));
      c = new wxLayoutConstraints;
      c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
      c->width.Absolute(widthMax);
      if(n == 0)
         c->top.SameAs(box, wxTop, 4*LAYOUT_Y_MARGIN);
      else
         c->top.Below(m_Choices[n-1], 2*LAYOUT_Y_MARGIN);
      c->height.AsIs();
      txt->SetConstraints(c);

      m_Choices[n] = new wxChoice(this, -1, wxDefaultPosition,
                                  wxDefaultSize, NUM_CRITERIA,
                                  sortCriteria);
      c = new wxLayoutConstraints;
      c->left.RightOf(txt, 2*LAYOUT_X_MARGIN);
      c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
      if(n == 0)
         c->top.SameAs(box, wxTop, 4*LAYOUT_Y_MARGIN);
      else
         c->top.Below(m_Choices[n-1], 2*LAYOUT_Y_MARGIN);
      c->height.AsIs();
      m_Choices[n]->SetConstraints(c);
   }

   m_UseThreading = new wxCheckBox(this, -1, _("Thread messages"));
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->top.Below(m_Choices[n-1], 2*LAYOUT_Y_MARGIN);
   c->height.AsIs();
   m_UseThreading->SetConstraints(c);

   m_ReSortOnChange = new wxCheckBox(this, -1, _("Re-sort on status change"));
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->top.Below(m_UseThreading, 2*LAYOUT_Y_MARGIN);
   c->height.AsIs();
   m_ReSortOnChange->SetConstraints(c);

   Layout();
   SetDefaultSize(380,310);

   TransferDataToWindow();
   m_OldSortOrder = m_SortOrder;
}


bool wxMessageSortingDialog::TransferDataFromWindow()
{
   bool uses_scoring = false;
   int selection;
   m_SortOrder = 0;
   for( int n = NUM_SORTLEVELS-1; n >= 0; n--)
   {
      m_SortOrder <<= 4;
      selection = m_Choices[n]->GetSelection();
      if(selection == MSO_SCORE
         || selection == MSO_SCORE_REV)
         uses_scoring = true;
      m_SortOrder += selection;
   }

   GetProfile()->writeEntry(MP_MSGS_SORTBY, m_SortOrder);
   GetProfile()->writeEntry(MP_MSGS_USE_THREADING,
                         m_UseThreading->GetValue());
   GetProfile()->writeEntry(MP_MSGS_RESORT_ON_CHANGE,
                         m_ReSortOnChange->GetValue());

   if(uses_scoring)
   {
      MModule *module = MModule::GetProvider(MMODULE_INTERFACE_SCORING);
      if(module)
         module->DecRef();
      else
      {
         wxString msg;
         msg.Printf(_("You have selected message score as a sort criterium\n"
                      "but not loaded any scoring plugin module yet.\n"
                      "Scoring will only work if you load an extension\n"
                      "module providing the '%s' interface.\n"
                      "Otherwise all messages will have score 0."),
                    MMODULE_INTERFACE_SCORING);
         MDialog_ErrorMessage(msg,this);
      }
   }
   return TRUE;
}

bool wxMessageSortingDialog::TransferDataToWindow()
{
   long sortOrder = READ_CONFIG(GetProfile(), MP_MSGS_SORTBY);
   /* Sort order is stored as 4 bits per hierarchy:
      0xdcba --> 1. sort by "a", then by "b", ...
   */

   m_SortOrder = sortOrder;

   long num;
   for( int n = 0; n < NUM_SORTLEVELS; n++)
   {
      num = sortOrder & 0x00000F; // lowest four bits
      ASSERT(n < NUM_SORTLEVELS);
      m_Choices[n]->SetSelection(num);
      sortOrder >>= 4;
   }
   m_UseThreading->SetValue(
      READ_CONFIG(GetProfile(), MP_MSGS_USE_THREADING) != 0);
   m_ReSortOnChange->SetValue(
      READ_CONFIG(GetProfile(), MP_MSGS_RESORT_ON_CHANGE) != 0);
   return TRUE;
}

/* Configuration dialog for sorting messages. */
extern
bool ConfigureSorting(ProfileBase *profile, wxWindow *parent)
{
   wxMessageSortingDialog dlg(profile, parent);
   if ( dlg.ShowModal() == wxID_OK && dlg.WasChanged() )
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}


//-----------------------------------------------------------------------------

static wxString searchCriteria[] =
{
   gettext_noop("Full Message"),
   gettext_noop("Message Content"),
   gettext_noop("Message Header"),
   gettext_noop("Subject Field"),
   gettext_noop("To Field"),
   gettext_noop("From Field"),
   gettext_noop("CC Field"),
};

#define SEARCH_CRIT_INVERT_FLAG  0x1000
#define SEARCH_CRIT_MASK  0x0FFF

// defining it like this makes it much more difficult to forget to update it
static const size_t NUM_SEARCHCRITERIA  = WXSIZEOF(searchCriteria);

class wxMessageSearchDialog : public wxOptionsPageSubdialog
{
public:
   wxMessageSearchDialog(SearchCriterium *crit,
                         ProfileBase *profile, wxWindow *parent);

   // reset the selected options to their default values
   virtual bool TransferDataFromWindow();
   virtual bool TransferDataToWindow();
   bool WasChanged(void) { return m_Criterium != m_OldCriterium;};

protected:

   void UpdateCritStruct(void)
      {
         m_CritStruct->m_What = (SearchCriterium::Type)
            (m_Choices->GetSelection()&SEARCH_CRIT_MASK);
         m_CritStruct->m_Invert = m_Invert->GetValue();
         m_CritStruct->m_Key = m_Keyword->GetValue();
      }
   SearchCriterium *m_CritStruct;
   wxChoice    *m_Choices;
   wxCheckBox  *m_Invert;
   wxTextCtrl  *m_Keyword;
   int         m_OldCriterium;
   int         m_Criterium;
};

wxMessageSearchDialog::wxMessageSearchDialog(SearchCriterium *crit,
                                             ProfileBase *profile,
                                             wxWindow *parent)
                      : wxOptionsPageSubdialog(profile,parent,
                                               _("Search folder for messages"),
                                               "MessageSearchDialog")
{
   ASSERT(crit);
   m_CritStruct = crit;
   wxStaticBox *box = CreateStdButtonsAndBox(_("Message search criteria"), FALSE,
                                               MH_DIALOG_SEARCHMSGS);

   wxClientDC dc(this);
   dc.SetFont(wxSystemSettings::GetSystemFont(wxSYS_DEFAULT_GUI_FONT));

   wxLayoutConstraints *c;

   wxStaticText *critlabel = new wxStaticText(this, -1,_("Search for text in"));
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.AsIs();
   c->top.SameAs(box, wxTop, 4*LAYOUT_Y_MARGIN);
   c->height.AsIs();
   critlabel->SetConstraints(c);

   m_Choices = new wxChoice(this, -1, wxDefaultPosition,
                            wxDefaultSize, NUM_SEARCHCRITERIA,
                            searchCriteria);
   c = new wxLayoutConstraints;
   c->left.RightOf(critlabel, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(box, wxTop, 4*LAYOUT_Y_MARGIN);
   c->height.AsIs();
   m_Choices->SetConstraints(c);

   wxStaticText *keylabel = new wxStaticText(this, -1,_("Search for"));
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.AsIs();
   c->top.Below(m_Choices, 2*LAYOUT_Y_MARGIN);
   c->height.AsIs();
   keylabel->SetConstraints(c);

   m_Keyword = new wxTextCtrl(this, -1);
   c = new wxLayoutConstraints;
   c->left.RightOf(keylabel, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(keylabel, wxTop, 0);
   c->height.AsIs();
   m_Keyword->SetConstraints(c);

   m_Invert = new wxCheckBox(this, -1, _("Invert selection"));
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->top.Below(keylabel, 2*LAYOUT_Y_MARGIN);
   c->height.AsIs();
   m_Invert->SetConstraints(c);

   Layout();
   SetDefaultSize(380,310);

   TransferDataToWindow();
   m_OldCriterium = m_Criterium;
}


bool wxMessageSearchDialog::TransferDataFromWindow()
{
   m_Criterium = m_Choices->GetSelection();
   if(m_Invert->GetValue() != 0)
      m_Criterium |= SEARCH_CRIT_INVERT_FLAG;

   GetProfile()->writeEntry(MP_MSGS_SEARCH_CRIT, m_Criterium);
   GetProfile()->writeEntry(MP_MSGS_SEARCH_ARG, m_Keyword->GetValue());

   UpdateCritStruct();
   return TRUE;
}

bool wxMessageSearchDialog::TransferDataToWindow()
{
   m_Criterium = READ_CONFIG(GetProfile(), MP_MSGS_SEARCH_CRIT);
   m_Choices->SetSelection(m_Criterium & SEARCH_CRIT_MASK);
   m_Invert->SetValue((m_Criterium & SEARCH_CRIT_INVERT_FLAG) != 0);
   m_Keyword->SetValue(READ_CONFIG(GetProfile(), MP_MSGS_SEARCH_ARG));
   UpdateCritStruct();
   return TRUE;
}

/* Configuration dialog for sorting messages. */
extern
bool ConfigureSearchMessages(class SearchCriterium *crit,
                             ProfileBase *profile, wxWindow *parent)
{
   wxMessageSearchDialog dlg(crit, profile, parent);
   if ( dlg.ShowModal() == wxID_OK && dlg.WasChanged() )
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}

//-----------------------------------------------------------------------------


static wxString DateFormatsLabels[] =
{
   gettext_noop("31 (Day)"),
   gettext_noop("Mon"),
   gettext_noop("Monday"),
   gettext_noop("12 (Month)"),
   gettext_noop("Nov"),
   gettext_noop("November"),
   gettext_noop("99 (year)"),
   gettext_noop("1999 (year)"),
   gettext_noop("24 (hour)"),
   gettext_noop("12 (hour)"),
   gettext_noop("am/pm"),
   gettext_noop("59 (minutes)"),
   gettext_noop("29 (seconds)"),
   gettext_noop("EST (timezone)"),
   gettext_noop("Default format including time"),
   gettext_noop("Default format without time")
};

static wxString DateFormats[] =
{
   "%d",
   "%a",
   "%A",
   "%m",
   "%b",
   "%B",
   "%y",
   "%Y",
   "%H",
   "%I",
   "%p",
   "%M",
   "%S",
   "%Z",
   "%c",
   "%x"
};

static const int NUM_DATE_FMTS  = WXSIZEOF(DateFormats);
static const int NUM_DATE_FMTS_LABELS  = WXSIZEOF(DateFormatsLabels);

static const int NUM_DATE_FIELDS = WXSIZEOF(DateFormats);;

class wxDateTextCtrl : public wxTextCtrl
{
public:
   wxDateTextCtrl(wxWindow *parent) : wxTextCtrl(parent,-1)
      {
         m_menu = new wxMenu("", wxMENU_TEAROFF);
         m_menu->SetTitle(_("Format Specifiers:"));
         for ( int n = 0; n < NUM_DATE_FMTS;n++ )
            m_menu->Append(n, _(DateFormatsLabels[n]));
      }
   ~wxDateTextCtrl()
      { delete m_menu; }
   void OnRClick(wxMouseEvent& event)
      { (void)PopupMenu(m_menu, event.GetPosition()); }
   void OnMenu(wxCommandEvent &event)
      {
         ASSERT(event.GetId() >= 0 && event.GetId() < NUM_DATE_FMTS);
         WriteText(DateFormats[event.GetId()]);
      }

protected:
   wxMenu *m_menu;

   DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(wxDateTextCtrl, wxTextCtrl)
   EVT_MENU(-1, wxDateTextCtrl::OnMenu)
   EVT_RIGHT_DOWN(wxDateTextCtrl::OnRClick)
END_EVENT_TABLE()

class wxDateFmtDialog : public wxOptionsPageSubdialog
{
public:
   // ctor & dtor
   wxDateFmtDialog(ProfileBase *profile, wxWindow *parent);
   virtual ~wxDateFmtDialog() { m_timer->Stop(); delete m_timer; }

   // transfer data to/from dialog
   virtual bool TransferDataFromWindow();
   virtual bool TransferDataToWindow();

   // returns TRUE if the format string was changed
   bool WasChanged(void) { return m_DateFmt != m_OldDateFmt;}

   // update the example control to show the current type
   void UpdateExample();

   // event handlers
   void OnUpdate(wxCommandEvent& event) { UpdateExample(); }

   void StopTimer(void) { m_timer->Stop(); }
protected:
   // update timer
   class ExampleUpdateTimer : public wxTimer
   {
   public:
      ExampleUpdateTimer(wxDateFmtDialog *dialog)
      {
         m_dialog = dialog;
      }

      virtual void Notify() { m_dialog->UpdateExample(); }

   private:
      wxDateFmtDialog *m_dialog;
   } *m_timer;

   // data
   wxString  m_DateFmt,
             m_OldDateFmt;

   // GUI controls
   wxCheckBox   *m_UseGMT;       // checked => use GMT for time display
   wxStaticText *m_statExample;  // shows the example of current format
   wxTextCtrl   *m_textctrl;     // the time/date format to use

private:
   DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(wxDateFmtDialog, wxOptionsPageSubdialog)
   EVT_TEXT(-1, wxDateFmtDialog::OnUpdate)
   EVT_CHECKBOX(-1, wxDateFmtDialog::OnUpdate)
END_EVENT_TABLE()

#ifdef _MSC_VER
   // 'this' : used in base member initializer list (so what??)
#   pragma warning(disable:4355)
#endif

wxDateFmtDialog::wxDateFmtDialog(ProfileBase *profile, wxWindow *parent)
               : wxOptionsPageSubdialog(profile,
                                        parent,
                                        _("Date Format"),
                                        "DateFormatDialog")
{
   wxASSERT(NUM_DATE_FMTS == NUM_DATE_FMTS_LABELS);

   wxStaticBox *box = CreateStdButtonsAndBox(_("Date Format"), FALSE,
                                             MH_DIALOG_DATEFMT);

   wxLayoutConstraints *c;

   wxStaticText *stattext = new wxStaticText
                                (
                                 this,
                                 -1,
                                 _("Press the right mouse button over the "
                                   "input field to insert format specifiers.\n")
                                );
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(box, wxTop, 4*LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   stattext->SetConstraints(c);

   m_textctrl = new wxDateTextCtrl(this);
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->top.Below(stattext, 2*LAYOUT_Y_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_Y_MARGIN);
   c->height.AsIs();
   m_textctrl->SetConstraints(c);

   stattext = new wxStaticText(this, -1, _("Here is how it will look like: "));
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->top.Below(m_textctrl, 2*LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   stattext->SetConstraints(c);

   m_statExample = new wxStaticText(this, -1, "");
   c = new wxLayoutConstraints;
   c->left.RightOf(stattext);
   c->top.Below(m_textctrl, 2*LAYOUT_Y_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_Y_MARGIN);
   c->height.AsIs();
   m_statExample->SetConstraints(c);

   m_UseGMT = new wxCheckBox(this, -1, _("Display time in GMT/UST."));
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->top.Below(m_statExample, 4*LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   m_UseGMT->SetConstraints(c);

   SetDefaultSize(380, 240, FALSE /* not minimal */);
   TransferDataToWindow();
   m_OldDateFmt = m_DateFmt;

   m_timer = new ExampleUpdateTimer(this);
   // update each second
   m_timer->Start(1000);
}

#ifdef _MSC_VER
#   pragma warning(default:4355)
#endif

void
wxDateFmtDialog::UpdateExample()
{
   time_t ltime;
   (void)time(&ltime);

   m_statExample->SetLabel(strutil_ftime(ltime,
                                         m_textctrl->GetValue(),
                                         m_UseGMT->GetValue()));
}

bool
wxDateFmtDialog::TransferDataFromWindow()
{
   m_DateFmt = m_textctrl->GetValue();
   GetProfile()->writeEntry(MP_DATE_FMT, m_DateFmt);
   GetProfile()->writeEntry(MP_DATE_GMT, m_UseGMT->GetValue());

   return TRUE;
}

bool
wxDateFmtDialog::TransferDataToWindow()
{
#ifdef OS_WIN
   // MP_DATE_FMT contains '%' which are being (mis)interpreted as env var
   // expansion characters under Windows
   ProfileEnvVarSave noEnvVars(GetProfile());
#endif // OS_WIN

   m_DateFmt = READ_CONFIG(GetProfile(), MP_DATE_FMT);
   m_UseGMT->SetValue( READ_CONFIG(GetProfile(), MP_DATE_GMT) != 0);
   m_textctrl->SetValue(m_DateFmt);

   return TRUE;
}


/* Configuration dialog for sorting messages. */
extern
bool ConfigureDateFormat(ProfileBase *profile, wxWindow *parent)
{
   wxDateFmtDialog dlg(profile, parent);
   if ( dlg.ShowModal() == wxID_OK && dlg.WasChanged() )
   {
      dlg.StopTimer();
      return TRUE;
   }
   else
   {
      dlg.StopTimer();
      return FALSE;
   }
}

void
wxXFaceButton::SetFile(const wxString &filename)
{
   wxBitmap bmp;
   if(filename.Length() != 0)
   {
      bool success = FALSE;
      if(wxFileExists(filename))
         bmp = XFace::GetXFaceImg(filename, &success, m_Parent).ConvertToBitmap();
      if(! success)
      {
         bmp = wxBitmap(wxTheApp->GetStdIcon(wxICON_HAND));
         m_XFace = "";
      }
      else
         m_XFace = filename;
   }
   else
      bmp = mApplication->GetIconManager()->wxIconManager::GetBitmap("noxface");
   SetBitmapLabel(bmp);
   SetBitmapFocus(bmp);
   SetBitmapSelected(bmp);
}


class wxXFaceDialog : public wxOptionsPageSubdialog
{
public:
   wxXFaceDialog(ProfileBase *profile, wxWindow *parent);

   // reset the selected options to their default values
   virtual bool TransferDataFromWindow();
   virtual bool TransferDataToWindow();
   bool WasChanged(void)
      {
         return
            m_Button->GetFile() != m_OldXFace
            || m_Checkbox->GetValue() != m_OldUseXFace;
      };

   void OnButton(wxCommandEvent & event );
protected:
   wxString     m_OldXFace;
   bool          m_OldUseXFace;

   wxXFaceButton *m_Button;
   wxCheckBox    *m_Checkbox;
   bool m_Changed;
   DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(wxXFaceDialog, wxOptionsPageSubdialog)
   EVT_BUTTON(-1, wxXFaceDialog::OnButton)
   EVT_CHECKBOX(-1, wxXFaceDialog::OnButton)
END_EVENT_TABLE()


wxXFaceDialog::wxXFaceDialog(ProfileBase *profile,
                             wxWindow *parent)
   : wxOptionsPageSubdialog(profile,parent,
                            _("Choose a XFace"),
                            "XFaceChooser")
{
   wxStaticBox *box = CreateStdButtonsAndBox(_("XFace"), FALSE,
                                             MH_DIALOG_XFACE);
   wxLayoutConstraints *c;

   wxStaticText *stattext = new wxStaticText(this, -1,
                                             _("XFaces are small images that can be included\n"
                                               "in your mail message to help recognise you.\n"
                                               "They are limited to 48*48 pixels black and white,\n"
                                               "but Mahogany will automatically convert any file\n"
                                               "to this specification automatically.\n"
                                               "\n"
                                               "Click on the button to change it."));
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(box, wxTop, 6*LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   stattext->SetConstraints(c);


   m_Button = new wxXFaceButton(this, -1, "");
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->top.Below(stattext, 2*LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   m_Button->SetConstraints(c);

   m_Checkbox = new wxCheckBox(this, -1, _("Use XFace."));
   c = new wxLayoutConstraints;
   c->left.RightOf(m_Button, 4*LAYOUT_X_MARGIN);
   c->top.Below(stattext, 2*LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   m_Checkbox->SetConstraints(c);

   m_Changed = FALSE;

   SetDefaultSize(280, 220, FALSE /* not minimal */);
   TransferDataToWindow();
   m_OldXFace = m_Button->GetFile();
   m_OldUseXFace = m_Checkbox->GetValue();
}


void
wxXFaceDialog::OnButton(wxCommandEvent & event )
{
   wxObject *obj = event.GetEventObject();
   if ( obj != m_Button && obj != m_Checkbox)
   {
      event.Skip();
      return;
   }

   if(obj == m_Button)
   {
      wxString path, file, xface;
      String newface;
      xface = m_Button->GetFile();
      path = xface.BeforeLast('/');
      file = xface.AfterLast('/');
      newface = wxPFileSelector(GetProfile()->GetName()+"/xfacefilerequester",
                                _("Please pick an image file"),
                                path, file, NULL,
                                NULL, 0, this);
      m_Button->SetFile(newface);
   }
   m_Button->Enable(m_Checkbox->GetValue() != 0);
}

bool
wxXFaceDialog::TransferDataToWindow()
{
   m_Button->SetFile(READ_CONFIG(GetProfile(), MP_COMPOSE_XFACE_FILE));
   m_Checkbox->SetValue( READ_CONFIG(GetProfile(), MP_COMPOSE_USE_XFACE) != 0);
   m_Button->Enable(m_Checkbox->GetValue() != 0);
   return TRUE;
}

bool
wxXFaceDialog::TransferDataFromWindow()
{
   GetProfile()->writeEntry(MP_COMPOSE_XFACE_FILE, m_Button->GetFile());
   GetProfile()->writeEntry(MP_COMPOSE_USE_XFACE, m_Checkbox->GetValue());
   return TRUE;
}

extern
bool PickXFaceDialog(ProfileBase *profile, wxWindow *parent)
{
   wxXFaceDialog dlg(profile, parent);
   return ( dlg.ShowModal() == wxID_OK && dlg.WasChanged() );
}



extern
void CheckExpungeDialog(ASMailFolder *mf, wxWindow *parent)
{
   /// For all non-NNTP folders, check if the user wants to
   /// auto-expunge the messages?
   String msg;
   msg.Printf(_("Do you want to expunge all deleted messages\n"
                "in folder '%s'?"),
              mf->GetName().c_str());
   if(mf->GetType() != MF_NNTP
      && mf->GetType() != MF_NEWS
      && mf->CountMessages(MailFolder::MSG_STAT_DELETED,MailFolder::MSG_STAT_DELETED)
      && MDialog_YesNoDialog(msg,
                             parent,
                             MDIALOG_YESNOTITLE,
                             true,
                             ProfileBase::FilterProfileName(mf->GetProfile()->GetName())+"_AutoExpunge"))
   {
      (void) mf->ExpungeMessages();
   }
}

// ----------------------------------------------------------------------------
// reenable previously disabled persistent message boxes
//
// TODO should all this stuff go to a separate file?
// ----------------------------------------------------------------------------

static const struct
{
   const char *name;
   const char *desc;
} gs_persMsgBoxData[] =
{
   { "AskSpecifyDir",            gettext_noop("prompt for global directory if not found") },
   { "SendOutboxOnExit",         gettext_noop("ask whether to send unset messages on exit") },
   { "AbandonCriticalFolders",   gettext_noop("prompt before abandoning critical folders ") },
   { "GoOnlineToSendOutbox",     gettext_noop("ask whether to go online to send Outbox") },
   { "FixTemplate",              gettext_noop("propose to fix template with errors") },
   { "AskForSig",                gettext_noop("ask for signature if none found") },
   { "UnsavedCloseAnyway",       gettext_noop("propose to save message before closing") },
   { "MessageSent",              gettext_noop("show notification after sending the message") },
   { "AskForExtEdit",            gettext_noop("propose to change external editor settings if unset") },
   { "MimeTypeCorrect",          gettext_noop("ask confirmation for guessed MIME types") },
   { "ConfigNetFromCompose",     gettext_noop("propose to configure network settings before sending the message if necessary") },
   { "SendemptySubject",         gettext_noop("ask confirmation before sending messages without subject") },
   { "ConfirmFolderDelete",      gettext_noop("ask confirmation before removing folder from the folder tree") },
   { "ConfirmFolderPhysDelete",  gettext_noop("ask confirmation before deleting folder with its contents") },
   { "MarkRead",                 gettext_noop("ask whether to mark all articles as read before closing folder") },
   { "DialUpConnectedMsg",       gettext_noop("show notification on dial-up network connection") },
   { "DialUpDisconnectedMsg",    gettext_noop("show notification on dial-up network disconnection") },
   { MP_CONFIRMEXIT,             gettext_noop("ask confirmation before exiting the program") },
   { "AskLogin",                 gettext_noop("ask for the login name when opening the folder if required") },
   { "AskPwd",                   gettext_noop("ask for the password when opening the folder if required") },
   { "GoOfflineSendFirst",       gettext_noop("propose to send outgoing messages before hanging up") },
   { "OpenUnaccessibleFolder",   gettext_noop("warn before trying to reopen folder which couldn't be opened the last time") },
   { "ChangeUnaccessibleFolderSettings", gettext_noop("propose to change settings of a folder which couldn't be opened the last time before reopening it") },
   { "AskUrlBrowser",            gettext_noop("ask for the WWW browser if it was not configured") },
   { "OptTestAsk",               gettext_noop("propose to test new settings after changing any important ones") },
   { "WarnRestartOpt",           gettext_noop("warn if some options changes don't take effect until program restart") },
   { "SaveTemplate",             gettext_noop("propose to save changed template before closing it") },
   { "DialUpOnOpenFolder",       gettext_noop("propose to start dial up networking before trying to open a remote folder") },
   { "NetDownOpenAnyway",        gettext_noop("warn before opening remote folder while not being online") },
   { "NoNetPingAnyway",          gettext_noop("ask whether to ping remote folders offline") },
   { "MailNoNetQueuedMessage",   gettext_noop("show notification if the message is queued in Outbox and not sent out immediately") },
   { "MailQueuedMessage",        gettext_noop("show notification for queued messages") },
   { "MailSentMessage",          gettext_noop("show notification for sent messages") },
   { "TestMailSent",             gettext_noop("show succesful test message") },
   { "AdbDeleteEntry",           gettext_noop("ask for confirmation before deleing the address book entries") },
   { "ConfirmAdbImporter",       gettext_noop("ask for confirmation before importing unreckognized address book files") },
   { "BbdbSaveDialog",           gettext_noop("ask for confirmation before saving address books in BBDB format") },
   { "FolderGroupHint",          gettext_noop("show explanation after creating a folder group") },
   { "SignatureTooLong",         gettext_noop("warn if signature is longer than netiquette recommends") },
   //{ "", gettext_noop("") },
};

/// return the name to use for the given persistent msg box
extern String GetPersMsgBoxName(MPersMsgBox which)
{
   ASSERT_MSG( M_MSGBOX_MAX == WXSIZEOF(gs_persMsgBoxData),
               "should be kept in sync!" );

   return gs_persMsgBoxData[which].name;
}

/// return a user-readable description of the pers msg box with given name
static
String GetPersistentMsgBoxDesc(const String& name)
{
   String s;
   for ( size_t n = 0; n < WXSIZEOF(gs_persMsgBoxData); n++ )
   {
      if ( gs_persMsgBoxData[n].name == name )
      {
         s = _(gs_persMsgBoxData[n].desc);
         break;
      }
   }

   if ( !s )
   {
      s.Printf(_("unknown (%s)"), name.c_str());
   }

   return s;
}

class ReenableDialog : public wxManuallyLaidOutDialog
{
public:
   ReenableDialog(wxWindow *parent);

   // adds all entries for the given folder to the listctrl
   void AddAllEntries(wxConfigBase *config,
                      const String& folder,
                      wxArrayString& entries);

   // get the selected items
   const wxArrayInt& GetSelections() const { return m_selections; }

   // return TRUE if there are any msg boxes to reenable
   bool ShouldShow() const { return m_listctrl->GetItemCount() != 0; }

   // transfer data (selections) from control
   virtual bool TransferDataFromWindow();

private:
   wxArrayInt  m_selections;
   wxListCtrl *m_listctrl;
};

ReenableDialog::ReenableDialog(wxWindow *parent)
              : wxManuallyLaidOutDialog(parent,
                                        _("Mahogany"),
                                        "ReenableDialog")
{
   // layout the controls
   // -------------------
   wxLayoutConstraints *c;

   // Ok and Cancel buttons and a static box around everything else
   wxStaticBox *box = CreateStdButtonsAndBox(_("&Settings"));

   // an explanatory text
   wxStaticText *text = new wxStaticText(this, -1,
                                         _("Select the settings to reset"));
   c = new wxLayoutConstraints;
   c->top.SameAs(box, wxTop, 4*LAYOUT_Y_MARGIN);
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   text->SetConstraints(c);

   // and a listctrl
   m_listctrl = new wxPListCtrl("PMsgBoxList", this, -1,
                                wxDefaultPosition, wxDefaultSize,
                                wxLC_REPORT);
   c = new wxLayoutConstraints;
   c->top.Below(text, 2*LAYOUT_Y_MARGIN);
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->bottom.SameAs(box, wxBottom, 2*LAYOUT_Y_MARGIN);
   m_listctrl->SetConstraints(c);

   // setup the listctrl columns
   m_listctrl->InsertColumn(0, _("Setting"));
   m_listctrl->InsertColumn(1, _("Value"));
   m_listctrl->InsertColumn(2, _("Folder"));

   // finishing initialization
   // ------------------------

   m_listctrl->SetFocus();

   // set the minimal and initial window size
   SetDefaultSize(6*wBtn, 8*hBtn);
}

void ReenableDialog::AddAllEntries(wxConfigBase *config,
                                   const String& folder,
                                   wxArrayString& entries)
{
   long dummy;
   String name;
   bool cont = config->GetFirstEntry(name, dummy);
   while ( cont )
   {
      int index = m_listctrl->GetItemCount();
      m_listctrl->InsertItem(index, GetPersistentMsgBoxDesc(name));

      // decode the remembered value
      String value;
      long val = config->Read(name, 0l);
      switch ( val )
      {
         case wxYES:
         case wxID_YES:
            value = _("yes");
            break;

         case wxNO:
         case wxID_NO:
            value = _("no");
            break;

         default:
            // must be a simple msg box
            value = _("off");
      }

      m_listctrl->SetItem(index, 1, value);
      m_listctrl->SetItem(index, 2, !folder ? String(_("all folders")) : folder);

      // and remember the config path in case we'll delete it later
      if ( !!folder )
      {
         name.Prepend(folder + '/');
      }
      entries.Add(name);

      cont = config->GetNextEntry(name, dummy);
   }
}

bool ReenableDialog::TransferDataFromWindow()
{
   ASSERT_MSG( m_selections.IsEmpty(),
               "TransferDataFromWindow() called twice?" );

   int index = -1;
   for ( ;; )
   {
      index = m_listctrl->GetNextItem(index,
                                      wxLIST_NEXT_ALL,
                                      wxLIST_STATE_SELECTED);
      if ( index == -1 )
         break;

      m_selections.Add(index);
   }

   return TRUE;
}

extern
bool ReenablePersistentMessageBoxes(wxWindow *parent)
{
   // NB: working with global config in M is tricky - here, we need to create
   //     the dialog before setting the config path because the dlg ctor will
   //     change the current path in the global config...
   ReenableDialog dlg(parent);

   // get all entries under Settings/MessageBox
   wxConfigBase *config = wxConfigBase::Get();
   CHECK( config, FALSE, "no app config?" );

   wxString root;
   root << '/' << M_SETTINGS_CONFIG_SECTION << "MessageBox";
   config->SetPath(root);

   wxArrayString entries;
   dlg.AddAllEntries(config, "", entries);

   long dummy;
   String name;
   bool cont = config->GetFirstGroup(name, dummy);
   while ( cont )
   {
      config->SetPath(name);

      dlg.AddAllEntries(config, name, entries);

      config->SetPath("..");

      cont = config->GetNextGroup(name, dummy);
   }

   if ( dlg.ShouldShow() )
   {
      // now show the dialog allowing to choose those entries whih the user
      // wants to reenable
      if ( dlg.ShowModal() )
      {
         const wxArrayInt& selections = dlg.GetSelections();
         size_t count = selections.GetCount();
         if ( count )
         {
            config->SetPath(root);

            for ( size_t n = 0; n < count; n++ )
            {
               config->DeleteEntry(entries[selections[n]]);
            }

            wxLogStatus(_("Reenabled %d previously disabled message boxes."),
                        count);

            return true;
         }
         else
         {
            wxLogStatus(_("No message boxes were reenabled."));
         }
      }
   }
   else
   {
      MDialog_Message(_("No message boxes have been disabled yet."), parent);
   }

   return false;
}

#ifdef USE_SSL
/// Accept or reject certificate
extern "C"
{
   int AcceptCertificateDialog(const char *subject, const char *issuer,
                               const char *fingerprint)
   {
      wxString info;
      info << _("The server presents the following certificate:\n")
           << '\n'
           << _("Name : ") << subject << '\n'
           << _("Issuer : ") << issuer << '\n'
           << _("Fingerprint: ") << fingerprint << '\n'
           << '\n'
           << _("Do you accept this certificate?");
      return (int) MDialog_YesNoDialog(info,
                                       NULL, _("SSL certificate verification"),
                                       true);
   }
}


#endif



class wxLicenseDialog : public wxOptionsPageSubdialog
{
public:
   wxLicenseDialog(ProfileBase *profile, wxWindow *parent);
};

wxLicenseDialog::wxLicenseDialog(ProfileBase *profile, wxWindow *parent)
   : wxOptionsPageSubdialog(profile,parent,
                            _("Mahogany Licensing Conditions"),
                            "LicensingDialog")
{
   wxStaticBox *box = CreateStdButtonsAndBox(_("Licensing Conditions"), FALSE,
                                             MH_DIALOG_LICENSE);
   wxHtmlWindow *license = new wxHtmlWindow(this);

   wxMemoryFSHandler::AddFile("splash.png", wxBITMAP(Mahogany),
                              wxBITMAP_TYPE_PNG);

   license->SetPage("<body text=#000000 bgcolor=#ffffff>"
                "<center><img src=\"memory:splash.png\"><br>"
                "</center>"
#include "license.html"
      );
   wxMemoryFSHandler::RemoveFile("splash.png");


   wxLayoutConstraints *c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(box, wxTop, 6*LAYOUT_Y_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->bottom.SameAs(box, wxBottom, 6*LAYOUT_Y_MARGIN);
   license->SetConstraints(c);

   wxButton *button = (wxButton *) FindWindow(wxID_OK);
   button->SetLabel(_("Accept"));

   button = (wxButton *) FindWindow(wxID_CANCEL);
   button->SetLabel(_("Reject"));

   SetDefaultSize(400, 400, FALSE /* not minimal */);
   SetAutoLayout(TRUE);
}


extern
bool ShowLicenseDialog(wxWindow *parent)
{
   ProfileBase *p = mApplication->GetProfile();
   wxLicenseDialog dlg(p, parent);
   return ( dlg.ShowModal() == wxID_OK );
}
