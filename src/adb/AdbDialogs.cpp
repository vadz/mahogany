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
#   include "Profile.h"
#   include "gui/wxIconManager.h"

#   include <wx/layout.h>
#   include <wx/listbox.h>              // for wxListBox
#   include <wx/stattext.h>             // for wxStaticText
#   include <wx/statbmp.h>
#   include <wx/statbox.h>
#   include <wx/choicdlg.h>
#   include <wx/filedlg.h>
#endif //USE_PCH

#include "adb/AdbImport.h"
#include "adb/AdbExport.h"
#include "adb/AdbImpExp.h"
#include "adb/AdbManager.h"

#include "gui/wxDialogLayout.h"
#include "gui/wxMDialogs.h"

#include <wx/confbase.h>

// ----------------------------------------------------------------------------
// dialog classes
// ----------------------------------------------------------------------------

// the dialog allowing the user to choose the file name to import addresses
// from and the format they are in
class wxAdbImportDialog : public wxManuallyLaidOutDialog
{
public:
   // ctor & dtor
   wxAdbImportDialog(wxWindow *parent);
   virtual ~wxAdbImportDialog();

   // accessors (call them only after ShowModal)

      // if empty, find importer ourself
   const wxString& GetImporterName() const { return m_importer; }

      // if empty, no importer chosen
   const wxString& GetImporterDesc() const { return m_desc; }

      // if empty, get the filename from importer
   const wxString& GetFileName() const { return m_filename; }

   // fill the listbox with importer names
   virtual bool TransferDataToWindow();

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
      wxString path;
      path << _T('/') << M_SETTINGS_CONFIG_SECTION << _T("/AdbImportFile");
      return path;
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
   DECLARE_NO_COPY_CLASS(wxAdbImportDialog)
};

// the dialog allowing the user to choose from possibly many ADB expansions
class wxAdbExpandDialog : public wxManuallyLaidOutDialog
{
public:
   wxAdbExpandDialog(ArrayAdbElements& aEverything,
                     ArrayAdbEntries& aMoreEntries,
                     size_t nGroups,
                     wxFrame *parent);

   // get the index of the selected item in the listbox
   int GetSelection() const { return m_listbox->GetSelection(); }

   virtual bool TransferDataToWindow();

   // control ids
   enum
   {
      Btn_More = 100,
      Btn_Delete
   };

protected:
   // event handlers
   void OnLboxDblClick(wxCommandEvent& /* event */) { EndModal(wxID_OK); }
   void OnBtnMore(wxCommandEvent& event);
   void OnBtnDelete(wxCommandEvent& event);
   void OnUpdateBtnDelete(wxUpdateUIEvent& event)
   {
      // this catches both the case when there is no selection at all and when
      // a group is selected: as we can't delete groups (yet?), we disable the
      // delete button then as well
      event.Enable(m_listbox->GetSelection() >= (int)m_nGroups);
   }

private:
   // the listbox containing the expansion possibilities
   wxListBox *m_listbox;

   // the buttons to show more entries (may be NULL) and to delete the selected
   // one (never NULL)
   wxButton *m_btnMore,
            *m_btnDelete;

   // the main and additional alements array
   ArrayAdbElements& m_aEverything;
   ArrayAdbEntries& m_aMoreEntries;

   // the number of groups in the beginning of m_aEverything
   size_t m_nGroups;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxAdbExpandDialog)
};

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxAdbImportDialog, wxManuallyLaidOutDialog)
   EVT_TEXT(-1, wxAdbImportDialog::OnText)
   EVT_CHECKBOX(-1, wxAdbImportDialog::OnCheckbox)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxAdbExpandDialog, wxManuallyLaidOutDialog)
   EVT_LISTBOX_DCLICK(-1, wxAdbExpandDialog::OnLboxDblClick)

   EVT_BUTTON(wxAdbExpandDialog::Btn_More, wxAdbExpandDialog::OnBtnMore)
   EVT_BUTTON(wxAdbExpandDialog::Btn_Delete, wxAdbExpandDialog::OnBtnDelete)

   EVT_UPDATE_UI(wxAdbExpandDialog::Btn_Delete,
                 wxAdbExpandDialog::OnUpdateBtnDelete)
END_EVENT_TABLE()

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxAdbImportDialog
// ----------------------------------------------------------------------------

wxAdbImportDialog::wxAdbImportDialog(wxWindow *parent)
                 : wxManuallyLaidOutDialog(parent,
                                           _("Import address book"),
                                           _T("AdbImport"))
{
   wxLayoutConstraints *c;

   // global layout description, from top to bottom, from left to right:
   //    icon, multiline static text with explanations,
   //    text entry zone and browse button for file/directory name
   //    checkbox
   //    listbox
   //    buttons

   // buttons
   (void)CreateStdButtonsAndBox(_T(""), TRUE /* no box please */);
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
                                             GetBitmap(_T("adbimport")));
   c = new wxLayoutConstraints;
   c->top.SameAs(canvas, wxTop);
   c->left.SameAs(canvas, wxLeft);
   c->width.Absolute(32);
   c->height.Absolute(32);
   bmp->SetConstraints(c);

