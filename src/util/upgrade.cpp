///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   upgrade.cpp - functions to upgrade from previous version of M
// Purpose:
// Author:      Vadim Zeitlin
// Modified by:
// Created:     20.08.98
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
#include "Mpch.h"

#ifndef   USE_PCH
#  include "Mcommon.h"
#  include "PathFinder.h"
#  include "Profile.h"
#  include "MApplication.h"
#  include "strutil.h"
#endif  //USE_PCH

#  include "Message.h"
#  include "MailFolder.h"
#  include "MailFolderCC.h"
#  include "SendMessageCC.h"

#ifdef USE_PYTHON
#  include <Python.h>
#  include "PythonHelp.h"
#  include "MScripts.h"
#endif

#include "Mdefaults.h"

#include <wx/log.h>
#include <wx/confbase.h>
#include <wx/utils.h>         // wxGetFullHostName()
#include <wx/dialup.h>        // for IsAlwaysOnline()

#define USE_WIZARD

#ifdef USE_WIZARD
   #include <wx/wizard.h>
#else
   #include "gui/wxOptionsDlg.h" // for ShowOptionsDialog()
#endif

#include "gui/wxDialogLayout.h"

#include "Mupgrade.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// this enum contains only versions with incompatible changes between them
enum MVersion
{
   Version_None,     // this is the first installation of M on this machine
   Version_Alpha001, // first public version
   Version_Alpha010, // some config strucutre changes (due to wxPTextEntry)
   Version_Alpha020, // folder host name is now ServerName, not HostName
   Version_NoChange, // any version from which we don't need to upgrade
   Version_Unknown   // some unrecognized version
};

#ifdef USE_WIZARD

#ifndef OS_WIN
   #define USE_HELPERS_PAGE
#endif // OS_WIN

// ids for install wizard pages
enum InstallWizardPageId
{
   InstallWizard_WelcomePage,          // say hello
   InstallWizard_IdentityPage,         // ask name, e-mail
   InstallWizard_ServersPage,          // ask POP, SMTP, NNTP servers
   InstallWizard_DialUpPage,           // set up dial-up networking
   InstallWizard_MiscPage,             // signature, other common options
#ifdef USE_HELPERS_PAGE
   InstallWizard_HelpersPage,          // external programs set up
#endif // USE_HELPERS_PAGE
   InstallWizard_ImportPage,           // propose to import ADBs (and folders)
   InstallWizard_FinalPage,            // say that everything is ok
   InstallWizard_PagesMax,             // the number of pages
   InstallWizard_Done = -1             // invalid page index
};

// ----------------------------------------------------------------------------
// wizardry
// ----------------------------------------------------------------------------

static wxWizardPage *gs_wizardPages[InstallWizard_PagesMax];

// the data which is collected by the wizard
//
// FIXME for now I just use a global - this is ugly, but I just can't find a
//       place to stick it into right now
struct InstallWizardData
{
   // identity page
   String name;         // personal name, not login one (we can find it out)
   String email;

   // servers page
   String pop,
          imap,
          smtp,
          nntp;

   // misc page
   bool   isSigFile;    // TRUE => signature is a filename,
   String signature;    // FALSE => it's a signature itself

   // dial up page
   bool   useDialUp;

#if defined(OS_WIN)
   String connection;
#elif defined(OS_UNIX)
   String dialCommand,
          hangupCommand;

   // helpers page
   String browser;
#endif // OS_UNIX
} gs_installWizardData;

// the base class for our wizards pages: it allows to return page ids (and not
// the pointers themselves) from GetPrev/Next and processes [Cancel] in a
// standard way an provides other useful functions for the derived classes
class InstallWizardPage : public wxWizardPage
{
public:
   InstallWizardPage(wxWizard *wizard, InstallWizardPageId id)
      : wxWizardPage(wizard) { m_id = id; }

   // override to return the id of the next/prev page: default versions go to
   // the next/prev in order of InstallWizardPageIds
   virtual InstallWizardPageId GetPrevPageId() const;
   virtual InstallWizardPageId GetNextPageId() const;

   // the dial up page should be shown only if this function returns TRUE
   static bool ShouldShowDialUpPage();

   // implement the wxWizardPage pure virtuals in terms of our ones
   virtual wxWizardPage *GetPrev() const
      { return GetPageById(GetPrevPageId()); }
   virtual wxWizardPage *GetNext() const
      { return GetPageById(GetNextPageId()); }

   void OnWizardCancel(wxWizardEvent& event);

protected:
   wxWizardPage *GetPageById(InstallWizardPageId id) const;

   // creates an "enhanced panel" for placing controls into under the static
   // text (explanation)
   wxEnhancedPanel *CreateEnhancedPanel(wxStaticText *text);

private:
   // id of this page
   InstallWizardPageId m_id;

