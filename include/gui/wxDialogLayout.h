///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   gui/wxDialogLayout.h - helper functions for laying out the
//              dialog controls in a consistent manner
// Purpose:
// Author:      Vadim Zeitlin
// Modified by:
// Created:     23.12.98
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _GUI_WXDIALOGLAYOUT_H
#define _GUI_WXDIALOGLAYOUT_H

// -----------------------------------------------------------------------------
// headers we have to include
// -----------------------------------------------------------------------------

#ifndef USE_PCH
#  include <wx/textctrl.h>
#endif // USE_PCH

#include <wx/persctrl.h>   // needed for wxPNotebook

#include "gui/wxMDialogs.h"
#include "MEvent.h"

// use wxListbook instead of wxNotebook if available
#if wxUSE_LISTBOOK
   #define wxNotebook wxListbook
   #define wxPNotebook wxPListbook
   #define wxNotebookEvent wxListbookEvent

   #define wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED wxEVT_COMMAND_LISTBOOK_PAGE_CHANGED
   #define wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGING wxEVT_COMMAND_LISTBOOK_PAGE_CHANGING

   #undef EVT_NOTEBOOK_PAGE_CHANGED
   #define EVT_NOTEBOOK_PAGE_CHANGED EVT_LISTBOOK_PAGE_CHANGED
   #undef EVT_NOTEBOOK_PAGE_CHANGING
   #define EVT_NOTEBOOK_PAGE_CHANGING EVT_LISTBOOK_PAGE_CHANGING

   #undef wxNB_TOP
   #define wxNB_TOP wxLB_TOP
   #undef wxNB_BOTTOM
   #define wxNB_BOTTOM wxLB_BOTTOM
   #undef wxNB_LEFT
   #define wxNB_LEFT wxLB_LEFT
   #undef wxNB_RIGHT
   #define wxNB_RIGHT wxLB_RIGHT
#endif // wxUSE_LISTBOOK

// -----------------------------------------------------------------------------
// forward declarations
// -----------------------------------------------------------------------------

class WXDLLEXPORT wxFrame;
class WXDLLEXPORT wxCheckBox;
class WXDLLEXPORT wxCheckListBox;
class WXDLLEXPORT wxChoice;
class WXDLLEXPORT wxControl;
class WXDLLEXPORT wxComboBox;
class WXDLLEXPORT wxListBox;
class WXDLLEXPORT wxRadioBox;
class WXDLLEXPORT wxScrolledWindow;
class WXDLLEXPORT wxStaticBitmap;
class WXDLLEXPORT wxStaticBox;
class WXDLLEXPORT wxStaticText;

class /* WXDLLEXPORT */ wxColorBrowseButton;
class /* WXDLLEXPORT */ wxDirBrowseButton;
class /* WXDLLEXPORT */ wxFileBrowseButton;
class /* WXDLLEXPORT */ wxFileOrDirBrowseButton;
class /* WXDLLEXPORT */ wxFontBrowseButton;
class /* WXDLLEXPORT */ wxFolderBrowseButton;
class /* WXDLLEXPORT */ wxIconBrowseButton;
class /* WXDLLEXPORT */ wxTextBrowseButton;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

/// flags for CreateFileEntry()
enum
{
   FileEntry_Open = 0,
   FileEntry_Save = 1,
   FileEntry_ExistingOnly = 2       // makes sense with Open only
};

/// the kinds of browse buttons created by CreateEntryWithButton()
enum BtnKind
{
   // the following 3 values must be consecutive, GetBtnType() relies on it
   FileBtn,             // open an existing file
   FileNewBtn,          // choose any file (might not exist)
   FileSaveBtn,         // save to a file

   // the following 3 values must be consecutive, GetBtnType() relies on it
   FileOrDirBtn,        // open an existing file or directory
   FileOrDirNewBtn,     // choose any file or directory
   FileOrDirSaveBtn,    // save to a file or directory

   DirBtn,              // choose a directory
   ColorBtn,            // choose a colour
   FontBtn,             // choose a font
   FolderBtn            // choose a folder
};

