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
   MH_RELEASE_NOTES,
   MH_FAQ,
   /// help about certain controls and dialogs
   MH_CONTROLS = 2000,
   MH_MAIN_WINDOW = 2001,
   MH_FOLDER_VIEW = 2002,
   MH_MESSAGE_VIEW = 2003,
   MH_FOLDER_OPEN_DIALOG = 2004,
   /// help IDs that are hard-coded (i.e. in resource files)
   MH_FIXED = 30000
};

#endif
