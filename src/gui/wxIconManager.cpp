/*-*- c++ -*-********************************************************
 * wxIconManager - allocating and deallocating icons for drawing    *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.7  1998/05/11 20:57:31  VZ
 * compiles again under Windows + new compile option USE_WXCONFIG
 *
 * Revision 1.6  1998/04/30 19:12:35  VZ
 * (minor) changes needed to make it compile with wxGTK
 *
 * Revision 1.5  1998/04/22 19:56:32  KB
 * Fixed _lots_ of problems introduced by Vadim's efforts to introduce
 * precompiled headers. Compiles and runs again under Linux/wxXt. Header
 * organisation is not optimal yet and needs further
 * cleanup. Reintroduced some older fixes which apparently got lost
 * before.
 *
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

#ifndef	USE_PCH
#	include  "Mcommon.h"
#	include	<CommonBase.h> // vAR() macro
#	include	<Mdefaults.h>
#	include	"gui/wxIconManager.h"
#	include "strutil.h"
#endif

#ifdef    OS_WIN
#	define   unknown_xpm     "unknown"
#	define   txt_xpm         "txt"
#	define   audio_xpm       "audio"
#	define   application_xpm "application"
#	define   image_xpm       "image"
#	define   video_xpm       "video"
#	define   postscript_xpm  "postscript"
#	define   dvi_xpm         "dvi"
#	define   hlink_xpm	"hlink"
#	define   ftplink_xpm	"ftplink"
#else   //real XPMs
#	include  "../src/icons/unknown.xpm"
#	include  "../src/icons/txt.xpm"
#	include  "../src/icons/audio.xpm"
#	include  "../src/icons/application.xpm"
#	include  "../src/icons/image.xpm"
#	include  "../src/icons/video.xpm"
#	include  "../src/icons/postscript.xpm"
#	include  "../src/icons/dvi.xpm"
#	include  "../src/icons/hlink.xpm"
#	include  "../src/icons/ftplink.xpm"
#endif  //Win/Unix

#include    "gui/wxIconManager.h"

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
   AddIcon(M_ICON_HLINK_HTTP, hlink_xpm);
   AddIcon(M_ICON_HLINK_FTP, ftplink_xpm);

   #if  USE_WXGTK
      // @@@@ no appropriate ctor in wxGTK
      unknownIcon = NULL;
   #else
      unknownIcon = GLOBAL_NEW wxIcon(unknown_xpm);
   #endif 
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
   IconData id;

   id.iconName = iconName;

   #if  USE_WXGTK
     // @@@@ no appropriate ctor in wxGTK
     id.iconPtr = NULL;
   #else
     id.iconPtr = GLOBAL_NEW wxIcon(data);
   #endif

   iconList->push_front(id);
}


