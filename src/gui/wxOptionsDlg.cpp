///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   gui/wxOptionsDlg.cpp - M options dialog
// Purpose:     allows to easily change from one dialog all program options
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

#ifndef USE_PCH
#  include "Mcommon.h"
#  include "MApplication.h"
#  include "Profile.h"
#  include "guidef.h"
#  include "MHelp.h"

#  include <wx/dynarray.h>
#  include <wx/checkbox.h>
#  include <wx/listbox.h>
#  include <wx/radiobox.h>
#  include <wx/statbox.h>
#  include <wx/stattext.h>
#  include <wx/textctrl.h>

#  include <wx/utils.h>    // for wxStripMenuCodes
#endif

#include <wx/log.h>
#include <wx/imaglist.h>
#include <wx/notebook.h>
#include <wx/resource.h>
#include <wx/confbase.h>

#include <wx/menuitem.h>
#include <wx/checklst.h>

#include <wx/layout.h>

#include <wx/persctrl.h>

#include "MDialogs.h"
#include "Mdefaults.h"
#include "Mupgrade.h"
#include "Mcallbacks.h"

#include "gui/wxIconManager.h"
#include "gui/wxDialogLayout.h"
#include "gui/wxOptionsDlg.h"
#include "gui/wxOptionsPage.h"

#include "HeadersDialogs.h"

// first and last are shifted by -1, i.e. the range of fields for the page Foo
// is from ConfigField_FooFirst + 1 to ConfigField_FooLast inclusive.
//
// only the wxOptionsPage ctor knows about it, so if this is (for some reason)
// changed, it would be the only place to change.
//
// if you modify this enum, you must modify the data below too (search for
// DONT_FORGET_TO_MODIFY to find it)
enum ConfigFields
{
   // network & identity
   ConfigField_IdentFirst = -1,
   ConfigField_UsernameHelp,
   ConfigField_Username,
   ConfigField_HostnameHelp,
   ConfigField_Hostname,
   ConfigField_AddDefaultHostname,
   ConfigField_ReturnAddress,
   ConfigField_SetReplyFromTo,
   ConfigField_MailServer,
   ConfigField_NewsServer,
   ConfigField_PersonalName,
   ConfigField_UserLevel,
   ConfigField_TimeoutInfo,
   ConfigField_OpenTimeout,
   ConfigField_ReadTimeout,
   ConfigField_WriteTimeout,
   ConfigField_CloseTimeout,
   ConfigField_RshTimeout,
   ConfigField_IdentLast = ConfigField_RshTimeout,

   // compose
   ConfigField_ComposeFirst = ConfigField_IdentLast,
   ConfigField_UseOutgoingFolder,
   ConfigField_OutgoingFolder,
   ConfigField_WrapMargin,
   ConfigField_ReplyString,
   ConfigField_ReplyCollapse,
   ConfigField_ReplyCharacter,

   ConfigField_Signature,
   ConfigField_SignatureFile,
   ConfigField_SignatureSeparator,
   ConfigField_XFace,
   ConfigField_XFaceFile,
   ConfigField_AdbSubstring,
   ConfigField_ComposeViewFontFamily,
   ConfigField_ComposeViewFontSize,
   ConfigField_CopmposViewFGColour,
   ConfigField_ComposeViewBGColour,

   ConfigField_ComposeHeaders,

   ConfigField_ComposeLast = ConfigField_ComposeHeaders,

   // folders
   ConfigField_FoldersFirst = ConfigField_ComposeLast,
   ConfigField_OpenFolders,
   ConfigField_MainFolder,
//   ConfigField_NewMailFolder,
   ConfigField_PollIncomingDelay,
   ConfigField_UpdateInterval,
   ConfigField_FolderProgressThreshold,
   ConfigField_AutoShowFirstMessage,
   ConfigField_FoldersLast = ConfigField_AutoShowFirstMessage,

#ifdef USE_PYTHON
   // python
   ConfigField_PythonFirst = ConfigField_FoldersLast,
   ConfigField_Python_HelpText,
   ConfigField_EnablePython,
   ConfigField_PythonPath,
   ConfigField_StartupScript,
   ConfigField_CallbackFolderOpen,
   ConfigField_CallbackFolderUpdate,
   ConfigField_CallbackFolderExpunge,
   ConfigField_CallbackSetFlag,
   ConfigField_CallbackClearFlag,
   ConfigField_PythonLast = ConfigField_CallbackClearFlag,
#else  // !USE_PYTHON
   ConfigField_PythonLast = ConfigField_FoldersLast,
#endif // USE_PYTHON

   // message view
   ConfigField_MessageViewFirst = ConfigField_PythonLast,
   ConfigField_MessageViewPreviewOnSelect,
   ConfigField_MessageViewFontFamily,
   ConfigField_MessageViewFontSize,
   ConfigField_MessageViewFGColour,
   ConfigField_MessageViewBGColour,
   ConfigField_MessageViewUrlColour,
   ConfigField_MessageViewHeaderNamesColour,
   ConfigField_MessageViewHeaderValuesColour,
   ConfigField_MessageViewInlineGraphics,
#ifdef OS_UNIX
   ConfigField_MessageViewConvertGraphicsFormat,
   ConfigField_MessageViewFaxSupport,
   ConfigField_MessageViewFaxDomains,
#endif // Unix
   ConfigField_MessageViewMaxHelpText,
   ConfigField_MessageViewMaxMsgSize,
   ConfigField_MessageViewMaxHeadersNum,
   ConfigField_MessageViewHeaders,
   ConfigField_MessageViewLast = ConfigField_MessageViewHeaders,

   // autocollecting addresses options
   ConfigField_AdbFirst = ConfigField_MessageViewLast,
   ConfigField_AutoCollect_HelpText,
   ConfigField_AutoCollect,
   ConfigField_AutoCollectAdb,
   ConfigField_AutoCollectNameless,
   ConfigField_Bbdb_HelpText,
   ConfigField_Bbdb_IgnoreAnonymous,
   ConfigField_Bbdb_GenerateUnique,
   ConfigField_Bbdb_AnonymousName,
   ConfigField_Bbdb_SaveOnExit,
   ConfigField_AdbLast = ConfigField_Bbdb_SaveOnExit,

   // helper programs
   ConfigField_HelpersFirst = ConfigField_AdbLast,
   ConfigField_HelpersHelp1,
   ConfigField_Browser,
   ConfigField_BrowserIsNetscape,
   ConfigField_HelpersHelp2,
   ConfigField_HelpBrowser,
   ConfigField_HelpBrowserIsNetscape,
   ConfigField_HelpersHelp3,
   ConfigField_ExternalEditor,
   ConfigField_ImageConverter,
   ConfigField_NewMailCommand,
   ConfigField_HelpersLast = ConfigField_NewMailCommand,

