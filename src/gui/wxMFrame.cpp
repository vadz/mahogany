/*-*- c++ -*-********************************************************
 * wxMFrame: a basic window class                                   *
 *                                                                  *
 * (C) 1997, 1998 by Karsten Ballüder (Ballueder@usa.net)           *
 *                                                                  *
 * $Id$
 *******************************************************************/

#ifdef __GNUG__
#   pragma implementation "wxMFrame.h"
#endif

#include "Mpch.h"
#include "Mcommon.h"

#ifndef  USE_PCH
#   include "guidef.h"
#   include "strutil.h"
#   include "MFrame.h"
#   include "kbList.h"
#   include "PathFinder.h"
#   include "MimeList.h"
#   include "MimeTypes.h"
#   include "Profile.h"
#   include "Mdefaults.h"
#   include "MApplication.h"
#   include "gui/wxMApp.h"
#   include "MailFolder.h"
#   include "Message.h"

#   include <wx/confbase.h>
#endif

#ifdef USE_PYTHON
#  include <Python.h>
#  include "PythonHelp.h"
#endif // Python

#include "MHelp.h"
#include "MFolder.h"

#include "MailCollector.h"

#include "MessageTemplate.h"
#include "TemplateDialog.h"

#include "FolderView.h"
#include "MDialogs.h"

#include "MFolderDialogs.h"
#include "MModule.h"
#include "MImport.h"

#include "gui/wxFiltersDialog.h" // for ConfigureAllFilters()
#include "gui/wxIconManager.h"
#include "gui/wxOptionsDlg.h"
#include "adb/AdbFrame.h"
#include "gui/wxMFrame.h"
#include "gui/wxComposeView.h"
#include "gui/wxFolderView.h"
#include "gui/wxIdentityCombo.h"

#include <wx/fontmap.h>          // for GetEncodingDescription()

IMPLEMENT_DYNAMIC_CLASS(wxMFrame, wxFrame)

BEGIN_EVENT_TABLE(wxMFrame, wxFrame)
   EVT_MENU(-1,    wxMFrame::OnCommandEvent)
   EVT_TOOL(-1,    wxMFrame::OnCommandEvent)
   EVT_CLOSE(wxMFrame::OnCloseWindow)
END_EVENT_TABLE()

#ifdef OS_WIN
    // ok, wxPrintDialog is the right one
#else
    typedef wxGenericPrintDialog wxPrintDialog;
#endif

bool wxMFrame::RestorePosition(const char *name,
                               int *x, int *y, int *w, int *h, bool *i)
{
   wxCHECK( x && y && w && h, FALSE ); // no NULL pointers please

   wxConfigBase *pConf = mApplication->GetProfile()->GetConfig();
   if ( pConf != NULL )
   {
      pConf->SetPath(String(M_FRAMES_CONFIG_SECTION)+name);
      *x = pConf->Read(MP_XPOS, MP_XPOS_D);
      *y = pConf->Read(MP_YPOS,MP_YPOS_D);
      *w = pConf->Read(MP_WIDTH, MP_WIDTH_D);
      *h = pConf->Read(MP_HEIGHT, MP_HEIGHT_D);
      if(i)
          *i = pConf->Read(MP_ICONISED, MP_ICONISED_D) != 0;

      // assume that if one entry existed, then the other existed too
      return pConf->HasEntry(MP_XPOS);
   }
   else
   {
      wxLogDebug("Can't restore position/size of window '%s'.", name);
      *x = MP_XPOS_D;
      *y = MP_YPOS_D;
      *w = MP_WIDTH_D;
      *h = MP_HEIGHT_D;
      if(i) *i = MP_ICONISED_D;
      return FALSE;
   }
}

wxMFrame::wxMFrame(const String &iname, wxWindow *parent)
        : MFrameBase(iname)
{
   initialised = false;
   Create(iname,parent);
}

void
wxMFrame::Create(const String &iname, wxWindow *parent)
{
   wxCHECK_RET( !initialised, "wxMFrame created twice" );

   SetName(Str(iname));

   const char *name = MFrameBase::GetName();
   int xpos, ypos, width, height;
   bool iconised;
   RestorePosition(name, &xpos, &ypos, &width, &height, &iconised);

   // use name as default title
   wxFrame::Create(parent, -1, name, wxPoint(xpos, ypos), wxSize(width,height));
   //Show(true);

   Iconize(iconised);

   SetIcon(ICON("MFrame"));

   initialised = true;
   SetMenuBar(new wxMenuBar(wxMB_DOCKABLE));
}

