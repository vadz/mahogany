/*-*- c++ -*-********************************************************
 * wxIconManager - allocating and deallocating icons for drawing    *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.4  1998/03/26 23:05:41  VZ
 * Necessary changes to make it compile under Windows (VC++ only)
 * Header reorganization to be able to use precompiled headers
 *
 * Revision 1.1  1998/03/14 12:21:22  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifdef __GNUG__
#pragma implementation "wxIconManager.h"
#endif

#include	"Mpch.h"
#include  "Mcommon.h"

#include	"gui/wxIconManager.h"

#ifdef    OS_WIN
  #define   unknown_xpm     "unknown"
  #define   txt_xpm         "txt"
  #define   audio_xpm       "audio"
  #define   application_xpm "application"
  #define   image_xpm       "image"
  #define   video_xpm       "video"
  #define   postscript_xpm  "postscript"
  #define   dvi_xpm         "dvi"
#else   //real XPMs
  #include	"../src/icons/unknown.xpm"
  #include	"../src/icons/txt.xpm"
  #include	"../src/icons/audio.xpm"
  #include	"../src/icons/application.xpm"
  #include	"../src/icons/image.xpm"
  #include	"../src/icons/video.xpm"
  #include	"../src/icons/postscript.xpm"
  #include	"../src/icons/dvi.xpm"
#endif  //Win/Unix

wxIconManager::wxIconManager()
{
   iconList = GLOBAL_NEW std::list<IconData>;

   //AddIcon("unknown", unknown_xpm);
   AddIcon("TEXT", txt_xpm);
   AddIcon("AUDIO", audio_xpm);
   AddIcon("APPLICATION", application_xpm);
   AddIcon("APPLICATION/POSTSCRIPT", postscript_xpm);
   AddIcon("APPLICATION/DVI", dvi_xpm);
   AddIcon("IMAGE", image_xpm);
   AddIcon("VIDEO", video_xpm);

   unknownIcon = GLOBAL_NEW wxIcon(unknown_xpm);
}


wxIconManager::~wxIconManager()
{
   std::list<IconData>::iterator i;

   for(i = iconList->begin(); i != iconList->end(); i++)
      GLOBAL_DELETE (*i).iconPtr;
   GLOBAL_DELETE unknownIcon;
}

wxIcon *
wxIconManager::GetIcon(String const &iconName)
{
   std::list<IconData>::iterator i;

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
wxIconManager::AddIcon(String const &iconName,  IconResourceType data)
{
   IconData	id;

   id.iconName = iconName;

   id.iconPtr = GLOBAL_NEW wxIcon(data);

   iconList->push_front(id);
}