   // true/false as usual, -1 if not tested for it yet
   static int ms_isUsingDialUp;

   DECLARE_EVENT_TABLE()
};

// first page: welcome the user, explain what goes on
class InstallWizardWelcomePage : public InstallWizardPage
{
public:
   InstallWizardWelcomePage(wxWizard *wizard);

   // the next page depends on whether the user want or not to use the wizard
   virtual InstallWizardPageId GetNextPageId() const;

   virtual bool TransferDataFromWindow();

private:
   wxCheckBox *m_checkbox;
   bool        m_useWizard;
};

// second page: ask the basic info about the user (name, e-mail)
class InstallWizardIdentityPage : public InstallWizardPage
{
public:
   InstallWizardIdentityPage(wxWizard *wizard);

   virtual bool TransferDataFromWindow()
   {
      gs_installWizardData.name = m_name->GetValue();
      gs_installWizardData.email = m_email->GetValue();

      return TRUE;
   }

private:
   wxTextCtrl *m_name,
              *m_email;
};

class InstallWizardServersPage : public InstallWizardPage
{
public:
   InstallWizardServersPage(wxWizard *wizard);

   virtual bool TransferDataFromWindow()
   {
      return TRUE;
   }

private:
   wxTextCtrl *m_pop,
              *m_imap,
              *m_smtp,
              *m_nntp;
};

class InstallWizardDialUpPage : public InstallWizardPage
{
public:
   InstallWizardDialUpPage(wxWizard *wizard);

   virtual bool TransferDataFromWindow()
   {
      return TRUE;
   }
};

#ifdef USE_HELPERS_PAGE

class InstallWizardHelpersPage : public InstallWizardPage
{
public:
   InstallWizardHelpersPage(wxWizard *wizard);

   virtual bool TransferDataFromWindow()
   {
      return TRUE;
   }
};

#endif // USE_HELPERS_PAGE

class InstallWizardMiscPage : public InstallWizardPage
{
public:
   InstallWizardMiscPage(wxWizard *wizard);

   virtual bool TransferDataFromWindow()
   {
      return TRUE;
   }
};

class InstallWizardImportPage : public InstallWizardPage
{
public:
   InstallWizardImportPage(wxWizard *wizard);

   virtual bool TransferDataFromWindow()
   {
      return TRUE;
   }
};

class InstallWizardFinalPage : public InstallWizardPage
{
public:
   InstallWizardFinalPage(wxWizard *wizard);
};

// ----------------------------------------------------------------------------
// prototypes
// ----------------------------------------------------------------------------

// the function which runs the install wizard
extern bool RunInstallWizard();

#endif // USE_WIZARD

// ============================================================================
// implementation
// ============================================================================

#ifdef USE_WIZARD

// ----------------------------------------------------------------------------
// wizard pages code
// ----------------------------------------------------------------------------

// InstallWizardPage
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(InstallWizardPage, wxWizardPage)
   EVT_WIZARD_CANCEL(-1, InstallWizardPage::OnWizardCancel)
END_EVENT_TABLE()

int InstallWizardPage::ms_isUsingDialUp = -1;

bool InstallWizardPage::ShouldShowDialUpPage()
{
   if ( ms_isUsingDialUp == -1 )
   {
      wxDialUpManager *man = wxDialUpManager::Create();

      // if we have a LAN connection, then we don't need to configure dial-up
      // netowrking, but if we don't, then we probably do
      ms_isUsingDialUp = !man->IsAlwaysOnline();

      delete man;
   }

   return ms_isUsingDialUp != 0;
}

InstallWizardPageId InstallWizardPage::GetPrevPageId() const
{
   int id = m_id - 1;
   if ( id == InstallWizard_DialUpPage && !ShouldShowDialUpPage() )
   {
      // skip it
      id--;
   }

   return id < 0 ? InstallWizard_Done : (InstallWizardPageId)id;
}

InstallWizardPageId InstallWizardPage::GetNextPageId() const
{
   int id = m_id + 1;
   if ( id == InstallWizard_DialUpPage && !ShouldShowDialUpPage() )
   {
      // skip it
      id++;
   }

   return id < InstallWizard_PagesMax ? (InstallWizardPageId)id
                                      : InstallWizard_Done;
}

void InstallWizardPage::OnWizardCancel(wxWizardEvent& event)
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