#if 0
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
#else
   wxStaticText *msg = new wxStaticText(canvas, -1,
                                        _("Select the file or directory to import the data\n"
                                          "from or select the import format and check the\n"
                                          "'location' checkbox below"));
      c = new wxLayoutConstraints;
      c->top.SameAs(canvas, wxTop, LAYOUT_Y_MARGIN);
      c->left.RightOf(bmp, 2*LAYOUT_X_MARGIN);
      c->width.AsIs();
      c->height.AsIs();
      msg->SetConstraints(c);
#endif

   // text and browse button
   int widthMax;
   wxString label(_("&File:"));
   GetTextExtent(label, &widthMax, NULL);
   m_text = m_panel->CreateFileEntry(label, (long)widthMax, msg, &m_browseBtn);

   // checkboxes
   m_autoLocation = new wxPCheckBox(_T("AdbImportAutoFile"), canvas, -1,
                                    _("Determine the &location automatically"));
   c = new wxLayoutConstraints;
   c->top.Below(m_text, LAYOUT_Y_MARGIN);
   c->left.SameAs(canvas, wxLeft);
   c->width.AsIs();
   c->height.AsIs();
   m_autoLocation->SetConstraints(c);

   m_autoFormat = new wxPCheckBox(_T("AdbImportAutoFormat"), canvas, -1,
                                  _("Determine the &format automatically"));
   c = new wxLayoutConstraints;
   c->top.Below(m_autoLocation, LAYOUT_Y_MARGIN);
   c->left.SameAs(canvas, wxLeft);
   c->width.AsIs();
   c->height.AsIs();
   m_autoFormat->SetConstraints(c);

   // listbox
   m_listbox = new wxPListBox(_T("AdbImportFormat"), canvas, -1);
   c = new wxLayoutConstraints;
   c->top.Below(m_autoFormat, LAYOUT_Y_MARGIN);
   c->left.SameAs(canvas, wxLeft);
   c->right.SameAs(canvas, wxRight);
   c->bottom.SameAs(canvas, wxBottom);
   m_listbox->SetConstraints(c);

   // final steps
   String file = wxConfigBase::Get()->Read(GetFileProfilePath(), _T(""));
   if ( !file.empty() )
   {
      m_text->SetValue(file);
   }

   SetDefaultSize(4*LAYOUT_X_MARGIN + msg->GetSize().x + 32, 12*hBtn);
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

bool wxAdbImportDialog::TransferDataToWindow()
{
   // populate the listbox
   size_t nCount = AdbImporter::EnumImporters(m_importerNames, m_importerDescs);

   if ( !nCount )
   {
      wxLogError(_("Sorry, no import filters found - importing address "
                   "books is not available in this version of the program."));

      return FALSE;
   }

   for ( size_t n = 0; n < nCount; n++ )
   {
      m_listbox->Append(m_importerDescs[n]);
   }

   return TRUE;
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
   wxConfigBase::Get()->Write(GetFileProfilePath(), m_text->GetValue());
}

// ----------------------------------------------------------------------------
// wxAdbExpandDialog
// ----------------------------------------------------------------------------

wxAdbExpandDialog::wxAdbExpandDialog(ArrayAdbElements& aEverything,
                                     ArrayAdbEntries& aMoreEntries,
                                     size_t nGroups,
                                     wxFrame *parent)
                 : wxManuallyLaidOutDialog(parent,
                                           _("Expansion options"),
                                           _T("AdrListSelect")),
                   m_aEverything(aEverything),
                   m_aMoreEntries(aMoreEntries),
                   m_nGroups(nGroups)
{
   /*
      the dialog layout is like this:

      listbox
      listbox  [more...]
      listbox  [delete ]
      listbox

           [ok] [cancel]
    */

   wxStaticBox *box = CreateStdButtonsAndBox(_("Please choose an entry:"));

   m_listbox = new wxListBox(this, -1);

   // don't show the "More" button if there are no more matches
   m_btnMore = aMoreEntries.IsEmpty()
                  ? NULL
                  : new wxButton(this, Btn_More, _("&More matches"));

   m_btnDelete = new wxButton(this, Btn_Delete, _("&Delete"));
#if wxUSE_TOOLTIPS
   m_btnDelete->SetToolTip(_("Don't propose the selected address in this "
                             "dialog any more and remove it from the list now"));
#endif // wxUSE_TOOLTIPS

   // we have to fill the listbox here or it won't have the correct size
   size_t nEntryCount = aEverything.GetCount();
   for( size_t nEntry = 0; nEntry < nEntryCount; nEntry++ )
   {
      m_listbox->Append(aEverything[nEntry]->GetDescription());
   }

   wxLayoutConstraints *c;
   c = new wxLayoutConstraints;
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->width.AsIs();
   c->height.AsIs();

   if ( m_btnMore )
   {
#if wxUSE_TOOLTIPS
      m_btnMore->SetToolTip(_("Show more matching entries"));
#endif // wxUSE_TOOLTIPS

      c->bottom.SameAs(box, wxCentreY, LAYOUT_Y_MARGIN);
      m_btnMore->SetConstraints(c);

      c = new wxLayoutConstraints;
      c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
      c->width.SameAs(m_btnMore, wxWidth);
      c->height.AsIs();
      c->top.Below(m_btnMore, 2*LAYOUT_Y_MARGIN);
   }
   else // no "More" button
   {
      // just position the "Delete" button in the centre
      c->centreY.SameAs(box, wxCentreY);
   }

   m_btnDelete->SetConstraints(c);

   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.LeftOf(m_btnDelete, LAYOUT_X_MARGIN);
   c->top.SameAs(box, wxTop, 3*LAYOUT_Y_MARGIN);
   c->bottom.SameAs(box, wxBottom, 2*LAYOUT_Y_MARGIN);
   m_listbox->SetConstraints(c);

   SetDefaultSize(5*wBtn, 10*hBtn);
}

