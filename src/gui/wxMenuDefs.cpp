///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   gui/wxMenuDefs.cpp - definitions of all application menus
// Purpose:     gathering all the menus in one place makes it easier to find
//              and modify them
// Author:      Vadim Zeitlin
// Modified by:
// Created:     06.08.98
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
#  include "guidef.h"
#  include "MApplication.h"
#  include "Mdefaults.h"
#  include "Profile.h"            // FIXME: Why is this header needed?
#  include "gui/wxIconManager.h"

#  include <wx/toolbar.h>
#  include <wx/choice.h>
#  include <wx/menu.h>
#  include <wx/frame.h>                 // for wxFrame
#endif // USE_PCH

#include "gui/wxIdentityCombo.h"        // for CreateIdentCombo

#include "gui/wxMenuDefs.h"

#ifndef wxHAS_RADIO_MENU_ITEMS
   #define wxITEM_NORMAL FALSE
   #define wxITEM_CHECK TRUE
   #define wxITEM_RADIO TRUE
#endif

class MOption;

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_MSGVIEW_DEFAULT_ENCODING;
extern const MOption MP_TBARIMAGES;

// ----------------------------------------------------------------------------
// local data
// ----------------------------------------------------------------------------

// structure describing a menu item
struct MenuItemInfo
{
   int idMenu;
   const wxChar *label;
   const wxChar *helpstring;
#ifdef wxHAS_RADIO_MENU_ITEMS
   wxItemKind kind;
#else
   bool kind;
#endif
};

struct TbarItemInfo
{
   const wxChar *bmp;     // image file or ressource name
   int         id;      // id of the associated command (-1 => separator)
   const wxChar *text;    // text for the buttons if image is not shown
   const wxChar *tooltip; // tooltip text
};

// array of descriptions of all toolbar buttons, should be in sync with the enum
// in wxMenuDefs.h!
static const TbarItemInfo g_aToolBarData[] =
{
   // separator
   { wxEmptyString, -1, wxEmptyString, wxEmptyString },

   // common for all frames
   { _T("tb_close"),        WXMENU_FILE_CLOSE,  gettext_noop("Close"), gettext_noop("Close this window") },
   { _T("tb_adrbook"),      WXMENU_EDIT_ADB,    gettext_noop("Addresses"), gettext_noop("Edit address book") },
   { _T("tb_preferences"),  WXMENU_EDIT_PREF,   gettext_noop("Options"), gettext_noop("Edit preferences")  },

   // main frame
   { _T("tb_check_mail"),    WXMENU_FILE_COLLECT,  gettext_noop("Check mail"), gettext_noop("Check for new mail")    },
   { _T("tb_open"),          WXMENU_FOLDER_OPEN,   gettext_noop("Open"), gettext_noop("Open folder")           },
   { _T("tb_mail_compose"),  WXMENU_FILE_COMPOSE,  gettext_noop("Compose"), gettext_noop("Compose a new message") },
   { _T("tb_help"),          WXMENU_HELP_CONTEXT,  gettext_noop("Help"), gettext_noop("Help")                  },
   { _T("tb_exit"),          WXMENU_FILE_EXIT,     gettext_noop("Exit"), gettext_noop("Exit the program")      },

   // compose frame
   { _T("tb_print"),     WXMENU_COMPOSE_PRINT,     gettext_noop("Print"), gettext_noop("Print")         },
   { _T("tb_new"),       WXMENU_COMPOSE_CLEAR,     gettext_noop("Clear"), gettext_noop("Clear Window")  },
   { _T("tb_attach"),    WXMENU_COMPOSE_INSERTFILE,gettext_noop("Attach"), gettext_noop("Insert File")   },
   { _T("tb_editor"),    WXMENU_COMPOSE_EXTEDIT,   gettext_noop("Editor"), gettext_noop("Call external editor")   },
   { _T("tb_mail_send"), WXMENU_COMPOSE_SEND,      gettext_noop("Send"), gettext_noop("Send Message")  },

   // folder and message view frames
   { _T("tb_next_unread"),   WXMENU_MSG_NEXT_UNREAD,gettext_noop("Next unread"), gettext_noop("Select next unread message") },
   { _T("tb_mail"),          WXMENU_MSG_OPEN,      gettext_noop("Open"), gettext_noop("Open message")      },
   { _T("tb_mail_reply"),    WXMENU_MSG_REPLY,     gettext_noop("Reply"), gettext_noop("Reply to message")  },
   { _T("tb_mail_replyall"), WXMENU_MSG_REPLY_ALL, gettext_noop("Reply to all"), gettext_noop("Reply to all recipients")  },
   { _T("tb_mail_forward"),  WXMENU_MSG_FORWARD,   gettext_noop("Forward"), gettext_noop("Forward message")   },
   { _T("tb_print"),         WXMENU_MSG_PRINT,     gettext_noop("Print"), gettext_noop("Print message")     },
   { _T("tb_trash"),         WXMENU_MSG_DELETE,    gettext_noop("Delete"), gettext_noop("Delete message")    },

   // ADB edit frame
   { _T("tb_new"),      WXMENU_ADBEDIT_NEW,     gettext_noop("New"), gettext_noop("Create new entry")        },
   { _T("tb_delete"),   WXMENU_ADBEDIT_DELETE,  gettext_noop("Delete"), gettext_noop("Delete")                  },
   { _T("tb_undo"),     WXMENU_ADBEDIT_UNDO,    gettext_noop("Undo"), gettext_noop("Undo")                    },
   { _T("tb_lookup"),   WXMENU_ADBFIND_NEXT,    gettext_noop("Find"), gettext_noop("Find next")               },
};

wxCOMPILE_TIME_ASSERT( WXSIZEOF(g_aToolBarData) == WXTBAR_MAX,
                       WrongTbarDataCount);

// arrays containing tbar buttons for each frame (must be -1 terminated!)
// the "Close", "Help" and "Exit" icons are added to all frames (except that
// "Close" is not added to the main frame because there it's the same as "Exit")
//
// TODO: should we also add "Edit adb"/"Preferences" to all frames by default?

static const int g_aMainTbar[] =
{
   WXTBAR_MAIN_COLLECT,
   WXTBAR_SEP, // WXTBAR_MAIN_OPEN, -- this command is almost useless
   WXTBAR_MAIN_COMPOSE,
   WXTBAR_SEP,
   WXTBAR_MSG_NEXT_UNREAD,
   WXTBAR_MSG_OPEN,
   WXTBAR_MSG_REPLY,
   WXTBAR_MSG_REPLYALL,
   WXTBAR_MSG_FORWARD,
   WXTBAR_MSG_PRINT,
   WXTBAR_MSG_DELETE,
   WXTBAR_SEP,
   WXTBAR_ADB,
   -1
};

static const int g_aComposeTbar[] =
{
   WXTBAR_COMPOSE_SEND,
   WXTBAR_SEP,
   WXTBAR_COMPOSE_PRINT,
   WXTBAR_COMPOSE_CLEAR,
   WXTBAR_COMPOSE_INSERT,
   WXTBAR_COMPOSE_EXTEDIT,
   WXTBAR_SEP,
   WXTBAR_ADB,
   -1
};

static const int g_aFolderTbar[] =
{
//   WXTBAR_MAIN_OPEN,
   WXTBAR_MSG_OPEN,
   WXTBAR_SEP,
   WXTBAR_MAIN_COMPOSE,
   WXTBAR_SEP,
   WXTBAR_MSG_NEXT_UNREAD,
   WXTBAR_MSG_REPLY,
   WXTBAR_MSG_FORWARD,
   WXTBAR_MSG_PRINT,
   WXTBAR_MSG_DELETE,
   WXTBAR_SEP,
   WXTBAR_ADB,
   -1
};

