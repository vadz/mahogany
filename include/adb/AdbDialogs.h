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

class WXDLLEXPORT wxWindow;
class WXDLLEXPORT wxFrame;

class AdbEntryGroup;
class ArrayAdbElements;
class ArrayAdbEntries;

/**
  Show the dialog allowing the user to import any address book, return the name
  of the native book used for import in the out parameter and return TRUE if
  the import succeeded - FALSE and log the error message(s) if it failed.
 */
extern bool AdbShowImportDialog(wxWindow *parent = NULL,
                                String *nameOfNativeAdb = NULL);

/**
  Show the dialog allowing the user to export the given address book

  @return TRUE on success
 */
extern bool AdbShowExportDialog(AdbEntryGroup& group);

/**
  Show the ADB expansion dialog allowing the user to choose one of the entries
  or groups matched by the expansion.

  @param aEverything the array of groups and entries to show initially
  @param aMoreEntries the array of entries to show if the user chooses
                      to see all possible matches
  @param nGroups first nGroups elements of aEverything are groups, not entries
  @param parent the parent frame
  @return the index in the concatenation of aEverything and aMoreEntries arrays
          or -1 if the dialog was cancelled
 */
extern int AdbShowExpandDialog(ArrayAdbElements& aEverything,
                               ArrayAdbEntries& aMoreEntries,
                               size_t nGroups,
                               wxFrame *parent);

#endif // _ADB_DIALOGS_H
