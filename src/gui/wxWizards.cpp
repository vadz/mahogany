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
   MWizard_CreateFolder_Type,
   MWizard_CreateFolder_Imap,
   MWizard_CreateFolder_Pop3,
   MWizard_CreateFolder_Nntp,
   MWizard_CreateFolder_File,
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
                  : mApplication->GetIconManager()->GetBitmap("install_welcome"))
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
   virtual wxWizardPage *GetPrev() const { return GetPageById(GetPreviousPageId()); }
   virtual wxWizardPage *GetNext() const { return GetPageById(GetNextPageId()); }

   /// Override these two in derived classes if needed:
   virtual MWizardPageId GetNextPageId() const
      { return m_wizard->GetNextPageId(m_id); }
   virtual MWizardPageId GetPreviousPageId() const
      { return m_wizard->GetPrevPageId(m_id); }

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
      "This wizard will help you to set up\n"
      "a new mailbox entry.\n"
      "This can be either for an existing\n"
      "mailbox or mail account or for a local\n"
      "mailboxthat you want to create new.\n"
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


// CreateFolderWizardTypePage
// ----------------------------------------------------------------------------

class MWizard_CreateFolder_TypePage : public MWizardPage
{
public:
   MWizard_CreateFolder_TypePage(MWizard *wizard);
   virtual bool TransferDataFromWindow();
   virtual MWizardPageId GetNextPageId() const;
private:
   wxChoice *m_Type;
};


MWizard_CreateFolder_TypePage::MWizard_CreateFolder_TypePage(MWizard *wizard)
   : MWizardPage(wizard, MWizard_CreateFolder_Type)
{
   wxStaticText *msg = new wxStaticText(
      this, -1,
      _("To create a new mailbox entry, you must first choose\n"
        "of which type this mailbox is."));
   
   wxEnhancedPanel *panel = CreateEnhancedPanel(msg);
   
   wxArrayString labels;
   labels.Add(_("Mailbox type"));
   
   long maxwidth = GetMaxLabelWidth(labels, panel->GetCanvas());

      // a choice control, the entries are taken from the label string which is
      // composed as: "LABEL:entry1:entry2:entry3:...."
   m_Type = panel->CreateChoice(_("Mailbox type:Local File:Remote IMAP:Remote POP3:"
                                  "Remote IMAP Hierarchy:Remote News Hierarchy:"
                                  "Remote Newsgroup:Local Newsgroup"),
                                maxwidth, NULL);
   panel->Layout();
}

bool
MWizard_CreateFolder_TypePage::TransferDataFromWindow()
{
   return TRUE;
}

MWizardPageId
MWizard_CreateFolder_TypePage::GetNextPageId() const
{
   switch(m_Type->GetSelection())
   {
   case 0:
      return MWizard_CreateFolder_File;
   case 1:
      return MWizard_CreateFolder_Imap;
   case 2:
      return MWizard_CreateFolder_Pop3;
   case 3:
      //return MWizard_CreateFolder_Imap; // hierarchy
   case 4:
      //return MWizard_CreateFolder_Nntp; // hierarchy
   case 5:
      return MWizard_CreateFolder_Nntp;
   case 6:
      //return MWizard_CreateFolder_News;
   default:
      ASSERT_MSG(0,"Not implemented yet");
      return MWizard_CreateFolder_Final;
   }
}

// CreateFolderWizardImapPage
// ----------------------------------------------------------------------------

class MWizard_CreateFolder_ImapPage : public MWizardPage
{
public:
   MWizard_CreateFolder_ImapPage(MWizard *wizard);
   virtual bool TransferDataFromWindow();
   virtual MWizardPageId GetNextPageId() const;
   virtual MWizardPageId GetPreviousPageId() const
      { return MWizard_CreateFolder_Type; }
private:
   wxTextCtrl *m_Server, *m_UserId, *m_Password;
};