wxWizardPage *InstallWizardPage::GetPageById(InstallWizardPageId id) const
{
   if ( id == InstallWizard_Done )
      return (wxWizardPage *)NULL;

   if ( !gs_wizardPages[id] )
   {
#define CREATE_PAGE(id)                                              \
      case InstallWizard_##id##Page:                              \
         gs_wizardPages[InstallWizard_##id##Page] =               \
            new InstallWizard##id##Page((wxWizard *)GetParent()); \
         break

      switch ( id )
      {
         CREATE_PAGE(Identity);
         CREATE_PAGE(Servers);
         CREATE_PAGE(DialUp);
         CREATE_PAGE(Misc);
#ifdef USE_HELPERS_PAGE
         CREATE_PAGE(Helpers);
#endif // USE_HELPERS_PAGE
         CREATE_PAGE(Import);
         CREATE_PAGE(Final);
      }

#undef CREATE_PAGE
   }

   return gs_wizardPages[id];
}

wxEnhancedPanel *InstallWizardPage::CreateEnhancedPanel(wxStaticText *text)
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

// InstallWizardWelcomePage
// ----------------------------------------------------------------------------

InstallWizardWelcomePage::InstallWizardWelcomePage(wxWizard *wizard)
                        : InstallWizardPage(wizard, InstallWizard_WelcomePage)
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
//      sizePage = wizard->GetSize();
   
   // adjust the vertical position
   m_checkbox->Move(-1, sizePage.y - 2*sizeBox.y);
}

bool InstallWizardWelcomePage::TransferDataFromWindow()
{
   m_useWizard = !m_checkbox->GetValue();

   return TRUE;
}

InstallWizardPageId InstallWizardWelcomePage::GetNextPageId() const
{
   return m_useWizard ? InstallWizard_IdentityPage : InstallWizard_Done;
}

// InstallWizardIdentityPage
// ----------------------------------------------------------------------------

InstallWizardIdentityPage::InstallWizardIdentityPage(wxWizard *wizard)
                         : InstallWizardPage(wizard, InstallWizard_IdentityPage)
{
   wxStaticText *text = new wxStaticText(this, -1, _(
         "Please specify your name and e-mail address:\n"
         "they will be used for sending the messages"
                                     ));

   wxEnhancedPanel *panel = CreateEnhancedPanel(text);

   wxArrayString labels;
   labels.Add(_("&Personal name:"));
   labels.Add(_("&E-mail:"));

   long widthMax = GetMaxLabelWidth(labels, panel);

   m_name = panel->CreateTextWithLabel(labels[0], widthMax, NULL);
   m_email = panel->CreateTextWithLabel(labels[1], widthMax, m_name);

   panel->Layout();
}

// InstallWizardServersPage
// ----------------------------------------------------------------------------

InstallWizardServersPage::InstallWizardServersPage(wxWizard *wizard)
                        : InstallWizardPage(wizard, InstallWizard_ServersPage)
{
   wxStaticText *text = new wxStaticText(this, -1, _(
         "To receive e-mail, you need to have at least one\n"
         "mail server. Mail server may use POP3 or IMAP4,\n"
         "but you need only one of them (IMAP4 is better).\n"
         "\n"
         "To read Usenet discussion groups, you need to\n"
         "specify the NNTP (news) server and to be able to\n"
         "send e-mail, SMTP server is required.\n"
         "\n"
         "All of these fields may be filled later as well\n"
         "(and you will be able to specify multiple servers too)"
                                     ));

   wxEnhancedPanel *panel = CreateEnhancedPanel(text);

   wxArrayString labels;
   labels.Add(_("&POP server:"));
   labels.Add(_("&IMAP server:"));
   labels.Add(_("&SMTP server:"));
   labels.Add(_("&NNTP server:"));

   long widthMax = GetMaxLabelWidth(labels, panel);

   m_pop = panel->CreateTextWithLabel(labels[0], widthMax, NULL);
   m_imap = panel->CreateTextWithLabel(labels[1], widthMax, m_pop);
   m_smtp = panel->CreateTextWithLabel(labels[2], widthMax, m_imap);
   m_nntp = panel->CreateTextWithLabel(labels[3], widthMax, m_smtp);

   panel->Layout();
}

// InstallWizardDialUpPage
// ----------------------------------------------------------------------------

InstallWizardDialUpPage::InstallWizardDialUpPage(wxWizard *wizard)
                       : InstallWizardPage(wizard, InstallWizard_DialUpPage)
{
   // TODO ask for the connection name under Windows and commands for Unix
   new wxStaticText(this, -1, "This page is under construction");
}

#ifdef USE_HELPERS_PAGE

// InstallWizardHelpersPage
// ----------------------------------------------------------------------------

InstallWizardHelpersPage::InstallWizardHelpersPage(wxWizard *wizard)
                        : InstallWizardPage(wizard, InstallWizard_HelpersPage)
{
   // TODO ask for the web browser
   new wxStaticText(this, -1, "This page is under construction");
}

#endif // USE_HELPERS_PAGE

// InstallWizardMiscPage
// ----------------------------------------------------------------------------

InstallWizardMiscPage::InstallWizardMiscPage(wxWizard *wizard)
                     : InstallWizardPage(wizard, InstallWizard_MiscPage)
{
   // TODO ask for signature (either file or text)
   new wxStaticText(this, -1, "This page is under construction");
}

