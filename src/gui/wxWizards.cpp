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




/** 
  The MWizard is a very simple to use Wizard class. Each wizard uses a 
  set of pages (can be shared across wizards) and knows on which page
  to start and finish. The decision which page is next/previous is
  handled by the page which can query the Wizard for a numeric id to
  know what kind of wizard is running it.
 */

// ids for install wizard pages
enum MWizardPageId
{
   MWizard_CreateFolder_First,
   MWizard_CreateFolder_Welcome = MWizard_CreateFolder_First,     // say hello
   MWizard_CreateFolder_Type,
   MWizard_CreateFolder_FirstType,
   MWizard_CreateFolder_File =    MWizard_CreateFolder_FirstType,
   MWizard_CreateFolder_MH,
   MWizard_CreateFolder_Imap,
   MWizard_CreateFolder_Pop3,
   MWizard_CreateFolder_ImapServer,
   MWizard_CreateFolder_NntpServer,
   MWizard_CreateFolder_ImapHier,
   MWizard_CreateFolder_NntpHier,
   MWizard_CreateFolder_Nntp,
   MWizard_CreateFolder_News,
   MWizard_CreateFolder_NewsHier,
   MWizard_CreateFolder_LastType = MWizard_CreateFolder_NewsHier,
   MWizard_CreateFolder_Final,                            // say that everything is ok
   MWizard_CreateFolder_Last = MWizard_CreateFolder_Final,
   MWizard_PagesMax = MWizard_CreateFolder_Final,  // the number of pages

   MWizard_PageNone = -1          // illegal value
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
         m_First = first;
         m_Last = last;
         // NULL the pages array
         memset(m_WizardPages, 0, sizeof(m_WizardPages));
      }

   /** Returns a numeric type for the Wizard to allow pages to change
       their behaviour for different Wizards. No deeper meaning.
   */
   MWizardType GetType(void) const { return m_Type; }

   MWizardPageId GetFirstPageId(void) const { return m_First; }
   MWizardPageId GetLastPageId(void) const { return m_Last; }
   bool Run(void);
   class MWizardPage * GetPageById(MWizardPageId id);

private:
   MWizardType m_Type;
   class MWizardPage * m_WizardPages[MWizard_PagesMax];
   MWizardPageId m_First, m_Last;
};


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
         m_Wizard = wizard;
      }

    /* These are two wxWindows wxWizard functions that we must override 
    */
   virtual wxWizardPage *GetPrev() const { return GetPageById(GetPreviousPageId()); }
   virtual wxWizardPage *GetNext() const { return GetPageById(GetNextPageId()); }

   /// Override these two in derived classes if needed:
   virtual MWizardPageId GetNextPageId() const;
   virtual MWizardPageId GetPreviousPageId() const;

   MWizardPageId GetPageId() const { return m_id; }
   MWizard * GetWizard() const { return (MWizard *)m_Wizard; }
   
   void OnWizardCancel(wxWizardEvent& event);
   
protected:
   wxWizardPage *GetPageById(MWizardPageId id) const
      { return GetWizard()->GetPageById(id); }
      

   // creates an "enhanced panel" for placing controls into under the static
   // text (explanation)
   wxEnhancedPanel *CreateEnhancedPanel(wxStaticText *text);

private:
   // id of this page
   MWizardPageId m_id;
   // the wizard
   MWizard *m_Wizard;
   
   DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(MWizardPage, wxWizardPage)
   EVT_WIZARD_CANCEL(-1, MWizardPage::OnWizardCancel)
END_EVENT_TABLE()


MWizardPageId
MWizardPage::GetPreviousPageId(void) const
{
   return  GetPageId() > GetWizard()->GetFirstPageId() ?
      (MWizardPageId) (GetPageId()-1) : GetWizard()->GetFirstPageId();
}

MWizardPageId
MWizardPage::GetNextPageId(void) const
{
   return GetPageId() < m_Wizard->GetLastPageId() ?
      (MWizardPageId) (GetPageId()+1)
      : m_Wizard->GetLastPageId();
}

