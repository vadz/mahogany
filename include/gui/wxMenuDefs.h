// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //////
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
// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //////

#ifndef  WXMENUDEFS_H
#define  WXMENUDEFS_H

class wxMenu;
class wxToolBar;

/// append an item to menu
extern void AppendToMenu(wxMenu *menu, int n);

/// appends all items in the given range (see enum below) to the menu
extern void AppendToMenu(wxMenu *menu, int nFirst, int nLast);

/// check wether an ID belongs to a given menu
#define  WXMENU_CONTAINS(menu,id)   \
   (WXMENU_##menu##_BEGIN < (id) && WXMENU_##menu##_END >= (id))

/// creates a menu, appends all it's items to it and appends it to menu bar
#define  WXADD_MENU(menubar, menu, caption)                                \
   {                                                                       \
      wxMenu *pMenu = new wxMenu;                                          \
      AppendToMenu(pMenu, WXMENU_##menu##_BEGIN + 1, WXMENU_##menu##_END); \
      menubar->Append(pMenu, caption);                                     \
   }

/** Definition of all numeric menu IDs.
    Include each menu in WXMENU_menuname_BEGIN and WXMENU_menuname_END, so it
    can be tested for by WXMENU_CONTAIN(menu,id).

    There should be no gaps in this enum (AppendToMenu assumes it) and the
    value of each _BEGIN must be the same as the value of the preceding _END
*/
enum
{
   WXMENU_SEPARATOR = -1,

   WXMENU_FILE_BEGIN = 0,
   WXMENU_FILE_OPEN,
   WXMENU_FILE_COMPOSE,
   WXMENU_FILE_POST,
   WXMENU_FILE_SEP,
   WXMENU_FILE_PRINT_SETUP,
   WXMENU_FILE_PAGE_SETUP,
#ifdef USE_PS_PRINTING
   // extra postscript printing
   WXMENU_FILE_PRINT_SETUP_PS,
//   WXMENU_FILE_PAGE_SETUP_PS,
#endif
   WXMENU_FILE_SEP2,
   WXMENU_FILE_CREATE,
#ifdef USE_PYTHON
   WXMENU_FILE_SEP3,
   WXMENU_FILE_SCRIPT,
#endif // USE_PYTHON

   WXMENU_FILE_SEP4,
   WXMENU_FILE_CLOSE,
   WXMENU_FILE_SEP5,
   WXMENU_FILE_EXIT,
   WXMENU_FILE_END = WXMENU_FILE_EXIT,

   WXMENU_EDIT_BEGIN = WXMENU_FILE_END,
   WXMENU_EDIT_ADB,
   WXMENU_EDIT_PREF,
   WXMENU_EDIT_SEP1,
   WXMENU_EDIT_RESTORE_PREF,
   WXMENU_EDIT_SEP2,
   WXMENU_EDIT_SAVE_PREF,
   WXMENU_EDIT_END = WXMENU_EDIT_SAVE_PREF,

   WXMENU_MSG_EDIT_BEGIN = WXMENU_EDIT_END,
   WXMENU_MSG_EDIT_CUT,
   WXMENU_MSG_EDIT_COPY,
   WXMENU_MSG_EDIT_PASTE,
   WXMENU_MSG_EDIT_SEP0,
   WXMENU_MSG_EDIT_ADB,
   WXMENU_MSG_EDIT_PREF,
   WXMENU_MSG_EDIT_SEP1,
   WXMENU_MSG_EDIT_RESTORE_PREF,
   WXMENU_MSG_EDIT_SEP2,
   WXMENU_MSG_EDIT_SAVE_PREF,
   WXMENU_MSG_EDIT_END = WXMENU_MSG_EDIT_SAVE_PREF,

   WXMENU_MSG_BEGIN = WXMENU_MSG_EDIT_END,
   WXMENU_MSG_OPEN,
   WXMENU_MSG_PRINT,
   WXMENU_MSG_PRINT_PREVIEW,
#ifdef USE_PS_PRINTING
   // extra postscript printing
   WXMENU_MSG_PRINT_PS,
   WXMENU_MSG_PRINT_PREVIEW_PS,
#endif
   WXMENU_MSG_REPLY,
   WXMENU_MSG_FORWARD,
   WXMENU_MSG_SEP1,
   WXMENU_MSG_SAVE_TO_FILE,
   WXMENU_MSG_SAVE_TO_FOLDER,
   WXMENU_MSG_MOVE_TO_FOLDER,
   WXMENU_MSG_DELETE,
   WXMENU_MSG_UNDELETE,
   WXMENU_MSG_EXPUNGE,
   WXMENU_MSG_SEP2,
   WXMENU_MSG_SELECTALL,
   WXMENU_MSG_DESELECTALL,
   WXMENU_MSG_SEP3,
   WXMENU_MSG_TOGGLEHEADERS,
   WXMENU_MSG_SHOWRAWTEXT,
   WXMENU_MSG_END = WXMENU_MSG_SHOWRAWTEXT,

   WXMENU_COMPOSE_BEGIN = WXMENU_MSG_END,
   WXMENU_COMPOSE_INSERTFILE,
   WXMENU_COMPOSE_SEND,
   WXMENU_COMPOSE_PRINT,
   WXMENU_COMPOSE_SEP1,
   WXMENU_COMPOSE_LOADTEXT,
   WXMENU_COMPOSE_SAVETEXT,
   WXMENU_COMPOSE_CLEAR,
   WXMENU_COMPOSE_SEP2,
   WXMENU_COMPOSE_EXTEDIT,
   WXMENU_COMPOSE_END = WXMENU_COMPOSE_EXTEDIT,

   WXMENU_ADBBOOK_BEGIN = WXMENU_COMPOSE_END,
   WXMENU_ADBBOOK_NEW,
   WXMENU_ADBBOOK_OPEN,
   WXMENU_ADBBOOK_SEP,
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
   WXMENU_HELP_RELEASE_NOTES,
   WXMENU_HELP_FAQ,
   WXMENU_HELP_SEP,
   WXMENU_HELP_CONTEXT,
   WXMENU_HELP_CONTENTS,
   WXMENU_HELP_SEARCH,
   WXMENU_HELP_COPYRIGHT,
   WXMENU_HELP_END = WXMENU_HELP_COPYRIGHT,

   WXMENU_END = WXMENU_HELP_END,

   WXMENU_MIME_BEGIN = WXMENU_END,
   WXMENU_MIME_HANDLE,
   WXMENU_MIME_INFO,
   WXMENU_MIME_SAVE,
   WXMENU_MIME_DISMISS,
   WXMENU_MIME_END,

   // mouse events on embedded objects in wxLayoutWindow
   WXMENU_LAYOUT_BEGIN = WXMENU_MIME_END,
   WXMENU_LAYOUT_LCLICK,            // left click
   WXMENU_LAYOUT_RCLICK,            // right click
   WXMENU_LAYOUT_DBLCLICK,          // left button only
   WXMENU_LAYOUT_END,

   WXMENU_POPUP_MIME_OFFS = 1000
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

   WXTBAR_MAIN_OPEN,
   WXTBAR_MAIN_COMPOSE,
   WXTBAR_MAIN_HELP,
   WXTBAR_MAIN_EXIT,

   WXTBAR_COMPOSE_PRINT,
   WXTBAR_COMPOSE_CLEAR,
   WXTBAR_COMPOSE_INSERT,
   WXTBAR_COMPOSE_EXTEDIT,
   WXTBAR_COMPOSE_SEND,

   WXTBAR_MSG_OPEN,
   WXTBAR_MSG_FORWARD,
   WXTBAR_MSG_REPLY,
   WXTBAR_MSG_PRINT,
   WXTBAR_MSG_DELETE,

   WXTBAR_ADB_NEW,
   WXTBAR_ADB_DELETE,
   WXTBAR_ADB_UNDO,
   WXTBAR_ADB_FINDNEXT,

   WXTBAR_MAX
};

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

// not only it adds the icons, but also calls CreateTools() or
// Realize()/Layout() hiding MSW/GTK differences
extern void AddToolbarButtons(wxToolBar *ToolBar, wxFrameId frameId);

#endif
