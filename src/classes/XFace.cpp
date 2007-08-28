/*-*- c++ -*-********************************************************
 * XFace.cc -  a class encapsulating XFace handling                 *
 *                                                                  *
 * (C) 1998-1999 by Karsten Ballüder (ballueder@gmx.net)            *
 *                                                                  *
 * $Id$
 *******************************************************************/

#include "Mpch.h"

#ifndef USE_PCH
#  include "Mcommon.h"
#  include "strutil.h"
#  include "Mdefaults.h"
#  include "MApplication.h"             // for mApplication
#  include "gui/wxIconManager.h"     // for wxIconManager

#  ifdef XFACE_WITH_WXIMAGE
#     include "MApplication.h"
#     include "Profile.h"
#  endif
#endif // USE_PCH

#include "XFace.h"

#ifdef XFACE_WITH_WXIMAGE
#   include "PathFinder.h"
#endif

extern const MOption MP_ICONPATH;

#ifdef HAVE_COMPFACE_H
   extern "C"
   {
#     include  <compface.h>
   };
#else
#  ifdef  CC_MSC
#     pragma message("No compface library found, compiling empty XFace class!")
#  else
#     warning  "No compface library found, compiling empty XFace class!"
#  endif
#endif


XFace::XFace()
{
   initialised  = false;
   data = NULL;
   xface = NULL;
}

bool
XFace::CreateFromData(const char *idata)
{
#ifndef  HAVE_COMPFACE_H
   return false;
#else
   if(data)  delete [] data;
   data = strutil_strdup(idata);
   if(xface) delete [] xface;

   xface = new char[2500];
   strncpy(xface, data, 2500);
   if(compface(xface) < 0)
   {
      delete [] xface;
      delete [] data;
      xface = data = NULL;
      return false;
   }
   //convert it:
   String out = strutil_enforceCRLF(wxString::FromAscii(xface));
   delete [] xface;
   xface = strutil_strdup(out.ToAscii());
   initialised = true;
   return true;
#endif
}

// reads from the data, not from a file in memory, it's different!
bool
XFace::CreateFromXpm(const char *xpmdata)
{
#ifndef  HAVE_COMPFACE_H
   return false;
#else
   if(data)
      delete [] data;

   char
      *buf = strutil_strdup(xpmdata),
      *ptr = buf,
      *token,
      buffer[20],
      zero = 0, one = 0;
   int
      n,i,l;
   long
      value;
   String
      dataString,
      tstr;

   initialised = false;
   do
   {
      token = strutil_strsep(&ptr, "\n\r");
      if(! token)
         break;
      if(zero == 0 || one == 0)
      {
         strncpy(buffer,token+4,8);
         tstr = wxString::FromAscii(buffer);
         strutil_tolower(tstr);

         if(tstr == _T("#000000") || tstr == _T("gray0"))
            zero = token[0];
         else if(tstr == _T("#ffffff")
               || tstr >= _T("gray100")
               || tstr >= _T("white"))
            one = token[0];
      }
      else  // now the data will follow
         break;
   }
   while(token);
   if(! token) // something went wrong
   {
      delete [] buf;
      return false;
   }

   for(l = 0; l < 48; l++)
   {
      for(n = 0; n <= 32; n+= 16)
      {
         value = 0;
         for(i = 0; i < 16; i++)
         {
            if(token[n+i] == one)
               value += 1;
            if(i != 15)
               value <<= 1;
         }
         value = value ^ 0xffff;
         sprintf(buffer,"0x%04lX", value);
         dataString += wxString::FromAscii(buffer);
         dataString += _T(',');
      }
      dataString += _T('\n');
      token = strutil_strsep(&ptr, "\n\r");
      if(l < 47 && ! token)
      {
         delete [] buf;
         return false;
      }
   }
   delete [] buf;
   return CreateFromData(dataString.ToAscii());
#endif
}


#if XFACE_WITH_WXIMAGE

