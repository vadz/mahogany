/*-*- c++ -*-********************************************************
 * wxMDialogs.h : wxWindows version of dialog boxes                 *
 *                                                                  *
 * (C) 1998-1999 by Karsten Ballüder (karsten@phy.hw.ac.uk)         *
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

#include <errno.h>

#ifndef USE_PCH
#   include "Mcommon.h"
#   include "Mdefaults.h"
#   include "guidef.h"
#   include "strutil.h"
#   include "MFrame.h"
#   include "Profile.h"
#   include "MApplication.h"
#   include "MailFolder.h"
#   include "Profile.h"
#endif

#include "MHelp.h"

#include "gui/wxMApp.h"
#include "gui/wxMIds.h"

#include "MFolder.h"
#include "MDialogs.h"
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

#include "MFolderDialogs.h"

#include "adb/AdbEntry.h"
#include "adb/AdbBook.h"
#include "adb/AdbManager.h"

#include "gui/wxFolderTree.h"
#include "gui/wxFolderView.h"
#include "gui/wxDialogLayout.h"

#ifdef    OS_WIN
#  define mahogany   "mahogany"
#  define background "background"
#else   //real XPMs
#  include "../src/icons/background.xpm"
#  include "../src/icons/mahogany.xpm"
#  ifdef USE_PYTHON
#     include "../src/icons/pythonpower.xpm"
#  endif // USE_PYTHON
#endif  //Win/Unix

// ----------------------------------------------------------------------------
// global vars and functions
// ----------------------------------------------------------------------------
MFrame *g_pSplashScreen = NULL;

extern void CloseSplash()
{
  if ( g_pSplashScreen ) {
    g_pSplashScreen->Show(FALSE);

    // and it will be closed when timeout elapses (it's the most fool proof
    // solution, if not the most direct one)
  }
}

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

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
class MFolderDialog : public wxDialog
{
public:
   MFolderDialog(wxWindow *parent, const wxPoint& pos, const wxSize& size);
   ~MFolderDialog() { delete m_tree; }

   // accessors
   MFolder *GetFolder() const { return m_folder; }

   // base class virtuals implemented
   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

private:
   MFolder      *m_folder;
   wxFolderTree *m_tree;
};


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
   MGuiLocker lock;
   CloseSplash();
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

   MGuiLocker lock;
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

   {
      MGuiLocker lock;
      MDialog_ErrorMessage(message,parent, wxString("Mahogany : ")+title,true);
   }
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

   MGuiLocker lock;
   CloseSplash();
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
   
   MGuiLocker lock;
   CloseSplash();
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
   MGuiLocker lock;
   CloseSplash();

   // VZ: disabling this code because it is almost useless now with the advent
   //     of wxPFileSelector()
#if 0
   if(! profile)
      profile = mApplication->GetProfile();

   if(! path)
      path = save ?
         profile->readEntry(MP_DEFAULT_SAVE_PATH,MP_DEFAULT_SAVE_PATH_D)
         : profile->readEntry(MP_DEFAULT_LOAD_PATH,MP_DEFAULT_LOAD_PATH_D);
   if(! filename)
      filename = save ?
         profile->readEntry(MP_DEFAULT_SAVE_FILENAME,MP_DEFAULT_SAVE_FILENAME_D)
         : profile->readEntry(MP_DEFAULT_LOAD_FILENAME,MP_DEFAULT_LOAD_FILENAME_D);
   if(! extension)
      extension = save ?
         profile->readEntry(MP_DEFAULT_SAVE_EXTENSION,MP_DEFAULT_SAVE_EXTENSION_D)
         : profile->readEntry(MP_DEFAULT_LOAD_EXTENSION,MP_DEFAULT_LOAD_EXTENSION_D);
   if(! wildcard)
      wildcard = save ?
         profile->readEntry(MP_DEFAULT_SAVE_WILDCARD,MP_DEFAULT_SAVE_WILDCARD_D)
         : profile->readEntry(MP_DEFAULT_LOAD_WILDCARD,MP_DEFAULT_LOAD_WILDCARD_D);
#endif // 0

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
   MGuiLocker lock;
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

// the main difference is that it goes away as soon as you click it
// or after some time (if not disabled in the ctor).
//
// It is also unique and must be removed before showing any message boxes
// (because it has on top attribute) with CloseSplash() function.
class wxAboutWindow : public wxLayoutWindow
{
public:
  // fills the window with some pretty text
  wxAboutWindow(wxFrame *parent, bool bCloseOnTimeout = true);

  // mouse event handler closes the parent window
  void OnClick(wxMouseEvent&) { DoClose(); }

  // close the about frame
  void DoClose() { if(GetParent()) GetParent()->Close(true); delete m_pTimer; }

private:
  // timer which calls our DoClose() when it expires
  class CloseTimer : public wxTimer
  {
  public:
    CloseTimer(wxAboutWindow *window)
    {
      m_window = window;

      Start(READ_APPCONFIG(MP_SPLASHDELAY)*1000);
    }

    virtual void Notify() { m_window->DoClose(); }

  private:
    wxAboutWindow *m_window;
  } *m_pTimer;

  DECLARE_EVENT_TABLE();
};

class wxAboutFrame : public wxFrame
{
public:
  wxAboutFrame(bool bCloseOnTimeout);
  ~wxAboutFrame() { g_pSplashScreen = NULL; }
};

BEGIN_EVENT_TABLE(wxAboutWindow, wxLayoutWindow)
  EVT_LEFT_DOWN(OnClick)
  EVT_MIDDLE_DOWN(OnClick)
  EVT_RIGHT_DOWN(OnClick)
END_EVENT_TABLE()

wxAboutWindow::wxAboutWindow(wxFrame *parent, bool bCloseOnTimeout)
             : wxLayoutWindow(parent)
{
   // strings used for primitive alignment of text
   static const char *align = "                 ";
   static const char *align2 = "        ";
#ifdef __WXMSW__
   static const char *align4 = "    ";
#endif

   wxLayoutList *ll = GetLayoutList();
   wxBitmap *bm = new wxBitmap(background);
   SetBackgroundBitmap(bm);

   SetCursorVisibility(0);
   wxColour col("blue");
   Clear(wxDECORATIVE, 30, (int)wxNORMAL, (int)wxBOLD, FALSE, &col);

   // unfortunately, I can't make it transparent under Windows, so it looks
   // really ugly - disabling for now
#ifdef __WXMSW__
   ll->Insert(align4);
   ll->Insert("Welcome to ");
   ll->LineBreak();
   ll->Insert(align2);
   ll->Insert("Mahogany!");
   ll->LineBreak();
#else
   ll->Insert(new wxLayoutObjectIcon(wxBitmap(mahogany)));
   ll->LineBreak();
#endif // 0

   ll->SetFont(wxROMAN, 10, wxNORMAL, wxNORMAL, FALSE, "yellow");
   ll->LineBreak();
   String version = _("Version: ");
   version += M_VERSION_STRING;
   ll->Insert(align);
   ll->Insert(align);
   ll->Insert(version);
   ll->LineBreak();
   version = _("compiled for ");
#ifdef OS_UNIX
   version += M_OSINFO;
#else // Windows
   // TODO put Windows version info here
   version += "Windows";
#endif // Unix/Windows

   ll->Insert(align);
   ll->Insert(align);
   ll->Insert(version);
   ll->LineBreak();
   ll->LineBreak();

   ll->SetFontSize(12);
   ll->Insert(align);
   ll->Insert(_("Copyright (c) 1999 by Karsten Ballüder"));
   ll->LineBreak();
   ll->LineBreak();
   ll->Insert(align);
   ll->Insert(align2);
   ll->Insert(_("Written by Karsten Ballüder"));
   ll->LineBreak();
   ll->Insert(align);
   ll->Insert(align);
   ll->Insert(_("and Vadim Zeitlin"));
   ll->LineBreak();
   ll->LineBreak();
   ll->SetFontSize(8);
   ll->Insert(align);
   ll->Insert(_("This software is provied 'as is' and without any expressed or implied"));
   ll->LineBreak();
   ll->Insert(align);
   ll->Insert(_("warranties, including, without limitation, the implied warranties"));
   ll->LineBreak();
   ll->Insert(align);
   ll->Insert(_("of merchantibility and fitness for a particular purpose."));
   ll->LineBreak();
   ll->Insert(align);
   ll->Insert(_("This is OpenSource(TM) software."));
#ifdef USE_PYTHON
   ll->LineBreak();
   ll->LineBreak();
   ll->Insert(align);
   ll->Insert(align);
   ll->Insert(new wxLayoutObjectIcon(wxIcon(pythonpower)));
#endif // Python
   
   Refresh();
   ResizeScrollbars(true); // let them disappear
   // start a timer which will close us (if not disabled)
   if ( bCloseOnTimeout ) {
     m_pTimer = new CloseTimer(this);
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
                      wxSize(400, 350),
#else  // !MSW
                      wxSize(320, 270),
#endif // MSW/!MSW
                      /* no border styles at all */ wxSTAY_ON_TOP )
{
   wxCHECK_RET( g_pSplashScreen == NULL, "one splash is more than enough" );
   g_pSplashScreen = (MFrame *)this;

   (void)new wxAboutWindow(this, bCloseOnTimeout);

   Centre(wxCENTER_FRAME | wxBOTH);
   Show(TRUE);
}

