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

class WXDLLIMPEXP_FWD_CORE wxMenu;
class MFolder;

class wxFolderMenu
{
public:
   // we're not ref counted (should we?), so use normal ctor/dtor
   wxFolderMenu() { m_data = NULL; }
   virtual ~wxFolderMenu();

   /// get the "real" menu: you MUST Remove() it or call Detach() later
   wxMenu *GetMenu() const;

   /// get an MFolder for the given menu id (caller must DecRef() it)
   MFolder *GetFolder(int id) const;

#ifdef __WXGTK__
   /// detach us from the real menu, shouldn't be needed but for wxGTK bug
   void Detach();
#endif // __WXGTK__

private:
   // implementation details
   class wxFolderMenuData *m_data;
};

#endif // _M_GUI_FOLDERMENU_H_