// =============================================================================
// GUI classes declared in this file
// =============================================================================

// ----------------------------------------------------------------------------
// a stand-in for the wxPDialog class which is not written yet...
// ----------------------------------------------------------------------------

class wxPDialog : public wxMDialog
{
public:
   // ctor restores position/size
   wxPDialog(const wxString& profileKey,
             wxWindow *parent,
             const wxString& title,
             long style = 0);

   // dtor saves position/size
   virtual ~wxPDialog();

   virtual bool LastSizeRestored() const { return m_didRestoreSize; }

private:
   wxString m_profileKey;
   bool     m_didRestoreSize;

   DECLARE_NO_COPY_CLASS(wxPDialog)
};

// ----------------------------------------------------------------------------
// a class containing common code for dialog layout
// ----------------------------------------------------------------------------

class wxManuallyLaidOutDialog : public wxPDialog
{
public:
   // this class should have default ctor for the derived class convenience,
   // although this makes absolutely no sense for us
   wxManuallyLaidOutDialog(wxWindow *parent = NULL,
                           const wxString& title = _T(""),
                           const wxString& profileKey = _T(""));

   // get the minimal size previously set by SetDefaultSize()
   virtual const wxSize& GetMinimalSize() const { return m_sizeMin; }

   // get the buttons size
   virtual inline wxSize GetButtonSize() const { return wxSize(wBtn, hBtn); }

   // event handlers
   virtual void OnHelp(wxCommandEvent & /*ev*/);

protected:
   // the constants for CreateStdButtonsAndBox() second argument
   enum
   {
      StdBtn_Default = 0,
      StdBtn_NoBox = 1,
      StdBtn_NoCancel = 2
   };

   // create Ok and Cancel buttons and a static box around all other ctrls
   // (if noBox is TRUE, the returned value is NULL and wxStaticBox is not
   // created). If helpId != -1, add a Help button.
   virtual wxStaticBox *CreateStdButtonsAndBox(const wxString& boxTitle,
                                               int flags = StdBtn_Default,
                                               int helpId = -1);

   // create just the buttons
   void CreateStdButtons() { (void)CreateStdButtonsAndBox(_T(""), TRUE); }

   // set the diaqlog size if it wasn't restored from profile
   virtual void SetDefaultSize(int width, int height,
                               bool setAsMinimalSizeToo = TRUE);

   // these variables are set in the ctor and are the basic measurement unites
   // for us (we allow direct access to them for derived classes for
   // compatibility with existing code)
   int hBtn, wBtn;

private:
   // the minimal size (only if SetDefaultSize() was called)
   wxSize m_sizeMin;

   // the helpId or -1
   int    m_helpId;

   DECLARE_DYNAMIC_CLASS_NO_COPY(wxManuallyLaidOutDialog)
   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// a dialog which contains a notebook with the standard Ok/Cancel/Apply buttons
// below it and, optionally, some extra controls above/below the notebook. For
// example, options dialog and folder creation dialogs in M derive from this
// class.
//
// this class sends MEventId_OptionsChange event when either of Ok/Cancel/Apply
// buttons is pressed. To do it, it needs a profile pointer which is retrieved
// via virtual GetProfile() function - and it is only used for this purpose.
// ----------------------------------------------------------------------------

class wxOptionsEditDialog : public wxManuallyLaidOutDialog
{
public:
   // ctor
   wxOptionsEditDialog(wxFrame *parent,
                       const wxString& title,
                       const wxString& profileKey = _T(""));

   // populate the dialog
      // create the controls above the main notebook, return the last control
      // created (in the top-to-bottom order)
   virtual wxControl *CreateControlsAbove(wxPanel * /* panel */) { return NULL; }
      // create the notebook itself (assign the pointer to m_notebook)
   virtual void CreateNotebook(wxPanel *panel) = 0;
      // create the controls below the main notebook
   virtual void CreateControlsBelow(wxPanel * /* panel */) { }
      // create the notebook and the standard Ok/Cancel/Apply buttons, calls
      // CreateControlsAbove/Below() and CreateNotebook() which may be
      // overriden in the derived classes
   void CreateAllControls();

