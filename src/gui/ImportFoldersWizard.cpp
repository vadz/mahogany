///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   src/gui/ImportFoldersWizard.cpp
// Purpose:     Implementation of the folder import wizard.
// Author:      Karsten Ballüder (ballueder@gmx.net)
// Created:     2010-05-18 (extracted from src/gui/wxWizards.cpp)
// CVS-ID:      $Id$
// Copyright:   (c) 2000-2010 M-Team
// License:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"
   #include "strutil.h"
   #include "Mdefaults.h"
   #include "MApplication.h"
   #include "guidef.h"
   #include "gui/wxIconManager.h"
#endif // USE_PCH

#include "MailFolderCC.h"

#include "gui/wxDialogLayout.h"
#include "gui/MWizard.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

namespace
{

enum
{
   MWizard_ImportFolders_Choice,
   MWizard_ImportFolders_MH,
   MWizard_ImportFolders_Max
};

} // anonymous namespace

// ----------------------------------------------------------------------------
// ImportFoldersWizard: propose to import existing folders
// ----------------------------------------------------------------------------

class ImportFoldersWizard : public MWizard
{
public:
   ImportFoldersWizard()
      : MWizard(MWizard_ImportFolders_Max, _("Import existing mail folders"))
      {
      }

   struct Params
   {
      // MH params
      String pathRootMH;
      bool takeAllMH;

      // what to do?
      bool importMH;

      // def ctor
      Params()
      {
         importMH = false;
      }
   };

   Params& GetParams() { return m_params; }

private:
   Params m_params;

   virtual wxWizardPage *DoCreatePage(MWizardPageId id);

   DECLARE_NO_COPY_CLASS(ImportFoldersWizard)
};

// MWizard_ImportFolders_ChoicePage
// ----------------------------------------------------------------------------

// first page: propose all folders we can import to the user
class MWizard_ImportFolders_ChoicePage : public MWizardPage
{
public:
   MWizard_ImportFolders_ChoicePage(MWizard *wizard);

   virtual MWizardPageId GetPreviousPageId() const { return MWizard_PageNone; }
   virtual MWizardPageId GetNextPageId() const;

   void OnCheckBox(wxCommandEvent& event);

private:
   wxCheckBox *m_checkMH;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(MWizard_ImportFolders_ChoicePage)
};

BEGIN_EVENT_TABLE(MWizard_ImportFolders_ChoicePage, MWizardPage)
   EVT_CHECKBOX(-1, MWizard_ImportFolders_ChoicePage::OnCheckBox)
END_EVENT_TABLE()

MWizard_ImportFolders_ChoicePage::MWizard_ImportFolders_ChoicePage(MWizard *wizard)
                             : MWizardPage(wizard,
                                           MWizard_ImportFolders_Choice)
{
   m_checkMH = NULL;

   bool hasMH = false,
        hasSomethingToImport = false;

   if ( MailFolderCC::ExistsMH() )
   {
      hasMH = true;
      hasSomethingToImport = true;
   }

   wxString msgText;
   if ( hasSomethingToImport )
   {
      msgText = _("Please choose what would you like to do.");
   }
   else
   {
      msgText = _("Mahogany didn't find any folders to import on this system.");
   }

   wxStaticText *msg = new wxStaticText(this, -1, msgText);

   wxEnhancedPanel *panel = CreateEnhancedPanel(msg);
   wxArrayString labels;
   if ( hasMH )
   {
      labels.Add(_("Import MH folders"));
   }

   long maxwidth = GetMaxLabelWidth(labels, panel->GetCanvas());

   maxwidth += 10;

   if ( hasMH )
   {
      m_checkMH = panel->CreateCheckBox(labels[0], maxwidth, NULL);

      // by default, import them all
      m_checkMH->SetValue(true);
   }

   panel->Layout();
}

MWizardPageId MWizard_ImportFolders_ChoicePage::GetNextPageId() const
{
   if ( m_checkMH && m_checkMH->GetValue() )
      return MWizard_ImportFolders_MH;

   //else if ( m_check && m_check->GetValue() ) ...

   return MWizard_PageNone;
}

void MWizard_ImportFolders_ChoicePage::OnCheckBox(wxCommandEvent&)
{
   SetNextButtonLabel(GetNextPageId() == MWizard_PageNone);
}

// MWizard_ImportFolders_MHPage
// ----------------------------------------------------------------------------

// first page: propose all folders we can import to the user
class MWizard_ImportFolders_MHPage : public MWizardPage
{
public:
   MWizard_ImportFolders_MHPage(MWizard *wizard);

   virtual MWizardPageId GetPreviousPageId() const
      { return MWizard_ImportFolders_Choice; }
   virtual MWizardPageId GetNextPageId() const
      { return MWizard_PageNone; }

   virtual bool TransferDataFromWindow();

private:
   wxTextCtrl *m_textTop;
   wxCheckBox *m_checkAll;

   DECLARE_NO_COPY_CLASS(MWizard_ImportFolders_MHPage)
};

MWizard_ImportFolders_MHPage::MWizard_ImportFolders_MHPage(MWizard *wizard)
                         : MWizardPage(wizard, MWizard_ImportFolders_MH)
{
   wxStaticText *msg = new wxStaticText(this, -1, _(
            "You may import either just the top level MH folder and\n"
            "later create manually all or some of its subfolders using\n"
            "the popup menu in the folder tree or import all existing\n"
            "MH subfolders at once.\n"
            "\n"
            "Don't change the default location of the top of MH tree\n"
            "(the MH root) unless you really know what you are doing."
      ));

   wxEnhancedPanel *panel = CreateEnhancedPanel(msg);
   wxArrayString labels;
   labels.Add(_("MH root"));
   labels.Add(_("Import all"));

   long maxwidth = GetMaxLabelWidth(labels, panel->GetCanvas());

   maxwidth += 5;

   m_textTop = panel->CreateDirEntry(labels[0], maxwidth, NULL);
   m_checkAll = panel->CreateCheckBox(labels[1], maxwidth, m_textTop);

   // init controls
   m_textTop->SetValue(MailFolderCC::InitializeMH());
   m_checkAll->SetValue(true);

   panel->Layout();
}

bool MWizard_ImportFolders_MHPage::TransferDataFromWindow()
{
   ImportFoldersWizard::Params& params =
      ((ImportFoldersWizard *)GetWizard())->GetParams();

   params.importMH = true;
   params.takeAllMH = m_checkAll->GetValue();
   params.pathRootMH = m_textTop->GetValue();

   return true;
}

wxWizardPage *
ImportFoldersWizard::DoCreatePage(MWizardPageId id)
{
#define CREATE_PAGE(pid) \
   case MWizard_##pid: \
      return new MWizard_##pid##Page(this); \

   switch ( id )
   {
      CREATE_PAGE(ImportFolders_Choice);
      CREATE_PAGE(ImportFolders_MH);
   }
#undef CREATE_PAGE

   return NULL;
}

void RunImportFoldersWizard()
{
   ImportFoldersWizard *wizard = new ImportFoldersWizard();
   if ( wizard->Run() )
   {
      const ImportFoldersWizard::Params& params = wizard->GetParams();
      if ( params.importMH )
      {
         MailFolderCC::ImportFoldersMH(params.pathRootMH, params.takeAllMH);
      }
   }

   delete wizard;
}
