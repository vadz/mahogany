/*-*- c++ -*-********************************************************
 * MHelp.h : M's help system, based on numeric help topic ids       *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *******************************************************************/
#ifndef MHELP_H
#define MHELP_H

// NB: we use #define here because this file is sourced by MS Help compiler
//     which doesn't understand things like 'enum' or 'const int'. Also, all
//     ids *must* start with MH_ prefix for the same reason.

/// Definition of all numeric help IDs
#define MH_CONTENTS 0
#define MH_ABOUT    1
#define MH_SEARCH   2

/// general help topics
#define MH_TOPICS        1000
#define MH_RELEASE_NOTES 1001
#define MH_FAQ           1002
#define MH_COPYRIGHT     1003

/// help about certain controls and dialogs
#define MH_CONTROLS           2000
#define MH_MAIN_FRAME         2001
#define MH_FOLDER_VIEW        2002
#define MH_MESSAGE_VIEW       2003
#define MH_FOLDER_OPEN_DIALOG 2004
#define MH_FOLDER_CREATE_DIALOG 2005
#define MH_FOLDER_VIEW_KEYBINDINGS 2002 // MH_FOLDER_VIEW
#define MH_FOLDER_TREE       2006
#define MH_COMPOSE_MAIL      2100
#define MH_COMPOSE_NEWS      2101
#define MH_ADB               3000
#define MH_ADB_ADB           3001
#define MH_ADB_BBDB          3002
#define MH_ADB_USING         3100
#define MH_ADB_FRAME         3200
#define MH_OPAGE_COMPOSE     4100
#define MH_OPAGE_MESSAGEVIEW 4101
#define MH_OPAGE_IDENT       4102
#define MH_OPAGE_PYTHON      4103
#define MH_OPAGE_ADB         4104
#define MH_OPAGE_OTHERS      4105
#define MH_OPAGE_HELPERS     4106
#define MH_OPAGE_FOLDERS     4107
#define MH_OPAGE_NETWORK     4108
#define MH_OPAGE_SYNC        4109
#define MH_OPAGE_NEWMAIL     4110
#define MH_OPTIONSNOTEBOOK   4120
#define MH_DIALOG_MODULES    4200
#define MH_DIALOG_SORTING    4210
#define MH_DIALOG_DATEFMT    4220
#define MH_DIALOG_FOLDERDLG  4230
#define MH_DIALOG_XFACE      4240
#define MH_DIALOG_SEARCHMSGS 4250
#define MH_DIALOG_FILTERS    4260
#define MH_DIALOG_FILTERS_DETAILS    4261
#define MH_DIALOG_FOLDER_FILTERS    4262
#define MH_DIALOG_QUICK_FILTERS    4263
#define MH_DIALOG_LICENSE    4270
#define MH_DIALOG_GLOBALPASSWD 4280
#define MH_PYTHON            5000

/** All modules related help should be from this value on and be
    reported to the main authors for inclusion in the docs. */
#define MH_MODULES          10000
/// PalmOS connectivity module help range: (10-19)
#define MH_MODULES_PALMOS          10010
#define MH_MODULES_PALMOS_CONFIG   10011
/// Calendar module
#define MH_MODULES_CALENDAR          10020
#define MH_MODULES_CALENDAR_CONFIG   10021
#define MH_MODULES_CALENDAR_DATEDLG  10022
#endif // MHELP_H
