///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   gui/wxFolderMenu.cpp - wxFolderMenu implementation
// Purpose:     we implement a wxMenu containing all folders in the folder tree
//              here - it is currently used only by wxFolderView in its popup
//              menu for "Quick move" but might be used elsewhere later
// Author:      Vadim Zeitlin
// Modified by:
// Created:     23.04.00
// CVS-ID:      $Id$
// Copyright:   (c) 2000 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

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
#endif // USE_PCH

#include "MEvent.h"
#include "MFolder.h"
#include "gui/wxMenuDefs.h"

#include "gui/wxFolderMenu.h"

// ----------------------------------------------------------------------------
// wxFolderMenu private data
// ----------------------------------------------------------------------------

class wxFolderMenuData : public MEventReceiver
{
public:
   wxFolderMenuData()
   {
      m_eventCookie = MEventManager::Register(*this, MEventId_FolderTreeChange);

      m_menu = NULL;
   }

   virtual ~wxFolderMenuData()
   {
      MEventManager::Deregister(m_eventCookie);

      ResetMenu();
   }

   // get the menu creating it if necessary
   wxMenu *GetMenu()
   {
      if ( !m_menu )
         CreateMenu();

      return m_menu;
   }

   // get the folder names we used to create the menu the last time
   const wxArrayString& GetFolderNames() const
   {
      // we must had created the menu first
      ASSERT_MSG(!m_folderNames.IsEmpty(), _T("wxFolderMenuData used incorrectly"));

      return m_folderNames;
   }

#ifdef __WXGTK__
   void Detach()
   {
      m_menu = NULL;
   }
#endif // __WXGTK__

   // base class generic event handler
   virtual bool OnMEvent(MEventData& /* data */)
   {
      OnFolderTreeChangeEvent();

      // continue processing the event
      return TRUE;
   }

protected:
   // create the full menu
   void CreateMenu();

   // forget the menu we have
   void ResetMenu()
   {
      if ( m_menu )
      {
         delete m_menu;
         m_menu = NULL;
      }
   }

   // event handler for folder tree change events
   void OnFolderTreeChangeEvent() { ResetMenu(); }

private:
   // CreateMenu() helper
   void AddSubFoldersToMenu(wxString& folderName,
                            MFolder *folder,
                            wxMenu *menu,
                            size_t& id);

   // MEventManager cookie
   void *m_eventCookie;

   // the menu, if NULL it means we have to create it
   wxMenu *m_menu;

   // the names of the folders we have in the menu: array is indexed by the menu
   // item ids offset by WXMENU_POPUP_FOLDER_MENU
   wxArrayString m_folderNames;
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// recursive helper for CreateFolderMenu
void wxFolderMenuData::AddSubFoldersToMenu(wxString& folderName,
                                           MFolder *folder,
                                           wxMenu *menu,
                                           size_t& id)
{
   size_t nSubfolders = folder->GetSubfolderCount();
   for ( size_t n = 0; n < nSubfolders; n++ )
   {
      MFolder_obj subfolder = folder->GetSubfolder(n);
      if ( !subfolder.IsOk() )
      {
         FAIL_MSG( _T("no subfolder?") );

         continue;
      }

      // to allow moving the messages to the parent folder, create an entry
      // for it here (but don't do it for the root folder)
      if ( (n == 0) && (folder->GetType() != MF_ROOT) )
      {
         menu->Append(WXMENU_POPUP_FOLDER_MENU + id++, folder->GetName());
         m_folderNames.Add(folderName);
         menu->AppendSeparator();
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
      else // subfolder has more subfolders, make it a submenu
      {
         wxMenu *submenu = new wxMenu;
         AddSubFoldersToMenu(subfolderName, subfolder, submenu, id);
         menu->Append(WXMENU_POPUP_FOLDER_MENU + id++, name, submenu);
      }

      // in any case, add it to the id->name map
      m_folderNames.Add(subfolderName);
   }
}

void wxFolderMenuData::CreateMenu()
{
   // get the root folder
   MFolder_obj folder("");

   // and add all of its children to the menu recursively
   m_menu = new wxMenu;
   size_t id = 0;
   wxString folderName;
   AddSubFoldersToMenu(folderName, folder, m_menu, id);
}

// ----------------------------------------------------------------------------
// public wxFolderMenu class
// ----------------------------------------------------------------------------

wxFolderMenu::~wxFolderMenu()
{
   delete m_data;
}

wxMenu *wxFolderMenu::GetMenu() const
{
   if ( !m_data )
   {
      // const_cast
      ((wxFolderMenu *)this)->m_data = new wxFolderMenuData;
   }

   return m_data->GetMenu();
}

MFolder *wxFolderMenu::GetFolder(int id) const
{
   CHECK( m_data, NULL, _T("must call wxFolderMenu::GetMenu() first") );

   ASSERT_MSG( id >= WXMENU_POPUP_FOLDER_MENU, _T("bad id in wxFolderMenu::GetFolder") );

   const wxArrayString& names = m_data->GetFolderNames();
   size_t idx = (size_t)(id - WXMENU_POPUP_FOLDER_MENU);
   if ( idx >= names.GetCount() )
   {
      // don't assert - just not our menu item
      return NULL;
   }

   return MFolder::Get(names[idx]);
}

#ifdef __WXGTK__

void wxFolderMenu::Detach()
{
   CHECK_RET( m_data, _T("must call wxFolderMenu::GetMenu() first") );

   m_data->Detach();
}

#endif // __WXGTK__

