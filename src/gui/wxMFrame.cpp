///////////////////////////////////////////////////////////////////////////////
// Project:     Mahogany
// File name:   gui/wxMFrame.cpp - base frame class
// Purpose:     GUI functionality common to all Mahogany frames
// Author:      M-Team
// Modified by:
// Created:     1997
// CVS-ID:      $Id$
// Copyright:   (c) 1997-2001 Mahogany Team
// License:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef  USE_PCH
#  include "Mcommon.h"
#  include "MApplication.h"
#  include "MHelp.h"
#  include "gui/wxMFrame.h"
#  include "Mdefaults.h"
#  include "gui/wxIconManager.h"

#  include <wx/menu.h>
#  include <wx/toolbar.h>
#  include <wx/choice.h>
#endif // USE_PCH

#ifdef USE_PYTHON
#  include "MPython.h"
#  include "PythonHelp.h"
#  include <wx/ffile.h>
#endif // Python

#include "ConfigSourceLocal.h"
#include "FolderMonitor.h"

#include "TemplateDialog.h"

#include "Composer.h"
#include "MImport.h"

#include "gui/wxFiltersDialog.h" // for ConfigureAllFilters()
#include "gui/wxOptionsDlg.h"
#include "gui/wxMDialogs.h"
#include "adb/AdbFrame.h"
#include "gui/wxIdentityCombo.h"
#include "gui/wxMenuDefs.h"

#include <wx/fontmap.h>          // for GetEncodingDescription()
#ifdef __WINE__
// it includes wrapwin.h which includes windows.h which defines CreateFile under Windows
#undef CreateFile
#endif // __WINE__
#include <wx/confbase.h>

#include <wx/printdlg.h>

class MPersMsgBox;

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_CURRENT_IDENTITY;
extern const MOption MP_DOCKABLE_TOOLBARS;
extern const MOption MP_FLAT_TOOLBARS;
extern const MOption MP_HEIGHT;
extern const MOption MP_ICONISED;
extern const MOption MP_MAXIMISED;
extern const MOption MP_MSGVIEW_DEFAULT_ENCODING;
extern const MOption MP_SHOW_TOOLBAR;
extern const MOption MP_SHOW_STATUSBAR;
extern const MOption MP_SHOW_FULLSCREEN;
extern const MOption MP_USEPYTHON;
extern const MOption MP_WIDTH;
extern const MOption MP_XPOS;
extern const MOption MP_YPOS;

// ----------------------------------------------------------------------------
// persistent message boxes we use here
// ----------------------------------------------------------------------------

extern const MPersMsgBox *M_MSGBOX_GO_OFFLINE_SEND_FIRST;

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

inline MOptionValue GetOptionValue(wxConfigBase *config, const MOption opt)
{
   MOptionValue value;
   const char *name = GetOptionName(opt);
   if ( IsNumeric(opt) )
      value.Set(config->Read(name, GetNumericDefault(opt)));
   else
      value.Set(config->Read(name, GetStringDefault(opt)));

   return value;
}

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

#ifdef USE_PYTHON

class PythonOptionChangeHandler : public MEventReceiver
{
public:
   PythonOptionChangeHandler(wxMFrame *frame)
   {
      m_frame = frame;
      m_eventCookie = MEventManager::Register(*this, MEventId_OptionsChange);
   }

   bool OnMEvent(MEventData& event)
   {
      if ( event.GetId() == MEventId_OptionsChange )
      {
         if ( ((MEventOptionsChangeData &)event).GetChangeKind() ==
                  MEventOptionsChangeData::Ok )
         {
            m_frame->UpdateRunPyScriptMenu();
         }
      }

      // propagate further
      return TRUE;
   }

   virtual ~PythonOptionChangeHandler()
   {
      if ( m_eventCookie )
         MEventManager::Deregister(m_eventCookie);
   }

private:
   wxMFrame *m_frame;
   void *m_eventCookie;
};

