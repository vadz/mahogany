/*-*- c++ -*-********************************************************
 * MHelp.h : M's help system, based on numeric help topic ids       *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *******************************************************************/
#ifndef	MHELP_H
#define MHELP_H

/** Definition of all numeric help IDs.
*/
enum
{
   MH_CONTENTS = 0,
   MH_ABOUT,
   MH_SEARCH,
   /// general help topics
   MH_TOPICS = 1000,
   MH_RELEASE_NOTES = 1001,
   MH_FAQ = 1002,
   MH_COPYRIGHT = 1003,
   /// help about certain controls and dialogs
   MH_CONTROLS = 2000,
   MH_MAIN_FRAME = 2001,
   MH_FOLDER_VIEW = 2001,
   MH_MESSAGE_VIEW = 2003,
   MH_FOLDER_OPEN_DIALOG = 2004,
   MH_FOLDER_CREATE_DIALOG = 2005,
   MH_FOLDER_VIEW_KEYBINDINGS = MH_FOLDER_VIEW,
   MH_ADB = 3000,
   MH_ADB_ADB = 3001,
   MH_ADB_BBDB = 3002,
   MH_ADB_USING = 3100,
   MH_PYTHON = 5000
};

#endif
