/*-*- c++ -*-********************************************************
 * wxIconManager - allocating and deallocating icons for drawing    *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *******************************************************************/

#ifdef __GNUG__
#pragma implementation "wxIconManager.h"
#endif

#include "Mpch.h"

#ifndef  USE_PCH
#  include "Mcommon.h"
#  include "CommonBase.h"   // VAR() macro
#  include "strutil.h"
#  include "gui/wxMApp.h"
#  include "Mdefaults.h"
#  include "Profile.h"
#  include "PathFinder.h"
#  include "MApplication.h"
#endif

#include "gui/wxIconManager.h"

#ifdef    OS_WIN
#  define   unknown_xpm     "unknown"
#  define   txt_xpm         "txt"
#  define   audio_xpm       "audio"
#  define   application_xpm "application"
#  define   image_xpm       "image"
#  define   video_xpm       "video"
#  define   postscript_xpm  "postscript"
#  define   dvi_xpm         "dvi"
#  define   hlink_xpm       "hlink"
#  define   ftplink_xpm     "ftplink"
#  define   MFrame_xpm      "mframe"
#  define   MainFrame_xpm          "MainFrame"
#  define   MainFrame_xpm          "MainFrame"
#  define   tb_exit                "tb_exit"
#  define   tb_help                "tb_help"
#  define   tb_open                "tb_open"
#  define   tb_mail_compose        "tb_mail_compose"
#  define   tb_book_open           "tb_book_open"
#  define   tb_preferences         "tb_preferences"
#else   //real XPMs
#  include  "../src/icons/unknown.xpm"
#  include  "../src/icons/hlink.xpm"
#  include  "../src/icons/ftplink.xpm"
#  include  "../src/icons/MFrame.xpm"
#  include  "../src/icons/MainFrame.xpm"
#endif  //Win/Unix

/// valid filename extensions for icon files
static const char *wxIconManagerFileExtensions[] =
{ 
   ".xpm", ".png", ".gif", "*.jpg", NULL
};


wxIconManager::wxIconManager()
{
   // NB: IconData has a dtor, we must call it ourselves and not let ~kbList
   //     do it (which leads to memory leaks)
   m_iconList = new IconDataList(FALSE);

   AddIcon(M_ICON_HLINK_HTTP, hlink_xpm);
   AddIcon(M_ICON_HLINK_FTP, ftplink_xpm);
   AddIcon("MFrame", MFrame_xpm);
   AddIcon("MainFrame", MainFrame_xpm);
   m_unknownIcon = new wxIcon(unknown_xpm);
}


wxIconManager::~wxIconManager()
{
   IconDataList::iterator i;
   for ( i = m_iconList->begin(); i != m_iconList->end(); i++ ) {
      // against what your common sense may tell you, the icons we manage
      // should *not* be deleted here because wxWindows does it too!
      // now we do!
      IconData *id = *i;
      delete id->iconPtr;
      delete id;
   }

   delete m_iconList;
}

wxBitmap *
wxIconManager::GetBitmap(const String& bmpName)
{
#  ifdef    OS_WIN
   {
      // look in the ressources
      wxBitmap *bmp = new wxBitmap(bmpName);
      if ( bmp->Ok() )
         return bmp;
      else
         delete bmp;

      // try the other standard locations now
   }
#  endif  //Windows

   return GetIcon(bmpName);
}

/*
  We now always return a newly created wxIcon using the copy
  constructor. Such we make sure that the original icon exists
  throughout the lifetime of the application and wxIconManager will
  delete it at program exit.
  The copy constructor doesn't copy but keeps track of reference
  counts for us.
  If a class doesn't delete the icon it requested, this will lead to a 
  wrong reference count but no memory loss as the icon exists all the
  time anyway.
*/

wxIcon *
wxIconManager::GetIcon(String const &_iconName)
{
#  ifdef    OS_WIN
   {
      // first, look in the ressources
      wxIcon *icon = new wxIcon(_iconName);
      if ( icon->Ok() )
         return  icon;
      else
         delete icon;

      // ok, it failed - now do all the usual stuff
   }
#  endif  //Windows

   IconDataList::iterator i;
   String key;
   String iconName = _iconName;
   
   strutil_tolower(iconName);
   wxLogTrace("wxIconManager::GetIcon(%s) called.", iconName.c_str());
   
   for(i = m_iconList->begin(); i != m_iconList->end(); i++)
   {
      if(strcmp((*i)->iconName.c_str(), iconName.c_str())==0)
        return new wxIcon((*i)->iconPtr);
   }

   // not found, now look for MIME subtype, after '/':
   key = strutil_after(iconName, '/');
   for(i = m_iconList->begin(); i != m_iconList->end(); i++)
   {
      if(strcmp((*i)->iconName.c_str(), key.c_str())==0)
        return new wxIcon((*i)->iconPtr);
   }

   // not found, now look for iconName without '/':
   key = strutil_before(iconName, '/');
   for(i = m_iconList->begin(); i != m_iconList->end(); i++)
   {
      if(strcmp((*i)->iconName.c_str(), key.c_str())==0)
        return new wxIcon((*i)->iconPtr);
   }

   // new step: try to load the icon files .png,.xpm,.gif:
   wxIcon *icn = new wxIcon();
   int c;
   bool found;
   PathFinder pf(READ_APPCONFIG(MC_ICONPATH), true);
   pf.AddPaths(mApplication.GetLocalDir()+"/icons", true);
   pf.AddPaths(mApplication.GetGlobalDir()+"/icons", true);

   IconData *id;

   String name;
   for(c = 0; wxIconManagerFileExtensions[c]; c++)
   {
      key = strutil_after(iconName,'/');
      name = key + wxIconManagerFileExtensions[c];
      name = pf.FindFile(name, &found);
      if(! found)
      {
         key  = strutil_before(iconName,'/');
         name = key + wxIconManagerFileExtensions[c];
         name = pf.FindFile(name, &found);
      }
      if(! found)
         continue;
      if(icn->LoadFile(Str(name),0))
      {
         id = new IconData;
         id->iconPtr = icn;
         id->iconName = key;
         m_iconList->push_front(id);
         return new wxIcon(icn);
      }
   }   
   return new wxIcon(m_unknownIcon);
}

void
wxIconManager::AddIcon(String const &iconName,  IconResourceType data)
{
   // load icon
   wxIcon *icon = new wxIcon(data);
   if ( !icon->Ok() ) {
      delete icon;

      return;
   }

   // only loaded icons should be added to the list
   IconData *id = new IconData;

   id->iconName = iconName;
   id->iconPtr  = icon;
   m_iconList->push_front(id);
}