#endif // USE_PYTHON

// ----------------------------------------------------------------------------
// wxWin macros
// ----------------------------------------------------------------------------

IMPLEMENT_ABSTRACT_CLASS(wxMFrame, wxFrame)

BEGIN_EVENT_TABLE(wxMFrame, wxFrame)
   EVT_MENU(-1,    wxMFrame::OnCommandEvent)
   EVT_TOOL(-1,    wxMFrame::OnCommandEvent)
   EVT_CLOSE(wxMFrame::OnCloseWindow)
END_EVENT_TABLE()

// ============================================================================
// wxMFrame implementation
// ============================================================================

// ----------------------------------------------------------------------------
// misc wxMFrame methods
// ----------------------------------------------------------------------------

Profile *wxMFrame::GetFolderProfile(void) const
{
   Profile *profile = mApplication->GetProfile();

   CHECK( profile, NULL, _T("no global profile??") );

   profile->IncRef();

   return profile;
}

// ----------------------------------------------------------------------------
// wxMFrame creation
// ----------------------------------------------------------------------------

wxMFrame::wxMFrame(const String &name, wxWindow *parent)
        : MFrameBase(name)
{
#ifdef USE_PYTHON
   m_pyOptHandler = new PythonOptionChangeHandler(this);
#endif // USE_PYTHON

   m_initialised = false;

   Create(name, parent);
}

wxToolBar *
wxMFrame::CreateToolBar(void)
{
   int style = wxTB_HORIZONTAL | wxNO_BORDER;
   if ( READ_APPCONFIG_BOOL(MP_DOCKABLE_TOOLBARS) )
      style |= wxTB_DOCKABLE;
   if ( READ_APPCONFIG_BOOL(MP_FLAT_TOOLBARS) )
      style |= wxTB_FLAT;

   wxToolBar *tb = wxFrame::CreateToolBar(style, -1, _("Mahogany Toolbar"));
   tb->SetMargins(4, 4);

   return tb;
}

void
wxMFrame::SetTitle(String const &title)
{
   wxString t = _("Mahogany : ") + title;

#ifdef DEBUG
   t += _(" [debug build]");
#endif

   wxFrame::SetTitle(t.c_str());
}

void
wxMFrame::Create(const String &name, wxWindow *parent)
{
   wxCHECK_RET( !m_initialised, _T("wxMFrame created twice") );

   SetName(name);

   int xpos, ypos, width, height;
   bool startIconised, startMaximised;
   RestorePosition(MFrameBase::GetName(), &xpos, &ypos, &width, &height,
                   &startIconised, &startMaximised);

   // use name as default title
   if ( !wxFrame::Create(parent, -1, name,
                         wxPoint(xpos, ypos), wxSize(width,height)) )
   {
      wxFAIL_MSG( _T("Failed to create a frame!") );

      return;
   }

   SetIcon(ICON(_T("MFrame")));

   // no "else": a frame can be maximized and iconized, meaning that it will
   // become maximized when restored
   if ( startMaximised )
      Maximize();
   if ( startIconised )
      Iconize();

   m_initialised = true;
   SetMenuBar(new wxMenuBar(wxMB_DOCKABLE));
}

wxMFrame::~wxMFrame()
{
#ifdef USE_PYTHON
   delete m_pyOptHandler;
#endif // USE_PYTHON

   SaveState(MFrameBase::GetName(), this, Save_Geometry | Save_State | Save_View);
}

// ----------------------------------------------------------------------------
// menu stuff
// ----------------------------------------------------------------------------