   // function called when the user chooses Apply or Ok button and something
   // has really changed in the dialog: return TRUE from it to allow change
   // (and close the dialog), FALSE to forbid it and keep the dialog opened.
   virtual bool OnSettingsChange();

   // set the page (if -1 not changed)
   void SetNotebookPage(int page)
   {
      if ( page != -1 )
         m_notebook->SetSelection(page);
   }

   // notifications from the notebook pages
      // something changed, set the dirty flag (must be called to enable the
      // Apply button)
   virtual void SetDirty() { m_bDirty = TRUE; EnableButtons(TRUE); }
      // something important change
   virtual void SetDoTest() { SetDirty(); m_bTest = TRUE; }
      // some setting changed, but won't take effect until restart
   virtual void SetGiveRestartWarning() { m_bRestartWarning = TRUE; }

   // get/set the dialog data
   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

   // callbacks
   void OnHelp(wxCommandEvent &event);
   void OnOK(wxCommandEvent& event);
   void OnApply(wxCommandEvent& event);
   void OnCancel(wxCommandEvent& event);

   // unimplemented default ctor for DECLARE_DYNAMIC_CLASS
   wxOptionsEditDialog() { wxFAIL_MSG(_T("unaccessible")); }

   // disable or reenable Ok and Apply buttons
   virtual void EnableButtons(bool enable);

protected:
   // clear the dirty flag
   virtual void ResetDirty()
   {
      m_bTest =
      m_bRestartWarning =
      m_bDirty = FALSE;
      m_btnApply->Enable(FALSE);
   }

   // the helper for the handlers of Apply/Ok buttons, returns TRUE if the
   // changes were accepted
   bool DoApply();

   wxPNotebook *m_notebook;

   // get the profile for event sending
   virtual Profile *GetProfile() const = 0;

private:
   // send a notification event about options change using m_lastBtn value
   void SendOptionsChangeEvent();

   wxButton *m_btnHelp,
            *m_btnOk,
            *m_btnApply;

   // this profile is first retrieved using GetProfile(), but it's only done
   // once and then it is reused so that [Cancel] will call Discard() on the
   // same profile as [Apply] called Suspend() on and not on some other profile
   // with the same path
   Profile *m_profileForButtons;

   // flags
   bool m_bDirty,           // something changed
        m_bTest,            // test new settings?
        m_bRestartWarning;  // changes will take effect after restart


   // Ok/Cancel/Apply depending on the last button pressed
   MEventOptionsChangeData::ChangeKind m_lastBtn;

   DECLARE_DYNAMIC_CLASS_NO_COPY(wxOptionsEditDialog)
   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// a base class for all dialogs which are used to edit the profile settings: it
// adds the member profile variable and the "hasChanges" flag. As it derives
// from wxPDialog, it saves and restores its position.
// ----------------------------------------------------------------------------

class wxProfileSettingsEditDialog : public wxManuallyLaidOutDialog
{
public:
   wxProfileSettingsEditDialog(Profile *profile,
                                       const wxString& profileKey,
                                       wxWindow *parent,
                                       const wxString& title);

   virtual ~wxProfileSettingsEditDialog();
   virtual Profile *GetProfile() const { return m_profile; }

   virtual bool HasChanges() const { return m_hasChanges; }
   virtual void MarkDirty() { m_hasChanges = TRUE; }

protected:
   Profile *m_profile;
   bool         m_hasChanges;

   DECLARE_NO_COPY_CLASS(wxProfileSettingsEditDialog)
};

// ----------------------------------------------------------------------------
// this class eats all command events which shouldn't propagate upwards to the
// parent: this is useful when we're shown from the options dialog because the
// option pages there suppose that all command events can only originate from
// their controls.
// ----------------------------------------------------------------------------

class wxOptionsPageSubdialog : public wxProfileSettingsEditDialog
{
public:
   wxOptionsPageSubdialog(Profile *profile,
                                  wxWindow *parent,
                                  const wxString& label,
                                  const wxString& windowName);

