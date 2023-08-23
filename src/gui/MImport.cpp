///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   gui/MImport.cpp - GUI support for import operations
// Purpose:     allow the user to select what and from where to import
// Author:      Vadim Zeitlin
// Modified by:
// Created:     23.05.00
// CVS-ID:      $Id$
// Copyright:   (c) 2000 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
#  include "Mcommon.h"
#  include "MApplication.h"
#  include "guidef.h"
#  include "gui/wxIconManager.h"

#  include <wx/sizer.h>
#  include <wx/statbmp.h>
#  include <wx/statbox.h>
#  include <wx/listbox.h>
#  include <wx/checkbox.h>
#endif // USE_PCH

#include <wx/datetime.h>
#include <wx/statline.h>

#include "gui/wxMDialogs.h"
#include "MFolder.h"

#include "MImport.h"

class MPersMsgBox;

// ----------------------------------------------------------------------------
// persistent msgboxes we use here
// ----------------------------------------------------------------------------

extern const MPersMsgBox *M_MSGBOX_IMPORT_FOLDERS_UNDER_ROOT;

// ----------------------------------------------------------------------------
// array classes
// ----------------------------------------------------------------------------

WX_DEFINE_ARRAY(MImporter *, wxArrayImporters);

// ----------------------------------------------------------------------------
// private function prototypes
// ----------------------------------------------------------------------------

static bool FindAllImporters(wxArrayImporters& importers,
                             wxArrayString& prognames);

static void FreeImporters(const wxArrayImporters& importers);

// ----------------------------------------------------------------------------
// ImportersArray_obj: small helper class which frees importers automatically
// ----------------------------------------------------------------------------

class ImportersArrayFree
{
public:
   ImportersArrayFree(wxArrayImporters& importers) : m_importers(importers)
   {
   }

   ~ImportersArrayFree() { FreeImporters(m_importers); }

private:
   wxArrayImporters& m_importers;

   DECLARE_NO_COPY_CLASS(ImportersArrayFree)
};

// ----------------------------------------------------------------------------
// wxImportDialog: the dialog from which the user launches importing
// ----------------------------------------------------------------------------

class wxImportDialog : public wxDialog
{
public:
   wxImportDialog(MImporter& importer, wxWindow *parent);

   // did import succeed?
   bool IsOk() const { return m_ok; }

   // get the log listbox (for wxImportDialogLog)
   wxListBox *GetLogListBox() const { return m_listbox; }

   // event handlers
   void OnOk(wxCommandEvent& event);

private:
   static wxString GetImportDialogTitle(const MImporter& importer);

   void SetOkBtnLabel(const wxString& label)
   {
      wxWindow *btnOk = FindWindow(wxID_OK);
      if ( btnOk )
         btnOk->SetLabel(label);
   }

   MImporter& m_importer;

   wxListBox  *m_listbox;
   wxCheckBox *m_checkADB,
              *m_checkFolders,
              *m_checkSettings,
              *m_checkFilters;

   bool m_done, m_ok;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxImportDialog)
};

// ----------------------------------------------------------------------------
// wxImportDialogLog: a log target which logs everything into a listbox in the
// import dialog and also lets the messages pass through to the old logger
// ----------------------------------------------------------------------------

class wxImportDialogLog : public wxLog
{
public:
   wxImportDialogLog(wxImportDialog *dlg, wxLog *logOld)
   {
      m_dialog = dlg;
      m_logOld = logOld;
   }

   virtual ~wxImportDialogLog() { delete wxLog::SetActiveTarget(m_logOld); }

   virtual void DoLogRecord(wxLogLevel WXUNUSED(level),
                            const wxString& szString,
                            const wxLogRecordInfo& info)
   {
      const wxLongLong t = info.timestampMS;
      m_dialog->GetLogListBox()->Append(
            wxString::Format(_T("%s:\t%s"),
                             wxDateTime(t).FormatTime(),
                             szString)
         );
   }
   //} just to balance parenthesis

private:
   wxImportDialog *m_dialog;
   wxLog *m_logOld;
};

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxImportDialog, wxDialog)
   EVT_BUTTON(wxID_OK, wxImportDialog::OnOk)
END_EVENT_TABLE()

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxImportDialog
// ----------------------------------------------------------------------------

