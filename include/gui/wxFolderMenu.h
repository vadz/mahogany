///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   gui/wxFolderMenu.h - a specialized tree control for folders
// Purpose:     a function to create a menu containing all the folder, used in
//              popup menus for "Move to folder" &c
// Author:      Vadim Zeitlin
// Modified by:
// Created:     23.04.00
// CVS-ID:      $Id$
// Copyright:   (c) 2000 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _M_GUI_FOLDERMENU_H_
#define _M_GUI_FOLDERMENU_H_

class WXDLLEXPORT wxMenu;
class MFolder;

/// there are no objects of this class, it is used only as namespace
class wxFolderMenu
{
public:
   /// returns a menu containing all folders (except the root one)
   static wxMenu *Create();

   /// get an MFolder for the given menu id (caller must DecRef() it)
   static MFolder *GetFolder(wxMenu *menu, int id);

   /// call this immediately after youfinished using the menu
   static void Delete(wxMenu *menu);

private:
   // never used
   wxFolderMenu();
   ~wxFolderMenu();

   GCC_DTOR_WARN_OFF
};

#endif // _M_GUI_FOLDERMENU_H_