   virtual void OnChange(wxCommandEvent& event);

private:
   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxOptionsPageSubdialog)
};

// ----------------------------------------------------------------------------
// A panel class which has some useful functions for control creation and which
// supports (vertical only so far) scrolling which makes it useful for the large
// dialogs.
// ----------------------------------------------------------------------------

class wxEnhancedPanel : public wxPanel
{
public:
   wxEnhancedPanel(wxWindow *parent, bool enableScrolling = TRUE);

   // all these functions create the corresponding control and position it
   // below the "last" which may be NULL in which case the new control is put
   // just below the panel top.
   //
   // widthMax parameter is the max width of the labels and is used to align
   // labels/text entries, it can be found with GetMaxWidth() function below.
   // -----------------------------------------------------------------------

      // a listbox with 3 standard buttons: Add, Modify and Delete
      // (TODO: this is probably not flexible enough)
   wxListBox  *CreateListbox(const wxChar *label, wxControl *last);

      // a checkbox with a label
   wxCheckBox *CreateCheckBox(const wxChar *label,
                              long widthMax,
                              wxControl *last);

      // a label with three choices: No, Ask, Yes
   wxRadioBox *CreateActionChoice(const wxChar *label,
                                  long widthMax,
                                  wxControl *last,
                                  wxCoord nRightMargin = 0);

      // a radiobox, the entries are taken from the label string which is
      // composed as: "LABEL:entry1:entry2:entry3:...."
   wxRadioBox *CreateRadioBox(const wxChar *label,
                              long widthMax,
                              wxControl *last,
                              wxCoord nRightMargin = 0);

      // nRightMargin is the distance to the right edge of the panel to leave
      // (0 means default)
   wxTextCtrl *CreateTextWithLabel(const wxChar *label,
                                   long widthMax,
                                   wxControl *last,
                                   wxCoord nRightMargin = 0,
                                   int style = wxTE_LEFT);

      // create a simple static text control
   wxStaticText *CreateMessage(const wxChar *label, wxControl *last);

      // a combobox, the entries are taken from the label string which is
      // composed as: "LABEL:entry1:entry2:entry3:...."
   wxComboBox *CreateComboBox(const wxChar *label,
                              long widthMax,
                              wxControl *last,
                              wxCoord nRightMargin = 0)
   {
      return (wxComboBox *)CreateComboBoxOrChoice(TRUE, label, widthMax,
                                                  last, nRightMargin);
   }

      // a choice control, the entries are taken from the label string which is
      // composed as: "LABEL:entry1:entry2:entry3:...."
   wxChoice *CreateChoice(const wxChar *label,
                          long widthMax,
                          wxControl *last,
                          wxCoord nRightMargin = 0)
   {
      return (wxChoice *)CreateComboBoxOrChoice(FALSE, label, widthMax,
                                                last, nRightMargin);
   }

      // a button: the label string is "label:id" where id is the id for the
      // button
   wxButton *CreateButton(const wxString& label, wxControl *last);

      // a button: the label string is "label:id" where id is the id for the
      // button
   wxXFaceButton *CreateXFaceButton(const wxString& label, long
                                    widthMax, wxControl *last);

      // if ppButton != NULL, it's filled with the pointer to the ">>" browse
      // button created by this function (it will be a wxFileBrowseButton)
   wxTextCtrl *CreateFileEntry(const wxChar *label,
                               long widthMax,
                               wxControl *last,
                               wxFileBrowseButton **ppButton = NULL,
                               bool open = TRUE,
                               bool existingOnly = TRUE)
   {
      return CreateEntryWithButton(label, widthMax, last,
                                   GetBtnType(FileBtn, open, existingOnly),
                                   (wxTextBrowseButton **)ppButton);
   }
      // if ppButton != NULL, it's filled with the pointer to the ">>" browse
      // button created by this function (it will be wxFileOrDirBrowseButton)
   wxTextCtrl *CreateFileOrDirEntry(const wxChar *label,
                                    long widthMax,
                                    wxControl *last,
                                    wxFileOrDirBrowseButton **ppButton = NULL,
                                    bool open = TRUE,
                                    bool existingOnly = TRUE)
   {
      return CreateEntryWithButton(label, widthMax, last,
                                   GetBtnType(FileOrDirBtn, open, existingOnly),
                                   (wxTextBrowseButton **)ppButton);
   }

