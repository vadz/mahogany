///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   adb/AdbDialogs.cpp - assorted ADB-related dialogs
// Purpose:     dialogs to import an ADB, select ADB entry expansion, ...
// Author:      Vadim Zeitlin
// Modified by:
// Created:     08.07.99
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// M
#include "Mpch.h"

#ifndef  USE_PCH
#   include "Mcommon.h"
#   include "MApplication.h"
#   include "guidef.h"

#   include <wx/layout.h>
#   include <wx/listbox.h>
#   include <wx/stattext.h>
#   include <wx/statbmp.h>
#endif //USE_PCH

#include "Mdefaults.h"

#include "adb/AdbImport.h"

#include "gui/wxIconManager.h"
#include "gui/wxBrowseButton.h"
#include "gui/wxDialogLayout.h"
#include "gui/wxMDialogs.h"

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// FIXME there is also one in wxMDialogs.cpp!
static inline wxFrame *GetDialogParent(wxWindow *parent)
{
  return parent == NULL ? mApplication->TopLevelFrame()
                        : GetFrame(parent);
}

// ----------------------------------------------------------------------------
// dialog classes
// ----------------------------------------------------------------------------

class wxAdbImportDialog : public wxManuallyLaidOutDialog
{
public:
   // ctor & dtor
   wxAdbImportDialog(wxFrame *parent);
   virtual ~wxAdbImportDialog();

   // accessors (call them only after ShowModal)

      // if empty, find importer ourself
   const wxString& GetImporterName() const { return m_importer; }

      // if empty, no importer chosen
   const wxString& GetImporterDesc() const { return m_desc; }

      // if empty, get the filename from importer
   const wxString& GetFileName() const { return m_filename; }

   // save the controls values
   virtual bool TransferDataFromWindow();

   // set the controls state
   void DoUpdateUI();

   // event handlers
   void OnText(wxCommandEvent& WXUNUSED(event)) { DoUpdateUI(); }
   void OnCheckbox(wxCommandEvent& WXUNUSED(event)) { DoUpdateUI(); }

private:
   // return path where we store the last value of the text ctrl
   static wxString GetFileProfilePath()
   {
      return wxString(M_SETTINGS_CONFIG_SECTION) + "AdbImportFile";
   }

   // stored data
   wxString m_importer,
            m_desc;
   wxString m_filename;

   // dialog controls
   wxEnhancedPanel    *m_panel;
   wxFileBrowseButton *m_browseBtn;
   wxTextCtrl         *m_text;
   wxCheckBox         *m_autoLocation,
                      *m_autoFormat;
   wxListBox          *m_listbox;
   wxButton           *m_btnOk;

   // info about all importers
   wxArrayString       m_importerNames,
                       m_importerDescs;

   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxAdbImportDialog, wxManuallyLaidOutDialog)
   EVT_TEXT(-1, wxAdbImportDialog::OnText)
   EVT_CHECKBOX(-1, wxAdbImportDialog::OnCheckbox)
END_EVENT_TABLE()

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxAdbImportDialog
// ----------------------------------------------------------------------------

