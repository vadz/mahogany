/*-*- c++ -*-********************************************************
 * wxIconManager - allocating and deallocating icons for drawing    *
 *                                                                  *
 * (C) 1997-1999 by Karsten Ballüder (Ballueder@usa.net)            *
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

/// a bit lame, but should work in any reasonable case
inline bool IsMimeType(const wxString& str) { return str.Find('/') != -1; }

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

static const int wxTraceIconLoading = 0x000;

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// private data
// ----------------------------------------------------------------------------

/// the path where the last icon was found
wxString wxIconManager::ms_IconPath = "";

/// valid filename extensions for icon files
static const char *wxIconManagerFileExtensions[] =
{ 
   ".xpm", ".png", ".gif", ".jpg", NULL
};

bool wxIconManager::m_knowHandlers = false;
long wxIconManager::m_wxBitmapHandlers[] =
{
   wxBITMAP_TYPE_XPM,  // XPM must be first entry!
   wxBITMAP_TYPE_PNG,  //wxGTK
   wxBITMAP_TYPE_GIF,
   wxBITMAP_TYPE_TIF,
   //wxBITMAP_TYPE_JPEG, //wxGTK optional -- dangerous, do not use
   wxBITMAP_TYPE_BMP,  //wxGTK
   wxBITMAP_TYPE_ANY,
   wxBITMAP_TYPE_CUR, 
   wxBITMAP_TYPE_ICO,  //wxGTK ??
   -1
};

static const char *HandlerNames[]    =
{
   "XPM", "PNG", "GIF", "TIF",
   // "JPG" is dangerous
   "BMP", "ANY", "CUR", "ICO"
};

// ----------------------------------------------------------------------------
// helper functions to load images
// ----------------------------------------------------------------------------

#define   WXICONMANAGER_DEFAULTSIZE 100

wxImage &
wxIconManager::LoadImage(String filename, bool *success)
{
   bool loaded = false;
   wxImage *img = new wxImage();

   // If we haven't been called yet, find out which image handlers
   // are supported:
   if(! m_knowHandlers) // first time initialisation
   {
#ifdef DEBUG
      wxLogDebug("Checking for natively supported image formats:");
      wxString formats;
#endif
      
      for(int i = 0; m_wxBitmapHandlers[i] != -1; i++)
         if(wxImage::FindHandler( m_wxBitmapHandlers[i] ) == NULL)
            m_wxBitmapHandlers[i] = 0; // not available
#ifdef DEBUG
         else
            formats << HandlerNames[i] << ',';
      formats << "XPM";
      wxLogDebug(formats);
#endif
      m_knowHandlers = true;
   }
   
   // suppress any error logging from image handlers, some of them
   // will fail.
   {
      wxLogNull logNo;
      
      for(int i = 0; (!loaded) && m_wxBitmapHandlers[i] != -1; i++)
         if(m_wxBitmapHandlers[i])
            loaded = img->LoadFile(filename, m_wxBitmapHandlers[i]);
   }// normal logging again
#ifdef OS_UNIX
   if(! loaded) // try to use imageMagick to convert image to another format:
   {
      String oldfilename = filename;
      String tempfile = filename;
      int format = READ_APPCONFIG(MP_TMPGFXFORMAT);
      switch(format)
      {
         case 0: // xpm
            tempfile += ".xpm"; break;
         case 1: // png
            tempfile += ".png"; break;
         case 2: // bmp
            tempfile += ".bmp"; break;
         default: // cannot happen
            wxASSERT(0);
      }

      // strip leading path
      int i = tempfile.Length();
      while(i && tempfile.c_str()[i] != '/')
         i--;
      tempfile.assign(tempfile,i+1,tempfile.length()-1-i);
      tempfile = String(
         (getenv("TMP") && strlen(getenv("TMP")))
         ? getenv("TMP"):"/tmp"
         ) + String('/') + tempfile;

      String strConvertProgram = READ_APPCONFIG(MP_CONVERTPROGRAM);
      String strFormatSpec = strutil_extract_formatspec(strConvertProgram);
      if ( strFormatSpec != "ss" )
      {
         wxLogError(_("The setting for image conversion program should include "
                      "exactly two '%%s' format specificators.\n"
                      "The current format specificators are incorrect and "
                      "the default value will be used instead."));

         strConvertProgram = MP_CONVERTPROGRAM_D;
      }
      
      String command;
      command.Printf(strConvertProgram, filename.c_str(), tempfile.c_str());

      wxLogTrace(wxTraceIconLoading,
                 "wxIconManager::LoadImage() calling '%s'...",
                 command.c_str());
      if(system(command) == 0)
      {
         wxLogNull lo; // suppress error messages
         if(format != 0) // not xpm which we handle internally
         {
            if(format == 1) // PNG!
               loaded = img->LoadFile(tempfile, wxBITMAP_TYPE_PNG);
            else // format == 2 BMP
               loaded = img->LoadFile(tempfile, wxBITMAP_TYPE_BMP);
         }
      }
      if(tempfile.length()) // using a temporary file
         wxRemoveFile(tempfile);
   }
#endif // OS_UNIX

   // if everything else failed, try xpm loading:
   if((! loaded) /*&& m_wxBitmapHandlers[0] == 0*/) // try our own XPM loading code
   {
      char ** cpptr = LoadImageXpm(filename);
      if(cpptr)
      {
         *img = wxImage(cpptr);
         wxIconManager::FreeImage(cpptr);
         loaded = true;
      }
   }
   if(success)
      *success = loaded;
   return *img;
}

