///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   gui/wxMenuDefs.h - items definitions for menus and toolbars
// Purpose:     the definitions of all menu (and toolbar, inspite the name)
//              items are gathered here complemented with some convenience
//              functions for menu/toolbar creation
// Author:      Karsten Ballüder
// Modified by: Vadim Zeitlin
// Created:     09.08.98
// CVS-ID:      $Id$
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _GUI_WXMENUDEFS_H_
#define _GUI_WXMENUDEFS_H_

#include <wx/fontenc.h>

class WXDLLEXPORT wxMenu;
class WXDLLEXPORT wxToolBar;
class WXDLLEXPORT wxWindow;

// ----------------------------------------------------------------------------
// menu functions
// ----------------------------------------------------------------------------

/// append an item to menu
extern void AppendToMenu(wxMenu *menu, int& n);

/// appends all items in the given range (see enum below) to the menu
extern void AppendToMenu(wxMenu *menu, int nFirst, int nLast);

/// check wether an ID belongs to a given menu
#define  WXMENU_CONTAINS(menu,id)   \
   (WXMENU_##menu##_BEGIN < (id) && WXMENU_##menu##_END >= (id))

/// creates a menu, appends all it's items to it and appends it to menu bar
extern void CreateMMenu(class wxMenuBar *menubar,
                        int menu_begin, int menu_end,
                        const wxString& caption);

#define WXADD_MENU(menubar,menu,caption) \
    CreateMMenu(menubar,  WXMENU_##menu##_BEGIN,WXMENU_##menu##_END,caption)

/// A numeric Id to identify the menu for purpose of enabling/disabling:
enum MMenuId
{
   MMenu_File,
   MMenu_Folder,
   MMenu_Edit,
   MMenu_Message,
   MMenu_View,
   MMenu_Language,
   MMenu_Help,
   MMenu_Plugins
};

/// Enable/disable a given menu:for the frame containing the given window
extern void EnableMMenu(MMenuId id, wxWindow *win, bool enable);

/// Find a submenu with the given id (from the WXMENU_XXX enum below)
extern wxMenu *FindSubmenu(wxWindow *win, int id);

/// Get the encoding selected by the user from the language menu
extern wxFontEncoding GetEncodingFromMenuCommand(int id);

/// Check the entry corresponding to this encoding in the language submenu
extern void CheckLanguageInMenu(wxWindow *win, wxFontEncoding encoding);

// ----------------------------------------------------------------------------
// all existing menu commands
// ----------------------------------------------------------------------------

/** Definition of all numeric menu IDs.
    Include each menu in WXMENU_menuname_BEGIN and WXMENU_menuname_END, so it
    can be tested for by WXMENU_CONTAIN(menu,id).

    There should be no gaps in this enum (AppendToMenu assumes it) and the
    value of each _BEGIN must be the same as the value of the preceding _END
*/
enum
{
   WXMENU_SUBMENU = -2,
   WXMENU_SEPARATOR = -1,

   // under MSW the system may generate command events with low ids like
   // IDOK/IDCANCEL/... itself (for example IDCANCEL == 2 is generated when
   // Escape is pressed) which shouldn't conflict with our commands, so start
   // at something > 0
   WXMENU_BEGIN = 100,
   WXMENU_FILE_BEGIN = WXMENU_BEGIN - 1,
   WXMENU_FILE_COMPOSE,
   WXMENU_FILE_COMPOSE_WITH_TEMPLATE,
   WXMENU_FILE_POST,
   WXMENU_FILE_COLLECT,
   WXMENU_FILE_SEP1,
   WXMENU_FILE_PRINT_SETUP,
   WXMENU_FILE_PAGE_SETUP,
#ifdef USE_PS_PRINTING
   // extra postscript printing
   WXMENU_FILE_PRINT_SETUP_PS,
// WXMENU_FILE_PAGE_SETUP_PS,
#endif // USE_PS_PRINTING
#ifdef USE_PYTHON
   WXMENU_FILE_SEP2,
   WXMENU_FILE_RUN_PYSCRIPT,
#endif // USE_PYTHON

   WXMENU_FILE_SEP3,
   WXMENU_FILE_SEND_OUTBOX,
#ifdef USE_DIALUP
   WXMENU_FILE_SEP4,
   WXMENU_FILE_NET_ON,
   WXMENU_FILE_NET_OFF,
#endif // USE_DIALUP
   WXMENU_FILE_SEP5,
   WXMENU_FILE_IDENT_SUBMENU_BEGIN,
      WXMENU_FILE_IDENT_ADD,
      WXMENU_FILE_IDENT_CHANGE,
      WXMENU_FILE_IDENT_EDIT,
      WXMENU_FILE_IDENT_DELETE,
   WXMENU_FILE_IDENT_SUBMENU_END,
   WXMENU_FILE_SEP6,
   WXMENU_FILE_IMPORT,
   WXMENU_FILE_SEP7,
   WXMENU_FILE_CLOSE,
   WXMENU_FILE_SEP8,
   WXMENU_FILE_AWAY_MODE,
   WXMENU_FILE_EXIT,
   WXMENU_FILE_END = WXMENU_FILE_EXIT,

   WXMENU_FOLDER_BEGIN = WXMENU_FILE_END,
   WXMENU_FOLDER_OPEN,
   WXMENU_FOLDER_OPEN_RO,
   WXMENU_FOLDER_CREATE,
   WXMENU_FOLDER_RENAME,
   WXMENU_FOLDER_MOVE,
   WXMENU_FOLDER_REMOVE,
   WXMENU_FOLDER_DELETE,
   WXMENU_FOLDER_CLEAR,
   WXMENU_FOLDER_CLOSE,
   WXMENU_FOLDER_CLOSEALL,
   WXMENU_FOLDER_SEP1,
   WXMENU_FOLDER_UPDATE,
   WXMENU_FOLDER_UPDATEALL,
   WXMENU_FOLDER_SEP2,
   WXMENU_FOLDER_BROWSESUB,
   WXMENU_FOLDER_IMPORTTREE,
   WXMENU_FOLDER_SEP3,
   WXMENU_FOLDER_SEARCH,
   WXMENU_FOLDER_SEP4,
   WXMENU_FOLDER_FILTERS,
   WXMENU_FOLDER_PROP,
   WXMENU_FOLDER_END = WXMENU_FOLDER_PROP,

   WXMENU_EDIT_BEGIN = WXMENU_FOLDER_END,
   WXMENU_EDIT_CUT,
   WXMENU_EDIT_COPY,
   WXMENU_EDIT_PASTE,
   WXMENU_EDIT_PASTE_QUOTED,
   WXMENU_EDIT_SELECT_ALL,
   WXMENU_EDIT_SEP1,
   WXMENU_EDIT_FIND,
   WXMENU_EDIT_FINDAGAIN,
   WXMENU_EDIT_SEP2,
   WXMENU_EDIT_ADB,
   WXMENU_EDIT_PREF,
   WXMENU_EDIT_MODULES,
   WXMENU_EDIT_FILTERS,
   WXMENU_EDIT_TEMPLATES,
   WXMENU_EDIT_SEP3,
   WXMENU_EDIT_SAVE_PREF,
   WXMENU_EDIT_EXPORT_PREF,
   WXMENU_EDIT_IMPORT_PREF,
   WXMENU_EDIT_RESTORE_PREF,
   WXMENU_EDIT_END = WXMENU_EDIT_RESTORE_PREF,

   // if you modify the entries in the message menu don't forget to update
   // MSGVIEW entries below too!
   WXMENU_MSG_BEGIN = WXMENU_EDIT_END,
   WXMENU_MSG_OPEN,
   WXMENU_MSG_EDIT,
   WXMENU_MSG_PRINT,
   WXMENU_MSG_PRINT_PREVIEW,
#ifdef USE_PS_PRINTING
   // extra postscript printing
   WXMENU_MSG_PRINT_PS,
   WXMENU_MSG_PRINT_PREVIEW_PS,
#endif
   WXMENU_MSG_SEP1,
   WXMENU_MSG_SEND_SUBMENU_BEGIN,
      WXMENU_MSG_REPLY,
      WXMENU_MSG_REPLY_WITH_TEMPLATE,
      WXMENU_MSG_REPLY_SENDER,
      WXMENU_MSG_REPLY_SENDER_WITH_TEMPLATE,
      WXMENU_MSG_REPLY_ALL,
      WXMENU_MSG_REPLY_ALL_WITH_TEMPLATE,
      WXMENU_MSG_REPLY_LIST,
      WXMENU_MSG_REPLY_LIST_WITH_TEMPLATE,
      WXMENU_MSG_FORWARD,
      WXMENU_MSG_FORWARD_WITH_TEMPLATE,
      WXMENU_MSG_FOLLOWUP_TO_NEWSGROUP,
   WXMENU_MSG_SEND_SUBMENU_END,
   WXMENU_MSG_BOUNCE,
   WXMENU_MSG_RESEND,
   WXMENU_MSG_SEP2,

   WXMENU_MSG_FILTER,
   WXMENU_MSG_QUICK_FILTER,
   WXMENU_MSG_SAVE_TO_FILE,
   WXMENU_MSG_SAVE_TO_FOLDER,
   WXMENU_MSG_MOVE_TO_FOLDER,
   WXMENU_MSG_DELETE,
   WXMENU_MSG_DELETE_EXPUNGE,
   WXMENU_MSG_UNDELETE,
#if 0
   WXMENU_MSG_DELDUPLICATES,
#endif // 0
   WXMENU_MSG_EXPUNGE,
   WXMENU_MSG_SEP3,

   WXMENU_MSG_GOTO_MSGNO,
   WXMENU_MSG_NEXT_UNREAD,
   WXMENU_MSG_NEXT_FLAGGED,
   WXMENU_MSG_SEP4,

   WXMENU_MSG_FLAG,
   WXMENU_MSG_MARK_READ,
   WXMENU_MSG_MARK_UNREAD,
   WXMENU_MSG_SEP5,

   WXMENU_MSG_SELECT_SUBMENU_BEGIN,
      WXMENU_MSG_SELECTALL,
      WXMENU_MSG_SELECTUNREAD,
      WXMENU_MSG_SELECTFLAGGED,
      WXMENU_MSG_DESELECTALL,
   WXMENU_MSG_SELECT_SUBMENU_END,
   WXMENU_MSG_SEP6,

   WXMENU_MSG_SAVEADDRESSES,
   WXMENU_MSG_END = WXMENU_MSG_SAVEADDRESSES,

   WXMENU_VIEW_BEGIN = WXMENU_MSG_END,
   WXMENU_VIEW_VIEWERS_SUBMENU_BEGIN,
      // this submenu is filled dynamically and the ids of its items start at
      // WXMENU_VIEW_VIEWERS_BEGIN defined below
   WXMENU_VIEW_VIEWERS_SUBMENU_END,
   WXMENU_VIEW_SEP1,
   // for backwards compatibility the commands below have MSG (as they were in
   // the "Message" menu earlier), even though if they're in "View" menu now
   WXMENU_MSG_TOGGLEHEADERS,
   WXMENU_MSG_SHOWRAWTEXT,
#ifdef EXPERIMENTAL_show_uid
   WXMENU_MSG_SHOWUID,
#endif // EXPERIMENTAL_show_uid
   WXMENU_MSG_SHOWMIME,
   WXMENU_VIEW_SEP2,
   WXMENU_VIEW_FILTERS_SUBMENU_BEGIN,
      // this submenu is filled dynamically and the ids of its items start at
      // WXMENU_VIEW_FILTERS_BEGIN defined below
   WXMENU_VIEW_FILTERS_SUBMENU_END,
   WXMENU_VIEW_END = WXMENU_VIEW_FILTERS_SUBMENU_END,

   WXMENU_COMPOSE_BEGIN = WXMENU_VIEW_END,
   WXMENU_COMPOSE_INSERTFILE,
   WXMENU_COMPOSE_LOADTEXT,
   WXMENU_COMPOSE_SEND,
   WXMENU_COMPOSE_SAVE_AS_DRAFT,
   WXMENU_COMPOSE_SEND_LATER,
   WXMENU_COMPOSE_SEND_KEEP_OPEN,
   WXMENU_COMPOSE_SEP1,
   WXMENU_COMPOSE_PRINT,
   WXMENU_COMPOSE_PREVIEW,
   WXMENU_COMPOSE_SAVETEXT,
   WXMENU_COMPOSE_CLEAR,
   WXMENU_COMPOSE_EVAL_TEMPLATE,
   WXMENU_COMPOSE_SEP2,
   WXMENU_COMPOSE_EXTEDIT,
   WXMENU_COMPOSE_SEP3,
   WXMENU_COMPOSE_CUSTOM_HEADERS,
   WXMENU_COMPOSE_END = WXMENU_COMPOSE_CUSTOM_HEADERS,

   WXMENU_LANG_BEGIN = WXMENU_COMPOSE_END,
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
   WXMENU_LANG_SEP5,
   WXMENU_LANG_SET_DEFAULT,
   WXMENU_LANG_END = WXMENU_LANG_SET_DEFAULT,

   WXMENU_ADBBOOK_BEGIN = WXMENU_LANG_END,
   WXMENU_ADBBOOK_NEW,
   WXMENU_ADBBOOK_OPEN,
   WXMENU_ADBBOOK_SEP,
   WXMENU_ADBBOOK_EXPORT,
   WXMENU_ADBBOOK_IMPORT,
   WXMENU_ADBBOOK_VCARD_SUBMENU_START,
      WXMENU_ADBBOOK_VCARD_IMPORT,
      WXMENU_ADBBOOK_VCARD_EXPORT,
   WXMENU_ADBBOOK_VCARD_SUBMENU_END,
   WXMENU_ADBBOOK_SEP3,
#ifdef DEBUG
   WXMENU_ADBBOOK_FLUSH,
   WXMENU_ADBBOOK_SEP2,
#endif // debug
   WXMENU_ADBBOOK_PROP,
   WXMENU_ADBBOOK_END = WXMENU_ADBBOOK_PROP,

   WXMENU_ADBEDIT_BEGIN = WXMENU_ADBBOOK_END,
   WXMENU_ADBEDIT_NEW,
   WXMENU_ADBEDIT_DELETE,
   WXMENU_ADBEDIT_RENAME,
   WXMENU_ADBEDIT_SEP1,
   WXMENU_ADBEDIT_CUT,
   WXMENU_ADBEDIT_COPY,
   WXMENU_ADBEDIT_PASTE,
   WXMENU_ADBEDIT_SEP2,
   WXMENU_ADBEDIT_UNDO,
   WXMENU_ADBEDIT_END = WXMENU_ADBEDIT_UNDO,

   WXMENU_ADBFIND_BEGIN = WXMENU_ADBEDIT_END,
   WXMENU_ADBFIND_FIND,
   WXMENU_ADBFIND_NEXT,
   WXMENU_ADBFIND_SEP,
   WXMENU_ADBFIND_GOTO,
   WXMENU_ADBFIND_END = WXMENU_ADBFIND_GOTO,

   WXMENU_HELP_BEGIN = WXMENU_ADBFIND_END,
   WXMENU_HELP_ABOUT,
   WXMENU_HELP_TIP,
   WXMENU_HELP_RELEASE_NOTES,
   WXMENU_HELP_FAQ,
   WXMENU_HELP_SEP,
   WXMENU_HELP_CONTEXT,
   WXMENU_HELP_CONTENTS,
   WXMENU_HELP_SEARCH,
   WXMENU_HELP_COPYRIGHT,
   WXMENU_HELP_END = WXMENU_HELP_COPYRIGHT,

   WXMENU_END,

   WXMENU_MIME_BEGIN = WXMENU_END,
   WXMENU_MIME_INFO,
   WXMENU_MIME_OPEN,
   WXMENU_MIME_OPEN_WITH,
   WXMENU_MIME_SAVE,
   WXMENU_MIME_VIEW_AS_TEXT,
   WXMENU_MIME_DISMISS,
   WXMENU_MIME_END,

   WXMENU_URL_BEGIN = WXMENU_MIME_END,
   WXMENU_URL_OPEN,
   WXMENU_URL_OPEN_NEW,
   WXMENU_URL_COMPOSE,
   WXMENU_URL_REPLYTO,
   WXMENU_URL_FORWARDTO,
   WXMENU_URL_ADD_TO_ADB,
   WXMENU_URL_COPY,
   WXMENU_URL_END,

   WXMENU_MODULES,

   // mouse events on embedded objects in wxLayoutWindow
   WXMENU_LAYOUT_BEGIN,
   WXMENU_LAYOUT_LCLICK,            // left click
   WXMENU_LAYOUT_RCLICK,            // right click
   WXMENU_LAYOUT_DBLCLICK,          // left button only
   WXMENU_LAYOUT_END,
   WXMENU_POPUP_MIME_OFFS = WXMENU_LAYOUT_END,
   WXMENU_POPUP_MODULES_OFFS = WXMENU_POPUP_MIME_OFFS + 100,
   WXMENU_POPUP_FOLDER_MENU = WXMENU_POPUP_MIME_OFFS + 100,

   // pseudo messages used to communicate with MsgCmdProc
   WXMENU_MSG_DROP_TO_FOLDER = 5000,
   WXMENU_MSG_DRAG,

   // The following entries are reserved for use by plugin modules
   WXMENU_MODULES_BEGIN = 10000,

   WXMENU_MODULES_PALMOS_BEGIN = WXMENU_MODULES_BEGIN,
   WXMENU_MODULES_PALMOS_SYNC = WXMENU_MODULES_PALMOS_BEGIN,
   WXMENU_MODULES_PALMOS_BACKUP,
   WXMENU_MODULES_PALMOS_RESTORE,
   WXMENU_MODULES_PALMOS_INSTALL,
   WXMENU_MODULES_PALMOS_CONFIG,
   WXMENU_MODULES_PALMOS_END = WXMENU_MODULES_PALMOS_BEGIN+20,

   WXMENU_MODULES_CALENDAR_BEGIN = WXMENU_MODULES_PALMOS_END,
   WXMENU_MODULES_CALENDAR_SHOW = WXMENU_MODULES_CALENDAR_BEGIN,
   WXMENU_MODULES_CALENDAR_CONFIG,
   WXMENU_MODULES_CALENDAR_ADD_REMINDER,
   WXMENU_MODULES_CALENDAR_END = WXMENU_MODULES_CALENDAR_BEGIN+20,

   WXMENU_MODULES_MIGRATE_BEGIN = WXMENU_MODULES_CALENDAR_END,
   WXMENU_MODULES_MIGRATE_DO,
   WXMENU_MODULES_MIGRATE_END,

   WXMENU_VIEW_FILTERS_BEGIN = WXMENU_MODULES_MIGRATE_END,
   WXMENU_VIEW_FILTERS_END = WXMENU_VIEW_FILTERS_BEGIN + 30,

   WXMENU_VIEW_VIEWERS_BEGIN = WXMENU_VIEW_FILTERS_END,
   WXMENU_VIEW_VIEWERS_END = WXMENU_VIEW_VIEWERS_BEGIN + 30,

   WXMENU_MODULES_END,

   WXMENU_DEBUG_BEGIN = WXMENU_MODULES_END,
   WXMENU_DEBUG_END = WXMENU_MODULES_END + 20
};

/**
  Almost the same thing for the toolbars, except that as some toolbar buttons
  are common to more than one frame we have an extra level of indirection but
  we don't need _BEGIN/_END stuff
 */
enum
{
   WXTBAR_SEP,

   WXTBAR_CLOSE,
   WXTBAR_ADB,
   WXTBAR_PREFERENCES,

   WXTBAR_MAIN_COLLECT,
   WXTBAR_MAIN_OPEN,
   WXTBAR_MAIN_COMPOSE,
   WXTBAR_MAIN_HELP,
   WXTBAR_MAIN_EXIT,

   WXTBAR_COMPOSE_PRINT,
   WXTBAR_COMPOSE_CLEAR,
   WXTBAR_COMPOSE_INSERT,
   WXTBAR_COMPOSE_EXTEDIT,
   WXTBAR_COMPOSE_SEND,

   WXTBAR_MSG_NEXT_UNREAD,
   WXTBAR_MSG_OPEN,
   WXTBAR_MSG_REPLY,
   WXTBAR_MSG_REPLYALL,
   WXTBAR_MSG_FORWARD,
   WXTBAR_MSG_PRINT,
   WXTBAR_MSG_DELETE,

   WXTBAR_ADB_NEW,
   WXTBAR_ADB_DELETE,
   WXTBAR_ADB_UNDO,
   WXTBAR_ADB_FINDNEXT,

   WXTBAR_MAX
};

// ----------------------------------------------------------------------------
// toolbar functions
// ----------------------------------------------------------------------------

/// all frames which have toolbars
enum wxFrameId
{
   WXFRAME_MAIN,
   WXFRAME_COMPOSE,
   WXFRAME_FOLDER,
   WXFRAME_MESSAGE,
   WXFRAME_ADB,
   WXFRAME_MAX
};

// tbar styles: note that these values shouldn't be changed, they correspond to
// the radiobox selections in the options dialog!
enum
{
   TbarShow_Icons = 1,
   TbarShow_Text = 2,
   TbarShow_Both = TbarShow_Icons | TbarShow_Text
};

/// creates the toolbar for the given frame
extern void CreateMToolbar(wxFrame *parent, wxFrameId frameId);

/// enables or disables the given button
extern void EnableToolbarButton(wxToolBar *toolbar, int nButton, bool enable);

#endif // _GUI_WXMENUDEFS_H_