      // create an entry with a button to browse for the directories
   wxTextCtrl *CreateDirEntry(const wxChar *label,
                              long widthMax,
                              wxControl *last,
                              wxDirBrowseButton **ppButton = NULL)
   {
      return CreateEntryWithButton(label, widthMax, last,
                                   DirBtn,
                                   (wxTextBrowseButton **)ppButton);
   }
      // another entry with a browse button
   wxColorBrowseButton *CreateColorEntry(const wxChar *label,
                                         long widthMax,
                                         wxControl *last);

      // creates a static bitmap with a label and a browse button
   wxStaticBitmap *CreateIconEntry(const wxChar *label,
                                   long widthMax,
                                   wxControl *last,
                                   wxIconBrowseButton *btnIcon);

      // create a text control with a browse button allowing to browse for
      // folders
   wxTextCtrl *CreateFolderEntry(const wxChar *label,
                                 long widthMax,
                                 wxControl *last,
                                 wxFolderBrowseButton **ppButton = NULL)
   {
      return CreateEntryWithButton(label, widthMax, last,
                                   FolderBtn,
                                   (wxTextBrowseButton **)ppButton);
   }

      // create a text control with a browse button allowing to browse for
      // fonts
   wxTextCtrl *CreateFontEntry(const wxChar *label,
                               long widthMax,
                               wxControl *last,
                               wxFontBrowseButton **ppButton = NULL)
   {
      return CreateEntryWithButton(label, widthMax, last,
                                   FontBtn,
                                   (wxTextBrowseButton **)ppButton);
   }

   // UpdateUI helpers: enable disable several controls at once
   //
   // NB: these functions assume that control ids are consecutif,
   //     i.e. the label always precedes the text ctrl &c
   // -----------------------------------------------------------

      // enable/disable the text control and its label
   void EnableTextWithLabel(wxTextCtrl *control, bool enable);

      // enable/disable the text control with label and button
   void EnableTextWithButton(wxTextCtrl *control, bool enable);

      // enable/disable the colour browse button and its text with label
   void EnableColourBrowseButton(wxColorBrowseButton *btn, bool bEnable);

      // enable/disable the combobox and its label
   void EnableComboBox(wxComboBox *control, bool enable)
      { EnableControlWithLabel(control, enable); }

      // works for any control preceded by the label
   void EnableControlWithLabel(wxControl *control, bool enable);

      // enable/disable listbox and its buttons
   void EnableListBox(wxListBox *control, bool enable);

      // get the label corresponding to the given control or NULL
   wxStaticText *GetLabelForControl(wxControl *control);


   // get the canvas - all the controls should be created as children of this
   // canvas, not of the page itself
   wxWindow *GetCanvas() const
   {
      // first cast to wxWindow is needed if wxScrolledWindow is only forward
      // declared and not visible from here
      return m_canvas ? (wxWindow *)m_canvas : (wxWindow *)this; // const_cast
   }

   // forces a call to Layout() to get everything nicely laid out
   virtual bool Layout() { return DoLayout(GetClientSize()); }

   // show or hide the vertical scrollbar depending on whether there is enough
   // place or not
   void RefreshScrollbar(const wxSize& size);

private:
   // called from CreateXXX() functions to set up the top constraint which is
   // either just below the "last", or below the page top (with some
   // additional margin possibly specified by the 3rd argument)
   void SetTopConstraint(wxLayoutConstraints *c,
                         wxControl *last,
                         size_t extraSpace = 0);

   // return the right btn type for the given "base" type and parameters
   static BtnKind GetBtnType(BtnKind base, bool open, bool existing)
   {
      int ofs;
      if ( open )
      {
         ofs = existing ? 0 : 1;
      }
      else
      {
         ofs = 2;
      }

      return (BtnKind)(base + ofs);
   }

   wxTextCtrl *CreateEntryWithButton(const wxChar *label,
                                     long widthMax,
                                     wxControl *last,
                                     BtnKind kind,
                                     wxTextBrowseButton **ppButton = NULL);