MWizard_CreateFolder_ImapPage::MWizard_CreateFolder_ImapPage(MWizard *wizard)
   : MWizardPage(wizard, MWizard_CreateFolder_Type)
{
   wxStaticText *msg = new wxStaticText(
      this, -1,
      _("In order to connect to the IMAP server,\n"
        "you need a Login and a Password."));
   
   wxEnhancedPanel *panel = CreateEnhancedPanel(msg);
   
   wxArrayString labels;
   labels.Add(_("Server Host"));
   labels.Add(_("Your Login for it"));
   labels.Add(_("Your Password"));
   
   long maxwidth = GetMaxLabelWidth(labels, panel->GetCanvas());

   m_Server   = panel->CreateTextWithLabel(labels[0], maxwidth, NULL);
   m_UserId   = panel->CreateTextWithLabel(labels[1], maxwidth, m_Server);
   m_Password = panel->CreateTextWithLabel(labels[2], maxwidth, m_UserId);
   panel->Layout();
}

bool
MWizard_CreateFolder_ImapPage::TransferDataFromWindow()
{
   return TRUE;
}

MWizardPageId
MWizard_CreateFolder_ImapPage::GetNextPageId() const
{
   return MWizard_CreateFolder_Final;
}

// CreateFolderWizardPop3Page
// ----------------------------------------------------------------------------

class MWizard_CreateFolder_Pop3Page : public MWizardPage
{
public:
   MWizard_CreateFolder_Pop3Page(MWizard *wizard);
   virtual bool TransferDataFromWindow();
   virtual MWizardPageId GetNextPageId() const;
   virtual MWizardPageId GetPreviousPageId() const
      { return MWizard_CreateFolder_Type; }

private:
   wxTextCtrl *m_Server, *m_UserId, *m_Password;
};


MWizard_CreateFolder_Pop3Page::MWizard_CreateFolder_Pop3Page(MWizard *wizard)
   : MWizardPage(wizard, MWizard_CreateFolder_Type)
{
   wxStaticText *msg = new wxStaticText(
      this, -1,
      _("In order to connect to the POP3 server,\n"
        "you need a Login and a Password."));
   
   wxEnhancedPanel *panel = CreateEnhancedPanel(msg);
   
   wxArrayString labels;
   labels.Add(_("Server Host"));
   labels.Add(_("Your Login for it"));
   labels.Add(_("Your Password"));
   
   long maxwidth = GetMaxLabelWidth(labels, panel->GetCanvas());

   m_Server   = panel->CreateTextWithLabel(labels[0], maxwidth, NULL);
   m_UserId   = panel->CreateTextWithLabel(labels[1], maxwidth, m_Server);
   m_Password = panel->CreateTextWithLabel(labels[2], maxwidth, m_UserId);
   panel->Layout();
}

bool
MWizard_CreateFolder_Pop3Page::TransferDataFromWindow()
{
   return TRUE;
}

MWizardPageId
MWizard_CreateFolder_Pop3Page::GetNextPageId() const
{
   return MWizard_CreateFolder_Final;
}


// CreateFolderWizardNntpPage
// ----------------------------------------------------------------------------

class MWizard_CreateFolder_NntpPage : public MWizardPage
{
public:
   MWizard_CreateFolder_NntpPage(MWizard *wizard);
   virtual bool TransferDataFromWindow();
   virtual MWizardPageId GetNextPageId() const;
   virtual MWizardPageId GetPreviousPageId() const
      { return MWizard_CreateFolder_Type; }
private:
   wxChoice *m_Type;
   wxTextCtrl *m_Server, *m_UserId, *m_Password;
};