/* static */
wxImage
XFace::GetXFaceImg(const String& filename,
                   bool *hasimg,
                   wxWindow * /* parent */)
{
#if !defined(__CYGWIN__) && !defined(__MINGW32__) // FIXME undefined reference to wxIconManager::LoadImage() when linking
   bool success = false;
   wxImage img;
   if(filename.Length())
   {
      img = wxIconManager::LoadImage(filename, &success);
      if(! success)
      {
         String msg;
         msg.Printf(_("Could not load XFace file '%s'."),
                    filename.c_str());
      }
   }
   if(success)
   {
      if(img.GetWidth() != 48 || img.GetHeight() != 48)
         img = img.Scale(48,48);
      // Now, check if we have some non-B&W colours:
      int intensity;
      for(int y = 0; y < 48; y++)
         for(int x = 0; x < 48; x++)
         {
            intensity = img.GetRed(x,y) + img.GetGreen(x,y) +
               img.GetBlue(x,y);
            if(intensity >= (3*255)/2)
               img.SetRGB(x,y,255,255,255);
            if(intensity <= (3*255)/2)
               img.SetRGB(x,y,0,0,0);
         }
      if(hasimg) *hasimg = true;
   }
   else
   {
      PathFinder pf(READ_APPCONFIG(MP_ICONPATH), true);
      pf.AddPaths(mApplication->GetLocalDir() + DIR_SEPARATOR + _T("icons"), true);
      pf.AddPaths(mApplication->GetDataDir() + DIR_SEPARATOR + _T("icons"), true);
      String name = pf.FindFile(_T("xface.xpm"), &success);
      if(success)
         img = wxIconManager::LoadImage(name, &success);
      if(hasimg) *hasimg = success;
   }
   return img;
#else // CYGWIN
   return NULL;
#endif
}

/* static */
String
XFace::ConvertImgToXFaceData(wxImage &img)
{
   int l, n, i;
   int value;
   String dataString;
   String tmp;

   for(l = 0; l < 48; l++)
   {
      for(n = 0; n <= 32; n+= 16)
      {
         value = 0;
         for(i = 0; i < 16; i++)
         {
            if(img.GetRed(n+i,l) != 0)
               value += 1;
            if(i != 15)
               value <<= 1;
         }
         value = value ^ 0xffff;
         tmp.Printf(_T("0x%04lX"), (unsigned long)value);
         dataString += tmp;
         dataString += _T(',');
      }
      dataString += _T('\n');
   }
   return dataString;
}


bool
XFace::CreateFromFile(const String& filename)
{
   wxImage img = GetXFaceImg(filename);
   String datastring = ConvertImgToXFaceData(img);
   return CreateFromData(datastring.ToAscii());
}


#if 0
/**
   Create an XFace from a wxImage.
   @param image image to read
   @return true on success
*/
bool
XFace::CreateFromImage(wxImage *image)
{
#ifndef  HAVE_COMPFACE_H
   return false;
#else
   if(data)
      delete [] data;

   wxChar
      buffer[20];
   int
      n,i,y;
   long
      value;
   String
      dataString,
      tstr;

   initialised = false;
   for(y = 0; y < 48; y++)
   {
      for(n = 0; n <= 32; n+= 16)
      {
   value = 0;
   for(i = 0; i < 16; i++)
   {
            if(image->GetRed(n+i,y) != 0)  // evaluate red only
         value += 1;
      if(i != 15)
               value <<= 1;
   }
         value = value ^ 0xffff;
   sprintf(buffer,"0x%04lX", value);
   dataString += buffer;
         dataString += _T(',');
      }
      dataString += _T('\n');
   }
   return CreateFromData(dataString);
#endif
}
#endif

#endif // with wxImage

bool
XFace::CreateFromXFace(const char *xfacedata)
{
#ifndef HAVE_COMPFACE_H
   return false;
#else
   if(data) delete [] data;
   if(xface) delete [] xface;
   initialised = false;

   xface = new char [2500];
   strncpy(xface, xfacedata, 2500);
   data = new char [5000];
   strncpy(data, xface, 5000);
   if(uncompface(data) < 0)
   {
      delete [] data;
      delete [] xface;
      data = xface = NULL;
      return false;
   }
   String out = strutil_enforceCRLF(wxString::FromAscii(xface));
   delete [] xface;
   xface = strutil_strdup(out.ToAscii());
   initialised = true;
   return true;
#endif
}