static const int g_aMsgTbar[] =
{
//   WXTBAR_MAIN_OPEN,
   WXTBAR_MAIN_COMPOSE,
   WXTBAR_MSG_REPLY,
   WXTBAR_MSG_FORWARD,
   WXTBAR_MSG_PRINT,
   WXTBAR_MSG_DELETE,
   WXTBAR_SEP,
   WXTBAR_ADB,
   -1
};

static const int g_aAdbTbar[] =
{
   WXTBAR_SEP,
   WXTBAR_ADB_NEW,
   WXTBAR_ADB_DELETE,
   WXTBAR_ADB_UNDO,
   WXTBAR_SEP,
   WXTBAR_ADB_FINDNEXT,
   -1
};

// this array stores all toolbar buttons for the given frame (the index in it
// is from the previous enum)
static const int *g_aFrameToolbars[WXFRAME_MAX] =
{
   g_aMainTbar,
   g_aComposeTbar,
   g_aFolderTbar,
   g_aMsgTbar,
   g_aAdbTbar
};


// array of descriptions of all menu items
//
// NB: by convention, if the menu item opens another window (or a dialog), it
//     should end with an ellipsis (`...')
//
// The following accelerators are still unused:
//    with Ctrl:           E     K M      T      
//    with Shift-Ctrl:  B DE  HIJ  M OPQ  T VWXY 
static const MenuItemInfo g_aMenuItems[] =
{
   // ABCDEFGHIJKLMNOPQRSTUVWXYZ (VZ: leave it here, it's a cut-&-paste buffer)

   // file
   // available accels: BFGHJKLOQVWZ
   { WXMENU_FILE_COMPOSE,  gettext_noop("Compose &new message\tCtrl-N"),  gettext_noop("Start a new message")      , wxITEM_NORMAL },
   { WXMENU_FILE_COMPOSE_WITH_TEMPLATE,
                           gettext_noop("Compose with &template...\tShift-Ctrl-N"),  gettext_noop("Compose a new message using after choosing a temple for it")      , wxITEM_NORMAL },
   { WXMENU_FILE_POST,     gettext_noop("Post news &article"),   gettext_noop("Write a news article and post it")      , wxITEM_NORMAL },
   { WXMENU_FILE_COLLECT,  gettext_noop("&Check mail\tShift-Ctrl-C"), gettext_noop("Check all incoming folders for new mail and download it now") , wxITEM_NORMAL },
   { WXMENU_SEPARATOR,     wxEmptyString,                  wxEmptyString                         , wxITEM_NORMAL },
   { WXMENU_FILE_PRINT_SETUP,    gettext_noop("P&rint setup"),     gettext_noop("Configure printing")  , wxITEM_NORMAL },
   { WXMENU_FILE_PAGE_SETUP,    gettext_noop("Pa&ge setup"),     gettext_noop("Configure page setup")  , wxITEM_NORMAL },
#ifdef USE_PS_PRINTING
   // extra postscript printing
   { WXMENU_FILE_PRINT_SETUP_PS,    gettext_noop("&Print PS Setup"),     gettext_noop("Configure PostScript printing")  , wxITEM_NORMAL },
// { WXMENU_FILE_PAGE_SETUP_PS,    gettext_noop("P&age PS Setup"),     gettext_noop("Configure PostScript page setup")  , wxITEM_NORMAL },
#endif // USE_PS_PRINTING

#ifdef USE_PYTHON
   { WXMENU_SEPARATOR,     wxEmptyString,                  wxEmptyString                         , wxITEM_NORMAL },
   { WXMENU_FILE_RUN_PYSCRIPT,   gettext_noop("R&un Python script..."),    gettext_noop("Run a simple python script"), wxITEM_NORMAL },
#endif // USE_PYTHON

   { WXMENU_SEPARATOR,     wxEmptyString,                  wxEmptyString                         , wxITEM_NORMAL },
   { WXMENU_FILE_SEND_OUTBOX, gettext_noop("&Send messages...\tShift-Ctrl-S"), gettext_noop("Sends messages still in outgoing mailbox"), wxITEM_NORMAL },

#ifdef USE_DIALUP
   { WXMENU_SEPARATOR,     wxEmptyString,                  wxEmptyString                         , wxITEM_NORMAL },
   { WXMENU_FILE_NET_ON,    gettext_noop("Conn&ect to network"), gettext_noop("Activate dial-up networking")        , wxITEM_NORMAL },
   { WXMENU_FILE_NET_OFF,   gettext_noop("Shut&down network"), gettext_noop("Shutdown dial-up networking")        , wxITEM_NORMAL },
#endif // USE_DIALUP

   { WXMENU_SEPARATOR,     wxEmptyString,                  wxEmptyString                         , wxITEM_NORMAL },
   { WXMENU_SUBMENU,       gettext_noop("&Identity"), wxEmptyString, wxITEM_NORMAL },
      { WXMENU_FILE_IDENT_ADD, gettext_noop("&Add..."),  gettext_noop("Create a new identity")        , wxITEM_NORMAL },
      { WXMENU_FILE_IDENT_CHANGE, gettext_noop("&Change..."), gettext_noop("Change the current identity")        , wxITEM_NORMAL },
      { WXMENU_FILE_IDENT_EDIT, gettext_noop("&Edit..."),  gettext_noop("Edit the current identity parameters")        , wxITEM_NORMAL },
      { WXMENU_FILE_IDENT_DELETE, gettext_noop("&Delete..."), gettext_noop("Remove an identity")        , wxITEM_NORMAL },
   { WXMENU_SUBMENU,       wxEmptyString, wxEmptyString, wxITEM_NORMAL },

   { WXMENU_SEPARATOR,     wxEmptyString,                  wxEmptyString                         , wxITEM_NORMAL },
   { WXMENU_FILE_IMPORT,   gettext_noop("I&mport..."),  gettext_noop("Import settings from another e-mail program")        , wxITEM_NORMAL },

   { WXMENU_SEPARATOR,     wxEmptyString,                  wxEmptyString                         , wxITEM_NORMAL },
   { WXMENU_FILE_CLOSE,    gettext_noop("&Close window\tCtrl-W"),     gettext_noop("Close this window")        , wxITEM_NORMAL },
   { WXMENU_SEPARATOR,     wxEmptyString,                  wxEmptyString                         , wxITEM_NORMAL },
   { WXMENU_FILE_AWAY_MODE,gettext_noop("Awa&y mode\tCtrl-Y"),        gettext_noop("Toggle unattended mode on/off"), wxITEM_CHECK },
   { WXMENU_FILE_EXIT,     gettext_noop("E&xit\tCtrl-Q"),             gettext_noop("Quit the application")     , wxITEM_NORMAL },

   // folder
   // available accels: DGJKQVWXYZ
   { WXMENU_FOLDER_OPEN,      gettext_noop("&Open...\tCtrl-O"),   gettext_noop("Open an existing message folder")                  , wxITEM_NORMAL },
   { WXMENU_FOLDER_OPEN_RO,   gettext_noop("Open read-onl&y..."), gettext_noop("Open a folder in read only mode")                  , wxITEM_NORMAL },
   { WXMENU_FOLDER_CREATE,    gettext_noop("&Create..."), gettext_noop("Create a new folder definition")               , wxITEM_NORMAL },
   { WXMENU_FOLDER_RENAME,    gettext_noop("Re&name..."), gettext_noop("Rename the selected folder")               , wxITEM_NORMAL },
   { WXMENU_FOLDER_MOVE,      gettext_noop("&Move..."), gettext_noop("Move the selected folder")               , wxITEM_NORMAL },
   { WXMENU_FOLDER_REMOVE,    gettext_noop("&Remove from tree"), gettext_noop("Remove the selected folder from the folder tree")               , wxITEM_NORMAL },
   { WXMENU_FOLDER_DELETE,    gettext_noop("&Delete"), gettext_noop("Delete all messages in the folder and remove it")               , wxITEM_NORMAL },
   { WXMENU_FOLDER_CLEAR,     gettext_noop("C&lear..."), gettext_noop("Delete all messages in the folder")               , wxITEM_NORMAL },
   { WXMENU_FOLDER_CLOSE,     gettext_noop("Clos&e"), gettext_noop("Close the current folder")               , wxITEM_NORMAL },
   { WXMENU_FOLDER_CLOSEALL,  gettext_noop("Close &all"), gettext_noop("Close all opened folders")               , wxITEM_NORMAL },
   { WXMENU_SEPARATOR,        wxEmptyString,                  wxEmptyString                         , wxITEM_NORMAL },
   { WXMENU_FOLDER_UPDATE,    gettext_noop("&Update"), gettext_noop("Update the shown status of this folder"), wxITEM_NORMAL },
   { WXMENU_FOLDER_UPDATEALL, gettext_noop("Update sub&tree"), gettext_noop("Update the status of all folders under the currently selected one in the folder tree"), wxITEM_NORMAL },
   { WXMENU_SEPARATOR,        wxEmptyString,                  wxEmptyString                         , wxITEM_NORMAL },
   { WXMENU_FOLDER_IMPORTTREE,gettext_noop("&Import file folders..."),
                              gettext_noop("Create folders for all files in a directory"), wxITEM_NORMAL },
   { WXMENU_FOLDER_BROWSESUB, gettext_noop("&Browse..."), gettext_noop("Show subfolders of the current folder")               , wxITEM_NORMAL },
   { WXMENU_SEPARATOR,        wxEmptyString,                  wxEmptyString                         , wxITEM_NORMAL },
   { WXMENU_SUBMENU,          gettext_noop("&Select"), wxEmptyString, wxITEM_NORMAL },
      { WXMENU_MSG_SELECTALL, gettext_noop("Select &all\tCtrl-A"),       gettext_noop("Select all messages")      , wxITEM_NORMAL },
      { WXMENU_MSG_SELECTUNREAD, gettext_noop("Select all &unread\tShift-Ctrl-A"), gettext_noop("Select all unread messages")      , wxITEM_NORMAL },
      { WXMENU_MSG_SELECTFLAGGED, gettext_noop("Select all &flagged"), gettext_noop("Select all flagged messages")      , wxITEM_NORMAL },
      { WXMENU_MSG_DESELECTALL,gettext_noop("&Deselect all\tCtrl-B"),    gettext_noop("Deselect all messages")    , wxITEM_NORMAL },
   { WXMENU_SUBMENU,          wxEmptyString, wxEmptyString, wxITEM_NORMAL },
   { WXMENU_FOLDER_SEARCH,    gettext_noop("Searc&h..."), gettext_noop("Search for the messages in folder(s)") , wxITEM_NORMAL },
   { WXMENU_SEPARATOR,        wxEmptyString,                  wxEmptyString                         , wxITEM_NORMAL },
   { WXMENU_FOLDER_FILTERS,   gettext_noop("&Filters..."), gettext_noop("Edit the filters to use for current folder")               , wxITEM_NORMAL },
   { WXMENU_FOLDER_PROP,      gettext_noop("&Properties..."), gettext_noop("Show the properties of the current folder")               , wxITEM_NORMAL },

   // normal edit

   // the available accelerators for this menu:
   // BHJNVUWYZ
   { WXMENU_EDIT_CUT,  gettext_noop("Cu&t\tCtrl-X"), gettext_noop("Cut selection and copy it to clipboard")           , wxITEM_NORMAL },
   { WXMENU_EDIT_COPY, gettext_noop("&Copy\tCtrl-C"), gettext_noop("Copy selection to clipboard")           , wxITEM_NORMAL },
   { WXMENU_EDIT_PASTE,gettext_noop("&Paste\tCtrl-V"), gettext_noop("Paste from clipboard")           , wxITEM_NORMAL },
   { WXMENU_EDIT_PASTE_QUOTED,gettext_noop("Paste &quoted\tShift-Ctrl-V"), gettext_noop("Paste from clipboard as quoted text")           , wxITEM_NORMAL },
   { WXMENU_EDIT_SELECT_ALL, gettext_noop("Select a&ll"), gettext_noop("Select the entire contents of the message view window"), wxITEM_NORMAL },
   { WXMENU_SEPARATOR,     wxEmptyString,                  wxEmptyString                         , wxITEM_NORMAL },
   { WXMENU_EDIT_FIND,  gettext_noop("&Find...\tF3"), gettext_noop("Find text in message") , wxITEM_NORMAL },
   { WXMENU_EDIT_FINDAGAIN, gettext_noop("Find a&gain\tCtrl-F3"), gettext_noop("Find the same text again") , wxITEM_NORMAL },
   { WXMENU_SEPARATOR,     wxEmptyString,                  wxEmptyString                         , wxITEM_NORMAL },
   { WXMENU_EDIT_ADB,      gettext_noop("Address boo&ks...\tCtrl-D"), gettext_noop("Edit the address book(s)") , wxITEM_NORMAL },
   { WXMENU_EDIT_PREF,     gettext_noop("Pr&eferences...\tCtrl-Shift-E"),   gettext_noop("Change options")           , wxITEM_NORMAL },
   { WXMENU_EDIT_MODULES,  gettext_noop("M&odules..."), gettext_noop("Choose which extension modules to use")           , wxITEM_NORMAL },
   { WXMENU_EDIT_FILTERS,  gettext_noop("Filter &rules..."), gettext_noop("Edit rules for message filtering")   , wxITEM_NORMAL },
   { WXMENU_EDIT_TEMPLATES,gettext_noop("Te&mplates..."), gettext_noop("Edit templates used for message composition")   , wxITEM_NORMAL },
   { WXMENU_EDIT_SPAM_CONFIG, gettext_noop("Sp&am filters..."), gettext_noop("Set the options for spam detection"), wxITEM_NORMAL },
   { WXMENU_SEPARATOR,     wxEmptyString,                  wxEmptyString                         , wxITEM_NORMAL },
   { WXMENU_EDIT_SAVE_PREF,gettext_noop("&Save preferences"), gettext_noop("Save options")             , wxITEM_NORMAL },
   { WXMENU_EDIT_EXPORT_PREF,gettext_noop("E&xport preferences..."), gettext_noop("Export options")             , wxITEM_NORMAL },
   { WXMENU_EDIT_IMPORT_PREF,gettext_noop("&Import preferences..."), gettext_noop("Import options")             , wxITEM_NORMAL },
   { WXMENU_EDIT_RESTORE_PREF, gettext_noop("Restore &defaults..."), gettext_noop("Restore default options values") , wxITEM_NORMAL },

   // msg

   // no more available accelerators for this menu!

   { WXMENU_MSG_OPEN,      gettext_noop("&Open"),             gettext_noop("View selected message in a separate window")    , wxITEM_NORMAL },
   { WXMENU_MSG_EDIT,      gettext_noop("Ed&it in composer\tCtrl-E"), gettext_noop("Edit selected message in composer")    , wxITEM_NORMAL },
   { WXMENU_MSG_PRINT,     gettext_noop("&Print\tCtrl-P"),    gettext_noop("Print this message")       , wxITEM_NORMAL },
   { WXMENU_MSG_PRINT_PREVIEW, gettext_noop("Print Previe&w"),gettext_noop("Preview a printout of this message")       , wxITEM_NORMAL },
#ifdef USE_PS_PRINTING
   { WXMENU_MSG_PRINT_PS,     gettext_noop("PS-Prin&t"),      gettext_noop("Print this message as PostScript")       , wxITEM_NORMAL },
   { WXMENU_MSG_PRINT_PREVIEW_PS,     gettext_noop("PS&-Print Preview"),      gettext_noop("View PostScript printout")       , wxITEM_NORMAL },
#endif // USE_PS_PRINTING
   { WXMENU_SEPARATOR,     wxEmptyString,                  wxEmptyString                         , wxITEM_NORMAL },
   { WXMENU_SUBMENU,       gettext_noop("Rep&ly"), wxEmptyString, wxITEM_NORMAL },
      // the available accelerators for this menu:
      // BCDGIJKOQUVWXYZ
      {
         WXMENU_MSG_REPLY,
         gettext_noop("&Reply\tCtrl-R"),
         gettext_noop("Reply to this message using default reply command"),
         wxITEM_NORMAL
      },
      {
         WXMENU_MSG_REPLY_WITH_TEMPLATE,
         gettext_noop("Reply with &template...\tShift-Ctrl-R"),
         gettext_noop("Reply to this message after choosing a template to use"),
         wxITEM_NORMAL
      },
      {
         WXMENU_MSG_REPLY_SENDER,
         gettext_noop("Reply to &sender"),
         gettext_noop("Reply to the sender of this message only"),
         wxITEM_NORMAL
      },
      {
         WXMENU_MSG_REPLY_SENDER_WITH_TEMPLATE,
         gettext_noop("Reply to sender with t&emplate..."),
         gettext_noop("Reply to the sender of this message after choosing a template to use"),
         wxITEM_NORMAL
      },
      {
         WXMENU_MSG_REPLY_ALL,
         gettext_noop("Reply to &all\tCtrl-G"),
         gettext_noop("Reply to all recipients of this message (group reply)"),
         wxITEM_NORMAL
      },
      {
         WXMENU_MSG_REPLY_ALL_WITH_TEMPLATE,
         gettext_noop("Reply to all with te&mplate...\tShift-Ctrl-G"),
         gettext_noop("Reply to all recipients of this message after choosing a template to use"),
         wxITEM_NORMAL
      },
      {
         WXMENU_MSG_REPLY_LIST,
         gettext_noop("Reply to &list\tCtrl-L"),
         gettext_noop("Reply to the mailing list this message was sent to"),
         wxITEM_NORMAL
      },
      {
         WXMENU_MSG_REPLY_LIST_WITH_TEMPLATE,
         gettext_noop("Reply to list with tem&plate...\tShift-Ctrl-L"),
         gettext_noop("Reply to the mailing list after choosing a template to use"),
         wxITEM_NORMAL
      },
      {
         WXMENU_MSG_FORWARD,
         gettext_noop("&Forward\tCtrl-F"),
         gettext_noop("Forward this message to another recipient"),
         wxITEM_NORMAL
      },
      {
         WXMENU_MSG_FORWARD_WITH_TEMPLATE,
         gettext_noop("Forward wit&h template...\tShift-Ctrl-F"),
         gettext_noop("Forward this message after choosing a template to use"),
         wxITEM_NORMAL
      },
      {
         WXMENU_MSG_FOLLOWUP_TO_NEWSGROUP,
         gettext_noop("Follow-up to &newsgroup"),
         gettext_noop("Follow-up (reply to) this message to the newsgroup"),
         wxITEM_NORMAL
      },
   { WXMENU_SUBMENU,       wxEmptyString, wxEmptyString, wxITEM_NORMAL },
   { WXMENU_MSG_BOUNCE,    gettext_noop("&Bounce..."), gettext_noop("Redirect the selected messages to another recipient"), wxITEM_NORMAL },
   { WXMENU_MSG_RESEND,    gettext_noop("R&esend..."), gettext_noop("Send the selected messages again."), wxITEM_NORMAL },
   { WXMENU_SEPARATOR,     wxEmptyString,                  wxEmptyString                         , wxITEM_NORMAL },
   { WXMENU_MSG_FILTER,    gettext_noop("Appl&y filter rules..."), gettext_noop("Apply filter rules to selected messages")     , wxITEM_NORMAL },
   { WXMENU_MSG_QUICK_FILTER, gettext_noop("&Quick filter..."), gettext_noop("Create a new filter for the selected message")     , wxITEM_NORMAL },
   { WXMENU_MSG_SAVE_TO_FILE, gettext_noop("Sa&ve as file..."), gettext_noop("Export message to a file")   , wxITEM_NORMAL },
   { WXMENU_MSG_SAVE_TO_FOLDER, gettext_noop("&Copy to folder..."),gettext_noop("Save message to another folder")   , wxITEM_NORMAL },
   { WXMENU_MSG_MOVE_TO_FOLDER, gettext_noop("&Move to folder..."),gettext_noop("Move message to another folder")   , wxITEM_NORMAL },
   { WXMENU_MSG_DELETE,    gettext_noop("&Delete"), gettext_noop("Delete messages(s) (mark it as deleted or move to trash)")      , wxITEM_NORMAL },
   { WXMENU_MSG_DELETE_EXPUNGE,    gettext_noop("&Zap"), gettext_noop("Permanently and unrecoverably delete message(s)")      , wxITEM_NORMAL },
   { WXMENU_MSG_UNDELETE,  gettext_noop("&Undelete"),         gettext_noop("Undelete message")         , wxITEM_NORMAL },
   { WXMENU_MSG_EXPUNGE,   gettext_noop("E&xpunge"), gettext_noop("Remove all messages marked as deleted from the folder.")                  , wxITEM_NORMAL },
   { WXMENU_SEPARATOR,     wxEmptyString,                  wxEmptyString                         , wxITEM_NORMAL },
   { WXMENU_MSG_GOTO_MSGNO,   gettext_noop("&Jump to message...\tCtrl-J"), gettext_noop("Select the message by its number")     , wxITEM_NORMAL },
   { WXMENU_MSG_NEXT_UNREAD,   gettext_noop("&Next unread\tCtrl-U"), gettext_noop("Select next unread message")     , wxITEM_NORMAL },
   { WXMENU_MSG_NEXT_FLAGGED,   gettext_noop("Nex&t flagged"), gettext_noop("Select next flagged message")     , wxITEM_NORMAL },
   { WXMENU_SEPARATOR,     wxEmptyString,                  wxEmptyString                         , wxITEM_NORMAL },
   { WXMENU_MSG_FLAG,      gettext_noop("Fla&g\tCtrl-I"), gettext_noop("Mark message as flagged/unflagged")         , wxITEM_NORMAL },
   { WXMENU_MSG_MARK_READ,   gettext_noop("Mark &read"), gettext_noop("Mark message as read"), wxITEM_NORMAL },
   { WXMENU_MSG_MARK_UNREAD,   gettext_noop("Mar&k unread\tShift-Ctrl-U"), gettext_noop("Mark message as unread"), wxITEM_NORMAL },
   { WXMENU_SEPARATOR,     wxEmptyString,                  wxEmptyString                         , wxITEM_NORMAL },
   { WXMENU_SUBMENU,       gettext_noop("&Spam"), wxEmptyString, wxITEM_NORMAL },
      { WXMENU_MSG_SPAM_MARK, gettext_noop("Dispose as &spam\tShift-Ctrl-Del"), gettext_noop("Classify the message as spam and treat it accordingly"), wxITEM_NORMAL },
      { WXMENU_MSG_SPAM_UNMARK, gettext_noop("Mark as &ham"), gettext_noop("Classify the message as non-spam"), wxITEM_NORMAL },
      { WXMENU_MSG_SPAM_CHECK, gettext_noop("Chec&k message...\tShift-Ctrl-K"), gettext_noop("Check if this message is spam"), wxITEM_NORMAL },
   { WXMENU_SUBMENU,       wxEmptyString, wxEmptyString, wxITEM_NORMAL },
   { WXMENU_SEPARATOR,     wxEmptyString,                  wxEmptyString                         , wxITEM_NORMAL },
   { WXMENU_MSG_SAVEADDRESSES, gettext_noop("Extract &addresses..."), gettext_noop("Save all or some addresses of the message in an address book"), wxITEM_NORMAL },

   // view

   // the available accelerators for this menu:
   // ABCDEFGIJKNOPQRSTUVXYZ
   { WXMENU_SUBMENU,       gettext_noop("&Viewer to use"), wxEmptyString, wxITEM_NORMAL },
   { WXMENU_SUBMENU,       wxEmptyString, wxEmptyString, wxITEM_NORMAL },
   { WXMENU_SEPARATOR,     wxEmptyString,                  wxEmptyString                         , wxITEM_NORMAL },
   { WXMENU_MSG_TOGGLEHEADERS, gettext_noop("Show &headers"), gettext_noop("Toggle display of message header") , wxITEM_CHECK },
   { WXMENU_MSG_SHOWRAWTEXT,  gettext_noop("Show ra&w message\tCtrl-Z"), gettext_noop("Show the raw message text") , wxITEM_NORMAL },
#ifdef EXPERIMENTAL_show_uid
   { WXMENU_MSG_SHOWUID, "Show message UID&L", wxEmptyString, wxITEM_NORMAL },
#endif // EXPERIMENTAL_show_uid
   { WXMENU_MSG_SHOWMIME,  gettext_noop("Show &MIME structure...\tShift-Ctrl-Z"), gettext_noop("Show the MIME structure of the message") , wxITEM_NORMAL },
   { WXMENU_SEPARATOR,     wxEmptyString,                  wxEmptyString                         , wxITEM_NORMAL },
   { WXMENU_SUBMENU,       gettext_noop("&Filters"), wxEmptyString, wxITEM_NORMAL },
   { WXMENU_SUBMENU,       wxEmptyString, wxEmptyString, wxITEM_NORMAL },

   // compose

   // the available accelerators for this menu:
   // ABFGJMQRUXYZ
   { WXMENU_COMPOSE_INSERTFILE,     gettext_noop("&Insert file...\tCtrl-I"),
                                    gettext_noop("Attach a file to the message")            , wxITEM_NORMAL },
   { WXMENU_COMPOSE_LOADTEXT,       gettext_noop("I&nsert text...\tCtrl-T"),
                                    gettext_noop("Insert text file")         , wxITEM_NORMAL },
   { WXMENU_SEPARATOR,     wxEmptyString,                  wxEmptyString                         , wxITEM_NORMAL },
   { WXMENU_COMPOSE_SEND,           gettext_noop("&Send\tShift-Ctrl-X"),
                                    gettext_noop("Send the message (either now or later depending on the options)")     , wxITEM_NORMAL },
   { WXMENU_COMPOSE_SEND_NOW,       gettext_noop("Send right n&ow"),
                                    gettext_noop("Send the message immediately")     , wxITEM_NORMAL },
   { WXMENU_COMPOSE_SAVE_AS_DRAFT,  gettext_noop("Close and save as &draft"),
                                    gettext_noop("Close the window and save the message in the drafts folder")     , wxITEM_NORMAL },
   { WXMENU_COMPOSE_SEND_LATER,     gettext_noop("Send &Later\tShift-Ctrl-L"),
                                    gettext_noop("Schedule the message to be send at a later time.")     , wxITEM_NORMAL },
   { WXMENU_COMPOSE_SEND_KEEP_OPEN, gettext_noop("Send and &keep\tShift-Ctrl-K"),
                                    gettext_noop("Send the message now and keep the editor open")     , wxITEM_NORMAL },
   { WXMENU_SEPARATOR,     wxEmptyString,                  wxEmptyString                         , wxITEM_NORMAL },
   { WXMENU_COMPOSE_PRINT,          gettext_noop("&Print\tCtrl-P"),            gettext_noop("Print the message")        , wxITEM_NORMAL },
   { WXMENU_COMPOSE_PREVIEW, gettext_noop("Previe&w..."),   gettext_noop("View the message as it would be sent"), wxITEM_NORMAL },
   { WXMENU_COMPOSE_SAVETEXT,gettext_noop("Save &text..."),   gettext_noop("Save (append) message text to file"), wxITEM_NORMAL },
   { WXMENU_COMPOSE_CLEAR, gettext_noop("&Clear"),            gettext_noop("Delete message contents")  , wxITEM_NORMAL },
   { WXMENU_COMPOSE_EVAL_TEMPLATE, gettext_noop("E&valuate template"), gettext_noop("Use the template to create skeleton of a message")  , wxITEM_NORMAL },
   { WXMENU_SEPARATOR,     wxEmptyString,                  wxEmptyString                         , wxITEM_NORMAL },
   { WXMENU_COMPOSE_EXTEDIT, gettext_noop("&External editor\tCtrl-E"),gettext_noop("Invoke alternative editor"), wxITEM_NORMAL },
   { WXMENU_SEPARATOR,     wxEmptyString,                  wxEmptyString                         , wxITEM_NORMAL },
   { WXMENU_COMPOSE_CUSTOM_HEADERS, gettext_noop("Custom &header...\tCtrl-H"), gettext_noop("Add/edit header fields not shown on the screen"), wxITEM_NORMAL },

   // language

   // the available accelerators for this menu:
   // JKMQVWXZ
   { WXMENU_LANG_DEFAULT, gettext_noop("De&fault"), gettext_noop("Use the default encoding"), wxITEM_RADIO },
   { WXMENU_LANG_ISO8859_1, gettext_noop("&Western European (ISO-8859-1)"), wxEmptyString, wxITEM_RADIO },
   { WXMENU_LANG_ISO8859_2, gettext_noop("Ce&ntral European (ISO-8859-2)"), wxEmptyString, wxITEM_RADIO },
   { WXMENU_LANG_ISO8859_3, gettext_noop("Es&peranto (ISO-8859-3)"), wxEmptyString, wxITEM_RADIO },
   { WXMENU_LANG_ISO8859_4, gettext_noop("Baltic &old (ISO-8859-4)"), wxEmptyString, wxITEM_RADIO },
   { WXMENU_LANG_ISO8859_5, gettext_noop("C&yrillic (ISO-8859-5)"), wxEmptyString, wxITEM_RADIO },
   { WXMENU_LANG_ISO8859_6, gettext_noop("&Arabic (ISO-8859-6)"), wxEmptyString, wxITEM_RADIO },
   { WXMENU_LANG_ISO8859_7, gettext_noop("&Greek (ISO-8859-7)"), wxEmptyString, wxITEM_RADIO },
   { WXMENU_LANG_ISO8859_8, gettext_noop("Heb&rew (ISO-8859-8)"), wxEmptyString, wxITEM_RADIO },
   { WXMENU_LANG_ISO8859_9, gettext_noop("&Turkish (ISO-8859-9)"), wxEmptyString, wxITEM_RADIO },
   { WXMENU_LANG_ISO8859_10, gettext_noop("Nor&dic (ISO-8859-10)"), wxEmptyString, wxITEM_RADIO },
   { WXMENU_LANG_ISO8859_11, gettext_noop("T&hai (ISO-8859-11)"), wxEmptyString, wxITEM_RADIO },
   { WXMENU_LANG_ISO8859_12, gettext_noop("&Indian (ISO-8859-12)"), wxEmptyString, wxITEM_RADIO },
   { WXMENU_LANG_ISO8859_13, gettext_noop("&Baltic (ISO-8859-13)"), wxEmptyString, wxITEM_RADIO },
   { WXMENU_LANG_ISO8859_14, gettext_noop("Ce&ltic (ISO-8859-14)"), wxEmptyString, wxITEM_RADIO },
   { WXMENU_LANG_ISO8859_15, gettext_noop("Western European with &Euro (ISO-8859-15)"), wxEmptyString, wxITEM_RADIO },
   { WXMENU_LANG_CP1250, gettext_noop("Windows Central European (CP 125&0)"), wxEmptyString, wxITEM_RADIO },
   { WXMENU_LANG_CP1251, gettext_noop("Windows Cyrillic (CP 125&1)"), wxEmptyString, wxITEM_RADIO },
   { WXMENU_LANG_CP1252, gettext_noop("Windows Western European (CP 125&2)"), wxEmptyString, wxITEM_RADIO },
   { WXMENU_LANG_CP1253, gettext_noop("Windows Greek (CP 125&3)"), wxEmptyString, wxITEM_RADIO },
   { WXMENU_LANG_CP1254, gettext_noop("Windows Turkish (CP 125&4)"), wxEmptyString, wxITEM_RADIO },
   { WXMENU_LANG_CP1255, gettext_noop("Windows Hebrew (CP 125&5)"), wxEmptyString, wxITEM_RADIO },
   { WXMENU_LANG_CP1256, gettext_noop("Windows Arabic (CP 125&6)"), wxEmptyString, wxITEM_RADIO },
   { WXMENU_LANG_CP1257, gettext_noop("Windows Baltic (CP 125&7)"), wxEmptyString, wxITEM_RADIO },
   { WXMENU_LANG_KOI8, gettext_noop("Russian (KOI&8-R)"), wxEmptyString, wxITEM_RADIO },
   { WXMENU_LANG_UTF7, gettext_noop("Uni&code (UTF-7)"), wxEmptyString, wxITEM_RADIO },
   { WXMENU_LANG_UTF8, gettext_noop("&Unicode (UTF-8)"), wxEmptyString, wxITEM_RADIO },
   { WXMENU_SEPARATOR, wxEmptyString, wxEmptyString, wxITEM_RADIO },
   { WXMENU_LANG_SET_DEFAULT, gettext_noop("&Set default encoding..."), wxEmptyString                         , wxITEM_NORMAL },

   // ADB book management
   { WXMENU_ADBBOOK_NEW,   gettext_noop("&New..."),           gettext_noop("Create a new address book"), wxITEM_NORMAL },
   { WXMENU_ADBBOOK_OPEN,  gettext_noop("&Open..."),          gettext_noop("Open an address book file"), wxITEM_NORMAL },
   { WXMENU_SEPARATOR,     wxEmptyString,                  wxEmptyString                         , wxITEM_NORMAL },
   { WXMENU_ADBBOOK_EXPORT,gettext_noop("&Export..."),        gettext_noop("Export address book data to another programs format"), wxITEM_NORMAL },
   { WXMENU_ADBBOOK_IMPORT,gettext_noop("&Import..."),        gettext_noop("Import data from an address book in another programs format"), wxITEM_NORMAL },
   { WXMENU_SUBMENU,       gettext_noop("&vCard"), wxEmptyString, wxITEM_NORMAL },
      { WXMENU_ADBBOOK_VCARD_IMPORT, gettext_noop("I&mport vCard file..."),
                                     gettext_noop("Create an entry from vCard file"), wxITEM_NORMAL },
      { WXMENU_ADBBOOK_VCARD_EXPORT, gettext_noop("E&xport vCard file..."),
                                     gettext_noop("Export entry to a vCard file"), wxITEM_NORMAL },
   { WXMENU_SUBMENU,       wxEmptyString, wxEmptyString, wxITEM_NORMAL },
   { WXMENU_SEPARATOR,     wxEmptyString,                  wxEmptyString                         , wxITEM_NORMAL },
#ifdef DEBUG
   { WXMENU_ADBBOOK_FLUSH, _T("&Flush"),                       _T("Save changes to disk")                         , wxITEM_NORMAL },
   { WXMENU_SEPARATOR,     wxEmptyString,                  wxEmptyString                         , wxITEM_NORMAL },
#endif // debug
   { WXMENU_ADBBOOK_PROP,  gettext_noop("&Properties..."), gettext_noop("View properties of the current address book")            , wxITEM_NORMAL },

   // ADB edit
   { WXMENU_ADBEDIT_NEW,   gettext_noop("&New entry..."),     gettext_noop("Create new entry/group")   , wxITEM_NORMAL },
   { WXMENU_ADBEDIT_DELETE,gettext_noop("&Delete"),           gettext_noop("Delete the selected items"), wxITEM_NORMAL },
   { WXMENU_ADBEDIT_RENAME,gettext_noop("&Rename..."),        gettext_noop("Rename the selected items"), wxITEM_NORMAL },
   { WXMENU_SEPARATOR,     wxEmptyString,                  wxEmptyString                         , wxITEM_NORMAL },
   { WXMENU_ADBEDIT_CUT,   gettext_noop("Cu&t"),              gettext_noop("Copy and delete selected items")                    , wxITEM_NORMAL },
   { WXMENU_ADBEDIT_COPY,  gettext_noop("&Copy"),             gettext_noop("Copy selected items")      , wxITEM_NORMAL },
   { WXMENU_ADBEDIT_PASTE, gettext_noop("&Paste"),            gettext_noop("Paste here")               , wxITEM_NORMAL },
   { WXMENU_SEPARATOR,     wxEmptyString,                  wxEmptyString                         , wxITEM_NORMAL },
   { WXMENU_ADBEDIT_UNDO,  gettext_noop("&Undo changes"),     gettext_noop("Undo all changes to the entry being edited")       , wxITEM_NORMAL },

   // ADB search
   { WXMENU_ADBFIND_FIND,  gettext_noop("&Find..."),          gettext_noop("Find entry by name or contents")                 , wxITEM_NORMAL },
   { WXMENU_ADBFIND_NEXT,  gettext_noop("Find &next\tCtrl-G"),        gettext_noop("Go to the next match")     , wxITEM_NORMAL },
   { WXMENU_SEPARATOR,     wxEmptyString,                  wxEmptyString                         , wxITEM_NORMAL },
   { WXMENU_ADBFIND_GOTO,  gettext_noop("&Go To..."),         gettext_noop("Go to specified entry")    , wxITEM_NORMAL },

   // help
   { WXMENU_HELP_ABOUT,    gettext_noop("&About..."),         gettext_noop("Displays the program information and copyright")  , wxITEM_NORMAL },
   { WXMENU_HELP_TIP,      gettext_noop("Show a &tip..."),    gettext_noop("Show a tip about using Mahogany")  , wxITEM_NORMAL },
   { WXMENU_HELP_RELEASE_NOTES,    gettext_noop("&Release Notes..."), gettext_noop("Displays notes about the current release.")  , wxITEM_NORMAL },
   { WXMENU_HELP_FAQ,    gettext_noop("&FAQ..."),         gettext_noop("Displays the list of Frequently Asked Questions.")  , wxITEM_NORMAL },
   { WXMENU_SEPARATOR,     wxEmptyString,                  wxEmptyString                         , wxITEM_NORMAL },
   { WXMENU_HELP_CONTEXT, gettext_noop("&Help\tCtrl-F1"),    gettext_noop("Help on current context..."), wxITEM_NORMAL },
   { WXMENU_HELP_CONTENTS, gettext_noop("Help &Contents\tF1"),    gettext_noop("Contents of help system..."), wxITEM_NORMAL },
   { WXMENU_HELP_SEARCH,   gettext_noop("&Search Help..."),      gettext_noop("Search help system for keyword..."), wxITEM_NORMAL },
   { WXMENU_HELP_COPYRIGHT,   gettext_noop("C&opyright"), gettext_noop("Show Copyright."), wxITEM_NORMAL },
};

