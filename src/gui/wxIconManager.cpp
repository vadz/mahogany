/*-*- c++ -*-********************************************************
 * wxIconManager - allocating and deallocating icons for drawing    *
 *                                                                  *
 * (C) 1997, 1998 by Karsten Ballüder (Ballueder@usa.net)           *
 *                                                                  *
 * $Id$         *
 *******************************************************************/
#ifdef __GNUG__
#pragma implementation "wxIconManager.h"
#endif

#include	<CommonBase.h>	//VAR() macro
#include	<Mdefaults.h>
#include	<wxIconManager.h>
#include	<strutil.h>

#include	<unknown.xpm>
#include	<txt.xpm>
#include	<audio.xpm>
#include	<application.xpm>
#include	<image.xpm>
#include	<video.xpm>
#include	<postscript.xpm>
#include	<dvi.xpm>
#include	<hlink.xpm>
#include	<ftplink.xpm>

wxIconManager::wxIconManager()
{
   iconList = NEW list<IconData>;

   //AddIcon("unknown", unknown_xpm);
   AddIcon("TEXT", txt_xpm);
   AddIcon("AUDIO", audio_xpm);
   AddIcon("APPLICATION", application_xpm);
   AddIcon("APPLICATION/POSTSCRIPT", postscript_xpm);
   AddIcon("APPLICATION/DVI", dvi_xpm);
   AddIcon("IMAGE", image_xpm);
   AddIcon("VIDEO", video_xpm);
   AddIcon(M_ICON_HLINK_HTTP, hlink_xpm);
   AddIcon(M_ICON_HLINK_FTP, ftplink_xpm);
   unknownIcon = NEW wxIcon(unknown_xpm);
}


wxIconManager::~wxIconManager()
{
   list<IconData>::iterator i;

   for(i = iconList->begin(); i != iconList->end(); i++)
      DELETE (*i).iconPtr;
   DELETE unknownIcon;
}

wxIcon *
wxIconManager::GetIcon(String const &iconName)
{
   list<IconData>::iterator i;

   for(i = iconList->begin(); i != iconList->end(); i++)
   {
      if(strcmp((*i).iconName.c_str(), iconName.c_str())==0)
	 return (*i).iconPtr;
   }

   // not found, now look for iconName without '/':
   String key = strutil_before(iconName, '/');
   for(i = iconList->begin(); i != iconList->end(); i++)
   {
      if(strcmp((*i).iconName.c_str(), key.c_str())==0)
	 return (*i).iconPtr;
   }
   
   return unknownIcon;
}

void
wxIconManager::AddIcon(String const &iconName,  char *data[])
{
   IconData	id;

   id.iconName = iconName;
   id.iconPtr = NEW wxIcon(data);
   iconList->push_front(id);
}


