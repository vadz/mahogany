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
#include "Mcommon.h"

#include <errno.h>

#ifndef USE_PCH
#  include "guidef.h"
#  include "strutil.h"
#  include "MFrame.h"
#  include "MLogFrame.h"
#  include "PathFinder.h"
#  include "MimeList.h"
#  include "MimeTypes.h"
#  include "Profile.h"
#  include "MApplication.h"
#endif

#include "Mdefaults.h"

#include "gui/wxMApp.h"
#include "gui/wxMIds.h"

#include "MDialogs.h"
#include "gui/wxlwindow.h"

#include "gui/wxIconManager.h"

#include <wx/dynarray.h>
#include <wx/radiobox.h>
//FIXME#include <wx/minifram.h>

#include "adb/AdbEntry.h"
#include "adb/AdbBook.h"

#ifdef    OS_WIN
#  define M_32x32         "Micon"
#else   //real XPMs
#  include "../src/icons/M_32x32.xpm"
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

// an array of AdbEntries
WX_DEFINE_ARRAY(AdbEntry *, ArrayAdbEntries);

// better looking wxTextEntryDialog
class MTextInputDialog : public wxDialog
{
public:
  MTextInputDialog(wxWindow *parent, const wxString& strText,
                    const wxString& strCaption, const wxString& strPrompt);

  // accessors
  const wxString& GetText() const { return m_strText; }

  // base class virtuals implemented
  virtual bool TransferDataToWindow();
  virtual bool TransferDataFromWindow();

private:
  wxString    m_strText;
  wxTextCtrl *m_text;
};

// ----------------------------------------------------------------------------
// functions
// ----------------------------------------------------------------------------

// under Windows we don't use wxCENTRE style which uses the generic message box
// instead of the native one (and thus it doesn't have icons, for example)
inline long Style(long style)
{
# ifdef OS_WIN
    return style;
# else //OS_WIN
    return style | wxCENTRE;
# endif
}

