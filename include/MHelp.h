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
   MH_FOLDER_VIEW = 2002,
   MH_MESSAGE_VIEW = 2003,
   MH_FOLDER_OPEN_DIALOG = 2004,
   MH_FOLDER_CREATE_DIALOG = 2005,
   MH_FOLDER_VIEW_KEYBINDINGS = MH_FOLDER_VIEW,
   MH_FOLDER_TREE = 2006,
   MH_COMPOSE_MAIL = 2100,
   MH_COMPOSE_NEWS = 2101,
   MH_ADB = 3000,
   MH_ADB_ADB = 3001,
   MH_ADB_BBDB = 3002,
   MH_ADB_USING = 3100,
   MH_OPAGE_COMPOSE     = 4100,
   MH_OPAGE_MESSAGEVIEW = 4101,
   MH_OPAGE_IDENT       = 4102,
   MH_OPAGE_PYTHON      = 4103,
   MH_OPAGE_ADB         = 4104,
   MH_OPAGE_OTHERS      = 4105,
   MH_OPAGE_HELPERS     = 4106,
   MH_OPAGE_FOLDERS     = 4107,
   MH_OPTIONSNOTEBOOK   = 4120,
   
   MH_PYTHON = 5000
};

#endif
