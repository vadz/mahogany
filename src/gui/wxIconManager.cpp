/*-*- c++ -*-********************************************************
 * wxIconManager - allocating and deallocating icons for drawing    *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *******************************************************************/

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------
#ifdef __GNUG__
#pragma implementation "wxIconManager.h"
#endif

#include "Mpch.h"

#ifndef  USE_PCH
#  include "Mcommon.h"
#  include "strutil.h"
#  include "gui/wxMApp.h"
#  include "Profile.h"
#  include "PathFinder.h"
#  include "MApplication.h"
#endif

#include "Mdefaults.h"

#include "gui/wxIconManager.h"

#include <wx/mimetype.h>

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
#  define   MainFrame_xpm   "MainFrame"
#  define   MainFrame_xpm   "MainFrame"
#  define   tb_exit         "tb_exit"
#  define   tb_help         "tb_help"
#  define   tb_open         "tb_open"
#  define   tb_mail_compose "tb_mail_compose"
#  define   tb_book_open    "tb_book_open"
#  define   tb_preferences  "tb_preferences"
#else   //real XPMs
#  include  "../src/icons/unknown.xpm"
#  include  "../src/icons/hlink.xpm"
#  include  "../src/icons/ftplink.xpm"
#  include  "../src/icons/MFrame.xpm"
#  include  "../src/icons/MainFrame.xpm"
#endif  //Win/Unix

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

/// @@ a bit lame, but should work in any reasonable case
inline bool IsMimeType(const wxString& str) { return str.Find('/') != -1; }

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// private date
// ----------------------------------------------------------------------------

/// valid filename extensions for icon files
static const char *wxIconManagerFileExtensions[] =
{ 
   ".xpm", ".png", ".gif", ".jpg", NULL
};

// ----------------------------------------------------------------------------
// helper functions to load images
// ----------------------------------------------------------------------------

#define   WXICONMANAGER_DEFAULTSIZE 100

char **
wxIconManager::LoadImage(String filename)
{
   String tempfile;
   String oldfilename = filename;
   char **cpptr = NULL;
   
   // lets convert to xpm using image magick:
   if(! wxMatchWild("*.xpm",filename,FALSE))
   {
#ifdef OS_UNIX
      int i;
      tempfile = filename + ".xpm";
      // strip leading path
      i = tempfile.length();
      while(i && tempfile.c_str()[i] != '/')
         i--;
      if(i > 0) i--; // skip the slash
      tempfile.assign(tempfile,i,tempfile.length()-1);
      tempfile << (getenv("TMP") ? getenv("TMP") : "/tmp")
               << '/' << tempfile;
      String command;
      command << "convert " << filename << tempfile;
      system(command); // don't check for returncode, will fail later
      // if it didn't work
      cpptr = LoadXpm(tempfile);
#endif
   }
   if(! cpptr)  // try loading the file itself as an xpm
      cpptr = LoadXpm(filename);
   if(tempfile.length()) // using a temporary file
      wxRemoveFile(tempfile);
   return cpptr;
}

char **
wxIconManager::LoadXpm(String filename)
{
   // this is the actual XPM loading:
   size_t maxlines = WXICONMANAGER_DEFAULTSIZE;
   size_t line = 0;
   
   char **cpptr = (char **) malloc(maxlines * sizeof(char *));
   ASSERT(cpptr);

   ifstream in(filename);
   if(in)
   {  
      String str;
      
      do
      {
         if(line == maxlines)
         {
            maxlines = maxlines + maxlines/2;
            cpptr = (char **) realloc(cpptr,maxlines * sizeof(char *));
            ASSERT(cpptr);
         }
         strutil_getstrline(in,str);
         // We only load the actual data, that is, lines starting with 
         // a double quote and ending in a comma:  "data",  --> data
         if(str.c_str()[0] == '"')
         {
            str = str.substr(1,str.length()-1);
            str = strutil_before(str,'"');
            cpptr[line++] = strutil_strdup(str);
         }
      }while(! in.fail());
      cpptr[line++] = NULL;
      cpptr = (char **)realloc(cpptr,line*sizeof(char *));
      ASSERT(cpptr);
   }
   else
   {
      free(cpptr);
      cpptr = NULL;
   }
   return cpptr;
}

void
wxIconManager::FreeImage(char **cpptr)
{
   char **cpptr2 = cpptr;
   while(*cpptr2)
// broken: compiler bug?      delete[] *(cpptr2++);
   {
      delete [] *cpptr2;
      cpptr2++;
   }
   free(cpptr);
}

// ----------------------------------------------------------------------------
// wxIconManager implementation
// ----------------------------------------------------------------------------
wxIconManager::wxIconManager()
{
   m_iconList = new IconDataList();

   AddIcon(M_ICON_HLINK_HTTP, hlink_xpm);
   AddIcon(M_ICON_HLINK_FTP, ftplink_xpm);
   AddIcon("MFrame", MFrame_xpm);
   AddIcon("MainFrame", MainFrame_xpm);
   m_unknownIcon = wxIcon(unknown_xpm);
}