bool wxAdbExpandDialog::TransferDataToWindow()
{
   m_listbox->SetFocus();
   if ( m_listbox->GetCount() )
      m_listbox->Select(0);

   return true;
}

void wxAdbExpandDialog::OnBtnMore(wxCommandEvent&)
{
   size_t nEntryCount = m_aMoreEntries.GetCount();
   for( size_t nEntry = 0; nEntry < nEntryCount; nEntry++ )
   {
      m_listbox->Append(m_aMoreEntries[nEntry]->GetDescription());
   }

   // nothing more to add
   m_btnMore->Disable();
}

void wxAdbExpandDialog::OnBtnDelete(wxCommandEvent& WXUNUSED(event))
{
   size_t n = (size_t)m_listbox->GetSelection();
   CHECK_RET( n >= m_nGroups, _T("should be disabled") );

   // first remove it from the listbox
   m_listbox->Delete(n);

   // now remove it from the internal data as well
   AdbEntry *entry;

   size_t countMain = m_aEverything.GetCount();
   if ( n < countMain )
   {
      entry = (AdbEntry *)m_aEverything[n];
      m_aEverything.RemoveAt(n);
   }
   else // an additional entry
   {
      n -= countMain;

      entry = m_aMoreEntries[n];
      m_aMoreEntries.RemoveAt(n);
   }

   // remember to not use it for the expansion again
   entry->SetField(AdbField_ExpandPriority, _T("-1"));

   entry->DecRef();
}

// ----------------------------------------------------------------------------
// public interface
// ----------------------------------------------------------------------------

bool AdbShowImportDialog(wxWindow *parent, String *nameOfNativeAdb)
{
   wxWindow *frame = GetDialogParent(parent);
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
      CHECK( importer, FALSE, _T("should have either importer or filename") );

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
                     _T("AdbImportFile"),
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
   adbname += _T(".adb");

   if ( !MInputBox(&adbname,
                   _("Address book import"),
                   _("Native address book to create: "),
                   frame,
                   _T("AdbImportBook"),
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
   bool ok = AdbImport(filename, adbname, _T(""), importer);

   // release it only if we created it
   SafeDecRef(importer);

   return ok;
}

bool AdbShowExportDialog(AdbEntryGroup& group)
{
   wxArrayString names, descs;
   wxString name;

   size_t n = AdbExporter::EnumExporters(names, descs);
   if( n > 1 )
   {
      int
         w = 400,
         h = 400;

      int idx = wxGetSingleChoiceIndex
         (
            _("Please choose an export format:"),
            wxString(_T("Mahogany : "))+_("ADB export options"),
            n,
            &descs[0],
            NULL,
            -1, -1, // x,y
            TRUE,   //centre
            w, h
            );
      if(idx >= 0)
         name = names[idx];
      else
         return FALSE; // cancelled

   }
   else
   {
      // we have at most 1 exporter, so don't propose to choose
      name = n == 0 ? String(_T("AdbTextExporter")) : names[0];
   }

   AdbExporter *exporter = AdbExporter::GetExporterByName(name);
   if ( !exporter )
   {
      // this can only happen if EnumExporters() returned 0
      wxLogError(_("Cannot export address book - the functionality "
                   "is missing in this version of the program."));

      return FALSE;
   }

   bool ok = AdbExport(group, *exporter);
   exporter->DecRef();

   return ok;
}

int
AdbShowExpandDialog(ArrayAdbElements& aEverything,
                    ArrayAdbEntries& aMoreEntries,
                    size_t nGroups,
                    wxFrame *parent)
{
   int choice;

   size_t count = aEverything.GetCount();
   switch ( count )
   {
      case 0:
         // nothing to choose from
         choice = -1;
         break;

      case 1:
         // don't ask user to choose among one entry and itself!
         choice = 0;
         break;

      default:
         // do show the dialog
         wxAdbExpandDialog dialog(aEverything, aMoreEntries, nGroups, parent);

         choice = dialog.ShowModal() == wxID_OK ? dialog.GetSelection() : -1;
   }

   return choice;
}