// returns the argument if it's !NULL of the top-level application frame
inline MWindow *GetParent(MWindow *parent)
{
  return parent == NULL ? mApplication.TopLevelFrame() : parent;
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
                                   const wxString& strPrompt)
   : wxDialog(parent, -1, strCaption,
              wxDefaultPosition,
              wxDefaultSize,
              wxDEFAULT_DIALOG_STYLE | wxDIALOG_MODAL),
     m_strText(strText)
{
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
  m_text = new wxTextCtrl(this, -1, "",
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
  m_text->SetSelection(-1, -1); // select everything

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
               wxWindow *parent,
               const char *szKey,
               const char *def)
{
  static const wxString strSectionName = "/Prompts/";

  wxConfigBase *pConf = NULL;

  if ( !IsEmpty(szKey) ) {
    pConf = Profile::GetAppConfig();
    if ( pConf != NULL )
      pConf->Read(pstr, strSectionName + szKey, def);
  }

  MTextInputDialog dlg(GetParent(parent), *pstr, wxString("M - "+strCaption), strPrompt);
  if ( dlg.ShowModal() == wxID_OK ) {
    *pstr = dlg.GetText();

    if ( pConf != NULL )
      pConf->Write(strSectionName + szKey, *pstr);

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
                     MWindow *parent,
                     const char *title,
                     bool /* modal */)
{
   wxMessageBox(msg, wxString(title), Style(wxOK|wxICON_EXCLAMATION),
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
               MWindow *parent,
               const char *title,
               bool modal)
{
   String
      msg;
   
   msg = String(message) + String(("\nSystem error: "))
      + String(strerror(errno));

   MDialog_ErrorMessage(msg.c_str(), parent, wxString(title), modal);
}

   
/** display error message and exit application
       @param message the text to display
       @param title  title for message box window
       @param parent the parent frame
   */
void
MDialog_FatalErrorMessage(const char *message,
              MWindow *parent,
              const char *title)
{
   String msg = String(message) + _("\nExiting application...");

   MDialog_ErrorMessage(message,parent, wxString(title),true);
   mApplication.Exit(true);
}

   
/** display normal message:
       @param message the text to display
       @param parent the parent frame
       @param title  title for message box window
       @param modal  true to make messagebox modal
   */
void
MDialog_Message(const char *msg,
                MWindow *parent,
                const char *title,
                bool /* modal */)
{
   wxMessageBox(msg, wxString(title), Style(wxOK|wxICON_INFORMATION),
                GetParent(parent));
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
             MWindow *parent,
             bool /* modal */,
             const char *title,
             bool /* YesDefault */)
{
   return wxMessageBox(message, wxString(title), Style(wxYES_NO|wxICON_QUESTION),
                       GetParent(parent)) == wxYES;
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
#if 0
const char *
MDialog_FileRequester(const char *message,
          MWindow *parent,
          const char *path,
          const char *filename,
          const char *extension,
            const char *wildcard,
            bool save,
            ProfileBase *profile)
#endif
const char *
MDialog_FileRequester(String const & message,
                      MWindow *parent,
                      String path,
                      String filename,
                      String extension,
                      String wildcard,
                      bool save,
                      ProfileBase *profile)
{
   if(! profile)
      profile = mApplication.GetProfile();
   
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

   if(parent == NULL)
      parent = mApplication.TopLevelFrame();
   
   return wxFileSelector(
      message, path, filename, extension, wildcard, 0, parent);
}

int
MDialog_AdbLookupList(ArrayAdbEntries& aEntries,
                      MWindow *parent)
{
   wxArrayString aChoices;
   wxString strName, strEMail;

   size_t nEntryCount = aEntries.Count();
   AdbEntry *pEntry;
   for( size_t nEntry = 0; nEntry < nEntryCount; nEntry++ )
   {
      pEntry = aEntries[nEntry];
      pEntry->GetField(AdbField_FullName, &strName);
      pEntry->GetField(AdbField_EMail, &strEMail);

      aChoices.Add(strName + " <" + strEMail + ">");
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
               _("Please choose an entry:"),
               _("Expansion options"),
               nEntryCount,
               &aChoices[0],
               parent,
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

      Start(READ_APPCONFIG(MC_SPLASHDELAY));
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
   SetBackgroundColour(wxColour("navy"));

   wxLayoutList &ll = GetLayoutList();
   ll.SetFontColour("white", "navy");
   ll.SetEditable(true);

   ll.LineBreak();
   ll.LineBreak();

   ll.SetFontSize(30);
   ll.SetFontWeight(wxBOLD);
   ll.Insert("   Welcome");
   ll.LineBreak();
   ll.Insert("         to");
   ll.LineBreak();
   ll.Insert("         ");
   ll.Insert(new wxLayoutObjectIcon(ICON("M_32x32")));
   ll.Insert("!");
   ll.SetFontWeight(wxNORMAL);
   ll.SetFontSize(10);

   ll.LineBreak();
   ll.LineBreak();
   ll.LineBreak();
   ll.Insert("    Version: " M_VERSION_STRING);
   ll.LineBreak();
   ll.LineBreak();
   ll.LineBreak();
   ll.Insert("  Copyright (c) 1998 by Karsten Ballüder");
   ll.LineBreak();
   ll.LineBreak();
   ll.Insert("          Written by Karsten Ballüder");
   ll.LineBreak();
   ll.Insert("                      and Vadim Zeitlin");
   ll.LineBreak();
   ll.SetEditable(false);

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
                      wxDefaultPosition, wxSize(220, 300),
                      wxDOUBLE_BORDER | wxSTAY_ON_TOP)
{
   wxCHECK_RET( g_pSplashScreen == NULL, "one splash is more than enough" );
   g_pSplashScreen = (MFrame *)this;

   (void)new wxAboutWindow(this, bCloseOnTimeout);

   Centre(wxBOTH);
   Show(TRUE);
}

void
MDialog_AboutDialog( MWindow * /* parent */, bool bCloseOnTimeout)
{
   (void)new wxAboutFrame(bCloseOnTimeout);
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
   wxString    m_choices[Folder_Max];

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
   
   m_choices[Folder_Inbox] = _("INBOX");
   m_choices[Folder_File]  = _("Message box file");
   m_choices[Folder_POP]   = _("POP3");
   m_choices[Folder_IMAP]  = _("IMAP");
   m_choices[Folder_News]  = _("News");
   long xRadio = labelWidth + inputWidth + 20;
   m_FolderTypeRadioBox = new wxRadioBox( this, M_WXID_PEP_RADIO,
                                          _("Folder Type"),
                                          wxPoint(xRadio, 2*LAYOUT_Y_MARGIN),
                                          wxSize(-1,-1),
                                          5, m_choices,
                                          1, wxRA_VERTICAL );

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
   int type = m_FolderTypeRadioBox->GetSelection();

   m_FolderPathTextCtrl->Enable(TRUE);
   if ( type == Folder_POP || type == Folder_IMAP )
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
      if(type == 0)
      {
         m_FolderPathTextCtrl->SetValue("INBOX");
         m_FolderPathTextCtrl->Enable(FALSE);
      }
   }
}

bool
wxPEP_Folder::TransferDataFromWindow(void)
{
   int type;
   m_Profile->writeEntry(MP_FOLDER_TYPE,type = m_FolderTypeRadioBox->GetSelection());
   m_Profile->writeEntry(MP_FOLDER_PATH,m_FolderPathTextCtrl->GetValue());
   m_Profile->writeEntry(MP_UPDATEINTERVAL,atoi(m_UpdateIntervalTextCtrl->GetValue()));
   
   if ( type == Folder_POP || type == Folder_IMAP )
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
   int type = READ_CONFIG(m_Profile, MP_FOLDER_TYPE);
   
   // the trouble is that if INBOX.profile doesn't exist (yet), we get the
   // wrong value here (FIXME: this is not the right solution neither!)
   if ( type == MP_FOLDER_TYPE_D && 
        ((Profile *)m_Profile)->GetProfileName() == "INBOX" ) { // yuck (FIXME)
      type = Folder_Inbox;
   }

   m_FolderTypeRadioBox->SetSelection(type);
   m_FolderPathTextCtrl->SetValue(READ_CONFIG(m_Profile,MP_FOLDER_PATH));
   m_UpdateIntervalTextCtrl->SetValue(strutil_ltoa(READ_CONFIG(m_Profile,MP_UPDATEINTERVAL)));
   if ( type == Folder_POP || type == Folder_IMAP )
   {
      m_UserIdTextCtrl->SetValue(READ_CONFIG(m_Profile,MP_POP_LOGIN));
      m_PasswordTextCtrl->SetValue(READ_CONFIG(m_Profile,MP_POP_PASSWORD));
   }
   UpdateUI();

   return true;
}

void
MDialog_FolderProfile(MWindow *parent, ProfileBase *profile)
{
   // show a modal dialog
   wxPEP_Folder dlg(profile, NULL);
   dlg.Centre(wxCENTER_FRAME | wxBOTH);
   dlg.ShowModal();
}


void
MDialog_FolderCreate(MWindow *parent)
{
   wxString name = "NewFolder";
   
   if(! MInputBox(&name,
                  _("M - New Folder"),
                  _("Symbolic name for the new folder"),
                  parent,
                  "NewFolderDialog",
                  "NewFolder"))
      return;

   ProfileBase *profile = new Profile(name, NULL);

   MDialog_FolderProfile(parent, profile);

   delete profile;
}