// InstallWizardImportPage
// ----------------------------------------------------------------------------

InstallWizardImportPage::InstallWizardImportPage(wxWizard *wizard)
                       : InstallWizardPage(wizard, InstallWizard_ImportPage)
{
   // TODO autodetect all mailers installed and propose to import as much as
   //      possible.
   //
   // NB: use an external function for this, this functionality should be
   //     separate from the update module
   new wxStaticText(this, -1, "This page is under construction");
}

// InstallWizardFinalPage
// ----------------------------------------------------------------------------

InstallWizardFinalPage::InstallWizardFinalPage(wxWizard *wizard)
                      : InstallWizardPage(wizard, InstallWizard_FinalPage)
{
   (void)new wxStaticText(this, -1, _(
      "Congratulations! You have successfully finished configuring\n"
      "Mahogany and may now start using it.\n"
      "\n"
      "Please remember to consult online help and the documentaion\n"
      "files included in the distribution in case of problems and,\n"
      "of course, remember about Mahogany users mailing list.\n"
      "\n"
      "\n"
      "\n"
      "We hope you will enjoy Mahogany!\n"
      "                    M-Team"
                                     ));
}

#endif // USE_WIZARD

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

#ifdef USE_WIZARD

bool RunInstallWizard()
{
   // as we use a static array, make sure that only one install wizard is
   // running at any time (a sensible requirment anyhow)
   static bool gs_isWizardRunning = false;

   if ( gs_isWizardRunning )
   {
      FAIL_MSG( "the installation wizard is already running!" );

      return false;
   }

   gs_isWizardRunning = true;

   wxIconManager *iconManager = mApplication->GetIconManager();
   wxWizard *wizard = wxWizard::Create
                      (
                        NULL,                         // parent
                        -1,                           // id
                        _("Mahogany Installation"),   // title
                        iconManager->GetBitmap("install_welcome") // def image
                      );

   // NULL the pages array
   memset(gs_wizardPages, 0, sizeof(gs_wizardPages));

   gs_wizardPages[InstallWizard_WelcomePage] = new InstallWizardWelcomePage(wizard);
   if ( wizard->RunWizard(gs_wizardPages[InstallWizard_WelcomePage]) )
   {
      // transfer the wizard settings from InstallWizardData

      // TODO
   }

   wizard->Destroy();

   gs_isWizardRunning = false;

   return true;
}

#endif // USE_WIZARD

static bool
UpgradeFromNone()
{
#ifdef USE_WIZARD // new code which uses the configuration wizard
   (void)RunInstallWizard();
#else // old code which didn't use the setup wizard
   wxLog *log = wxLog::GetActiveTarget();
   if ( log ) {
      wxLogMessage(_("As it seems that you are running Mahogany for the first\n"
                     "time, you should probably set up some of the options\n"
                     "needed by the program (especially network parameters)."));
      log->Flush();
   }

   ShowOptionsDialog();
#endif

   SetupInitialConfig();

   return true;
}

static bool
UpgradeFrom001()
{
   // Config structure incompatible changes from 0.01a to 0.02a:
   // 1) last prompts stored as 0, 1, 2, ... subkeys of /Prompts/foo instead
   //    of a single value /Prompts/foo.
   // 2) last values of the adb editor search control stored as subkeys of
   //    /AdbEditor/LastSearch instead of a single value with this name.
   // 3) /AdbEditor/LastPage is /AdbEditor/Notebook/Page
   // 4) /AdbEditor/LastAdb{Dir|File} => AdbFilePrompt/AdbFilePromptPath

   // TODO
   return true;
}

#define COPYENTRY(type)  { type val; rc &= _this->Read(entry, &val); rc &= dest->Write(newentry,val); }