wxIconManager::~wxIconManager()
{
   delete m_iconList;
}

wxBitmap
wxIconManager::GetBitmap(const String& bmpName)
{
#  ifdef    OS_WIN
   {
      // look in the ressources
      wxBitmap bmp(bmpName);
      if ( bmp.Ok() )
         return bmp;

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

wxIcon 
wxIconManager::GetIcon(String const &_iconName)
{
   IconDataList::iterator i;
   String key;
   String iconName = _iconName;

   strutil_tolower(iconName);
   wxLogTrace("wxIconManager::GetIcon(%s) called...", iconName.c_str());

   // first always look in the cache
   for(i = m_iconList->begin(); i != m_iconList->end(); i++)
   {
      if(strcmp((*i)->iconName.c_str(), iconName.c_str())==0) {
        wxLogTrace("... icon was in the cache.");

        return (*i)->iconRef;
      }
   }

   if ( IsMimeType(iconName) )
   {
     // not found, now look for MIME subtype, after '/':
     key = strutil_after(iconName, '/');
     for(i = m_iconList->begin(); i != m_iconList->end(); i++)
     {
        if(strcmp((*i)->iconName.c_str(), key.c_str())==0) {
          wxLogTrace("... icon was in the cache.");

          return (*i)->iconRef;
        }
     }

     // not found, now look for iconName without '/':
     key = strutil_before(iconName, '/');
     for(i = m_iconList->begin(); i != m_iconList->end(); i++)
     {
        if(strcmp((*i)->iconName.c_str(), key.c_str())==0) {
          wxLogTrace("... icon was in the cache.");

          return (*i)->iconRef;
        }
     }
   }

   // next step: try to load the icon files .png,.xpm,.gif:
   wxIcon icn;
   int c;
   bool found;
   PathFinder pf(READ_APPCONFIG(MC_ICONPATH), true);
   pf.AddPaths(mApplication->GetLocalDir()+"/icons", true);
   pf.AddPaths(mApplication->GetGlobalDir()+"/icons", true);

   IconData *id;

   String name;
   for(c = 0; wxIconManagerFileExtensions[c]; c++)
   {
      name = iconName + wxIconManagerFileExtensions[c];
      name = pf.FindFile(name, &found);

      if ( !found && IsMimeType(iconName) )
      {
         key = strutil_after(iconName,'/');
         name = key + wxIconManagerFileExtensions[c];
         name = pf.FindFile(name, &found);

         if ( !found )
         {
            key  = strutil_before(iconName,'/');
            name = key + wxIconManagerFileExtensions[c];
            name = pf.FindFile(name, &found);
         }
      }

      if( found )
      {
#ifdef   OS_UNIX
         char **ptr = LoadImage(name);
         if(ptr)
         {
            icn = wxIcon(ptr);
            FreeImage(ptr);
#else
         // Windows:
         if(icn.LoadFile(Str(name),0))
         {
#endif   
            id = new IconData;
            id->iconRef = icn;
            id->iconName = key;
            wxLogTrace("... icon found in '%s'", name.c_str());
            m_iconList->push_front(id);
            return icn;
         }
      }
   }

#  ifdef    OS_WIN
   // last, look in the ressources
   {
      wxIcon icon(_iconName);
      if ( icon.Ok() ) {
         wxLogTrace("... icon found in the ressources.");
         return icon;
      }

      // ok, it failed - now do all the usual stuff
   }
#  endif  //Windows

   wxLogTrace("... icon not found.");

   return m_unknownIcon;
}

wxIcon wxIconManager::GetIconFromMimeType(const String& type)
{
   // the order of actions is important: we first try to find "exact" match,#
   // but if we can't, we fall back to a standard icon and look for partial
   // matches only if there is none
   wxIcon icon = GetIcon(type);
   if ( icon == m_unknownIcon )
   {
      wxMimeTypesManager& mimeManager = mApplication->GetMimeManager();
      wxFileType *fileType = mimeManager.GetFileTypeFromMimeType(type);
      if ( fileType != NULL ) {
         fileType->GetIcon(&icon);

         delete fileType;
      }
   }
   if ( icon == m_unknownIcon )
      icon = GetIcon(type.Before('/'));
   if ( icon == m_unknownIcon )
      icon = GetIcon(type.After('/'));

   return icon;
}

void
wxIconManager::AddIcon(String const &iconName,  IconResourceType data)
{
   // load icon
   wxIcon icon(data);
   if ( icon.Ok() )
   {
      // only loaded icons should be added to the list
      IconData *id = new IconData;

      id->iconName = iconName;
      id->iconRef  = icon;
      m_iconList->push_front(id);
   }
}