   // other options
   ConfigField_OthersFirst = ConfigField_HelpersLast,
   ConfigField_ShowLog,
   ConfigField_Splash,
   ConfigField_SplashDelay,
   ConfigField_AutosaveDelay,
   ConfigField_ConfirmExit,
   ConfigField_OpenOnClick,
   ConfigField_DateFormat,
   ConfigField_ShowNewMail,
#ifdef OS_UNIX
   ConfigField_AFMPath,
   ConfigField_OthersLast = ConfigField_AFMPath,
#else // !Unix
   ConfigField_OthersLast = ConfigField_ShowNewMail,
#endif // Unix/!Unix
   // the end
   ConfigField_Max
};

// a structure giving the name of config key and it's default value which may
// be either numeric or a string (no special type for boolean values right
// now)
struct ConfigValueDefault
{
   ConfigValueDefault(const char *name_, long value)
      { bNumeric = TRUE; name = name_; lValue = value; }

   ConfigValueDefault(const char *name_, const char *value)
      { bNumeric = FALSE; name = name_; szValue = value; }

   long GetLong() const { wxASSERT( bNumeric ); return lValue; }
   const char *GetString() const { wxASSERT( !bNumeric ); return szValue; }

   bool IsNumeric() const { return bNumeric; }

   const char *name;
   union
   {
      long        lValue;
      const char *szValue;
   };
   bool bNumeric;
};

struct ConfigValueNone : public ConfigValueDefault
{
   ConfigValueNone() : ConfigValueDefault("none",0L) { }
};

// -----------------------------------------------------------------------------
// our notebook class
// -----------------------------------------------------------------------------

// notebook for the options
class wxOptionsNotebook : public wxNotebookWithImages
{
public:
   // icon names
   static const char *s_aszImages[];

   wxOptionsNotebook(wxWindow *parent);

   // the profile we use - just the global one here
   ProfileBase *GetProfile() const { return ProfileBase::CreateProfile(""); }
};

// -----------------------------------------------------------------------------
// dialog classes
// -----------------------------------------------------------------------------

class wxOptionsDialog : public wxNotebookDialog
{
public:
   wxOptionsDialog(wxFrame *parent);

   ~wxOptionsDialog();

   // notifications from the notebook pages
      // something important change
   void SetDoTest() { SetDirty(); m_bTest = TRUE; }
      // some setting changed, but won't take effect until restart
   void SetGiveRestartWarning() { m_bRestartWarning = TRUE; }

   // override base class functions
   virtual void CreateNotebook(wxPanel *panel);
   virtual bool TransferDataToWindow();

   virtual bool OnSettingsChange();

   // unimplemented default ctor for DECLARE_DYNAMIC_CLASS
   wxOptionsDialog() { wxFAIL_MSG("should be never used"); }

protected:
   // unset the dirty flag
   virtual void ResetDirty();

   // implement base class pure virtual
   virtual ProfileBase *GetProfile() const
   {
      return ((wxOptionsNotebook *)m_notebook)->GetProfile();
   }

private:
   bool  m_bTest,            // test new settings?
         m_bRestartWarning;  // changes will take effect after restart

   DECLARE_DYNAMIC_CLASS(wxOptionsDialog)
};

class wxRestoreDefaultsDialog : public wxProfileSettingsEditDialog
{
public:
   wxRestoreDefaultsDialog(ProfileBase *profile, wxFrame *parent);

   // reset the selected options to their default values
   virtual bool TransferDataFromWindow();

private:
   wxCheckListBox *m_checklistBox;
};

// ----------------------------------------------------------------------------
// event tables and such
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxOptionsDialog, wxNotebookDialog)

BEGIN_EVENT_TABLE(wxOptionsPage, wxNotebookPageBase)
   // any change should make us dirty
   EVT_CHECKBOX(-1, wxOptionsPage::OnControlChange)
   EVT_RADIOBOX(-1, wxOptionsPage::OnControlChange)
   EVT_COMBOBOX(-1, wxOptionsPage::OnControlChange)
   EVT_TEXT(-1, wxOptionsPage::OnChange)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxOptionsPageCompose, wxOptionsPage)
   // buttons invoke subdialogs
   EVT_BUTTON(-1, wxOptionsPageCompose::OnButton)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxOptionsPageMessageView, wxOptionsPage)
   // buttons invoke subdialogs
   EVT_BUTTON(-1, wxOptionsPageMessageView::OnButton)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxOptionsPageFolders, wxOptionsPage)
   EVT_BUTTON(wxOptionsPage_BtnNew,    wxOptionsPageFolders::OnNewFolder)
   EVT_BUTTON(wxOptionsPage_BtnModify, wxOptionsPageFolders::OnModifyFolder)
   EVT_BUTTON(wxOptionsPage_BtnDelete, wxOptionsPageFolders::OnDeleteFolder)

   EVT_IDLE(wxOptionsPageFolders::OnIdle)
END_EVENT_TABLE()

// ============================================================================
// data: both of these arrays *must* be in sync with ConfigFields enum!
// ============================================================================