/** Copies all entries and optionally subgroups from path from to path
    to in the wxConfig.
    NOTE: Currently both from and to should be absolute paths!
    @param from  absolute group from where to copy entries
    @param to    absolute group where to copy to
    @param recursive if true, copy all subgroups, too
    @param dest      if non-NULL, use that config as the destination otherwise use this
    @return false on error
*/
static bool
CopyEntries(wxConfigBase *_this,
            const wxString &from, const wxString &to,
            bool recursive = TRUE,
            wxConfigBase *dest = NULL)
{
   wxString oldPath = _this->GetPath();
   bool rc = TRUE;

   if(dest == NULL) dest = _this;

   // Build a list of all entries to copy:
   _this->SetPath(from);

   long
      index = 0;
   wxString
      entry, newentry;
   bool ok;
   for ( ok = _this->GetFirstEntry(entry, index);
         ok ;
         ok = _this->GetNextEntry(entry, index))
   {
      newentry = to;
      newentry << '/' << entry;
      switch(_this->GetEntryType(entry))
      {
      case wxConfigBase::Type_String:
         COPYENTRY(wxString); break;
      case wxConfigBase::Type_Integer:
         COPYENTRY(long); break;
      case wxConfigBase::Type_Float:
         COPYENTRY(double); break;
      case wxConfigBase::Type_Boolean:
         COPYENTRY(bool); break;
      case wxConfigBase::Type_Unknown:
         wxFAIL_MSG("unexpected entry type");
      }
   }
   if(recursive)
   {
      wxString
         fromgroup, togroup;
      wxString
         *groups;

      size_t
         idx = 0,
         n = _this->GetNumberOfGroups(FALSE);
      if(n > 0)
      {
         groups = new wxString[n];
         index = 0;
         for ( ok = _this->GetFirstGroup(entry, index);
               ok ;
               ok = _this->GetNextGroup(entry, index))
         {
            wxASSERT(idx < n);
            groups[idx++] = entry;
         }
         for(idx = 0; idx < n; idx++)
         {
            fromgroup = from;
            fromgroup << '/' << groups[idx];
            togroup = to;
            togroup << '/' << groups[idx];
            rc &= CopyEntries(_this, fromgroup, togroup, recursive, dest);
         }
         delete [] groups;
      }
   }

   _this->SetPath(oldPath);
   return rc;
}

static bool
UpgradeFrom010()
{
   /* Config structure incompatible changes from 0.10a to 0.20a:
      New Profile system. Visible change is that the
      M/Profile/Folders/ hierarchy no longer exists, entries from
      M/Profile/Folders/xxx must be merged into M/Profile/xxx.

      Passwords are now encrypted using an extremly simple encryption
      algorithms. Totally unsafe but better than cleartext.
    */

   bool rc = true;

   //FIXME paths need adjustment for windows?
   rc &= CopyEntries(mApplication->GetProfile()->GetConfig(),
                     "/M/Profiles/Folders","/M/Profiles", true);

   rc &= CopyEntries(mApplication->GetProfile()->GetConfig(),
                     "/AdbEditor","/M/Profiles/AdbEditor", true);

   ProfileBase
      *p = ProfileBase::CreateProfile(""),
      *p2;
   kbStringList
      folders;
   String
      group, pw, tmp;
   long
      index = 0;
   // We need to rename the old mainfolder, to remove its leading
   // slash:
   String
      mainFolder = p->readEntry(MP_MAINFOLDER,MP_MAINFOLDER_D);
   if(mainFolder.Length())
   {
      if(mainFolder[0u] == '/')
      {
         wxString tmp = mainFolder.Mid(1);
         mainFolder = tmp;
         p->writeEntry(MP_MAINFOLDER, mainFolder);
      }
   }

   //FIXME paths need adjustment for windows?
   wxConfigBase *c = mApplication->GetProfile()->GetConfig();
   // Delete obsolete groups:
   c->DeleteGroup("/M/Profiles/Folders");
   c->DeleteGroup("/AdbEditor");

   /* Encrypt passwords in new location and make sure we have no
      illegal old profiles around. */
   p->ResetPath(); // to be safe
   for ( bool ok = p->GetFirstGroup(group, index);
         ok ;
         ok = p->GetNextGroup(group, index))
   {
      tmp = group;
      tmp << '/' << MP_PROFILE_TYPE;
      if(p->readEntry(tmp, MP_PROFILE_TYPE_D) == ProfileBase::PT_FolderProfile)
         folders.push_back(new String(group));
   }
   for(kbStringList::iterator i = folders.begin(); i != folders.end();i++)
   {
      group = **i;
      p->SetPath(group);
      if(p->readEntry(MP_FOLDER_TYPE, MP_FOLDER_TYPE_D) != MP_FOLDER_TYPE_D)
      {
         p2 = ProfileBase::CreateProfile(group);
         pw = p2->readEntry(MP_FOLDER_PASSWORD, MP_FOLDER_PASSWORD_D);
         if(pw.Length()) // only if we have a password
            p2->writeEntry(MP_FOLDER_PASSWORD, strutil_encrypt(pw));
         p2->DecRef();
         p->ResetPath();
      }
      else
      {
         p->ResetPath();
         p->DeleteGroup(group);
         String msg = _("Deleted illegal folder profile: '");
         msg << p->GetName() << '/' << group << '\'';
         wxLogMessage(msg);
      }
   }
   p->DecRef();
   //FIXME check returncodes!


   return rc;

}



class UpgradeFolderTraversal : public MFolderTraversal
{
public:
   UpgradeFolderTraversal(MFolder* folder) : MFolderTraversal(*folder)
      { }

