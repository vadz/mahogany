/*-*- c++ -*-********************************************************
 * wxMenuDefs.h : define numeric ids and menu names                 *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/
#ifndef	WXMENUDEFS_H
#define  WXMENUDEFS_H

/// append an item to menu
extern void AppendToMenu(wxMenu *menu, int n);

/// appends all items in the given range (see enum below) to the menu
extern void AppendToMenu(wxMenu *menu, int nFirst, int nLast);

/// check wether an ID belongs to a given menu
#define  WXMENU_CONTAINS(menu,id)   \
   (WXMENU_##menu##_BEGIN < (id) && WXMENU_##menu##_END > (id))

/// creates a menu, appends all it's items to it and appends it to menu bar
#define  WXADD_MENU(menubar, menu, caption)                             \
   {                                                                       \
      wxMenu *pMenu = new wxMenu;                                          \
      AppendToMenu(pMenu, WXMENU_##menu##_BEGIN + 1, WXMENU_##menu##_END); \
      menubar->Append(pMenu, wxGetTranslation(caption));                   \
   }

/** Definition of all numeric menu IDs.
    Include each menu in WXMENU_menuname_BEGIN and WXMENU_menuname_END, so it
    can be tested for by WXMENU_CONTAIN(menu,id).

    There should be no gaps in this enum (AppendToMenu assumes it) and the
    value of each _BEGIN must be the same as the value of the preceding _END
*/
enum
{
   WXMENU_LAYOUT_CLICK,

   WXMENU_FILE_BEGIN = WXMENU_LAYOUT_CLICK,
   WXMENU_FILE_OPEN,
   WXMENU_FILE_COMPOSE,
#ifdef DEBUG
   WXMENU_FILE_TEST,
#endif
   WXMENU_FILE_CLOSE,
   WXMENU_FILE_SEP,
   WXMENU_FILE_EXIT,
   WXMENU_FILE_END,

   WXMENU_EDIT_BEGIN = WXMENU_FILE_END,
   WXMENU_EDIT_ADB,
   WXMENU_EDIT_PREF,
   WXMENU_EDIT_SEP,
   WXMENU_EDIT_SAVE_PREF,
   WXMENU_EDIT_END,

   WXMENU_MSG_BEGIN = WXMENU_EDIT_END,
   WXMENU_MSG_OPEN,
   WXMENU_MSG_PRINT,
   WXMENU_MSG_REPLY,
   WXMENU_MSG_FORWARD,
   WXMENU_MSG_SEP1,
   WXMENU_MSG_SAVE_TO_FILE,
   WXMENU_MSG_SAVE_TO_FOLDER,
   WXMENU_MSG_DELETE,
   WXMENU_MSG_UNDELETE,
   WXMENU_MSG_EXPUNGE,
   WXMENU_MSG_SEP2,
   WXMENU_MSG_SELECTALL,
   WXMENU_MSG_DESELECTALL,
   WXMENU_MSG_END,

   WXMENU_COMPOSE_BEGIN = WXMENU_MSG_END,
   WXMENU_COMPOSE_INSERTFILE,
   WXMENU_COMPOSE_SEND,
   WXMENU_COMPOSE_PRINT,
   WXMENU_COMPOSE_SEP,
   WXMENU_COMPOSE_CLEAR,
   WXMENU_COMPOSE_END,

   WXMENU_HELP_BEGIN = WXMENU_COMPOSE_END,
   WXMENU_HELP_ABOUT,
   WXMENU_HELP_END,

   WXMENU_END = WXMENU_HELP_END,

   WXMENU_MIME_BEGIN = WXMENU_END,
   WXMENU_MIME_HANDLE,
   WXMENU_MIME_INFO,
   WXMENU_MIME_SAVE,
   WXMENU_MIME_DISMISS,
   WXMENU_MIME_END,

   WXMENU_POPUP_MIME_OFFS = 100
};

#endif