void MWizardPage::OnWizardCancel(wxWizardEvent& event)
{
   if ( !MDialog_YesNoDialog(_("Do you really want to abort the wizard?")) )
   {
      // not confirmed
      event.Veto();
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
   return RunWizard(GetPageById(GetFirstPageId()));
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
   virtual MWizardPageId GetPreviousPageId() const
      { return MWizard_PageNone; }
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
      "mailbox or mail account or for a new\n"
      "mailbox that you want to create.\n"
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
   return m_useWizard ? MWizardPage::GetNextPageId() : GetWizard()->GetLastPageId();
}


// CreateFolderWizardTypePage
// ----------------------------------------------------------------------------

enum FolderEntryType
{
   ET_FILE = 0, ET_MH, ET_IMAP, ET_POP3,
   ET_IMAP_SERVER, ET_NNTP_SERVER,
   ET_IMAP_HIER, ET_NNTP_HIER,
   ET_NNTP, ET_NEWS_HIER, ET_NEWS
};

class MWizard_CreateFolder_TypePage : public MWizardPage
{
public:
   MWizard_CreateFolder_TypePage(MWizard *wizard);
   virtual bool TransferDataFromWindow();
   virtual MWizardPageId GetNextPageId() const;
private:
   wxChoice *m_TypeCtrl;
};


MWizard_CreateFolder_TypePage::MWizard_CreateFolder_TypePage(MWizard *wizard)
   : MWizardPage(wizard, MWizard_CreateFolder_Type)
{
   wxStaticText *msg = new wxStaticText(
      this, -1,
      _("A mailbox entry can represent differnt\n"
        "things: a mailbox file, remote servers\n"
        "or mailboxes, newsgroups or news hierarchies\n"
        "either locally or on a remote server.\n"
        "\n"
        "So, first you need to decide what kind\n"
        "of entry you would like to create:"));
   
   wxEnhancedPanel *panel = CreateEnhancedPanel(msg);
   
   wxArrayString labels;
   labels.Add(_("Mailbox type"));
   
   long maxwidth = GetMaxLabelWidth(labels, panel->GetCanvas());

      // a choice control, the entries are taken from the label string which is
      // composed as: "LABEL:entry1:entry2:entry3:...."
   m_TypeCtrl = panel->CreateChoice(_("Enty type:"
                                      "Local Mailbox File:"
                                      "Local MH Folder:"
                                      "IMAP Mailbox:POP3 Mailbox:"
                                      "IMAP Server:NNTP News Server:"
                                      "IMAP Hierarchy:NNTP News Hierarchy:"
                                      "NNTP Newsgroup:Local News Hierarchy:"
                                      "Local Newsgroup"), 
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
   int sel = m_TypeCtrl->GetSelection();
   sel += MWizard_CreateFolder_FirstType;
   
   CHECK(sel >= MWizard_CreateFolder_FirstType
         && sel <= MWizard_CreateFolder_LastType,
         MWizard_CreateFolder_Final, "Unknown folder type");
   return (MWizardPageId) sel;
}

// CreateFolderServerPage - common page for different server/hierarchy types
// ----------------------------------------------------------------------------

class MWizard_CreateFolder_ServerPage : public MWizardPage
{
public:
   MWizard_CreateFolder_ServerPage(MWizard *wizard,
                                   MWizardPageId id,
                                   FolderEntryType type);
   virtual bool TransferDataFromWindow();
   virtual MWizardPageId GetNextPageId() const
      { return MWizard_CreateFolder_Final; }
   virtual MWizardPageId GetPreviousPageId() const
      { return MWizard_CreateFolder_Type; }
private:
   wxTextCtrl *m_Name, *m_Server, *m_UserId, *m_Password, *m_Path;
   FolderEntryType m_Type;
};

MWizard_CreateFolder_ServerPage::MWizard_CreateFolder_ServerPage(
   MWizard *wizard, MWizardPageId id, FolderEntryType type)
   : MWizardPage(wizard, id)
{
   m_Type = type;

   wxString
      entry,
      pathName = _("Mailbox name"),
      msg = _("To access a%s\n"
              "the following access parameters\n"
              "are needed:\n");
      bool
      needsServer = FALSE,
      needsUserId = FALSE,
      needsPassword = FALSE,
      needsPath = FALSE;
   
   switch(type)
   {
   case ET_IMAP:
      entry = _("n IMAP mailbox");
      needsServer = TRUE;
      needsUserId = TRUE;
      needsPassword = TRUE;
      needsPath = TRUE;
      break;
   case ET_POP3:
      entry = _(" POP3 mailbox");
      needsServer = TRUE;
      needsUserId = TRUE;
      needsPassword = TRUE;
      break;
   case ET_NNTP:
      entry = _(" NNTP newsgroup");
      needsServer = TRUE;
      needsPath = TRUE;
      pathName = _("Newsgroup");
      break;
   case ET_IMAP_SERVER:
      entry = _("n IMAP server");
      needsServer = TRUE;
      needsUserId = TRUE;
      needsPassword = TRUE;
      break;
   case ET_NNTP_SERVER:
      entry = _(" NNTP newsserver");
      needsServer = TRUE;
      break;
   case ET_IMAP_HIER:
      entry = _("n IMAP mail hierarchy");
      needsServer = TRUE;
      needsUserId = TRUE;
      needsPassword = TRUE;
      needsPath = TRUE;
      pathName = _("Path on IMAP server");
      break;
   case ET_NNTP_HIER:
      entry = _(" NNTP news hierarchy");
      needsServer = TRUE;
      needsPath = TRUE;
      pathName = _("Hierarchy name");
      break;
   case ET_NEWS:
      entry = _(" local newsgroup");
      needsPath = TRUE;
      pathName = _("Newsgroup");
      break;
   case ET_NEWS_HIER:
      entry = _(" local news hierarchy");
      needsServer = FALSE;
      needsPath = TRUE;
      pathName = _("Hierarchy name");
      break;
   case ET_FILE:
      entry = _(" local mailbox file");
      needsPath = TRUE;
      break;
   case ET_MH:
      entry = _(" local MH mailbox");
      needsPath = TRUE;
      break;
   }

   wxString text;
   text.Printf(msg, entry.c_str());
      
   wxStaticText *msgCtrl = new wxStaticText(
      this, -1,text);
   
   wxEnhancedPanel *panel = CreateEnhancedPanel(msgCtrl);
   
   wxArrayString labels;
   labels.Add(_("Server Host"));
   labels.Add(_("Your Login"));
   labels.Add(_("Your Password"));
   labels.Add(pathName);
   labels.Add(_("Entry Name"));   
   long maxwidth = GetMaxLabelWidth(labels, panel->GetCanvas());

   wxTextCtrl *last = NULL;
   m_Server   = needsServer ?
      last = panel->CreateTextWithLabel(labels[0],maxwidth,last) : NULL; 
   m_UserId   = needsUserId ?
      last = panel->CreateTextWithLabel(labels[1],maxwidth,last) : NULL;
   m_Password = needsPassword ?
      last = panel->CreateTextWithLabel(labels[2],maxwidth,last, 0, wxPASSWORD) :
      NULL;
   m_Path     = needsPath ?
      panel->CreateFileOrDirEntry(labels[3], maxwidth,
                                  last,NULL,TRUE,FALSE) : NULL;
   wxStaticText * msg2 = panel->CreateMessage(
      _("You also need to give your new enty\n"
        "a name to appear in the tree.\n"
        "If you leave it empty, Mahogany\n"
        "will choose one for you."), m_Path);
   m_Name = panel->CreateTextWithLabel(labels[4],maxwidth, msg2);

   panel->Layout();
}

bool
MWizard_CreateFolder_ServerPage::TransferDataFromWindow()
{
   return TRUE;
}


// CreateFolderWizardXXXPage - the various folder types
// ----------------------------------------------------------------------------

#define DEFINE_TYPEPAGE(type,type2) \
class MWizard_CreateFolder_##type##Page : public \
  MWizard_CreateFolder_ServerPage \
{ \
public: \
   MWizard_CreateFolder_##type##Page(MWizard *wizard) \
      : MWizard_CreateFolder_ServerPage(wizard,\
                                        MWizard_CreateFolder_##type,\
                                        ET_##type2)\
 { } \
}

DEFINE_TYPEPAGE(File,FILE);
DEFINE_TYPEPAGE(MH,MH);
DEFINE_TYPEPAGE(Imap,IMAP);
DEFINE_TYPEPAGE(Pop3,POP3);
DEFINE_TYPEPAGE(ImapServer,IMAP_SERVER);
DEFINE_TYPEPAGE(NntpServer,NNTP_SERVER);
DEFINE_TYPEPAGE(ImapHier,IMAP_HIER);
DEFINE_TYPEPAGE(NntpHier,NNTP_HIER);
DEFINE_TYPEPAGE(Nntp,NNTP);
DEFINE_TYPEPAGE(News,NEWS);
DEFINE_TYPEPAGE(NewsHier,NEWS_HIER);
#undef DEFINE_TYPEPAGE

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
   if ( id == GetLastPageId() || id == MWizard_PageNone)
      return (MWizardPage *)NULL;

   if ( !m_WizardPages[id] )
   {
#define CREATE_PAGE(pid)                             \
      case MWizard_##pid##:                          \
         m_WizardPages[MWizard_##pid] =               \
            new MWizard_##pid##Page(this); \
         break

      switch ( id )
      {
         CREATE_PAGE(CreateFolder_Welcome);
         CREATE_PAGE(CreateFolder_Type);
         CREATE_PAGE(CreateFolder_Final);
         CREATE_PAGE(CreateFolder_File);
         CREATE_PAGE(CreateFolder_MH);
         CREATE_PAGE(CreateFolder_Imap);
         CREATE_PAGE(CreateFolder_Pop3);
         CREATE_PAGE(CreateFolder_ImapServer);
         CREATE_PAGE(CreateFolder_NntpServer);
         CREATE_PAGE(CreateFolder_ImapHier);
         CREATE_PAGE(CreateFolder_NntpHier);
         CREATE_PAGE(CreateFolder_Nntp);
         CREATE_PAGE(CreateFolder_News);
         CREATE_PAGE(CreateFolder_NewsHier);
      case MWizard_PageNone:
         ASSERT_MSG(0,"illegal MWizard PageId");
      }
#undef CREATE_PAGE
   }
   return m_WizardPages[id];
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