   virtual bool OnVisitFolder(const wxString& folderName)
      {
         Profile_obj profile(folderName);
         bool found;
         String hostname = profile->readEntry(MP_OLD_FOLDER_HOST, "", &found);
         if ( found )
         {
            // delete the old entry, create the new one
            wxConfigBase *config = profile->GetConfig();
            if ( config )
            {
               config->DeleteEntry(MP_OLD_FOLDER_HOST);

               wxLogTrace("Successfully converted folder '%s'",
                          folderName.c_str());
            }
            else
            {
               // what can we do? nothing...
               FAIL_MSG( "profile without config - can't delete entry" );
            }

            profile->writeEntry(MP_FOLDER_HOST, hostname);
         }

         return TRUE;
      }
};

static bool
UpgradeFrom020()
{
   /* Config structure incompatible changes from 0.20a to 0.21a:
      in the folder settings, HostName has become ServerName
    */

   // enumerate all folders recursively
   MFolder_obj folderRoot("");
   UpgradeFolderTraversal traverse(folderRoot);
   traverse.Traverse();

   // TODO it would be very nice to purge the redundant settings from config
   //      because the older versions wrote everything to it and it has only
   //      been fixed in 0.21a -- but this seems a bit too complicated

   return TRUE;
}

// ----------------------------------------------------------------------------
// global functions
// ----------------------------------------------------------------------------

// upgrade from the specified version to the current one
extern bool
Upgrade(const String& fromVersion)
{
   // what is the old version?
   MVersion oldVersion;
   if ( fromVersion.IsEmpty() )
      oldVersion = Version_None;
   else if ( fromVersion == "0.01a" )
      oldVersion = Version_Alpha001;
   else if ( fromVersion == "0.02a" || fromVersion == "0.10a")
      oldVersion = Version_Alpha010;
   else if ( fromVersion == "0.20a" )
      oldVersion = Version_Alpha020;
   else if ( fromVersion == "0.21a" || fromVersion == "0.22a" ||
             fromVersion == "0.23a")
      oldVersion = Version_NoChange;
   else
      oldVersion = Version_Unknown;

   bool success = TRUE;
   switch ( oldVersion )
   {
   case Version_None:
      UpgradeFromNone();
      break;

   case Version_Alpha001:
      if ( success )
         success = UpgradeFrom001();
      // fall through

   case Version_Alpha010:
      if ( success )
         success = UpgradeFrom010();
      // fall through

   case Version_Alpha020:
      if ( success && UpgradeFrom020() )
         wxLogMessage(_("Configuration information and program files were "
                        "successfully upgraded from the version '%s'."),
                      fromVersion.c_str());
      else
         wxLogError(_("Configuration information and program files "
                      "could not be upgraded from version '%s', some "
                      "settings might be lost.\n"
                      "\n"
                      "It is recommended that you uninstall and reinstall "
                      "the program before using it."),
                    fromVersion.c_str());
      break;
   case Version_NoChange:
      break;
   default:
      FAIL_MSG("invalid version value");
      // fall through

   case Version_Unknown:
      wxLogError(_("The previously installed version of Mahogany was "
                   "probably newer than this one. Cannot upgrade."));
      return FALSE;
   }

   return TRUE;
}




/// only used to find new mail folder
class NewMailFolderTraversal : public MFolderTraversal
{
public:
   NewMailFolderTraversal(MFolder* folder) : MFolderTraversal(*folder)
      { }

   String GetNewMailFolder(void) const { return m_NewMailFolder; }
   bool OnVisitFolder(const wxString& folderName)
      {
         MFolder *f = MFolder::Get(folderName);
         if(f && f->GetFlags() & MF_FLAGS_NEWMAILFOLDER)
         {
            if(m_NewMailFolder.Length())
            {
               ERRORMESSAGE((_(
                  "Found additional folder '%s'\n"
                  "marked as central new mail folder. Ignoring it."),
                             f->GetName().c_str()));
               f->SetFlags(f->GetFlags() & !MF_FLAGS_NEWMAILFOLDER);
            }
            else
            {
               m_NewMailFolder = f->GetName();
               if(f->GetFlags() & MF_FLAGS_INCOMING)
               {
                  ERRORMESSAGE((_("Cannot auto-collect mail from the new mail folder\n"
                                  "'%s'\n"
                                  "Corrected configuration data."),
                                f->GetName().c_str()));
                  f->SetFlags(f->GetFlags() & !MF_FLAGS_INCOMING);
               }
            }
         }
         if(f) f->DecRef();
         return true;
      }
private:
   String m_NewMailFolder;
};

/** Make sure we have /Profiles/INBOX set up and the global
    NewMailFolder folder.
    Returns TRUE if the profile already existed, FALSE if it was just created
 */
