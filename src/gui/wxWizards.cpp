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
#   include "MFolder.h"
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

   MWizard_PageNone = 12345678          // illegal value
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
   virtual MWizard * GetWizard() const { return (MWizard *)m_Wizard; }
   
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



// The CreateFolderWizard
//
// ----------------------------------------------------------------------------

class CreateFolderWizard : public MWizard
{
public:
   CreateFolderWizard(MFolder *parentFolder,
                      wxWindow *parent = NULL)
      : MWizard( MWizardType_CreateFolder,
                 MWizard_CreateFolder_First,
                 MWizard_CreateFolder_Last,
                 _("Create a Mailbox Entry"),
                 NULL, parent)
      {
         m_ParentFolder = parentFolder;
         SafeIncRef(m_ParentFolder);
      }
   ~CreateFolderWizard()
      {
         SafeDecRef(m_ParentFolder);
      }
   struct FolderParams
   {
      int m_FolderType;
      int m_FolderFlags;
      wxString m_Name;
      wxString m_Path;
      wxString m_Server;
      wxString m_Login;
      wxString m_Password;
   };

   FolderParams * GetParams() { return &m_Params; }
   MFolder * GetPFolder() { return m_ParentFolder; }
protected:
   MFolder * m_ParentFolder;
   FolderParams m_Params;
};


// CreateFolderWizardWelcomePage
// ----------------------------------------------------------------------------

// first page: welcome the user, explain what goes on
class MWizard_CreateFolder_WelcomePage : public MWizardPage
{
public:
   MWizard_CreateFolder_WelcomePage(MWizard *wizard);
   virtual MWizardPageId GetPreviousPageId() const
      { return MWizard_PageNone; }
private:
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
      "If you decide not to use this dialog,\n"
      "just press [Cancel] at any time and\n"
      "you can create the entry manually."
      ));
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
   virtual bool TransferDataToWindow();
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

#define CREATE_CTRL(name, creat) \
if(needs##name) { m_##name = creat; last = m_##name; } else m_##name = NULL
   
   wxControl *last = NULL;
   CREATE_CTRL(Server,
               panel->CreateTextWithLabel(labels[0],maxwidth,last)); 
   CREATE_CTRL(UserId,
               panel->CreateTextWithLabel(labels[1],maxwidth,last)); 
   CREATE_CTRL(Password,
               panel->CreateTextWithLabel(labels[2],maxwidth,last, 0,
                                          wxPASSWORD)); 
   CREATE_CTRL(Path, panel->CreateFileOrDirEntry(labels[3], maxwidth,
                                  last,NULL,TRUE,FALSE));
#undef CREATE_CTRL
   wxStaticText * msg2 = panel->CreateMessage(
      _("\n"
        "You also need to give your new enty\n"
        "a name to appear in the tree."
        ), m_Path);
   m_Name = panel->CreateTextWithLabel(labels[4],maxwidth, msg2);

   panel->Layout();
}

bool
MWizard_CreateFolder_ServerPage::TransferDataFromWindow()
{
   CreateFolderWizard::FolderParams *params =
      ((CreateFolderWizard*)GetWizard())->GetParams();
   
   if(m_Name && m_Name->GetValue() != params->m_Name)
      params->m_Name = m_Name->GetValue();
   if(m_Path && params->m_Path != m_Path->GetValue())
      params->m_Path = m_Path->GetValue();
   if(m_Server && params->m_Server != m_Server->GetValue())
      params->m_Server = m_Server->GetValue();
   if(m_UserId && params->m_Login != m_UserId->GetValue())
      params->m_Login = m_UserId->GetValue();
   if(m_Password && params->m_Password != m_Password->GetValue())
   {
      wxString msg;
      msg = _("You have specified a password here, which is slightly unsafe.\n");
      msg << _("\n"
               "Notice that the password will be stored in your configuration with\n"
               "very weak encryption. If you are concerned about security, leave it\n"
               "empty and Mahogany will prompt you for it whenever needed.");
      msg << _("\nDo you want to save the password anyway?");
      if ( MDialog_YesNoDialog(msg, this, MDIALOG_YESNOTITLE, true, "AskPwd") )
         params->m_Password = m_Password->GetValue();
   }

   params->m_FolderType = 0;
   params->m_FolderFlags = MF_FLAGS_DEFAULT;
   
   switch(m_Type)
   {
   case ET_IMAP:
   case ET_IMAP_SERVER:
   case ET_IMAP_HIER:
      params->m_FolderType = MF_IMAP;
      if(m_Type == ET_IMAP_HIER)
         params->m_FolderFlags |= MF_FLAGS_GROUP;
      break;
   case ET_POP3:
      params->m_FolderType = MF_POP;
      break;
   case ET_NNTP:
   case ET_NNTP_SERVER:
   case ET_NNTP_HIER:
      params->m_FolderType = MF_NNTP;
      params->m_FolderFlags |= MF_FLAGS_ANON; // by default
      if(m_Type == ET_NNTP_HIER)
         params->m_FolderFlags |= MF_FLAGS_GROUP;
      break;
   case ET_NEWS:
   case ET_NEWS_HIER:
      params->m_FolderType = MF_NEWS;
      if(m_Type == ET_NEWS_HIER)
         params->m_FolderFlags |= MF_FLAGS_GROUP;
      break;
   case ET_FILE:
      params->m_FolderType = MF_FILE;
      break;
   case ET_MH:
      params->m_FolderType = MF_MH;
      break;
   }
   return TRUE;
}