// consistency check which ensures (well, helps to ensure) that the array
// and enum are in sync
wxCOMPILE_TIME_ASSERT( WXSIZEOF(g_aMenuItems) == WXMENU_END - WXMENU_BEGIN,
                       WrongMenuDataCount );

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// menu stuff
// ----------------------------------------------------------------------------

static inline const MenuItemInfo& GetMenuItem(int n)
{
   n -= WXMENU_BEGIN;

   ASSERT_MSG( 0 <= n && (size_t)n < WXSIZEOF(g_aMenuItems),
               _T("bad menu item index") );

   return g_aMenuItems[n];
}

void AppendToMenu(wxMenu *menu, int& n)
{
   int id = GetMenuItem(n).idMenu;
   switch ( id )
   {
      case WXMENU_SEPARATOR:
         menu->AppendSeparator();
         break;

      case WXMENU_SUBMENU:
         {
            // append all entries until the next one with id == WXMENU_SUBMENU
            // to a submenu
            wxMenu *submenu = new wxMenu();

            int nSubMenu = n;
            for ( n++; GetMenuItem(n).idMenu != WXMENU_SUBMENU; n++ )
            {
               AppendToMenu(submenu, n);
            }

            const MenuItemInfo& mii = GetMenuItem(nSubMenu);

            // we don't have the menu id field for the submenus so use their
            // offset in the menu item array
            //
            // note that if this changes, FindSubmenu() will have to change too!
            menu->Append(WXMENU_BEGIN + n,
                         wxGetTranslation(mii.label),
                         submenu,
                         wxGetTranslation(mii.helpstring));
         }
         break;

      default:
         const MenuItemInfo& mii = GetMenuItem(n);

         menu->Append(id,
                      wxGetTranslation(mii.label),
                      wxGetTranslation(mii.helpstring),
                      mii.kind);
   }
}

