/*-*- c++ -*-********************************************************
 * wxMDialogs.h : wxWindows version of dialog boxes                 *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
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
#include <wx/radiobox.h>
#include <wx/confbase.h>
#include <wx/persctrl.h>
#include <wx/gauge.h>
#include <wx/stattext.h>

#include "adb/AdbEntry.h"
#include "adb/AdbBook.h"
#include "adb/AdbManager.h"

#include "gui/wxFolderTree.h"
#include "gui/wxFolderView.h"
#include "gui/wxDialogLayout.h"

#ifdef    OS_WIN
#  define M_32x32         "Micon"
#else   //real XPMs
#  include "../src/icons/M_32x32.xpm"
#  include "../src/icons/background.xpm"
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

void wxSafeYield(void)
{
   wxNode *node;
   for ( node = wxTopLevelWindows.GetFirst();
         node;
         node = node->GetNext() )
   {
      wxWindow *win = ((wxWindow*)node->GetData());
      win->Enable(false);
   }

   wxYield();

   for ( node = wxTopLevelWindows.GetFirst();
         node;
         node = node->GetNext() )
   {
      wxWindow *win = ((wxWindow*)node->GetData());
      win->Enable(true);
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
                   const wxString& strDefault);

  // accessors
  const wxString& GetText() const { return m_strText; }

  // base class virtuals implemented
  virtual bool TransferDataToWindow();
  virtual bool TransferDataFromWindow();

private:
  wxString      m_strText;
  wxPTextEntry *m_text;
};

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
// MProgressDialog: a progress bar dialog box with an optional "Cancel" button
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(MProgressDialog, wxFrame)
   EVT_BUTTON(-1, MProgressDialog::OnCancel)
   EVT_CLOSE(MProgressDialog::OnClose)
END_EVENT_TABLE()


MProgressDialog::MProgressDialog(wxString const &title,
                                 wxString const &message,
                                 int maximum,
                                 wxWindow *parent,
                                 bool parentOnly,
                                 bool abortButton)
{
// dangerous: startup window gets deleted while this is running!
//   if( !parent )
//      parent = wxTheApp->GetTopWindow();

   m_state = abortButton ? Continue : Uncancelable;
   m_disableParentOnly = parentOnly;

   int height = 70;     // FIXME arbitrary numbers
   if ( abortButton )
      height += 35;
   wxFrame::Create(parent, -1, _(title),
                   wxPoint(0, 0), wxSize(220, height),
                   wxCAPTION | wxSTAY_ON_TOP | wxTHICK_FRAME);

   wxLayoutConstraints *c;

   wxControl *ctrl = new wxStaticText(this, -1, _(message));
   c = new wxLayoutConstraints;
   c->left.SameAs(this, wxLeft, 10);
   c->top.SameAs(this, wxTop, 10);
   c->width.AsIs();
   c->height.AsIs();
   ctrl->SetConstraints(c);

   m_gauge = new wxGauge(this, -1, maximum);
   c = new wxLayoutConstraints;
   c->left.SameAs(this, wxLeft, 2*LAYOUT_X_MARGIN);
   c->top.Below(ctrl, 2*LAYOUT_Y_MARGIN);
   c->right.SameAs(this, wxRight, 2*LAYOUT_X_MARGIN);
   c->height.AsIs();
   m_gauge->SetConstraints(c);

   if ( abortButton )
   {
      ctrl = new wxButton(this, -1, _("Cancel"));
      c = new wxLayoutConstraints;
      c->centreX.SameAs(this, wxCentreX);
      c->top.Below(m_gauge, 2*LAYOUT_Y_MARGIN);
      c->width.AsIs();
      c->height.AsIs();
      ctrl->SetConstraints(c);
   }

   m_gauge->SetValue(0);

   SetAutoLayout(TRUE);
   Show(TRUE);
   Centre(wxCENTER_FRAME | wxBOTH);

   EnableDisableEvents(false);
   wxYield();
}


void
MProgressDialog::EnableDisableEvents(bool enable)
{
   wxWindow *parent = GetParent();
   if ( m_disableParentOnly && parent != NULL )
   {
      parent->Enable(enable);
   }
   else
   {
      wxNode *node;
      for ( node = wxTopLevelWindows.GetFirst();
            node;
            node = node->GetNext() )
      {
         wxWindow *win = ((wxWindow*)node->GetData());
         win->Enable(enable);
      }
   }

   // always enable ourselves
   Enable(true);
}

bool
MProgressDialog::Update(int value)
{
   m_gauge->SetValue(value);

   wxYield();

   return m_state != Canceled;
}

void MProgressDialog::OnClose(wxCloseEvent& event)
{
   if ( m_state == Uncancelable )
      event.Veto(TRUE);
   else
      m_state = Canceled;
}

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
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) // make it resizealbe
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
static inline wxWindow *GetParent(const wxWindow *parent)
{
  return parent == NULL ? mApplication->TopLevelFrame() : (wxWindow *)parent;
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
                                   const wxString& strDefault)
   : wxDialog(parent, -1, wxString("M: ") + strCaption,
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
  m_text = new wxPTextEntry(strConfigPath, this, -1, "",
                            wxPoint(x + widthLabel + LAYOUT_X_MARGIN, y),
                            wxSize(widthText, heightText));

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
  m_text->SetValue(m_strText);

  // select everything so that it's enough to type a single letter to erase
  // the old value (this way it's as unobtrusive as you may get)
  m_text->SetSelection(-1, -1);

  return TRUE;
}

bool MTextInputDialog::TransferDataFromWindow()
{
  wxString strText = m_text->GetValue();
  if ( strText.IsEmpty() ) {
    // imitate [Cancel] button
    EndModal(wxID_CANCEL);

    return FALSE;
  }
  else {
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
               const char *def)
{
  wxString strConfigPath;
  strConfigPath << "/Prompts/" << szKey;

  MTextInputDialog dlg(GetParent(parent), *pstr,
                       strCaption, strPrompt, strConfigPath, def);

  if ( dlg.ShowModal() == wxID_OK ) {
    *pstr = dlg.GetText();

    return TRUE;
  }
  else {
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
   wxMessageBox(msg, wxString("M: ") + title, Style(wxOK|wxICON_EXCLAMATION),
                GetParent(parent));
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

   MDialog_ErrorMessage(msg.c_str(), parent, wxString("M: ")+title, modal);
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

   MDialog_ErrorMessage(message,parent, wxString("M: ")+title,true);
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
   wxString caption = "M: ";
   caption += title;

   if ( configPath != NULL )
   {
      wxPMessageBox(configPath, message, caption,
                    Style(wxOK | wxICON_INFORMATION),
                    GetParent(parent));
   }
   else
   {
      wxMessageBox(message, caption,
                   Style(wxOK | wxICON_INFORMATION),
                   GetParent(parent));
   }
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
                    bool /* YesDefault */,
                    const char *configPath)
{
   wxString caption = "M: ";
   caption += title;

   if ( configPath != NULL )
   {
      return wxPMessageBox(configPath, message, caption,
                           wxYES_NO | wxICON_QUESTION | wxCENTRE,
                           GetParent(parent)) == wxYES;
   }
   else
   {
      return wxMessageBox(message, caption,
                          Style(wxYES_NO | wxICON_QUESTION),
                          GetParent(parent)) == wxYES;
   }
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
   wxArrayString aChoices;
   wxString strName, strEMail;

   size_t nEntryCount = aEntries.Count();
   for( size_t nEntry = 0; nEntry < nEntryCount; nEntry++ )
   {
      aChoices.Add(aEntries[nEntry]->GetDescription());
   }

   int w,h;
   parent = GetParent(parent);
   parent->GetClientSize(&w,&h);
   w = (w * 8) / 10;
   h = (h * 8) / 10;

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
               wxString("M: ")+_("Please choose an entry:"),
               _("Expansion options"),
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
  void DoClose() { GetParent()->Close(true); delete m_pTimer; }

private:
  // timer which calls our DoClose() when it expires
  class CloseTimer : public wxTimer
  {
  public:
    CloseTimer(wxAboutWindow *window)
    {
      m_window = window;

      Start(READ_APPCONFIG(MP_SPLASHDELAY));
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
   wxLayoutList *ll = GetLayoutList();
   wxBitmap *bm = new wxBitmap(background);
   SetBackgroundBitmap(bm);
   Clear(wxROMAN, 30, wxNORMAL, wxBOLD, FALSE, "yellow");

   ll->LineBreak();
   ll->LineBreak();
   ll->Insert(_("   Mahogany Mail"));
   ll->LineBreak();
   ll->SetFontWeight(wxNORMAL);
   ll->SetFontSize(10);

   String version = _("    Version: ");
   version += M_VERSION_STRING;
   ll->Insert(version);
   ll->LineBreak();
   version = _("        compiled for: ");
#ifdef OS_UNIX
   version += M_OSINFO;
#else // Windows
   // TODO put Windows version info here
   version += "Windows";
#endif // Unix/Windows

   ll->Insert(version);
   ll->LineBreak();
   ll->LineBreak();
   ll->LineBreak();
   ll->Insert(_("  Copyright (c) 1999 by Karsten Ballüder"));
   ll->LineBreak();
   ll->LineBreak();
   ll->Insert(_("          Written by Karsten Ballüder"));
   ll->LineBreak();
   ll->Insert(_("                      and Vadim Zeitlin"));
   ll->LineBreak();
   DoPaint();
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
                      wxDefaultPosition, wxSize(300, 300),
                      /* 0 style for borderless wxDOUBLE_BORDER |*/ wxSTAY_ON_TOP)
{
   wxCHECK_RET( g_pSplashScreen == NULL, "one splash is more than enough" );
   g_pSplashScreen = (MFrame *)this;

   (void)new wxAboutWindow(this, bCloseOnTimeout);

   Centre(wxBOTH);
   Show(TRUE);
}

void
MDialog_AboutDialog( const MWindow * /* parent */, bool bCloseOnTimeout)
{
   (void)new wxAboutFrame(bCloseOnTimeout);
   wxYield(); // to make sure it gets displayed at startup
}


/** A base class for dialog panels */
class wxProfileEditPanel : public wxDialog
{
public:
   wxProfileEditPanel(wxWindow *parent, const String& title)
      : wxDialog(parent, -1, title, wxPoint(0,0), wxSize(400, 400))
      {}

   /// virtual destructor
   virtual ~wxProfileEditPanel() {}

protected:
   ProfileBase  *m_Profile;
   wxWindow *m_Parent;
};

class wxPEP_Folder : public wxProfileEditPanel
{
public:
   wxPEP_Folder(ProfileBase *profile, wxWindow *parent);

   // transfer data to/from window
   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

   // update controls
   void    UpdateUI(void);

   void OnEvent(wxCommandEvent&);

private:
   // profile settings:
   int    m_FolderType;
   String m_FolderPath;
   int    m_UpdateInterval;
   String m_UserId;
   String m_Password;

   wxRadioBox *m_FolderTypeRadioBox;
   wxTextCtrl *m_FolderPathTextCtrl;
   wxTextCtrl *m_UpdateIntervalTextCtrl;
   wxTextCtrl *m_UserIdTextCtrl;
   wxTextCtrl *m_PasswordTextCtrl;
   wxStaticText *m_PathStaticText;
   wxButton   *m_CancelButton, *m_OkButton, *m_UndoButton;
   wxString    m_choices[News - Inbox];

   DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(wxPEP_Folder, wxProfileEditPanel)
  EVT_RADIOBOX(M_WXID_PEP_RADIO, wxPEP_Folder::OnEvent)
  EVT_BUTTON(M_WXID_PEP_UNDO, wxPEP_Folder::OnEvent)
END_EVENT_TABLE()

#define   MkTextCtrl(control,label,id) \
  (void) new wxStaticText(this, id, _(label), pos, wxSize(labelWidth,labelHeight)); \
  control = new wxTextCtrl(this, id, "",wxPoint(pos.x+labelWidth+LAYOUT_X_MARGIN,pos.y),wxSize(inputWidth,-1)); \
  pos.y += labelHeight;

#define   MkTextCtrlAndLabel(control,labelctrl,label,id) \
  labelctrl = new wxStaticText(this, id, _(label), pos, wxSize(labelWidth,labelHeight)); \
  control = new wxTextCtrl(this, id, "",wxPoint(pos.x+labelWidth+LAYOUT_X_MARGIN,pos.y),wxSize(inputWidth,-1)); \
  pos.y += labelHeight;

#define  MkButton(control,label,id) \
  control = new wxButton(this, id, _(label), pos, \
                         wxSize(widthBtn, heightBtn))

void
wxPEP_Folder::OnEvent(wxCommandEvent& event)
{
   switch(event.GetId())
   {
      case M_WXID_PEP_UNDO:
         TransferDataToWindow();
         break;

      case M_WXID_PEP_RADIO:
         UpdateUI();
         break;

      default:
         event.Skip();
   }
}

wxPEP_Folder::wxPEP_Folder(ProfileBase *profile, wxWindow *parent)
            : wxProfileEditPanel(parent, _("Folder properties"))
{
   m_Profile = profile;
   m_Parent = parent;
   wxASSERT(m_Profile);

   wxPoint  pos = wxPoint(2*LAYOUT_X_MARGIN, 2*LAYOUT_Y_MARGIN);
   long labelWidth, labelHeight;
   long inputWidth;

   // first determine the longest button caption
   const char *label = "Update interval in seconds";
   wxClientDC dc(this);
   dc.SetFont(wxSystemSettings::GetSystemFont(wxSYS_DEFAULT_GUI_FONT));
   dc.GetTextExtent(_(label), &labelWidth, &labelHeight);
   size_t heightBtn = TEXT_HEIGHT_FROM_LABEL(labelHeight),
          widthBtn = BUTTON_WIDTH_FROM_HEIGHT(heightBtn);

   labelHeight *= 2;
   labelWidth += 10;
   inputWidth = labelWidth;
   inputWidth += 10;

   pos.y += LAYOUT_Y_MARGIN;
   MkTextCtrlAndLabel(m_FolderPathTextCtrl, m_PathStaticText, "",-1);
   MkTextCtrl(m_UpdateIntervalTextCtrl, label,-1);
   MkTextCtrl(m_UserIdTextCtrl, "User ID",-1);
   MkTextCtrl(m_PasswordTextCtrl, "Password",-1);

   m_choices[Inbox] = _("INBOX");
   m_choices[File]  = _("Message box file");
   m_choices[POP]   = _("POP3");
   m_choices[IMAP]  = _("IMAP");
   m_choices[News]  = _("News");
   long xRadio = labelWidth + inputWidth + 20;
   m_FolderTypeRadioBox = new wxRadioBox( this, M_WXID_PEP_RADIO,
                                          _("Folder Type"),
                                          wxPoint(xRadio, 2*LAYOUT_Y_MARGIN),
                                          wxSize(-1,-1),
                                          5, m_choices,
                                          // vertical radiobox
                                          1, wxRA_SPECIFY_COLS );

   int widthRadio, heightRadio;
   m_FolderTypeRadioBox->GetSize(&widthRadio, &heightRadio);

   long widthTotal = xRadio + widthRadio;
   pos.y += 3*LAYOUT_Y_MARGIN;
   pos.x = widthTotal - 3*widthBtn - 2*LAYOUT_X_MARGIN;

   // buttons are always aligned to the right and have the same size
   MkButton(m_OkButton, "OK", wxID_OK);
   pos.x += widthBtn + LAYOUT_X_MARGIN;
   MkButton(m_UndoButton, "&Undo", M_WXID_PEP_UNDO);
   pos.x += widthBtn + LAYOUT_X_MARGIN;
   MkButton(m_CancelButton, "Cancel", wxID_CANCEL);

   Fit();
   UpdateUI();
}

void
wxPEP_Folder::UpdateUI(void)
{
   FolderType type = GetFolderType(m_FolderTypeRadioBox->GetSelection());

   m_FolderPathTextCtrl->Enable(TRUE);
   if ( FolderTypeHasUserName(type) )
   {
      m_UserIdTextCtrl->Enable(TRUE);
      m_PasswordTextCtrl->Enable(TRUE);
      m_PathStaticText->SetLabel(_("Hostname"));
   }
   else
   {
      m_UserIdTextCtrl->Enable(FALSE);
      m_PasswordTextCtrl->Enable(FALSE);
      m_PathStaticText->SetLabel(_("Pathname or name of folder"));
      if ( type == Inbox )
      {
         m_FolderPathTextCtrl->SetValue("INBOX");
         m_FolderPathTextCtrl->Enable(FALSE);
      }
   }
}

bool
wxPEP_Folder::TransferDataFromWindow(void)
{
   FolderType type = GetFolderType(m_FolderTypeRadioBox->GetSelection());

   m_Profile->writeEntry(MP_FOLDER_TYPE,type);
   m_Profile->writeEntry(MP_FOLDER_PATH,m_FolderPathTextCtrl->GetValue());
   m_Profile->writeEntry(MP_UPDATEINTERVAL,atoi(m_UpdateIntervalTextCtrl->GetValue()));

   if ( FolderTypeHasUserName(type) )
   {
      m_Profile->writeEntry(MP_POP_LOGIN,m_UserIdTextCtrl->GetValue());
      m_Profile->writeEntry(MP_POP_PASSWORD,m_PasswordTextCtrl->GetValue());
   }

   // if we return FALSE, it means that entered data is invalid and the dialog
   // wouldn't be closed
   return true;
}

bool
wxPEP_Folder::TransferDataToWindow(void)
{
   FolderType type = GetFolderType(READ_CONFIG(m_Profile, MP_FOLDER_TYPE));

   // the trouble is that if INBOX.profile doesn't exist (yet), we get the
   // wrong value here (FIXME: this is not the right solution neither!)
   if ( type == MP_FOLDER_TYPE_D &&
        ((ProfileBase *)m_Profile)->GetProfileName() == "INBOX" ) { // yuck (FIXME)
      type = Inbox;
   }

   m_FolderTypeRadioBox->SetSelection(type);
   m_FolderPathTextCtrl->SetValue(READ_CONFIG(m_Profile,MP_FOLDER_PATH));
   m_UpdateIntervalTextCtrl->SetValue(strutil_ltoa(READ_CONFIG(m_Profile,MP_UPDATEINTERVAL)));
   if ( FolderTypeHasUserName(type) )
   {
      m_UserIdTextCtrl->SetValue(READ_CONFIG(m_Profile,MP_POP_LOGIN));
      m_PasswordTextCtrl->SetValue(READ_CONFIG(m_Profile,MP_POP_PASSWORD));
   }
   UpdateUI();

   return true;
}

void
MDialog_FolderProfile(const MWindow * /* parent */, ProfileBase *profile)
{
   // show a modal dialog
   wxPEP_Folder dlg(profile, NULL);
   dlg.Centre(wxCENTER_FRAME | wxBOTH);
   dlg.ShowModal();
}


void
MDialog_FolderCreate(const MWindow *parent)
{
   wxString name = "NewFolder";

   if(! MInputBox(&name,
                  _("M - New Folder"),
                  _("Symbolic name for the new folder"),
                  parent,
                  "NewFolderDialog",
                  "NewFolder"))
      return;

   ProfileBase *profile = ProfileBase::CreateProfile(name, NULL);

   MDialog_FolderProfile(parent, profile);

   delete profile;
}

#if 0   // there is already Karstens version...
void
MDialog_FolderOpen(const MWindow *parent)
{
   MFolder *folder = MDialog_FolderChoose(parent);
   if ( folder != NULL )
   {
      // open a view on this folder
      (void)new wxFolderViewFrame(folder->GetName(),
                                  mApplication->TopLevelFrame());
   }
   //else: cancelled
}
#endif // 0

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










#if 0
/* NO LONGER USED */
// ----------------------------------------------------------------------------
// Ressource dialogs
// ----------------------------------------------------------------------------
class wxMRDialog : public wxDialog
{
public:
   wxMRDialog(int helpID = MH_CONTENTS)
      { m_helpID = helpID; } // we cannot Centre() it here because we don't have a parent yet!

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

protected:
   //    callbacks
   void OnButton(wxCommandEvent& event);

   int m_helpID;
   DECLARE_EVENT_TABLE()
};

// a general wxr dialog
BEGIN_EVENT_TABLE(wxMRDialog, wxDialog)
   EVT_BUTTON(-1,     wxMRDialog::OnButton)
END_EVENT_TABLE()

void wxMRDialog::OnButton(wxCommandEvent&  event)
{
   wxWindow * win = (wxWindow *)event.GetEventObject();
   if(win->GetName() == "OK_BUTTON")
   {
      TransferDataFromWindow();
      EndModal(1);
   }
   else if(win->GetName() == "CANCEL_BUTTON")
      EndModal(0);
   else if(win->GetName() == "HELP_BUTTON")
      mApplication->Help(m_helpID,this);
}

bool wxMRDialog::TransferDataToWindow(void)
{
   return true;
}

bool wxMRDialog::TransferDataFromWindow(void)
{
   return true;
}

class wxMROpenFolderDialog : public wxMRDialog
{
public:
   wxMROpenFolderDialog() : wxMRDialog(MH_FOLDER_OPEN_DIALOG) {}
   virtual bool TransferDataFromWindow(void);
   void OnRadio(wxCommandEvent &event);
   void UpdateRadioBox(void);

   int m_Type;
   wxString m_UserID, m_Password, m_Hostname;

   DECLARE_EVENT_TABLE()
};

// a general wxr dialog
BEGIN_EVENT_TABLE(wxMROpenFolderDialog, wxMRDialog)
   EVT_BUTTON(-1,     wxMRDialog::OnButton)
   EVT_RADIOBOX(-1,   wxMROpenFolderDialog::OnRadio)
END_EVENT_TABLE()

void
wxMROpenFolderDialog::OnRadio(wxCommandEvent & /* event */)
{
   UpdateRadioBox();
}

void wxMROpenFolderDialog::UpdateRadioBox(void)
{
   // FIXME: this profile will later inherit from a higher level passed 
   // in when creating the dialog.
   ProfileBase *profile = ProfileBase::CreateEmptyProfile();


   wxRadioBox * win = (wxRadioBox *)wxFindWindowByName("FolderType",
                                                       this);
   wxASSERT(win);

   wxStaticText *UidLabel = (wxStaticText *)wxFindWindowByName("UIDLABEL",this);
   wxStaticText *HostLabel = (wxStaticText *)wxFindWindowByName("HOSTLABEL",this);
   wxTextCtrl   *UserID = (wxTextCtrl *)wxFindWindowByName("UserID",this);
   wxTextCtrl   *Password = (wxTextCtrl *)wxFindWindowByName("Password",this);
   wxTextCtrl   *Hostname = (wxTextCtrl *)wxFindWindowByName("MailHost",this);
   switch(win->GetSelection())
   {
   case 0: // INBOX
      HostLabel->SetLabel(_("Folder Name"));
      Hostname->SetValue(_("INBOX"));
      Hostname->Enable(false);
      UserID->Enable(false);
      Password->Enable(false);
      break;
   case 1: // mbox
      HostLabel->SetLabel(_("Folder Name"));
      UserID->Enable(false);
      Password->Enable(false);
      Hostname->Enable(true);
      break;
   case 2: case 3: // POP3, IMAP
      HostLabel->SetLabel(_("Mailhost"));
      UserID->Enable(true);
      Password->Enable(true);
      Hostname->Enable(true);
      Hostname->SetValue(READ_CONFIG(profile,MP_SMTPHOST));
      break;
   case 4: // NNTP
      HostLabel->SetLabel(_("Newsserver"));
      Hostname->SetValue(READ_CONFIG(profile,MP_NNTPHOST));
      UidLabel->SetLabel(_("Newsgroup"));
      UserID->Enable(true);
      Password->Enable(false);
      Hostname->Enable(true);
      break;

   default:
      /* nothing, keep compiler happy */
      ;
   }
   profile->DecRef();
}

bool
wxMROpenFolderDialog::TransferDataFromWindow(void)
{
   wxRadioBox *ctrl = (wxRadioBox*)wxFindWindowByName("FolderType",this);
   ASSERT(ctrl); m_Type = ctrl->GetSelection();
   wxTextCtrl *tctrl = (wxTextCtrl *)wxFindWindowByName("UserID",this);
   ASSERT(tctrl); m_UserID = tctrl->GetValue();
   tctrl = (wxTextCtrl *)wxFindWindowByName("Password",this);
   ASSERT(tctrl); m_Password = tctrl->GetValue();
   tctrl = (wxTextCtrl *)wxFindWindowByName("MailHost",this);
   ASSERT(tctrl); m_Hostname = tctrl->GetValue();
   return true;
}

#include   "wx/resource.h"
#include   "wxr/FODialog.wxr"

void
MDialog_FolderOpen(wxMFrame *parent)
{
   int rc = 0;

   if(! parent)
      parent = mApplication->TopLevelFrame();
   wxResourceParseData(OpenFolderDialog);
   wxMROpenFolderDialog *dialog = new wxMROpenFolderDialog;
   if (dialog->LoadFromResource(parent, "OpenFolderDialog"))
   {
      dialog->Centre();
      dialog->UpdateRadioBox();
      rc = dialog->ShowModal();
      if(rc == wxOK)
      {
         MailFolder *mf =
            MailFolder::OpenFolder((MailFolder::Type)dialog->m_Type,
                                   dialog->m_Hostname, NULL,
                                   dialog->m_UserID, dialog->m_Password);
         if(mf)
            if(wxFolderViewFrame::Create(mf, parent))
               mf->DecRef(); // now the folder view has increfed the folder
      }
      dialog->Close(TRUE);
   }
}
#endif
