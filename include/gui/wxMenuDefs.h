/*-*- c++ -*-********************************************************
 * wxMenuDefs.h : define numeric ids and menu names                 *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/
#ifndef	WXMENUDEFS_H
#define WXMENUDEFS_H

/// check wether an ID belongs to a given menu
#define   WXMENU_CONTAINS(menu,id)   (WXMENU_##menu##_BEGIN < id && WXMENU_##menu##_END > id)

/** Definition of all numeric menu IDs.
    Include each menu in WXMENU_menuname_BEGIN and
    WXMENU_menuname_END, so it can be tested for by
    WXMENU_CONTAIN(menu,id).
*/
enum
{
   WXMENU_LAYOUT_CLICK = 1,
   WXMENU_FILE_BEGIN,
   WXMENU_FILE_OPEN,
   WXMENU_FILE_COMPOSE,
   WXMENU_FILE_TEST,
   WXMENU_FILE_CLOSE,
   WXMENU_FILE_EXIT,
   WXMENU_FILE_END,
   WXMENU_EDIT_BEGIN,
   WXMENU_EDIT_ADB,
   WXMENU_EDIT_PREFERENCES,
   WXMENU_EDIT_SAVE_PREFERENCES,
   WXMENU_EDIT_END,
   WXMENU_MSG_BEGIN,
   WXMENU_MSG_PRINT,
   WXMENU_MSG_DELETE,
   WXMENU_MSG_UNDELETE,
   WXMENU_MSG_SAVE_TO_FILE,
   WXMENU_MSG_SAVE_TO_FOLDER,
   WXMENU_MSG_OPEN,
   WXMENU_MSG_REPLY,
   WXMENU_MSG_FORWARD,
   WXMENU_MSG_SELECTALL,
   WXMENU_MSG_DESELECTALL,
   WXMENU_MSG_EXPUNGE,
   WXMENU_MSG_END,
   WXMENU_COMPOSE_BEGIN,
   WXMENU_COMPOSE_INSERTFILE,
   WXMENU_COMPOSE_SEND,
   WXMENU_COMPOSE_PRINT,
   WXMENU_COMPOSE_CLEAR,
   WXMENU_COMPOSE_END,
   WXMENU_MIME_BEGIN,
   WXMENU_MIME_HANDLE,
   WXMENU_MIME_INFO,
   WXMENU_MIME_SAVE,
   WXMENU_MIME_DISMISS,
   WXMENU_MIME_END,
   WXMENU_HELP_BEGIN,
   WXMENU_HELP_ABOUT,
   WXMENU_HELP_END,
   WXMENU_POPUP_MIME_OFFS = 100
};

#endif