void AppendToMenu(wxMenu *menu, int nFirst, int nLast)
{
   // in debug mode we also verify if the keyboard accelerators are ok
#ifdef DEBUG
   wxString strAccels;
#endif

   for ( int n = nFirst; n <= nLast; n++ ) {
#ifdef DEBUG
      const wxChar *label = wxGetTranslation(GetMenuItem(n).label);
      if ( !IsEmpty(wxConvertWX2MB(label)) ) {
         const wxChar *p = wxStrchr(label, '&');
         if ( p == NULL ) {
            wxLogWarning(_T("Menu label '%s' doesn't have keyboard accelerator."),
                         label);
         }
         else {
            char c = toupper(*++p);
            if ( strAccels.Find(c) != -1 ) {
               wxLogWarning(_T("Duplicate accelerator %c (in '%s')"), c, label);
            }

            strAccels += c;
         }
      }
      // else: it must be a separator

#endif //DEBUG

      AppendToMenu(menu, n);
   }
}

// ----------------------------------------------------------------------------
// toolbar stuff
// ----------------------------------------------------------------------------

// add the given button to the toolbar
void AddToolbarButton(wxToolBar *toolbar, int flags, int nButton)
{
   if ( nButton == WXTBAR_SEP )
   {
      toolbar->AddSeparator();
   }
   else
   {
      const TbarItemInfo& tbii = g_aToolBarData[nButton];

      // we use either bitmap or label, but not both
      wxString label;
      if ( flags & TbarShow_Text )
      {
         label = tbii.text;
      }

      wxBitmap bmp;
      if ( flags & TbarShow_Icons )
      {
         wxString iconName = tbii.bmp;
#ifdef __WXMSW__
         bmp = mApplication->GetIconManager()->GetBitmap(iconName);
#else
         bmp = mApplication->GetIconManager()->GetIcon(iconName);
#endif
      }

      toolbar->AddTool(tbii.id, label, bmp, String(wxGetTranslation(tbii.tooltip)));
   }
}

