///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   include/Mpers.h
// Purpose:     persistent controls names used by M
// Author:      Vadim Zeitlin
// Modified by:
// Created:     16.01.00
// CVS-ID:      $Id$
// Copyright:   (c) 2000 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _MPERS_H
#define _MPERS_H

// the idea is to associate a description to each control name so that
// something sensible could be shown to the user in
// ReenablePersistentMessageBoxes() function

enum MPersMsgBox
{
   M_MSGBOX_ASK_SPECIFY_DIR,
#ifdef OS_UNIX
   M_MSGBOX_ASK_RUNASROOT,
#endif   
   M_MSGBOX_SEND_OUTBOX_ON_EXIT,
   M_MSGBOX_ABANDON_CRITICAL,
   M_MSGBOX_GO_ONLINE_TO_SEND_OUTBOX,
   M_MSGBOX_FIX_TEMPLATE,
   M_MSGBOX_ASK_FOR_SIG,
   M_MSGBOX_UNSAVED_PROMPT,
   M_MSGBOX_MESSAGE_SENT,
   M_MSGBOX_ASK_FOR_EXT_EDIT,
   M_MSGBOX_MIME_TYPE_CORRECT,
   M_MSGBOX_CONFIG_NET_FROM_COMPOSE,
   M_MSGBOX_SEND_EMPTY_SUBJECT,
   M_MSGBOX_CONFIRM_FOLDER_DELETE,
   M_MSGBOX_MARK_READ,
   M_MSGBOX_DIALUP_CONNECTED_MSG,
   M_MSGBOX_DIALUP_DISCONNECTED_MSG,
   M_MSGBOX_CONFIRM_EXIT,
   M_MSGBOX_ASK_LOGIN,
   M_MSGBOX_ASK_PWD,
   M_MSGBOX_GO_OFFLINE_SEND_FIRST,
   M_MSGBOX_OPEN_UNACCESSIBLE_FOLDER,
   M_MSGBOX_CHANGE_UNACCESSIBLE_FOLDER_SETTINGS,
   M_MSGBOX_ASK_URL_BROWSER,
   M_MSGBOX_OPT_TEST_ASK,
   M_MSGBOX_WARN_RESTART_OPT,
   M_MSGBOX_SAVE_TEMLPATE,
   M_MSGBOX_DIALUP_ON_OPEN_FOLDER,
   M_MSGBOX_NET_DOWN_OPEN_ANYWAY,
   M_MSGBOX_NO_NET_PING_ANYWAY,
   M_MSGBOX_MAIL_NO_NET_QUEUED_MESSAGE,
   M_MSGBOX_MAIL_QUEUED_MESSAGE,
   M_MSGBOX_MAIL_SENT_MESSAGE,
   M_MSGBOX_TEST_MAIL_SENT,
   M_MSGBOX_ADB_DELETE_ENTRY,
   M_MSGBOX_CONFIRM_ADB_IMPORTER,
   M_MSGBOX_BBDB_SAVE_DIALOG,
   M_MSGBOX_FOLDER_GROUP_HINT,
   M_MSGBOX_SIGNATURE_LENGTH,
   M_MSGBOX_MODULES_WARNING,
   M_MSGBOX_REMEMBER_PWD,
   M_MSGBOX_SHOWLOGWINHINT,
   M_MSGBOX_AUTOEXPUNGE,
   M_MSGBOX_SUSPENDAUTOCOLLECT,
   M_MSGBOX_RULESMISMATCHWARN,
   M_MSGBOX_MAX
};

/// return the name to use for the given persistent msg box
extern String GetPersMsgBoxName(MPersMsgBox which);

#endif // _MPERS_H