MWizard_CreateFolder_NntpPage::MWizard_CreateFolder_NntpPage(MWizard *wizard)
   : MWizardPage(wizard, MWizard_CreateFolder_Type)
{
   wxStaticText *msg = new wxStaticText(
      this, -1,
      _("To create a new mailbox entry, you must first choose\n"
        "of which type this mailbox is."));
   
   wxEnhancedPanel *panel = CreateEnhancedPanel(msg);
   
   wxArrayString labels;
   labels.Add(_("Mailbox type"));
   labels.Add(_("Server Host"));
   labels.Add(_("Your Login for it"));
   labels.Add(_("Your Password"));
   
   long maxwidth = GetMaxLabelWidth(labels, panel->GetCanvas());

      // a choice control, the entries are taken from the label string which is
      // composed as: "LABEL:entry1:entry2:entry3:...."
   m_Type = panel->CreateChoice(_("Mailbox type:Local File:Remote IMAP:Remote POP3:"
                                  "Remote IMAP Hierarchy:Remote News Hierarchy:"
                                  "Remote Newsgroup:Local Newsgroup"),
                                maxwidth, NULL);
   m_Server   = panel->CreateTextWithLabel(labels[1], maxwidth, m_Type);
   m_UserId   = panel->CreateTextWithLabel(labels[2], maxwidth, m_Server);
   m_Password = panel->CreateTextWithLabel(labels[3], maxwidth, m_UserId);
   panel->Layout();
}

bool
MWizard_CreateFolder_NntpPage::TransferDataFromWindow()
{
   return TRUE;
}

MWizardPageId
MWizard_CreateFolder_NntpPage::GetNextPageId() const
{
   return MWizard_CreateFolder_Final;
}

// CreateFolderWizardFilePage
// ----------------------------------------------------------------------------

class MWizard_CreateFolder_FilePage : public MWizardPage
{
public:
   MWizard_CreateFolder_FilePage(MWizard *wizard);
   virtual bool TransferDataFromWindow();
   virtual MWizardPageId GetNextPageId() const;
   virtual MWizardPageId GetPreviousPageId() const
      { return MWizard_CreateFolder_Type; }
private:
   wxChoice *m_Type;
   wxTextCtrl *m_Server, *m_UserId, *m_Password;
};


MWizard_CreateFolder_FilePage::MWizard_CreateFolder_FilePage(MWizard *wizard)
   : MWizardPage(wizard, MWizard_CreateFolder_Type)
{
   wxStaticText *msg = new wxStaticText(
      this, -1,
      _("To create a new mailbox entry, you must first choose\n"
        "of which type this mailbox is."));
   
   wxEnhancedPanel *panel = CreateEnhancedPanel(msg);
   
   wxArrayString labels;
   labels.Add(_("Mailbox type"));
   labels.Add(_("Server Host"));
   labels.Add(_("Your Login for it"));
   labels.Add(_("Your Password"));
   
   long maxwidth = GetMaxLabelWidth(labels, panel->GetCanvas());

      // a choice control, the entries are taken from the label string which is
      // composed as: "LABEL:entry1:entry2:entry3:...."
   m_Type = panel->CreateChoice(_("Mailbox type:Local File:Remote IMAP:Remote Pop3:"
                                  "Remote IMAP Hierarchy:Remote News Hierarchy:"
                                  "Remote Newsgroup:Local Newsgroup"),
                                maxwidth, NULL);
   m_Server   = panel->CreateTextWithLabel(labels[1], maxwidth, m_Type);
   m_UserId   = panel->CreateTextWithLabel(labels[2], maxwidth, m_Server);
   m_Password = panel->CreateTextWithLabel(labels[3], maxwidth, m_UserId);
   panel->Layout();
}

bool
MWizard_CreateFolder_FilePage::TransferDataFromWindow()
{
   return TRUE;
}

MWizardPageId
MWizard_CreateFolder_FilePage::GetNextPageId() const
{
   return MWizard_CreateFolder_Final;
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
         CREATE_PAGE(CreateFolder_Type);
         CREATE_PAGE(CreateFolder_Imap);
         CREATE_PAGE(CreateFolder_Pop3);
         CREATE_PAGE(CreateFolder_Nntp);
         CREATE_PAGE(CreateFolder_File);
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
