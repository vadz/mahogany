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

#include "Mdefaults.h"

#include "MApplication.h"
#include "gui/wxMApp.h"

#include "MDialogs.h"
#include "gui/wxlwindow.h"

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

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

  MTextInputDialog dlg(GetParent(parent), *pstr, strCaption, strPrompt);
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
   wxMessageBox(msg, title, Style(wxOK|wxICON_EXCLAMATION), GetParent(parent));
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

   MDialog_ErrorMessage(msg.c_str(), parent, title, modal);
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

   MDialog_ErrorMessage(message,parent,title,true);
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
   wxMessageBox(msg, title, Style(wxOK|wxICON_INFORMATION), GetParent(parent));
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
   return wxMessageBox(message, title, Style(wxYES_NO|wxICON_QUESTION),
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
   return wxMessageBox(message, _("Decision"), Style(wxYES_NO|wxICON_QUESTION),
                       GetParent(parent)) == wxYES;
}

#if 0
AdbEntry *
MDialog_AdbLookupList(AdbExpandListType *adblist, MWindow *parent)
{
   AdbExpandListType::iterator i;

   int
      idx = 0,
      val,
      n = adblist->size();
   String
      tmp;
   char
      ** choices = new char *[n];
   AdbEntry
      ** entries = new AdbEntry *[n],
      *result;

   if(parent == NULL)
      parent = mApplication.TopLevelFrame();
   
   
   for(i = adblist->begin(); i != adblist->end(); i++, idx++)
   {
      tmp =(*i)->formattedName + String(" <")
	 + (*i)->email.preferred.c_str() + String(">");
      choices[idx] = strutil_strdup(tmp);
      entries[idx] = (*i);
   }
   int w,h;
   if(parent)
   {
      parent->GetClientSize(&w,&h);
      w = (int) (w*0.8);
      h = (int) (h*0.8);
   }
   else
   {
      w = 200;
      h = 250;
   }  
   val = wxGetSingleChoiceIndex(
      _("Please choose an entry:"),
      _("Expansion options"),
      n, (char **)choices, parent,
      -1,-1, // x,y
      TRUE,//centre
      w,h);

   if(val == -1)
      result = NULL;
   else
      result = entries[val];

   for(idx = 0; idx < n; idx++)
      delete [] choices[idx];
   
   delete [] choices;
   delete [] entries;
  
   return result;
}
#endif
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