void
wxMFrame::AddFileMenu(void)
{
#ifndef wxMENU_TEAROFF
   // FIXME WXWIN-COMPATIBILITY
   wxMenu *fileMenu = new wxMenu();
#else
   wxMenu *fileMenu = new wxMenu(wxEmptyString, wxMENU_TEAROFF);
#endif

   AppendToMenu(fileMenu, WXMENU_FILE_BEGIN + 1, WXMENU_FILE_CLOSE - 1);

   wxWindow *parent = GetParent();

   // skip "Close" menu item for the main frame - it is the same as "Exit" for
   // it
   int n = WXMENU_FILE_CLOSE;
   if ( parent != NULL )
   {
      AppendToMenu(fileMenu, n);
   }

   // +2 because WXMENU_FILE_CLOSE has a separator with it
   n += 2;
   AppendToMenu(fileMenu, n, WXMENU_FILE_END);

   GetMenuBar()->Append(fileMenu, _("&Mail"));

#ifdef USE_PYTHON
   UpdateRunPyScriptMenu();
#endif // USE_PYTHON
}

#ifdef USE_PYTHON

void wxMFrame::UpdateRunPyScriptMenu()
{
   wxMenuBar *mbar = GetMenuBar();
   wxMenuItem *item = mbar->FindItem(WXMENU_FILE_RUN_PYSCRIPT);

   CHECK_RET( item, _T("where is \"Run Python script\" menu item?") );

   extern bool IsPythonInitialized();

   item->Enable(IsPythonInitialized() && READ_APPCONFIG_BOOL(MP_USEPYTHON));
}

#endif // USE_PYTHON

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

// ----------------------------------------------------------------------------
// saving and restoring frame position
// ----------------------------------------------------------------------------

/* static */
wxConfigBase *wxMFrame::GetFrameOptionsConfig(const char *name)
{
   wxConfigBase *pConf = mApplication->GetProfile()->GetConfig();
   if ( pConf != NULL )
   {
      String path;
      path << Profile::GetFramesPath() << '/' << name;
      pConf->SetPath(path);
   }

   return pConf;
}

bool wxMFrame::RestorePosition(const char *name,
                               int *x, int *y, int *w, int *h,
                               bool *i, bool *m)
{
   // only i and m might be NULL
   CHECK( x && y && w && h, FALSE,
          _T("NULL pointer in wxMFrame::RestorePosition") );

   wxConfigBase * const pConf = GetFrameOptionsConfig(name);
   if ( pConf != NULL )
   {
      *x = GetOptionValue(pConf, MP_XPOS);
      *y = GetOptionValue(pConf, MP_YPOS);
      *w = GetOptionValue(pConf, MP_WIDTH);
      *h = GetOptionValue(pConf, MP_HEIGHT);

      if ( i )
          *i = GetOptionValue(pConf, MP_ICONISED).GetBoolValue();
      if ( m )
          *m = GetOptionValue(pConf, MP_MAXIMISED).GetBoolValue();


      // assume that if one entry existed, then the other existed too
      return pConf->HasEntry(GetOptionName(MP_XPOS));
   }
   else
   {
      // it's ok if it's done the first time
      *x = GetNumericDefault(MP_XPOS);
      *y = GetNumericDefault(MP_YPOS);
      *w = GetNumericDefault(MP_WIDTH);
      *h = GetNumericDefault(MP_HEIGHT);
      if ( i )
         *i = GetNumericDefault(MP_ICONISED) != 0;
      if ( m )
         *m = GetNumericDefault(MP_MAXIMISED) != 0;

      return FALSE;
   }
}

void wxMFrame::CreateToolAndStatusBars()
{
   bool tb, sb;

   wxConfigBase * const pConf = GetFrameOptionsConfig();
   if ( pConf != NULL )
   {
      tb = GetOptionValue(pConf, MP_SHOW_TOOLBAR).GetBoolValue();
      sb = GetOptionValue(pConf, MP_SHOW_STATUSBAR).GetBoolValue();
   }
   else
   {
      tb = GetNumericDefault(MP_SHOW_TOOLBAR) != 0;
      sb = GetNumericDefault(MP_SHOW_STATUSBAR) != 0;
   }

   wxMenuBar * const mb = GetMenuBar();
   mb->Check(WXMENU_VIEW_TOOLBAR, tb);
   mb->Check(WXMENU_VIEW_STATUSBAR, sb);

   if ( tb )
      DoCreateToolBar();
   if ( sb )
      DoCreateStatusBar();
}

