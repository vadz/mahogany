///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   adb/AdbDialogs.h - assorted ADB-related dialogs
// Purpose:     dialogs to import an ADB, select ADB entry expansion, ...
// Author:      Vadim Zeitlin
// Modified by:
// Created:     08.07.99
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _ADB_DIALOGS_H
#define _ADB_DIALOGS_H

class wxWindow;

// ----------------------------------------------------------------------------
// show dialog allowing the user to import any address book, return the name of
// the native book used for import in the out parameter and return TRUE if the
// import succeeded - FALSE and log the error message(s) if it failed.
// ----------------------------------------------------------------------------

extern bool AdbShowImportDialog(wxWindow *parent = NULL,
                                String *nameOfNativeAdb = NULL);

#endif // _ADB_DIALOGS_H