wxAdbImportDialog::wxAdbImportDialog(wxFrame *parent)
                 : wxManuallyLaidOutDialog(parent,
                                           _("Import address book"),
                                           "AdbImport")
{
   wxLayoutConstraints *c;

   // global layout description, from top to bottom, from left to right:
   //    icon, multiline static text with explanations,
   //    text entry zone and browse button for file/directory name
   //    checkbox
   //    listbox
   //    buttons

   // buttons
   (void)CreateStdButtonsAndBox("", TRUE /* no box please */);
   m_btnOk = (wxButton *)FindWindow(wxID_OK);

   // panel for all other items
   m_panel = new wxEnhancedPanel(this);
   c = new wxLayoutConstraints;
   c->top.SameAs(this, wxTop, LAYOUT_Y_MARGIN);
   c->left.SameAs(this, wxLeft, LAYOUT_X_MARGIN);
   c->right.SameAs(this, wxRight, LAYOUT_X_MARGIN);
   c->bottom.Above(m_btnOk, -2*LAYOUT_Y_MARGIN);
   m_panel->SetConstraints(c);

   m_panel->SetAutoLayout(TRUE);

   // all items must be created on the canvas, not the panel itself
   wxWindow *canvas = m_panel->GetCanvas();

   // icon and help message
   wxStaticBitmap *bmp = new wxStaticBitmap(canvas, -1,
                                            mApplication->GetIconManager()->
                                             GetBitmap("adbimport"));
   c = new wxLayoutConstraints;
   c->top.SameAs(canvas, wxTop);
   c->left.SameAs(canvas, wxLeft);
   c->width.Absolute(32);
   c->height.Absolute(32);
   bmp->SetConstraints(c);

   wxArrayString lines;
   wxSize sizeMsg = SplitTextMessage
                    (
                     _("Select the file or directory to import the data\n"
                       "from or select the import format and check the\n"
                       "'location' checkbox below"),
                     &lines
                    );

   wxStaticText *msg = NULL;
   size_t nLines = lines.GetCount();
   for ( size_t nLine = 0; nLine < nLines; nLine++ )
   {
      c = new wxLayoutConstraints;
      if ( msg )
      {
         c->top.Below(msg, LAYOUT_Y_MARGIN / 2);
      }
      else
      {
         c->top.SameAs(canvas, wxTop, LAYOUT_Y_MARGIN);
      }

      c->left.RightOf(bmp, 2*LAYOUT_X_MARGIN);
      c->width.Absolute(sizeMsg.x);
      c->height.Absolute(sizeMsg.y);

      msg = new wxStaticText(canvas, -1, lines[nLine]);
      msg->SetConstraints(c);
   }

   // text and browse button
   int widthMax;
   wxString label(_("&File:"));
   GetTextExtent(label, &widthMax, NULL);
   m_text = m_panel->CreateFileEntry(label, (long)widthMax, msg, &m_browseBtn);

   // checkboxes
   m_autoLocation = new wxPCheckBox("AdbImportAutoFile", canvas, -1,
                                    _("Determine the &location automatically"));
   c = new wxLayoutConstraints;
   c->top.Below(m_text, LAYOUT_Y_MARGIN);
   c->left.SameAs(canvas, wxLeft);
   c->width.AsIs();
   c->height.AsIs();
   m_autoLocation->SetConstraints(c);

   m_autoFormat = new wxPCheckBox("AdbImportAutoFormat", canvas, -1,
                                  _("Determine the &format automatically"));
   c = new wxLayoutConstraints;
   c->top.Below(m_autoLocation, LAYOUT_Y_MARGIN);
   c->left.SameAs(canvas, wxLeft);
   c->width.AsIs();
   c->height.AsIs();
   m_autoFormat->SetConstraints(c);

   // listbox
   m_listbox = new wxListBox(canvas, -1);
   c = new wxLayoutConstraints;
   c->top.Below(m_autoFormat, LAYOUT_Y_MARGIN);
   c->left.SameAs(canvas, wxLeft);
   c->right.SameAs(canvas, wxRight);
   c->bottom.SameAs(canvas, wxBottom);
   m_listbox->SetConstraints(c);

   // populate the listbox
   size_t nCount = AdbImporter::EnumImporters(m_importerNames, m_importerDescs);
   for ( size_t n = 0; n < nCount; n++ )
   {
      m_listbox->Append(m_importerDescs[n]);
   }

   // final steps
   String file = mApplication->GetProfile()->readEntry(GetFileProfilePath(),
                                                       "");
   if ( !!file )
   {
      m_text->SetValue(file);
   }

   SetDefaultSize(4*LAYOUT_X_MARGIN + sizeMsg.x + 32, 12*hBtn);
   Centre();
   DoUpdateUI();
}

