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
#endif

#include   "Mdefaults.h"
#include   "MApplication.h"
#include   "gui/wxMApp.h"
#include   "gui/wxMIds.h"

#include "MDialogs.h"
#include "gui/wxlwindow.h"

#include   <wx/dynarray.h>
#include   <wx/radiobox.h>

#include "adb/AdbEntry.h"
#include "adb/AdbBook.h"

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

  static const int LAYOUT_X_MARGIN;
  static const int LAYOUT_Y_MARGIN;

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

const int MTextInputDialog::LAYOUT_X_MARGIN = 5;
const int MTextInputDialog::LAYOUT_Y_MARGIN = 5;

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

  uint widthText = 3*widthLabel,
       heightText = TEXT_HEIGHT_FROM_LABEL(heightLabel);
  uint heightBtn = TEXT_HEIGHT_FROM_LABEL(heightLabel),
       widthBtn = BUTTON_WIDTH_FROM_HEIGHT(heightBtn);
  uint widthDlg = widthLabel + widthText + 3*LAYOUT_X_MARGIN,
       heightDlg = heightText + heightBtn + 3*LAYOUT_Y_MARGIN;

  uint x = LAYOUT_X_MARGIN,
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
                     bool modal)
{
   wxMessageBox(msg, wxString("M -" + wxString(title)), Style(wxOK|wxICON_EXCLAMATION), GetParent(parent));
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

   MDialog_ErrorMessage(msg.c_str(), parent, wxString("M -" + wxString(title)), modal);
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

   MDialog_ErrorMessage(message,parent,wxString("M -" + wxString(title)),true);
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
                bool modal)
{
   wxMessageBox(msg, wxString("M -" + wxString(title)), Style(wxOK|wxICON_INFORMATION), GetParent(parent));
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
             bool modal,
             const char *title,
             bool YesDefault)
{
   return wxMessageBox(message, wxString("M -" + wxString(title)), Style(wxYES_NO|wxICON_QUESTION),
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

// simple "Yes/No" dialog
bool
MDialog_YesNoDialog(String const &message,
                    MWindow *parent,
                    bool modal,
                    bool YesDefault)
{
   return wxMessageBox(message, _("M - Decision"), Style(wxYES_NO|wxICON_QUESTION),
                       GetParent(parent)) == wxYES;
}

int
MDialog_AdbLookupList(ArrayAdbEntries& aEntries,
                      MWindow *parent)
{
   wxArrayString aChoices;
   wxString strName, strEMail;

   uint nEntryCount = aEntries.Count();
   AdbEntry *pEntry;
   for( uint nEntry = 0; nEntry < nEntryCount; nEntry++ )
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

// simple AboutDialog to be displayed at startup
void
MDialog_AboutDialog( MWindow *parent)
{

   wxFrame *frame = new wxFrame();

   frame->Create(parent,-1, _("Welcome"));

   wxLayoutWindow *lw = new wxLayoutWindow(frame);

   wxLayoutList &ll = lw->GetLayoutList();
   
   frame->Show(FALSE);

   ll.SetEditable(true);
   ll.Insert("Welcome to M!");
   ll.LineBreak();
   
   ll.SetEditable(false);

   frame->Show(TRUE);
   frame->Fit();
}



/** A base class for dialog panels */
class wxProfileEditPanel : public wxPanel
{
public:
   wxProfileEditPanel(wxWindow *parent)
      : wxPanel(parent, -1, wxPoint(0,0), wxSize(400, 400))
      {}
   /// transfer settings from panel to profile
   virtual void    UpdateProfile(void) = 0;
   /// transfer settings from profile to panel
   virtual void    UpdatePanel(void) = 0;
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
   /// transfer settings from panel to profile
   void    UpdateProfile(void);
   /// transfer settings from profile to panel
   void    UpdatePanel(void);
   /// update controls
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
   wxButton   *m_CancelButton, *m_OkButton, *m_UndoButton;
   wxString choices[5];

   DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(wxPEP_Folder, wxPanel)
//  EVT_CHECKBOX(-1, wxAdbEMailPage::OnCheckBox)

  EVT_BUTTON(M_WXID_PEP_OK, wxPEP_Folder::OnEvent)
  EVT_BUTTON(M_WXID_PEP_UNDO, wxPEP_Folder::OnEvent)
  EVT_BUTTON(M_WXID_PEP_CANCEL, wxPEP_Folder::OnEvent)
  EVT_RADIOBOX(M_WXID_PEP_RADIO, wxPEP_Folder::OnEvent)
END_EVENT_TABLE()


#define   MkTextCtrl(control,label,id) \
  (void) new wxStaticText(this, id, _(label), pos, wxSize(labelWidth,labelHeight)); \
  control = new wxTextCtrl(this, id, "",wxPoint(pos.x+labelWidth,pos.y),wxSize(inputWidth,-1)); \
  pos.y += labelHeight;\

#define  MkButton(control,label,id) \
  control = new wxButton(this, id, _(label), pos);

void
wxPEP_Folder::OnEvent(wxCommandEvent& event)
{
   switch(event.GetId())
   {
   case M_WXID_PEP_OK:
      UpdateProfile();
   case M_WXID_PEP_CANCEL:
      m_Parent->Close();
      break;
   case M_WXID_PEP_UNDO:
      UpdatePanel();
      break;
   case M_WXID_PEP_RADIO:
      UpdateUI();
      break;
   default:
      event.Skip();
   }
}

wxPEP_Folder::wxPEP_Folder(ProfileBase *profile, wxWindow *parent)
   : wxProfileEditPanel(parent)
{
   m_Profile = profile;
   m_Parent = parent;
   wxASSERT(m_Profile);
   wxASSERT(m_Parent);

   wxPoint  pos = wxPoint(10,10);
   long labelWidth, labelHeight;
   long inputWidth;
   
   // first determine the longest button caption
   wxClientDC dc(this);
   dc.SetFont(wxSystemSettings::GetSystemFont(wxSYS_DEFAULT_GUI_FONT));
   dc.GetTextExtent(_("Update interval in seconds"), &labelWidth, &labelHeight);
   labelHeight *= 2;
   labelWidth += 10;
   dc.GetTextExtent("/home/karsten/MailFolders/foldername", &inputWidth, NULL);
   inputWidth += 10;
   
   MkTextCtrl(m_FolderPathTextCtrl, "Path or name of folder",-1);
   MkTextCtrl(m_UpdateIntervalTextCtrl, "Update interval in seconds",-1);
   MkTextCtrl(m_UserIdTextCtrl, "User ID",-1);
   MkTextCtrl(m_PasswordTextCtrl, "Password",-1);
   
   choices[0] = _("INBOX");
   choices[1] = _("Message box file");
   choices[2] = _("POP3");
   choices[3] = _("IMAP");
   choices[4] = _("NNTP/News");
   m_FolderTypeRadioBox = new wxRadioBox( this, M_WXID_PEP_RADIO, _("Folder Type"),
                                          wxPoint(labelWidth+inputWidth+20,10),
                                          wxSize(-1,-1),
                                          5, choices,
                                          1, wxRA_VERTICAL );

   pos.y +=10;
   int x,y;
   MkButton(m_OkButton,"Ok",M_WXID_PEP_OK);
   m_OkButton->GetSize(&x,&y);
   pos.x += 2*x;
   MkButton(m_UndoButton,"Undo",M_WXID_PEP_UNDO);
   pos.x += 2*x;
   MkButton(m_CancelButton,"Cancel",M_WXID_PEP_CANCEL);

   UpdatePanel();
   Fit();
}

void
wxPEP_Folder::UpdateUI(void)
{
   int type = m_FolderTypeRadioBox->GetSelection();
   
   if(type == 2 || type == 3) // we need defines for that: pop/imap
   {
      m_UserIdTextCtrl->Enable(TRUE);
      m_PasswordTextCtrl->Enable(TRUE);
   }
   else
   {
      m_UserIdTextCtrl->Enable(FALSE);
      m_PasswordTextCtrl->Enable(FALSE);
   }
}

void
wxPEP_Folder::UpdateProfile(void)
{
   int type;
   m_Profile->writeEntry(MP_FOLDER_TYPE,type = m_FolderTypeRadioBox->GetSelection());
   m_Profile->writeEntry(MP_FOLDER_PATH,m_FolderPathTextCtrl->GetValue());
   m_Profile->writeEntry(MP_UPDATEINTERVAL,atoi(m_UpdateIntervalTextCtrl->GetValue()));
   
   if(type == 2 || type == 3) // we need defines for that: pop/imap
   {
      m_Profile->writeEntry(MP_POP_LOGIN,m_UserIdTextCtrl->GetValue());
      m_Profile->writeEntry(MP_POP_PASSWORD,m_PasswordTextCtrl->GetValue());
   }
}

void
wxPEP_Folder::UpdatePanel(void)
{
   int type = READ_CONFIG(m_Profile,MP_FOLDER_TYPE);
   
   m_FolderTypeRadioBox->SetSelection(type);
   m_FolderPathTextCtrl->SetValue(READ_CONFIG(m_Profile,MP_FOLDER_PATH));
   m_UpdateIntervalTextCtrl->SetValue(strutil_ltoa(READ_CONFIG(m_Profile,MP_UPDATEINTERVAL)));
   if(type == 2 || type == 3) // we need defines for that: pop/imap
   {
      m_UserIdTextCtrl->SetValue(READ_CONFIG(m_Profile,MP_POP_LOGIN));
      m_PasswordTextCtrl->SetValue(READ_CONFIG(m_Profile,MP_POP_PASSWORD));
   }
   UpdateUI();
}



void
MDialog_FolderProfile(MWindow *parent, ProfileBase *profile)
{
   // for now, open a frame and display the panel, return immediately
   wxMFrame *frame = new wxMFrame("FolderProfileFrame",parent);
   (void) new wxPEP_Folder(profile,frame);
   frame->SetTitle(_("M - Folder Profile Settings"));
   frame->Fit();
   frame->Show(TRUE);
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

   wxDialog *dlg = new wxDialog(parent, -1, _("M - Folder Profile Settings"),
                             wxDefaultPosition,
                             wxDefaultSize,
                             wxDIALOG_MODAL);

   (void) new wxPEP_Folder(profile,dlg);
   dlg->Fit();
   dlg->ShowModal();
   delete profile;
}