void
wxMFrame::AddFileMenu(void)
{
#ifndef wxMENU_TEAROFF
   // FIXME WXWIN-COMPATIBILITY
   wxMenu *fileMenu = new wxMenu();
#else
   wxMenu *fileMenu = new wxMenu("", wxMENU_TEAROFF);
#endif

   AppendToMenu(fileMenu, WXMENU_FILE_BEGIN + 1, WXMENU_FILE_CLOSE - 1);

#ifdef USE_WXWINDOWS2
   wxWindow *parent = GetParent();
#endif

   int n = WXMENU_FILE_CLOSE;
   if ( parent != NULL )
   {
      AppendToMenu(fileMenu, n);
   }

   // +2 because WXMENU_FILE_CLOSE has a separator with it
   n += 2;
   AppendToMenu(fileMenu, n, WXMENU_FILE_END);

   GetMenuBar()->Append(fileMenu, _("&Mail"));
}

void
wxMFrame::AddEditMenu(void)
{
   WXADD_MENU(GetMenuBar(), EDIT, _("&Edit"));
}

void
wxMFrame::AddHelpMenu(void)
{
   WXADD_MENU(GetMenuBar(), HELP, _("&Help"));
}

void
wxMFrame::AddMessageMenu(void)
{
   WXADD_MENU(GetMenuBar(), MSG, _("Me&ssage"));
}

void
wxMFrame::AddLanguageMenu(void)
{
   WXADD_MENU(GetMenuBar(), LANG, _("&Language"));

   // initially use the default charset
   CheckLanguageInMenu(this, wxFONTENCODING_DEFAULT);
}

wxMFrame::~wxMFrame()
{
   SavePosition(MFrameBase::GetName(), this);
}

void
wxMFrame::SetTitle(String const &title)
{
   wxString t = _("Mahogany : ") + title;

   wxFrame::SetTitle(t.c_str());
}

void
wxMFrame::SavePosition(const char *name, wxWindow *frame)
{
	SavePositionInternal(name, frame, FALSE);
}

void
wxMFrame::SavePosition(const char *name, wxFrame *frame)
{
	SavePositionInternal(name, frame, TRUE);
}
void
wxMFrame::SavePositionInternal(const char *name, wxWindow *frame, bool isFrame)
{
   int x,y;

   wxConfigBase *pConf = mApplication->GetProfile()->GetConfig();
   pConf->SetPath(String(M_FRAMES_CONFIG_SECTION)+name);
   if ( pConf != NULL )
   {
      frame->GetPosition(&x,&y);
      pConf->Write(MP_XPOS, (long)x);
      pConf->Write(MP_YPOS, (long)y);

      frame->GetSize(&x,&y);
      pConf->Write(MP_WIDTH, (long)x);
      pConf->Write(MP_HEIGHT, (long)y);

      if(isFrame)
        pConf->Write(MP_ICONISED, ((wxFrame *)frame)->IsIconized());
   }
}

