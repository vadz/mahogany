///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   gui/wxFolderMenu.cpp - CreateFolderMenu() implementation
// Purpose:
// Author:      Vadim Zeitlin
// Modified by:
// Created:     23.04.00
// CVS-ID:      $Id$
// Copyright:   (c) 2000 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// TODO: cache the menu, only update it when folder create/delete events come

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
#  include "Mcommon.h"

#  include <wx/menu.h>

#  include "gui/wxMenuDefs.h"
#endif // USE_PCH

#include "MFolder.h"

#include "gui/wxFolderMenu.h"

// ----------------------------------------------------------------------------
// global data
// ----------------------------------------------------------------------------

// so far we support having only one folder menu - this is ok as we only use
// it in popup menus now and there is at most one of popup menus at a time.
//
// however, if we decide to change this, we can allow multiple menus without
// changing the public API.

static wxMenu *gs_menuFolders = NULL;
static wxArrayString gs_folderNames;

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// recursive helper for CreateFolderMenu
static void AddSubFoldersToMenu(wxString& folderName, MFolder *folder, wxMenu *menu, size_t& id)
{
   size_t nSubfolders = folder->GetSubfolderCount();
   for ( size_t n = 0; n < nSubfolders; n++ )
   {
      MFolder *subfolder = folder->GetSubfolder(n);
      if ( !subfolder )
      {
         FAIL_MSG( "no subfolder?" );

         continue;
      }

      wxString name = subfolder->GetName();
      wxString subfolderName = folderName;
      if ( !!subfolderName )
      {
         // there shouldn't be slash in the beginning
         subfolderName += '/';
      }
      subfolderName += name;

      if ( subfolder->GetSubfolderCount() == 0 )
      {
         // it's a simple menu item
         menu->Append(WXMENU_POPUP_FOLDER_MENU + id++, name);
      }
      else // subfolders has more subfolders, make it a submenu
      {
         wxMenu *submenu = new wxMenu;
         AddSubFoldersToMenu(subfolderName, subfolder, submenu, id);
         menu->Append(WXMENU_POPUP_FOLDER_MENU + id++, name, submenu);
      }

      // in any case, add it to the id->name map
      gs_folderNames.Add(subfolderName);

      subfolder->DecRef();
   }
}

// ----------------------------------------------------------------------------
// public API
// ----------------------------------------------------------------------------

wxMenu *wxFolderMenu::Create()
{
   ASSERT_MSG( !gs_menuFolders && gs_folderNames.IsEmpty(),
               "forgot to call wxFolderMenu::Delete()?" );

   MFolder *folder = MFolder::Get("");
   gs_menuFolders = new wxMenu;
   size_t id = 0;
   wxString folderName;
   AddSubFoldersToMenu(folderName, folder, gs_menuFolders, id);
   folder->DecRef();

   return gs_menuFolders;
}

MFolder *wxFolderMenu::GetFolder(wxMenu *menu, int id)
{
   CHECK( menu && menu == gs_menuFolders, NULL, "bad menu in wxFolderMenu::GetFolder" );
   ASSERT_MSG( id >= WXMENU_POPUP_FOLDER_MENU, "bad id in wxFolderMenu::GetFolder" );

   id -= WXMENU_POPUP_FOLDER_MENU;
   if ( id >= (int)gs_folderNames.GetCount() )
   {
      // don't assert - just not our menu item
      return NULL;
   }

   return MFolder::Get(gs_folderNames[(size_t)id]);
}

void wxFolderMenu::Delete(wxMenu *menu)
{
   ASSERT_MSG( menu == gs_menuFolders, "bad parameter in wxFolderMenu::Delete()" );

   delete gs_menuFolders;
   gs_menuFolders = NULL;

   gs_folderNames.Empty();
}