void wxMFrame::ShowInInitialState()
{
   bool showFullScreen;
   wxConfigBase * const pConf = GetFrameOptionsConfig();
   if ( pConf )
      showFullScreen = GetOptionValue(pConf, MP_SHOW_FULLSCREEN).GetBoolValue();
   else
      showFullScreen = GetNumericDefault(MP_SHOW_FULLSCREEN) != 0;

   Show(true);

   if ( showFullScreen )
      ShowFullScreen(true);
}

void
wxMFrame::SavePosition(const char *name, wxWindow *frame)
{
   SaveState(name, frame, Save_Geometry);
}

void
wxMFrame::SavePosition(const char *name, wxFrame *frame)
{
   SaveState(name, frame, Save_Geometry | Save_State);
}

// helper function which either writes the given boolean value option to the
// config or deletes it from it if the new value is the same as default one
//
// notice that this only works for the frame values as there is no inheritance
// with the frames profiles, otherwise deleting the value wouldn't have been
// enough
static void
UpdateBoolConfigValue(wxConfigBase *pConf,
                      const MOption opt,
                      bool value)
{
   // to compare boolean value with option value we need to cast it to long to
   // avoid ambiguities
   const long lValue = value;

   if ( GetOptionValue(pConf, opt) != lValue )
   {
      // current value must be changed but how?
      if ( lValue == GetNumericDefault(opt) )
      {
         // it's enough to just delete the existing value
         pConf->DeleteEntry(GetOptionName(opt));
      }
      else // we must really write the new value to the config
      {
         pConf->Write(GetOptionName(opt), value);
      }
   }
   //else: we already have the right value in the config, nothing to do

   ASSERT_MSG( GetOptionValue(pConf, opt) == lValue,
               "didn't update a boolean option correctly" );
}

void
wxMFrame::SaveState(const char *name, wxWindow *frame, int flags)
{
   wxConfigBase *pConf = GetFrameOptionsConfig(name);
   if ( !pConf )
      return;

   if ( flags & Save_State )
   {
      wxFrame *fr = wxDynamicCast(frame, wxFrame);
      CHECK_RET( fr, "should have a frame when saving state" );

      if ( flags & Save_View )
      {
         UpdateBoolConfigValue(pConf, MP_SHOW_TOOLBAR,
                               fr->GetToolBar() != NULL);
         UpdateBoolConfigValue(pConf, MP_SHOW_STATUSBAR,
                               fr->GetStatusBar() != NULL);
      }

      UpdateBoolConfigValue(pConf, MP_SHOW_FULLSCREEN, fr->IsFullScreen());

      // IsIconized() is broken in wxGTK, it returns TRUE sometimes when the
      // frame is not at all iconized - no idea why :-(
#ifdef __WXGTK__
      bool isIconized = false;
#else
      bool isIconized = fr->IsIconized();
#endif
      bool isMaximized = fr->IsMaximized();

      UpdateBoolConfigValue(pConf, MP_ICONISED, isIconized);
      UpdateBoolConfigValue(pConf, MP_MAXIMISED, isMaximized);

      if ( isMaximized || isIconized )
      {
         // don't remember the coords in this case: wxWindows returns
         // something weird for them for iconized frames and we don't need
         // them for maximized ones anyhow
         return;
      }
   }

   int x, y;
   frame->GetPosition(&x, &y);
   pConf->Write(GetOptionName(MP_XPOS), (long)x);
   pConf->Write(GetOptionName(MP_YPOS), (long)y);

   frame->GetSize(&x,&y);
   pConf->Write(GetOptionName(MP_WIDTH), (long)x);
   pConf->Write(GetOptionName(MP_HEIGHT), (long)y);
}

