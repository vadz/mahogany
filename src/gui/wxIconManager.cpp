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
#  include  "Mcommon.h"
#  include  "CommonBase.h"   // VAR() macro
#  include  "strutil.h"
#endif

#include "Mdefaults.h"
#include "Profile.h"
#include "PathFinder.h"
#include "MApplication.h"
#include "wxMApp.h"

#ifdef    OS_WIN
#  define   unknown_xpm     "unknown"
#  define   txt_xpm         "txt"
#  define   audio_xpm       "audio"
#  define   application_xpm "application"
#  define   image_xpm       "image"
#  define   video_xpm       "video"
#  define   postscript_xpm  "postscript"
#  define   dvi_xpm         "dvi"
#  define   hlink_xpm   "hlink"
#  define   ftplink_xpm "ftplink"
#  define   mframe_xpm  "mframe"
#else   //real XPMs
#  include  "../src/icons/unknown.xpm"
#  include  "../src/icons/hlink.xpm"
#  include  "../src/icons/ftplink.xpm"
#  include  "../src/icons/MFrame.xpm"
#endif  //Win/Unix

#include    "gui/wxIconManager.h"


/// valid filename extensions for icon files
static const char *wxIconManagerFileExtensions[] = { ".xpm", ".png", ".gif",
                                           "*.jpg", NULL };


wxIconManager::wxIconManager()
{
   iconList = new IconDataList;

   AddIcon(M_ICON_HLINK_HTTP, hlink_xpm);
   AddIcon(M_ICON_HLINK_FTP, ftplink_xpm);
   AddIcon("MFrame", ftplink_xpm);
   unknownIcon = new wxIcon(unknown_xpm,-1,-1);
}


wxIconManager::~wxIconManager()
{
   IconDataList::iterator i;

   for(i = iconList->begin(); i != iconList->end(); i++)
      if((*i)->iconPtr)
         delete (*i)->iconPtr;
   if(unknownIcon)
      delete unknownIcon;
}

wxIcon *
wxIconManager::GetIcon(String const &_iconName)
{
   IconDataList::iterator i;
   String key;
   String iconName = _iconName;
   
   strutil_tolower(iconName);
#ifdef   DEBUG
   cerr << "wxIconManager::GetIcon() request for: " << iconName << endl;
#endif
   
   for(i = iconList->begin(); i != iconList->end(); i++)
   {
      if(strcmp((*i)->iconName.c_str(), iconName.c_str())==0)
        return (*i)->iconPtr;
   }

   // not found, now look for MIME subtype, after '/':
   key = strutil_after(iconName, '/');
   for(i = iconList->begin(); i != iconList->end(); i++)
   {
      if(strcmp((*i)->iconName.c_str(), key.c_str())==0)
        return (*i)->iconPtr;
   }

   // not found, now look for iconName without '/':
   key = strutil_before(iconName, '/');
   for(i = iconList->begin(); i != iconList->end(); i++)
   {
      if(strcmp((*i)->iconName.c_str(), key.c_str())==0)
        return (*i)->iconPtr;
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
         iconList->push_front(id);
         return icn;
      }
   }   
   return unknownIcon;
}

void
wxIconManager::AddIcon(String const &iconName,  IconResourceType data)
{
   IconData *id = new IconData;

   id->iconName = iconName;
   id->iconPtr = GLOBAL_NEW wxIcon(data,-1,-1);
   iconList->push_front(id);
}


