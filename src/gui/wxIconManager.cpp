/*-*- c++ -*-********************************************************
 * wxIconManager - allocating and deallocating icons for drawing    *
 *                                                                  *
 * (C) 1997-1999 by Karsten Ballüder (ballueder@gmx.net)            *
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
#   pragma implementation "wxIconManager.h"
#endif

#include "Mpch.h"

#ifndef  USE_PCH
#  include "Mcommon.h"
#  include "MApplication.h"
#  include "Profile.h"
#  include "strutil.h"
#  include "gui/wxIconManager.h"
#  include "Mdefaults.h"
#endif // USE_PCH

#if wxCHECK_VERSION(2, 5, 0)
#  include <wx/iconloc.h>
#endif

#include "gui/wxMDialogs.h"
#include "PathFinder.h"         // for PathFinder

#include <wx/mimetype.h>
#include <wx/file.h>
#include <wx/filename.h>

#ifdef USE_ICONS_FROM_RESOURCES
#  define   unknown_xpm     "unknown"
#  define   MFrame_xpm      "mframe"
#  define   MainFrame_xpm   "MainFrame"
#else   //real XPMs
#  include  "../src/icons/unknown.xpm"
#  include  "../src/icons/MFrame.xpm"
#  include  "../src/icons/MainFrame.xpm"
#endif  //Win/Unix

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

/// a bit lame, but should work in any reasonable case
inline bool IsMimeType(const wxString& str) { return str.Find('/') != -1; }

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_CONVERTPROGRAM;
extern const MOption MP_ICONPATH;
extern const MOption MP_TMPGFXFORMAT;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

static const wxChar *wxTraceIconLoading = _T("iconload");

/** @name built-in icon names */
//@{
/// for hyperlinks/http
#define   M_ICON_HLINK_HTTP   "M-HTTPLINK"
/// for hyperlinks/ftp
#define   M_ICON_HLINK_FTP    "M-FTPLINK"
/// unknown icon
#define   M_ICON_UNKNOWN      "UNKNOWN"
//@}

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// private data
// ----------------------------------------------------------------------------

/// the path where the last icon was found
wxString wxIconManager::ms_IconPath = wxEmptyString;

/// valid filename extensions for icon files
static const wxChar *wxIconManagerFileExtensions[] =
{
   _T(".xpm"), _T(".png"), _T(".bmp"), _T(".jpg"), _T(".gif"), _T(".pcx"), _T(".pnm"), NULL
};

/// how many image handlers do we have
int wxIconManager::ms_NumOfHandlers = 0;

// how many formats/extensions stored in the array above?
#define NUMBER_OF_FORMATS 4

bool wxIconManager::m_knowHandlers = false;
long wxIconManager::m_wxBitmapHandlers[] =
{
   wxBITMAP_TYPE_XPM,  // XPM must be first entry!
   wxBITMAP_TYPE_PNG,  //wxGTK
   wxBITMAP_TYPE_BMP,  //wxGTK
   wxBITMAP_TYPE_JPEG, //wxGTK optional
   wxBITMAP_TYPE_GIF,
   wxBITMAP_TYPE_PNM,
   wxBITMAP_TYPE_PCX,
   wxBITMAP_TYPE_TIF,
   wxBITMAP_TYPE_ANY,
   wxBITMAP_TYPE_CUR,
   wxBITMAP_TYPE_ICO,  //wxGTK ??
   -1
};

static const wxChar *HandlerNames[]    =
{
   _T("xpm"), _T("png"), _T("bmp"), _T("jpg"),
   _T("gif"),
   _T("pnm"), _T("pcx"),
   _T("tif"),
   _T("any"), _T("cur"), _T("ico")
};

// ----------------------------------------------------------------------------
// static helper functions to load images
// ----------------------------------------------------------------------------

#define   WXICONMANAGER_DEFAULTSIZE 100