// the labels of all fields, their types and also the field they "depend on"
// (being dependent on another field only means that if that field is disabled
//  or unchecked, we're disabled too)
//
// if you modify this array, search for DONT_FORGET_TO_MODIFY and modify data
// there too
wxOptionsPage::FieldInfo wxOptionsPage::ms_aFields[] =
{
   // network config and identity
   { gettext_noop("The user and host names are used to compose "
                  "the return address, unless\n"
                  "a different return address is explicitly specified."),
                                                   Field_Message, -1 },
   { gettext_noop("&Username"),                    Field_Text,    -1 },
   { gettext_noop("The host name is also used as a default host "
                  "name for local mail addresses."),
                                                   Field_Message, -1 },
   { gettext_noop("&Hostname"),                    Field_Text | Field_Vital,   -1, },
   { gettext_noop("&Add this hostname if none specified"), Field_Bool, -1 },
   { gettext_noop("&Return/Reply address"),        Field_Text | Field_Vital,   -1, },
   { gettext_noop("Reply return address from &To: field"), Field_Bool, -1, },
   { gettext_noop("SMTP (&mail) server"),          Field_Text | Field_Vital,   -1, },
   { gettext_noop("NNTP (&news) server"),          Field_Text,    -1,                        },
   { gettext_noop("&Personal name"),               Field_Text,    -1,                        },
   { gettext_noop("User &level:novice:advanced"),  Field_Combo,   -1,                        },
   { gettext_noop("The following timeout values are used for TCP connections to\n"
                  "remote mail or news servers. Their scope is global, but they\n"
                  "will get set from the folder that has been opened last.\n")
                  "All values are in seconds.", Field_Message, -1 },
   { gettext_noop("&Open timeout"),                Field_Number,    -1,                        },
   { gettext_noop("&Read timeout"),                Field_Number,    -1,                        },
   { gettext_noop("&Write timeout"),               Field_Number,    -1,                        },
   { gettext_noop("&rsh timeout"),                 Field_Number,    -1,                        },
   { gettext_noop("&Close timeout"),               Field_Number,    -1,                        },

   // compose
   { gettext_noop("Sa&ve sent messages"),          Field_Bool,    -1,                        },
   { gettext_noop("&Folder file for sent messages"),
                                                   Field_File,    ConfigField_UseOutgoingFolder },
   { gettext_noop("&Wrap margin"),                 Field_Number,  -1,                        },
   { gettext_noop("&Reply string in subject"),     Field_Text,    -1,                        },
   { gettext_noop("Co&llapse reply markers"
                  ":no:collapse:collapse & count"),            Field_Combo,   -1,                        },
   { gettext_noop("Quote &character"),             Field_Text,    -1,                        },

   { gettext_noop("&Use signature"),               Field_Bool,    -1,                        },
   { gettext_noop("&Signature file"),              Field_File,    ConfigField_Signature      },
   { gettext_noop("Use signature se&parator"),     Field_Bool,    ConfigField_Signature      },
   { gettext_noop("Us&e XFace"),                   Field_Bool,    -1,                        },
   { gettext_noop("&XFace file"),                  Field_File,    ConfigField_XFace          },
   { gettext_noop("Mail alias substring ex&pansion"),
                                                   Field_Bool,    -1,                        },
   { gettext_noop("Font famil&y"
                  ":default:decorative:roman:script:swiss:modern:teletype"),
                                                   Field_Combo,   -1},
   { gettext_noop("Font si&ze"),                   Field_Number,  -1},
   { gettext_noop("Foreground c&olour"),           Field_Color,   -1},
   { gettext_noop("Back&ground colour"),           Field_Color,   -1},

   { gettext_noop("Configure &headers..."),        Field_SubDlg,  -1},

   // folders
   { gettext_noop("Folders to open on &startup"),  Field_List |
                                                   Field_Restart, -1,           },
   { gettext_noop("Folder opened in &main frame"), Field_Text,    -1,                        },
//   { gettext_noop("Folder where to collect &new mail"), Field_Text, -1},
   { gettext_noop("Poll for &new mail interval in seconds"), Field_Number, -1},
   { gettext_noop("&Ping/check folder interval in seconds"), Field_Number, -1},
   { gettext_noop("&Automatically select first message in viewer"), Field_Bool, -1},
   { gettext_noop("&Threshold for displaying progress dialog"), Field_Number, -1},


#ifdef USE_PYTHON
   // python
   { gettext_noop("Python is the built-in scripting language which can be\n")
     gettext_noop("used to extend Mahogany's functionality. It is not essential\n")
     gettext_noop("for the program's normal operation."), Field_Message, -1},
   { gettext_noop("&Enable Python"),               Field_Bool,    -1,                        },
   { gettext_noop("Python &Path"),                 Field_Text,    ConfigField_EnablePython   },
   { gettext_noop("&Startup script"),              Field_File,    ConfigField_EnablePython   },
   { gettext_noop("&Folder open callback"),        Field_Text,    ConfigField_EnablePython   },
   { gettext_noop("Folder &update callback"),      Field_Text,    ConfigField_EnablePython   },
   { gettext_noop("Folder e&xpunge callback"),     Field_Text,    ConfigField_EnablePython   },
   { gettext_noop("Flag &set callback"),           Field_Text,    ConfigField_EnablePython   },
   { gettext_noop("Flag &clear callback"),         Field_Text,    ConfigField_EnablePython   },
#endif // USE_PYTHON

   // message views:
   { gettext_noop("Preview message when &selected"), Field_Bool,    -1                     },
   { gettext_noop("&Font family"
                  ":default:decorative:roman:script:swiss:modern:teletype"),
                                                   Field_Combo,   -1 },
   { gettext_noop("Font si&ze"),                   Field_Number,  -1 },
   { gettext_noop("Foreground c&olour"),           Field_Color,   -1 },
   { gettext_noop("Back&ground colour"),           Field_Color,   -1 },
   { gettext_noop("Colour for &URLs"),             Field_Color,   -1 },
   { gettext_noop("Colour for header &names"),     Field_Color,   -1 },
   { gettext_noop("Colour for header &values"),    Field_Color,   -1 },
   { gettext_noop("&Inline graphics"),             Field_Bool,    -1 },
#ifdef OS_UNIX
   { gettext_noop("Conversion &graphics format"
                  ":XPM:PNG:BMP:JPG"),             Field_Combo,   ConfigField_MessageViewInlineGraphics },
   { gettext_noop("Support special &fax mailers"), Field_Bool,    -1 },
   { gettext_noop("&Domains sending faxes"),       Field_Text,    ConfigField_MessageViewFaxSupport},
#endif // unix
   { gettext_noop("The following settings allow to limit the amount of data\n"
                  "retrieved from remote server: if the message size or\n"
                  "number is greater than the value specified here, you\n"
                  "will be asked for confirmation before transfering data."),
                                                   Field_Message,  -1 },
   { gettext_noop("Maximum size of &message (in Kb)"),
                                                   Field_Number,   -1 },
   { gettext_noop("Maximum &number of messages"),  Field_Number,   -1 },
   { gettext_noop("Configure &headers to show..."),Field_SubDlg,   -1 },

   // adb: autocollect and bbdb options
   { gettext_noop("Mahogany may automatically remember all e-mail addresses in the messages you\n"
                  "receive in a special addresss book. This is called 'address\n"
                  "autocollection' and may be turned on or off from this page."),
                                                   Field_Message, -1                     },
   { gettext_noop("&Autocollect addresses"),       Field_Action,  -1,                    },
   { gettext_noop("Address &book to use"),         Field_Text, ConfigField_AutoCollect   },
   { gettext_noop("Ignore addresses without &names"), Field_Bool, ConfigField_AutoCollect},
   { gettext_noop("The following settings configure the support of the Big Brother\n"
                  "addressbook (BBDB) format. This is supported only for compatibility\n"
                  "with other software (emacs). The normal addressbook is unaffected by\n"
                  "these settings."), Field_Message, -1},
   { gettext_noop("&Ignore entries without names"), Field_Bool, -1 },
   { gettext_noop("&Generate unique aliases"),      Field_Bool, -1 },
   { gettext_noop("&Name for nameless entries"),    Field_Text, ConfigField_Bbdb_GenerateUnique },
   { gettext_noop("&Save on exit"),                 Field_Action, -1 },

   // helper programs
   { gettext_noop("The following program will be used to open URLs embedded in messages:"),       Field_Message, -1                      },
   { gettext_noop("Open &URLs with"),             Field_File,    -1                      },
   { gettext_noop("URL &browser is Netscape"),    Field_Bool,    -1                      },
   { gettext_noop("The following program will be used to view the online help system:"),     Field_Message, -1                      },
   { gettext_noop("&Help viewer"),                Field_File,    -1                      },
   { gettext_noop("Help &viewer is Netscape"),    Field_Bool,    -1                      },
   { gettext_noop("&External editor"),            Field_File,    -1                      },
   { gettext_noop("&Image format converter"),     Field_File,    -1                      },
   { gettext_noop("The following line will be executed each time new mail is received:"),       Field_Message, -1                      },
   { gettext_noop("&New Mail Command"),           Field_File,    -1                      },

   // other options
   { gettext_noop("Show &log window"),             Field_Bool,    -1,                    },
   { gettext_noop("&Splash screen at startup"),    Field_Bool | Field_Restart, -1,                    },
   { gettext_noop("Splash screen &delay"),         Field_Number,  ConfigField_Splash     },
   { gettext_noop("&Autosave delay"),              Field_Number | Field_Restart, -1                     },
   { gettext_noop("Confirm &exit"),                Field_Bool | Field_Restart, -1                     },
   { gettext_noop("Open folder on single &click"), Field_Bool,    -1                     },
   { gettext_noop("&Format for the date"),         Field_Text,    -1                     },
   { gettext_noop("Show new mail &notifications"), Field_Bool,    -1                     },
#ifdef OS_UNIX
   { gettext_noop("&Path where to find AFM files"), Field_Text,    -1                     },
#endif
};

