/*-*- c++ -*-********************************************************
 * wxIconManager - allocating and deallocating icons for drawing    *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$ *
 ********************************************************************
 * $Log$
 * Revision 1.10  1998/06/28 19:45:52  KB
 * wxComposeView now sends mail, still a bit buggy, but mainly complete.
 *
 * Revision 1.9  1998/06/22 22:42:32  VZ
 *
 * kbList/CHECK/PY_CALLBACK small changes
 *
 * Revision 1.8  1998/06/05 16:56:23  VZ
 *
 * many changes among which:
 *  1) AppBase class is now the same to MApplication as FrameBase to wxMFrame,
 *     i.e. there is wxMApp inheriting from AppBse and wxApp
 *  2) wxMLogFrame changed (but will probably change again because I wrote a
 *     generic wxLogFrame for wxWin2 - we can as well use it instead)
 *  3) Profile stuff simplified (but still seems to work :-), at least with
 *     wxConfig), no more AppProfile separate class.
 *  4) wxTab "#ifdef USE_WXWINDOWS2"'d out in wxAdbEdit.cc because not only
 *     it doesn't work with wxWin2, but also breaks wxClassInfo::Initialize
 *     Classes
 *  5) wxFTCanvas tweaked and now _almost_ works (but not quite)
 *  6) constraints in wxComposeView changed to work under both wxGTK and
 *     wxMSW (but there is an annoying warning about unsatisfied constraints
 *     coming from I don't know where)
 *  7) some more wxWin2 specific things corrected to avoid (some) crashes.
 *  8) many other minor changes I completely forgot about.
 *
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

#include "Mpch.h"

#ifndef  USE_PCH
#  include  "Mcommon.h"
#  include  "CommonBase.h"   // VAR() macro
#  include  "strutil.h"
#endif

#include "Mdefaults.h"

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
#else   //real XPMs
#  include  "../src/icons/unknown.xpm"
#  include  "../src/icons/txt.xpm"
#  include  "../src/icons/audio.xpm"
#  include  "../src/icons/application.xpm"
#  include  "../src/icons/image.xpm"
#  include  "../src/icons/video.xpm"
#  include  "../src/icons/postscript.xpm"
#  include  "../src/icons/dvi.xpm"
#  include  "../src/icons/hlink.xpm"
#  include  "../src/icons/ftplink.xpm"
#endif  //Win/Unix

#include    "gui/wxIconManager.h"

wxIconManager::wxIconManager()
{
   iconList = GLOBAL_NEW IconDataList;

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
      unknownIcon = new wxIcon(unknown_xpm);
#endif 
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
wxIconManager::GetIcon(String const &iconName)
{
   IconDataList::iterator i;

   for(i = iconList->begin(); i != iconList->end(); i++)
   {
      if(strcmp((*i)->iconName.c_str(), iconName.c_str())==0)
        return (*i)->iconPtr;
   }

   // not found, now look for iconName without '/':
   String key = strutil_before(iconName, '/');
   for(i = iconList->begin(); i != iconList->end(); i++)
   {
      if(strcmp((*i)->iconName.c_str(), key.c_str())==0)
        return (*i)->iconPtr;
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

   iconList->push_front(&id);
}