/* static */
wxImage &
wxIconManager::LoadImage(String filename, bool *success, bool showDlg)
{
   bool loaded = false;
   wxImage *img = new wxImage();

   // If we haven't been called yet, find out which image handlers
   // are supported:
   if(! m_knowHandlers) // first time initialisation
   {
      ms_NumOfHandlers = 0;
      for(int i = 0; m_wxBitmapHandlers[i] != -1; i++)
         if(wxImage::FindHandler( m_wxBitmapHandlers[i] ) == NULL)
            m_wxBitmapHandlers[i] = 0; // not available
         else
         {
            ms_NumOfHandlers ++;
         }

      m_knowHandlers = true;
   }

   MProgressDialog *pdlg = NULL;
   int   step = 0;
   if(showDlg)
   {
      pdlg = new MProgressDialog(_("Please wait"), _("Loading image..."),
                                 ms_NumOfHandlers+3);
   }
   // suppress any error logging from image handlers, some of them
   // will fail.
   {
      wxLogNull logNo;

      for(int i = 0; (!loaded) && m_wxBitmapHandlers[i] != -1; i++)
         if(m_wxBitmapHandlers[i])
         {
            loaded = img->LoadFile(filename, m_wxBitmapHandlers[i]);
            if(pdlg)
            {
               if(!pdlg->Update(++step))
               {
                  if(success) *success = loaded;
                  delete pdlg;
                  return *img;
               }
            }
         }
   }// normal logging again
#ifdef OS_UNIX
   if(! loaded) // try to use imageMagick to convert image to another format:
   {
      String oldfilename = filename;
      String tempfile = filename;
      int format = READ_APPCONFIG(MP_TMPGFXFORMAT);
      if((format < 0 || format > NUMBER_OF_FORMATS)
         || (format != 0 && m_wxBitmapHandlers[format] == 0)) //xpm we do ourselves
      {
         wxLogInfo(_("Unsupported intermediary image format '%s' specified,\n"
                     "reset to '%s'."),
                   ((format < 0 || format >NUMBER_OF_FORMATS) ?
                    _T("unknown") : HandlerNames[format]),
                   HandlerNames[0]);
         format = 0;
         mApplication->GetProfile()->writeEntry(MP_TMPGFXFORMAT,format);
      }
      tempfile += wxIconManagerFileExtensions[format];
      // strip leading path
      int i = tempfile.Length();
      while(i && tempfile.c_str()[i] != '/')
         i--;
      tempfile.assign(tempfile,i+1,tempfile.length()-1-i);
      tempfile = String(
         (wxGetenv(_T("TMP")) && wxStrlen(wxGetenv(_T("TMP"))))
         ? wxGetenv(_T("TMP")) : _T("/tmp")
         ) + _T('/') + tempfile;
      if(wxFile::Exists(filename))
      {
         String strConvertProgram = READ_APPCONFIG(MP_CONVERTPROGRAM);
         String strFormatSpec = strutil_extract_formatspec(strConvertProgram);
         if ( strFormatSpec != _T("ss") )
         {
            wxLogError(_("The setting for image conversion program should include "
                         "exactly two '%%s' format specificators.\n"
                         "The current setting '%s' is incorrect and "
                         "the default value will be used instead."),
                       strConvertProgram.c_str());
            strConvertProgram = GetStringDefault(MP_CONVERTPROGRAM);
         }
         String command;
         command.Printf(strConvertProgram, filename.c_str(), tempfile.c_str());
         wxLogTrace(wxTraceIconLoading,
                    _T("wxIconManager::LoadImage() calling '%s'..."),
                    command.c_str());
         if(wxSystem(command) == 0)
         {
            if(pdlg)
            {
               if(!pdlg->Update(++step))
               {
                  if(success) *success = false;
                  delete pdlg;
                  return *img;
               }
            }
            wxLogNull lo; // suppress error messages
            if(format != 0) // not xpm which we handle internally
            {
               switch(format)
               {
               case 1: // PNG!
                  loaded = img->LoadFile(tempfile, wxBITMAP_TYPE_PNG);
                  break;
               case 2: // 2 BMP
                  loaded = img->LoadFile(tempfile, wxBITMAP_TYPE_BMP);
                  break;
               case 3: // 3 JPG
                  loaded = img->LoadFile(tempfile, wxBITMAP_TYPE_JPEG);
                  break;
               }
            }
         } // system()
         if(tempfile.length()) // using a temporary file
            wxRemoveFile(tempfile);
      }// if(wxFile::Exists())
      if(pdlg)
      {
         if(!pdlg->Update(++step))
         {
            if(success) *success = loaded;
            delete pdlg;
            return *img;
         }
      }
   }//! loaded
#endif // OS_UNIX

   // if everything else failed, try xpm loading:
   if((! loaded) /*&& m_wxBitmapHandlers[0] == 0*/) // try our own XPM loading code
   {
      char ** cpptr = LoadImageXpm(filename);
      if(pdlg)
         pdlg->Update(++step); // ignore break here
      if(cpptr)
      {
         *img = wxBitmap(cpptr).ConvertToImage();
         wxIconManager::FreeImage(cpptr);
         loaded = true;
      }
   }
   if(success)
      *success = loaded;
   if(pdlg) delete pdlg;
   return *img;
}

