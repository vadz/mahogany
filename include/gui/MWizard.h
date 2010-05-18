///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   include/gui/MWizard.h
// Purpose:     Declaration of MWizard class.
// Author:      Karsten Ballüder (ballueder@gmx.net)
// Created:     2010-05-18 (extracted from src/gui/wxWizards.cpp)
// CVS-ID:      $Id$
// Copyright:   (c) 2000-2010 M-Team
// License:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef M_GUI_MWIZARD_H_
#define M_GUI_MWIZARD_H_

#include <wx/sizer.h>         // we use GetPageAreaSizer()
#include <wx/stattext.h>
#include <wx/wizard.h>

#include "MFolder.h"

#include "gui/wxDialogLayout.h"

// ids for install wizard pages
enum
{
   MWizard_PageNone = -1            // illegal value
};

typedef int MWizardPageId;

/**
  The MWizard is a very simple to use Wizard class. Each wizard uses a
  set of pages (can be shared across wizards) and knows on which page
  to start and finish. The decision which page is next/previous is
  handled by the page which can query the Wizard for a numeric id to
  know what kind of wizard is running it.
 */
class MWizard : public wxWizard
{
public:
   MWizard(int numPages,
           const wxString &title,
           const wxBitmap * bitmap = NULL,
           wxWindow *parent = NULL)
      : wxWizard( parent, -1, title,
                  // using bitmap before '?' results in a compile error with
                  // Borland C++ - go figure
                  !bitmap ? mApplication->GetIconManager()->
                              GetBitmap(_T("install_welcome"))
                          : *bitmap),
         m_numPages(numPages)
      {
         m_WizardPages = new wxWizardPage *[numPages];
         memset(m_WizardPages, 0, numPages*sizeof(wxWizardPage *));
      }

   virtual ~MWizard()
   {
      delete [] m_WizardPages;
   }

   MWizardPageId GetFirstPageId() const { return 0; }
   MWizardPageId GetLastPageId() const { return m_numPages - 1; }

   virtual bool Run()
   {
      wxWizardPage * const pageFirst = GetPageById(GetFirstPageId());
      GetPageAreaSizer()->Add(pageFirst);

      return RunWizard(pageFirst);
   }

   wxWizardPage *GetPageById(MWizardPageId id)
   {
      if ( id == GetLastPageId()+1 || id == MWizard_PageNone)
         return NULL;

      CHECK( id >= 0 && id < m_numPages, NULL, "page index out of range" );

      const int ofs = id - GetFirstPageId();
      if ( !m_WizardPages[ofs] )
      {
         m_WizardPages[ofs] = DoCreatePage(id);

         ASSERT_MSG( m_WizardPages[ofs], "unknown wizard page?" );
      }

      return m_WizardPages[ofs];
   }

private:
   /// Must be overridden in the derived classes to actually create pages.
   virtual wxWizardPage *DoCreatePage(MWizardPageId id) = 0;

   const int m_numPages;

   wxWizardPage **m_WizardPages;

   DECLARE_NO_COPY_CLASS(MWizard)
};

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
      m_Wizard = wizard;

      Connect(wxEVT_WIZARD_CANCEL,
                  wxWizardEventHandler(MWizardPage::OnWizardCancel));
   }

    /* These are two wxWindows wxWizard functions that we must override
    */
   virtual wxWizardPage *GetPrev() const { return GetPageById(GetPreviousPageId()); }
   virtual wxWizardPage *GetNext() const { return GetPageById(GetNextPageId()); }

   /// Override these two in derived classes if needed:
   virtual MWizardPageId GetNextPageId() const
   {
      return GetPageId() < m_Wizard->GetLastPageId() ?
         (MWizardPageId) (GetPageId()+1)
         : m_Wizard->GetLastPageId();
   }
   virtual MWizardPageId GetPreviousPageId() const
   {
      return  GetPageId() > GetWizard()->GetFirstPageId() ?
         (MWizardPageId) (GetPageId()-1) : GetWizard()->GetFirstPageId();
   }

   MWizardPageId GetPageId() const { return m_id; }
   virtual MWizard * GetWizard() const { return (MWizard *)m_Wizard; }

   void OnWizardCancel(wxWizardEvent& event)
   {
      if ( !MDialog_YesNoDialog(_("Do you really want to abort the wizard?")) )
      {
         // not confirmed
         event.Veto();
      }
   }

protected:
   wxWizardPage *GetPageById(MWizardPageId id) const
      { return GetWizard()->GetPageById(id); }

   // set the label of the next button depending on whether we're going to
   // advance to the next page or if this is the last one
   void SetNextButtonLabel(bool isLast)
   {
      wxButton * const
         btn = wxDynamicCast(GetParent()->FindWindow(wxID_FORWARD), wxButton);
      if ( btn )
      {
         btn->SetLabel(isLast ? _("&Finish") : _("&Next >"));
      }
   }

   // creates an "enhanced panel" for placing controls into under the static
   // text (explanation)
   wxEnhancedPanel *CreateEnhancedPanel(wxStaticText *text)
   {
      wxSize sizeLabel = text->GetSize();
      wxSize sizePage = ((wxWizard *)GetParent())->GetClientSize();
      wxCoord y = sizeLabel.y + 2*LAYOUT_Y_MARGIN;

      wxEnhancedPanel *panel = new wxEnhancedPanel(this, false /* no scrolling */);
      panel->SetSize(0, y, sizePage.x, sizePage.y - y);

      panel->SetAutoLayout(true);

      return panel;
   }

private:
   // id of this page
   MWizardPageId m_id;
   // the wizard
   MWizard *m_Wizard;

   DECLARE_NO_COPY_CLASS(MWizardPage)
};

#endif // M_GUI_MWIZARD_H_