void
MDialog_AboutDialog( const MWindow * /* parent */, bool bCloseOnTimeout)
{
   (void)new wxAboutFrame(bCloseOnTimeout);
   wxSafeYield(); // to make sure it gets displayed at startup
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
             : wxDialog(parent, -1, _("Choose folder"),
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

bool MFolderDialog::TransferDataToWindow()
{
   // restore last folder from config
   return true;
}

bool MFolderDialog::TransferDataFromWindow()
{
   m_folder = m_tree->GetSelection();
   if ( m_folder != NULL )
   {
      // save the folder to config
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

#define NUM_SORTLEVELS 6
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
   gettext_noop("Thread")
};

// dedining it liek this makes it much more difficult to forget to update it
static const int NUM_CRITERIA  = WXSIZEOF(sortCriteria);

#define NUM_LABELS 2
static wxString labels[NUM_LABELS] =
{
   gettext_noop("First, sort by"),
   gettext_noop("then, sort by")
};

class wxMessageSortingDialog : public wxManuallyLaidOutDialog
{
public:
   wxMessageSortingDialog(ProfileBase *profile, wxWindow *parent);
   ~wxMessageSortingDialog();

   // reset the selected options to their default values
   virtual bool TransferDataFromWindow(); 
   virtual bool TransferDataToWindow();
   bool WasChanged(void) { return FALSE;};
protected:
   ProfileBase *m_Profile;
   wxChoice    *m_Choices[NUM_CRITERIA];
};

wxMessageSortingDialog::wxMessageSortingDialog(ProfileBase *profile, wxWindow *parent)
   : wxManuallyLaidOutDialog( parent,
                              _("Message sorting"),
                              "MessageSortingDialog")
{
   m_Profile = profile;
   profile->IncRef(); // paranoid
   
   SetDefaultSize(380,400);

   wxStaticBox *box = CreateStdButtonsAndBox(_("Sort messages by"),MH_DIALOG_SORTING);

   wxClientDC dc(this);
   dc.SetFont(wxSystemSettings::GetSystemFont(wxSYS_DEFAULT_GUI_FONT));
   long width, widthMax = 0;
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
                                           n < NUM_LABELS ? _(labels[n]) :
                                           _(labels[NUM_LABELS-1]),
                                           wxDefaultPosition);
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
}

wxMessageSortingDialog::~wxMessageSortingDialog()
{
   m_Profile->DecRef();
}


bool wxMessageSortingDialog::TransferDataFromWindow()
{
   return TRUE;
}

bool wxMessageSortingDialog::TransferDataToWindow()
{
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