// add all buttons for the given frame to the toolbar
void CreateMToolbar(wxFrame *parent, wxFrameId frameId)
{
   wxASSERT_MSG( frameId < WXFRAME_MAX, _T("unknown frame id in CreateMToolbar") );

   // first radiobox button (0) corresponds to TbarShow_Icons (1)
   int flags = READ_APPCONFIG(MP_TBARIMAGES);
   flags++;

   long style = wxTB_HORIZONTAL | wxTB_FLAT | wxTB_DOCKABLE | wxTB_TEXT;
   if ( !(flags & TbarShow_Icons) )
      style |= wxTB_NOICONS;

   wxToolBar *toolbar = parent->CreateToolBar(style);

#ifdef __WXMSW__
   if ( flags & TbarShow_Icons )
   {
      // we use the icons of non standard size
      toolbar->SetToolBitmapSize(wxSize(24, 24));
   }
#endif

   const int *aTbarIcons = g_aFrameToolbars[frameId];

#ifdef __WXGTK__
   // it looks better like this under GTK
   AddToolbarButton(toolbar, flags, WXTBAR_SEP);
#endif

   for ( size_t nButton = 0; aTbarIcons[nButton] != -1 ; nButton++ ) {
      AddToolbarButton(toolbar, flags, aTbarIcons[nButton]);
   }

   // show the identity combo in the main frame
   if ( frameId == WXFRAME_MAIN )
   {
      wxControl *combo = CreateIdentCombo(toolbar);
      if (combo) toolbar->AddControl(combo);
   }

   // next add the "Help" button
   AddToolbarButton(toolbar, flags, WXTBAR_SEP);
   AddToolbarButton(toolbar, flags, WXTBAR_MAIN_HELP);
   AddToolbarButton(toolbar, flags, WXTBAR_SEP);

   // finally, add the "Close" icon - but only if we're not the main frame
   if ( frameId != WXFRAME_MAIN )
   {
      AddToolbarButton(toolbar, flags, WXTBAR_CLOSE);
      AddToolbarButton(toolbar, flags, WXTBAR_SEP);
   }

   // and the "Exit" button for all frames
   AddToolbarButton(toolbar, flags, WXTBAR_MAIN_EXIT);

   // must do it for the toolbar to be shown properly
   toolbar->Realize();
}