bool
XFace::CreateXpm(String &xpm)
{
#ifndef HAVE_COMPFACE_H
   return false;
#else
   int
      l,c,q;
   char
      *ptr, *buf, *token;

   buf = strutil_strdup(data);
   ptr = buf;

   xpm = wxEmptyString;
   xpm +=
      _T("/* XPM */\n"
      "static char *xface[] = {\n"
      "/* width height num_colors chars_per_pixel */\n"
      "\"    48    48        2            1\",\n"
      "/* colors */\n"
      "\"# c #000000\",\n"
      "\". c #ffffff\",\n");
   for(l = 0; l < 48; l++)
   {
      xpm += '"';
      for(c = 0; c < 3; c++)
      {
         token = strutil_strsep(&ptr,",\n\r");
         if(strlen(token) == 0)
            token = strutil_strsep(&ptr, ",\n\r");  // skip end of line
         if(token)
         {
            token += 2;  // skip  0x
            for(q = 0; q < 4; q++)
            {
               switch(token[q])
               {
                  case '0':
                     xpm += _T("...."); break;
                  case '1':
                     xpm += _T("...#"); break;
                  case '2':
                     xpm += _T("..#."); break;
                  case '3':
                     xpm += _T("..##"); break;
                  case '4':
                     xpm += _T(".#.."); break;
                  case '5':
                     xpm += _T(".#.#"); break;
                  case '6':
                     xpm += _T(".##."); break;
                  case '7':
                     xpm += _T(".###"); break;
                  case '8':
                     xpm += _T("#..."); break;
                  case '9':
                     xpm += _T("#..#"); break;
                  case 'a': case 'A':
                     xpm += _T("#.#."); break;
                  case 'b': case 'B':
                     xpm += _T("#.##"); break;
                  case 'c': case 'C':
                     xpm += _T("##.."); break;
                  case 'd': case 'D':
                     xpm += _T("##.#"); break;
                  case 'e': case 'E':
                     xpm += _T("###."); break;
                  case 'f': case 'F':
                     xpm += _T("####"); break;
                  default:
                     break;
               }
            }

         }
      }
      xpm += _T('"');
      if(l < 47)
         xpm += _T(",\n");
      else
         xpm += _T("\n};\n");
   }
   return true;
#endif
}

bool
XFace::CreateXpm(char ***xpm)
{
#ifndef HAVE_COMPFACE_H
   return false;
#else
   int
      l,c,q;
   char
      *ptr, *buf, *token;
   int
      line = 0;
   String
      tmp;

   *xpm = (char **) malloc(sizeof(char *)*52);

   buf = strutil_strdup(data);
   ptr = buf;

   (*xpm)[line++] = strutil_strdup(" 48 48 2 1");
   (*xpm)[line++] = strutil_strdup("# c #000000");
   (*xpm)[line++] = strutil_strdup(". c #ffffff");
   for(l = 0; l < 48; l++)
   {
      tmp = wxEmptyString;
      for(c = 0; c < 3; c++)
      {
         token = strutil_strsep(&ptr,",\n\r");
         if(strlen(token) == 0)
            token = strutil_strsep(&ptr, ",\n\r");  // skip end of line
         if(token)
         {
            token += 2;  // skip  0x
            for(q = 0; q < 4; q++)
            {
               switch(token[q])
               {
                  case '0':
                     tmp += _T("...."); break;
                  case '1':
                     tmp += _T("...#"); break;
                  case '2':
                     tmp += _T("..#."); break;
                  case '3':
                     tmp += _T("..##"); break;
                  case '4':
                     tmp += _T(".#.."); break;
                  case '5':
                     tmp += _T(".#.#"); break;
                  case '6':
                     tmp += _T(".##."); break;
                  case '7':
                     tmp += _T(".###"); break;
                  case '8':
                     tmp += _T("#..."); break;
                  case '9':
                     tmp += _T("#..#"); break;
                  case 'a': case 'A':
                     tmp += _T("#.#."); break;
                  case 'b': case 'B':
                     tmp += _T("#.##"); break;
                  case 'c': case 'C':
                     tmp += _T("##.."); break;
                  case 'd': case 'D':
                     tmp += _T("##.#"); break;
                  case 'e': case 'E':
                     tmp += _T("###."); break;
                  case 'f': case 'F':
                     tmp += _T("####"); break;
                  default:
                     break;
               }
            }

         }
      }
      (*xpm)[line++] = strutil_strdup(tmp.ToAscii());
   }
   delete [] buf;
   (*xpm)[line++] = NULL;
   return true;
#endif
}

String
XFace::GetHeaderLine(void) const
{
   if(xface)
      return wxString::FromAscii(xface);
   else
      return wxEmptyString;
}

XFace::~XFace()
{
   delete[] data;
   delete[] xface;
   initialised = false;
}


