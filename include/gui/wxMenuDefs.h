/*-*- c++ -*-********************************************************
 * wxMenuDefs.h : define numeric ids and menu names                 *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.2  1998/05/06 17:39:25  VZ
 * changed the menu ids to start from 1 (workaround for wxGTK where control
 * id of 0 is still special and matches any id)
 *
 * Revision 1.1  1998/03/14 12:21:15  karsten
 * first try at a complete archive
 *
 *******************************************************************/
#ifndef	WXMENUDEFS_H
#define WXMENUDEFS_H

enum
{
   WXMENU_FILE_OPEN = 1,
   WXMENU_FILE_COMPOSE,
   WXMENU_FILE_TEST,
   WXMENU_FILE_CLOSE,
   WXMENU_FILE_EXIT,
   WXMENU_FILE_ADBEDIT,
   WXMENU_MSG_PRINT,
   WXMENU_MSG_DELETE,
   WXMENU_MSG_SAVE,
   WXMENU_MSG_OPEN,
   WXMENU_MSG_REPLY,
   WXMENU_MSG_FORWARD,
   WXMENU_MSG_SELECTALL,
   WXMENU_MSG_DESELECTALL,
   WXMENU_MSG_EXPUNGE,
   WXMENU_COMPOSE_INSERTFILE,
   WXMENU_COMPOSE_SEND,
   WXMENU_COMPOSE_PRINT,
   WXMENU_COMPOSE_CLEAR,
   WXMENU_MIME_HANDLE,
   WXMENU_MIME_INFO,
   WXMENU_MIME_SAVE,
   WXMENU_HELP_ABOUT,
   WXMENU_POPUP_MIME_OFFS = 100
};

#endif