extern void
CreateMMenu(wxMenuBar *menubar, int menu_begin, int menu_end, const wxString &caption)
{
   wxMenu *pMenu = new wxMenu(wxEmptyString, wxMENU_TEAROFF);
   AppendToMenu(pMenu, menu_begin+1, menu_end);
   menubar->Append(pMenu, caption);
}


/// Function to enable/disable a given menu:
extern void
EnableMMenu(MMenuId id, wxWindow *win, bool enable)
{
   wxFrame *frame = GetFrame(win);
   CHECK_RET(frame, _T("no parent frame in EnableMMenu"));

   wxMenuBar *mb = frame->GetMenuBar();
   CHECK_RET(mb, _T("no menu bar in EnableMMenu"));

   // only the main frame has MMenu_Folder, so adjust the menu index
   if ( frame->GetParent() && (id > MMenu_Folder) )
   {
      id = (MMenuId)(id - 1);
   }

   if(id == MMenu_Plugins)
   {
      if(mb->GetMenuCount() > MMenu_Help)
         mb->EnableTop(id,enable);
      else
      {
         // no Plugin menu
         if(id != MMenu_Plugins)
            mb->EnableTop(id-1,enable);
      }
   }
   else
   {
      mb->EnableTop(id, enable);
   }
}

extern wxMenu *
FindSubmenu(wxWindow *win, int id)
{
   wxFrame *frame = GetFrame(win);
   CHECK( frame, NULL, _T("no parent frame in FindSubmenu") );

   wxMenuBar *mb = frame->GetMenuBar();
   CHECK( mb, NULL, _T("no menu bar in FindSubmenu") );

   // we use the index in the array/enum as id for the submenus, see
   // AppendToMenu()
   wxMenuItem *menuitem = mb->FindItem(WXMENU_BEGIN + id);
   CHECK( menuitem, NULL, _T("no such menuitem in FindSubmenu") );

   ASSERT_MSG( menuitem->IsSubMenu(),
               _T("this menuitem is not a submenu in FindSubmenu") );

   return menuitem->GetSubMenu();
}