extern bool
VerifyInbox(void)
{
   bool rc = TRUE;
   ProfileBase *parent = mApplication->GetProfile();
   // INBOX has no meaning under Windows
#ifndef OS_WIN
   // Do we need to create the INBOX (special folder for incoming mail)?
   if ( parent->HasEntry("INBOX") )
      rc = TRUE;
   else
   {
      rc = FALSE;
      ProfileBase *ibp = ProfileBase::CreateProfile("INBOX");
      ibp->writeEntry(MP_PROFILE_TYPE, ProfileBase::PT_FolderProfile);
      if(READ_APPCONFIG(MP_NEWMAIL_FOLDER) != "INBOX"
         && MDialog_YesNoDialog(
            _("Normally Mahogany will automatically collect all mail\n"
              "from your system's default mail folder (INBOX,\n"
              "representing your mail spool entry),\n"
              "and move it to a special 'New Mail' folder.\n"
              "This is generally considered good practice as you\n"
              "should not leave your new mail in INBOX.\n"
              "\n"
              "Please confirm this. If you select No, your mail\n"
              "will remain in INBOX and you need to check that\n"
              "folder manually."
               ),
            NULL, _("Collect mail from INBOX?"), true))
         ibp->writeEntry(MP_FOLDER_TYPE, MF_INBOX|MF_FLAGS_INCOMING|MF_FLAGS_DONTDELETE);
      else
      {
         ibp->writeEntry(MP_FOLDER_TYPE, MF_INBOX|MF_FLAGS_DONTDELETE);
         MDialog_Message(_(
            "Mahogany will not automatically collect mail from your\n"
            "system's default INBOX.\n"
            "You can change this in the INBOX folder's preferences\n"
            "dialog at any time."));

      }
      ibp->writeEntry(MP_FOLDER_COMMENT, _("Default system folder for incoming mail."));
      ibp->DecRef();
   }
#endif

   /*
    * Is the newmail folder properly configured?
    */
   MFolder_obj folderRoot("");
   NewMailFolderTraversal traverse(folderRoot);
   traverse.Traverse(true); // ignore result
   String foldername = traverse.GetNewMailFolder();
   if(foldername.Length() == 0) // shouldn't happen unless run for the first time
      foldername = READ_APPCONFIG(MP_NEWMAIL_FOLDER);
   /* We need to keep the old "New Mail" name for now, to keep it
      backwards compatible. */
   if(foldername.IsEmpty()) // this must not be
      foldername = "New Mail";
   mApplication->GetProfile()->writeEntry(MP_NEWMAIL_FOLDER, foldername);
   strutil_delwhitespace(foldername);

   static const long flagsNewMail = MF_FLAGS_NEWMAILFOLDER |
                                    MF_FLAGS_DONTDELETE |
                                    MF_FLAGS_KEEPOPEN;

   // Do we need to create the NewMailFolder?
   ProfileBase *ibp = ProfileBase::CreateProfile(foldername);
   if (!  parent->HasEntry(foldername) )
   {
      ibp->writeEntry(MP_PROFILE_TYPE, ProfileBase::PT_FolderProfile);
      ibp->writeEntry(MP_FOLDER_TYPE,
                      CombineFolderTypeAndFlags(MF_FILE, flagsNewMail));
      ibp->writeEntry(MP_FOLDER_PATH, strutil_expandfoldername(foldername));
      ibp->writeEntry(MP_FOLDER_COMMENT,
                      _("Folder where Mahogany will collect all new mail."));
      rc = FALSE;
   }

   // Make sure the flags and type are valid
   int typeAndFlags = READ_CONFIG(ibp,MP_FOLDER_TYPE);
   int oldflags = GetFolderFlags(typeAndFlags);
   int flags = oldflags;
   if( flags & MF_FLAGS_INCOMING )
      flags ^= MF_FLAGS_INCOMING;
   flags |= flagsNewMail;
   if( (flags != oldflags) || (GetFolderType(typeAndFlags) != MF_FILE) )
   {
      ibp->writeEntry(MP_FOLDER_TYPE,
                      CombineFolderTypeAndFlags(MF_FILE, flags));
   }
   ibp->DecRef();

   /*
    * Set up the SentMail folder:
    */
   if( READ_APPCONFIG(MP_USEOUTGOINGFOLDER) )
   {
      // this line is for backwards compatibility only
      foldername = READ_APPCONFIG(MP_OUTGOINGFOLDER);
      strutil_delwhitespace(foldername);
      // We have to stick with the following for now, until we add
      // some upgrade functionality which can set this to a different
      // (translated) value.  "SentMail" is the old default name.
      if(foldername.Length() == 0)
         foldername = "SentMail";
      mApplication->GetProfile()->writeEntry(MP_OUTGOINGFOLDER, foldername);
      ProfileBase *ibp = ProfileBase::CreateProfile(foldername);
      if (!  parent->HasEntry(foldername) )
      {
         ibp->writeEntry(MP_PROFILE_TYPE, ProfileBase::PT_FolderProfile);
         ibp->writeEntry(MP_FOLDER_TYPE, MF_FILE|MF_FLAGS_KEEPOPEN);
         ibp->writeEntry(MP_FOLDER_PATH, strutil_expandfoldername(foldername));
         ibp->writeEntry(MP_FOLDER_COMMENT,
                         _("Folder where Mahogany will store copies of outgoing messages."));
         rc = FALSE;
      }
      // Make sure the flags are valid:
      flags = READ_CONFIG(ibp,MP_FOLDER_TYPE);
      oldflags = flags;
      if(flags & MF_FLAGS_INCOMING) flags ^= MF_FLAGS_INCOMING;
      if(flags & MF_FLAGS_DONTDELETE) flags ^= MF_FLAGS_DONTDELETE;
      if(flags != oldflags)
         ibp->writeEntry(MP_FOLDER_TYPE, flags);
      ibp->DecRef();
   }

   /*
    * Set up the Trash folder:
    */
   if( READ_APPCONFIG(MP_USE_TRASH_FOLDER) )
   {
      foldername = READ_APPCONFIG(MP_TRASH_FOLDER);
      strutil_delwhitespace(foldername);
      if(foldername.Length() == 0)
      {
         wxLogError(_("The name for the trash folder was empty, using default '%s'."),
                    MP_TRASH_FOLDER_D);
         foldername = MP_TRASH_FOLDER_D;
      }
      mApplication->GetProfile()->writeEntry(MP_TRASH_FOLDER, foldername);
      ProfileBase *p = ProfileBase::CreateProfile(foldername);
      // Don't overwrite settings if entry already exists:
      if (!  parent->HasEntry(foldername) )
      {
         p->writeEntry(MP_PROFILE_TYPE, ProfileBase::PT_FolderProfile);
         p->writeEntry(MP_FOLDER_TYPE, MF_FILE|MF_FLAGS_KEEPOPEN);
         p->writeEntry(MP_FOLDER_PATH, strutil_expandfoldername(foldername));
         p->writeEntry(MP_FOLDER_COMMENT,
                         _("Folder where Mahogany will store deleted messages."));
         rc = FALSE;
      }
      // Make sure the flags are valid:
      flags = READ_CONFIG(p,MP_FOLDER_TYPE);
      oldflags = flags;
      if(flags & MF_FLAGS_INCOMING) flags ^= MF_FLAGS_INCOMING;
      if(flags & MF_FLAGS_DONTDELETE) flags ^= MF_FLAGS_DONTDELETE;
      if(flags != oldflags)
         p->writeEntry(MP_FOLDER_TYPE, flags);
      // 14=Trash icon from wxFolderTree.cpp:
      p->writeEntry("Icon", 14l);
      p->DecRef();
   }
   return rc;
}