// ----------------------------------------------------------------------------
// callbacks
// ----------------------------------------------------------------------------

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
               if ( templ.empty() )
               {
                  // cancelled by user
                  break;
               }
            }

            Profile_obj profile(GetFolderProfile());
            Composer *composeView = Composer::CreateNewMessage(templ, profile);

            composeView->InitText();
         }
         break;

      case WXMENU_FILE_SEND_OUTBOX:
         mApplication->SendOutbox();
         break;

      case WXMENU_FILE_POST:
         {
            Profile_obj profile(GetFolderProfile());
            Composer *composeView = Composer::CreateNewArticle(profile);

            composeView->InitText();
         }

      case WXMENU_FILE_COLLECT:
         {
            FolderMonitor *mailCollector = mApplication->GetFolderMonitor();
            if ( mailCollector )
            {
               // when the user explicitly checks for the new mail, also update
               // the currently opened folder(s) and give the verbose messages
               mailCollector->CheckNewMail(FolderMonitor::Interactive |
                                           FolderMonitor::Opened);
            }
         }
         break;

#ifdef USE_PYTHON
      case WXMENU_FILE_RUN_PYSCRIPT:
         {
            wxString path = mApplication->GetDataDir();
            if ( !path.empty() )
               path += DIR_SEPARATOR;
            path += _T("scripts");

            wxString filename = MDialog_FileRequester
                                (
                                 _("Please select a Python script to run."),
                                 this,
                                 path, "",
                                 "py", "*.py",
                                 false,
                                 NULL /* profile */
                                );
            if ( !filename.empty() )
            {
               PythonRunScript(filename);
            }
            //else: cancelled by user
         }
         break;