extern void
EnableToolbarButton(wxToolBar *toolbar, int nButton, bool enable)
{
   CHECK_RET( toolbar, _T("no toolbar in EnableToolbarButton") );

   toolbar->EnableTool(g_aToolBarData[nButton].id, enable);
}

// ----------------------------------------------------------------------------
// language menu stuff
// ----------------------------------------------------------------------------

// Check the entry corresponding to this encoding in LANG submenu
extern void CheckLanguageInMenu(wxWindow *win, wxFontEncoding encoding)
{
   wxFrame *frame = GetFrame(win);
   CHECK_RET(frame, _T("no parent frame in CheckLanguageInMenu"));

   wxMenuBar *mb = frame->GetMenuBar();
   CHECK_RET(mb, _T("no menu bar in CheckLanguageInMenu"));

   int id;
   switch ( encoding )
   {
      case wxFONTENCODING_ISO8859_1:
      case wxFONTENCODING_ISO8859_2:
      case wxFONTENCODING_ISO8859_3:
      case wxFONTENCODING_ISO8859_4:
      case wxFONTENCODING_ISO8859_5:
      case wxFONTENCODING_ISO8859_6:
      case wxFONTENCODING_ISO8859_7:
      case wxFONTENCODING_ISO8859_8:
      case wxFONTENCODING_ISO8859_9:
      case wxFONTENCODING_ISO8859_10:
      case wxFONTENCODING_ISO8859_11:
      case wxFONTENCODING_ISO8859_12:
      case wxFONTENCODING_ISO8859_13:
      case wxFONTENCODING_ISO8859_14:
      case wxFONTENCODING_ISO8859_15:
         id = WXMENU_LANG_ISO8859_1 + encoding - wxFONTENCODING_ISO8859_1;
         break;

      case wxFONTENCODING_CP1250:
      case wxFONTENCODING_CP1251:
      case wxFONTENCODING_CP1252:
      case wxFONTENCODING_CP1253:
      case wxFONTENCODING_CP1254:
      case wxFONTENCODING_CP1255:
      case wxFONTENCODING_CP1256:
      case wxFONTENCODING_CP1257:
         id = WXMENU_LANG_CP1250 + encoding - wxFONTENCODING_CP1250;
         break;

      case wxFONTENCODING_KOI8:
         id = WXMENU_LANG_KOI8;
         break;

      case wxFONTENCODING_UTF7:
         id = WXMENU_LANG_UTF7;
         break;

      case wxFONTENCODING_UTF8:
         id = WXMENU_LANG_UTF8;
         break;

      default:
         wxFAIL_MSG( _T("Unexpected encoding in CheckLanguageInMenu") );

      case wxFONTENCODING_DEFAULT:
         id = WXMENU_LANG_DEFAULT;
   }

#ifdef wxHAS_RADIO_MENU_ITEMS
   mb->Check(id, TRUE);
#else // !wxHAS_RADIO_MENU_ITEMS
   // emulate the "radio menu items" as wxWin doesn't yet have this
   static const int menuIds[] =
   {
      WXMENU_LANG_DEFAULT,
      WXMENU_LANG_ISO8859_1,
      WXMENU_LANG_ISO8859_2,
      WXMENU_LANG_ISO8859_3,
      WXMENU_LANG_ISO8859_4,
      WXMENU_LANG_ISO8859_5,
      WXMENU_LANG_ISO8859_6,
      WXMENU_LANG_ISO8859_7,
      WXMENU_LANG_ISO8859_8,
      WXMENU_LANG_ISO8859_9,
      WXMENU_LANG_ISO8859_10,
      WXMENU_LANG_ISO8859_11,
      WXMENU_LANG_ISO8859_12,
      WXMENU_LANG_ISO8859_13,
      WXMENU_LANG_ISO8859_14,
      WXMENU_LANG_ISO8859_15,
      WXMENU_LANG_CP1250,
      WXMENU_LANG_CP1251,
      WXMENU_LANG_CP1252,
      WXMENU_LANG_CP1253,
      WXMENU_LANG_CP1254,
      WXMENU_LANG_CP1255,
      WXMENU_LANG_CP1256,
      WXMENU_LANG_CP1257,
      WXMENU_LANG_KOI8,
      WXMENU_LANG_UTF7,
      WXMENU_LANG_UTF8,
   };

   for ( size_t nId = 0; nId < WXSIZEOF(menuIds); nId++ )
   {
      int idCur = menuIds[nId];
      mb->Check(idCur, idCur == id);
   }
#endif // wxHAS_RADIO_MENU_ITEMS/!wxHAS_RADIO_MENU_ITEMS
}

