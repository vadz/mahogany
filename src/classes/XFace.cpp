/*-*- c++ -*-********************************************************
 * XFace.cc -  a class encapsulating XFace handling                 *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                 *
 *******************************************************************/

#ifdef __GNUG__
#pragma implementation "XFace.h"
#endif

#include	"Mpch.h"
#include	"Mcommon.h"

#include	"XFace.h"
#include	"strutil.h"
#include        "kbList.h"
#include        "gui/wxIconManager.h"
#include	<stdio.h>

#ifdef HAVE_COMPFACE_H
extern "C" {
#include	<compface.h>
	   };
#else
#   ifdef  CC_MSC
#      pragma message("No compface library found, compiling empty XFace class!")
#   else
#      warning	"No compface library found, compiling empty XFace class!"
#   endif
#endif



XFace::XFace()
{
   initialised  = false;
   data = NULL;
   xface = NULL;
}

bool
XFace::CreateFromXpm(const char *xpmdata)
{
#ifndef	HAVE_COMPFACE_H
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
      dataString;

   initialised = false;
   do
   {
      token = strsep(&ptr, "\n\r");
      if(! token)
	 break;
      if(token[0] == '/' && token[1] == '*')
	 continue;	// ignore comments
      if(zero == 0 || one == 0)
      {
	 if(strncmp(token+5,"#000000",7) == 0)
	 zero = token[1];
	 else if(strncmp(token+5,"#ffffff",7) == 0
		 || strncmp(token+5,"#FFFFFF",7) == 0)
	    one = token[1];
      }
      else	// now the data will follow
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
	    if(token[n+i+1] == one)
	       value += 1;
	    value <<= 1;
	 }
	 sprintf(buffer,"%lx", value);
	 dataString += buffer;
	 dataString += ',';
      }
      dataString += '\n';
      token = strsep(&ptr, "\n\r");
      if(! token)
      {
	 delete [] buf;
	 return false;
      }
   }
   delete [] buf;
   if(data)  delete [] data;
   data = strutil_strdup(dataString);
   if(xface) delete [] xface;
   xface = new char[2500];
   strcpy(xface, data);

#if  !USE_WXGTK
     if(compface(xface) < 0)
     {
        delete [] xface;
        delete [] data;
        xface = data = NULL;
        return false;
     }
#endif // wxGTK
   initialised = true;
   return true;
#endif
}


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
   
   xpm = "";
   xpm +=
      "/* XPM */\n"
      "static char *xface[] = {\n"
      "/* width height num_colors chars_per_pixel */\n"
      "\"    48    48        2            1\",\n"
      "/* colors */\n"
      "\"# c #000000\",\n"
      "\". c #ffffff\",\n";
   for(l = 0; l < 48; l++)
   {
      xpm += '"';
      for(c = 0; c < 3; c++)
      {
	 token = strsep(&ptr,",\n\r");
	 if(strlen(token) == 0)
	    token = strsep(&ptr, ",\n\r");	// skip end of line
	 if(token)
	 {
	    token += 2;	// skip  0x
	    for(q = 0; q < 4; q++)
	    {
	       switch(token[q])
	       {
	       case '0':
		  xpm += "...."; break;
	       case '1':
		  xpm += "...#"; break;
	       case '2':
		  xpm += "..#."; break;
	       case '3':
		  xpm += "..##"; break;
	       case '4':
		  xpm += ".#.."; break;
	       case '5':
		  xpm += ".#.#"; break;
	       case '6':
		  xpm += ".##."; break;
	       case '7':
		  xpm += ".###"; break;
	       case '8':
		  xpm += "#..."; break;
	       case '9':
		  xpm += "#..#"; break;
	       case 'a': case 'A':
		  xpm += "#.#."; break;
	       case 'b': case 'B':
		  xpm += "#.##"; break;
	       case 'c': case 'C':
		  xpm += "##.."; break;
	       case 'd': case 'D':
		  xpm += "##.#"; break;
	       case 'e': case 'E':
		  xpm += "###."; break;
	       case 'f': case 'F':
		  xpm += "####"; break;
	       default:
		  break;
	       }
	    }
	    
	 }
      }
      xpm += '"';
      if(l < 47)
	 xpm += ",\n";
      else
	 xpm += "\n};\n";
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
   
   *xpm = (char **) malloc(sizeof(char *)*51);
   
   buf = strutil_strdup(data);
   ptr = buf;
   
   (*xpm)[line++] = strutil_strdup(" 48 48 2 1");
   (*xpm)[line++] = strutil_strdup("# c #000000");
   (*xpm)[line++] = strutil_strdup(". c #ffffff");
   for(l = 0; l < 48; l++)
   {
      tmp = "";
      for(c = 0; c < 3; c++)
      {
	 token = strsep(&ptr,",\n\r");
	 if(strlen(token) == 0)
	    token = strsep(&ptr, ",\n\r");	// skip end of line
	 if(token)
	 {
	    token += 2;	// skip  0x
	    for(q = 0; q < 4; q++)
	    {
	       switch(token[q])
	       {
	       case '0':
		  tmp += "...."; break;
	       case '1':
		  tmp += "...#"; break;
	       case '2':
		  tmp += "..#."; break;
	       case '3':
		  tmp += "..##"; break;
	       case '4':
		  tmp += ".#.."; break;
	       case '5':
		  tmp += ".#.#"; break;
	       case '6':
		  tmp += ".##."; break;
	       case '7':
		  tmp += ".###"; break;
	       case '8':
		  tmp += "#..."; break;
	       case '9':
		  tmp += "#..#"; break;
	       case 'a': case 'A':
		  tmp += "#.#."; break;
	       case 'b': case 'B':
		  tmp += "#.##"; break;
	       case 'c': case 'C':
		  tmp += "##.."; break;
	       case 'd': case 'D':
		  tmp += "##.#"; break;
	       case 'e': case 'E':
		  tmp += "###."; break;
	       case 'f': case 'F':
		  tmp += "####"; break;
	       default:
		  break;
	       }
	    }
	    
	 }
      }
      (*xpm)[line++] = strutil_strdup(tmp);
   }
   return true;
#endif
}

String
XFace::GetHeaderLine(void) const
{
   String header = "X-Face: ";
   if(xface)
      return header + xface;
   else
      return "";
}
   
XFace::~XFace()
{
   if(data)	delete[] data;
   if(xface)	delete[] xface;
   initialised = false;
}