#endif   // USE_PYTHON

      case WXMENU_FILE_AWAY_MODE:
         mApplication->SetAwayMode(GetMenuBar()->IsChecked(id));
         break;

      case WXMENU_FILE_EXIT:
         // flush MEvent queues for safety
         MEventManager::DispatchPending();

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
         (void) ConfigureAllFilters(this);
         break;

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
         if ( Profile::FlushAll() )
         {
            wxLogStatus(this, _("Program preferences successfully saved."));
         }
         else
         {
            ERRORMESSAGE((_("Couldn't save preferences.")));
         }
         break;

      case WXMENU_EDIT_CONFIG_SOURCES:
         ShowConfigSourcesDialog(this);
         break;

      case WXMENU_EDIT_EXPORT_PREF:
      case WXMENU_EDIT_IMPORT_PREF:
         {
            const bool doExport = id == WXMENU_EDIT_EXPORT_PREF;

            String path = MDialog_FileRequester
                          (
                              doExport ? _("Choose file to export settings to")
                                       : _("Choose file to import settings from"),
                              this,
                              wxEmptyString, wxEmptyString, wxEmptyString, wxEmptyString,
                              doExport    // true => save, false => load
                          );
            if ( path.empty() )
               break;

            ConfigSource_obj
               configSrc(ConfigSourceLocal::CreateDefault()),
               configDst(ConfigSourceLocal::CreateFile(path));
            if ( !doExport )
            {
               configSrc.Swap(configDst);
            }

            bool ok = ConfigSource::Copy(*configDst, *configSrc);

            if ( doExport )
            {
               if ( ok )
               {
                  wxLogStatus(this,
                              _("Settings successfully exported to file \"%s\""),
                              path.c_str());
               }
               else
               {
                  wxLogError(_("Failed to export settings to the file \"%s\"."),
                             path.c_str());
               }
            }
            else // import
            {
               if ( ok )
               {
                  wxLogStatus(this,
                              _("Settings successfully imported from \"%s\""),
                              path.c_str());
               }
               else
               {
                  wxLogError(_("Failed to import settings from the file \"%s\"."),
                             path.c_str());
               }
            }
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

         // printing:
      case WXMENU_FILE_PRINT_SETUP:
         OnPrintSetup();
         break;
      case WXMENU_FILE_PAGE_SETUP:
         OnPageSetup();
         break;

#ifdef USE_PS_PRINTING
      case WXMENU_FILE_PRINT_SETUP_PS:
         OnPrintSetup();
         break;

      case WXMENU_FILE_PAGE_SETUP_PS:
         OnPageSetup();
         break;
#endif // USE_PS_PRINTING

#ifdef USE_DIALUP
      case WXMENU_FILE_NET_ON:
         mApplication->GoOnline();
         break;

      case WXMENU_FILE_NET_OFF:
         if(mApplication->CheckOutbox())
         {
            if ( MDialog_YesNoDialog
                 (
                  _("You have outgoing messages queued.\n"
                    "Do you want to send them before going offline?"),
                  this,
                  MDIALOG_YESNOTITLE,
                  M_DLG_YES_DEFAULT,
                  M_MSGBOX_GO_OFFLINE_SEND_FIRST
                 ) )
            {
               mApplication->SendOutbox();
            }
         }
         mApplication->GoOffline();
         break;
#endif // USE_DIALUP

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

               // update the identity combo in the toolbar of the main frame if
               // any (note that this will update all the other existing
               // identity combo boxes as they keep themselves in sync
               // internally)
               //
               // TODO: we really should have a virtual wxMFrame::GetIdentCombo
               //       as we might not always create the main frame in the
               //       future but other frames (e.g. composer) may have the
               //       ident combo as well
               wxMFrame *frameTop = mApplication->TopLevelFrame();
               if ( frameTop )
               {
                  wxToolBar *tbar = frameTop->GetToolBar();
                  if ( tbar )
                  {
                     wxWindow *win = tbar->FindWindow(IDC_IDENT_COMBO);
                     if ( win )
                     {
                        wxChoice *combo = wxDynamicCast(win, wxChoice);
                        combo->Append(ident);
                     }
                  }
                  else
                  {
                     FAIL_MSG(_T("where is the main frames toolbar?"));
                  }
               }

               wxLogStatus(this, _("Created new identity '%s'."), ident.c_str());
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
                     profile->DeleteEntry(GetOptionName(MP_CURRENT_IDENTITY));
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

         // edit an identity's parameters
      case WXMENU_FILE_IDENT_EDIT:
         {
            String ident;
            wxArrayString identities = Profile::GetAllIdentities();
            if ( identities.IsEmpty() )
            {
               wxLogError(_("There are no existing identities to edit.\n"
                            "Please create an identity first."));
            }
            else
            {
               if ( identities.GetCount() > 1 )             
               {
                  int rc = MDialog_GetSelection
                           (
                            _("Which identity would you like to edit?"),
                            MDIALOG_YESNOTITLE,
                            identities,
                            this
                           );

                  if ( rc != -1 )
                  {
                     ident = identities[(size_t)rc];
                  }
                  //else: dialog was cancelled
               }
               else // only one identity
               {
                  // use the current one
                  ident = READ_APPCONFIG_TEXT(MP_CURRENT_IDENTITY);
               }
            }

            if ( !ident.empty() )
            {
               ShowIdentityDialog(ident, this);
            }
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

            if ( !ident.empty() )
            {
               Profile *profile = mApplication->GetProfile();

               if ( ident == READ_APPCONFIG(MP_CURRENT_IDENTITY) )
               {
                  // can't keep this one
                  profile->writeEntry(MP_CURRENT_IDENTITY, wxEmptyString);
               }

               // FIXME: will this really work? if there are objects which
               //        use this identity the section will be recreated...
               String identSection;
               identSection << Profile::GetIdentityPath() << '/' << ident;
               profile->DeleteGroup(identSection);

               // update the identity combo in the toolbar if any
               wxWindow *win = GetToolBar()->FindWindow(IDC_IDENT_COMBO);
               if ( win )
               {
                  wxChoice *combo = wxDynamicCast(win, wxChoice);
                  combo->Delete(combo->FindString(ident));
               }

               wxLogStatus(this, _("Identity '%s' deleted."), ident.c_str());
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

               wxFONTENCODING_CP1250,          // WinLatin2
               wxFONTENCODING_CP1251,          // WinCyrillic
               wxFONTENCODING_CP1252,          // WinLatin1
               wxFONTENCODING_CP1253,          // WinGreek (8859-7)
               wxFONTENCODING_CP1254,          // WinTurkish
               wxFONTENCODING_CP1255,          // WinHebrew
               wxFONTENCODING_CP1256,          // WinArabic
               wxFONTENCODING_CP1257,          // WinBaltic (almost the same as Latin 7)

               wxFONTENCODING_KOI8,            // == KOI8-R
               wxFONTENCODING_UTF7,            // == UTF-7
               wxFONTENCODING_UTF8,            // == UTF-8
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

      case WXMENU_VIEW_TOOLBAR:
         if ( GetMenuBar()->IsChecked(id) )
         {
            DoCreateToolBar();
         }
         else // hide the toolbar
         {
            delete GetToolBar();
            SetToolBar(NULL);
         }
         break;

      case WXMENU_VIEW_STATUSBAR:
         if ( GetMenuBar()->IsChecked(id) )
         {
            DoCreateStatusBar();
         }
         else // hide the status bar
         {
            delete GetStatusBar();
            SetStatusBar(NULL);
         }
         break;

      case WXMENU_VIEW_FULLSCREEN:
         ShowFullScreen(GetMenuBar()->IsChecked(id));
         break;
   }
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
   // if there was a menu command, the user must be back so check if we don't
   // have to exit from away mode
   mApplication->UpdateAwayMode();

   OnMenuCommand(event.GetId());
}

// ----------------------------------------------------------------------------
// printing
// ----------------------------------------------------------------------------

void
wxMFrame::OnPrintSetup()
{
#if wxUSE_PRINTING_ARCHITECTURE
   wxPrintDialogData printDialogData(*mApplication->GetPrintData());
   wxPrintDialog printerDialog(this, &printDialogData);

   // FIXME: this doesn't exist any more in wx 2.6, we need another way to do it
   //printerDialog.GetPrintDialogData().SetSetupDialog(TRUE);
   if ( printerDialog.ShowModal() == wxID_OK )
   {
      mApplication->SetPrintData(
            printerDialog.GetPrintDialogData().GetPrintData());
   }
#endif // wxUSE_PRINTING_ARCHITECTURE
}

void wxMFrame::OnPageSetup()
{
#if wxUSE_PRINTING_ARCHITECTURE
   wxPageSetupDialog pageSetupDialog(this, mApplication->GetPageSetupData());
   if ( pageSetupDialog.ShowModal() == wxID_OK )
   {
      wxPageSetupData& pageData = pageSetupDialog.GetPageSetupData();
      mApplication->SetPrintData(pageData.GetPrintData());
      mApplication->SetPageSetupData(pageData);
   }
#endif // wxUSE_PRINTING_ARCHITECTURE
}

#ifdef USE_PS_PRINTING

void wxMFrame::OnPrintSetupPS()
{
#if wxUSE_PRINTING_ARCHITECTURE
   wxGetApp().SetPrintMode(wxPRINT_POSTSCRIPT);

   wxPrintDialogData printDialogData(* ((wxMApp *)mApplication)->GetPrintData());
   wxPrintDialog printerDialog(this, & printDialogData);

   printerDialog.GetPrintDialogData().SetSetupDialog(TRUE);
   if ( printerDialog.ShowModal() == wxID_OK )
   {
      (*((wxMApp *)mApplication)->GetPrintData())
         = printerDialog.GetPrintDialogData().GetPrintData();
   }
#endif // wxUSE_PRINTING_ARCHITECTURE
}

#endif // USE_PS_PRINTING

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