// translate the menu event id to the encoding
extern wxFontEncoding GetEncodingFromMenuCommand(int id)
{
   int encoding;
   switch ( id )
   {
      default:
         wxFAIL_MSG(_T("unexpected menu event id in GetEncodingFromMenuEvent"));
         // fall through

      case WXMENU_LANG_DEFAULT:
         encoding = READ_APPCONFIG(MP_MSGVIEW_DEFAULT_ENCODING);
         break;

      case WXMENU_LANG_ISO8859_1:
      case WXMENU_LANG_ISO8859_2:
      case WXMENU_LANG_ISO8859_3:
      case WXMENU_LANG_ISO8859_4:
      case WXMENU_LANG_ISO8859_5:
      case WXMENU_LANG_ISO8859_6:
      case WXMENU_LANG_ISO8859_7:
      case WXMENU_LANG_ISO8859_8:
      case WXMENU_LANG_ISO8859_9:
      case WXMENU_LANG_ISO8859_10:
      case WXMENU_LANG_ISO8859_11:
      case WXMENU_LANG_ISO8859_12:
      case WXMENU_LANG_ISO8859_13:
      case WXMENU_LANG_ISO8859_14:
      case WXMENU_LANG_ISO8859_15:
         encoding = wxFONTENCODING_ISO8859_1 + id - WXMENU_LANG_ISO8859_1;
         break;

      case WXMENU_LANG_CP1250:
      case WXMENU_LANG_CP1251:
      case WXMENU_LANG_CP1252:
      case WXMENU_LANG_CP1253:
      case WXMENU_LANG_CP1254:
      case WXMENU_LANG_CP1255:
      case WXMENU_LANG_CP1256:
      case WXMENU_LANG_CP1257:
         encoding = wxFONTENCODING_CP1250 + id - WXMENU_LANG_CP1250;
         break;

      case WXMENU_LANG_KOI8:
         encoding = wxFONTENCODING_KOI8;
         break;

      case WXMENU_LANG_UTF7:
         encoding = wxFONTENCODING_UTF7;
         break;

      case WXMENU_LANG_UTF8:
         encoding = wxFONTENCODING_UTF8;
         break;
   }

   return (wxFontEncoding)encoding;
}

/* vi: set tw=0: */