wxImportDialog::wxImportDialog(MImporter& importer, wxWindow *parent)
              : wxDialog(parent, -1, GetImportDialogTitle(importer),
                         wxDefaultPosition, wxDefaultSize,
                         wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
                m_importer(importer)
{
   m_done = false;
   m_ok = true;

   wxBoxSizer *topsizer = new wxBoxSizer( wxVERTICAL );

   wxBoxSizer *sizerText = new wxBoxSizer( wxHORIZONTAL );
   sizerText->Add( new wxStaticBitmap(this, -1,
                     mApplication->GetIconManager()->GetBitmap(_T("import"))),
         0, wxCENTRE | wxALL, 5 );
   sizerText->Add( CreateTextSizer(
            _("Please choose the actions below you would like to perform\n"
               "and then click the [Start] button to start importing.") ),
         1, wxCENTRE | wxALL, 5 );
   topsizer->Add(sizerText, 0, wxEXPAND | wxALL, 5);

   wxStaticBox *box = new wxStaticBox(this, -1, _("&What to do:"));
   wxStaticBoxSizer *actionsSizer = new wxStaticBoxSizer(box, wxVERTICAL);
   m_checkSettings = new wxCheckBox(this, -1, _("import &settings"));
   m_checkADB = new wxCheckBox(this, -1, _("import &address books"));
   m_checkFolders = new wxCheckBox(this, -1, _("import &folders"));
   m_checkFilters = new wxCheckBox(this, -1, _("import filter &rules"));
   actionsSizer->Add(m_checkADB, 0, wxEXPAND);
   actionsSizer->Add(m_checkFolders, 0, wxEXPAND);
   actionsSizer->Add(m_checkSettings, 0, wxEXPAND);
   actionsSizer->Add(m_checkFilters, 0, wxEXPAND);

   int flags = m_importer.GetFeatures();

   #define INIT_IMPORT(what)                    \
      if ( flags & MImporter::Import_##what )   \
         m_check##what->SetValue(TRUE);         \
      else                                      \
         m_check##what->Disable()

   INIT_IMPORT(ADB);
   INIT_IMPORT(Folders);
   INIT_IMPORT(Settings);
   INIT_IMPORT(Filters);

   #undef INIT_IMPORT

   topsizer->Add( actionsSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 15 );

   m_listbox = new wxListBox(this, -1);
   m_listbox->Disable();
   box = new wxStaticBox(this, -1, _("Import log"));
   wxStaticBoxSizer *resultSizer = new wxStaticBoxSizer(box, wxVERTICAL);
   resultSizer->Add(m_listbox, 1, wxEXPAND);
   topsizer->Add( resultSizer, 1, wxEXPAND | wxLEFT | wxRIGHT, 15 );

   topsizer->Add( new wxStaticLine( this, -1 ), 0, wxEXPAND | wxLEFT|wxRIGHT|wxTOP, 10 );

   topsizer->Add( CreateButtonSizer( wxOK|wxCANCEL ), 0, wxCENTRE | wxALL, 10 );
   SetOkBtnLabel(_("&Start"));

   SetAutoLayout( TRUE );
   SetSizer( topsizer );

   topsizer->SetSizeHints( this );
   topsizer->Fit( this );

   Centre( wxBOTH );
}

wxString wxImportDialog::GetImportDialogTitle(const MImporter& importer)
{
   return wxString::Format(_("Mahogany: Import Dialog for %s"),
                           importer.GetProgName());
}

void wxImportDialog::OnOk(wxCommandEvent& event)
{
   if ( m_done )
   {
      // let the normal processing (dismiss dialog) to take place
      event.Skip();
   }
   else
   {
      wxLog *logOld = wxLog::GetActiveTarget();
      wxLog::SetActiveTarget(new wxImportDialogLog(this, logOld));

      const char *progname = m_importer.GetProgName();

      #define DO_IMPORT(what, desc) \
         if ( m_check##what->GetValue() ) \
         { \
            MBusyCursor bc; \
            m_check##what->Disable(); \
            wxLogMessage(_("Importing %s %s"), progname, desc); \
            if ( !m_importer.Import##what() ) \
               m_ok = false; \
         }

      DO_IMPORT(Settings, _("settings"));
      DO_IMPORT(ADB, _("address books"));

      if ( m_checkFolders->GetValue() )
      {
         MFolder *folderParent = NULL;
         int flags = 0;

         String msg;
         msg.Printf(_("Would you like to put %s folders under a subgroup\n"
                      "(otherwise they will be created at the tree "
                      "root level?"), progname);
         if ( MDialog_YesNoDialog
              (
                  msg,
                  this,
                  _("Import Folders"),
                  M_DLG_YES_DEFAULT,
                  M_MSGBOX_IMPORT_FOLDERS_UNDER_ROOT
              ) )
         {
            folderParent = MDialog_FolderChoose(this);
         }

         // TODO: this should be customizable, right now we just always put
         //       the system folders under a subgroup without letting the user
         //       to put them elsewhere nor (which is probably more useful) to
         //       not import them at all
         if ( !folderParent )
         {
            String folderName;
            folderName.Printf(_("%s System Folders"), progname);
            folderParent = MFolder::Get(folderName);
            if ( !folderParent )
            {
               folderParent = CreateFolderTreeEntry
                              (
                               NULL,
                               folderName,
                               MF_GROUP,
                               0,
                               wxEmptyString,
                               FALSE
                              );
            }
         }
         else
         {
            // the user has chosen the folder, put the folders there
            flags |= MImporter::ImportFolder_AllUseParent;
         }

         flags |= MImporter::ImportFolder_SystemUseParent;

         MBusyCursor bc;

         wxLogMessage(_("Importing %s %s"), progname, _("folders"));
         if ( !m_importer.ImportFolders(folderParent, flags) )
            m_ok = false;

         SafeDecRef(folderParent);
      }

      DO_IMPORT(Filters, _("filter rules"));

      #undef DO_IMPORT

      m_done = true;

      // disable all checkboxes as they can't be used any longer
      m_checkADB->Disable();
      m_checkFolders->Disable();
      m_checkSettings->Disable();
      m_checkFilters->Disable();

      SetOkBtnLabel(_("Ok"));

      // can't cancel import any longer
      wxWindow *btnCancel = FindWindow(wxID_CANCEL);
      if ( btnCancel )
         btnCancel->Disable();

      if ( m_ok )
      {
         wxLogMessage(_("%s configuration settings imported successfully."),
                      progname);
      }
      else
      {
         wxLogError(_("Importing from %s failed."), progname);
      }

      m_listbox->Enable();

      wxLog::SetActiveTarget(logOld);
   }
}

// ----------------------------------------------------------------------------
// helpers: common part of HasImporters() and ShowImportDialog()
// ----------------------------------------------------------------------------

// fills the provided arrays with all importers which can be used (i.e. their
// Applies() returned true)
//
// return true if we have any importers, false otherwise
static bool FindAllImporters(wxArrayImporters& importers,
                             wxArrayString& prognames)
{
   MModuleListing *listing = MModule::ListAvailableModules(M_IMPORTER_INTERFACE);
   if ( !listing )
      return false;

   size_t count = listing->Count();
   for ( size_t n = 0; n < count; n++ )
   {
      const MModuleListingEntry& entry = (*listing)[n];

      // load the module
      MImporter *importer = (MImporter *)MModule::LoadModule(entry.GetName());
      if ( importer )
      {
         if ( importer->Applies() )
         {
            importers.Add(importer);
            prognames.Add(importer->GetProgName());
         }
         else
         {
            importer->DecRef();
         }
      }
      else
      {
         wxLogDebug(_T("Couldn't load importer module '%s'."), entry.GetName());
      }
   }

   listing->DecRef();

   return importers.GetCount() > 0;
}

// frees all importers in the array
static void FreeImporters(const wxArrayImporters& importers)
{
   size_t count = importers.GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      importers[n]->DecRef();
   }
}

// ----------------------------------------------------------------------------
// public API
// ----------------------------------------------------------------------------

extern bool HasImporters()
{
   wxArrayImporters importers;
   wxArrayString prognames;

   ImportersArrayFree impoFree(importers);

   return FindAllImporters(importers, prognames);
}

extern bool ShowImportDialog(MImporter& importer, wxWindow *parent)
{
   wxImportDialog dlg(importer, parent);

   return dlg.ShowModal() == wxID_OK && dlg.IsOk();
}

extern bool ShowImportDialog(wxWindow *parent)
{
   // first, find all (applicable) importers
   wxArrayImporters importers;
   wxArrayString prognames;

   ImportersArrayFree impoFree(importers);

   if ( !FindAllImporters(importers, prognames) )
   {
      wxLogWarning(_("None of Mahogany importers can be used, sorry."));

      return false;
   }

   // then let the user choose which ones he wants to use
   wxArrayInt selections;
   size_t count = MDialog_GetSelections(
                                 _("Please select the programs you want to\n"
                                   "import the settings from."),
                                 _("Mahogany: Import Settings"),
                                 prognames,
                                 &selections,
                                 parent,
                                 _T("Importers"),
                                 wxSize(150, 200)
                                );

   // and do import from those he chose
   bool doneSomething = false;
   size_t n;
   for ( n = 0; n < count; n++ )
   {
      if ( ShowImportDialog(*importers[selections[n]], parent) )
         doneSomething  = true;
   }

   return doneSomething;
}
