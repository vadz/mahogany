// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //////
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
// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //////

#ifndef _GUI_WXDIALOGLAYOUT_H
#define _GUI_WXDIALOGLAYOUT_H

// -----------------------------------------------------------------------------
// forward declarations
// -----------------------------------------------------------------------------
class wxPNotebook;
class wxFrame;
class wxControl;
class wxListBox;
class wxCheckBox;
class wxFileBrowseButton;
class wxStaticText;
class wxRadioBox;
class wxComboBox;
// -----------------------------------------------------------------------------
// classes
// -----------------------------------------------------------------------------

// a notebook with images
class wxNotebookWithImages : public wxPNotebook
{
public:
   // configPath is used to store the last active page, aszImages is the NULL
   // terminated array of the icon names (image size should be 32x32)
   wxNotebookWithImages(const wxString& configPath,
                        wxWindow *parent,
                        const char *aszImages[]);

   virtual ~wxNotebookWithImages();
};


// a dialog which contains a notebook with the standard Ok/Cancel/Apply buttons
// below it and, optionally, some extra controls above/below the notebook. For
// example, options dialog and folder creation dialogs in M derive from this
// class.
class wxNotebookDialog : public wxDialog
{
public:
   // ctor
   wxNotebookDialog(wxFrame *parent, const wxString& title);

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
   virtual bool OnSettingsChange() { return TRUE; }

   // set the page (if -1 not changed)
   void SetNotebookPage(int page)
   {
      if ( page != -1 )
         m_notebook->SetSelection(page);
   }

   // something changed, set the dirty flag (must be called to enable the
   // Apply button)
   void SetDirty()
   {
      m_bDirty = TRUE;
      if ( m_btnApply )
        m_btnApply->Enable(TRUE);
   }

   // get/set the dialog data
   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

   // callbacks
   void OnOK(wxCommandEvent& event);
   void OnApply(wxCommandEvent& event);
   void OnCancel(wxCommandEvent& event);

   // unimplemented default ctor for DECLARE_DYNAMIC_CLASS
   wxNotebookDialog() { }

   // disable or reenable Ok and Apply buttons
   void EnableButtons(bool enable)
   {
      if ( m_btnApply )
      {
          m_btnApply->Enable(enable);
          m_btnOk->Enable(enable);
      }
   }

protected:
   // clear the dirty flag
   virtual void ResetDirty()
   {
      m_bDirty = FALSE;
      m_btnApply->Enable(FALSE);
   }

   wxPNotebook *m_notebook;

private:
   wxButton    *m_btnOk,
               *m_btnApply;

   bool m_bDirty;

   DECLARE_EVENT_TABLE()
};

// a base class for notebook pages which provides some handy functions for
// layin out the controls inside the page. It is well suited for the pages
// showing the controls in a row (text fields with labels for example).
class wxNotebookPageBase : public wxPanel
{
public:
   wxNotebookPageBase(wxNotebook *parent) : wxPanel(parent, -1)
   {
      SetAutoLayout(TRUE);
   }

protected:
   // all these functions create the corresponding control and position it
   // below the "last" which may be NULL in which case the new control is put
   // just below the panel top.
   //
   // widthMax parameter is the max width of the labels and is used to align
   // labels/text entries, it can be found with GetMaxWidth() function below.

      // a listbox with 3 standard buttons: Add, Modify and Delete
      // (FIXME this is probably not flexible enough)
   wxListBox  *CreateListbox(const char *label, wxControl *last);
      // a checkbox with a label
   wxCheckBox *CreateCheckBox(const char *label,
                              long widthMax,
                              wxControl *last);
      // a label with three choices: No, Ask, Yes
   wxRadioBox *CreateActionChoice(const char *label,
                                  long widthMax,
                                  wxControl *last,
                                  size_t nRightMargin = 0);
      // nRightMargin is the distance to the right edge of the panel to leave
      // (0 means deafult)
   wxTextCtrl *CreateTextWithLabel(const char *label,
                                   long widthMax,
                                   wxControl *last,
                                   size_t nRightMargin = 0,
                                   int style = 0);
   wxStaticText *CreateMessage(const char *label,
                             wxControl *last);

   // A ComboBox, the entries are taken from the label string which 
   // is composed as: "LABEL:entry1:entry2:entry3:...."
   wxComboBox *CreateComboBox(const char *label,
                              long widthMax,
                              wxControl *last,
                              size_t nRightMargin = 0);
   // if ppButton != NULL, it's filled with the pointer to the ">>" browse
      // button created by this function
   wxTextCtrl *CreateFileEntry(const char *label, long widthMax,
                               wxControl *last,
                               wxFileBrowseButton **ppButton = NULL);
private:
   // called from CreateXXX() functions to set up the top constraint which is
   // either just below the "last", or below the page top (with some
   // additional margin possibly specified by the 3rd argument)
   void SetTopConstraint(wxLayoutConstraints *c,
                         wxControl *last,
                         size_t extraSpace = 0);
};

// -----------------------------------------------------------------------------
// helper functions
// -----------------------------------------------------------------------------

// determine the maximal width of the given strings (win is the window to use
// for font calculations)
extern long GetMaxLabelWidth(const wxArrayString& labels, wxWindow *win);

#endif // _GUI_WXDIALOGLAYOUT_H