void
wxMFrame::OnMenuCommand(int id)
{
   // is it a module generated entry?
   if(id >= WXMENU_MODULES_BEGIN && id < WXMENU_MODULES_END)
   {
      ProcessModulesMenu(id);
      return;
   }

   switch(id)
   {
      case WXMENU_FILE_CLOSE:
         Close();
         break;

      case WXMENU_FILE_COMPOSE_WITH_TEMPLATE:
      case WXMENU_FILE_COMPOSE:
         {
            wxString templ;
            if ( id == WXMENU_FILE_COMPOSE_WITH_TEMPLATE )
            {
               templ = ChooseTemplateFor(MessageTemplate_NewMessage, this);
            }

            wxComposeView *composeView = wxComposeView::CreateNewMessage
                                         (
                                          templ,
                                          GetFolderProfile()
                                         );
            composeView->InitText();
            composeView->Show();
         }
         break;

      case WXMENU_FILE_SEND_OUTBOX:
         mApplication->SendOutbox();
         break;

      case WXMENU_FILE_POST:
         {
            wxComposeView *composeView = wxComposeView::CreateNewArticle
                                         (
                                          GetFolderProfile()
                                         );
            composeView->InitText();
            composeView->Show();
         }

      case WXMENU_FILE_COLLECT:
         //MEventManager::Send( new MEventPingData );
         mApplication->GetMailCollector()->Collect();
         break;

#ifdef USE_PYTHON
      case WXMENU_FILE_SCRIPT:
      {
         String
            path,
            filename;

         path << mApplication->GetGlobalDir() << DIR_SEPARATOR << "scripts";
         filename = MDialog_FileRequester(_("Please select a Python script to run."),
                                          this,
                                          path, filename,
                                          "py", "*.py",
                                          false,
                                          NULL /* profile */);
         if(! strutil_isempty(filename))
         {
            FILE *file = fopen(filename,"rb");
            if(file)
            {
               PyH_RunScript(file,filename);
               fclose(file);
            }
         }
         //else: cancelled by user
      }
      break;
#endif   // USE_PYTHON

      case WXMENU_FILE_EXIT:
         wxYield(); // just to flush MEvent queues for safety
         if ( CanClose() )
         {
            // this frame has been already asked whether it wants to exit, so
            // don't ask it again
            mApplication->AddToFramesOkToClose(this);

            // exit the application if other frames don't object
            mApplication->Exit();
         }
         break;

      case WXMENU_FILE_IMPORT:
         ShowImportDialog(this);
         break;

      case WXMENU_EDIT_ADB:
         ShowAdbFrame(this);
         break;
      case WXMENU_EDIT_PREF:
         ShowOptionsDialog(this);
         break;
      case WXMENU_EDIT_FILTERS:
      {
         (void) ConfigureAllFilters(this);
         break;
      }
      case WXMENU_EDIT_MODULES:
         ShowModulesDialog(this);
         break;
      case WXMENU_EDIT_TEMPLATES:
         EditTemplates(this);
         break;

      case WXMENU_EDIT_RESTORE_PREF:
         (void)ShowRestoreDefaultsDialog(mApplication->GetProfile(), this);
         break;
      case WXMENU_EDIT_SAVE_PREF:
         {
            // FIXME any proper way to flush all profiles at once?
            wxConfigBase *config = mApplication->GetProfile()->GetConfig();
            bool ok = config != NULL;
            if ( ok )
               ok = config->Flush();

            if ( ok )
               wxLogStatus(this, _("Program preferences successfully saved."));
            else
               ERRORMESSAGE((_("Couldn't save preferences.")));
         }
         break;

      case WXMENU_HELP_ABOUT:
         MDialog_AboutDialog(this, false /* don't timeout */);
         break;

      case WXMENU_HELP_TIP:
         MDialog_ShowTip(this);
         break;

      case WXMENU_HELP_CONTEXT:
         MDialog_Message(_("Help not implemented for current context, yet."),this,_("Sorry"));
         break;

      case WXMENU_HELP_CONTENTS:
         mApplication->Help(MH_CONTENTS,this);
         break;

      case WXMENU_HELP_RELEASE_NOTES:
         mApplication->Help(MH_RELEASE_NOTES,this);
         break;

      case WXMENU_HELP_FAQ:
         mApplication->Help(MH_FAQ,this);
         break;

      case WXMENU_HELP_SEARCH:
         mApplication->Help(MH_SEARCH,this);
         break;

      case WXMENU_HELP_COPYRIGHT:
         mApplication->Help(MH_COPYRIGHT,this);
         break;

#ifdef DEBUG
      case WXMENU_HELP_WIZARD:
         {
            extern bool RunInstallWizard();

            wxLogStatus("Running installation wizard...");

            wxLogMessage("Wizard returned %s",
                          RunInstallWizard() ? "true" : "false");
         }
         break;
#endif // DEBUG

         // printing:
      case WXMENU_FILE_PRINT_SETUP:
         OnPrintSetup();
         break;
   //   case WXMENU_FILE_PAGE_SETUP:
   //      OnPageSetup();
   //      break;
#ifdef USE_PS_PRINTING
      case WXMENU_FILE_PRINT_SETUP_PS:
         OnPrintSetup();
         break;
#endif
   //   case WXMENU_FILE_PAGE_SETUP_PS:
   //      OnPageSetup();
   //      break;
      case WXMENU_FILE_NET_ON:
         mApplication->GoOnline();
         break;
      case WXMENU_FILE_NET_OFF:
         if(mApplication->CheckOutbox())
         {
            if ( MDialog_YesNoDialog(
            _("You have outgoing messages queued.\n"
              "Do you want to send them before going offline?"),
            this,
            MDIALOG_YESNOTITLE,
            TRUE /* yes default */, "GoOfflineSendFirst") )
               mApplication->SendOutbox();
         }
         mApplication->GoOffline();
         break;

         // create a new identity and edit it
      case WXMENU_FILE_IDENT_ADD:
         {
            wxString ident;
            if ( MInputBox(&ident,
                           _("Mahogany: Create new identity"),
                           _("Enter the identity name:"),
                           this,
                           "NewIdentity") )
            {
               ShowIdentityDialog(ident, this);

               // update the identity combo in the toolbar if any
               wxWindow *win = GetToolBar()->FindWindow(IDC_IDENT_COMBO);
               if ( win )
               {
                  wxChoice *combo = wxDynamicCast(win, wxChoice);
                  combo->Append(ident);
               }
            }
         }
         break;

         // change the current identity
      case WXMENU_FILE_IDENT_CHANGE:
         {
            wxArrayString identities = Profile::GetAllIdentities();
            if ( identities.IsEmpty() )
            {
               wxLogError(_("There are no existing identities to choose from.\n"
                            "Please create an identity first."));
            }
            else
            {
               identities.Insert(_("Default"), 0);
               int rc = MDialog_GetSelection
                        (
                         _("Select the new identity"),
                         MDIALOG_YESNOTITLE,
                         identities,
                         this
                        );

               if ( rc != -1 )
               {
                  Profile *profile = mApplication->GetProfile();
                  if ( rc == 0 )
                  {
                     // restore the default identity
                     profile->DeleteEntry(MP_CURRENT_IDENTITY);
                  }
                  else
                  {
                     wxString ident = identities[(size_t)rc];
                     profile->writeEntry(MP_CURRENT_IDENTITY, ident);
                  }

                  // update the identity combo in the toolbar if any
                  wxWindow *win = GetToolBar()->FindWindow(IDC_IDENT_COMBO);
                  if ( win )
                  {
                     wxChoice *combo = wxDynamicCast(win, wxChoice);
                     combo->SetSelection(rc);
                  }

                  // TODO: should update everything (all options might have
                  //       changed)
               }
               //else: dialog cancelled, nothing to do
            }
         }
         break;

         // edit the current identity parameters
      case WXMENU_FILE_IDENT_EDIT:
         {
            wxString ident = READ_APPCONFIG(MP_CURRENT_IDENTITY);
            if ( !ident )
            {
               // if there is no current identity, choose any among the
               // existing ones
               wxArrayString identities = Profile::GetAllIdentities();
               if ( identities.IsEmpty() )
               {
                  wxLogError(_("There are no existing identities to edit.\n"
                               "Please create an identity first."));
               }
               else
               {
                  int rc = MDialog_GetSelection
                           (
                            _("Which identity would you like to edit?"),
                            MDIALOG_YESNOTITLE,
                            identities,
                            this
                           );
                  if ( rc != -1 )
                     ident = identities[(size_t)rc];
                  //else: cancelled
               }
            }

            if ( !!ident )
               ShowIdentityDialog(ident, this);
         }
         break;

      case WXMENU_FILE_IDENT_DELETE:
         {
            String ident;
            wxArrayString identities = Profile::GetAllIdentities();
            if ( identities.IsEmpty() )
            {
               wxLogError(_("There are no existing identities to delete."));
            }
            else
            {
               int rc = MDialog_GetSelection
                        (
                         _("Which identity would you like to delete?"),
                         MDIALOG_YESNOTITLE,
                         identities,
                         this
                        );
               if ( rc != -1 )
               {
                  ident = identities[(size_t)rc];
               }
               //else: cancelled
            }

            if ( !!ident )
            {
               if ( ident == READ_APPCONFIG(MP_CURRENT_IDENTITY) )
               {
                  // can't keep this one
               }

               // FIXME: will this really work? if there are objects which
               //        use this identity the section will be recreated...
               String identSection;
               identSection << M_IDENTITY_CONFIG_SECTION << '/' << ident;
               mApplication->GetProfile()->DeleteGroup(identSection);

               // update the identity combo in the toolbar if any
               wxWindow *win = GetToolBar()->FindWindow(IDC_IDENT_COMBO);
               if ( win )
               {
                  wxChoice *combo = wxDynamicCast(win, wxChoice);
                  combo->Delete(combo->FindString(ident));
               }

               wxLogStatus(_("Identity '%s' deleted."), ident.c_str());
            }
         }
         break;

      case WXMENU_LANG_SET_DEFAULT:
         {
            static const wxFontEncoding encodingsSupported[] =
            {
               wxFONTENCODING_ISO8859_1,       // West European (Latin1)
               wxFONTENCODING_ISO8859_2,       // Central and East European (Latin2)
               wxFONTENCODING_ISO8859_3,       // Esperanto (Latin3)
               wxFONTENCODING_ISO8859_4,       // Baltic (old) (Latin4)
               wxFONTENCODING_ISO8859_5,       // Cyrillic
               wxFONTENCODING_ISO8859_6,       // Arabic
               wxFONTENCODING_ISO8859_7,       // Greek
               wxFONTENCODING_ISO8859_8,       // Hebrew
               wxFONTENCODING_ISO8859_9,       // Turkish (Latin5)
               wxFONTENCODING_ISO8859_10,      // Variation of Latin4 (Latin6)
               wxFONTENCODING_ISO8859_11,      // Thai
               wxFONTENCODING_ISO8859_12,      // doesn't exist currently, but put it
                                               // here anyhow to make all ISO8859
                                               // consecutive numbers
               wxFONTENCODING_ISO8859_13,      // Baltic (Latin7)
               wxFONTENCODING_ISO8859_14,      // Latin8
               wxFONTENCODING_ISO8859_15,      // Latin9 (a.k.a. Latin0, includes euro)
               wxFONTENCODING_ISO8859_MAX,

               wxFONTENCODING_CP1250,          // WinLatin2
               wxFONTENCODING_CP1251,          // WinCyrillic
               wxFONTENCODING_CP1252,          // WinLatin1
               wxFONTENCODING_CP1253,          // WinGreek (8859-7)
               wxFONTENCODING_CP1254,          // WinTurkish
               wxFONTENCODING_CP1255,          // WinHebrew
               wxFONTENCODING_CP1256,          // WinArabic
               wxFONTENCODING_CP1257,          // WinBaltic (same as Latin 7)

               wxFONTENCODING_KOI8,            // == KOI8-R
            };

            wxArrayString encDescs;
            encDescs.Add(_("Default 7 bit (US ASCII)"));
            for ( size_t n = 0; n < WXSIZEOF(encodingsSupported); n++ )
            {
               encDescs.Add(
                     wxFontMapper::GetEncodingDescription(
                        encodingsSupported[n]
                     )
               );
            }

            int choice = MDialog_GetSelection
                         (
                           _("Please choose the default encoding:\n"
                             "it will be used by default in both\n"
                             "message viewer and composer."),
                           _("Choose default encoding"),
                           encDescs,
                           this
                         );

            wxFontEncoding enc;
            if ( choice == -1 )
            {
               // cancelled, do nothing
               break;
            }
            else if ( choice == 0 )
            {
               enc = wxFONTENCODING_DEFAULT;
            }
            else
            {
               enc = encodingsSupported[choice - 1];
            }

            // remember the encoding as default
            mApplication->GetProfile()->writeEntry(MP_MSGVIEW_DEFAULT_ENCODING,
                                                   enc);
         }
         break;
   }
}