// FIXME ugly, ugly, ugly... config settings should be living in an array from
//       the beginning which would avoid us all these contorsions
#define CONFIG_ENTRY(name)  ConfigValueDefault(name, name##_D)
// even worse: dummy entries for message fields
#define CONFIG_NONE()  ConfigValueNone()

// if you modify this array, search for DONT_FORGET_TO_MODIFY and modify data
// there too
static const ConfigValueDefault gs_aConfigDefaults[] =
{
   // network and identity
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_USERNAME),
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_HOSTNAME),
   CONFIG_ENTRY(MP_ADD_DEFAULT_HOSTNAME),
   CONFIG_ENTRY(MP_RETURN_ADDRESS),
   CONFIG_ENTRY(MP_SET_REPLY_FROM_TO),
   CONFIG_ENTRY(MP_SMTPHOST),
   CONFIG_ENTRY(MP_NNTPHOST),
   CONFIG_ENTRY(MP_PERSONALNAME),
   CONFIG_ENTRY(MP_USERLEVEL),
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_TCP_OPENTIMEOUT),
   CONFIG_ENTRY(MP_TCP_READTIMEOUT),
   CONFIG_ENTRY(MP_TCP_WRITETIMEOUT),
   CONFIG_ENTRY(MP_TCP_RSHTIMEOUT),
   CONFIG_ENTRY(MP_TCP_CLOSETIMEOUT),

   // compose
   CONFIG_ENTRY(MP_USEOUTGOINGFOLDER),
   CONFIG_ENTRY(MP_OUTGOINGFOLDER),
   CONFIG_ENTRY(MP_COMPOSE_WRAPMARGIN),
   CONFIG_ENTRY(MP_REPLY_PREFIX),
   CONFIG_ENTRY(MP_REPLY_COLLAPSE_PREFIX),
   CONFIG_ENTRY(MP_REPLY_MSGPREFIX),

   CONFIG_ENTRY(MP_COMPOSE_USE_SIGNATURE),
   CONFIG_ENTRY(MP_COMPOSE_SIGNATURE),
   CONFIG_ENTRY(MP_COMPOSE_USE_SIGNATURE_SEPARATOR),
   CONFIG_ENTRY(MP_COMPOSE_USE_XFACE),
   CONFIG_ENTRY(MP_COMPOSE_XFACE_FILE),
   CONFIG_ENTRY(MP_ADB_SUBSTRINGEXPANSION),
   CONFIG_ENTRY(MP_CVIEW_FONT),
   CONFIG_ENTRY(MP_CVIEW_FONT_SIZE),
   CONFIG_ENTRY(MP_CVIEW_FGCOLOUR),
   CONFIG_ENTRY(MP_CVIEW_BGCOLOUR),
   CONFIG_NONE(),

   // folders
   CONFIG_ENTRY(MP_OPENFOLDERS),
   CONFIG_ENTRY(MP_MAINFOLDER),
//   CONFIG_ENTRY(MP_NEWMAIL_FOLDER),
   CONFIG_ENTRY(MP_POLLINCOMINGDELAY),
   CONFIG_ENTRY(MP_UPDATEINTERVAL),
   CONFIG_ENTRY(MP_AUTOSHOW_FIRSTMESSAGE),
   CONFIG_ENTRY(MP_FOLDERPROGRESS_THRESHOLD),
   // python
#ifdef USE_PYTHON
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_USEPYTHON),
   CONFIG_ENTRY(MP_PYTHONPATH),
   CONFIG_ENTRY(MP_STARTUPSCRIPT),
   CONFIG_ENTRY(MCB_FOLDEROPEN),
   CONFIG_ENTRY(MCB_FOLDERUPDATE),
   CONFIG_ENTRY(MCB_FOLDEREXPUNGE),
   CONFIG_ENTRY(MCB_FOLDERSETMSGFLAG),
   CONFIG_ENTRY(MCB_FOLDERCLEARMSGFLAG),
#endif // USE_PYTHON

   // message views
   CONFIG_ENTRY(MP_PREVIEW_ON_SELECT),
   CONFIG_ENTRY(MP_MVIEW_FONT),
   CONFIG_ENTRY(MP_MVIEW_FONT_SIZE),
   CONFIG_ENTRY(MP_MVIEW_FGCOLOUR),
   CONFIG_ENTRY(MP_MVIEW_BGCOLOUR),
   CONFIG_ENTRY(MP_MVIEW_URLCOLOUR),
   CONFIG_ENTRY(MP_MVIEW_HEADER_NAMES_COLOUR),
   CONFIG_ENTRY(MP_MVIEW_HEADER_VALUES_COLOUR),
   CONFIG_ENTRY(MP_INLINE_GFX),
#ifdef OS_UNIX
   CONFIG_ENTRY(MP_TMPGFXFORMAT),
   CONFIG_ENTRY(MP_INCFAX_SUPPORT),
   CONFIG_ENTRY(MP_INCFAX_DOMAINS),
#endif
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_MAX_MESSAGE_SIZE),
   CONFIG_ENTRY(MP_MAX_HEADERS_NUM),
   CONFIG_ENTRY(MP_MSGVIEW_HEADERS),

   // autocollect
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_AUTOCOLLECT),
   CONFIG_ENTRY(MP_AUTOCOLLECT_ADB),
   CONFIG_ENTRY(MP_AUTOCOLLECT_NAMED),
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_BBDB_IGNOREANONYMOUS),
   CONFIG_ENTRY(MP_BBDB_GENERATEUNIQUENAMES),
   CONFIG_ENTRY(MP_BBDB_ANONYMOUS),
   CONFIG_ENTRY(MP_BBDB_SAVEONEXIT),

   // helper programs
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_BROWSER),
   CONFIG_ENTRY(MP_BROWSER_ISNS),
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_HELPBROWSER),
   CONFIG_ENTRY(MP_HELPBROWSER_ISNS),
   CONFIG_ENTRY(MP_EXTERNALEDITOR),
   CONFIG_ENTRY(MP_CONVERTPROGRAM),
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_NEWMAILCOMMAND),

   // other
   CONFIG_ENTRY(MP_SHOWLOG),
   CONFIG_ENTRY(MP_SHOWSPLASH),
   CONFIG_ENTRY(MP_SPLASHDELAY),
   CONFIG_ENTRY(MP_AUTOSAVEDELAY),
   CONFIG_ENTRY(MP_CONFIRMEXIT),
   CONFIG_ENTRY(MP_OPEN_ON_CLICK),
   CONFIG_ENTRY(MP_DATE_FMT),
   CONFIG_ENTRY(MP_SHOW_NEWMAILMSG),
