/*-*- c++ -*-********************************************************
 * wxWizards.cpp : Wizards for various tasks                        *
 *                                                                  *
 * (C) 2000 by Karsten Ballüder (ballueder@gmx.net)                 *
 *                                                                  *
 * $Id$
 *******************************************************************/

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
#   include "Mcommon.h"
#   include "Mdefaults.h"
#   include "guidef.h"
#   include "strutil.h"
#   include "MFrame.h"
#   include "MDialogs.h"
#   include "Profile.h"
#   include "MApplication.h"
#   include "MailFolder.h"
#   include "Profile.h"
#   include "MModule.h"
#   include "MHelp.h"
#endif

#include "Mpers.h"

#include "MHelp.h"
#include "gui/wxMApp.h"
#include "gui/wxMIds.h"

#include "gui/wxIconManager.h"

#include <wx/window.h>
#include <wx/radiobox.h>
#include <wx/stattext.h>
#include <wx/statbmp.h>
#include <wx/choice.h>
#include <wx/textdlg.h>
#include <wx/help.h>
#include <wx/wizard.h>

#include "MFolderDialogs.h"

#include "gui/wxDialogLayout.h"


#include <errno.h>

// ----------------------------------------------------------------------------
// global vars and functions
// ----------------------------------------------------------------------------

// ids for install wizard pages
enum MWizardPageId
{
   MWizard_CreateFolder_First,
   MWizard_CreateFolder_Welcome = MWizard_CreateFolder_First,     // say hello
   MWizard_CreateFolder_Final,                            // say that everything is ok
   MWizard_CreateFolder_Last = MWizard_CreateFolder_Final,
   
   MWizard_PagesMax = MWizard_CreateFolder_Final,  // the number of pages
   MWizard_Done = -1                             // invalid page index
};


class MWizard : public wxWizard
{
public:
   enum MWizardType { MWizardType_CreateFolder };
   
   MWizard(MWizardType type,
           MWizardPageId first,
           MWizardPageId last,
           const wxString &title,
           const wxBitmap * bitmap = NULL,
           wxWindow *parent = NULL)
      : wxWizard( parent, -1, title,
                  bitmap ? *bitmap
                  : mApplication->GetIconManager()->GetBitmap("wizard"))
      {
         m_Type = type;
         m_FirstPage = first;
         m_LastPage = last;
         // NULL the pages array
         memset(m_wizardPages, 0, sizeof(m_wizardPages));
      }

   MWizardType GetType(void) const { return m_Type; }

   // override to return the id of the next/prev page: default versions go to
   // the next/prev in order of MWizardPageIds
   virtual MWizardPageId GetPrevPageId(MWizardPageId current) const;
   virtual MWizardPageId GetNextPageId(MWizardPageId current) const;

   bool Run(void);
   class MWizardPage * GetPageById(MWizardPageId id);

private:
   MWizardType m_Type;
   class MWizardPage * m_wizardPages[MWizard_PagesMax];
   MWizardPageId m_FirstPage, m_LastPage;
};

MWizardPageId
MWizard::GetPrevPageId(MWizardPageId current) const
{
   ASSERT(current >= m_FirstPage && current <= m_LastPage);
   return current > m_FirstPage ?
      (MWizardPageId) (current-1)
      : MWizard_Done;
}

MWizardPageId
MWizard::GetNextPageId(MWizardPageId current) const
{
   ASSERT(current >= m_FirstPage && current <= m_LastPage);
   return current < m_LastPage ?
      (MWizardPageId) (current+1)
      : MWizard_Done;
}


// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------
// the base class for our wizards pages: it allows to return page ids (and not
// the pointers themselves) from GetPrev/Next and processes [Cancel] in a
// standard way an provides other useful functions for the derived classes

class MWizardPage : public wxWizardPage
{
public:
   MWizardPage(MWizard *wizard, MWizardPageId id)
      : wxWizardPage(wizard)
      {
         m_id = id;
         m_wizard = wizard;
      }

   // implement the wxWizardPage pure virtuals in terms of our ones
   virtual wxWizardPage *GetPrev() const { return GetPageById(m_wizard->GetPrevPageId(m_id)); }
   virtual wxWizardPage *GetNext() const { return GetPageById(m_wizard->GetNextPageId(m_id)); }

   MWizardPageId GetPageId() const { return m_id; }
   MWizard * GetWizard() const { return (MWizard *)m_wizard; }
   
   void OnWizardCancel(wxWizardEvent& event);
   
protected:
   wxWizardPage *GetPageById(MWizardPageId id) const
      { return m_wizard->GetPageById(id); }
      

   // creates an "enhanced panel" for placing controls into under the static
   // text (explanation)
   wxEnhancedPanel *CreateEnhancedPanel(wxStaticText *text);

private:
   // id of this page
   MWizardPageId m_id;
   // the wizard
   MWizard *m_wizard;
   
   DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(MWizardPage, wxWizardPage)
   EVT_WIZARD_CANCEL(-1, MWizardPage::OnWizardCancel)
END_EVENT_TABLE()


void MWizardPage::OnWizardCancel(wxWizardEvent& event)
{
   if ( !MDialog_YesNoDialog(_("Do you really want to abort the wizard?")) )
   {
      // not confirmed
      event.Veto();
   }
   else
   {
      // wizard will be cancelled
      wxLogMessage(_("Please use the 'Options' dialog to configure\n"
                     "the program before using it!"));
   }
}



wxEnhancedPanel *MWizardPage::CreateEnhancedPanel(wxStaticText *text)
{
   wxSize sizeLabel = text->GetSize();
   wxSize sizePage = ((wxWizard *)GetParent())->GetPageSize();
//   wxSize sizePage = ((wxWizard *)GetParent())->GetSize();
   wxCoord y = sizeLabel.y + 2*LAYOUT_Y_MARGIN;

   wxEnhancedPanel *panel = new wxEnhancedPanel(this, FALSE /* no scrolling */);
   panel->SetSize(0, y, sizePage.x, sizePage.y - y);

   panel->SetAutoLayout(TRUE);

   return panel;
}


bool
MWizard::Run()
{
   return RunWizard(GetPageById(m_FirstPage));
}



// CreateFolderWizardWelcomePage
// ----------------------------------------------------------------------------

// first page: welcome the user, explain what goes on
class MWizard_CreateFolder_WelcomePage : public MWizardPage
{
public:
   MWizard_CreateFolder_WelcomePage(MWizard *wizard);
   virtual bool TransferDataFromWindow();
   virtual MWizardPageId GetNextPageId() const;
private:
   wxCheckBox *m_checkbox;
   bool        m_useWizard;
};


MWizard_CreateFolder_WelcomePage::MWizard_CreateFolder_WelcomePage(MWizard *wizard)
   : MWizardPage(wizard, MWizard_CreateFolder_Welcome)
{
   (void)new wxStaticText(this, -1, _(
      "Welcome to Mahogany!\n"
      "\n"
      "This wizard will help you to setup the most\n"
      "important settings needed to successfully use\n"
      "the program. You don't need to specify everything\n"
      "here - it can also be done later by opening the\n"
      "'Options' dialog - but if you do fill the, you\n"
      "should be able to start the program immediately.\n"
      "\n"
      "If you decide to not use the dialog, just check\n"
      "the box below or press [Cancel] at any moment."
                                         ));

   m_useWizard = true;
   m_checkbox = new wxCheckBox
                (
                  this, -1,
                  _("I'm an &expert and don't need the wizard")
                );

   wxSize sizeBox = m_checkbox->GetSize(),
   sizePage = wizard->GetPageSize();
   
   // adjust the vertical position
   m_checkbox->Move(-1, sizePage.y - 2*sizeBox.y);
}

bool
MWizard_CreateFolder_WelcomePage::TransferDataFromWindow()
{
   m_useWizard = !m_checkbox->GetValue();
   return TRUE;
}

MWizardPageId
MWizard_CreateFolder_WelcomePage::GetNextPageId() const
{
   return m_useWizard ? GetWizard()->GetNextPageId(GetPageId()) : MWizard_Done;
}


// CreateFolderWizard final page
// ----------------------------------------------------------------------------

class MWizard_CreateFolder_FinalPage : public MWizardPage
{
public:
   MWizard_CreateFolder_FinalPage(MWizard *wizard);
};


MWizard_CreateFolder_FinalPage::MWizard_CreateFolder_FinalPage(MWizard *wizard)
   : MWizardPage(wizard, MWizard_CreateFolder_Final)
{
   (void)new wxStaticText(this, -1, _(
      "Welcome to Mahogany!\n"
      "\n"
      "This wizard will help you to setup the most\n"
      "important settings needed to successfully use\n"
      "the program. You don't need to specify everything\n"
      "here - it can also be done later by opening the\n"
      "'Options' dialog - but if you do fill the, you\n"
      "should be able to start the program immediately.\n"
      "\n"
      "If you decide to not use the dialog, just check\n"
      "the box below or press [Cancel] at any moment."
                                         ));
}


// ============================================================================
// implementation
// ============================================================================


MWizardPage *
MWizard::GetPageById(MWizardPageId id)
{
   if ( id == MWizard_Done )
      return (MWizardPage *)NULL;

   if ( !m_wizardPages[id] )
   {
#define CREATE_PAGE(pid)                             \
      case MWizard_##pid##:                          \
         m_wizardPages[MWizard_##pid] =               \
            new MWizard_##pid##Page(this); \
         break

      switch ( id )
      {
         CREATE_PAGE(CreateFolder_Welcome);
         CREATE_PAGE(CreateFolder_Final);
      case MWizard_Done:
         ASSERT(0);
      }
#undef CREATE_PAGE
   }
   return m_wizardPages[id];
}


// The CreateFolderWizard
//
// ----------------------------------------------------------------------------

class CreateFolderWizard : public MWizard
{
public:
   CreateFolderWizard(wxWindow *parent = NULL)
      : MWizard( MWizardType_CreateFolder,
                 MWizard_CreateFolder_First,
                 MWizard_CreateFolder_Last,
                 _("Create a Mailbox Entry"),
                 NULL, parent)
      {
         
      }
};


bool
RunCreateFolderWizard(wxWindow *parent)
{
   MWizard *wizard = new CreateFolderWizard(parent);
   bool rc = wizard->Run();
   delete wizard;
   return rc;
}