// we have several things to do whenever something changes (like checkbox
// state), so it's more convenient to do them all at once here than have half a
// dozen of separate EVT_UPDATE_UI() handlers
void wxAdbImportDialog::DoUpdateUI()
{
   bool autoLocation = m_autoLocation->GetValue(),
        autoFormat = m_autoFormat->GetValue();

   m_panel->EnableTextWithButton(m_text, !autoLocation);
   m_listbox->Enable(!autoFormat);

   // both checkboxes can't be checked at once, so whenever one is checked,
   // disable the other
   m_autoLocation->Enable(!autoFormat);
   m_autoFormat->Enable(!autoLocation);

   // the ok button is enabled if we either have a filename and the import
   // format or, if either of these parts is missing, the corresponding
   // checkbox is enabled
   m_btnOk->Enable((autoLocation || !!m_text->GetValue()) &&
                   (autoFormat || m_listbox->GetSelection() != -1));
}

bool wxAdbImportDialog::TransferDataFromWindow()
{
   if ( !m_autoLocation->GetValue() )
   {
      // get the user specified filename
      m_filename = m_text->GetValue();
   }

   if ( !m_autoFormat->GetValue() )
   {
      // get the user specified format
      size_t index = (size_t)m_listbox->GetSelection();
      m_importer = m_importerNames[index];
      m_desc = m_importerDescs[index];
   }

   return TRUE;
}

wxAdbImportDialog::~wxAdbImportDialog()
{
   // save the file entry zone value in the profile
   mApplication->GetProfile()->writeEntry(GetFileProfilePath(),
                                          m_text->GetValue());
}

// ----------------------------------------------------------------------------
// public interface
// ----------------------------------------------------------------------------

bool AdbShowImportDialog(wxWindow *parent, String *nameOfNativeAdb)
{
   wxFrame *frame = GetDialogParent(parent);
   wxAdbImportDialog dlg(frame);
   if ( dlg.ShowModal() != wxID_OK )
   {
      // cancelled
      return FALSE;
   }

   wxString importerName = dlg.GetImporterName(),
            importerDesc = dlg.GetImporterDesc(),
            filename = dlg.GetFileName();

   AdbImporter *importer = NULL;
   if ( !!importerName )
   {
      importer = AdbImporter::GetImporterByName(importerName);
   }

   if ( !filename )
   {
      // we can't guess everything!
      CHECK( importer, FALSE, "should have either importer or filename" );

      filename = importer->GetDefaultFilename();
      if ( !filename )
      {
         wxLogWarning(_("Sorry, impossible to determine the location of "
                        "the default address book file for the format "
                        "'%s' - please specify the file manually in the "
                        "next dialog."), importerDesc.c_str());

         // make the message appear before the dialog box, otherwise it's
         // really confusing - we say that we determine the file location
         // ourself, yet ask the user for the file without any explanation
         // (and the explanation will appear later!)
         if ( wxLog::GetActiveTarget() )
         {
            wxLog::GetActiveTarget()->Flush();
         }

         filename = wxPFileSelector
                    (
                     "AdbImportFile",
                     _("Please select source address book file"),
                     NULL, NULL, NULL, NULL,
                     wxOPEN | wxFILE_MUST_EXIST,
                     frame
                    );
      }

      if ( !filename )
      {
         // cancelled by user
         SafeDecRef(importer);

         return FALSE;
      }
   }

   // ask for the name of the ADB to import data in
   wxString adbname, ext;
   wxSplitPath(filename, NULL, &adbname, &ext);
   if ( !adbname )
   {
      // this means that the file starts with '.' in which case just take the
      // extension (the real name, in fact) as the address book default name
      adbname = ext;
   }

   // the standard suffix for our address books
   adbname += ".adb";

   if ( !MInputBox(&adbname,
                   _("Address book import"),
                   _("Native address book to create: "),
                   frame,
                   "AdbImportBook",
                   adbname) )
   {
      // cancelled by user
      SafeDecRef(importer);

      return FALSE;
   }

   if ( *nameOfNativeAdb )
   {
      *nameOfNativeAdb = adbname;
   }

   // do import
   bool ok = AdbImport(filename, adbname, importer);

   // release it only if we created it
   SafeDecRef(importer);

   return ok;
}