#ifdef OS_UNIX
   CONFIG_ENTRY(MP_AFMPATH),
#endif
};

#undef CONFIG_ENTRY
#undef CONFIG_NONE

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxOptionsPage
// ----------------------------------------------------------------------------

wxOptionsPage::wxOptionsPage(wxNotebook *notebook,
                             const char *title,
                             ProfileBase *profile,
                             size_t nFirst,
                             size_t nLast,
                             int helpId)
             : wxNotebookPageBase(notebook)
{
   int image = notebook->GetPageCount();

   notebook->AddPage(this, title, FALSE /* don't select */, image);

   m_Profile = profile;
   m_Profile->IncRef();

   m_HelpId = helpId;
   // see enum ConfigFields for "+1"
   m_nFirst = nFirst + 1;
   m_nLast = nLast + 1;

   CreateControls();
}

void wxOptionsPage::CreateControls()
{
   size_t n;

   // first determine the longest label
   wxArrayString aLabels;
   for ( n = m_nFirst; n < m_nLast; n++ ) {
      // do it only for text control labels
      switch ( GetFieldType(n) ) {
      case Field_Number:
      case Field_File:
      case Field_Color:
      case Field_Bool:
         // fall through: for this purpose (finding the longest label)
         // they're the same as text
      case Field_Action:
      case Field_Text:
         break;

      default:
         // don't take into account the other types
         continue;
      }

      aLabels.Add(_(ms_aFields[n].label));
   }

   long widthMax = GetMaxLabelWidth(aLabels, this);

   // now create the controls
   wxControl *last = NULL; // last control created
   for ( n = m_nFirst; n < m_nLast; n++ ) {
      switch ( GetFieldType(n) ) {
      case Field_File:
         last = CreateFileEntry(_(ms_aFields[n].label), widthMax, last);
         break;

      case Field_Color:
         last = CreateColorEntry(_(ms_aFields[n].label), widthMax, last);
         break;

      case Field_Action:
         last = CreateActionChoice(_(ms_aFields[n].label), widthMax, last);
         break;

      case Field_Combo:
         last = CreateComboBox(_(ms_aFields[n].label), widthMax, last);
         break;

      case Field_Number:
         // fall through -- for now they're the same as text
      case Field_Text:
         last = CreateTextWithLabel(_(ms_aFields[n].label), widthMax, last);
         break;

      case Field_List:
         last = CreateListbox(_(ms_aFields[n].label), last);
         break;

      case Field_Bool:
         last = CreateCheckBox(_(ms_aFields[n].label), widthMax, last);
         break;

      case Field_Message:
         last = CreateMessage(_(ms_aFields[n].label), last);
         break;

      case Field_SubDlg:
         last = CreateButton(_(ms_aFields[n].label), last);
         break;

      default:
         wxFAIL_MSG("unknown field type in CreateControls");
      }

      wxCHECK_RET( last, "control creation failed" );

      FieldFlags flags = GetFieldFlags(n);
      if ( flags & Field_Vital )
         m_aVitalControls.Add(last);
      if ( flags & Field_Restart )
         m_aRestartControls.Add(last);

      m_aControls.Add(last);
      m_aDirtyFlags.Add(false);
   }
}

void wxOptionsPage::OnChange(wxEvent& event)
{
   wxOptionsDialog *dialog = GET_PARENT_OF_CLASS(this, wxOptionsDialog);

   UpdateUI();

   wxControl *control = (wxControl *)event.GetEventObject();
   int index = m_aControls.Index(control);

   if ( index != wxNOT_FOUND )
   {
      m_aDirtyFlags[(size_t)index] = true;
   }
   else
   {
      wxFAIL_MSG("unknown control in wxOptionsPage::OnChange");
   }

   if ( !dialog )
   {
      // we don't put an assert here because this does happen when we're a
      // page in the folder properties dialog, but we must have event.Skip()
      // here to allow the base class version mark the dialog as being dirty
      event.Skip();

      return;
   }

   if ( m_aVitalControls.Index(control) != -1 )
      dialog->SetDoTest();
   else
      dialog->SetDirty();

   if ( m_aRestartControls.Index(control) != -1 )
      dialog->SetGiveRestartWarning();
}

void wxOptionsPage::OnControlChange(wxEvent& event)
{
   OnChange(event);

   Refresh();
}

void wxOptionsPage::UpdateUI()
{
   for ( size_t n = m_nFirst; n < m_nLast; n++ ) {
      int nCheckField = ms_aFields[n].enable;
      if ( nCheckField != -1 ) {
         wxASSERT( nCheckField > 0 && nCheckField < ConfigField_Max );

         // avoid signed/unsigned mismatch in expressions
         size_t nCheck = (size_t)nCheckField;
         wxASSERT( GetFieldType(nCheck) == Field_Bool ||
                   GetFieldType(nCheck) == Field_Action );
         wxASSERT( nCheck >= m_nFirst && nCheck < m_nLast );

         // enable only if the checkbox is checked
         bool bEnable = true;
         if ( GetFieldType(nCheck) == Field_Bool )
         {
            wxCheckBox *checkbox = (wxCheckBox *)GetControl(nCheck);
            wxASSERT( checkbox->IsKindOf(CLASSINFO(wxCheckBox)) );

            bEnable = checkbox->GetValue();
         }
         else
         {
            wxRadioBox *radiobox = (wxRadioBox *)GetControl(nCheck);

            if ( radiobox->GetSelection() == 0 ) // FIXME hardcoded!
               bEnable = false;
         }

         wxControl *control = GetControl(n);
         control->Enable(bEnable);

         switch ( GetFieldType(n) ) {
            // for file entries, also disable the browse button
            case Field_File:
            case Field_Color:
               wxASSERT( control->IsKindOf(CLASSINFO(wxTextCtrl)) );

               EnableTextWithButton((wxTextCtrl *)control, bEnable);
               break;

            case Field_Text:
               wxASSERT( control->IsKindOf(CLASSINFO(wxTextCtrl)) );

               EnableTextWithLabel((wxTextCtrl *)control, bEnable);
               break;

            case Field_List:
               // also disable the buttons
            {
               long i;
               for ( i = wxOptionsPage_BtnNew; i <= wxOptionsPage_BtnNew; i++ ) {
                  wxWindow *win = FindWindow(i);
                  if ( win ) {
                     win->Enable(bEnable);
                  }
                  else {
                     wxFAIL_MSG("can't find listbox buttons by id");
                  }
               }
            }
            break;

            default:
               ;
         }
      }
      // this field is always enabled
   }
}