wxToolBar *
wxMFrame::CreateToolBar(void)
{
   int style = wxTB_HORIZONTAL | wxNO_BORDER;
   if(READ_APPCONFIG(MP_DOCKABLE_TOOLBARS) != 0)
      style |= wxTB_DOCKABLE;
   if(READ_APPCONFIG(MP_FLAT_TOOLBARS) != 0)
      style |= wxTB_FLAT;

   wxToolBar *tb = wxFrame::CreateToolBar(style, -1, _("Mahogany Toolbar"));
   tb->SetMargins(4, 4);

   return tb;
}

void
wxMFrame::OnCloseWindow(wxCloseEvent& event)
{
   if ( event.CanVeto() && !mApplication->IsOkToClose(this) && !CanClose() )
   {
      event.Veto();
   }
   else
   {
      Destroy();
   }
}

void
wxMFrame::OnCommandEvent(wxCommandEvent &event)
{
   OnMenuCommand(event.GetId());
}


void
wxMFrame::OnPrintSetup()
{
#ifdef OS_WIN
   wxGetApp().SetPrintMode(wxPRINT_WINDOWS);
#endif
   wxPrintDialogData printDialogData(* ((wxMApp *)mApplication)->GetPrintData());
   wxPrintDialog printerDialog(this, & printDialogData);

   printerDialog.GetPrintDialogData().SetSetupDialog(TRUE);
   printerDialog.ShowModal();

   (*((wxMApp *)mApplication)->GetPrintData())
      = printerDialog.GetPrintDialogData().GetPrintData();
}