bool
MWizard_CreateFolder_ServerPage::TransferDataToWindow()
{
   MFolder *f = ((CreateFolderWizard*)GetWizard())->GetPFolder();
   ProfileBase * p = ProfileBase::CreateProfile(f->GetName());
   CHECK(p,FALSE,"No profile?");
      
   m_Name->SetValue(_("New Folder"));
   if(GetPageId() != MWizard_CreateFolder_ImapServer
      && GetPageId() != MWizard_CreateFolder_NntpServer
      && GetPageId() != MWizard_CreateFolder_Pop3)
      m_Path->SetValue( READ_CONFIG(p,MP_FOLDER_PATH));

   if(GetPageId() != MWizard_CreateFolder_MH
      && GetPageId() != MWizard_CreateFolder_File
      && GetPageId() != MWizard_CreateFolder_News
      && GetPageId() != MWizard_CreateFolder_NewsHier
      )
   {
      switch(GetPageId())
      {
      case MWizard_CreateFolder_Imap:
      case MWizard_CreateFolder_ImapServer:
      case MWizard_CreateFolder_ImapHier:
         m_Server->SetValue(READ_CONFIG(p, MP_IMAPHOST));
         break;
      case MWizard_CreateFolder_Nntp:
      case MWizard_CreateFolder_NntpServer:
      case MWizard_CreateFolder_NntpHier:
         m_Server->SetValue(READ_CONFIG(p, MP_NNTPHOST));
         break;
      case MWizard_CreateFolder_Pop3:
         m_Server->SetValue(READ_CONFIG(p, MP_POPHOST));
         break;
      default:
         ASSERT_MSG(0,"This folder has no server setting!");
      }
      if(GetPageId() != MWizard_CreateFolder_Nntp
         && GetPageId() != MWizard_CreateFolder_News
         && GetPageId() != MWizard_CreateFolder_NntpHier
         && GetPageId() != MWizard_CreateFolder_NewsHier)
      {
         m_UserId->SetValue(READ_CONFIG(p, MP_USERNAME));
         m_Password->SetValue(READ_CONFIG(p,
                                          MP_FOLDER_PASSWORD));
      }
   }
   p->DecRef();
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
      "You have now specified the basic\n"
      "parameters for the new mailbox entry.\n"
      "\n"
      "If you want to use any more advanced\n"
      "access options, you can modify the\n"
      "complete set of parameters by invoking\n"
      "the folder properties dialog. Simply\n"
      "click the right mouse button on the\n"
      "entry in the tree."));
}


// ============================================================================
// implementation
// ============================================================================


MWizardPage *
MWizard::GetPageById(MWizardPageId id)
{
   if ( id == GetLastPageId()+1 || id == MWizard_PageNone)
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




MFolder *
RunCreateFolderWizard(MFolder *parent, wxWindow *parentWin)
{
   CHECK(parent,NULL, "No parent folder?");
   CreateFolderWizard *wizard = new CreateFolderWizard(parent, parentWin);
   MFolder *newfolder = NULL;
   if(wizard->Run() == TRUE)
   {
      CreateFolderWizard::FolderParams *params =
         wizard->GetParams();
      newfolder = CreateFolderTreeEntry(
         parent, params->m_Name,
         (FolderType) params->m_FolderType,
         params->m_FolderFlags,
         params->m_Path, TRUE);
      if(newfolder)
      {
         ProfileBase *p =
            ProfileBase::CreateProfile(newfolder->GetName());
         p->writeEntry(MP_FOLDER_PASSWORD, params->m_Password);
         FolderType type = (FolderType) params->m_FolderType;
         switch(type)
         {
         case MF_IMAP:
            p->writeEntry(MP_IMAPHOST, params->m_Server);
         case MF_NNTP:
            p->writeEntry(MP_NNTPHOST, params->m_Server);
            break;
         case MF_POP:
            p->writeEntry(MP_POPHOST, params->m_Server);
            break;
         default:
            ; // nothing
         }
         if(FolderTypeHasUserName(type))
         {
            p->writeEntry(MP_USERNAME, params->m_Login);
            p->writeEntry(MP_FOLDER_PASSWORD, params->m_Password);
         }
         p->DecRef();
      }
   }
   delete wizard;
   return newfolder;
}
