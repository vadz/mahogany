///////////////////////////////////////////////////////////////////////////////
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
///////////////////////////////////////////////////////////////////////////////

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
   FolderCreatePage_Compose,
   FolderCreatePage_Folder,
   FolderCreatePage_Max
};

// -----------------------------------------------------------------------------
// functions
// -----------------------------------------------------------------------------

/**
  shows folder creation dialog, returns a pointer to created folder or NULL.

  @param if parentFolder is NULL, any parent folder may be chosen, otherwise
         the new folder can only be created under parentFolder.
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
*/
extern MFolder *ShowFolderSelectionDialog(MFolder *folder,
                                          wxWindow *parent = NULL);

#endif // _MFOLDERDIALOGS_H