char **
wxIconManager::LoadImageXpm(String filename)
{
   String tempfile;
   String oldfilename = filename;
   char **cpptr = NULL;

   wxLogTrace(wxTraceIconLoading, "wxIconManager::LoadImage(%s) called...",
              filename.c_str());

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
      tempfile.assign(tempfile,i+1,tempfile.length()-1-i);
      tempfile = String(
         (getenv("TMP") && strlen(getenv("TMP")))
         ? getenv("TMP"):"/tmp"
         ) + String('/') + tempfile;
      String command;
      command.Printf(READ_APPCONFIG(MP_CONVERTPROGRAM),
                     filename.c_str(), tempfile.c_str());
      wxLogTrace(wxTraceIconLoading,
                 "wxIconManager::LoadImage() calling '%s'...",
                 command.c_str());
      if(system(command) == 0)
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
   bool found_xpm = false;
   
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
         if(line == 0 && strstr(str.c_str(),"/* XPM */")) // check whether
            // it's an xpm file
            found_xpm = true;
         if(line > 0 && ! found_xpm)
         {
            free(cpptr);
            return NULL;
         }
         // We only load the actual data, that is, lines starting with 
         // a double quote and ending in a comma:  "data",  --> data
         if(str.length() > 0 && str.c_str()[0] == '"')
         {
            str = str.substr(1,str.length()-1);
            str = strutil_before(str,'"');
            cpptr[line++] = strutil_strdup(str);
         }
      }while(! in.fail());
      cpptr[line++] = NULL;
      if(found_xpm)
      {
         cpptr = (char **)realloc(cpptr,line*sizeof(char *));
         ASSERT(cpptr);
      }
      else
      {
         free(cpptr);
         cpptr = NULL;
      }
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
   String iconName = _iconName;
   String key;

   strutil_tolower(iconName);
   wxLogTrace(wxTraceIconLoading,
              "wxIconManager::GetIcon(%s) called...",
              iconName.c_str());

   // first always look in the cache
   for(i = m_iconList->begin(); i != m_iconList->end(); i++)
   {
      if(strcmp((*i)->iconName.c_str(), iconName.c_str())==0)
      {
        wxLogTrace(wxTraceIconLoading, "... icon was in the cache.");
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
          wxLogTrace(wxTraceIconLoading, "... icon was in the cache.");

          return (*i)->iconRef;
        }
     }

     // not found, now look for iconName without '/':
     key = strutil_before(iconName, '/');
     for(i = m_iconList->begin(); i != m_iconList->end(); i++)
     {
        if(strcmp((*i)->iconName.c_str(), key.c_str())==0) {
          wxLogTrace(wxTraceIconLoading, "... icon was in the cache.");

          return (*i)->iconRef;
        }
     }
   }

   // next step: try to load the icon files .png,.xpm,.gif:
   wxIcon icn;
   int c;
   bool found;
   PathFinder pf(READ_APPCONFIG(MP_ICONPATH), true);
   pf.AddPaths(mApplication->GetLocalDir()+"/icons", true);
   pf.AddPaths(mApplication->GetGlobalDir()+"/icons", true);
   if(ms_IconPath.Length() > 0) pf.AddPaths(ms_IconPath,
                                           true /*prepend */);
   
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
         ms_IconPath = name.BeforeLast('/');
         char **ptr = LoadImageXpm(name);
         if(ptr)
         {
            icn = wxIcon(ptr);
            FreeImage(ptr);
#else
         // Windows:
         ms_IconPath = name.BeforeLast('\\');
         if(icn.LoadFile(Str(name),0))
         {
#endif   
            id = new IconData;
            id->iconRef = icn;
            id->iconName = iconName;
            wxLogTrace(wxTraceIconLoading, "... icon found in '%s'",
                       name.c_str());
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
         wxLogTrace(wxTraceIconLoading, "... icon found in the ressources.");
         return icon;
      }

      // ok, it failed - now do all the usual stuff
   }
#  endif  //Windows

   wxLogTrace(wxTraceIconLoading, "... icon not found.");

   return m_unknownIcon;
}

wxIcon wxIconManager::GetIconFromMimeType(const String& type)
{
   // the order of actions is important: we first try to find "exact" match,
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
      strutil_tolower(id->iconName);
      id->iconRef  = icon;
      m_iconList->push_front(id);
   }
}