// read the data from config
bool wxOptionsPage::TransferDataToWindow()
{
   // disable environment variable expansion here because we want the user to
   // edit the real value stored in the config
   ProfileEnvVarSave suspend(m_Profile, false);

   // check that we didn't forget to update one of the arrays...
   wxASSERT( WXSIZEOF(gs_aConfigDefaults) == ConfigField_Max );
   wxASSERT( WXSIZEOF(wxOptionsPage::ms_aFields) == ConfigField_Max );

   String strValue;
   long lValue = 0;
   for ( size_t n = m_nFirst; n < m_nLast; n++ )
   {
      if ( gs_aConfigDefaults[n].IsNumeric() )
      {
         lValue = m_Profile->readEntry(gs_aConfigDefaults[n].name,
                                       (int)gs_aConfigDefaults[n].lValue);
         strValue.Printf("%ld", lValue);
      }
      else {
         // it's a string
         strValue = m_Profile->readEntry(gs_aConfigDefaults[n].name,
                                         gs_aConfigDefaults[n].szValue);
      }

      wxControl *control = GetControl(n);
      switch ( GetFieldType(n) ) {
         case Field_Text:
         case Field_File:
         case Field_Color:
         case Field_Number:
            if ( GetFieldType(n) == Field_Number ) {
               wxASSERT( gs_aConfigDefaults[n].IsNumeric() );

               strValue.Printf("%ld", lValue);
            }
            else {
               wxASSERT( !gs_aConfigDefaults[n].IsNumeric() );
            }
            wxASSERT( control->IsKindOf(CLASSINFO(wxTextCtrl)) );

            ((wxTextCtrl *)control)->SetValue(strValue);
            break;

         case Field_Bool:
            wxASSERT( gs_aConfigDefaults[n].IsNumeric() );
            wxASSERT( control->IsKindOf(CLASSINFO(wxCheckBox)) );
            ((wxCheckBox *)control)->SetValue(lValue != 0);
            break;

         case Field_Action:
            wxASSERT( control->IsKindOf(CLASSINFO(wxRadioBox)) );
            ((wxRadioBox *)control)->SetSelection(lValue);
            break;

         case Field_Combo:
            wxASSERT( control->IsKindOf(CLASSINFO(wxComboBox)) );
            ((wxComboBox *)control)->SetSelection(lValue);
            break;

         case Field_List:
            wxASSERT( !gs_aConfigDefaults[n].IsNumeric() );
            wxASSERT( control->IsKindOf(CLASSINFO(wxListBox)) );

            // split it (FIXME what if it contains ';'?)
            {
               String str;
               for ( size_t m = 0; m < strValue.Len(); m++ ) {
                  if ( strValue[m] == ';' ) {
                     if ( !str.IsEmpty() ) {
                        ((wxListBox *)control)->Append(str);
                        str.Empty();
                     }
                     //else: nothing to do, two ';' one after another
                  }
                  else {
                     str << strValue[m];
                  }
               }

               if ( !str.IsEmpty() ) {
                  ((wxListBox *)control)->Append(str);
               }
            }
            break;

         case Field_Message:
         case Field_SubDlg:      // these settings will be read later
            break;

         default:
            wxFAIL_MSG("unexpected field type");
      }

      // the dirty flag was set from the OnChange() callback, reset it!
      ClearDirty(n);
   }

   return TRUE;
}

// write the data to config
bool wxOptionsPage::TransferDataFromWindow()
{
   String strValue;
   long lValue = 0;
   for ( size_t n = m_nFirst; n < m_nLast; n++ )
   {
      // only write the controls which were really changed
      if ( !IsDirty(n) )
         continue;

      wxControl *control = GetControl(n);
      switch ( GetFieldType(n) )
      {
         case Field_Text:
         case Field_File:
         case Field_Color:
         case Field_Number:
            wxASSERT( control->IsKindOf(CLASSINFO(wxTextCtrl)) );

            strValue = ((wxTextCtrl *)control)->GetValue();

            if ( GetFieldType(n) == Field_Number ) {
               wxASSERT( gs_aConfigDefaults[n].IsNumeric() );

               lValue = atol(strValue);
            }
            else {
               wxASSERT( !gs_aConfigDefaults[n].IsNumeric() );
            }
            break;

         case Field_Bool:
            wxASSERT( gs_aConfigDefaults[n].IsNumeric() );
            wxASSERT( control->IsKindOf(CLASSINFO(wxCheckBox)) );

            lValue = ((wxCheckBox *)control)->GetValue();
            break;

         case Field_Action:
            wxASSERT( gs_aConfigDefaults[n].IsNumeric() );
            wxASSERT( control->IsKindOf(CLASSINFO(wxRadioBox)) );

            lValue = ((wxRadioBox *)control)->GetSelection();
            break;

         case Field_Combo:
            wxASSERT( gs_aConfigDefaults[n].IsNumeric() );
            wxASSERT( control->IsKindOf(CLASSINFO(wxComboBox)) );

            lValue = ((wxComboBox *)control)->GetSelection();
            break;

         case Field_List:
            wxASSERT( !gs_aConfigDefaults[n].IsNumeric() );
            wxASSERT( control->IsKindOf(CLASSINFO(wxListBox)) );

            // join it (FIXME what if it contains ';'?)
            {
               wxListBox *listbox = (wxListBox *)control;
               for ( size_t m = 0; m < (size_t)listbox->Number(); m++ ) {
                  if ( !strValue.IsEmpty() ) {
                     strValue << ';';
                  }

                  strValue << listbox->GetString(m);
               }
            }
            break;

         case Field_Message:
         case Field_SubDlg:      // already done
            break;

         default:
            wxFAIL_MSG("unexpected field type");
      }

      if ( gs_aConfigDefaults[n].IsNumeric() )
      {
         m_Profile->writeEntry(gs_aConfigDefaults[n].name, (int)lValue);
      }
      else
      {
         // it's a string
         m_Profile->writeEntry(gs_aConfigDefaults[n].name, strValue);
      }
   }

   // TODO life is easy if we don't check for errors...
   return TRUE;
}

// ----------------------------------------------------------------------------
// wxOptionsPageCompose
// ----------------------------------------------------------------------------

wxOptionsPageCompose::wxOptionsPageCompose(wxNotebook *parent,
                                           ProfileBase *profile)
                    : wxOptionsPage(parent,
                                    _("Compose"),
                                    profile,
                                    ConfigField_ComposeFirst,
                                    ConfigField_ComposeLast,
                                    MH_OPAGE_COMPOSE)
{
}

void wxOptionsPageCompose::OnButton(wxCommandEvent& event)
{
   // FIXME there is only one button for now, but if we had several of them,
   //       how would we know which one was clicked?
   wxASSERT_MSG( event.GetEventObject() ==
                 GetControl(ConfigField_ComposeHeaders), "alien button" );

   // create and show the "outgoing headers" config dialog
   if ( ConfigureComposeHeaders(m_Profile, this) )
   {
      // something changed - make us dirty
      wxNotebookDialog *dialog = GET_PARENT_OF_CLASS(this, wxNotebookDialog);

      wxCHECK_RET( dialog, "options page without a parent dialog?" );

      dialog->SetDirty();
   }
}

// ----------------------------------------------------------------------------
// wxOptionsPageMessageView
// ----------------------------------------------------------------------------