   // create a wxComboBox or wxChoice
   wxControl *CreateComboBoxOrChoice(bool createCombobox,
                                     const wxChar *label,
                                     long widthMax,
                                     wxControl *last,
                                     wxCoord nRightMargin = 0);

   // event handlers
   void OnSize(wxSizeEvent& event);

   // actually layout the children *and* refresh the scrollbar too
   bool DoLayout(const wxSize& size);

   // the canvas on which all controls are created
   wxScrolledWindow *m_canvas;

   // the minimal size of the page when we still don't need scrollbars: to use
   // this, we should live in wxManuallyLaidOutDialog whose
   // SetDefaultSize()method had been called (then it will be the same size)
   wxSize m_sizeMin;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxEnhancedPanel)
};

// ----------------------------------------------------------------------------
// a base class for notebook pages which provides some handy functions for
// laying out the controls inside the page. It is well suited for the pages
// showing the controls in a row (text fields with labels for example).
// ----------------------------------------------------------------------------

class wxNotebookPageBase : public wxEnhancedPanel
{
public:
   wxNotebookPageBase(wxNotebook *parent) : wxEnhancedPanel(parent) { }

protected:
   // callbacks which will set the parent's dirty flag whenever
   // something changes
   // ---------------------------------------------------------
   void OnChange(wxCommandEvent& event);

private:
   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxNotebookPageBase)
};

// ----------------------------------------------------------------------------
// a notebook with images
// ----------------------------------------------------------------------------

class wxNotebookWithImages : public wxPNotebook
{
public:
   // configPath is used to store the last active page, aszImages is the NULL
   // terminated array of the icon names (image size should be 32x32)
   wxNotebookWithImages(const wxString& configPath,
                        wxWindow *parent,
                        const wxChar *aszImages[]);

   virtual ~wxNotebookWithImages();

   // should we show icons in the notebooks (uses MP_TBARIMAGES)?
   static bool ShouldShowIcons();

private:
   DECLARE_NO_COPY_CLASS(wxNotebookWithImages)
};

// =============================================================================
// helper functions to create/layout controls
// =============================================================================

// determine the maximal width of the given strings (win is the window to use
// for font calculations)
extern long GetMaxLabelWidth(const wxArrayString& labels, wxWindow *win);

// all these functions correspond to the wxEnhancedPanel methods except that
// they take an additional parent parameter
extern wxStaticText *CreateMessage(wxWindow *parent,
                                   const wxChar *label,
                                   wxControl *last);

extern wxTextCtrl *CreateTextWithLabel(wxWindow *parent,
                                       const wxChar *label,
                                       long widthMax,
                                       wxControl *last,
                                       wxCoord nRightMargin = 0,
                                       int style = wxTE_LEFT);

extern wxCheckBox *CreateCheckBox(wxWindow *parent,
                                  const wxChar *label,
                                  long widthMax,
                                  wxControl *last,
                                  wxCoord nRightMargin = 0);

extern wxRadioBox *CreateRadioBox(wxWindow *parent,
                                  const wxChar *label,
                                  long widthMax,
                                  wxControl *last,
                                  wxCoord nRightMargin = 0);

extern wxTextCtrl *CreateEntryWithButton(wxWindow *parent,
                                         const wxChar *label,
                                         long widthMax,
                                         wxControl *last,
                                         wxCoord nRightMargin = 0,
                                         BtnKind kind = FileBtn,
                                         wxTextBrowseButton **ppButton = NULL);

extern wxTextCtrl *CreateFileEntry(wxWindow *parent,
                                   const wxChar *label,
                                   long widthMax,
                                   wxControl *last,
                                   wxCoord nRightMargin = 0,
                                   wxFileBrowseButton **ppButton = NULL,
                                   int flags = FileEntry_Open |
                                               FileEntry_ExistingOnly);

extern void EnableTextWithLabel(wxWindow *parent,
                                wxTextCtrl *control,
                                bool enable);

#endif // _GUI_WXDIALOGLAYOUT_H