/* static */
char **
wxIconManager::LoadImageXpm(String filename)
{
   char **cpptr = NULL;

   wxLogTrace(wxTraceIconLoading, _T("wxIconManager::LoadImage(%s) called..."),
              filename.c_str());

   wxFileName fn(filename);
   if ( fn.GetExt() == _T("xpm") )
   {
      // try loading the file itself as an xpm
      cpptr = LoadXpm(filename);
   }
#ifdef OS_UNIX
   else // lets convert to xpm using image magick:
   {
      wxFileName fnXPM;
      fnXPM.AssignTempFileName(_T("Mimg"));
      fnXPM.SetExt(_T("xpm"));

      String tempfile(fnXPM.GetFullPath());
      String command;
      command.Printf(READ_APPCONFIG_TEXT(MP_CONVERTPROGRAM),
                     filename.c_str(), tempfile.c_str());
      wxLogTrace(wxTraceIconLoading,
                 _T("wxIconManager::LoadImage() calling '%s'..."),
                 command.c_str());
      if(wxSystem(command) == 0)
         cpptr = LoadXpm(tempfile);

      wxRemoveFile(tempfile);
   }
#endif // OS_UNIX

   return cpptr;
}

/* static */
char **
wxIconManager::LoadXpm(String filename)
{
   // this is the actual XPM loading:
   size_t maxlines = WXICONMANAGER_DEFAULTSIZE;
   size_t line = 0;

   char **cpptr = (char **) malloc(maxlines * sizeof(char *));
   ASSERT(cpptr);
   bool found_xpm = false;

   ifstream in(filename.fn_str());
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
         if(line == 0 && wxStrstr(str.c_str(), _T("/* XPM */"))) // check whether
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
            cpptr[line++] = strutil_strdup(wxConvertWX2MB(str));
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

/* static */
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
wxIconManager::wxIconManager(wxString sub_dir)
{
   m_iconList = new IconDataList;

   const String localDir(mApplication->GetLocalDir());

   m_GlobalDir = mApplication->GetDataDir();

   // look in the source directory if there is no global one
#ifdef M_TOP_SOURCEDIR
   if ( m_GlobalDir == localDir )
   {
      m_GlobalDir.clear();
      m_GlobalDir << M_TOP_SOURCEDIR << DIR_SEPARATOR << _T("src");
   }
#endif // defined(M_TOP_SOURCEDIR)

   m_GlobalDir << DIR_SEPARATOR << _T("icons");

   m_LocalDir << localDir << DIR_SEPARATOR << _T("icons");

   if(sub_dir == _T("default") || sub_dir == _("default"))
      sub_dir.clear();
   SetSubDirectory(sub_dir);

   m_unknownIcon = wxIcon(unknown_xpm);
}


wxIconManager::~wxIconManager()
{
   delete m_iconList;
}

void
wxIconManager::SetSubDirectory(wxString subDir)
{
   /* If nothing changed, we don't do anything: */
   if ( subDir != m_SubDir )
   {
      /* If we change the directory, we should also discard all old icons
         to get the maximum effect. */
      delete m_iconList;
      m_iconList = new IconDataList();

      if ( subDir.empty() )
      {
         m_SubDir.clear();
      }
      else
      {
         m_SubDir = DIR_SEPARATOR + subDir;

         if ( !wxDirExists(m_GlobalDir + m_SubDir) &&
               !wxDirExists(m_LocalDir + m_SubDir) )
         {
            // save ourselves some time when searching - we don't risk to find
            // anything in non-existent directories anyhow
            m_SubDir.clear();
         }
      }

      // Always add the built-in icons:
      //
      // VZ: they're unused right now and I don't know when are we supposed
      //     to use them?
#if 0
      AddIcon(M_ICON_HLINK_HTTP, hlink_xpm);
      AddIcon(M_ICON_HLINK_FTP, ftplink_xpm);
#endif // 0

      // frame icons
      AddIcon(_T("MFrame"), MFrame_xpm);
      AddIcon(_T("MainFrame"), MainFrame_xpm);
   }
}

wxBitmap
wxIconManager::GetBitmap(const String& bmpName)
{
   // note that this one always looks in the resources first, regardless of the
   // value of USE_ICONS_FROM_RESOURCES
#  ifdef OS_WIN
   {
      // suppress the log messages from bitmap ctor which may fail
      wxLogNull nolog;

      // look in the ressources
      wxBitmap bmp(bmpName);
      if ( bmp.Ok() )
         return bmp;

      // try the other standard locations now
   }
#  endif  //Windows

   return GetIcon(bmpName);
}

bool wxIconManager::FindInCache(const String& iconName, wxIcon *icon) const
{
   IconDataList::iterator i;
   for ( i = m_iconList->begin(); i != m_iconList->end(); i++ )
   {
      if (wxStrcmp((*i)->iconName.c_str(), iconName.c_str()) == 0 )
      {
         wxLogTrace(wxTraceIconLoading, _T("... icon was in the cache."));
         *icon = (*i)->iconRef;

         return true;
      }
   }

   return false;
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
wxIconManager::GetIcon(const String &iconNameOrig)
{
   String iconName = iconNameOrig;

   strutil_tolower(iconName);
   wxLogTrace(wxTraceIconLoading, _T("wxIconManager::GetIcon(%s) called..."),
              iconNameOrig.c_str());

   wxIcon icon;

   // first always look in the cache
   if ( FindInCache(iconName, &icon) )
      return icon;

   // next step: try to load the icon files .png,.xpm,.gif:
   if(m_GlobalDir.Length())
   {
      PathFinder pf(READ_APPCONFIG(MP_ICONPATH));

#ifdef M_TOP_SOURCEDIR
      // look in the source directory to make it possible to use the program
      // without installing it
      pf.AddPaths(String(M_TOP_SOURCEDIR) + _T("/src/icons"));
      pf.AddPaths(String(M_TOP_SOURCEDIR) + _T("/res"));
#endif // M_TOP_SOURCEDIR

      pf.AddPaths(m_GlobalDir, false);
      if(ms_IconPath.Length() > 0)
         pf.AddPaths(ms_IconPath,false, true /*prepend */);
      pf.AddPaths(m_LocalDir, false);
      if(m_SubDir.Length() > 1)  // 1 == "/" == empty
      {
         pf.AddPaths(m_GlobalDir+m_SubDir, false, true);
         pf.AddPaths(m_LocalDir+m_SubDir, false, true);
      }

      IconData *id;

      String name;
      for ( int ext = 0; wxIconManagerFileExtensions[ext]; ext++ )
      {
         // use iconNameOrig here to preserve the original case
         name = pf.FindFile(iconNameOrig + wxIconManagerFileExtensions[ext]);

         // but if it's not found, also fall back to the usual lower case
         if ( name.empty() )
         {
            name = pf.FindFile(iconName + wxIconManagerFileExtensions[ext]);
         }

         if ( !name.empty() )
         {
            ms_IconPath = name.BeforeLast('/');

            if ( !icon.LoadFile(name, wxBITMAP_TYPE_ANY) )
            {
               // try to load it via conversion to XPM
               char **ptr = LoadImageXpm(name);
               if(ptr)
               {
                  icon = wxIcon(ptr);
                  FreeImage(ptr);
               }
            }

            if ( icon.Ok() )
            {
               id = new IconData;
               id->iconRef = icon;
               id->iconName = iconName;
               wxLogTrace(wxTraceIconLoading, _T("... icon found in '%s'"),
                          name.c_str());
               m_iconList->push_front(id);
               return icon;
            }
         }
     } // for all extensions
   } // if globaldir is not empty

#ifdef OS_WIN
   // last, look in the resources
   {
      wxIcon icon(iconNameOrig);
      if ( icon.Ok() ) {
         wxLogTrace(wxTraceIconLoading, _T("... icon found in the ressources."));
         return icon;
      }

      // ok, it failed - now do all the usual stuff
   }
#endif  //Windows

   wxLogTrace(wxTraceIconLoading, _T("... icon not found."));

   return m_unknownIcon;
}


wxIcon
wxIconManager::GetIconFromMimeType(const String& type, const String& ext)
{
   wxIcon icon;

   // use the system icons by default
   wxMimeTypesManager& mimeManager = mApplication->GetMimeManager();
   wxFileType *fileType = mimeManager.GetFileTypeFromMimeType(type);
   if ( !fileType )
   {
      fileType = mimeManager.GetFileTypeFromExtension(ext);
   }

   if ( fileType )
   {
#ifdef wxHAS_ICON_LOCATION
      wxIconLocation iconLoc;
      if ( fileType->GetIcon(&iconLoc) )
      {
         wxLogNull noLog;

         icon = wxIcon(iconLoc);
      }
#else // wx 2.4.x or very early 2.5.0
      (void)fileType->GetIcon(&icon);
#endif

      delete fileType;
   }

   if ( icon.Ok() )
      return icon;


   // now try to find it by name
   icon = GetIcon(type);

#if wxCHECK_VERSION(2, 7, 2)
   if ( icon.IsSameAs(m_unknownIcon) )
#else
   if ( icon == m_unknownIcon )
#endif
   {
      // the generic icon for this class of things
      String primType = type.BeforeLast(_T('/'));
      if ( !primType.empty() )
         icon = GetIcon(primType);
   }

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