wxOptionsPageMessageView::wxOptionsPageMessageView(wxNotebook *parent,
                                                   ProfileBase *profile)
   : wxOptionsPage(parent,
                   _("Message Viewer"),
                   profile,
                   ConfigField_MessageViewFirst,
                   ConfigField_MessageViewLast,
                   MH_OPAGE_MESSAGEVIEW)
{
}

void wxOptionsPageMessageView::OnButton(wxCommandEvent& event)
{
   // FIXME there is only one button for now, but if we had several of them,
   //       how would we know which one was clicked?
   wxASSERT_MSG( event.GetEventObject() ==
                 GetControl(ConfigField_MessageViewHeaders), "alien button" );

   if ( ConfigureMsgViewHeaders(m_Profile, this) )
   {
      // something changed - make us dirty
      wxNotebookDialog *dialog = GET_PARENT_OF_CLASS(this, wxNotebookDialog);

      wxCHECK_RET( dialog, "options page without a parent dialog?" );

      dialog->SetDirty();
   }
}

// ----------------------------------------------------------------------------
// wxOptionsPageIdent
// ----------------------------------------------------------------------------

wxOptionsPageIdent::wxOptionsPageIdent(wxNotebook *parent,
                                       ProfileBase *profile)
                  : wxOptionsPage(parent,
                                  _("Identity"),
                                  profile,
                                  ConfigField_IdentFirst,
                                  ConfigField_IdentLast,
                                  MH_OPAGE_IDENT)
{
}

// ----------------------------------------------------------------------------
// wxOptionsPagePython
// ----------------------------------------------------------------------------

#ifdef USE_PYTHON

wxOptionsPagePython::wxOptionsPagePython(wxNotebook *parent,
                                         ProfileBase *profile)
                   : wxOptionsPage(parent,
                                   _("Python"),
                                   profile,
                                   ConfigField_PythonFirst,
                                   ConfigField_PythonLast,
                                   MH_OPAGE_PYTHON)
{
}

#endif // USE_PYTHON

// ----------------------------------------------------------------------------
// wxOptionsPageAdb
// ----------------------------------------------------------------------------


wxOptionsPageAdb::wxOptionsPageAdb(wxNotebook *parent,
                                    ProfileBase *profile)
                : wxOptionsPage(parent,
                                _("Adressbook"),
                                profile,
                                ConfigField_AdbFirst,
                                ConfigField_AdbLast,
                                MH_OPAGE_ADB)
{
}


// ----------------------------------------------------------------------------
// wxOptionsPageOthers
// ----------------------------------------------------------------------------

wxOptionsPageOthers::wxOptionsPageOthers(wxNotebook *parent,
                                         ProfileBase *profile)
                   : wxOptionsPage(parent,
                                   _("Miscellaneous"),
                                   profile,
                                   ConfigField_OthersFirst,
                                   ConfigField_OthersLast,
                                   MH_OPAGE_OTHERS)
{
}

bool wxOptionsPageOthers::TransferDataToWindow()
{
   // if the user checked "don't ask me again" checkbox in the message box
   // these setting might be out of date - synchronize

   // TODO this should be table based too probably...
   m_Profile->writeEntry(MP_CONFIRMEXIT, wxPMessageBoxEnabled(MP_CONFIRMEXIT));

   return wxOptionsPage::TransferDataToWindow();
}

bool wxOptionsPageOthers::TransferDataFromWindow()
{
   bool rc = wxOptionsPage::TransferDataFromWindow();
   if ( rc )
   {
      // now if the user checked "confirm exit" checkbox we must reenable
      // the message box by erasing the stored answer to it
      wxPMessageBoxEnable(MP_CONFIRMEXIT,
                          READ_CONFIG(m_Profile, MP_CONFIRMEXIT) != 0);
   }

   return rc;
}

// ----------------------------------------------------------------------------
// wxOptionsPageHelpers
// ----------------------------------------------------------------------------

wxOptionsPageHelpers::wxOptionsPageHelpers(wxNotebook *parent,
                                         ProfileBase *profile)
   : wxOptionsPage(parent,
                   _("Helpers"),
                   profile,
                   ConfigField_HelpersFirst,
                   ConfigField_HelpersLast,
                   MH_OPAGE_HELPERS)
{
}

// ----------------------------------------------------------------------------
// wxOptionsPageFolders
// ----------------------------------------------------------------------------

wxOptionsPageFolders::wxOptionsPageFolders(wxNotebook *parent,
                                           ProfileBase *profile)
   : wxOptionsPage(parent,
                   _("Folders"),
                   profile,
                   ConfigField_FoldersFirst,
                   ConfigField_FoldersLast,
                   MH_OPAGE_FOLDERS)
{
}

bool wxOptionsPageFolders::TransferDataToWindow()
{
   bool bRc = wxOptionsPage::TransferDataToWindow();
   if ( bRc ) {
      // we add the folder opened in the main frame to the list of folders
      // opened on startup if it's not yet among them
      wxListBox *listbox = (wxListBox *)GetControl(ConfigField_OpenFolders);
      wxString strMain = GetControlText(ConfigField_MainFolder);
      int n = listbox->FindString(strMain);
      if ( n == -1 ) {
         listbox->Append(strMain);
      }
   }

   return bRc;
}

bool wxOptionsPageFolders::TransferDataFromWindow()
{
   // undo what we did in TransferDataToWindow: remove the main folder from
   // the list of folders to be opened on startup
   wxListBox *listbox = (wxListBox *)GetControl(ConfigField_OpenFolders);
   wxString strMain = GetControlText(ConfigField_MainFolder);
   int n = listbox->FindString(strMain);
   if ( n != -1 ) {
      listbox->Delete(n);
   }

   return wxOptionsPage::TransferDataFromWindow();
}

void wxOptionsPageFolders::OnNewFolder(wxCommandEvent& event)
{
   wxString str;
   if ( !MInputBox(&str, _("Folders to open on startup"), _("Folder name"),
                   GET_PARENT_OF_CLASS(this, wxDialog), "LastStartupFolder") ) {
      return;
   }

   // check that it's not already there
   wxListBox *listbox = (wxListBox *)GetControl(ConfigField_OpenFolders);
   if ( listbox->FindString(str) != -1 ) {
      wxLogError(_("Folder '%s' is already present in the list, not added."),
                 str.c_str());
   }
   else {
      // ok, do add it
      listbox->Append(str);

      wxOptionsPage::OnChange(event);
   }
}

void wxOptionsPageFolders::OnModifyFolder(wxCommandEvent&)
{
   wxListBox *l = (wxListBox *)GetControl(ConfigField_OpenFolders);
   int nSel = l->GetSelection();

   wxCHECK_RET( nSel != -1, "should be disabled" );

   MDialog_FolderProfile(GET_PARENT_OF_CLASS(this, wxDialog),
                         l->GetString(nSel));
}

void wxOptionsPageFolders::OnDeleteFolder(wxCommandEvent& event)
{
   wxListBox *l = (wxListBox *)GetControl(ConfigField_OpenFolders);
   int nSel = l->GetSelection();

   wxCHECK_RET( nSel != -1, "should be disabled" );

   l->Delete(nSel);
   wxOptionsPage::OnChange(event);
}