void wxMFrame::OnPrintSetupPS()
{
   wxGetApp().SetPrintMode(wxPRINT_POSTSCRIPT);

   wxPrintDialogData printDialogData(* ((wxMApp *)mApplication)->GetPrintData());
   wxPrintDialog printerDialog(this, & printDialogData);

   printerDialog.GetPrintDialogData().SetSetupDialog(TRUE);
   printerDialog.ShowModal();

   (*((wxMApp *)mApplication)->GetPrintData())
      = printerDialog.GetPrintDialogData().GetPrintData();
}


void wxMFrame::OnPageSetup()
{
   *((wxMApp *)mApplication)->GetPageSetupData()
      = *((wxMApp *)mApplication)->GetPrintData();

      wxPageSetupDialog pageSetupDialog(this,
                                        ((wxMApp *)mApplication)->GetPageSetupData()
                                        );
    pageSetupDialog.ShowModal();

    *((wxMApp *)mApplication)->GetPrintData()
       = pageSetupDialog.GetPageSetupData().GetPrintData();
    *((wxMApp *)mApplication)->GetPageSetupData()
       = pageSetupDialog.GetPageSetupData();
}

/// Passes a menu id to modules for reacting to it.
bool
wxMFrame::ProcessModulesMenu(int id)
{
#ifndef USE_MODULES
   return FALSE;
#else
   if(id < WXMENU_MODULES_BEGIN || id > WXMENU_MODULES_END)
      return FALSE;

   MModuleListing *listing = MModule::ListLoadedModules();
   MModule *mptr = NULL;
   for(size_t i = 0; i < listing->Count(); i++)
   {
      mptr = (*listing)[i].GetModule();
      if(mptr->Entry(MMOD_FUNC_MENUEVENT, id) )
      {
         listing->DecRef();
         mptr->DecRef();
         return TRUE;
      }
      mptr->DecRef();
   }
   listing->DecRef();
   return FALSE;
#endif
}


#if 0
// unused so far:


void wxMFrame::OnPageSetup()
{
#ifdef OS_WIN
      wxGetApp().SetPrintMode(wxPRINT_WINDOWS);
#else
      wxGetApp().SetPrintMode(wxPRINT_POSTSCRIPT);
#endif
      wxPrintData &data = ((wxMApp *)mApplication)->GetPrintData();
      //FIXME data.SetOrientation(orientation);

#ifdef OS_WIN
      wxPageSetupDialog pageSetupDialog(this, & data);
#else
      wxGenericPageSetupDialog pageSetupDialog(this, & data);
#endif
      pageSetupDialog.ShowModal();

      data = pageSetupDialog.GetPageSetupData();
      //FIXME orientation = data.GetOrientation();
}

void wxMFrame::OnPageSetupPS()
{
      wxGetApp().SetPrintMode(wxPRINT_POSTSCRIPT);

      wxPrintData &data = ((wxMApp *)mApplication)->GetPrintData();
      //data.SetOrientation(orientation);

      wxGenericPageSetupDialog pageSetupDialog(this, & data);
      pageSetupDialog.ShowModal();

      //FIXME orientation = pageSetupDialog.GetPageSetupData().GetOrientation();
}
#endif