/** Make sure we have all "vital" things set up. */
extern bool
SetupInitialConfig(void)
{
   String host = READ_APPCONFIG(MP_HOSTNAME);
   if(host.Length() == 0)
      mApplication->GetProfile()->writeEntry(MP_HOSTNAME,wxGetFullHostName());

   mApplication->GetProfile()->writeEntry(MP_OUTGOINGFOLDER, _("Sent Mail"));
   mApplication->GetProfile()->writeEntry(MP_NEWMAIL_FOLDER, _("New Mail"));

   (void)VerifyInbox();

#if 0
#if defined ( USE_PYTHON )
// run a python script to set up things
   PyH_RunMScript(MSCRIPT_USER_SETUP);
#else
#   pragma warning "Missing functionality without Python!"
#endif
#endif

   return true;
}

extern bool
VerifyMailConfig(void)
{
// we send a mail to ourself

   String me;
   me = READ_APPCONFIG(MP_RETURN_ADDRESS);
   if(me.Length() == 0)
      me << READ_APPCONFIG(MP_USERNAME) << '@' << READ_APPCONFIG(MP_HOSTNAME);

// FIXME we should make sure that hostname can be resolved - otherwise this
// message will stay in sendmail queue _forever_ (at least it does happen
// here with sendmail 8.8.8 running under Solaris 2.6)
   String nil;
   SendMessageCC  sm(mApplication->GetProfile());
   sm.SetSubject(_("Mahogany Test Message"));
   sm.SetAddresses(me);
   String msg =
      _("If you have received this mail, your Mahogany configuration works.\n"
        "You should also try to reply to this mail and check that your reply arrives.");
   sm.AddPart(Message::MSG_TYPETEXT, msg.c_str(), msg.length());
   sm.SendOrQueue();
   mApplication->SendOutbox();
   msg.Empty();
   msg << _("Sent email message to:\n")
       << me
       << _("\n\nPlease check whether it arrives.");
   MDialog_Message(msg, NULL, _("Testing your configuration"), "TestMailSent");

   return true; // till we know something better
}