void wxOptionsPageFolders::OnIdle(wxIdleEvent&)
{
   bool bEnable = ((wxListBox *)GetControl(ConfigField_OpenFolders))
      ->GetSelection() != -1;

   long id;
   for ( id = wxOptionsPage_BtnModify; id <= wxOptionsPage_BtnDelete; id++ ) {
      wxWindow *win = FindWindow(id);
      if ( win ) {
         win->Enable(bEnable);
      }
      else {
         wxFAIL_MSG("can't find listbox buttons by id");
      }
   }
}

// ----------------------------------------------------------------------------
// wxOptionsDialog
// ----------------------------------------------------------------------------

wxOptionsDialog::wxOptionsDialog(wxFrame *parent)
               : wxNotebookDialog(parent, _("Program options"), "OptionsDlg")
{
}

bool
wxOptionsDialog::TransferDataToWindow()
{
   if ( !wxNotebookDialog::TransferDataToWindow() )
      return FALSE;

   int nPageCount = m_notebook->GetPageCount();
   for ( int nPage = 0; nPage < nPageCount; nPage++ ) {
      ((wxOptionsPage *)m_notebook->GetPage(nPage))->UpdateUI();
   }

   return TRUE;
}

bool
wxOptionsDialog::OnSettingsChange()
{
   if ( m_bTest )
   {
      if ( MDialog_YesNoDialog(_("Some important program settings were changed.\n"
                                 "\nWould you like to test the new setup "
                                 "(recommended)?"),
                               this,
                               _("Test setup?"),
                               true,
                               "OptTestAsk") )
      {
         if ( !VerifyMailConfig() )
         {
            return FALSE;
         }
      }
      else
      {
         // no test was done, assume it's ok...
         m_bTest = FALSE;
      }
   }

   if ( m_bRestartWarning )
   {
      MDialog_Message(_("Some of the changes to the program options will\n"
                        "only take effect when the progam will be run the\n"
                        "next time and not during this session."),
                      this, MDIALOG_MSGTITLE, "WarnRestartOpt");
      m_bRestartWarning = FALSE;
   }

   return TRUE;
}

void wxOptionsDialog::CreateNotebook(wxPanel *panel)
{
   m_notebook = new wxOptionsNotebook(panel);
}

// reset the dirty flag
void wxOptionsDialog::ResetDirty()
{
   wxNotebookDialog::ResetDirty();

   m_bTest =
      m_bRestartWarning = FALSE;
}

wxOptionsDialog::~wxOptionsDialog()
{
   // save settings
   ProfileBase::FlushAll();
}

// ----------------------------------------------------------------------------
// wxOptionsNotebook manages its own image list
// ----------------------------------------------------------------------------

// should be in sync with the enum OptionsPage in wxOptionsDlg.h!
const char *wxOptionsNotebook::s_aszImages[] =
{
   "ident",
   "compose",
   "folders",
#ifdef USE_PYTHON
   "python",
#endif
   "msgview",
   "adrbook",
   "helpers",
   "miscopt",
   NULL
};

// create the control and add pages too
wxOptionsNotebook::wxOptionsNotebook(wxWindow *parent)
                 : wxNotebookWithImages("OptionsNotebook", parent, s_aszImages)
{
   // don't forget to update both the array above and the enum!
   wxASSERT( WXSIZEOF(s_aszImages) == OptionsPage_Max + 1);

   ProfileBase *profile = GetProfile();

   // create and add the pages
   (void)new wxOptionsPageIdent(this, profile);
   (void)new wxOptionsPageCompose(this, profile);
   (void)new wxOptionsPageFolders(this, profile);
#ifdef USE_PYTHON
   (void)new wxOptionsPagePython(this, profile);
#endif
   (void)new wxOptionsPageMessageView(this, profile);
   (void)new wxOptionsPageAdb(this, profile);
   (void)new wxOptionsPageHelpers(this, profile);
   (void)new wxOptionsPageOthers(this, profile);

   profile->DecRef();
}

// ----------------------------------------------------------------------------
// wxRestoreDefaultsDialog implementation
// ----------------------------------------------------------------------------

wxRestoreDefaultsDialog::wxRestoreDefaultsDialog(ProfileBase *profile,
                                                 wxFrame *parent)
                       : wxProfileSettingsEditDialog
                         (
                           profile,
                           "RestoreOptionsDlg",
                           parent,
                           _("Restore default options")
                         )
{
   wxLayoutConstraints *c;

   // create the Ok and Cancel buttons in the bottom right corner
   wxStaticBox *box = CreateStdButtonsAndBox(_("&All settings"));

   // create a short help message above

   wxStaticText *msg = new wxStaticText
                           (
                              this, -1,
                              _("Please check the settings whose values\n"
                                "should be reset to the defaults.")
                           );
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(box, wxTop, 4*LAYOUT_Y_MARGIN);
   c->height.AsIs();
   msg->SetConstraints(c);

   // create the checklistbox in the area which is left
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->top.Below(msg, 2*LAYOUT_Y_MARGIN);
   c->bottom.SameAs(box, wxBottom, 2*LAYOUT_Y_MARGIN);

   m_checklistBox = new wxCheckListBox(this, -1);
   m_checklistBox->SetConstraints(c);

   // add the items to the checklistbox
   for ( size_t n = 0; n < ConfigField_Max; n++ )
   {
      switch ( wxOptionsPage::GetFieldType(n) )
      {
         case wxOptionsPage::Field_Message:
            // this is not a setting, just some help text
            continue;

         case wxOptionsPage::Field_SubDlg:
            // TODO inject in the checklistbox the settings of subdlg
            continue;

         default:
            break;
      }

      m_checklistBox->Append(
            wxStripMenuCodes(_(wxOptionsPage::ms_aFields[n].label)));
   }

   // set the initial and minimal size
   SetDefaultSize(4*wBtn, 10*hBtn);
}

bool wxRestoreDefaultsDialog::TransferDataFromWindow()
{
   // delete the values of all selected settings in this profile - this will
   // effectively restore their default values
   size_t count = (size_t)m_checklistBox->Number();
   for ( size_t n = 0; n < count; n++ )
   {
      if ( m_checklistBox->IsChecked(n) )
      {
         MarkDirty();

         GetProfile()->GetConfig()->DeleteEntry(gs_aConfigDefaults[n].name);
      }
   }

   return TRUE;
}

// ----------------------------------------------------------------------------
// our public interface
// ----------------------------------------------------------------------------

void ShowOptionsDialog(wxFrame *parent, OptionsPage page)
{
   wxOptionsDialog dlg(parent);
   dlg.CreateAllControls();
   dlg.SetNotebookPage(page);
   dlg.Layout();
   (void)dlg.ShowModal();
}

bool ShowRestoreDefaultsDialog(ProfileBase *profile, wxFrame *parent)
{
   wxRestoreDefaultsDialog dlg(profile, parent);
   (void)dlg.ShowModal();

   return dlg.HasChanges();
}
