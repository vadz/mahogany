// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //////
// Project:     M
// File name:   MFolderDialogs.h - functions to invoke dialogs dealing with
//              folder management, such as folder creation, changing folder
//              properties &c
// Purpose:
// Author:      Vadim Zeitlin
// Modified by:
// Created:     23.12.98
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
// // //// //// //// //// //// //// //// //// //// //// //// //// //// //// ///

#ifndef _MFOLDERDIALOGS_H
#define _MFOLDERDIALOGS_H

// -----------------------------------------------------------------------------
// forward declarations
// -----------------------------------------------------------------------------
class wxWindow;
class MFolder;

// -----------------------------------------------------------------------------
// constants
// -----------------------------------------------------------------------------

// which page of the folder creation dialog to show first?
enum FolderCreatePage
{
   FolderCreatePage_Default = -1, // default must have value -1
   FolderCreatePage_Folder,
   FolderCreatePage_Compose,
   FolderCreatePage_MsgView,
   FolderCreatePage_FolderView,
   FolderCreatePage_Max
};

// -----------------------------------------------------------------------------
// macros
// -----------------------------------------------------------------------------

// get the notebook page by index and cast it to the right type
#define GET_FOLDER_PAGE(nbook)   ((wxFolderPropertiesPage *)nbook->GetPage(FolderCreatePage_Folder))
#define GET_OPTIONS_PAGE(nbook, n)  ((wxOptionsPage *)nbook->GetPage(n))

// -----------------------------------------------------------------------------
// functions
// -----------------------------------------------------------------------------

/**
  shows folder creation dialog, returns a pointer to created folder or NULL.

  @param if parentFolder is NULL, any parent folder may be chosen, otherwise
         the new folder can only be created under parentFolder.

  @return the returned folder object must be DecRef()d by the caller (if !NULL)
*/
extern MFolder *ShowFolderCreateDialog
                (
                  wxWindow *parent = NULL,
                  FolderCreatePage page = FolderCreatePage_Default,
                  MFolder *parentFolder = NULL
                );

/**
  shows folder properties for the given folder and allows changing some of
  them, returns TRUE if anything was changed, FALSE otherwise
*/

extern bool ShowFolderPropertiesDialog(MFolder *folder,
                                       wxWindow *parent = NULL);

/**
  allows the user to choose a folder from all existing ones, returns the
  pointer to the folder chosen if the user closes the dialog with [Ok] or
  NULL if it was closed with [Cancel] button.

  @return the returned folder object must be DecRef()d by the caller (if !NULL)
*/
extern MFolder *ShowFolderSelectionDialog(MFolder *folder,
                                          wxWindow *parent = NULL);

/**
  shows all existing subfolders (not in the program, but on the server) of the
  given folder and allows the user to browse them and create them.

  @return TRUE if any folders were created, FALSE otherwise
*/
extern bool ShowFolderSubfoldersDialog(MFolder *folder,
                                       wxWindow *parent = NULL);

#endif // _MFOLDERDIALOGS_H
